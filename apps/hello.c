#include <stdio.h>
#include <unistd.h>

#include <nimbos.h>

int main()
{
    printf("Hello NimbOS!\n");

    int fd = nimbos_setup_syscall();
    if (fd <= 0) {
        printf("Failed to open NimbOS device `%s`\n", NIMBOS_DEV);
        return fd;
    }

    for (;;) {
        // printf("Sleep %d...\n", i);
        usleep(1000);
    }

    close(fd);
    return 0;
}
