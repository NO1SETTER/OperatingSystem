#include <am.h>
#pragma once
#define MODULE(mod) \
  typedef struct mod_##mod##_t mod_##mod##_t; \
  extern mod_##mod##_t *mod; \
  struct mod_##mod##_t

#define MODULE_DEF(mod) \
  extern mod_##mod##_t __##mod##_obj; \
  mod_##mod##_t *mod = &__##mod##_obj; \
  mod_##mod##_t __##mod##_obj

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

typedef struct task task_t;
typedef struct spinlock spinlock_t;
typedef struct semaphore sem_t;
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


struct ufs_stat;
MODULE(vfs) {
  void (*init)();
  int (*write)(int fd, void *buf, int count);
  int (*read)(int fd, void *buf, int count);
  int (*close)(int fd);
  int (*open)(const char *pathname, int flags);
  int (*lseek)(int fd, int offset, int whence);
  int (*link)(const char *oldpath, const char *newpath);
  int (*unlink)(const char *pathname);
  int (*fstat)(int fd, struct ufs_stat *buf);
  int (*mkdir)(const char *pathname);
  int (*chdir)(const char *path);
  int (*dup)(int fd);
};

typedef struct device device_t;
MODULE(dev) {
  void (*init)();
  device_t *(*lookup)(const char *name);
};
