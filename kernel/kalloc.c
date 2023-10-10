// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

// assign a separate freelist to each CPU and protect it with separate locks
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

#define MAX_LOCK_NAME 16
char kmem_locks_name[NCPU][MAX_LOCK_NAME];

void
kinit()
{
  // call initlock for every cpu's lock
  for(int i=0; i<NCPU; i++){
    char* kmem_lock_name = kmem_locks_name[i];
    snprintf(kmem_lock_name, MAX_LOCK_NAME - 1, "kmem_cpu_%d", i);
    initlock(&kmem[i].lock, kmem_lock_name);
  }
  // initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  // shunt down interrput since this operation is not atomically
  push_off();
  int cpu = cpuid();
  pop_off();

  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_cpu(p, cpu);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  // struct run *r;

  // if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
  //   panic("kfree");

  // // Fill with junk to catch dangling refs.
  // memset(pa, 1, PGSIZE);

  // r = (struct run*)pa;

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);
  push_off();
  int cpu = cpuid();
  pop_off();
  kfree_cpu(pa, cpu);
}

void kfree_cpu(void *pa, int cpu){
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);
  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

/*kallock V2.0*/
void* kalloc(){
  struct run *r;

  push_off();// turn off interrupt
  int cpu = cpuid();
  acquire(&kmem[cpu].lock);
  if(!kmem[cpu].freelist){// if current cpu's freelist is NULL
    int steal_pages_num = 64;// decide arbitrarily
    for(int i=0; i<NCPU; i++){
      if(i == cpu)
        continue;
      acquire(&kmem[i].lock);
      struct run *tmp_R = kmem[i].freelist;
      while(tmp_R && steal_pages_num){
        kmem[i].freelist = tmp_R->next;
        tmp_R->next = kmem[cpu].freelist;
        kmem[cpu].freelist = tmp_R;
        tmp_R = kmem[i].freelist;
        steal_pages_num--;
      }
      release(&kmem[i].lock);
      if(steal_pages_num == 0)break;// done stealing
    }
  }
  // after stealling
  r = kmem[cpu].freelist;
  if(r){
    kmem[cpu].freelist->next = r->next;
  }
  release(&kmem[cpu].lock);
  pop_off();// restore interrupt

  if(r){
    memset((char*)r, 5, PGSIZE);
  }
  return (void*)r;
}

/*kalloc v1.0*/
/*
* when current cpu's freelist is NULL, it will let other cpu allocate mem
* but after that this cpu's freelist is still NULL, so "stealing" will happend often
* that is not effective
*/
// void *
// kalloc(void)
// {
//   struct run *r;

//   // acquire(&kmem.lock);
//   // r = kmem.freelist;
//   // if(r)
//   //   kmem.freelist = r->next;
//   // release(&kmem.lock);

//   push_off();
//   int cpu = cpuid();
//   pop_off();
  
//   acquire(&kmem[cpu].lock);
//   r = kmem[cpu].freelist;
//   if(r){
//     kmem[cpu].freelist = r->next;
//     release(&kmem[cpu].lock);
//   } else {// steal
//     // we can't try to acquire another cpu free list lock
//     // without releasing the current cpu freelist lock,
//     // which is a classic deadlock situation caused by a 
//     // staggered locking sequence.
//     release(&kmem[cpu].lock);
//     // current cpu's freelist is NULL, let's probe others cpu's
//     for(int i=0; i<NCPU; i++){
//       if(i == cpu)
//         continue;
//       acquire(&kmem[i].lock);
//       r = kmem[i].freelist;
//       if(r){
//         kmem[i].freelist = r->next;
//         release(&kmem[i].lock); 
//         break;
//       }
//       release(&kmem[i].lock);
//     }
//   }

//   if(r)
//     memset((char*)r, 5, PGSIZE); // fill with junk
//   return (void*)r;
// }
