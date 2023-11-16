#include "kernel/types.h"
#include "user/user.h"


/**
 * 先发起ping，然后收到pong
 * */
void parent(int w_fd, int r_fd) {
    write(w_fd, "ping", w_fd);

    char buf[20];
    read(r_fd, buf, sizeof(buf));
    printf("%d: received pong\n", getpid());
}

/**
 * 先收到ping，然后发起pong
 * */

void child(int w_fd, int r_fd) {
    char buf[20];
    read(r_fd, buf, sizeof(buf));
    printf("%d: received ping\n", getpid());

    write(w_fd, "pong", w_fd);
}

/**
 * parent: p->c
 * child:  c->p
 * */
int main(int argc, char *argv[]) {
    int p2c[2];
    int c2p[2];
    pipe(p2c);
    pipe(c2p);
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(0);
    } else if (pid == 0) {

        int r_fd = dup(p2c[0]);


        int w_fd = dup(c2p[1]);

        close(p2c[1]);
        close(c2p[0]);

        child(w_fd, r_fd);

        close(p2c[0]);
        close(c2p[1]);
    } else {

        int r_fd = dup(c2p[0]);


        int w_fd = dup(p2c[1]);

        close(p2c[0]);
        close(c2p[1]); // 关闭不需要的fd

        parent(w_fd, r_fd);

        close(p2c[1]);
        close(c2p[0]);
    }
    exit(0);
}
