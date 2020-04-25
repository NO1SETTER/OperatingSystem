#include <common.h>
#define _DEBUG
static void os_init() {
  pmm->init();
}



void* allocated[1005];
int num=0;
extern void print_FreeBlock();
extern void print_AllocatedBlock();
extern void block_check(void *ptr);
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
      //printf("Allocating\n");
      void* ptr=pmm->alloc(size);
      block_check(ptr);
      #ifdef _DEBUG
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) {
        continue;}
      int r=rand()%num;
      #ifdef _DEBUG
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      printf("Successfully freed\n");
      #endif
    }
    print_FreeBlock();
    print_AllocatedBlock();
  }
  printf("Thread %d finished\n",_cpu());
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};


