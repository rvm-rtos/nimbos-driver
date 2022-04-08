KDIR ?= /lib/modules/$(shell uname -r)/build
RVM_DIR ?= $(PWD)/../jailhouse
DEBUG :=

KBUILD_EXTRA_SYMBOLS := $(RVM_DIR)/Module.symvers

ifneq ($(DEBUG),)
  KBUILD_CFLAGS += -DDEBUG
endif

module := nimbos_driver
subdir-y := apps/
obj-m := driver/

build:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

insmod: rmmod
	sudo insmod driver/$(module).ko
	sudo chmod 666 /dev/nimbos

rmmod:
ifneq ($(shell lsmod | grep nimbos),)
	sudo rmmod $(module)
endif

run: insmod
	./apps/hello
