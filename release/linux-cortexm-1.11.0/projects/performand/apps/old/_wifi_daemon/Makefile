#
# Makefile for eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

GS1500MDIR = gs1500m

OBJS = gs1500m.o

all : tcp_daemon

tcp_daemon : $(OBJS) tcp_daemon.o
	$(CC) $(CFLAGS) $(OBJS) tcp_daemon.o -lm -o tcp_daemon

gs1500m.o : $(GS1500MDIR)/gs1500m.c
	$(CC) $(CFLAGS) -I $(GS1500MDIR) -c $(GS1500MDIR)/gs1500m.c

clean:
	rm -f *.o tcp_daemon
