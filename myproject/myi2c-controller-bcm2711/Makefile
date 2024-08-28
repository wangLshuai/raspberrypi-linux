# SPDX-License-Identifier: GPL-2.0

ifeq ($INDEPEND_BUILD,1)
obj-$(CONFIG_MYI2C_CONTROLLER_BCM2711) := myi2c-controller-bcm2711.o
else # make INDEPEND_BUILD=1
obj-m := myi2c-controller-bcm2711.o
KERNELDIR ?= /lib/modules/`uname -r`/build
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
.PHONY:clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif