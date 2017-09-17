#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

include $(TOP_DIR)/common.mk

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

ROOTFS_APP_DIR=$(ROOTFS_DIR)/root/dnq

INCLUDES = -I. -I$(TOP_DIR)/include
CFLAGS = -g -DHWVER=$(HardwareVersion) -DSWVER=$(SoftwareVersion) -DUPGRD_SWVER=$(UPGRADE_SWVER) -Wl,--hash-style=sysv
LDFLAGS = -lm -lrabbitmq -lpthread -liconv
LDPATH = -L$(TOP_DIR)/lib
LIBS = 

AT := @
TARGET = dnq_manage dnq_upgrade macset

all: install image
install: prepare $(TARGET) version update
	@cp -rf $(TARGET) $(APP_DIR)

image: rootfs_jffs2 data_jffs2
	
prepare:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(UPDATE_DIR)
	@mkdir -p $(IMAGE_DIR)
	@mkdir -p $(DATA_DIR)
	@touch main.c

version:
	$(AT)echo "HWVER:$(HardwareVersion)" > version.txt
	$(AT)echo "SWVER:$(SoftwareVersion)" >> version.txt
	$(AT)echo "UPGRADE_SWVER:$(UPGRADE_SWVER)" >> version.txt

macset:
	$(AT)$(CC) macset.c -o macset
	@echo -e "\e[32m Create $@ success! \e[0m"

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

jffs2_image: rootfs_jffs2 data_jffs2
rootfs_jffs2: $(TARGET)
	@cp -rf $(TARGET) $(ROOTFS_APP_DIR)
	@cp -rf $(BIN_DIR)/rcS $(ROOTFS_DIR)/etc/init.d
	@cp -rf $(BIN_DIR)/profile $(ROOTFS_DIR)/etc/profile
	@cp -rf $(BIN_DIR)/busybox $(ROOTFS_DIR)/bin/busybox
	$(TOP_DIR)/bin/mkfs.jffs2 -s 0x1000 -e 0x10000 --pad=0x700000 -d $(ROOTFS_DIR) -o $(IMAGE_DIR)/rootfs_$(HardwareVersion)_$(SoftwareVersion).jffs2 -n -l
	@echo -e "\e[32m Create $@ success! \033[0m"

data_jffs2:
	$(TOP_DIR)/bin/mkfs.jffs2 -s 0x1000 -e 0x10000 --pad=0x380000 -d $(DATA_DIR) -o $(IMAGE_DIR)/data_$(HardwareVersion)_$(SoftwareVersion).jffs2 -n -l
	@echo -e "\e[32m Create $@ success! \033[0m"

manage_update: dnq_manage version.txt
	@tar zcvf $<.tar.gz $^
	#@crc32 $<.tar.gz | tr [a-z] [A-Z] > crc32.txt
	./mkdesc.sh $<.tar.gz manage_desc.txt
	cp $<.tar.gz manage_desc.txt $(UPDATE_DIR)
	@echo -e "\e[32m Create $<.tar.gz \033[0m"

upgrade_update: dnq_upgrade version.txt
	@tar zcvf $<.tar.gz $^
	./mkdesc.sh $<.tar.gz upgrd_desc.txt
	cp $<.tar.gz upgrd_desc.txt $(UPDATE_DIR)
	@echo -e "\e[32m Create $<.tar.gz \033[0m"

update_all: dnq_upgrade dnq_manage version.txt
	@tar zcvf $@.tar.gz $^
	./mkdesc.sh $@.tar.gz update_desc.txt
	cp $@.tar.gz update_desc.txt $(UPDATE_DIR)
	@echo -e "\e[32m Create $@.tar.gz \e[0m"

update: upgrade_update manage_update update_all
	@echo -e "\e[32m Create update success! \e[0m"

%.o: %.c
	$(AT)$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@
	@echo " CC   $@ "

#.PHONY clean
clean:
	-@rm -rf *.o
	-@rm -rf *.tar.gz *.txt
	-@rm -rf $(UPDATE_DIR)
	-@rm -rf $(TARGET)
