#!/bin/sh

while :
do
	./batstat >> /mnt/temp/batlog.txt
	sleep 10
done
