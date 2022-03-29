#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>

#include "file_ops.h"
#include "hypercall.h"
#include "nimbos.h"
#include "syscall_handler.h"

struct mem_region rt_region;

static struct resource *rt_mem_res;

static const struct file_operations nimbos_fops = {
    .owner = THIS_MODULE,
    .open = nimbos_open,
    .release = nimbos_close,
    .unlocked_ioctl = nimbos_ioctl,
    .compat_ioctl = nimbos_ioctl,
    .mmap = nimbos_mmap,
};

static struct miscdevice nimbos_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "nimbos",
    .fops = &nimbos_fops,
};

bool hypercall_use_vmcall;

static void init_hypercall(void)
{
    hypercall_use_vmcall = boot_cpu_has(X86_FEATURE_VMX);
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    pr_debug("IRQ %d %p(%d)\n", irq, get_current(), get_current()->pid);
    signal_all_handlers();
    return IRQ_HANDLED;
}

static int start_rtos(void)
{
    int err = 0;
    const struct firmware *nimbos_image;
    void *nimbos_mem;

    pr_info("nimbos-driver: RT memory region: [0x%llx-0x%llx], 0x%llx\n", rt_region.start,
            rt_region.start + rt_region.size - 1, rt_region.size);

    err = request_firmware(&nimbos_image, NIMBOS_FIRMWARE, nimbos_device.this_device);
    if (err) {
        pr_err("nimbos-driver: Missing RTOS image %s\n", NIMBOS_FIRMWARE);
        pr_info("%d", err);
        return err;
    }
    if (nimbos_image->size > rt_region.size) {
        err = -EINVAL;
        pr_err("nimbos-driver: firmware size 0x%lx is too large\n", nimbos_image->size);
        goto err_release_fw;
    }

    rt_mem_res = request_mem_region(rt_region.start, rt_region.size, "RTOS memory");
    if (!rt_mem_res) {
        err = -ENOMEM;
        pr_err("nimbos-driver: request_mem_region failed for RT image memory.\n");
        pr_notice("nimbos-driver: Did you reserve the memory with \"memmap=\" "
                  "or \"mem=\"?\n");
        goto err_release_fw;
    }
    nimbos_mem = ioremap(rt_region.start, rt_region.size);
    if (!nimbos_mem) {
        err = -EBUSY;
        pr_err("nimbos-driver: Unable to map RAM reserved for RT image at %08lx\n",
               (unsigned long)rt_region.start);
        goto err_release_mem_region;
    }

    memcpy(nimbos_mem, nimbos_image->data, nimbos_image->size);
    memset(nimbos_mem + nimbos_image->size, 0, rt_region.size - nimbos_image->size);

    flush_icache_range((unsigned long)nimbos_mem,
                       (unsigned long)(nimbos_mem + nimbos_image->size));
    vunmap(nimbos_mem);

    pr_info("Starting RTOS: entry=0x%llx, image_size=0x%lx\n", rt_region.start,
            nimbos_image->size);
    err = rvm_hypercall_arg1(RVM_HC_RT_START, rt_region.start);

    release_firmware(nimbos_image);
    return 0;

err_release_mem_region:
    if (rt_mem_res) {
        release_mem_region(rt_mem_res->start, resource_size(rt_mem_res));
        rt_mem_res = NULL;
    }

err_release_fw:
    release_firmware(nimbos_image);

    return err;
}

static int shutdown_rtos(void)
{
    pr_info("Shutting down RTOS...\n");
    return rvm_hypercall(RVM_HC_RT_SHUTDOWN);
}

static int __init nimbos_init(void)
{
    int err;

    pr_info("nimbos-driver: init...\n");

    init_hypercall();

    err = misc_register(&nimbos_device);
    if (err) {
        pr_err("nimbos-driver: cannot register misc device\n");
        return err;
    }

    pr_info("nimbos-driver: starting RTOS...\n");

    err = get_rt_memory_region(&rt_region);
    if (err) {
        pr_err("nimbos-driver: get RT memory region failed\n");
        goto err_unregister;
    }

    err = request_irq(NIMBOS_SYSCALL_IPI_IRQ, irq_handler, IRQF_SHARED, "nimbos-driver",
                      &nimbos_device);
    if (err) {
        pr_err("nimbos-driver: request_irq %d returns %d\n", NIMBOS_SYSCALL_IPI_IRQ, err);
        goto err_unregister;
    }

    err = start_rtos();
    if (err) {
        pr_err("nimbos-driver: start RTOS failed\n");
        goto err_free_irq;
    }

    pr_info("RTOS is started.\n");
    return 0;

err_free_irq:
    free_irq(NIMBOS_SYSCALL_IPI_IRQ, &nimbos_device);

err_unregister:
    misc_deregister(&nimbos_device);
    return err;
}

static void __exit nimbos_exit(void)
{
    pr_info("nimbos-driver: exit...\n");

    shutdown_rtos();

    free_irq(NIMBOS_SYSCALL_IPI_IRQ, &nimbos_device);

    if (rt_mem_res) {
        release_mem_region(rt_mem_res->start, resource_size(rt_mem_res));
    }

    misc_deregister(&nimbos_device);
}

module_init(nimbos_init);
module_exit(nimbos_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuekai Jia");
MODULE_DESCRIPTION("NimbOS driver");
MODULE_VERSION("0.1.0");
MODULE_FIRMWARE(NIMBOS_FIRMWARE);
