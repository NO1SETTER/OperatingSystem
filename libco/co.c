#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <assert.h>
#include <time.h>

#define STACK_SIZE 131073
enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};


struct co {
  const char *name ;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  int no;//协程号
  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t  stack[STACK_SIZE]; // 协程的堆栈
}__attribute__((aligned(64)));

struct co *current;//当前协程
struct co *allco[15000];//管理所有协程
int co_num=1;//已有协程数量,把main也看做一个协程

struct co *active[15000];//当前能够被调用的协程,即状态为CO_RUNNING和CO_NEW的协程
int active_num=0;
void co_check()
{ 
  #ifdef _DEBUG
  printf("Acitve coroutines: ");
  for(int i=0;i<active_num;i++)
  printf("%d ",active[i]->no);
  printf("\n");
  #endif
}

void co_push(struct co *now)
{  
  #ifdef _DEBUG
  printf("No %d is activated\n",now->no);
  #endif
  active[active_num++]=now;
  }

void co_remove(struct co *now)
{ 
  #ifdef _DEBUG
  co_check();
  printf("No %d is removed\n",now->no);
  #endif
  int pos=-1;
  for(int i=0;i<active_num;i++)
  if(active[i]==now)
  {
    pos=i;
    break;
  }
  assert(pos!=-1);
  for(int i=pos;i<active_num-1;i++)
    active[i]=active[i+1];
  active_num=active_num-1;
}


void align_check(struct co* now)
{
  #ifdef _DEBUG
printf("co at %p\n",(void *)now);
printf("co->stack at %p\n",(void *)&now->stack[STACK_SIZE-1]);
#endif
}

void co_end()//stack_switch_call的终点
{
asm volatile(
#if __x86_64__
      "push %%rbp"
      ::
#else
      "push %%ebp"
      ::
#endif
 );
 #ifdef _DEBUG
printf("no %d coroutine is ended\n",current->no);
#endif
current->status=CO_DEAD;
co_remove(current);
if(current->waiter)
{current->waiter->status=CO_RUNNING;
co_push(current->waiter);
}
asm volatile(
#if __x86_64__
    "pop %%rbp"
    ::
#else
    "pop %%ebp"
    ::
#endif
 );
co_yield();
}


static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  uintptr_t endfunc=(uintptr_t)co_end;
  asm volatile (//stack_switch_call本身可以不返回,
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi;push %%rsi;jmp *%1"
      : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg),"S"(endfunc) 
#else
    "movl %0, %%esp; movl %2, 4(%0);movl %%esi,(%%esp);jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg),"S"(endfunc)
#endif
  );
}

__attribute__((constructor)) void set_main()
{
  struct co* mainco=(struct co*)malloc(sizeof(struct co));
  mainco->no=0;
  mainco->status=CO_RUNNING;
  allco[0]=mainco;
  co_push(mainco);
  current=mainco;
 // srand((int)time(0));
}


struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co* newco=(struct co*)malloc(sizeof(struct co));
  align_check(newco);
  newco->name=name;
  newco->func=func;
  newco->arg=arg;
  newco->no=co_num;
  newco->status=CO_NEW;
  allco[co_num]=newco;
  co_num++;
  co_push(newco);
  return newco;
}//创建新的协程

//注意，一个线程在任一时刻只能wait一个线程
//但是一个线程有可能被多个线程wait，这种情况暂不处理
void co_wait(struct co *co) {
if(co->status==CO_DEAD)//被等待的协程已经结束
{}
else
{
co->waiter=current;
co_remove(current);
co_yield();//current被移除,在co_yield中重新设置一个current
}
}//co_wait只设置协程之间的关系


void co_yield() {
  int val=setjmp(current->context);//此时current已经不在active中
  if(val==0)//
  { 
    co_check();
    int nxt=rand()%active_num;
    struct co *nxtco=active[nxt];
    //printf("select coroutine %d as next\n",nxtco->no);
    current=nxtco;
    if(nxtco->status==CO_NEW)//调用新的协程，切换堆栈即可
    {
      nxtco->status=CO_RUNNING;
      stack_switch_call(&nxtco->stack[STACK_SIZE-1],nxtco->func,(uintptr_t)nxtco->arg);
    
    }
    else if(nxtco->status==CO_RUNNING)//调用已经开始的协程，直接恢复寄存器现场即可
    {
      longjmp(nxtco->context,1);
    }
  }
  else
  {

  }
}
