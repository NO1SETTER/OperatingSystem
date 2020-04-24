#include <common.h>


struct block//管理空闲块或非空闲块的数据结构
{
uintptr_t start,end;//[start,end)
uint32_t size;
struct block* prev;
struct block* next;
};

struct block* free_head;
struct block* alloc_head;//两个都是空的节点


void blink(struct block* pre,struct block*nxt)//直接连接
{
  if(pre)
  pre->next=nxt;
  if(nxt)
  nxt->prev=pre;
}

void bdelete(struct block* blk)//删除
{
assert(blk->prev);

if(blk->next)
{
(blk->prev)->next=blk->next;
(blk->next)->prev=blk->prev;
}
else
(blk->prev)->next=NULL;
}

void binsert(struct block* pre,struct block* nxt,bool is_merge)//插入
{
  //把next接在pre后
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
        free(nxt);
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
      ptr1->end=ptr2->end;
      ptr1->size=ptr1->end-ptr1->start;
      free(nxt);
      free(ptr2);
      }
      else if(ptr1->end==nxt->start)
      {ptr1->end=nxt->end;
      ptr1->size=ptr1->end-ptr1->start;
      free(nxt);
      }
      else if(nxt->end==ptr2->start)
      {ptr2->start=nxt->start;
      ptr2->size=ptr2->end-ptr2->start;
      free(nxt);
      }
      else//两不沾
      binsert(ptr1,nxt,0);
  }

  //is_merge代表连成一段的直接合并
}


void print_block(struct block *ptr)
{
  while(ptr)
  {
    printf("[%p,%p)\n",ptr->start,ptr->end);
    ptr=ptr->next;
  }
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


static void *kalloc(size_t size) {
    struct block*ptr=free_head->next;
  while(ptr)
  {
    uintptr_t valid_addr=GetValidAddress(ptr->start,size);
    if(valid_addr+size<=ptr->end)
    {//四种情况,靠头，靠尾，既靠头又靠尾，两不靠
    if(valid_addr==ptr->start&&valid_addr+size==ptr->end)
    {bdelete(ptr);
    binsert(alloc_head,ptr,0);//整个节点直接挪过来
    return (void *)valid_addr;
    }
    else if(valid_addr==ptr->start)
    {
      ptr->start=valid_addr+size;
      ptr->size=ptr->end-ptr->start;
      struct block *alloc_blk=(struct block*)malloc(sizeof(struct block));
      alloc_blk->start=valid_addr;
      alloc_blk->end=valid_addr+size;
      alloc_blk->size=size;
      binsert(alloc_head,alloc_blk,0);
     
      return (void*)valid_addr;
    }
    else if(valid_addr+size==ptr->end)
    {
      ptr->end=valid_addr;
      ptr->end=ptr->end-ptr->start;
      struct block *alloc_blk=(struct block*)malloc(sizeof(struct block));
      alloc_blk->start=valid_addr;
      alloc_blk->end=valid_addr+size;
      alloc_blk->size=size;
      binsert(alloc_head,alloc_blk,0);
      return (void*)valid_addr;
    }
    else
    {
      struct block*alloc_blk=(struct block*)malloc(sizeof(struct block));
      struct block*free_blk=(struct block*)malloc(sizeof(struct block));
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
    }
    }
    ptr=ptr->next;
  }
   return NULL;
}

static void kfree(void *ptr) {
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
            bdelete(blk_ptr);
            binsert(loc_ptr,blk_ptr,1);
            return;
          }
          if((loc_ptr->next)->start>=blk_ptr->end)//这两种情况均可以插入
          {
            bdelete(blk_ptr);
            binsert(loc_ptr,blk_ptr,1);
            return;
          }
        }
        loc_ptr=loc_ptr->next;
      }
    }
    blk_ptr=blk_ptr->next;
  }
  #ifdef _DEBUG
  printf("Block at %p has not been allocated or already freed\n",ptr);
  #endif
  return;
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)_heap.end - (uintptr_t)_heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, _heap.start, _heap.end);
  free_head=(struct block *)malloc(sizeof(struct block));
  alloc_head=(struct block *)malloc(sizeof(struct block));
  free_head->start=free_head->end=free_head->size=0;
  alloc_head->start=alloc_head->end=alloc_head->size=0;

  struct block *blk=(struct block *)malloc(sizeof(struct block));
  blk->start=(uintptr_t)_heap.start;
  blk->end=(uintptr_t)_heap.end;
  blk->size=blk->end-blk->end;
  blk->prev=free_head;
  free_head->next=blk;
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
