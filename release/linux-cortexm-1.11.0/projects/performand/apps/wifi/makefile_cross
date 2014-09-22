#
# Makefile for eMPL Linux userland implementation
# 

CC = $(CROSS_COMPILE_APPS)gcc
CFLAGS := -Os -Wall -mcpu=cortex-m3 -mthumb -L ${INSTALL_ROOT}/A2F/root/usr/lib -pthread -DVERBOSITY=1
LDFLAGS	:= -mcpu=cortex-m3 -mthumb -lrt

DIR_GUARD = @mkdir -p $(@D)

BUILDDIR := build
BINARIES := data_parser GS_Initialize_wifi
SOURCETREE = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call SOURCETREE,$d/,*.c))
EXT_SOURCES = $(call SOURCETREE,../Common/,*.c)
GAINSPAN_SOURCES = $(call SOURCETREE,gainspan/,*.c)

SOURCES := $(wildcard *.c) $(EXT_SOURCES) $(GAINSPAN_SOURCES)
OBJECTS := $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:%.c=%.o)))
VPATH := $(subst  ,:,$(dir $(SOURCES)))

all : $(BINARIES)
.PHONY : all

redo : clean $(BINARIES)
	
data_parser GS_Initialize_wifi : $(OBJECTS)
	$(CC) $(CFLAGS) $(filter-out $(addprefix $(BUILDDIR)/, $(BINARIES:%=%.o)) , $(OBJECTS)) $(addprefix $(BUILDDIR)/, $(@:%=%.o)) -lm -o $@ $(LDFLAGS)
	chmod +x $@

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(dir $<) -c $< -o $@

clean:
	rm -rf $(OBJECTS) $(BINARIES) $(BINARIES).gdb
