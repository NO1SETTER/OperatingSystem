#include <common.h>

static void os_init() {
  pmm->init();
}

void* allocated[1005];
int num=0;
extern void print_FreeBlock();
extern void print_AllocatedBlock();
static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  srand(0);
  for(int i=0;i<50;i++)
  { 
    #ifdef _DEBUG
    printf("Round %d\n",i);
    #endif
    int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size=rand()%2048;
      printf("Allocating\n");
      sp_lock(&global_lock);
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      printf("Allocated block of size %d at [%p,%p)\n",size,ptr,ptr+size);
      #endif
      allocated[num++]=ptr;
      sp_unlock(&global_lock);
    }
    else//kfree
    {
      sp_lock(&global_lock);
      if(num==0) continue;
      sp_unlock(&global_lock);
      int r=rand()%num;
      #ifdef _DEBUG
      printf("Trying to free %p\n",allocated[r]);
      #endif
      sp_lock(&global_lock);
      pmm->free(allocated[r]);      
      sp_unlock(&global_lock);
      #ifdef _DEBUG
      printf("Successfully freed\n");
      #endif
    }
    print_FreeBlock();
    print_AllocatedBlock();
  }
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};


