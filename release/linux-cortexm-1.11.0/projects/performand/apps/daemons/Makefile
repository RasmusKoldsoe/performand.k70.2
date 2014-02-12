CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -lrt -pthread -DVERBOSITY=1
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

VPATH = ../Common

OBJS = 	gps_utils.o \
	utils.o \
	memory_map.o 

COMMON = ../Common
GPIO_DIR = $(COMMON)/GPIO
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping

all : imu_daemon gps_daemon testing

imu_daemon : $(OBJS) imu_daemon.o
	$(CC) $(CFLAGS) $(OBJS) imu_daemon.o -lm -o imu_daemon

gps_daemon : $(OBJS) gps_daemon.o
	$(CC) $(CFLAGS) $(OBJS) gps_daemon.o -lm -o gps_daemon
	chmod +x gps_daemon

set_time_test: set_time_test.o
	$(CC) $(CFLAGS) $(OBJS) set_time_test.o -lm -o set_time_test
	chmod +x set_time_test

testing : $(OBJS) testing.o
	$(CC) $(CFLAGS) $(OBJS) testing.o -lm -o testing

gps_utils.o : gps_utils.c
	$(CC) $(CFLAGS) -c gps_utils.c

memory_map.o : $(MEMORY_MAP_DIR)/memory_map.c
	$(CC) $(CFLAGS) -c $(MEMORY_MAP_DIR)/memory_map.c

utils.o : $(COMMON)/utils.c
	$(CC) $(CFLAGS) -c $(COMMON)/utils.c

clean:
	rm -f *.o imu_daemon gps_daemon testing *.gdb

redo : clean all
	chmod +x gps_daemon
