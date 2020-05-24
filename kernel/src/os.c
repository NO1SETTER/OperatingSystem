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
  /*for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }*/
  
  int sel=0;
  if(sel==1)
  test1();
  else if(sel==2)
  test2();
  else if(sel==3)
  test3();
  else if(sel==4)
  test4();
  _intr_write(1);

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

int NR_IRQ=0;
struct EV_CTRL{
int seq;
int event;
handler_t handler;
struct EV_CTRL* next;
};

struct EV_CTRL ev_ctrl={-1,0,NULL,NULL};//用链表记录所有_Event
struct EV_CTRL* EV_HEAD=&ev_ctrl;

static _Context *os_trap(_Event ev,_Context *context)
{
  _Context *next = NULL;
  struct EV_CTRL*ptr=EV_HEAD->next;
  while(ptr)
  {
    if (ptr->event == _EVENT_NULL || ptr->event == ev.event) {
      _Context *r = ptr->handler(ev, context);
      //panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
  ptr=ptr->next;
  }

  //panic_on(!next, "returning NULL context");
  //panic_on(sane_context(next), "returning to invalid context");
  return next;
  return NULL;
}

static void on_irq (int seq,int event,handler_t handler)
{
  struct EV_CTRL* NEW_EV=(struct EV_CTRL*)pmm->alloc(sizeof(struct EV_CTRL));
  NEW_EV->seq=seq;
  NEW_EV->event=event;
  NEW_EV->handler=handler;
  struct EV_CTRL* ptr=EV_HEAD;
  while(ptr)
  {
    if(ptr->seq<seq)
    {
      if(ptr->next==NULL)
      {
        ptr->next=NEW_EV;
        break;
      }
      if((ptr->next)->seq>seq)
      {
        NEW_EV->next=ptr->next;
        ptr->next=NEW_EV;
        break;
      }
    }
    ptr=ptr->next;
  }
  return;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = on_irq,
};


