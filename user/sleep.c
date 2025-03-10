#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(1, "Usage: sleep NUMBER\n", 21);
        exit(-1);
    }

    int seconds = atoi(argv[1]);

    sleep(seconds);

    exit(0);
}