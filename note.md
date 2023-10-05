# questions
* Which registers contain arguments to functions? For example, which register holds 13 in main's call to printf?
    * In RISC-V calling conventions, function arguments are passed in registers a0, a1, a2, a3, etc. In the case of the call to printf in main, the value 13 is passed as an argument, so it will be in register a2.

* Where is the call to function f in the assembly code for main? Where is the call to g? (Hint: the compiler may inline functions.)
    * The call to function f in the assembly code for main is at address 24. It's done using a jalr (jump and link register) instruction to a target address calculated as an offset from the ra (return address) register.
    * The call to function g is also inlined within function f. You can see it at address 14, again using a jalr instruction to a target address calculated from the a0 register.

* At what address is the function printf located?
    * The address of the function printf is not explicitly mentioned in the assembly code, but it is indirectly called through the jalr instruction at address 30. The target address is calculated using the auipc and addi instructions.

* What value is in the register ra just after the jalr to printf in main?
    * The value in the register ra just after the jalr to printf in main is the address of the instruction immediately following the jalr instruction. In this case, it would be address 38, which is the start of the exit(0) code.

* Run the provided code:
    ```c
    unsigned int i = 0x00646c72;
    printf("H%x Wo%s", 57616, &i);
    ```
    * The output depends on little-endian encoding. Here's the explanation:
    * The hexadecimal value 0x00646c72 is little-endian, so it's stored as bytes in memory as 72 6C 64 00.
    * 57616 in hexadecimal is 0xE110. When formatted as %x, it remains the same.
    * &i points to the address where the little-endian bytes 72 6C 64 00 are stored.
    * So, the output will be: Hx Wo�ld

* In the following code, what is going to be printed after 'y='? Why does this happen?
    ```c
    printf("x=%d y=%d", 3);
    ```
    * In the code provided, the printf function is given two format specifiers (%d) but only one argument (3). This will cause printf to attempt to read two integers (%d placeholders) from the argument list, but there's only one integer (3) provided. This will result in undefined behavior, and the output is unpredictable. The behavior can vary depending on the compiler and system, and it may print garbage values or crash.


# hard questions
Implementing a feature like sigalarm in the xv6 operating system involves multiple steps and modifications to various parts of the operating system. Below is a high-level outline of the steps you need to take to add sigalarm support to xv6:

done
1. **Modify the Makefile**:
   - Update the Makefile to include `alarmtest.c` so that it gets compiled as an xv6 user program.

done
2. **Add System Call Declarations**:
   - In `user/user.h`, declare the new system calls `sigalarm` and `sigreturn` as follows:
     ```c
     int sigalarm(int ticks, void (*handler)());
     int sigreturn(void);
     ```
done
3. **Update syscall Handler**:
   - Modify `user/usys.pl` to generate the system call numbers for `sigalarm` and `sigreturn`.
   - Update `kernel/syscall.h` to include the declarations of the new system calls.
   - Modify `kernel/syscall.c` to handle the `sigalarm` and `sigreturn` system calls. For now, you can have `sys_sigreturn` return zero.

done
4. **Extend `struct proc`**:
   - Add fields to `struct proc` in `kernel/proc.h` to store the alarm interval, a pointer to the handler function, and a counter to keep track of the ticks passed.
done
5. **Initialize Fields**:
   - Initialize the new fields in `struct proc` during process creation in `kernel/proc.c`, specifically in the `allocproc` function.
done
6. **Handle Timer Interrupts**:
   - In `kernel/trap.c`, modify the `usertrap` function to check for timer interrupts. You should only manipulate a process's alarm ticks if there's a timer interrupt. For example:
     ```c
     if (which_dev == 2) {
       // Handle timer interrupt
       // Check and invoke the alarm function if necessary
     }
     ```
done
7. **Execute Alarm Function**:
   - When the timer interrupt occurs, and it's time to invoke the alarm function, execute the handler function specified by the process. Make sure to handle cases where the address of the user's alarm function might be zero.

8. **Context Switching and Register State**:
   - To ensure proper context switching and to return to the interrupted code correctly, you need to save and restore the necessary register states in `usertrap` and `sys_sigreturn`. The registers you need to save and restore will depend on what the handler function expects.

9. **Prevent Reentrant Calls**:
   - Ensure that reentrant calls to the handler are prevented. You can do this by setting a flag when the handler is called and clearing it when it returns. Avoid calling the handler again if the flag is already set.

10. **Re-arm the Alarm Counter**:
    - After each time the alarm goes off, you should re-arm the alarm counter based on the specified interval so that the handler is called periodically.

11. **Implement `sigreturn`**:
    - Implement the `sigreturn` system call to return control to the interrupted user code after the handler has finished executing. Restore the necessary register states, including `a0`, which holds the system call return value.

12. **Testing**:
    - Test your implementation with `alarmtest` to ensure that it prints "alarm!" correctly and that the tests pass as described in the exercise prompt.
    - Run `usertests -q` to check if your changes have not broken other parts of the kernel.

Please note that implementing sigalarm in xv6 is a non-trivial task that involves significant modifications to various parts of the operating system. Careful debugging and testing will be essential to ensure correct behavior.