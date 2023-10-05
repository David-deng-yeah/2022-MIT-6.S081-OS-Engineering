#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

/*
  * void(*handler)() is a function pointer declaration
  * void: this indicate that the function doesn't return any value
  * (*)(): this part specifies that `handler` is a pointer to a function
  *   1. (): this means that the function can take any number and type of arguments
  * so, void(*handler)() is a declartion for a function pointer that can point to function
  * with any arugument list or no argument
  */
uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 fn;
  argint(0, &ticks);
  argaddr(1, &fn);
  struct proc *p = myproc();
  p->alarm_ticks = ticks;
  // p->alarm_handler = (void(*)())(fn);
  p->alarm_handler = fn;
  p->have_return = 1;
  return 0;
  //return sigalarm(ticks, (void(*)())(fn));
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  // restore trapframe
  //memmove(p->trapframe, p->trapframe_backup, sizeof(struct trapframe));
  *p->trapframe = p->trapframe_backup;
  // reset the ticks->0
  //p->alarm_ticks = 0;
  //p->alarm_ticks_passed = 0;
  //p->alarm_handler = 0;
  p->have_return = 1;
  //return 0;
  return p->trapframe->a0;
  //return sigreturn();
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  // call backtree()
  backtree();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
