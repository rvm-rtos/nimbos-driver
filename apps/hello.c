#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "nimbos.h"

void syscall_handler(int signum)
{
    printf("SIG %d\n", signum);
}

int main()
{
    printf("Hello NimbOS!\n");

    int fd = open(NIMBOS_DEV, O_RDONLY);
    if (fd <= 0) {
        printf("Failed to open NimbOS device `%s`\n", NIMBOS_DEV);
        return fd;
    }
    register_nimbos_syscall_handler(fd, syscall_handler);

    printf("%d\n", fd);
    for (int i = 0; i < 100; i++) {
        printf("Sleep %d...\n", i);
        usleep(1000000);
    }

    close(fd);
    return 0;
}
