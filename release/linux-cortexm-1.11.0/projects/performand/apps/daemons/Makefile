CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -lrt -pthread -DVERBOSITY=0
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

VPATH = ../Common

OBJS = 	gps_utils.o \
	utils.o \
	memory_map.o \
	Queue.o \
	logging.o

COMMON = ../Common
GPIO_DIR = $(COMMON)/GPIO
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping
QUEUE_DIR = Queue

all : imu_daemon gps_daemon 

imu_daemon : $(OBJS) imu_daemon.o
	$(CC) $(CFLAGS) $(OBJS) imu_daemon.o -lm -o imu_daemon

gps_daemon : $(OBJS) gps_daemon.o
	$(CC) $(CFLAGS) $(OBJS) gps_daemon.o -lm -o gps_daemon
	chmod +x gps_daemon

set_time_test: set_time_test.o
	$(CC) $(CFLAGS) $(OBJS) set_time_test.o -lm -o set_time_test
	chmod +x set_time_test

gps_utils.o : gps_utils.c
	$(CC) $(CFLAGS) -c gps_utils.c

memory_map.o : $(MEMORY_MAP_DIR)/memory_map.c
	$(CC) $(CFLAGS) -c $(MEMORY_MAP_DIR)/memory_map.c

Queue.o : $(QUEUE_DIR)/Queue.c
	$(CC) $(CFLAGS) -c $(QUEUE_DIR)/Queue.c

utils.o : $(COMMON)/utils.c
	$(CC) $(CFLAGS) -c $(COMMON)/utils.c

logging.o : $(COMMON)/logging.c
	$(CC) $(CFLAGS) -c $(COMMON)/logging.c

clean:
	rm -f *.o imu_daemon gps_daemon rawGPSoutput *.gdb

redo : clean all
	chmod +x gps_daemon
	chmod +x rawGPSoutput
