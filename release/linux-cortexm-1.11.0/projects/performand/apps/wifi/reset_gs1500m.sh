#!/bin/sh

#Wifi Reset
echo 39 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio39/direction

echo 1 > /sys/class/gpio/gpio39/value
sleep 1
echo 0 > /sys/class/gpio/gpio39/value
sleep 1
echo 1 > /sys/class/gpio/gpio39/value

echo 39 > /sys/class/gpio/unexport

