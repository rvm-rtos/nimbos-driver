#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "nimbos.h"
#include "scf.h"

struct read_write_args {
    int fd;
    uint64_t buf_offset;
    uint64_t len;
};

static void *read_thread_fn(void *arg)
{
    struct read_write_args *args;
    struct syscall_queue_buffer *scf_buf = get_syscall_queue_buffer();
    uint16_t desc_index = (uint16_t)(long)arg;
    struct scf_descriptor *desc = get_syscall_request_from_index(scf_buf, desc_index);

    if (!desc) {
        return NULL;
    }

    args = offset_to_ptr(desc->args);
    char *buf = offset_to_ptr(args->buf_offset);
    int ret = read(args->fd, buf, args->len);
    // assert(ret == args->len);
    push_syscall_response(scf_buf, desc_index, ret);
    return NULL;
}

static void poll_requests(void)
{
    uint16_t desc_index;
    struct scf_descriptor desc;
    struct syscall_queue_buffer *scf_buf = get_syscall_queue_buffer();
    pthread_t thread; // FIXME: use global threads pool

    while (!pop_syscall_request(scf_buf, &desc_index, &desc)) {
        // printf("syscall: desc_index=%d, opcode=%d, args=0x%lx\n", desc_index,
        // desc.opcode, desc.args);
        switch (desc.opcode) {
        case IPC_OP_READ: {
            pthread_create(&thread, NULL, read_thread_fn, (void *)(long)desc_index);
            break;
        }
        case IPC_OP_WRITE: {
            struct read_write_args *args = offset_to_ptr(desc.args);
            char *buf = offset_to_ptr(args->buf_offset);
            int ret = write(args->fd, buf, args->len);
            assert(ret == args->len);
            push_syscall_response(scf_buf, desc_index, ret);
            break;
        }
        default:
            break;
        }
    }
}

static void nimbos_syscall_handler(int signum)
{
    if (signum == NIMBOS_SYSCALL_SIG_NUM) {
        poll_requests();
    }
}

int nimbos_setup_syscall()
{
    int fd = open(NIMBOS_DEV, O_RDWR);
    if (fd <= 0) {
        return fd;
    }
    int err = nimbos_setup_syscall_buffers(fd);
    if (err) {
        fprintf(stderr, "Failed to setup syscall buffers: %d\n", err);
        return err;
    }

    ioctl(fd, NIMBOS_SETUP_SYSCALL);
    signal(NIMBOS_SYSCALL_SIG_NUM, nimbos_syscall_handler);

    // handle requests before app starting
    poll_requests();

    return fd;
}
