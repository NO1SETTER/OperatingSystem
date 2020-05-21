#include <common.h>
//#define _DEBUG
static void os_init() {
  pmm->init();
}

typedef struct 
{
  intptr_t locked;
}lock_t;
extern lock_t print_lock;//print_lock内部不加别的锁,不产生ABBA型
extern void sp_lock(lock_t* lk);
extern void sp_unlock(lock_t *lk);


extern void check_allocblock(void *ptr);
extern void check_freeblock();
extern void print_FreeBlock();
extern void print_AllocatedBlock();

void* allocated[100005];
int num=0;

static void test1();
static void test2();
static void test3();
static void test4();
static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  
  int sel=4;
  if(sel==1)
  test1();
  else if(sel==2)
  test2();
  else if(sel==3)
  test3();
  else if(sel==4)
  test4();

  while (1) ;
}

static void test1()//在[0,2048)完全随机
{ printf("Conducting test1\n"); 
    for(int i=0;i<1000;i++)
  { 
    printf("Round %d for CPU#%d\n",i,_cpu());
    int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size=rand()%2048;
      if(size==0) continue;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocating size %d\n",size);
      sp_unlock(&print_lock);
      #endif
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      sp_unlock(&print_lock);
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      sp_unlock(&print_lock);
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Successfully freed\n");
      sp_unlock(&print_lock);
      #endif
    }
     printf("Finishing Round %d for CPU#%d\n",i,_cpu());
  }
}

static void test2()//交替测试大内存和小内存
{ printf("Conducting test2\n");
  
  for(int i=0;i<1000;i++)//小内存大内存交替分配释放
  {
    printf("Round %d for CPU#%d\n",i,_cpu());
    int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size;
      if(i%2)
      size=rand()%16;
      else
      size=rand()%2048+2048;
      if(size==0) continue;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocating size %d\n",size);
      sp_unlock(&print_lock);
      #endif
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      sp_unlock(&print_lock);
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      sp_unlock(&print_lock);
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Successfully freed\n");
      sp_unlock(&print_lock);
      #endif
    }
    printf("Finishing Round %d for CPU#%d\n",i,_cpu());
  }
}

static void test3()//频繁分配小内存
{ printf("Conducting test3\n");
  
  for(int i=0;i<10000;i++)//小内存大内存交替分配释放
  {
    printf("Round %d for CPU#%d\n",i,_cpu());
    int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size=rand()%128;
      if(size==0) continue;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocating size %d\n",size);
      sp_unlock(&print_lock);
      #endif
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      sp_unlock(&print_lock);
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      sp_unlock(&print_lock);
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Successfully freed\n");
      sp_unlock(&print_lock);
      #endif
    }
    printf("Finishing Round %d for CPU#%d\n",i,_cpu());
  }
}

static void test4()//频繁分配页
{ printf("Conducting test4\n");
  
  for(int i=0;i<2000;i++)
  {
    printf("Round %d for CPU#%d\n",i,_cpu());
    int rand_seed=rand()%5;
    if(rand_seed!=0)//kalloc
    {
      int size=rand()%(4096*4)+2048;
      if(size==0) continue;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocating size %d\n",size);
      sp_unlock(&print_lock);
      #endif
      void* ptr=pmm->alloc(size);
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Allocated block of size %d at [%p,%p) for CPU#%d\n",size,ptr,ptr+size,_cpu());
      sp_unlock(&print_lock);
      #endif
      allocated[num++]=ptr;
    }
    else//kfree
    {
      if(num==0) continue;
      int r=rand()%num;
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Trying to free %p for CPU#%d\n",allocated[r],_cpu());
      sp_unlock(&print_lock);
      #endif
      pmm->free(allocated[r]);      
      #ifdef _DEBUG
      sp_lock(&print_lock);
      printf("Successfully freed\n");
      sp_unlock(&print_lock);
      #endif
    }
    printf("Finishing Round %d for CPU#%d\n",i,_cpu());
  }
}
MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};


