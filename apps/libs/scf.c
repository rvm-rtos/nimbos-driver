#include <stdio.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include "scf.h"
#include "remap.h"
#include "nimbos.h"

#define ALIGN_UP(addr, align) ((addr + align - 1) & ~(align - 1))

static void *syscall_queue_buf_base;

struct syscall_queue_buffer g_syscall_queue_buffer;
int g_nimbos_fd;
int g_slot_num;

inline int *get_slot_num() {
    return &g_slot_num;
}

inline void set_slot_num(int slot_num) {
    g_slot_num = slot_num;
}

int nimbos_setup_syscall_buffers(int nimbos_fd, int slot_num)
{
    uint64_t syscall_queue_buf_paddr = NIMBOS_SYSCALL_QUEUE_BUF_SLOT_PADDR(slot_num);
    syscall_queue_buf_base = (void *)SHADOW_KERNEL_PADDR_TO_VADDR((void *)syscall_queue_buf_paddr);

    // printf("Shadow: map [%x, %x)\n", SHADOW_KERNEL_PADDR_TO_VADDR(NIMBOS_KERNEL_BASE_PADDR), SHADOW_KERNEL_PADDR_TO_VADDR(NIMBOS_KERNEL_BASE_PADDR) + NIMBOS_KERNEL_MAXSIZE);
    void *nimbos_kernel_base = mmap(
        (void *)SHADOW_KERNEL_PADDR_TO_VADDR(NIMBOS_KERNEL_BASE_PADDR), NIMBOS_KERNEL_MAXSIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE | MAP_FIXED, 
        nimbos_fd, 
        NIMBOS_KERNEL_BASE_PADDR - NIMBOS_BASE_PADDR
    );
    if (nimbos_kernel_base == MAP_FAILED) {
        return -ENOMEM;
    }

    struct syscall_queue_buffer_metadata *meta = syscall_queue_buf_base;
    struct scf_descriptor *desc;
    uint16_t *req_ring, *rsp_ring;
    uint16_t capacity = meta->capacity;

    // printf("magic:%x cap:%d lock:%d req:%d rsp:%d\n", meta->magic, meta->capacity, meta->lock,
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

    // printf("desc:%p req:%p rsp:%p\n", desc, req_ring, rsp_ring);

    g_syscall_queue_buffer = (struct syscall_queue_buffer){
        .capacity_mask = capacity - 1,
        .req_index_last = 0,
        .rsp_index_shadow = meta->rsp_index,
        .meta = meta,
        .desc = desc,
        .req_ring = req_ring,
        .rsp_ring = rsp_ring,
    };

    g_nimbos_fd = nimbos_fd;

    return 0;
}

inline struct syscall_queue_buffer *get_syscall_queue_buffer()
{
    return &g_syscall_queue_buffer;
}

inline int *get_nimbos_fd() {
    return &g_nimbos_fd;
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
    ioctl(*get_nimbos_fd(), NIMBOS_NOTIFY);
    return err;
}
