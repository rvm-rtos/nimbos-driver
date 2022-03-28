#include <linux/file.h>
#include <linux/module.h>

int nimbos_open(struct inode *inode, struct file *file) {
    pr_info("nimbos_open: %p\n", get_current());
    return 0;
}

int nimbos_close(struct inode *inode, struct file *file) {
    pr_info("nimbos_close: %p\n", get_current());
    return 0;
}
