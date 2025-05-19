#include "slot.h"
#include <linux/module.h>


static int current_slot = 0;

int free_slots[MAX_SLOTS];
int free_slot_down = 0;
int free_slot_up = 0;

inline int add_one_mod(int a) {
    return (a + 1) % MAX_SLOTS;
}

void slot_init(void) {
    int i = 0;
    for (; i < MAX_SLOTS; i++) {
        free_slots[i] = -1;
    }
}

int allocate_slot_num(void) {
    int index = free_slot_down;
    if (current_slot < MAX_SLOTS) {
        return current_slot++;
    } else {
        if (free_slot_up != free_slot_down) {
            free_slot_down = add_one_mod(free_slot_down);
            return free_slots[index];
        } else {
            return -1; // no free slots
        }
    }
}

void free_slot_num(int slot_num) {
    int index = free_slot_down;
    if (current_slot <= slot_num) {
        return;
    }

    while(index != free_slot_up) {
        if (free_slots[index] == slot_num) {
            return;
        }
        index = add_one_mod(index);
    }
    
    free_slot_up = add_one_mod(free_slot_up);
    free_slots[index] = slot_num;
}

void free_all_slots(void) {
    current_slot = 0;
    free_slot_down = 0;
    free_slot_up = 0;
    slot_init();
}