#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

CC=arm-linux-gcc
CXX=arm-linux-g++

#CC=gcc

OBJS = dnq_rabbitmq.o utils.o cJSON.o ngx_palloc.o dnq_log.o dnq_os.o \
	   dnq_lcd.o dnq_uart.c dnq_common.o

INCLUDES = -I. -I$(TOP_DIR)/include
CFLAGS = 
LDFLAGS = -lm -lrabbitmq -lpthread
LDPATH = -L$(TOP_DIR)/lib
LIBS = 

TARGET = dnq_manage

all: $(OBJS)
	$(CC) $(INCLUDES) $(LDPATH) -static $^ -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(INCLUDES) -c $^ -o $@

#.PHONY clean
clean:
	-@rm *.o
	-@rm $(TARGET)
