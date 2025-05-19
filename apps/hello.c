#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include <nimbos.h>
#include <remap.h>
#include <scf.h>

int main()
{
    printf("Hello NimbOS!\n");

    // Check For reserved space
    if (mmap((void*)0x0, NIMBOS_SIZE, PROT_NONE, MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
        printf("User space address [0x0, %lx] not reserved.\n", (size_t)NIMBOS_SIZE);
        return -1;
    }

    munmap((void*)0x0, NIMBOS_SIZE);

    // Nimbos kernel virtual address
    void* nimbos_kernel_virt_addr = (void *)SHADOW_KERNEL_PADDR_TO_VADDR(NIMBOS_KERNEL_BASE_PADDR);
    if (mmap((void*)nimbos_kernel_virt_addr, NIMBOS_KERNEL_MAXSIZE, PROT_NONE, MAP_SHARED | MAP_FIXED| MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
        printf("Kernel space address [%lx, %lx] not reserved.\n", (size_t)nimbos_kernel_virt_addr, (size_t)nimbos_kernel_virt_addr + NIMBOS_KERNEL_MAXSIZE);
        return -1;
    }

    munmap((void*)nimbos_kernel_virt_addr, NIMBOS_KERNEL_MAXSIZE);

    // Nimbos user stack 
    if (mmap((void*)NIMBOS_USER_STACK_BASE_VADDR, NIMBOS_USER_STACK_SIZE, PROT_NONE, MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
        printf("User stack address [%lx, %lx] not reserved.\n", (size_t)NIMBOS_USER_STACK_BASE_VADDR, (size_t)NIMBOS_USER_STACK_BASE_VADDR + NIMBOS_USER_STACK_SIZE);
        return -1;
    }

    munmap((void*)NIMBOS_USER_STACK_BASE_VADDR, NIMBOS_USER_STACK_SIZE);

    int fd = nimbos_setup_syscall();
    if (fd <= 0) {
        printf("Failed to open NimbOS device `%s`\n", NIMBOS_DEV);
        return fd;
    }

    // printf("NimbOS device opened wsith fd %d\n", fd);

    for (;;) {
        // printf("Sleep %d...\n", i);
        usleep(1000);
    }

    close(fd);
    return 0;
}
