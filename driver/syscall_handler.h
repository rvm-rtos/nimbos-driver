#ifndef _NIMBOS_SYSCALL_HANDLER_H
#define _NIMBOS_SYSCALL_HANDLER_H

#define syscall_handler_t struct task_struct *

int add_syscall_handler(syscall_handler_t task);
int del_syscall_handler(syscall_handler_t task);
void signal_all_handlers(void);

#endif /* !_NIMBOS_SYSCALL_HANDLER_H */
