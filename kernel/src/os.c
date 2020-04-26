#include <common.h>
#define _DEBUG
static void os_init() {
  pmm->init();
}

typedef struct 
{
  intptr_t locked;
}lock_t;

lock_t lkk;

extern void sp_lock(lock_t* lk);
extern void sp_unlock(lock_t *lk);

extern void check_allocblock(void *ptr);
extern void check_freeblock();
void* allocated[1005];
int num=0;
extern void print_FreeBlock();
extern void print_AllocatedBlock();
static void test1();
static void test2();

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  srand(0);
  int sel=1;
  if(sel==0)
  test1();
  else if(sel==1)
  test2();

  while (1) ;
}

static void test1()
{ printf("Conducting test1\n"); 
  for(int i=0;i<100;i++)
  { 
    #ifdef _DEBUG
    sp_lock(&lkk);
    printf("Round %d for CPU#%d\n",i,_cpu());
    sp_unlock(&lkk);
    #endif
    int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size=rand()%2048;
      #ifdef _DEBUG
      printf("Allocating size %d\n",size);
      #endif
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      printf("Successfully freed\n");
      #endif
    }
    //print_FreeBlock();
    //print_AllocatedBlock();
    }
}

static void test2()
{ printf("Conducting test2\n");
  
  for(int i=0;i<50;i++)//小内存大内存交替分配释放
  {
    #ifdef _DEBUG
    sp_lock(&lkk);
    printf("Round %d for CPU#%d\n",i,_cpu());
    sp_unlock(&lkk);
    #endif
    int rand_seed=rand()%5;
    int size;
    if(i%2)
    size=rand()%16;
    else
    size=rand()%2048+2048;
    if(rand_seed!=0)//kalloc
    {
      #ifdef _DEBUG
      printf("Allocating size %d\n",size);
      #endif
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      printf("Successfully freed\n");
      #endif
    }

  }
}
MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};


