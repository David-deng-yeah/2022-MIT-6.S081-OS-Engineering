#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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


#ifdef LAB_PGTBL
/*
* @ buf: the user's provided buffer's pointer
* @ num: number of page that we need to probe
* @ userspace: the user-level's destination that received the bitmask
*/
uint64
sys_pgaccess(void)
{ 
  // the starting virtual address
  uint64 va;// in real riscv system, address is 64bit, so uint64 is ok
  // the number of pages to check
  int num;
  //the user address of the buffer to store the results
  uint64 abits_addr;
  argaddr(0, &va);
  argint(1, &num);
  argaddr(2, &abits_addr);
  if(num < 0 || num > 32)
    return -1;
  
  uint32 bitmask = 0;
  pte_t *pte;
  struct proc *p = myproc();

  for(int i = 0; i < num; i++){
    if(va >= MAXVA)
      return -1;
    // find the i-th page in the pagetable, return the corresponding pte
    // 0 indicate not alloc the unmapped page
    pte = walk(p->pagetable, va, 0);
    
    if(pte == 0)
      return -1;
    /* if pte has been accessed add bit of ret and clear*/
    if(*pte & PTE_A){
      bitmask |= (1 << i);
      *pte ^= PTE_A;// clear PTE_A
    }
    /* va of next page */
    va += PGSIZE;    
  }

  if(copyout(p->pagetable, abits_addr, (char*)&bitmask, sizeof(bitmask)) < 0)
    return -1;
  return 0;
}
#endif

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
