CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

OBJS = gps_utils.o 

all : gps_daemon

gps_daemon : $(OBJS) gps_daemon.o
	$(CC) $(CFLAGS) $(OBJS) gps_daemon.o -lm -o gps_daemon

gps_utils.o : gps_utils.c
	$(CC) $(CFLAGS) -c gps_utils.c
	
clean:
	rm -f *.o gps_daemon

