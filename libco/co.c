#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#define STACK_SIZE 4096
enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  int no;//协程号
  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
};

struct co *current；//当前协程
struct co *allco[200];//管理所有协程
int co_num=1;//已有协程数量,把main也看做一个协程

struct co *active[200];//当前能够被调用的协程,即状态为CO_RUNNING和CO_NEW的协程
void copush(struct co *now)
{co(active_num++)=now;}
void coremove(struct co *now)
{ int pos=-1;
  for(int i=0;i<active_num;i++)
  if(active[i]==co)
  {
    pos==i;
    break;
  }
  assert(pos!=-1);
  for(int i=pos;i<active_num-1;i++)
    active[i]=active[i+1];
  active_num=active_num-1;
}
int active_num=1;

__attribute__((constructor)) void set_main()
{
  struct co* mainco=(co*)malloc(sizeof(struct co));
  mainco->no=0;
  mainco->status=CO_RUNNING;
  copush(mainco);
  current=mainco;
}


struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co* newco=(co*)malloc(sizeof(struct co));
  newco->name=name;
  newco->func=func;
  newco->arg=arg;
  newco->no==co_num;
  newco->status=CO_NEW;
  allco[co_num]=newco;
  co_num++;
  copush(newco);
  return NULL;
}//创建新的协程

void co_wait(struct co *co) {
if(co->status==CO_DEAD)//被等待的协程已经结束
{
}
else
{
co->waiter=current;
coremove(current);
co_yield();//current被移除,在co_yield中重新设置一个current
}
}//co_wait只设置协程之间的关系


void co_yield() {
  int val=setjmp(current->context);
  if(val==0)//
  {int nxt=rand()%active_num;
  struct nxtco=active_num[nxt];
    if(nxtco->status=CO_NEW)
    {
      stack_switch_call(&nxtco->stack[STACK_SIZE-1],nxtco->func,nxtco->arg);
    }
    else if(nxtco->status=CO_RUNNING)//已经开始的协程，直接恢复寄存器现场即可
    {
      longjmp(nxtco->context,1);
    }
  }
  else
  {

  }
}
