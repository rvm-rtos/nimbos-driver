#include <linux/file.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>

#include "nimbos.h"
#include "syscall_handler.h"

static int nimbos_register_syscall_handler(void)
{
    return add_syscall_handler(get_current());
}

static int nimbos_deregister_syscall_handler(syscall_handler_t handler)
{
    return del_syscall_handler(handler);
}

static int nimbos_syscall_setup(void)
{
    return nimbos_register_syscall_handler();
}

int nimbos_open(struct inode *inode, struct file *file)
{
    pr_info("nimbos_open: %p(%d)\n", get_current(), get_current()->pid);
    return 0;
}

int nimbos_close(struct inode *inode, struct file *file)
{
    pr_info("nimbos_close: %p(%d)\n", get_current(), get_current()->pid);
    nimbos_deregister_syscall_handler(get_current());
    return 0;
}

long nimbos_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
    long err;

    switch (ioctl) {
    case NIMBOS_SYSCALL_SETUP:
        err = nimbos_syscall_setup();
        break;
    default:
        err = -EINVAL;
        break;
    }
    return err;
}

int nimbos_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long paddr;
    unsigned long size;
    unsigned long map_size;

    map_size = vma->vm_end - vma->vm_start;
    paddr = rt_region.start + rt_region.size;
    switch (vma->vm_pgoff) {
    case 0: // data buffer
        size = NIMBOS_SYSCALL_DATA_BUF_SIZE;
        paddr -= NIMBOS_SYSCALL_QUEUE_BUF_SIZE + size;
        break;
    case 1: // queue buffer
        size = NIMBOS_SYSCALL_QUEUE_BUF_SIZE;
        paddr -= size;
        break;
    default:
        return -EINVAL;
    }

    pr_debug("nimbos_mmap: [0x%lx, 0x%lx) pgoff=%lx => paddr=0x%lx, size=0x%lx",
             vma->vm_start, vma->vm_end, vma->vm_pgoff, paddr, size);

    if (map_size > size) {
        return -ENOMEM;
    }
    return remap_pfn_range(vma, vma->vm_start, paddr >> PAGE_SHIFT, map_size,
                           vma->vm_page_prot);
}
