#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

enum t_status {
  T_NEW = 1, // 新创建，还未执行过
  T_RUNNING, // 已经执行过
  T_WAITING, // 在 co_wait 上等待
  T_DEAD,    // 已经结束，但还未释放资源
};

typedef _Context *(*handler_t)(_Event, _Context *);
MODULE(os) {
  void (*init)();
  void (*run)();
  _Context *(*trap)(_Event ev, _Context *context);
  void (*on_irq)(int seq, int event, handler_t handler);
};

MODULE(pmm) {
  void  (*init)();
  void *(*alloc)(size_t size);
  void  (*free)(void *ptr);
};
/*
static void*kalloc_safe(size_t size)
{
    int i = _intr_read();
  _intr_write(0);
  void *ret = pmm->alloc(size);
  if (i) _intr_write(1);
  return ret;
}
static void kfree_safe(void *ptr)
{
    int i = _intr_read();
  _intr_write(0);
  pmm->free(ptr);
  if (i) _intr_write(1);
}
*/

struct spinlock_t 
{
  const char *name;//锁名
  int lockid;//锁的序号
  intptr_t locked;//锁控制
  int holder;//锁的持有者
};

void sp_lockinit(struct spinlock_t* lk,const char *name);
void sp_lock(struct spinlock_t* lk);
void sp_unlock(struct spinlock_t *lk);

struct task_t
{
  struct
  {
    const char *name;
    enum t_status status;
    _Context *ctx;//貌似只要保证它指向栈顶就ok了，上面的可以不管分配在哪里
  };
  uint8_t *stack;
};//管理一个线程的信息

struct task_t* all_thread[10005];
int thread_num = 0;
struct task_t* active_thread[10005];
int active_num = 0;
struct task_t* wait_thread[10005];
int wait_num = 0;

void activate(struct task_t* t);
void random_activate();
void await(struct task_t *t);
void kill(struct task_t *t);

struct task_t *current=NULL;//当前task

typedef struct semaphore
{
struct spinlock_t lock;
const char *name;
int val;
}sem_t;


MODULE(kmt) {
  void (*init)();
  int  (*create)(struct task_t *task, const char *name, void (*entry)(void *arg), void *arg);
  void (*teardown)(struct task_t *task);
  void (*spin_init)(struct spinlock_t *lk, const char *name);
  void (*spin_lock)(struct spinlock_t *lk);
  void (*spin_unlock)(struct spinlock_t *lk);
  void (*sem_init)(sem_t *sem, const char *name, int value);
  void (*sem_wait)(sem_t *sem);
  void (*sem_signal)(sem_t *sem);
};
