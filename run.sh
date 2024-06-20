#!/usr/bin/env bash
# https://cloud.tencent.com/developer/article/1806436
# https://www.raspberrypi.com/documentation/computers/linux_kernel.html#cross-compile-the-kernel
export ARCH=arm64
export CROSS_COMPILE="aarch64-linux-gnu-"
# make O=./rpi_hw bcm2711_defconfig 
for i in $@
do
    case $i in
    -build)
        make O=./rpi_hw -j`nproc`
        exit 0;;
    -clean)
        make O=./rpi_hw clean
        exit 0;;
    esac        
done
   
