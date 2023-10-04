#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

//implement of sys_trace
uint64
sys_trace(void)
{
  // store the mask argument in the proc struct
  // to enable tracing for the current process 
  // and its children
  int mask;
  argint(0, &mask);
  // store the trace mask in the proc structure
  myproc()->trace_mask = mask;
  return 0; //success
}

// implement of sysinfo
uint64
sys_sysinfo(void)
{
  // get a address of user-level
  uint64 addr;
  argaddr(0, &addr);

  struct sysinfo info;
  info.freemem = freemem();
  info.nproc = numprocs();
  // ensure that info points to valid user space memory
  // copy &info to addr, so user can use it
  if(copyout(myproc()->pagetable, addr, (char *)&info, sizeof(info)) < 0){
    return -1; 
  } 
  return 0; // success
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
