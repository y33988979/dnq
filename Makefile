#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

TOP_DIR=${PWD}
include common.mk

CC=arm-linux-gcc
CXX=arm-linux-g++

INCLUDES := -I. -I$(TOP_DIR)/include
CFLAGS := 
LDFLAGS := -lm -lrabbitmq -lpthread
LDPATH := -L$(TOP_DIR)/lib
LIBS = 


DEPEND_LIB := lib/librabbitmq.so
TARGET = dnq

all: $(DEPEND_LIB)
	make -C src all

$(DEPEND_LIB):
	make -C extern

test:
	make -C tests all

test_clean:
	make -C tests clean

#.PHONY clean
clean:
	make -C src clean
