#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib
LDFLAGS	:= -mcpu=cortex-m3 -mthumb

OBJS = gpio_api.o

all : gpio_main_app

gpio_main_app : $(OBJS) gpio_main_app.o
	$(CC) $(CFLAGS) $(OBJS) gpio_main_app.o -lm -o gpio_main_app

gpio_api.o : gpio_api.c
	$(CC) $(CFLAGS) -c gpio_api.c

clean:
	rm -f *.o main main.gdb

