#include "kernel/types.h"
#include "user/user.h"

void xargs(char *argv[]) {
    int pid = fork();
    if (pid == 0) {
        exec(argv[0], argv);
    } else {
        wait(0);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: xargs <command>\n");
        exit(0);
    }
    char buf[512];
    gets(buf, sizeof(buf));
    while (buf[0]!='\0'){
        char **new_argv = malloc(sizeof(char *) * (argc + 1));
        for (int i = 1; i < argc; i++) {
            new_argv[i - 1] = argv[i];
        }
        new_argv[argc - 1] = buf;
        new_argv[argc] = 0;
        xargs(new_argv);

        gets(buf, sizeof(buf));
    }


    exit(0);
}