#include <stdio.h>

#include "ipc.h"

static void *syscall_send_buf_base;
static void *syscall_recv_buf_base;
static void *syscall_data_buf_base;

void nimbos_setup_ipc_buffer(int nimbos_fd)
{
    syscall_send_buf_base = mmap(0, NIMBOS_SYSCALL_SEND_BUF_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_POPULATE, nimbos_fd, 0x1000);
    syscall_recv_buf_base = mmap(0, NIMBOS_SYSCALL_RECV_BUF_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_POPULATE, nimbos_fd, 0x2000);
    syscall_data_buf_base = mmap(0, NIMBOS_SYSCALL_DATA_BUF_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_POPULATE, nimbos_fd, 0);
    // printf("%p %p %p\n", syscall_send_buf_base, syscall_recv_buf_base,
    //        syscall_data_buf_base);

    assert(syscall_send_buf_base != MAP_FAILED);
    assert(syscall_recv_buf_base != MAP_FAILED);
    assert(syscall_data_buf_base != MAP_FAILED);
}

inline struct ipc_buffer *get_send_buffer()
{
    return (struct ipc_buffer *)syscall_send_buf_base;
}

static inline struct ipc_buffer_entry *entry_at(struct ipc_buffer *buf, int idx)
{
    return &buf->entries[idx & buf->capacity_mask];
}

static inline int entry_count(struct ipc_buffer *buf)
{
    return buf->tail - buf->head;
}

int pop_syscall_request(struct ipc_buffer *buf, struct ipc_buffer_entry *out_entry)
{
    spin_lock(&buf->lock);

    int ret = 0;
    if (entry_count(buf) > 0) {
        *out_entry = *entry_at(buf, buf->head);
        buf->head += 1;
        ret = 1;
    }

    spin_unlock(&buf->lock);
    return ret;
}
