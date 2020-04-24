#include <common.h>

static void os_init() {
  pmm->init();
}

void* allocated[1005];
int num=0;
extern struct block* free_head;
extern struct block* alloc_head;//两个都是空的节点
extern void print_block(struct block *ptr);
static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  srand(0);
  for(int i=0;i<50;i++)
  { int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size=rand()%2048;
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      printf("Allocated block of size %d at [%p,%p)\n",size,ptr,ptr+size);
      #endif
      
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      printf("Trying to free %p\n",allocated[r]);
      #endif
      pmm->free(allocated[r]);
      #ifdef _DEBUG
      printf("Successfully freed\n");
      #endif
    }
    printf("Allocated blocks:\\");
    print_block(alloc_head);
    printf("Free blocks:\\");
    print_block(free_head);
  }
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};


