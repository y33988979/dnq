#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

CC=arm-linux-gcc
CXX=arm-linux-g++

#CC=gcc

OBJS = dnq_rabbitmq.o utils.o cJSON.o ngx_palloc.o dnq_log.o dnq_os.o \
	   dnq_lcd.o dnq_uart.c dnq_common.o dnq_mcu.o dnq_keypad.o main.o

INCLUDES = -I. -I$(TOP_DIR)/include
CFLAGS = -g
LDFLAGS = -lm -lrabbitmq -lpthread
LDPATH = -L$(TOP_DIR)/lib
LIBS = 

AT := @
TARGET = dnq_manage

all: $(OBJS)
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $(LDPATH) -static $^ -o $(TARGET).static $(LDFLAGS)
	@echo " LD   $^ "
	@echo " Create $(TARGET) success! (static compile)"
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $^ -o $(TARGET) $(LDPATH) $(LDFLAGS) 
	@echo " LD   $^ "
	@echo " Create $(TARGET) success!"

%.o: %.c
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@
	@echo " CC   $@ "

#.PHONY clean
clean:
	-$(AT)rm *.o
	-$(AT)rm $(TARGET)
