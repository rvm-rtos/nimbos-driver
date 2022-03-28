KDIR ?= /lib/modules/$(shell uname -r)/build
RVM_DIR ?= $(PWD)/../jailhouse

KBUILD_EXTRA_SYMBOLS := $(RVM_DIR)/Module.symvers

module := nimbos_driver
obj-m := $(module).o

build:
	@echo $(KBUILD_EXTRA_SYMBOLS)
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

insmod: rmmod
	sudo insmod $(module).ko

rmmod:
ifneq ($(shell lsmod | grep nimbos),)
	sudo rmmod $(module)
endif
