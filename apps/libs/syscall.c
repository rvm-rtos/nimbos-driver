#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ipc.h"
#include "nimbos.h"

struct read_write_args {
    int fd;
    uint64_t buf_offset;
    uint64_t len;
};

static void nimbos_syscall_handler(int signum)
{
    struct ipc_buffer_entry entry;

    if (signum != NIMBOS_SYSCALL_SIG_NUM)
        return;
    while (pop_syscall_request(get_send_buffer(), &entry)) {
        // printf("syscall: opcode=%d, args=0x%lx\n", entry.opcode, entry.args);
        switch (entry.opcode) {
        case IPC_OP_READ: {
            struct read_write_args *args = offset_to_ptr(entry.args);
            char *buf = offset_to_ptr(args->buf_offset);
            int ret = read(args->fd, buf, args->len);
            assert(ret == args->len);
            break;
        }
        case IPC_OP_WRITE: {
            struct read_write_args *args = offset_to_ptr(entry.args);
            char *buf = offset_to_ptr(args->buf_offset);
            int ret = write(args->fd, buf, args->len);
            assert(ret == args->len);
            break;
        }
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
