#!/bin/sh

#LED IND1
echo 65 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio65/direction

#LED IND2
echo 66 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio66/direction

#LED IND3
echo 67 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio67/direction

#Wifi Reset
echo 39 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio39/direction

#BLE Reset
echo 36 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio36/direction

#GPS Enable
echo 38 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio38/direction

while :
do 
	echo 1 > /sys/class/gpio/gpio65/value
	echo 1 > /sys/class/gpio/gpio66/value
	echo 1 > /sys/class/gpio/gpio67/value
	echo 1 > /sys/class/gpio/gpio38/value

	sleep 1
	echo 0 > /sys/class/gpio/gpio65/value
	echo 0 > /sys/class/gpio/gpio66/value
	echo 0 > /sys/class/gpio/gpio67/value
	echo 0 > /sys/class/gpio/gpio38/value

	sleep 1
done
