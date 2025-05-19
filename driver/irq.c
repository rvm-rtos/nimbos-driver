#include <linux/interrupt.h>
#include <linux/miscdevice.h>

#include "nimbos.h"
#include "irq.h"
#include "process.h"


extern struct miscdevice nimbos_device;

int slot_to_irq(int slot_num) {
    return slot_num + IRQ_SLOT_OFFSET;
}

int irq_to_slot(int irq_num) {
    return irq_num - IRQ_SLOT_OFFSET;
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    pr_debug("nimbos-driver: IRQ %d received.\n", irq);
    signal_process(irq_to_slot(irq));
    return IRQ_HANDLED;
}

int register_irq(int irq_num) {
    int err;
    err = request_irq(irq_num, irq_handler, IRQF_SHARED, "nimbos-driver",
        &nimbos_device);
    if (err) {
        pr_err("nimbos-driver: request_irq %d returns %d\n", irq_num, err);
        free_irq(irq_num, &nimbos_device);
        return err;
    }
    pr_info("nimbos-driver: registered irq: %d\n", irq_num);
    return irq_num++;
}

int unregister_irq(int irq_num) {
    free_irq(irq_num, &nimbos_device);
    return 0;
}