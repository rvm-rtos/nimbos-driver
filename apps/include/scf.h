#ifndef _SCF_H
#define _SCF_H

#include <assert.h>
#include <stddef.h>
#include <sys/mman.h>

#include "spin_lock.h"

#define NIMBOS_SYSCALL_DATA_BUF_SIZE  (1 << 20) // 1M
#define NIMBOS_SYSCALL_QUEUE_BUF_SIZE 4096      // 4K

#define SYSCALL_QUEUE_BUFFER_MAGIC 0x4643537f // "\x7fSCF"

enum scf_opcode {
    IPC_OP_NOP = 0,
    IPC_OP_READ = 1,
    IPC_OP_WRITE = 2,
    IPC_OP_OPEN = 3,
    IPC_OP_CLOSE = 4,
    IPC_OP_UNKNOWN = 0xff,
};

struct syscall_queue_buffer_metadata {
    uint32_t magic;
    spin_lock_t lock;
    uint16_t capacity;
    uint16_t req_index;
    uint16_t rsp_index;
};

struct scf_descriptor {
    uint8_t valid;
    uint8_t opcode;
    uint64_t args;
    uint64_t ret_val;
};

struct syscall_queue_buffer {
    uint16_t capacity_mask;
    uint16_t req_index_last;
    uint16_t rsp_index_shadow;
    struct syscall_queue_buffer_metadata *meta;
    struct scf_descriptor *desc;
    uint16_t *req_ring;
    uint16_t *rsp_ring;
};

_Static_assert(sizeof(struct syscall_queue_buffer_metadata) == 0xc);
_Static_assert(sizeof(struct scf_descriptor) == 0x18);

int nimbos_setup_syscall_buffers(int nimbos_fd);

void *offset_to_ptr(uint64_t offset);

struct syscall_queue_buffer *get_syscall_queue_buffer();

struct scf_descriptor *get_syscall_request_from_index(struct syscall_queue_buffer *buf,
                                                      uint16_t index);
int pop_syscall_request(struct syscall_queue_buffer *buf, uint16_t *out_index,
                        struct scf_descriptor *out_desc);
int push_syscall_response(struct syscall_queue_buffer *buf, uint16_t index,
                          uint64_t ret_val);

#endif /* !_SCF_H */
