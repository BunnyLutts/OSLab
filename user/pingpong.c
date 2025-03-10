#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int fildes[2];
    if (pipe(fildes) < 0) {
        printf("Error creating pipe\n");
        exit(-1);
    }

    int npid = fork();
    if (npid < 0) {
        printf("Error forking\n");
        exit(-1);
    } else if (npid == 0) { // Child process
        char byte;
        read(fildes[0], &byte, 1);
        int pid = getpid();
        printf("%d: received ping\n", pid);
        write(fildes[1], "", 1);
    } else { // Parent process
        char byte;
        write(fildes[1], "", 1);
        read(fildes[0], &byte, 1);
        int pid = getpid();
        printf("%d: received pong\n", pid);
    }

    exit(0);
}