#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

include $(TOP_DIR)/common.mk

ROOTFS_SRC_NAME=rootfs_48strip
ROOTFS_SRC_DIR=$(ROOTFS_SRC_NAME)

all: $(ROOTFS_SRC_DIR)/tar

$(ROOTFS_SRC_DIR)/tar:
	test -d $(ROOTFS_SRC_DIR) || tar xvf $(ROOTFS_SRC_NAME).tar.bz2 -C .
	cp -rf $(ROOTFS_SRC_DIR) $(ROOTFS_DIR)
	#touch $@

clean:
	-@rm -rf $(ROOTFS_SRC_DIR)

.PHONY: clean 

