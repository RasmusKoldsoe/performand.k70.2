#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb
LDFLAGS	:= -mcpu=cortex-m3 -mthumb

VPATH = ../Common

COMMON = ../Common
GPIO_DIR = $(COMMON)/GPIO
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping

all : mast_rot

mast_rot : mast_rot.o
	$(CC) $(CFLAGS) mast_rot.o -lm -o mast_rot

mast_rot.o : mast_rot.c
	$(CC) $(CFLAGS) $(DEFS) -c mast_rot.c

clean:
	rm -f *.o gs
