#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p2c[2], c2p[2];
    if (pipe(p2c) < 0) {
        printf("Error creating pipe\n");
        exit(-1);
    }
    if (pipe(c2p) < 0) {
        printf("Error creating pipe\n");
        exit(-1);
    }

    int npid = fork();
    if (npid < 0) {
        printf("Error forking\n");
        exit(-1);
    } else if (npid == 0) { // Child process
        char byte;
        read(p2c[0], &byte, 1);
        int pid = getpid();
        printf("%d: received ping\n", pid);
        write(c2p[1], "", 1);
    } else { // Parent process
        char byte;
        write(p2c[1], "", 1);
        read(c2p[0], &byte, 1);
        int pid = getpid();
        printf("%d: received pong\n", pid);
    }

    exit(0);
}