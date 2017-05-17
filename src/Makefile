#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

CC=arm-linux-gcc
CXX=arm-linux-g++

#CC=gcc

OBJS = dnq_rabbitmq.o utils.o cJSON.o ngx_palloc.o dnq_log.o dnq_os.o \
	   dnq_lcd.o dnq_uart.c dnq_common.o dnq_mcu.o dnq_checksum.o \
		dnq_keypad.o dnq_network.o dnq_manage.o dnq_gpio.o \
		main.o

UPGRADE_OBJS = utils.o dnq_checksum.o ngx_palloc.o dnq_log.o dnq_os.o dnq_upgrade.o

INCLUDES = -I. -I$(TOP_DIR)/include
CFLAGS = -g -DHWVER=$(HardwareVersion) -DSWVER=$(SoftwareVersion)
LDFLAGS = -lm -lrabbitmq -lpthread
LDPATH = -L$(TOP_DIR)/lib
LIBS = 

AT := @
TARGET = dnq_manage dnq_upgrade

all: $(TARGET)

dnq_manage: $(OBJS)
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $(LDPATH) -static $^ -o $@.static $(LDFLAGS)
	@echo " LD   $^ "
	@echo " Create $@ success! (static compile)"
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDPATH) $(LDFLAGS) 
	@echo " LD   $^ "
	@echo " Create $@ success!"

dnq_upgrade: $(UPGRADE_OBJS)
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDPATH) $(LDFLAGS) 
	@echo " LD   $^ "
	@echo " Create $@ success!"

%.o: %.c
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@
	@echo " CC   $@ "

#.PHONY clean
clean:
	-$(AT)rm *.o
	-$(AT)rm $(TARGET)
