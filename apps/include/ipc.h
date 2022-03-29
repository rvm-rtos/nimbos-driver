#ifndef _IPC_H
#define _IPC_H

#include <stddef.h>
#include <assert.h>
#include <sys/mman.h>

#include "spin_lock.h"

#define NIMBOS_SYSCALL_SEND_BUF_SIZE    4096        // 4K
#define NIMBOS_SYSCALL_RECV_BUF_SIZE    4096        // 4K
#define NIMBOS_SYSCALL_DATA_BUF_SIZE    (4096 * 16) // 64K

enum ipc_opcode {
    IPC_OP_NOP = 0,
    IPC_OP_READ = 1,
    IPC_OP_WRITE = 2,
    IPC_OP_OPEN = 3,
    IPC_OP_CLOSE = 4,
    IPC_OP_UNKNOWN = 0xff,
};

struct ipc_buffer_entry {
    uint8_t opcode;
    uint64_t args;
};

struct ipc_buffer {
    spin_lock_t lock;
    uint32_t head;
    uint32_t tail;
    uint32_t capacity;
    uint32_t capacity_mask;
    struct ipc_buffer_entry entries[];
};

_Static_assert(offsetof(struct ipc_buffer, lock) == 0);
_Static_assert(offsetof(struct ipc_buffer, head) == 4);
_Static_assert(offsetof(struct ipc_buffer, tail) == 8);
_Static_assert(offsetof(struct ipc_buffer, capacity) == 12);
_Static_assert(offsetof(struct ipc_buffer, capacity_mask) == 16);
_Static_assert(offsetof(struct ipc_buffer, entries) == 24);

void nimbos_setup_ipc_buffer(int nimbos_fd);

struct ipc_buffer *get_send_buffer();

int pop_syscall_request(struct ipc_buffer *buf, struct ipc_buffer_entry *out_entry);

#endif /* !_IPC_H */
