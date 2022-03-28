#include <linux/file.h>
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
    case NIMBOS_REG_SYSCALL_HANDLER:
        err = nimbos_register_syscall_handler();
        break;
    default:
        err = -EINVAL;
        break;
    }
    return err;
}
