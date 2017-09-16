#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

include $(TOP_DIR)/common.mk

ROOTFS_SRC_NAME=rootfs_48strip
BUILD_DIR=$(ROOTFS_SRC_NAME)

all: $(BUILD_DIR)/tar

$(BUILD_DIR)/tar:
	test -d $(BUILD_DIR) || tar xvf $(ROOTFS_SRC_NAME).tar.bz2 -C .
	cp -rf rootfs_48strip $(ROOTFS_DIR)
	#touch $@

clean:
	-@rm -rf $(BUILD_DIR)

.PHONY: clean 

