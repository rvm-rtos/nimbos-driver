#ifndef _NIMBOS_H
#define _NIMBOS_H

#define NIMBOS_FIRMWARE "nimbos.bin"

#define NIMBOS_SYSCALL_IPI_IRQ 13

struct mem_region {
    unsigned long long start;
    unsigned long long size;
};

extern int get_rt_memory_region(struct mem_region *region);

int nimbos_open(struct inode *inode, struct file *file);
int nimbos_close(struct inode *inode, struct file *file);

#endif /* !_NIMBOS_H */
