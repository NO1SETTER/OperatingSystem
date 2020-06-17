#include <common.h>
#define STACK_SIZE 4096
#define _DEBUG
#define P kmt->sem_wait
#define V kmt->sem_signal
static void sem_init(sem_t *sem, const char *name, int value);
static void sem_wait(sem_t *sem);
static void sem_signal(sem_t *sem);
//自旋锁部分
spinlock_t thread_ctrl_lock;
extern spinlock_t print_lock;//print_lock内部不加别的锁,不产生ABBA型
void sp_lockinit(spinlock_t* lk,const char *name);
void sp_lock(spinlock_t* lk);
void sp_unlock(spinlock_t *lk);
//线程创建释放
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
static void kmt_teardown(task_t *task);
//线程阻塞唤醒
int thread_num=0;
int active_num=0;
int wait_num=0;
void activate(int id,sem_t* sem);
void await(int id,sem_t* sem);
void kill(int id);
//中断处理程序
struct EVENT{//中断事件
int seq;
int event;
handler_t handler;
struct EVENT* next;
};
struct EVENT EV_HEAD={-1,0,NULL,NULL};//用链表记录所有_Event
struct EVENT * evhead=&EV_HEAD;
//中断处理函数
_Context* schedule(_Event ev,_Context* c);
_Context* cyield(_Event ev,_Context* c);
  //中断处理程序入口
static _Context *os_trap(_Event ev,_Context *context);
  //中断注册程序
static void on_irq (int seq,int event,handler_t handler);
//安全分配
void *kalloc_safe(size_t size);
void kfree_safe(void *ptr);


task_t* task_alloc()
{ return (task_t*)kalloc_safe(sizeof(task_t));
}

#ifdef DEBUG_LOCAL
sem_t empty;
sem_t fill;
  void producer(void *arg)
  {
    while(1)
    {
      P(&empty);
      printf("(");
      V(&fill);
    }
  }

  void consumer(void *arg)
  {
    while(1)
    {
      P(&fill);
      printf(")");
      V(&empty);
    }
  }
#endif


static void os_init() {
  pmm->init();
  kmt->init(); // 模块先初始化
  for(int i=0;i<_ncpu();i++)
    currents[i]=NULL;
  #ifdef DEV_ENBALE
    dev->init();
  #endif
kmt->spin_lock=sp_lock;
kmt->spin_unlock=sp_unlock;//这里会出现奇怪的“未赋值情况”

#ifdef DEBUG_LOCAL
  kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
  kmt->sem_init(&fill,  "fill",  0);
  
  char p[4][3]={"p1","p2","p3","p4"};
  char c[5][3]={"c1","c2","c3","c4","c5"};
  for(int i=0;i<4;i++)
    kmt->create(task_alloc(), p[i], producer, NULL);
  for(int i=0;i<5;i++)
    kmt->create(task_alloc(), c[i], consumer, NULL);
#endif
}



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
  }*/
  _intr_write(1);
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

void sp_lock(spinlock_t* lk)
{
  _intr_write(0);
  while(_atomic_xchg(&lk->locked,1));
}

void sp_unlock(spinlock_t *lk)
{
  _atomic_xchg(&lk->locked,0);
}

void sp_lockinit(spinlock_t* lk,const char *name)
{
  strcpy(lk->name,name);
  lk->locked=0;
}

_Context* schedule(_Event ev,_Context* c)//传入的c是current的最新上下文,要保存下来
{
      /*if(current==NULL)
      {
        current=all_thread[active_thread[0]];
      }
      else
      {
        current->ctx = c;
        current=all_thread[active_thread[rand()%active_num]]; 
      }
      assert(current);
      //printf("task %s running on CPU#%d\n",current->name,_cpu());
      return current->ctx;*/
      task_t* current=currents[_cpu()];
      if(!current)
        current=all_thread[0];//暂时的
      else
        current->ctx=c;
      
      task_t* rec=current;
      int reschedule=0;
      do{
        current=current->next;
        if(rec==current)//转了一圈都没找到
          reschedule=1;
        if(reschedule&&current->status)//由于指定队列内的都被阻塞,允许调度指定队列外的线程
         break;
      }while((current->id)%_ncpu()!=_cpu()||current->status!=T_RUNNING);
      return current->ctx;
}

_Context* cyield(_Event ev,_Context* c)
{
  _yield();
  return NULL;
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
  struct EVENT * NEW_EV=(struct EVENT*)kalloc_safe(sizeof(struct EVENT));
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

void kill(int id)//running->dead
{
  sp_lock(&thread_ctrl_lock);
  int pos=-1;
  for(int i=0;i<active_num;i++)
  {
    if (active_thread[i]==id) {
      pos = i;break;
     }
  }
  if(pos==-1)
  {
    sp_unlock(&thread_ctrl_lock);
    _intr_write(1);
    return; 
  }
  for(int i=pos;i<active_num-1;i++)
  active_thread[i]=active_thread[i+1];

  all_thread[id]->status=T_DEAD;
  sp_unlock(&thread_ctrl_lock);
  _intr_write(1);
}

static void kmt_init()
{
  kmt->spin_init(&thread_ctrl_lock,"thread_ctrl_lock");
  on_irq(0,_EVENT_YIELD,schedule);
  on_irq(1,_EVENT_IRQ_TIMER,cyield);
}

//task提前分配好,那么我们用一个指针数组管理所有这些分配好的task
//_Area{*start,*end;},
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  strcpy(task->name,name);//名字
  task->status=T_RUNNING;//状态
  task->id=thread_num;//id设置为当前进程数
  if(thread_num > 0)
  {
    all_thread[thread_num-1]->next=task;//设置链表形成环路
    task->next=all_thread[0];
  }
  _Area stack=(_Area){ task->ctx+1,task->stack+STACK_SIZE};  
  task->ctx=_kcontext(stack,entry,arg);//设置栈空间以及上下文
  all_thread[thread_num++]=task;//添加到所有线程中
  active_thread[active_num++]=task->id;//添加到活跃线程中
  return 0;
}

static void kmt_teardown(task_t *task)
{
  kill(task->id);//不会从all_thread中删去
  kfree_safe(task->stack);
}


static void sem_init(sem_t *sem, const char *name, int value)
{
  char lock_name[128];
  sprintf(lock_name,"%s_lock",name);
  kmt->spin_init(&sem->lock,lock_name);
  strcpy(sem->name,name);
  sem->val=value;
  sem->wnum=0;
}

static void sem_wait(sem_t *sem)
{
  sp_lock(&sem->lock);//sem->lock用于控制一切对sem的修改
  sp_lock(&thread_ctrl_lock);
  sem->val--;
  #ifdef _DEBUG
  printf("wait:%s->val = %d\n",sem->name,sem->val);
  #endif
  if(sem->val<0) 
  {
    task_t * cur=currents[_cpu()];
    printf("CPU#%d\n",_cpu());
    if(sem->wnum==0)
    {
      sem->waiter[sem->wnum++]=cur->id;
      #ifdef _DEBUG
      printf("%s blocked\n",cur->name);
      #endif
    }
    else
    {
      int judge=1;
      for(int i=0;i<sem->wnum;i++)
      {
        if(sem->waiter[i]==cur->id) judge=0;
      }
      if(judge) {
        sem->waiter[sem->wnum++]=cur->id;
        #ifdef _DEBUG
        printf("%s blocked\n",cur->name);
        #endif
        }
    }
    int pos=-1;
    for(int i=0;i<active_num;i++)
    {
      if(active_thread[i]==cur->id)
      {
        pos=i;break;
      }
    }
    assert(pos!=-1);
    for(int i=pos;i<active_num-1;i++)
    {
      active_thread[i]=active_thread[i+1];
    }
    active_num=active_num-1;

    sp_unlock(&thread_ctrl_lock);
    sp_unlock(&sem->lock);

    _intr_write(1);
    _yield();
    return;
  }
  sp_unlock(&thread_ctrl_lock);
  sp_unlock(&sem->lock);
  _intr_write(1);
}

static void sem_signal(sem_t *sem)
{
  sp_lock(&sem->lock);
  sp_lock(&thread_ctrl_lock);
  sem->val++;
  #ifdef _DEBUG
  printf("signal:%s->val = %d\n",sem->name,sem->val);
  #endif
    if(sem->wnum)
    {
      int no=rand()%sem->wnum;
      active_thread[active_num++]=sem->waiter[no];
      #ifdef _DEBUG
      printf("%s activated\n",all_thread[sem->waiter[no]]->name);
      #endif
      for(int i=no;i<sem->wnum-1;i++)
      sem->waiter[i]=sem->waiter[i+1];
      sem->wnum--;
    }
  sp_unlock(&thread_ctrl_lock);
  sp_unlock(&sem->lock);
  _intr_write(1);
}


void*kalloc_safe(size_t size)
{
    int i = _intr_read();
  _intr_write(0);
  void *ret = pmm->alloc(size);
  if (i) _intr_write(1);
  return ret;
}

void kfree_safe(void *ptr)
{
    int i = _intr_read();
  _intr_write(0);
  pmm->free(ptr);
  if (i) _intr_write(1);
}

MODULE_DEF(kmt) = {
  .init=kmt_init,
  .spin_init=sp_lockinit,
  .spin_lock=(void*)sp_lock,
  .spin_lock=(void*)sp_unlock,
  .create=kmt_create,
  .teardown=kmt_teardown,
  .sem_init=sem_init,
  .sem_wait=sem_wait,
  .sem_signal=sem_signal,
};
