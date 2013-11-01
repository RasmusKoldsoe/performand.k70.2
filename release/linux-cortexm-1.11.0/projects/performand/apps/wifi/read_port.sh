#!/bin/sh

while :
do 
	echo "AT\r\n" > /dev/ttyS3 | cat /dev/ttyS3
	sleep 1
done
