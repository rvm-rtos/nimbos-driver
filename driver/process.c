#include <linux/list.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>

#include "nimbos.h"
#include "irq.h"
#include "process.h"
#include "slot.h"

struct process_node {
    struct list_head entry;
    process_t process;
    int slot_num;
};

static LIST_HEAD(process_list);

int add_process(process_t process, int slot_num)
{
    struct process_node *h;

    list_for_each_entry(h, &process_list, entry)
    {
        if (h->process == process) {
            return -EEXIST;
        }
    }

    h = kzalloc(sizeof(*h), GFP_KERNEL);
    INIT_LIST_HEAD(&h->entry);
    h->process = process;
    h->slot_num = slot_num;
    register_irq(slot_to_irq(slot_num));
    list_add_tail(&h->entry, &process_list);
    pr_info("nimbos-driver: added process with slot: %d\n", slot_num);

    return 0;
}

int del_process(process_t process)
{
    struct process_node *h;

    list_for_each_entry(h, &process_list, entry)
    {
        if (h->process == process) {
            list_del(&h->entry);
            unregister_irq(slot_to_irq(h->slot_num));
            free_slot_num(h->slot_num);
            pr_info("Slot %d freed.\n", h->slot_num);
            kfree(h);
            return 0;
        }
    }

    return -EINVAL;
}

void del_all_processes(void) {
    struct process_node *h, *tmp;

    list_for_each_entry_safe(h, tmp, &process_list, entry) {
        list_del(&h->entry);
        unregister_irq(slot_to_irq(h->slot_num));
        free_slot_num(h->slot_num);
        kfree(h);
    }
}

void signal_all_processes(void)
{
    struct process_node *h;
    struct kernel_siginfo info;
    process_t process;
    int err;

    memset(&info, 0, sizeof(info));
    info.si_signo = NIMBOS_SYSCALL_SIG_NUM;
    info.si_code = SI_QUEUE;
    info.si_int = 2333;

    list_for_each_entry(h, &process_list, entry)
    {
        process = h->process;
        pr_debug("[Driver] Send signal to %p(%d)\n", process, process->pid);
        err = send_sig_info(NIMBOS_SYSCALL_SIG_NUM, &info, process);
        if (err) {
            pr_err("nimbos-driver: send signal to user returns %d\n", err);
        }
    }
}

void signal_process(int slot_num) {
    struct process_node *h;
    struct kernel_siginfo info;
    process_t process;
    int err;

    memset(&info, 0, sizeof(info));
    info.si_signo = NIMBOS_SYSCALL_SIG_NUM;
    info.si_code = SI_QUEUE;
    info.si_int = 2333;
    
    list_for_each_entry(h, &process_list, entry)
    {
        if (h->slot_num == slot_num) { 
            process = h->process;
            pr_debug("send signal to %p(%d)\n", process, process->pid);
            err = send_sig_info(NIMBOS_SYSCALL_SIG_NUM, &info, process);
            if (err) {
                pr_err("nimbos-driver: send signal to user returns %d\n", err);
            }
        }
    }
}
