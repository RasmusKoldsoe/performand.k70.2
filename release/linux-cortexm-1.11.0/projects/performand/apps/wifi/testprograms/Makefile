#
# Makefile for eMPL Linux userland implementation
# 

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os  -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=2
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

all : fgets_test

fgets_test : fgets_test.o
	$(CC) $(CFLAGS) fgets_test.o -lm -o fgets_test

scanf_test : scanf_test.o
	$(CC) $(CFLAGS) scanf_test.o -lm -o scanf_test
	chmod +x scanf_test

clean:
	rm -f *.o fgets_test *.gdb
