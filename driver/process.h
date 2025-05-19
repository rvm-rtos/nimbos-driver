#ifndef _PROCESS_H
#define _PROCESS_H

typedef struct task_struct * process_t;

int add_process(process_t task, int irq_num);
int del_process(process_t task);
void del_all_processes(void);
void signal_all_processes(void);
void signal_process(int irq_num);

#endif /* !_PROCESS_H */
