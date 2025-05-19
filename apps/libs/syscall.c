#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nimbos.h"
#include "scf.h"

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define BUF_SIZE 1024

static int thread_count = 1;

void print_maps() {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        perror("Failed to open /proc/self/maps");
        return;
    }

    char line[BUF_SIZE];
    printf("%-23s %-5s %8s %8s %5s %8s %s\n",
           "Address", "Perms", "Offset", "Dev", "Inode", "Size", "Path");

    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start, end;
        sscanf(line, "%"SCNxPTR"-%"SCNxPTR" ", &start, &end);
        printf("%lx-%lx\n", start, end);
    }

    fclose(fp);
}

static void nimbos_syscall_handler(int signum);

static void *read_thread_fn(void *arg)
{
    uint64_t* args;
    struct syscall_queue_buffer *scf_buf = get_syscall_queue_buffer();
    uint16_t desc_index = (uint16_t)(long)arg;
    struct scf_descriptor *desc = get_syscall_request_from_index(scf_buf, desc_index);

    if (!desc) {
        return NULL;
    }


    args = desc->args;
    int fd = (int)args[0];
    char *buf = (char *)args[1];
    if ((uint64_t) buf > NIMBOS_KERNEL_BASE_VADDR) {
        buf = NIMBOS_TO_SHADOW_KERNEL_VADDR(buf);
    }
    size_t len = (size_t)args[2];
    // printf("Shadow: read fd=%d, buf=%lx, len=%lu\n", fd, (uint64_t)buf, len);
    int ret = read(fd, buf, len);
    // assert(ret == args->len);
    push_syscall_response(scf_buf, desc_index, ret);
    return NULL;
}

void poll_requests(void)
{
    uint16_t desc_index;
    struct scf_descriptor desc;
    struct syscall_queue_buffer *scf_buf = get_syscall_queue_buffer();
    int nimbos_fd = *get_nimbos_fd();
    pthread_t thread; // FIXME: use global threads pool

    while (!pop_syscall_request(scf_buf, &desc_index, &desc)) {
        // printf("syscall: desc_index=%d, opcode=%d, args=0x%lx\n", desc_index,
        // desc.opcode, desc.args);
        switch (desc.opcode) {
        case IPC_OP_READ: {
            pthread_create(&thread, NULL, read_thread_fn, (void *)(long)desc_index);
            break;
        }
        case IPC_OP_WRITE: {
            uint64_t *args = desc.args;
            int fd = (int)args[0];
            char *buf = (char *)args[1];
            size_t len = (size_t)args[2];
            // printf("Shadow: write fd=%d, buf=%lx, len=%lu\n", fd, (uint64_t)buf, len);
            int ret = write(fd, buf, len);
            // printf("Shadow: write ret=%d\n", ret);
            assert(ret == (int)len);
            push_syscall_response(scf_buf, desc_index, ret);
            break;
        }
        case IPC_OP_OPEN: {
            uint64_t *args = desc.args;
            char *pathname = (char *)args[0];
            int flags = (int)args[1];
            int mode = (int)args[2];
            // printf("Shadow: open pathname=%s, flags=%x, mode=%o\n", pathname, flags, mode);
            int ret = open(pathname, flags, mode);
            push_syscall_response(scf_buf, desc_index, ret);
            break;
        }
        case IPC_OP_CLOSE: {
            uint64_t *args = desc.args;
            int fd = (int)args[0];
            // printf("Shadow: close fd=%d\n", fd);
            int ret = close(fd);
            push_syscall_response(scf_buf, desc_index, ret);
            break;
        }
        case IPC_OP_SYNCMAP: {
            // prepare args
            uint64_t *args = desc.args;
            void *vaddr = (void *)args[0];
            uint64_t len = args[1];
            uint64_t paddr = args[2];
            int prot = (int)args[3];

            // printf("Shadow: mmap vaddr=%p, len=%lu, paddr=%lx, flags=%x, fd=%d\n", vaddr, len, paddr, prot, nimbos_fd);
            void* mapped_ptr = mmap(vaddr, len, prot, MAP_SHARED | MAP_FIXED, nimbos_fd, paddr - NIMBOS_BASE_PADDR);
            int ret = 0;
            if (mapped_ptr == MAP_FAILED) {
                ret = -1;
            }
            push_syscall_response(scf_buf, desc_index, ret);
            // printf("Shadow: mmap ret=%d, mapped_ptr=%p\n", ret, mapped_ptr);
            break;
        }
        case IPC_OP_SYNCUNMAP: {
            uint64_t *args = desc.args;
            void *vaddr = (void *)args[0];
            // printf("Shadow: unmap vaddr=%p, len=%lu\n", vaddr, args->len);
            int ret = munmap(vaddr, args[1]);
            // int ret = 0;
            push_syscall_response(scf_buf, desc_index, ret);
            // printf("Shadow: unmap ret=%d\n", ret);
            break;
        }
        case IPC_OP_FORK: {
            int pip[2];
            int err = pipe(pip);
            if (err) {
                push_syscall_response(scf_buf, desc_index, err);
                break;
            }
            int pid = fork();
            if (pid) {
                // parent
                close(pip[1]);

                // wait for child process
                int ret;
                int n = read(pip[0], &ret, sizeof(int));
                if (n != sizeof(int)) {
                    ret = -1;
                }

                // printf("child pid=%d\n", pid);
                
                // push ret
                push_syscall_response(scf_buf, desc_index, ret);
                close(pip[0]);
            } else {
                // child
                close(pip[0]);
                int slot_num, response;

                int err = ioctl(nimbos_fd, NIMBOS_SETUP_SYSCALL, &slot_num);
                if (err) {
                    response = err;
                } else {
                    set_slot_num(slot_num);
                    nimbos_setup_syscall_buffers(nimbos_fd, slot_num);
                    response = slot_num;
                }

                int ret = write(pip[1], &response, sizeof(int));
                assert(ret == sizeof(int));
                close(pip[1]);
            }
            break;
        }
        case IPC_OP_STAT: {
            uint64_t *args = desc.args;
            const char* path = (char *)args[0];
            struct stat st;
            int ret = stat(path, &st);
            if (ret == 0) {
                push_syscall_response(scf_buf, desc_index, st.st_size);
            } else {
                push_syscall_response(scf_buf, desc_index, ret);
            }
            break;
        }
        case IPC_OP_CLONE: {
            thread_count++;
            push_syscall_response(scf_buf, desc_index, 0);
            break;
        }
        case IPC_OP_EXIT: {
            // printf("Shadow: exit, thread count: %d\n", thread_count);
            if (thread_count == 1) {
                ioctl(nimbos_fd, NIMBOS_EXIT, NULL);
                push_syscall_response(scf_buf, desc_index, 0);
                exit(0);
            } else {
                thread_count--;
                push_syscall_response(scf_buf, desc_index, 0);
            }
            break;
        }
        default:
            break;
        }
    }
}

static void nimbos_syscall_handler(int signum)
{
    if (signum == NIMBOS_SYSCALL_SIG_NUM) {
        poll_requests();
    }
}

int nimbos_setup_syscall()
{
    int fd = open(NIMBOS_DEV, O_RDWR);
    if (fd <= 0) {
        return fd;
    }
    int err = nimbos_setup_syscall_buffers(fd, 0);
    if (err) {
        fprintf(stderr, "Failed to setup syscall buffers: %d\n", err);
        return err;
    }

    int slot_num;
    err = ioctl(fd, NIMBOS_SETUP_SYSCALL, &slot_num);
    if (err) {
        fprintf(stderr, "Failed to setup syscall: %d\n", err);
        return err;
    }
    set_slot_num(slot_num);
    // printf("syscall: slot_num=%d\n", slot_num);
    signal(NIMBOS_SYSCALL_SIG_NUM, nimbos_syscall_handler);

    // handle requests before app starting
    poll_requests();

    return fd;
}
