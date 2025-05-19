#include <linux/cpu.h>
#include <linux/version.h>

unsigned long get_kallsyms_func(void);
unsigned long generic_kallsyms_lookup_name(const char *name);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
#define cpu_down(cpu) remove_cpu(cpu)
#define cpu_up(cpu) add_cpu(cpu)
#endif

#include <linux/kallsyms.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0) ||                           \
	LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)

#include <linux/kprobes.h>

static unsigned long (*kallsyms_lookup_name_sym)(const char *name);

static int _kallsyms_lookup_kprobe(struct kprobe *p, struct pt_regs *regs)
{
	return 0;
}

unsigned long get_kallsyms_func(void)
{
	struct kprobe probe;
	int ret;
	unsigned long addr;

	memset(&probe, 0, sizeof(probe));
	probe.pre_handler = _kallsyms_lookup_kprobe;
	probe.symbol_name = "kallsyms_lookup_name";
	ret = register_kprobe(&probe);
	if (ret)
		return 0;
	addr = (unsigned long)probe.addr;
	unregister_kprobe(&probe);
	return addr;
}

unsigned long generic_kallsyms_lookup_name(const char *name)
{
	/* singleton */
	if (!kallsyms_lookup_name_sym)
	{
		kallsyms_lookup_name_sym = (void *)get_kallsyms_func();
		if (!kallsyms_lookup_name_sym)
			return 0;
	}
	return kallsyms_lookup_name_sym(name);
}

#else

unsigned long generic_kallsyms_lookup_name(const char *name)
{
	return kallsyms_lookup_name(name);
}

#endif