#
# Copyright (C) ychen
# Copyright (C) Jiuzhou, Inc.
# 

include $(TOP_DIR)/common.mk

objects := busybox


objects_clean        := $(addsuffix _clean, $(objects))
objects_install      := $(addsuffix _install, $(objects))
objects_mod_install  := $(addsuffix _mod_install, $(objects))
objects_uninstall    := $(addsuffix _uninstall, $(objects))

depends := $(obj)
ifeq ($(depends),)
depends += $(objects)
endif

depends_clean        := $(addsuffix _clean, $(depends))
depends_install      := $(addsuffix _install, $(depends))
depends_mod_install  := $(addsuffix _mod_install, $(depends))
depends_uninstall    := $(addsuffix _uninstall, $(depends))

#==================================================================
#                           all
#==================================================================
.PHONY: all clean install mod_install uninstall prebuild

prebuild:

all: $(depends) prebuild

clean: $(depends_clean)

mod_install: $(depends_mod_install)

uninstall: $(depends_uninstall)

#==================================================================
#                        modules
#==================================================================
.PHONY: $(objects) $(objects_clean) $(objects_install) $(objects_mod_install) $(objects_uninstall)
	
$(objects):
	make -C $@ all

$(objects_clean):
	make -C $(patsubst %_clean,%,$@) clean

$(objects_install):
	make -C $(patsubst %_install,%,$@) install

$(objects_uninstall):
	 make -C $(patsubst %_uninstall,%,$@) uninstall

