#ifndef _NIMBOS_DRIVER_H
#define _NIMBOS_DRIVER_H

#define NIMBOS_FIRMWARE "nimbos.bin"

struct mem_region {
    unsigned long long start;
    unsigned long long size;
};

extern int get_rt_memory_region(struct mem_region *region);

#endif /* !_NIMBOS_DRIVER_H */
