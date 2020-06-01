#include <common.h>
//#define _DEBUG
#define DEBUG_LOCAL
#define STACK_SIZE 4096

void producer()
{
printf("(\n");
_yield();
}

void consumer()
{
printf(")\n");
_yield();
}

struct task_t* task_alloc()
{
  return (struct task_t*)pmm->alloc(sizeof(struct task_t));
}

struct sem_t empty;
struct sem_t fill;
static void os_init() {
  pmm->init();
  kmt->init(); // 模块先初始化

#ifdef DEBUG_LOCAL
  kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < 4; i++) // 4 个生产者
    kmt->create(task_alloc(), "producer", producer, NULL);
  for (int i = 0; i < 5; i++) // 5 个消费者
    kmt->create(task_alloc(), "consumer", consumer, NULL);
#endif
assert(0);
}

void sp_lockinit(struct spinlock_t* lk,const char *name)
{
  lk->name=name;
  lk->locked=0;;
}

void sp_lock(struct spinlock_t* lk)
{
  while(_atomic_xchg(&lk->locked,1));
}
void sp_unlock(struct spinlock_t *lk)
{
  _atomic_xchg(&lk->locked,0);
}

extern struct spinlock_t print_lock;//print_lock内部不加别的锁,不产生ABBA型
extern void check_allocblock(void *ptr);
extern void check_freeblock();
extern void print_FreeBlock();
extern void print_AllocatedBlock();

void* allocated[100005];
int num=0;

/*
static void test1();
static void test2();
static void test3();
static void test4();
*/

static void os_run() {
  /*for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  
  int sel=0;
  if(sel==1)
  test1();
  else if(sel==2)
  test2();
  else if(sel==3)
  test3();
  else if(sel==4)
  test4();*/
  _intr_write(1);
  _yield();
  while (1) ;
}
/*
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
}*/

int thread_num=0;
int active_num=0;
int wait_num=0;
struct task_t *current=NULL;//当前task

struct EVENT{
int seq;
int event;
handler_t handler;
struct EVENT* next;
};
struct EVENT EV_HEAD={-1,0,NULL,NULL};//用链表记录所有_Event
struct EVENT * evhead=&EV_HEAD;

_Context* schedule(_Event rv,_Context* c)
{
  if(current==NULL)
  {
    current=active_thread[0];
  }
  else
  {
    current->ctx = c;
    current = active_thread[rand()%active_num]; 
  }
  return current->ctx;
}

static _Context *os_trap(_Event ev,_Context *context)//对应_am_irq_handle + do_event
{
  _Context *pre=context; 
  _Context *next = NULL;
  struct EVENT *ptr=evhead->next;
  while(ptr)
  {
    if (ptr->event == _EVENT_NULL || ptr->event == ev.event) {
      _Context *r = ptr->handler(ev, context);
      //panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
    ptr=ptr->next;
  }
  if(next==NULL)
    next=pre;
  //panic_on(!next, "returning NULL context");
  //panic_on(sane_context(next), "returning to invalid context");
  return next;
}

static void on_irq (int seq,int event,handler_t handler)//原本是_cte_init中的一部分
{
  struct EVENT * NEW_EV=(struct EVENT*)pmm->alloc(sizeof(struct EVENT));
  NEW_EV->seq=seq;
  NEW_EV->event=event;
  NEW_EV->handler=handler;
  struct EVENT * ptr=evhead;
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

void activate(struct task_t* t)//wait->running
{
  int pos=-1;
  for(int i=0;i<wait_num;i++)
  {
    if (wait_thread[i]==t) {
      pos = i;
      break;}
  }
  assert(pos!=-1);
  for(int i=pos;i<wait_num-1;i++)
  wait_thread[i]=wait_thread[i+1];

  wait_num=wait_num-1;
  active_thread[active_num++]=t;
  t->status=T_RUNNING;
}

void random_activate()
{
  int pos=rand()%wait_num;
  struct task_t *t=wait_thread[pos];
  for(int i=pos;i<wait_num-1;i++)
  wait_thread[i]=wait_thread[i+1];

  wait_num=wait_num-1;
  active_thread[active_num++]=t;
  t->status=T_RUNNING;
}

void await(struct task_t* t)//running->wait
{
  int pos=-1;
  for(int i=0;i<active_num;i++)
  {
    if (active_thread[i]==t) {
      pos = i;
      break;}
  }
  assert(pos!=-1);
  for(int i=pos;i<active_num-1;i++)
  active_thread[i]=active_thread[i+1];

  active_num=active_num-1;
  wait_thread[wait_num++]=t;
  t->status=T_WAITING;
}

void kill(struct task_t* t)//running->dead
{
  int pos=-1;
  for(int i=0;i<active_num;i++)
  {
    if (active_thread[i]==t) {
      pos = i;
      break;}
  }
  assert(pos!=-1);
  t->status=T_DEAD;
}

static void kmt_init()
{on_irq(0,_EVENT_YIELD,schedule);
}

//task提前分配好,那么我们用一个指针数组管理所有这些分配好的task
//_Area{*start,*end;},start低地址,end高地址,也即栈顶
static int kmt_create(struct task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  all_thread[thread_num++]=task;
  task->name=name;
  task->status=T_RUNNING;
  task->stack=pmm->alloc(STACK_SIZE);
  _Area stack=(_Area){ task->stack,task->stack+STACK_SIZE};  
  task->ctx=_kcontext(stack,entry,arg);
  return 0;
}

static void kmt_teardown(struct task_t *task)
{
  kill(task);//不会从all_thread中删去
  pmm->free(task->stack);
}

static void sem_init(struct sem_t *sem, const char *name, int value)
{
sem->name=name;
sem->val=value;
}

static void sem_wait(struct sem_t *sem)
{
kmt->spin_lock(&sem->lock);
sem->val=sem->val-1;
int fail=0;
if(sem->val<0) fail=1;
kmt->spin_unlock(&sem->lock);

if(fail)
{
  await(current);
  _yield();//int $81
}
}

static void sem_signal(struct sem_t *sem)
{
kmt->spin_lock(&sem->lock);
sem->val=sem->val+1;
if(sem->val>=0)
random_activate();
kmt->spin_unlock(&sem->lock);
}

MODULE_DEF(kmt) = {
  .init=kmt_init,
  .spin_init=sp_lockinit,
  .spin_lock=sp_lock,
  .spin_lock=sp_unlock,
  .create=kmt_create,
  .teardown=kmt_teardown,
  .sem_init=sem_init,
  .sem_wait=sem_wait,
  .sem_signal=sem_signal,
};
