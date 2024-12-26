#
# Makefile for nncp
# Copyright (c) 2018-2021 Fabrice Bellard
#
#CONFIG_WIN32=y

ifdef CONFIG_WIN32
CROSS_PREFIX=x86_64-w64-mingw32-
EXE=.exe
LIBEXT=.lib
DLLEXT=.dll
else
LIBEXT=.a
DLLEXT=.so
endif

HOST_CC=gcc
CC=$(CROSS_PREFIX)gcc
CXX=$(CROSS_PREFIX)g++
AR=$(CROSS_PREFIX)ar
CFLAGS_VERSION:=-DCONFIG_VERSION=\"$(shell cat VERSION)\"
CFLAGS=-O3 -Wall -Wpointer-arith -g -fno-math-errno -fno-trapping-math -MMD -Wno-format-truncation $(CFLAGS_VERSION) -DLIBNC_CONFIG_FULL
LDFLAGS=-Wl,-rpath='$$ORIGIN/'
PROGS=nncp$(EXE)
LIBS+=-lm -lpthread

all: $(PROGS)

clean:
	rm -f *.o *.d $(PROGS)

nncp$(EXE): nncp.o cmdopt.o cp_utils.o arith.o preprocess.o cutils.o \
        libnc$(DLLEXT)
	$(CC) $(LDFLAGS) -o $@ $^ -lz $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(wildcard *.d)
