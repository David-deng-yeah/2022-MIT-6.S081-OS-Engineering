# SysCall
done
1. Add Trace System Call in User Space:
    * In the user-level code, add a new system call named trace in the user/trace.c file. (wrong, nothing need to do)
    * This system call should take an integer mask as an argument, which will specify which system calls to trace.

done
2. Define the Trace System Call in Kernel Space:
    * Add a prototype for the trace system call in user/user.h.
    * Define a syscall number for trace in kernel/syscall.h.
    * Create a system call stub for trace in user/usys.pl to map the system call to its number.

done
3. Implement sys_trace in kernel/sysproc.c:
    * Implement the sys_trace function in kernel/sysproc.c. This function should store the mask argument in the proc structure to enable tracing for the current process and its children.

done
4. Modify fork to Copy Trace Mask:
    * In the fork function in kernel/proc.c, add code to copy the trace mask from the parent process to the child process when a new process is created using fork.

done
5. Modify syscall in kernel/syscall.c:
    * Modify the syscall function in kernel/syscall.c to print trace output before each system call returns if the corresponding bit is set in the trace mask.
    * You will need to create an array of syscall names to index into when printing the trace output. You can define this array in the same file.
    * The trace output should include the process ID, the name of the system call, and the return value.

done
6. Compile and Test:
    * Make sure to add $U/_trace to UPROGS in the Makefile to compile the trace program.
    * Compile your modified xv6 kernel.
    * Run make qemu to boot the xv6 OS and test the trace program.
    * Run test cases similar to the ones mentioned in your assignment prompt to verify that system call tracing is working as expected.

7. Debug and Refine:
    * Debug any issues that may arise during testing and refine your code as needed.
    * Pay attention to error messages, incorrect behavior, and potential race conditions.
8. Submit:
    * Once you have successfully implemented and tested the system call tracing feature, submit your modified xv6 code for evaluation.

# SysInfo

kernel/sysfile.c
```c
uint64
sys_fstat(void)
{
  struct file *f;
  uint64 st; // user pointer to struct stat

  if(argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
    return -1;
  return filestat(f, st);
}
```

kernel/file.c
```c
// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
// this function is typically used when a user-level program 
// wants to obtain file info, like file-size, permissions, etc.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    // it locks the corresponding inode to prevent concurrent  
    // access to the inode
    ilock(f->ip);
    // retrieving the file's attributes and stores them in the `st`
    // structure
    stati(f->ip, &st);
    iunlock(f->ip);// unlock
    // Copy from kernel to user.
    // Copy len bytes from src to virtual address dstva in a given 
    // page table.
    // Return 0 on success, -1 on error.
    // @addr: the user-space address
    // usage: copies the stat to the user space address
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;// if the file is not a regular file or device file or any errors happended
    return 0;
  }
  return -1;
}
```

sysinfo
```c
struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};
```