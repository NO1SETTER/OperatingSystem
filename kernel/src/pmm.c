#include <common.h>
#define _DEBUG
//#define _BASIC_DEBUG
//#define _SLAB_ASSIST

#define PAGE_SIZE 4096 
#define STACK_SIZE 4096
#define BLOCK_AREA_SIZE 0x2000000
#define SLAB_SIZE 0x800000

void sp_lockinit(struct spinlock_t* lk,const char *name)
{
  lk->name=name;
  lk->locked=0;
}

void sp_lock(struct spinlock_t* lk)
{
  while(_atomic_xchg(&lk->locked,1));
  _intr_write(0);
}
void sp_unlock(struct spinlock_t *lk)
{
  _atomic_xchg(&lk->locked,0);
}


struct block//管理空闲块或非空闲块的数据结构
{
uintptr_t start,end;//管理[start,end)
uint32_t size;
struct block* prev;
struct block* next;
char none[8];
};

//用三个全局锁
struct spinlock_t glb_lock;//管理两个链表的锁
struct spinlock_t alloc_lock;//管理balloc和bfree并发性的锁
struct spinlock_t print_lock;//printf的锁,保证完整性
//两个链表的起始点
struct block* free_head;
struct block* alloc_head;//两个都是空的节点,管理全局
struct block* slab_free_head[8];
struct block* slab_alloc_head[8];//CPU#i的slab头和尾


//管理block内存
static void *balloc();
static void bfree(struct block* blk);
//调试用：打印，内部带print_lock锁,只在_DEBUG下被召唤出来
void print_AllocatedBlock();
void print_FreeBlock();
//调试用：检查有无重复,内部带print_lock锁，只在_DEBUG下被召唤出来
void check_allocblock(uintptr_t start,uintptr_t end);
void check_freeblock();

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
      nxt->next = pre->next;
      nxt->prev = pre;
      (nxt->next)->prev = nxt;
      (nxt->prev)->next = nxt;
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
    while(temp<align)
    {
      temp=temp*2;
    }

    uint32_t ret=(((uint32_t)(start-1)/temp)+1)*temp;
    return (uintptr_t)ret;
}

const int max_block_num=BLOCK_AREA_SIZE/sizeof(struct block);
//最多给管理的块分配0x2000000的空间
int FreeAllocNo[20000];
int FreeAllocNoNum=0;//记录被释放后的块的位置，方便直接使用
int maxpos=0;//当前已经分配到的最大位置，当mset为空时从这里开始分配
uintptr_t bstart;
static void *balloc()//专门给block分配空间用,直接从某一位置开始往上垒不用对齐
{
  #ifdef _BASIC_DEBUG
  sp_lock(&print_lock);
  printf("CPU#%d BALLOC\n",_cpu());
  sp_unlock(&print_lock);
  #endif
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
  #ifdef _BASIC_DEBUG
  sp_lock(&print_lock);
  printf("CPU#%d BFREE\n",_cpu());
  sp_unlock(&print_lock);
  #endif
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

    uint32_t size_align=aptr->size;
    uintptr_t start_align=aptr->start;
    int ct1=0;
    while((size_align&1)==0)
    {size_align>>=1;ct1++;}
    int ct2=0;
    while((start_align&1)==0)
    {start_align>>=1;ct2++;}
    if(ct1>ct2)
    {
      sp_lock(&print_lock);
      printf("Not aligned:size %d at [%p,%p)\n",aptr->size,aptr->start,aptr->end);
      assert(0);
      sp_unlock(&print_lock);
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
      if(fptr->start<end)
      {
        //printf("FreeBlock %p overlapped\n",fptr->start);
        assert(0);
      }
      end=fptr->end;
      fptr=fptr->next;
    }

}

static void *slab_kalloc(size_t size,int k)//对于CPU#k的slab_alloc，只用上输出锁
  { 
    #ifdef _SLAB_ASSIST
    #ifdef _BASIC_DEBUG
    printf("CPU#%d SLAB_KALLOC\n",_cpu());
    #endif
    struct block* ptr=slab_free_head[k]->next;
    while(ptr)
    {
      uintptr_t valid_addr=GetValidAddress(ptr->start,size);
      if(valid_addr+size<=ptr->end)
      {
      //四种情况,靠头，靠尾，既靠头又靠尾，两不靠
      if(valid_addr==ptr->start&&valid_addr+size==ptr->end)
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 1\n",_cpu());
        sp_unlock(&print_lock);
        #endif

      bdelete(ptr);
      binsert(slab_alloc_head[k],ptr,0);//整个节点直接挪过来
      return (void *)valid_addr;
      }
      else if(valid_addr==ptr->start)
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 2\n",_cpu());
        sp_unlock(&print_lock);
        #endif

        ptr->start=valid_addr+size;
        ptr->size=ptr->end-ptr->start;
        struct block *alloc_blk=(struct block*)balloc(sizeof(struct block));
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(slab_alloc_head[k],alloc_blk,0);
        return (void*)valid_addr;
      }
      else if(valid_addr+size==ptr->end)
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 3\n",_cpu());
        sp_unlock(&print_lock);
        #endif
        ptr->end=valid_addr;
        ptr->size=ptr->end-ptr->start;
        struct block *alloc_blk=(struct block*)balloc(sizeof(struct block));
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(slab_alloc_head[k],alloc_blk,0);
        return (void*)valid_addr;
      }
      else
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 4\n",_cpu());
        sp_unlock(&print_lock);
        #endif
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
        binsert(slab_alloc_head[k],alloc_blk,0);
        return (void*)valid_addr;
      }
      }
      ptr=ptr->next;
    }
    return NULL;
    #endif
    return NULL;
}

static bool slab_kfree(void *ptr,int k) {//从第k个CPU中找到是否有想要删除的对象
  #ifdef _SLAB_ASSIST
  #ifdef _BASIC_DEBUG
  sp_lock(&print_lock);
  printf("CPU#%d KFREE\n",_cpu());
  sp_unlock(&print_lock);
  #endif

  uintptr_t start=(uintptr_t)ptr;
  struct block* blk_ptr=slab_alloc_head[k]->next;
  while(blk_ptr)
  {
    if(blk_ptr->start==start)//找到了相应的块
    {
      bdelete(blk_ptr);
      struct block *loc_ptr=slab_free_head[k];//找到合适的插入free的位置
      while(loc_ptr)
      {
        if(loc_ptr->end<=start)
        {
          if((loc_ptr->next==NULL)||((loc_ptr->next)->start>=blk_ptr->end))
          { 
            #ifdef _BASIC_DEBUG
            sp_lock(&print_lock);
            printf("CPU#%d case 5\n",_cpu());
            sp_unlock(&print_lock);
            #endif
            binsert(loc_ptr,blk_ptr,1);
            return 1;
          }
        }
        loc_ptr=loc_ptr->next;
      } 
    }
    blk_ptr=blk_ptr->next;
  }
  #ifdef _DEBUG
  sp_lock(&print_lock);
  printf("Block at %p has not been allocated or already freed\n",ptr);
  sp_unlock(&print_lock);
  #endif
  return 0;
  #endif
  return 0;
}

static void *kalloc(size_t size)//对于两个链表的修改，分别用链表大锁锁好
  { 
    #ifdef _BASIC_DEBUG
    sp_lock(&print_lock);
    printf("CPU#%d KALLOC\n",_cpu());
    sp_unlock(&print_lock);
    #endif


    int k=_cpu();
    void * slab_ptr=slab_kalloc(size,k);
    if(slab_ptr)
    {
      return slab_ptr;
    }


    sp_lock(&glb_lock);
    struct block*ptr=free_head->next;
    while(ptr)
    {
      uintptr_t valid_addr=GetValidAddress(ptr->start,size);
      if(valid_addr+size<=ptr->end)
      {
      //四种情况,靠头，靠尾，既靠头又靠尾，两不靠
      if(valid_addr==ptr->start&&valid_addr+size==ptr->end)
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 1\n",_cpu());
        sp_unlock(&print_lock);
        #endif
      bdelete(ptr);
      binsert(alloc_head,ptr,0);//整个节点直接挪过来
      #ifdef _DEBUG
      //print_FreeBlock();
      //print_AllocatedBlock();
      check_freeblock();
      check_allocblock(valid_addr,valid_addr+size);
      #endif
      sp_unlock(&glb_lock);
      return (void *)valid_addr;
      }
      else if(valid_addr==ptr->start)
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 2\n",_cpu());
        sp_unlock(&print_lock);
        #endif
        ptr->start=valid_addr+size;
        ptr->size=ptr->end-ptr->start;
        struct block *alloc_blk=(struct block*)balloc(sizeof(struct block));
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(alloc_head,alloc_blk,0);
        #ifdef _DEBUG
        //print_FreeBlock();
        //print_AllocatedBlock();
        check_freeblock();
        check_allocblock(valid_addr,valid_addr+size);
        #endif
        sp_unlock(&glb_lock);
        return (void*)valid_addr;
      }
      else if(valid_addr+size==ptr->end)
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 3\n",_cpu());
        sp_unlock(&print_lock);
        #endif
        ptr->end=valid_addr;
        ptr->size=ptr->end-ptr->start;
        struct block *alloc_blk=(struct block*)balloc(sizeof(struct block));
        alloc_blk->start=valid_addr;
        alloc_blk->end=valid_addr+size;
        alloc_blk->size=size;
        binsert(alloc_head,alloc_blk,0);
        #ifdef _DEBUG
        //print_FreeBlock();
        //print_AllocatedBlock();
        check_freeblock();
        check_allocblock(valid_addr,valid_addr+size);
        #endif
        sp_unlock(&glb_lock);
        return (void*)valid_addr;
      }
      else
      { 
        #ifdef _BASIC_DEBUG
        sp_lock(&print_lock);
        printf("CPU#%d case 4\n",_cpu());
        sp_unlock(&print_lock);
        #endif
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
        //print_FreeBlock();
        //print_AllocatedBlock();
        check_freeblock();
        check_allocblock(valid_addr,valid_addr+size);
        #endif
        sp_unlock(&glb_lock);
        return (void*)valid_addr;
      }
      }
      ptr=ptr->next;
    }
    sp_unlock(&glb_lock);
    return NULL;
}

static void kfree(void *ptr) {
  #ifdef _BASIC_DEBUG
  sp_lock(&print_lock);
  printf("CPU#%d KFREE\n",_cpu());
  sp_unlock(&print_lock);
  #endif

  int k=_cpu();
    if(slab_kfree(ptr,k))
    {
      return ;
    }

  sp_lock(&glb_lock);
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
          if((loc_ptr->next==NULL)||((loc_ptr->next)->start>=blk_ptr->end))
          { 
            #ifdef _BASIC_DEBUG
            sp_lock(&print_lock);
            printf("CPU#%d case 5\n",_cpu());
            sp_unlock(&print_lock);
            #endif
            binsert(loc_ptr,blk_ptr,1);
            #ifdef _DEBUG
            //print_FreeBlock();
            //print_AllocatedBlock();
            check_freeblock();
            check_allocblock(blk_ptr->start,blk_ptr->end);
            #endif
            sp_unlock(&glb_lock);
            return;
          }
        }
        loc_ptr=loc_ptr->next;
      } 
    }
    blk_ptr=blk_ptr->next;
  }
  #ifdef _DEBUG
  sp_lock(&print_lock);
  printf("Block at %p has not been allocated or already freed\n",ptr);
  sp_unlock(&print_lock);
  #endif
  sp_unlock(&glb_lock);
  return;
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)_heap.end - (uintptr_t)_heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, _heap.start, _heap.end);
  bstart=(uintptr_t)_heap.end-BLOCK_AREA_SIZE;
  sp_lockinit(&alloc_lock,"balloc_lock");
  sp_lockinit(&glb_lock,"glb_lock");
  sp_lockinit(&print_lock,"print_lock");
  free_head=(struct block *)balloc(sizeof(struct block));
  alloc_head=(struct block *)balloc(sizeof(struct block));
  
  free_head->start=free_head->end=free_head->size=0;
  alloc_head->start=alloc_head->end=alloc_head->size=0;
  struct block *blk=(struct block *)balloc(sizeof(struct block));
  blk->start=(uintptr_t)_heap.start;
  blk->end=(uintptr_t)(_heap.end-BLOCK_AREA_SIZE-_ncpu()*SLAB_SIZE);//每个CPU分配0x800000用作slab,并分配0x2000000用作block的管理区域
  blk->size=blk->end-blk->end;
  blk->prev=free_head;
  free_head->next=blk;

  #ifdef _SLAB_ASSIST
  for(int i=0;i<_ncpu();i++)
  {
    slab_alloc_head[i]=(struct block *)balloc(sizeof(struct block));
    slab_free_head[i]=(struct block *)balloc(sizeof(struct block));
     struct block *slab_blk=(struct block *)balloc(sizeof(struct block));
     slab_blk->start=(uintptr_t)(_heap.end-BLOCK_AREA_SIZE-(i+1)*SLAB_SIZE);
     slab_blk->end=(uintptr_t)(_heap.end-BLOCK_AREA_SIZE-i*SLAB_SIZE);//每个CPU分配0x800000用作slab,并分配0x2000000用作block的管理区域
     slab_blk->size=SLAB_SIZE;
     slab_blk->prev=slab_free_head[i];
     slab_free_head[i]->next=slab_blk;
  }
  #endif
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};

