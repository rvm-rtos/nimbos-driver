#ifndef _NIMBOS_H
#define _NIMBOS_H

#include <signal.h>

#define NIMBOS_DEV  "/dev/nimbos"

#define NIMBOS_SYSCALL_SIG_NUM      44

#define NIMBOS_REG_SYSCALL_HANDLER  _IO(0, 0)

typedef void (*sighandler_t)(int);

int register_nimbos_syscall_handler(int nimbos_fd, sighandler_t handler)
{
    ioctl(nimbos_fd, NIMBOS_REG_SYSCALL_HANDLER);
    signal(NIMBOS_SYSCALL_SIG_NUM, handler);
    return 0;
}

#endif /* !_NIMBOS_H */
