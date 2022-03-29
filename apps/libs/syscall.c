#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ipc.h"
#include "nimbos.h"

static void nimbos_syscall_handler(int signum)
{
    struct ipc_buffer_entry entry;

    if (signum != NIMBOS_SYSCALL_SIG_NUM)
        return;
    while (pop_syscall_request(get_send_buffer(), &entry)) {
        printf("syscall: opcode=%d, args=0x%lx\n", entry.opcode, entry.args);
        switch (entry.opcode) {
        case IPC_OP_READ:
            break;
        case IPC_OP_WRITE:
            printf("write\n");
            break;
        default:
            break;
        }
    }
}

int nimbos_setup_syscall()
{
    int fd = open(NIMBOS_DEV, O_RDWR);
    if (fd <= 0) {
        return fd;
    }
    nimbos_setup_ipc_buffer(fd);
    ioctl(fd, NIMBOS_SETUP_SYSCALL);
    signal(NIMBOS_SYSCALL_SIG_NUM, nimbos_syscall_handler);
    return fd;
}
