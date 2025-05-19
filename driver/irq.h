#ifndef IRQ_H
#define IRQ_H

#include "nimbos.h"

#define IRQ_SLOT_OFFSET NIMBOS_SYSCALL_IPI_IRQ

int slot_to_irq(int slot_num);
int irq_to_slot(int irq_num);

int register_irq(int irq_num); 
int unregister_irq(int irq_num);
int unregister_all_irqs(void);

#endif