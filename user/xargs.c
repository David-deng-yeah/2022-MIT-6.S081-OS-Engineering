#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

#define stdin_ 0
/*
* xargs read line from stdin
* like: ls | xargs echo hello, stdin contains the result of ls,
* and argv = [xargs, echo, hello]
* and the execution is echo hello <result_stdin>
*/

int main(int argc, char *argv[])
{
    char *args[MAXARG+1];// [[], [], []]
    char buf[2048];
    if (argc < 2){
        fprintf(2, "xargs, failed , too less args");
        exit(1);
    }
    // copy command into the argv array
    int index = 0;
    for (int i=1; i<argc; i++){
        args[index++] = argv[i];
    }
    char *p = buf;
    // read lines frim stdin and run the command for each line
    // stdin is fd-0, so just read(0, ..., ...)
    while(read(stdin_, p, 1) == 1){// n is the bytes readed
        if(*p == '\n'){
            *p = 0;
            // fork a child to process to run the command
            if(fork() == 0){// child
                args[index] = buf;
                exec(argv[1], args);
                fprintf(2, "xargs, exec failed");
                exit(1);
            } else {// parent
                wait(0);
                p = buf;
            }
        } else {
            ++p;
        }
    }
    exit(0);
}

/* test sh
mkdir a
echo hello > a/b
mkdir c
echo hello > c/b
echo hello > b
find . b | xargs grep hello
*/