#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];
    if(pipe(p) == -1){
        fprintf(1, "error, pipe creation failed\n");
        exit(1);
    }
    char byte = 'a';
    int pid = fork();
    if(pid == -1){
        fprintf(2, "error, fork failed\n");
        exit(1);
    }
    if(pid == 0){// children process
        close(p[1]);
        read(p[0], &byte, 1);// read from the pipe
        close(p[0]);
        printf("%d: received ping\n", getpid());
        exit(0);
    } else {// parent process
        close(p[0]);
        write(p[1], &byte, 1);// write to the pipe
        close(p[1]);
        int status; // variable to hold child process exit status
        wait(&status);// wait for the child to exit and obtain its exit status
        printf("%d: received pong\n", getpid());
        exit(0);
    }
    exit(0);
}
