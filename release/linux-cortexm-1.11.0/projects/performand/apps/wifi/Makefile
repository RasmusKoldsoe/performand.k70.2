#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=1
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

VPATH = ../Common

APIDIR = gainspan/API
ATDIR = gainspan/AT
HWDIR = gainspan/hardware
PTDIR = gainspan/platform

COMMON = ../Common
GPIO_DIR = $(COMMON)/GPIO
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping

OBJS = 	GS_API_device.o \
	GS_API_network.o \
	GS_API_platform.o \
	AtCmdLib.o \
	GS_HAL.o \
	mstimer.o \
	serialport.o \
	gpio_api.o \
	utils.o \
	memory_map.o

all : GS_Initialize_wifi write_tcp_stream

GS_Initialize_wifi : $(OBJS) GS_Initialize_wifi.o
	$(CC) $(CFLAGS) $(OBJS) -I $(APIDIR) -I $(ATDIR) GS_Initialize_wifi.o -lm -o GS_Initialize_wifi
	
write_tcp_stream : $(OBJS) write_tcp_stream.o
	$(CC) $(CFLAGS) $(OBJS) -I $(APIDIR) -I $(ATDIR) write_tcp_stream.o -lm -o w_tcp_str

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

memory_map.o : $(MEMORY_MAP_DIR)/memory_map.c
	$(CC) $(CFLAGS) -c $(MEMORY_MAP_DIR)/memory_map.c

utils.o : $(COMMON)/utils.c
	$(CC) $(CFLAGS) -c $(COMMON)/utils.c

gpio_api.o : $(GPIO_DIR)/gpio_api.c
	$(CC) $(CFLAGS) -c $(GPIO_DIR)/gpio_api.c

clean:
	rm -f *.o gs GS_Initialize_wifi w_tcp_str *.gdb
