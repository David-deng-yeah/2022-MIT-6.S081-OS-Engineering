# problem 1
Creating a user-level threading system in xv6 requires implementing context switching between threads. This involves saving and restoring registers when switching between threads and ensuring that each thread executes its function on its own stack. Here's a step-by-step guide on how to implement this:

1. **Understanding the Problem:**
   Before diving into code changes, make sure you understand the problem and the existing code. You should familiarize yourself with the provided `uthread.c`, `uthread_switch.S`, and `uthread.asm` files, which contain the user-level threading package and test threads.

2. **Add Register State to `struct thread`:**
   Modify the `struct thread` in `uthread.c` to include registers that need to be saved and restored during context switches. You'll typically need to save and restore callee-save registers, which include `s0`-`s11` and `ra`. You can use arrays or individual members for each register.

3. **Implement `thread_create()`:**
   Modify the `thread_create()` function to set up the initial stack for the new thread. You should allocate memory for the stack, initialize the stack pointer (`sp`), and place the function to be executed on the stack, along with any arguments it may need. Ensure that the registers are initialized properly on the new thread's stack.

4. **Implement `thread_switch()` in Assembly:**
   Create or modify the `thread_switch()` function in `uthread_switch.S` to save the state of the currently running thread (i.e., the thread calling `thread_switch()`) and restore the state of the next thread. This involves saving the callee-save registers and restoring them in the next thread. You can use the `sw` and `lw` instructions to save and load register values from memory.

5. **Call `thread_switch()` in `thread_schedule()`:**
   Modify the `thread_schedule()` function in `uthread.c` to call `thread_switch()`. Ensure that you pass the correct arguments to `thread_switch()` so that it knows which thread to switch to and from.

6. **Test Your Implementation:**
   Compile and run your modified xv6 with the `make qemu` command. Use the provided `uthread` program to test your user-level threading system. You should see the test threads executing and yielding the CPU to each other.

7. **Debugging with GDB:**
   Use GDB to debug your code. You can set breakpoints and single-step through the code to check if the registers are being saved and restored correctly during context switches. Follow the hints provided in the exercise for debugging.

8. **Iterate and Refine:**
   Test your code thoroughly, and if you encounter any issues, iterate on your implementation and refine it until it passes the `uthread` test.

9. **Ensure Proper Cleanup:**
   Implement thread cleanup logic to free the memory allocated for thread stacks and any other resources when threads exit. This will prevent memory leaks.

10. **Document Your Code:**
    Make sure your code is well-documented with comments explaining the purpose of each function and significant code blocks. This will make it easier for others (and yourself) to understand and maintain the code.

Remember that implementing a user-level threading system is a complex task, and it may take some trial and error to get it right. Be patient, and use debugging tools like GDB to help you identify and fix issues.


# xv6-handbook: chapter 7
[quote](https://zhuanlan.zhihu.com/p/353580321)

# note

## thread-xv6

### introduction
xv6 support kenrel-thread, for every user process, they all have a kenrel-thread to execute systemcall from user process. Every kernel-thread share the kernel-memory.

on the other hand, in xv6, for every user-process, they only have one user-thread, and every user-process have their own memory address space. But in other more complicated OS like Linux, they allow one user-process have multiple user-thread, and these user-thread can share memory (since they all belonging to the same user-process).

### xv6 thread-scheduling
scheduling means stop one thread and then execute another thread, and every CPU-core have its own thread-Scheduler in xv6.

when you want to schedule thread-A to thread-B, you have to saved thread-A's station so that you can restore it.

the best situtation is that one process voluntarily yield and let other thread can run, but for some compute-intensive thread, they would not voluntarily yield, so we create a mechanism called time-interupt

so maybe in some scenarios, your user space's process/thread are calculating pi, but as long as time-interupt happened, a system-call trap will be triggered and kernel will control the CPU, and then kernel will voluntarily give the control of CPU to the thread-scheduler, and all above is the flow of interupt.

there are two kinds of flow, one is pre-emptive-scheduling, just the one we have talk about in the paragraph above. The other one is voluntary scheduling.

In xv6, the thread-scheduling is implementing like that: using pre-emptive-scheduling to let clock-interupt tranfer the control of CPU from user-space to kernel, after that, using voluntary-scheduling that kernel give the control to thread-scheduler.

so, when we doing thread-scheduling, there are different kinds of thread:
* runnable: thread are not working, but it will work on CPU as soon as it can
* running: woring on CPU
* sleeping: these kinds of thread are waiting for I/O, they will only wokr aftern I/O complelte.

Attention!! when a thread running on cpu, its pc and register will be saved in the CPU hardware. In other hand, when a thread is runnable which means it doesn't working, its info will be saved in somewhere in memory.

so when scheduling, we have to retrieve the info of runnable thread storing in the memory.

## scheduling 
[quote](https://mit-public-courses-cn-translatio.gitbook.io/mit6-s081/lec11-thread-switching-robert/11.3-xv6-thread-switching-1)

every process have its own user-stack, when user-process is running, actually is its user-thread running.

if the program impl a syscall or interupt, its user-space will be saved to trapframe, and then this program's kernel-thread will be activated, and then CPU will be transfered to kernel-stack. After kernel-thread finished its work, when it returns to user space, the registers and some states will resotred from trapframe.

PAY FUCKING ATTENTION!! if XV5 kernel decide to schedule from process-A to process-B, firstly, process-A's kernel-thread-a's control to CPU will be transfered to process-B's kernel-thread-b, and then from kernel-thread-b restoring to process-B.

and the flow from kernel-thread-a to kernel-thread-b has serverl stage:
1. xv6 saved kernel-thread-a's registers and sth-else to process-A's context
2. and then CPU-control will transfer to kernel-thread-b, which means kernel-thread-b's state must be RUNNABLE, so it's registers state must be saved in process-B's context, the same as trapframe(contains process-B's user-space state). So at that time, xv6 will restore kernel-thread-b's register with process-B's context.
3. and then kernel-thread-b will continue its interupt work, and then restore trapframe's user-space state, returning to process-B in user-space.

so the main point is that at:
1. switching from one user-process to another requires accessing the kernel from the process-A, saving the state of the user process and running the kernel-thread-a
2. then switch from the kernel-thread-a to kernel-thread-b
3. after that, the kernel-thread-b suspends itself and resumes the user registers of the process-B
4. finally return to the process-B to continue execution.

AND AT THIS FLOW, SCHEDULING WILL HAPPEND!!!!


some key points:
* every CPU have a different scheduler-thread, and it's kernel-thread, which have its own context
* actually, kernl-therad have two kinds. one is like thread-a, which has user-process, and its context are saved in its user-process; the other one is like scheduler-thread, they don't have user-process, so they saved their context in its struct cpu.
* context can also saved in trapframe, since every user-process can and only have one kernel-thread, and they are one-to-one correspondence, so that's OK.
* every kernel-thread has its own kernel stack.
* every cpu-core will only do one thing at a time, and every cpu-core will only run one thread at a time, it's either a kernel-thread, a user-thread, or a scheduler thread.

some registers, like pc, sp and ra, is quite important:
* pc: program counter, points to the address of instruction currently executing
* sp: stack pointer, store teh top address of current process's stack
* ra: return address, store the return address of the caller

## code explantion
### the structure of context, cpu and trapframe
```c
// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
...
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

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
...
  /* 112 */ uint64 a0;
...
  /* 168 */ uint64 a7;
...
  /* 256 */ uint64 t3;
...
  /* 280 */ uint64 t6;
};
```


### the structure of process
```c
...

// Per-process state
struct proc {
  ...
  // saved current-process's kernel stack, in which the process saved function-call
  // when it executing in kernel.
  uint64 kstack; 

  // lock protect lots of data, at least protect updating state.
  // for instance, two cpu's scheduler-thread will not pull the same runnable-thread
  // to execute since the protection of the lock-filed.
  struct spinlock lock;

  // enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
  // state saved current process's state: runnable, running, sleeping
  enum procstate state; 

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  ...
  struct trapframe *trapframe; // this saved user-space-thread registers
  struct context context;      // this saved kernel-thread registers
  ...
};
```

Q: how to identify different process's kernel thread
A: any kernel thread can call myproc() to obtain the process running on current CPU. myproc() will use register tp to obtain current CPU's id, and use the id to find the corresponding struct proc in a struct-array containing all running process.
```c
// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}
```

yield() and sched() is in proc.c, which is about scheduling.
sched() do noting but checking sth, and after that it call switch() to schedule kernel-thread.
* process-A -> kernel-thread-a -> yield() -> sched() -> switch() -> kernel-thread-b -> process-B
* in switch's ra, it should point to sched()

Q: why RISC-V has 32 registers, but swtch() only saved and restore 14 registers?
A: since swtch() is called by C-code, we know that Caller-Saved-Register will be saved in current stack by C-Compiler, and the number of Caller-Saved-Register was about 15-18, and swtch() only need to saved registers which is useful for swtch and being not saved by C-compiler.
```c
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}
```
kernel swtch.S
before the program executing ret, if you use gdb to print the value of sp register, you will find that it contains the address of current process's kernel stack, which is mapped to a high address in Virtual Memory's stack0 area, which is actually very, very early in the startup sequence, start.S create stack in this place, so that OS can call the first C program, so scheduler-thread is running on the bootstack are in CPU.
```S
.globl swtch
swtch:
        sd ra, 0(a0)
        sd sp, 8(a0)
        sd s0, 16(a0)
        sd s1, 24(a0)
        sd s2, 32(a0)
        sd s3, 40(a0)
        sd s4, 48(a0)
        sd s5, 56(a0)
        sd s6, 64(a0)
        sd s7, 72(a0)
        sd s8, 80(a0)
        sd s9, 88(a0)
        sd s10, 96(a0)
        sd s11, 104(a0)

        ld ra, 0(a1)
        ld sp, 8(a1)
        ld s0, 16(a1)
        ld s1, 24(a1)
        ld s2, 32(a1)
        ld s3, 40(a1)
        ld s4, 48(a1)
        ld s5, 56(a1)
        ld s6, 64(a1)
        ld s7, 72(a1)
        ld s8, 80(a1)
        ld s9, 88(a1)
        ld s10, 96(a1)
        ld s11, 104(a1)
        
        ret
```

scheduler() in proc.c

there are many steps involved in ceding CPU:
1. 

```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}
```