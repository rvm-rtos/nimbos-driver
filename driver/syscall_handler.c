#include <linux/list.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>

#include "nimbos.h"
#include "syscall_handler.h"

struct handler_node {
    struct list_head entry;
    syscall_handler_t handler;
};

static LIST_HEAD(handlers);

int add_syscall_handler(syscall_handler_t handler)
{
    struct handler_node *h;

    list_for_each_entry(h, &handlers, entry)
    {
        if (h->handler == handler) {
            return -EEXIST;
        }
    }

    h = kzalloc(sizeof(*h), GFP_KERNEL);
    INIT_LIST_HEAD(&h->entry);
    h->handler = handler;
    list_add_tail(&h->entry, &handlers);

    return 0;
}

int del_syscall_handler(struct task_struct *handler)
{
    struct handler_node *h;

    list_for_each_entry(h, &handlers, entry)
    {
        if (h->handler == handler) {
            list_del(&h->entry);
            kfree(h);
            return 0;
        }
    }

    return -EINVAL;
}

void signal_all_handlers(void)
{
    struct handler_node *h;
    struct kernel_siginfo info;
    struct task_struct *task;
    int err;

    memset(&info, 0, sizeof(info));
    info.si_signo = NIMBOS_SYSCALL_SIG_NUM;
    info.si_code = SI_QUEUE;
    info.si_int = 2333;

    list_for_each_entry(h, &handlers, entry)
    {
        task = h->handler;
        pr_debug("  send signal to %p(%d)\n", task, task->pid);
        err = send_sig_info(NIMBOS_SYSCALL_SIG_NUM, &info, task);
        if (err) {
            pr_err("nimbos-driver: send signal to user returns %d\n", err);
        }
    }
}
