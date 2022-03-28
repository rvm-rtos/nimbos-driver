#ifndef _NIMBOS_H
#define _NIMBOS_H

#define NIMBOS_FIRMWARE "nimbos.bin"

#define NIMBOS_SYSCALL_SIG_NUM      44
#define NIMBOS_SYSCALL_IPI_IRQ      13

#define NIMBOS_REG_SYSCALL_HANDLER  _IO(0, 0)

struct mem_region {
    unsigned long long start;
    unsigned long long size;
};

extern int get_rt_memory_region(struct mem_region *region);

int nimbos_open(struct inode *inode, struct file *file);
int nimbos_close(struct inode *inode, struct file *file);
long nimbos_ioctl(struct file *file, unsigned int ioctl, unsigned long arg);

#endif /* !_NIMBOS_H */
