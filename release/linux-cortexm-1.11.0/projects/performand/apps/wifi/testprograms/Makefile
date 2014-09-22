#
# Makefile for eMPL Linux userland implementation
# 

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os  -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=2
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

COMMON = ../../Common
MEMORY_MAP_DIR = $(COMMON)/MemoryMapping

all : fgets_test

fgets_test : fgets_test.o
	$(CC) $(CFLAGS) fgets_test.o -lm -o fgets_test

mem_problem_test : memory_map.o mem_problem_test.o
	$(CC) $(CFLAGS) memory_map.o  mem_problem_test.o -lm -o mem_problem_test

scanf_test : scanf_test.o
	$(CC) $(CFLAGS) scanf_test.o -lm -o scanf_test
	chmod +x scanf_test

memory_map.o : $(MEMORY_MAP_DIR)/memory_map.c
	$(CC) $(CFLAGS) -c $(MEMORY_MAP_DIR)/memory_map.c

clean:
	rm -f *.o fgets_test mem_problem_test scanf_test *.gdb
