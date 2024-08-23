# SPDX-License-Identifier: GPL-2.0

ifeq ($INDEPEND_BUILD,1)
obj-$(CONFIG_MY_PLATFORM_LED) := my_platform_led.o
else # make INDEPEND_BUILD=1
obj-m := my_platform_led.o
KERNELDIR ?= /lib/modules/`uname -r`/build
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
.PHONY:clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif