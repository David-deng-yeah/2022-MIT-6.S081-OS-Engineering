# macros of kernel/riscv.h
```c
#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

#define PTE_V (1L << 0) // valid
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4) // user can access

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// extract the three 9-bit page table indices from a virtual address.
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64) (va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
```
kernel/vm.c freewalk()
```c
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}
```
# struct trapframe
the trapframe struct is used to store the state of a process when it enters the trap handling code during a system call or an exception in xv6

a0 ~ a1: argument registers, which typically hold the arguments for system calls

```c
// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};
```

# exercise 3

user/pgtlbtesdt.c pgaccess_tyest()

```c
void
pgaccess_test()
{
  char *buf;
  unsigned int abits;
  printf("pgaccess_test starting\n");
  testname = "pgaccess_test";
  buf = malloc(32 * PGSIZE);
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");
  buf[PGSIZE * 1] += 1;
  buf[PGSIZE * 2] += 1;
  buf[PGSIZE * 30] += 1;
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");
  if (abits != ((1 << 1) | (1 << 2) | (1 << 30)))
    err("incorrect access bits set");
  free(buf);
  printf("pgaccess_test: OK\n");
}

```

kernel/syscall.c argaddr() argraw() argint()
```c
static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}
```

kernel/vm.c copyout() copyin()
```c
// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.

/*
* @ pagetable: a pointer to the pagetable that represents the user's address
* space
* @ dstva: the destination address in the user's address space
* where the data should be copied
* @ src: a pointer to the data in the kernel space that should be
* copied to the user space 
*/
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    // walkaddr look all pte in the pagetable and return the PA or 0 if not mapped
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    // the number of bytes that can be copied to the current page
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    // executing copy
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}
```

kernel/vm.c walk()

walk traverse a pagetable and find the pte that corresponds to 
a given VA. (it can also allocate new pagetable pages)

```c
// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];// pagetable entry index
    if(*pte & PTE_V) {// check if pte is valid
      // use PTE2PA to extract the next-level pagetable's pointer
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      // if the pte is not valid and the alloc is non-zero
      // walk() will allocate a new page usint kalloc()
      // and memset it to all-zero
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  // finally, walk() returns a pointer to the page table entry
  // that corresponds to the lowest-level page and the provided
  // virtual address va
  return &pagetable[PX(0, va)];
}
```

## procedure
1. Define PTE_A:
    * In kernel/riscv.h, define the access bit PTE_A as specified in the RISC-V privileged architecture manual. Typically, PTE_A is a bit in the page table entry (PTE) that is set by the hardware when a page is accessed (read or write). You need to consult the manual to determine the correct value for PTE_A.
2. Implement sys_pgaccess():
    * In kernel/sysproc.c, implement the sys_pgaccess() system call function. This function will be responsible for handling the pgaccess() system call. It should take the following steps:
    * Use argaddr() and argint() to parse the arguments provided by the user. You need to extract the starting virtual address, the number of pages to check, and the user address of the buffer to store the results.
    * Allocate a temporary buffer in the kernel to store the bitmask that represents which pages have been accessed. The size of this buffer depends on the number of pages to check.
    * Use a loop to iterate over the specified range of virtual addresses, starting from the provided starting virtual address.
    * For each virtual address, use the walk() function in kernel/vm.c to find the corresponding PTE in the page table.
    * Check if the PTE_A bit is set in the PTE. If it is set, mark the corresponding bit in the bitmask as accessed.
    * After scanning all the specified pages, you should have a filled bitmask that represents accessed pages.
    * Use the copyout() function to copy the bitmask from the kernel to the user's provided buffer.
3. Clear PTE_A:
    * After checking whether the PTE_A bit is set for each page, make sure to clear the PTE_A bit in the PTE. This is essential to ensure that you can detect page accesses since the last call to pgaccess(). If you don't clear this bit, it will remain set indefinitely.
4. Testing:
    * Create a test case for pgaccess() in the user/pgtlbtest.c file, similar to the pgaccess_test() function. This test case should call pgaccess() and verify that the returned bitmask matches the expected results.
5. Debugging (optional):
    * You can use the vmprint() function, as mentioned in your hints, to debug the page tables and verify that the access bits are correctly set and cleared.