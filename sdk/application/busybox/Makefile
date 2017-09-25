#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

include $(TOP_DIR)/common.mk

BUSYBOX_SRC_NAME=busybox-1.22.1
BUSYBOX_SRC_DIR=$(BUSYBOX_SRC_NAME)

all: $(BUSYBOX_SRC_DIR)/tar

$(BUSYBOX_SRC_DIR)/tar:
	test -d $(BUSYBOX_SRC_DIR) || tar xvf $(BUSYBOX_SRC_NAME).tar.bz2 -C .
	cp -rf configs/busybox_config $(BUSYBOX_SRC_DIR)/.config
	make -C $(BUSYBOX_SRC_DIR)
	cp $(BUSYBOX_SRC_DIR)/busybox $(BIN_DIR)
	-cp $(BUSYBOX_SRC_DIR)/busybox $(ROOTFS_DIR)/bin
	#touch $@

clean:
	-@rm -rf $(BUSYBOX_SRC_DIR)

.PHONY: clean 

