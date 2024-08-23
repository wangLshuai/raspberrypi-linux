# SPDX-License-Identifier: GPL-2.0

ifeq ($INDEPEND_BUILD,1)
obj-$(CONFIG_I2C_SHT2x) := i2c-sht2x.o
else # make INDEPEND_BUILD=1
obj-m := i2c-sht2x.o
KERNELDIR ?= /lib/modules/`uname -r`/build
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
.PHONY:clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif