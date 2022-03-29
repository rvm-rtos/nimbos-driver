#ifndef _SPIN_LOCK_H
#define _SPIN_LOCK_H

#include <sched.h>
#include <stdint.h>

typedef uint8_t spin_lock_t;

inline void spin_lock(spin_lock_t *lock)
{
    spin_lock_t expected = 0;
    while (!__atomic_compare_exchange_n(lock, &expected, 1, 0, __ATOMIC_ACQUIRE,
                                        __ATOMIC_RELAXED)) {
        expected = 0;
        do {
            sched_yield();
        } while (__atomic_load_n(lock, __ATOMIC_RELAXED) != 0);
    }
}

inline void spin_unlock(spin_lock_t *lock)
{
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

#endif /* !_SPIN_LOCK_H */
