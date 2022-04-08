#ifndef _NIMBOS_H
#define _NIMBOS_H

#define NIMBOS_FIRMWARE "nimbos.bin"

#define NIMBOS_SYSCALL_SIG_NUM 44
#define NIMBOS_SYSCALL_IPI_IRQ 13

#define NIMBOS_SYSCALL_DATA_BUF_SIZE (1 << 20)  // 1M
#define NIMBOS_SYSCALL_QUEUE_BUF_SIZE 4096      // 4K

#define NIMBOS_SYSCALL_SETUP _IO(0, 0)

struct mem_region {
    unsigned long long start;
    unsigned long long size;
};

extern struct mem_region rt_region;

extern int get_rt_memory_region(struct mem_region *region);

#endif /* !_NIMBOS_H */
