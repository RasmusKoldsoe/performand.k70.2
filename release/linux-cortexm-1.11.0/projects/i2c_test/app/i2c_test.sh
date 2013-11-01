#!/bin/sh
# This script tests the I2C device driver outside of the main kernel tree
# All relevant kernel components are loaded as modules

# Load I2C device driver. Whether i2c_stm32_debug= can be used,
# depends on how the driver is built (with DEBUG defined or not)
#insmod i2c-stm32.ko i2c_stm32_debug=0
insmod i2c-stm32.ko

# Load a module that registers the I2C controllers of SmartFusion
insmod i2c_stm32_dev.ko i2c_stm32_dev_debug=4

# Load a module that registers board-specific I2C devices
insmod i2c_stm32_slv.ko i2c_stm32_slv_debug=4

# Create device nodes for the I2C controllers
#mknod /dev/i2c-0 c 89 0
#mknod /dev/i2c-1 c 89 1

# Check that all in place for using I2C
ls -ls /sys/dev/char/ | busybox grep i2c
ls -lt /dev/i2c*

# Test I2C in varios ways
i2c_tools/i2cget -y 0 0x68 0 b
echo Hello EEPROM Flash! > /sys/devices/platform/i2c_stm32.0/i2c-0/0-0057/eeprom
#cp i2c-stm32.c /sys/devices/platform/i2c_stm32.0/i2c-0/0-0057/eeprom
#cat /sys/devices/platform/i2c_stm32.0/i2c-0/0-0057/eeprom > tmp.tmp
dd if=/sys/devices/platform/i2c_stm32.0/i2c-0/0-0057/eeprom of=tmp.tmp bs=1024 count=1
#i2c_tools/i2cget -y 0 0x68 0 b
#i2c_tools/i2cget -y 0 0x50 0 b
i2c_tools/i2cdetect -y 0
#i2c_tools/i2cdetect -y 1

# Clean-up so that the same script can be run again
# without having to reboot. Good for iteractive development
#rm /dev/i2c-0
#rm /dev/i2c-1
rmmod i2c_stm32_slv.ko
rmmod i2c_stm32_dev.ko
rmmod i2c-stm32.ko

