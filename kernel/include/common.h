#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <amdev.h>
//#define DEBUG_LOCAL
//#define DEV_ENABLE
enum t_status {
  T_NEW = 1, // 新创建，还未执行过
  T_RUNNING, // 已经执行过
  T_WAITING, // 在 co_wait 上等待
  T_DEAD,    // 已经结束，但还未释放资源
};

struct spinlock 
{
  char name[20];//锁名
  int lockid;//锁的序号
  intptr_t locked;//锁控制
  int holder;//锁的持有者
};

void sp_lockinit(spinlock_t* lk,const char *name);
void sp_lock(spinlock_t* lk);
void sp_unlock(spinlock_t *lk);

struct task
{
  struct
  {
    char name[15];
    int id;
    enum t_status status;
    struct task* next;//指向all_thread[id+1]
    _Context *ctx;//貌似只要保证它指向栈顶就ok了，上面的可以不管分配在哪里
  };
  uint8_t stack[4096];
};//管理一个线程的信息

task_t* all_thread[105];
int active_thread[105];//只记录线程的id,id对应它在all_thread中的位置

extern int thread_num;
extern int active_num;

extern  task_t *current;//当前task

extern spinlock_t thread_ctrl_lock;//管理控制这三个链表的锁
void kill(int id);

struct semaphore
{
spinlock_t lock;
char name[15];
int val;
int waiter[105];
int wnum;
};

struct device_t
{

};
