# SPDX-License-Identifier: GPL-2.0

ifeq ($INDEPEND_BUILD,1)
obj-$(CONFIG_OF_TEST) := of_test.o
else # make INDEPEND_BUILD=1
obj-m := of_test.o
KERNELDIR ?= /lib/modules/`uname -r`/build
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
.PHONY:clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif