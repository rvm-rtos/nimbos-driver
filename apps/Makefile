LD = $(CC) $(KBUILD_CFLAGS)

NOSTDINC_FLAGS :=
LINUXINCLUDE := -I$(src)/include
KBUILD_LDFLAGS :=
KBUILD_CFLAGS := -g -O3 -Wall -Werror -D__LINUX_COMPILER_TYPES_H

always := hello
subdir-y := libs/

ULIBS := $(obj)/libs/ulib.a

$(obj)/%: $(obj)/%.o $(ULIBS) FORCE
	$(call if_changed,ld)
