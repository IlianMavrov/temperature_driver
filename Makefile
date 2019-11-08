ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabi-

ifneq ($(KERNELRELEASE),)
obj-m := tmp_100.o
else
KDIR ?= $(HOME)/src/driver_Linux_training/linux-kernel-labs/src/linux
endif

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
