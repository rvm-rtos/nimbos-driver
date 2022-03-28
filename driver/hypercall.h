#ifndef _NIMBOS_HYPERCALL_H
#define _NIMBOS_HYPERCALL_H

#define RVM_HC_RT_START    1
#define RVM_HC_RT_SHUTDOWN 2

/*
 * As this is never called on a CPU without VM extensions,
 * we assume that where VMCALL isn't available, VMMCALL is.
 */
#define RVM_HYPERCALL_CODE                                                                         \
    "cmpb $0x01, %[use_vmcall]\n\t"                                                                \
    "jne 1f\n\t"                                                                                   \
    "vmcall\n\t"                                                                                   \
    "jmp 2f\n\t"                                                                                   \
    "1: vmmcall\n\t"                                                                               \
    "2:"

#define RVM_HYPERCALL_RESULT     "=a"(result)
#define RVM_HYPERCALL_USE_VMCALL [use_vmcall] "m"(hypercall_use_vmcall)
#define RVM_HYPERCALL_NUM        "a"(num)
#define RVM_HYPERCALL_ARG1       "D"(arg1)
#define RVM_HYPERCALL_ARG2       "S"(arg2)

extern bool hypercall_use_vmcall;

/**
 * Invoke a hypervisor without additional arguments.
 * @param num		Hypercall number.
 *
 * @return Result of the hypercall, semantic depends on the invoked service.
 */
static inline __u32 rvm_hypercall(__u32 num)
{
    __u32 result;

    asm volatile(RVM_HYPERCALL_CODE:RVM_HYPERCALL_RESULT
                 : RVM_HYPERCALL_USE_VMCALL, RVM_HYPERCALL_NUM
                 : "memory");
    return result;
}

/**
 * Invoke a hypervisor with one argument.
 * @param num		Hypercall number.
 * @param arg1		First argument.
 *
 * @return Result of the hypercall, semantic depends on the invoked service.
 */
static inline __u32 rvm_hypercall_arg1(__u32 num, unsigned long arg1)
{
    __u32 result;

    asm volatile(RVM_HYPERCALL_CODE:RVM_HYPERCALL_RESULT
                 : RVM_HYPERCALL_USE_VMCALL, RVM_HYPERCALL_NUM, RVM_HYPERCALL_ARG1
                 : "memory");
    return result;
}

/**
 * Invoke a hypervisor with two arguments.
 * @param num		Hypercall number.
 * @param arg1		First argument.
 * @param arg2		Second argument.
 *
 * @return Result of the hypercall, semantic depends on the invoked service.
 */
static inline __u32 rvm_hypercall_arg2(__u32 num, unsigned long arg1, unsigned long arg2)
{
    __u32 result;

    asm volatile(RVM_HYPERCALL_CODE:RVM_HYPERCALL_RESULT
                 : RVM_HYPERCALL_USE_VMCALL, RVM_HYPERCALL_NUM, RVM_HYPERCALL_ARG1,
                   RVM_HYPERCALL_ARG2
                 : "memory");
    return result;
}

#endif /* !_NIMBOS_HYPERCALL_H */
