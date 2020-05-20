#include <common.h>
//#define _DEBUG
typedef struct 
{
  char *name;//锁名
  int lockid;//锁的序号
  intptr_t locked;//锁控制
  int holder;//锁的持有者
}lock_t;

struct block//管理空闲块或非空闲块的数据结构
{
uintptr_t start,end;//管理[start,end)
uint32_t size;
struct block* prev;
struct block* next;
};

int lock_num=0;
lock_t glb_lock;//管理两个链表的锁
lock_t alloc_lock;//管理balloc和bfree并发性的锁
lock_t print_lock;//printf的锁,保证完整性

static void *balloc();
static void bfree(struct block* blk);
struct block* free_head;
struct block* alloc_head;//两个都是空的节点
void sp_lock(lock_t* lk);
void sp_unlock(lock_t* lk);

void sp_lockinit(lock_t* lk,const char *name,int id)
{
  strcpy(lk->name,name);
  lk->locked=0;
  lk->lockid=id;
}

void sp_lock(lock_t* lk)
{
  while(_atomic_xchg(&lk->locked,1))
  { 
   // sp_lock(&print_lock,0);
    printf("CPU#%d Acquiring lock  %s\n",_cpu(),lk->name);
   // sp_unlock(&print_lock,0);
  }
  lk->holder=_cpu();
  printf("CPU#%d holding lock %s\n",lk->holder,lk->name);
}
void sp_unlock(lock_t *lk)
{
  _atomic_xchg(&lk->locked,0);
  printf("CPU#%d Releasing lock  %s\n",_cpu(),lk->name);
  lk->holder=10000;//表示无holder
}
/*
void sp_lock(lock_t* lk,int log)
{
  while(_atomic_xchg(&lk->locked,1))
  { 
    sp_lock(&print_lock,0);
    printf("CPU#%d Acquiring lock  %s\n",_cpu(),lk->name);
    sp_unlock(&print_lock,0);
  }
  if(log)
  {
  sp_lock(&print_lock,0);
  printf("CPU#%d Acquires lock  %s\n",_cpu(),lk->name);
  sp_unlock(&print_lock,0);
  }
}
void sp_unlock(lock_t *lk,int log)
{
  _atomic_xchg(&lk->locked,0);
  if(log)
  {sp_lock(&print_lock,0);
  printf("CPU#%d Frees lock  %s\n",_cpu(),lk->name);
  sp_unlock(&print_lock,0);
  }
}*/

//锁pre,nxt;
void blink(struct block* pre,struct block*nxt)//直接连接
{
  if(pre)
  pre->next=nxt;
  if(nxt)
  nxt->prev=pre;}

//锁blk,blk->prev,blk->next;
void bdelete(struct block* blk)//删除
{
  if(blk->next)
  {
  (blk->prev)->next=blk->next;
  (blk->next)->prev=blk->prev;
  }
  else
  (blk->prev)->next=NULL;
}

//锁pre,pre->next,nxt
void binsert(struct block* pre,struct block* nxt,bool is_merge)//插入
{
  //把next接在pre后
  //assert(pre&&nxt);
  if(!is_merge)
  {
    if(pre->next==NULL)
    blink(pre,nxt);
    else
    {
      nxt->next=pre->next;
      nxt->prev=pre;
      (nxt->next)->prev=nxt;
      (nxt->prev)->next=nxt;
    }
    return;
  }
    //合并
    if(pre->next==NULL)
    {
      if(pre->end==nxt->start)
      {
        pre->end=nxt->end;
        pre->size=pre->end-pre->start;
        bfree(nxt);
      }
      else
      binsert(pre,nxt,0);
    }
    else
    {
      struct block* ptr1=pre;
      struct block* ptr2=pre->next;
      if(ptr1->end==nxt->start&&ptr2->start==nxt->end)
      {//三合一
      blink(ptr1,ptr2->next);
      ptr1->end=ptr2->end;
      ptr1->size=ptr1->end-ptr1->start;
      bfree(nxt);
      bfree(ptr2);
      }
      else if(ptr1->end==nxt->start)
      {ptr1->end=nxt->end;
      ptr1->size=ptr1->end-ptr1->start;
      bfree(nxt);
      }
      else if(nxt->end==ptr2->start)
      {ptr2->start=nxt->start;
      ptr2->size=ptr2->end-ptr2->start;
      bfree(nxt);
      }
      else//两不沾
      binsert(ptr1,nxt,0);
  }

  //is_merge代表连成一段的直接合并
}

void print_FreeBlock()
{
  #ifdef _DEBUG
  sp_lock(&print_lock);
  struct block* ptr=free_head->next;
  printf("Free blocks:\n");
  while(ptr)
  {
    printf("[%p,%p)\n",ptr->start,ptr->end);
    ptr=ptr->next;
  }
  sp_unlock(&print_lock);
  #endif
}

void print_AllocatedBlock()
{
  #ifdef _DEBUG
  sp_lock(&print_lock);
  struct block* ptr=alloc_head->next;
  printf("Allocated blocks:\n");
  while(ptr)
  {
    printf("[%p,%p)\n",ptr->start,ptr->end);
    ptr=ptr->next;
  }
  sp_unlock(&print_lock);
  #endif
}

uintptr_t GetValidAddress(uintptr_t start,int align)//返回从start开始对齐align的最小地址
{
  uint32_t temp=1;
    while(align%2==0)
    {temp=temp*2;
    align=align/2;
    }
    uint32_t ret=(((uint32_t)(start-1)/temp)+1)*temp;
    return (uintptr_t)ret;
}

const int max_block_num=0x2000000/sizeof(struct block);
//最多给管理的块分配0x2000000的空间
int FreeAllocNo[20000];
int FreeAllocNoNum=0;//记录被释放后的块的位置，方便直接使用
int maxpos=0;//当前已经分配到的最大位置，当mset为空时从这里开始分配
uintptr_t bstart;
static void *balloc()//专门给block分配空间用,直接从某一位置开始往上垒不用对齐
{
  sp_lock(&print_lock);
  printf("CPU#%d BALLOC\n",_cpu());
  sp_unlock(&print_lock);
  sp_lock(&alloc_lock);
  
  uintptr_t ret;
  if(FreeAllocNoNum)
  {
    int no=FreeAllocNo[--FreeAllocNoNum];//no不会大于maxpos
    ret=(uintptr_t)(bstart+no*sizeof(struct block));
  }
  else
  {
  ret=(uintptr_t)(bstart+maxpos*sizeof(struct block));
  maxpos=maxpos+1;
  }  
  sp_unlock(&alloc_lock);
  return (void *)ret;
}

static void bfree(struct block* blk)
{
  sp_lock(&print_lock);
  printf("CPU#%d BFREE\n",_cpu());
  sp_unlock(&print_lock);
  sp_lock(&alloc_lock);
  int no =((uintptr_t)blk-bstart)/sizeof(struct block);
  blk->next=NULL;
  blk->prev=NULL;
  FreeAllocNo[FreeAllocNoNum++]=no;
  sp_unlock(&alloc_lock);
}

void check_allocblock(uintptr_t start,uintptr_t end)
{
  struct block* aptr=alloc_head->next;
  while(aptr)
  {
    if(aptr->start==start&&aptr->end!=end)
    { 
      //printf("Allocated block overlapped\n");
      assert(0);
    }
    aptr=aptr->next;
  }
}

void check_freeblock()
{
    struct block* fptr=free_head->next;
    uintptr_t end=0;
    while(fptr)
    {
      if(!(fptr->start>=end))
      {
        //printf("FreeBlock %p overlapped\n",fptr->start);
        assert(0);
      }
      if(!(fptr->end>fptr->start))
      {
        //printf("end > start\n");
        assert(0);
      }
      end=fptr->end;
      fptr=fptr->next;
    }

}

static void *kalloc(size_t size)//对于两个链表的修改，分别用链表大锁锁好
  { sp_lock(&print_lock);
    printf("CPU#%d KALLOC\n",_cpu());
    sp_unlock(&print_lock);
    sp_lock(&glb_lock);
    struct block*ptr=free_head->next;
    while(ptr)
    {
      uintptr_t valid_addr=GetValidAddress(ptr->start,size);
      if(valid_addr+size<=ptr->end)
      {
      //四种情况,靠头，靠尾，既靠头又靠尾，两不靠
      if(valid_addr==ptr->start&&valid_addr+size==ptr->end)
      { sp_lock(&print_lock);
        printf("CPU#%d case 1\n",_cpu());
        sp_unlock(&print_lock);
      bdelete(ptr);
      binsert(alloc_head,ptr,0);//整个节点直接挪过来
      #ifdef _DEBUG
      print_FreeBlock();
      print_AllocatedBlock();
      //check_freeblock();
      //check_allocblock(valid_addr,valid_addr+size);
      #endif
      sp_unlock(&glb_lock);
      return (void *)valid_addr;
      }
      else if(valid_addr==ptr->start)
      { sp_lock(&print_lock);
        printf("CPU#%d case 2\n",_cpu());
        sp_unlock(&print_lock);
        ptr->start=valid_addr+size;
        ptr->size=ptr->end-ptr->start;
        struct block *alloc_blk=(struct block*)balloc(sizeof(struct block));
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(alloc_head,alloc_blk,0);
        #ifdef _DEBUG
        print_FreeBlock();
        print_AllocatedBlock();
        //check_freeblock();
        //check_allocblock(valid_addr,valid_addr+size);
        #endif
        sp_unlock(&glb_lock);
        return (void*)valid_addr;
      }
      else if(valid_addr+size==ptr->end)
      { sp_lock(&print_lock);
        printf("CPU#%d case 3\n",_cpu());
        sp_unlock(&print_lock);
        ptr->end=valid_addr;
        ptr->size=ptr->end-ptr->start;
        struct block *alloc_blk=(struct block*)balloc(sizeof(struct block));
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(alloc_head,alloc_blk,0);
        #ifdef _DEBUG
        print_FreeBlock();
        print_AllocatedBlock();
        //check_freeblock();
        //check_allocblock(valid_addr,valid_addr+size);
        #endif
        sp_unlock(&glb_lock);
        return (void*)valid_addr;
      }
      else
      { sp_lock(&print_lock);
        printf("CPU#%d case 4\n",_cpu());
        sp_unlock(&print_lock);
        struct block*alloc_blk=(struct block*)balloc(sizeof(struct block));
        struct block*free_blk=(struct block*)balloc(sizeof(struct block));
        free_blk->end=ptr->end;
        free_blk->start=valid_addr+size;
        free_blk->size=free_blk->end-free_blk->start;
        ptr->end=valid_addr;
        ptr->size=ptr->end-ptr->start;
        binsert(ptr,free_blk,0);
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(alloc_head,alloc_blk,0);
        #ifdef _DEBUG
        print_FreeBlock();
        print_AllocatedBlock();
        //check_freeblock();
        //check_allocblock(valid_addr,valid_addr+size);
        #endif
        sp_unlock(&glb_lock);
        return (void*)valid_addr;
      }
      }
      ptr=ptr->next;
    }
    sp_unlock(&glb_lock);
    assert(0);
    return NULL;
}

static void kfree(void *ptr) {
  sp_lock(&print_lock);
  printf("CPU#%d KFREE\n",_cpu());
  sp_unlock(&print_lock);
  sp_lock(&glb_lock);
  printf("haha\n");
  uintptr_t start=(uintptr_t)ptr;
  struct block* blk_ptr=alloc_head->next;
  while(blk_ptr)
  {
    if(blk_ptr->start==start)//找到了相应的块
    {
      bdelete(blk_ptr);
      struct block *loc_ptr=free_head;//找到合适的插入free的位置
      while(loc_ptr)
      {
        if(loc_ptr->end<=start)
        {
          if(loc_ptr->next==NULL)
          { 
            printf("LOCATED\n");
            sp_lock(&print_lock);
            printf("CPU#%d case 5\n",_cpu());
            sp_unlock(&print_lock);
            binsert(loc_ptr,blk_ptr,1);
            #ifdef _DEBUG
            print_FreeBlock();
            print_AllocatedBlock();
            //check_freeblock();
            #endif
            sp_unlock(&glb_lock);
            return;
          }
          if((loc_ptr->next)->start>=blk_ptr->end)//这两种情况均可以插入
          {
            printf("LOCATED\n");
            sp_lock(&print_lock);
            printf("CPU#%d case 6\n",_cpu());
            sp_unlock(&print_lock);
            binsert(loc_ptr,blk_ptr,1);
            #ifdef _DEBUG
            //check_freeblock();
            #endif
            print_FreeBlock();
            print_AllocatedBlock();
            sp_unlock(&glb_lock);
            return;
          }
        }
        loc_ptr=loc_ptr->next;
      } 
    }
    blk_ptr=blk_ptr->next;
  }
  printf("hrhr\n");
  #ifdef _DEBUG
  sp_lock(&print_lock);
  printf("Block at %p has not been allocated or already freed\n",ptr);
  sp_unlock(&print_lock);
  check_freeblock();
  #endif
  sp_unlock(&glb_lock);
  return;
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)_heap.end - (uintptr_t)_heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, _heap.start, _heap.end);
  bstart=(uintptr_t)_heap.end-0x2000000;
  sp_lockinit(&alloc_lock,"balloc_lock\0",0);
  sp_lockinit(&glb_lock,"glb_lock\0",1);
  sp_lockinit(&print_lock,"print_lock\0",2);
  free_head=(struct block *)balloc(sizeof(struct block));
  alloc_head=(struct block *)balloc(sizeof(struct block));
  free_head->start=free_head->end=free_head->size=0;
  alloc_head->start=alloc_head->end=alloc_head->size=0;

  struct block *blk=(struct block *)balloc(sizeof(struct block));
  blk->start=(uintptr_t)_heap.start;
  blk->end=(uintptr_t)(_heap.end-0x2000000);
  blk->size=blk->end-blk->end;
  blk->prev=free_head;
  free_head->next=blk;
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
