#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 


all : conv_quaternion

conv_quaternion : $(OBJS) conv_quaternion.o
	$(CC) $(CFLAGS) conv_quaternion.o -lm -o conv_quaternion

