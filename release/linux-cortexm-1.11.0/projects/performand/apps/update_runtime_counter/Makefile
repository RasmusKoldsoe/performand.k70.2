CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -DVERBOSITY=0
LDFLAGS	:= -mcpu=cortex-m3 -mthumb 

VPATH = ../Common

OBJS_INC = inc_rt_cnt.o \
	       utils.o

OBJS_READ = read_rt_cnt.o \
			utils.o

COMMON = ../Common

all : inc_rt_cnt read_rt_cnt

inc_rt_cnt : $(OBJS_INC)
	$(CC) $(CFLAGS) $(OBJS_INC) -lm -o inc_runtime_cnt

read_rt_cnt : $(OBJS_READ)
	$(CC) $(CFLAGS) $(OBJS_READ) -lm -o read_runtime_cnt

inc_rt_cnt.o : inc_rt_cnt.c
	$(CC) $(CFLAGS) -c inc_rt_cnt.c

read_rt_cnt.o : read_rt_cnt.c
	$(CC) $(CFLAGS) -c read_rt_cnt.c

utils.o : $(COMMON)/utils.c
	$(CC) $(CFLAGS) -c $(COMMON)/utils.c

clean:
	rm -f *.o inc_runtime_cnt read_runtime_cnt

redo : clean all
	chmod +x inc_runtime_cnt read_runtime_cnt
