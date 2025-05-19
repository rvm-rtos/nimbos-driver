obj-y := syscall.o scf.o

always-y := ulib.a

OBJECTS := $(addprefix $(obj)/,$(obj-y))

$(obj)/ulib.a: $(OBJECTS) FORCE
	$(call if_changed,ar)
