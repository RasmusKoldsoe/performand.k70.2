#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -O2 -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=0 -DSTD_BUF_SIZE=64
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

COMPARSER_DIR = COM_Parser
HCIPARSER_DIR = HCI_Parser
GATTPARSER_DIR = GATT_Parser
NETWORKSTAT_DIR = NetworkStat
QUEUE_DIR = Queue
SERIALLOGIC_DIR = SerialLogic
SENSORS_DIR = Sensors
COMMON = ../Common
GPIO_DIR = $(COMMON)/GPIO
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping

VPATH = ../Common

OBJS = dev_tools.o \
       utils.o \
       HCI_Defs.o \
       Queue.o \
       NetworkStatistics.o \
       gpio_api.o \
       memory_map.o \
       serialLogic.o \
       COM_Parser.o \
       HCI_Parser.o \
       GATT_Parser.o \
       APP.o \
       speedLog.o \
       wind.o \
       compass.o

all : bleCentral

redo : clean bleCentral
	chmod +x bleCentral

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

GATT_Parser.o : $(GATTPARSER_DIR)/GATT_Parser.c
	$(CC) $(CFLAGS) -c $(GATTPARSER_DIR)/GATT_Parser.c

APP.o : APP.c
	$(CC) $(CFLAGS) -c APP.c

speedLog.o : $(SENSORS_DIR)/speedLog.c
	$(CC) $(CFLAGS) -c $(SENSORS_DIR)/speedLog.c

wind.o : $(SENSORS_DIR)/wind.c
	$(CC) $(CFLAGS) -c $(SENSORS_DIR)/wind.c

compass.o : $(SENSORS_DIR)/compass.c
	$(CC) $(CFLAGS) -c $(SENSORS_DIR)/compass.c

gpio_api.o : $(GPIO_DIR)/gpio_api.c
	$(CC) $(CFLAGS) -c $(GPIO_DIR)/gpio_api.c

memory_map.o : $(MEMORY_MAP_DIR)/memory_map.c
	$(CC) $(CFLAGS) -c $(MEMORY_MAP_DIR)/memory_map.c

utils.o : $(COMMON)/utils.c
	$(CC) $(CFLAGS) -c $(COMMON)/utils.c


clean:
	rm -f *.o bleCentral bleCentral.gdb


