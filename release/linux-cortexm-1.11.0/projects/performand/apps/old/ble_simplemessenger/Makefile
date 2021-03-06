#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=1
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

COMPARSER_DIR = COM_Parser
HCIPARSER_DIR = HCI_Parser
NETWORKSTAT_DIR = NetworkStat
QUEUE_DIR = Queue
SERIALLOGIC_DIR = SerialLogic
COMMON = ../Common
GPIO_DIR = $(COMMON)/GPIO
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping

VPATH = ../Common

OBJS = dev_tools.o \
       serialLogic.o \
       Queue.o \
       NetworkStatistics.o \
       HCI_Parser.o \
       COM_Parser.o \
       HCI_Defs.o \
       gpio_api.o \
       memory_map.o

all : bleCentral

HCI_Defs.o : $(HCIPARSER_DIR)/HCI_Defs.c
	$(CC) $(CFLAGS) -c $(HCIPARSER_DIR)/HCI_Defs.c

bleCentral : $(OBJS) bleCentral.o
	$(CC) $(CFLAGS) $(OBJS) bleCentral.o -lm -o bleCentral

dev_tools.o : dev_tools.c
	$(CC) $(CFLAGS) -c dev_tools.c
	
serialLogic.o : $(SERIALLOGIC_DIR)/serialLogic.c
	$(CC) $(CFLAGS) -c $(SERIALLOGIC_DIR)/serialLogic.c

Queue.o : $(QUEUE_DIR)/Queue.c
	$(CC) $(CFLAGS) -c $(QUEUE_DIR)/Queue.c

NetworkStatistics.o : $(NETWORKSTAT_DIR)/NetworkStatistics.c
	$(CC) $(CFLAGS) -c $(NETWORKSTAT_DIR)/NetworkStatistics.c

HCI_Parser.o : $(HCIPARSER_DIR)/HCI_Parser.c
	$(CC) $(CFLAGS) -c $(HCIPARSER_DIR)/HCI_Parser.c

COM_Parser.o : $(COMPARSER_DIR)/COM_Parser.c
	$(CC) $(CFLAGS) -c $(COMPARSER_DIR)/COM_Parser.c

gpio_api.o : $(GPIO_DIR)/gpio_api.c
	$(CC) $(CFLAGS) -c $(GPIO_DIR)/gpio_api.c

memory_map.o : $(MEMORY_MAP_DIR)/memory_map.c
	$(CC) $(CFLAGS) -c $(MEMORY_MAP_DIR)/memory_map.c

clean:
	rm -f *.o bleCentral bleCentral.gdb


