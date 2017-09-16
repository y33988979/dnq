#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

ifeq ($(TOP_DIR),)
$(error "please source . setenv.sh first!!")	
endif

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

sdk:
	make -C sdk all

sdk_clean:
	make -C sdk clean

#.PHONY clean
clean:
	make -C src clean

distclean: sdk_clean clean
	-rm -rf $(PUB_DIR)/*

.PHONY: clean all sdk
