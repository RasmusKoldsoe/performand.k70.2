#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

APIDIR = gainspan/API
ATDIR = gainspan/AT
HWDIR = gainspan/hardware
PTDIR = gainspan/platform
UTILSDIR = utils

OBJS = 	GS_API_device.o \
	GS_API_network.o \
	GS_API_platform.o \
	AtCmdLib.o \
	GS_HAL.o \
	mstimer.o \
	serialport.o \
	gpio_api.o

all : main write_tcp_stream

main : $(OBJS) main.o
	$(CC) $(CFLAGS) $(OBJS) -I $(APIDIR) -I $(ATDIR) -I $(UTILSDIR) main.o -lm -o main

write_tcp_stream : write_tcp_stream.o
	$(CC) $(CFLAGS) $(OBJS) -I $(APIDIR) -I $(ATDIR) write_tcp_stream.o -lm -o w_tcp_str

gpio_api.o : $(UTILSDIR)/gpio_api.c
	$(CC) $(CFLAGS) -I $(UTILSDIR) -c $(UTILSDIR)/gpio_api.c

GS_API_device.o : $(APIDIR)/GS_API_device.c
	$(CC) $(CFLAGS) -I $(APIDIR) -I $(ATDIR) -c $(APIDIR)/GS_API_device.c

GS_API_network.o : $(APIDIR)/GS_API_network.c
	$(CC) $(CFLAGS) -I $(APIDIR) -I $(ATDIR) -c $(APIDIR)/GS_API_network.c

GS_API_platform.o : $(APIDIR)/GS_API_platform.c
	$(CC) $(CFLAGS) -I $(APIDIR) -I $(ATDIR) -c $(APIDIR)/GS_API_platform.c

AtCmdLib.o : $(ATDIR)/AtCmdLib.c
	$(CC) $(CFLAGS) -I $(ATDIR) -I $(ATDIR) -c $(ATDIR)/AtCmdLib.c

GS_HAL.o : $(HWDIR)/GS_HAL.c
	$(CC) $(CFLAGS) -I $(HWDIR) -I $(ATDIR) -c $(HWDIR)/GS_HAL.c

mstimer.o : $(PTDIR)/mstimer.c
	$(CC) $(CFLAGS) -I $(PTDIR) -c $(PTDIR)/mstimer.c

serialport.o : serialport.c
	$(CC) $(CFLAGS) -c serialport.c

clean:
	rm -f *.o gs
