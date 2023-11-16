#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/stat.h"


void find(char *path, char *filename) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        exit(0);
    }
    struct dirent de;
    while (1) {
        int n = read(fd, &de, sizeof(struct dirent));
        if (n != sizeof(struct dirent)) {
            close(fd);
            return;
        }
        if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }
        char buf[256];
        char *p = buf;
        strcpy(p, path);
        p += strlen(path);
        *p++ = '/';
        strcpy(p, de.name);
        p += strlen(de.name);
        *p++ = '\0';
        int fd1 = open(buf, O_RDONLY);
        if (fd1 < 0) {
            fprintf(2, "find: cannot open %s\n", buf);
            close(fd1);
            return;
        }
        struct stat st;
        if (fstat(fd1, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            close(fd1);
            return;
        }
        close(fd1);
        if (st.type == T_DIR && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0) {
            find(buf, filename);    // 递归查找目录下的文件
        } else if (st.type == T_FILE) {
            if (strcmp(de.name, filename) == 0) {
                printf("%s\n", buf);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: find <path> <filename>\n");
        exit(0);
    }
    find(argv[1], argv[2]);
    exit(0);
}
