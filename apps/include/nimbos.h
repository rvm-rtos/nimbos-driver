#ifndef _NIMBOS_H
#define _NIMBOS_H

#define NIMBOS_DEV "/dev/nimbos"

#define NIMBOS_SYSCALL_SIG_NUM 44

#define NIMBOS_SETUP_SYSCALL _IO(0, 0)

int nimbos_setup_syscall();

#endif /* !_NIMBOS_H */
