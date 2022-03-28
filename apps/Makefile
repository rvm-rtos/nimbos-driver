LD = $(CC) $(KBUILD_CFLAGS)

NOSTDINC_FLAGS :=
LINUXINCLUDE :=
KBUILD_LDFLAGS :=
KBUILD_CFLAGS := -g -O3 -Wall -Werror -D__LINUX_COMPILER_TYPES_H

always := hello

$(obj)/%: $(obj)/%.o FORCE
	$(call if_changed,ld)
