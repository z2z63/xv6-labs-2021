#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("sleep: missing operand\n");
        exit(1);
    }
    int sleep_time = atoi(argv[1]);
    if (sleep_time < 0) {
        printf("sleep: invalid time interval '%s'\n", argv[1]);
        exit(1);
    }
    sleep(sleep_time * 10);
    exit(0);
}