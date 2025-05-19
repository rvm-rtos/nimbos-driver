#ifndef _SLOT_H
#define _SLOT_H

#define MAX_SLOTS 4

int allocate_slot_num(void);
void free_slot_num(int slot_num);

void free_all_slots(void);

#endif // _SLOT_H