#!/bin/sh

echo "AT\r\n" > /dev/ttyS3 & cat /dev/ttyS3

