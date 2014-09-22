#
# Makefile for Cross-compilation of eMPL Linux userland implementation
#

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -mcpu=cortex-m3 -mthumb -Wall -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=0 -DSTD_BUF_SIZE=64
LDFLAGS	:= -mcpu=cortex-m3 -mthumb

DIR_GUARD = @mkdir -p $(@D)

BUILDDIR := build
BINARY := bleCentral
SOURCETREE = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call SOURCETREE,$d/,*.c))
EXT_SOURCES = $(call SOURCETREE,../Common/,*.c)

SOURCES := $(wildcard *.c) $(EXT_SOURCES) $(SOURCETREE) 
OBJECTS := $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:%.c=%.o)))
VPATH := $(subst  ,:,$(dir $(SOURCES)))


all : bleCentral

redo : clean $(BINARY)

bleCentral : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -lm -o bleCentral $(LDFLAGS)
	chmod +x $(BINARY)

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(dir $<) -c $< -o $@

clean:
	rm -rf $(OBJECTS) $(BINARY) $(BINARY).gdb


