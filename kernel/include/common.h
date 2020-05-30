#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


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
typedef struct spinlock 
{
  const char *name;//锁名
  int lockid;//锁的序号
  intptr_t locked;//锁控制
  int holder;//锁的持有者
}spinlock_t;

typedef struct task
{
  struct
  {
    const char *name;
    struct task *next;` 
    _Context *ctx;
  };
  uint8_t* stack;
  void (*entry)(void *arg);
  void *arg;
  int active;
}task_t;//管理一个线程的信息
struct task_t *current;//当前task

void set_unworkable(task_t* t)
{
  t->active=0;
}

void set_workable(task_t* t)
{
  t->active=1;
}

typedef struct semaphore
{
spinlock_t lock;
const char *name;
int val;
} sem_t;

MODULE(kmt) {
  void (*init)();
  int  (*create)(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
  void (*teardown)(task_t *task);
  void (*spin_init)(spinlock_t *lk, const char *name);
  void (*spin_lock)(spinlock_t *lk);
  void (*spin_unlock)(spinlock_t *lk);
  void (*sem_init)(sem_t *sem, const char *name, int value);
  void (*sem_wait)(sem_t *sem);
  void (*sem_signal)(sem_t *sem);
};
