ifneq ($(KERNELRELEASE),)
   obj-m := char_dev.o
else
#KERNELDIR ?= /lib/modules/$(shell uname -r)/build
KERNELDIR ?= /lib/modules/4.10.0-40-generic/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
