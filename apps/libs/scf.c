#include <stdio.h>
#include <sys/errno.h>

#include "scf.h"

#define ALIGN_UP(addr, align) ((addr + align - 1) & ~(align - 1))

static void *syscall_data_buf_base;
static void *syscall_queue_buf_base;

struct syscall_queue_buffer g_syscall_queue_buffer;

int nimbos_setup_syscall_buffers(int nimbos_fd)
{
    syscall_data_buf_base = mmap(0, NIMBOS_SYSCALL_DATA_BUF_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_POPULATE, nimbos_fd, 0);
    if (syscall_data_buf_base == MAP_FAILED) {
        return -ENOMEM;
    }

    syscall_queue_buf_base = mmap(0, NIMBOS_SYSCALL_QUEUE_BUF_SIZE, PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_POPULATE, nimbos_fd, 0x1000);
    if (syscall_queue_buf_base == MAP_FAILED) {
        return -ENOMEM;
    }
    // printf("%p %p\n", syscall_data_buf_base, syscall_queue_buf_base);

    struct syscall_queue_buffer_metadata *meta = syscall_queue_buf_base;
    struct scf_descriptor *desc;
    uint16_t *req_ring, *rsp_ring;
    uint16_t capacity = meta->capacity;

    // printf("%x %d %d %d %d %d\n", meta->magic, meta->capacity, meta->lock,
    // meta->req_index, meta->rsp_index);

    if (meta->magic != SYSCALL_QUEUE_BUFFER_MAGIC) {
        return -EINVAL;
    }
    if (!capacity || (capacity & (capacity - 1)) != 0) {
        return -EINVAL;
    }

    desc = (void *)meta + ALIGN_UP(sizeof(struct syscall_queue_buffer_metadata), 8);
    req_ring = (void *)desc + capacity * sizeof(struct scf_descriptor);
    rsp_ring = (void *)req_ring + capacity * sizeof(uint16_t);

    g_syscall_queue_buffer = (struct syscall_queue_buffer){
        .capacity_mask = capacity - 1,
        .req_index_last = 0,
        .rsp_index_shadow = meta->rsp_index,
        .meta = meta,
        .desc = desc,
        .req_ring = req_ring,
        .rsp_ring = rsp_ring,
    };

    return 0;
}

inline void *offset_to_ptr(uint64_t offset)
{
    return syscall_data_buf_base + offset;
}

inline struct syscall_queue_buffer *get_syscall_queue_buffer()
{
    return &g_syscall_queue_buffer;
}

static inline int has_request(struct syscall_queue_buffer *buf)
{
    return buf->req_index_last != buf->meta->req_index;
}

struct scf_descriptor *get_syscall_request_from_index(struct syscall_queue_buffer *buf,
                                                      uint16_t index)
{
    if (index > buf->capacity_mask) {
        return NULL;
    }
    return &buf->desc[index];
}

int pop_syscall_request(struct syscall_queue_buffer *buf, uint16_t *out_index,
                        struct scf_descriptor *out_desc)
{
    int err;
    spin_lock(&buf->meta->lock);
    // printf("pop_syscall_request %d %d\n", buf->req_index_last, buf->meta->req_index);

    if (has_request(buf)) {
        __sync_synchronize();
        uint16_t idx = buf->req_ring[buf->req_index_last & buf->capacity_mask];
        if (idx > buf->capacity_mask) {
            err = -EINVAL;
            goto end;
        }
        *out_index = idx;
        *out_desc = buf->desc[idx];
        buf->req_index_last += 1;
        err = 0;
    } else {
        err = -EBUSY;
    }

end:
    spin_unlock(&buf->meta->lock);
    return err;
}

int push_syscall_response(struct syscall_queue_buffer *buf, uint16_t index,
                          uint64_t ret_val)
{
    int err;
    spin_lock(&buf->meta->lock);

    if (index > buf->capacity_mask) {
        err = -EINVAL;
        goto end;
    }

    buf->desc[index].ret_val = ret_val;
    buf->rsp_ring[buf->rsp_index_shadow & buf->capacity_mask] = index;
    buf->rsp_index_shadow++;
    __sync_synchronize();
    buf->meta->rsp_index = buf->rsp_index_shadow;
    err = 0;

end:
    spin_unlock(&buf->meta->lock);
    return err;
}
