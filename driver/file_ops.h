#ifndef _NIMBOS_FILE_OPS_H
#define _NIMBOS_FILE_OPS_H

#include <asm/apic.h>

extern void (*apic_send_IPI_allbutself_sym)(int vec);

int nimbos_open(struct inode *inode, struct file *file);
int nimbos_close(struct inode *inode, struct file *file);
long nimbos_ioctl(struct file *file, unsigned int ioctl, unsigned long arg);
int nimbos_mmap(struct file *file, struct vm_area_struct *vma);

#endif /* !_NIMBOS_FILE_OPS_H */
