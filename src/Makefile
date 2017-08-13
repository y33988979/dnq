#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

CC=arm-linux-gcc
CXX=arm-linux-g++

#CC=gcc

OBJS = dnq_rabbitmq.o utils.o cJSON.o ngx_palloc.o dnq_log.o dnq_os.o \
	   dnq_lcd.o dnq_uart.c dnq_common.o dnq_mcu.o dnq_sensor.o \
	   dnq_checksum.o dnq_keypad.o dnq_network.o dnq_manage.o dnq_gpio.o \
	   dnq_config.o main.o

UPGRADE_OBJS = utils.o dnq_checksum.o \
			   dnq_upgrade.o \
			   #dnq_network.o \
			   #dnq_os.o \
			   #dnq_log.o dnq_common.o \
			   ngx_palloc.o \

INCLUDES = -I. -I$(TOP_DIR)/include
CFLAGS = -g -DHWVER=$(HardwareVersion) -DSWVER=$(SoftwareVersion) -DUPGRD_SWVER=$(UPGRADE_SWVER) -Wl,--hash-style=sysv
LDFLAGS = -lm -lrabbitmq -lpthread -liconv
LDPATH = -L$(TOP_DIR)/lib
LIBS = 

AT := @
TARGET = dnq_manage dnq_upgrade 

all: prepare $(TARGET) version 

prepare:
	@touch main.c

version:
	-$(AT)rm version.txt
	$(AT)echo "HWVER:$(HardwareVersion)" > version.txt
	$(AT)echo "SWVER:$(SoftwareVersion)" >> version.txt

dnq_manage: $(OBJS)
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $(LDPATH) -static $^ -o $@.static $(LDFLAGS)
	@echo " LD   $^ "
	@echo " Create $@ success! (static compile)"
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDPATH) $(LDFLAGS) 
	@tar zcvf $@.tar.gz $@
	@echo " LD   $^ "
	@echo " Create $@ success!"

dnq_upgrade: $(UPGRADE_OBJS)
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDPATH) $(LDFLAGS) 
	@tar zcvf $@.tar.gz $@
	@echo " LD   $^ "
	@echo " Create $@ success!"

%.o: %.c
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@
	@echo " CC   $@ "

#.PHONY clean
clean:
	-rm *.o
	-rm $(TARGET)
