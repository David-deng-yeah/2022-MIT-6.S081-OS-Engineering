#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N 35

// void sieve(int l_fd, int r_fd, int prime)
// {
//     int num;
//     while(read(l_fd, &num, sizeof(int)) > 0){// l_fd not close
//         if(num % prime != 0){
//             printf("prime %d\n", num);
//         }
//     }
//     close(l_fd);
//     exit(0);
// }

void sieve(int p[2]){
    int res;
    read(p[0], &res, sizeof(int));
    if(res == -1){
        exit(0);
    }
    printf("prime %d\n", res);

    int p2[2];
    pipe(p2);
    if(fork() == 0){// children process
        close(p2[1]);
        close(p[0]);
        sieve(p2);
    } else {// parent process
        close(p2[0]);// not read
        int buf;
        while(read(p[0], &buf, sizeof(int)) && buf != -1){
            if(buf % res != 0){
                write(p2[1], &buf, sizeof(int));
            }
        }
        buf = -1;
        write(p2[1], &buf, sizeof(int));
        wait(0);
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    int p[2];
    // if(pipe(p) == -1){
    //     fprintf(1, "error, pipe creation fail\n");
    //     exit(1);
    // }
    pipe(p);

    if(fork() == 0){// child process
        close(p[1]);
        //int prime;
        //read(p[0], &prime, sizeof(int));
        //printf("prime %d\n", prime);

        sieve(p);

        // int p2[2];
        // while(read(p[0], &prime, sizeof(int)) > 0){
        //     pipe(p2);
        //     if(fork() == 0){// child process
        //         close(p2[1]);// close write
        //         sieve(p[0], p2[1], prime);// 
        //     } else { // parent process
        //         close(p2[0]);
        //         write(p2[1], &prime, sizeof(int));
        //         wait(0);
        //     }
        // }
        // close(p[0]);

        exit(0);
    } else {// parent process
        close(p[0]);
        int i;
        for(i=2; i <= N; i++){
            write(p[1], &i, sizeof(int));
        }
        i = -1;
        write(p[1], &i, sizeof(int));
        close(p[1]);
        // wait(0);
        // exit(0);
    }
    wait(0);
    exit(0);
}
