#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib
LDFLAGS	:= -mcpu=cortex-m3 -mthumb


all : sizeof_test


sizeof_test :
	$(CC) $(CFLAGS) sizeof_test.c -o sizeof_test


clean:
	rm -f *.o sizeof_test sizeof_test.gdb


