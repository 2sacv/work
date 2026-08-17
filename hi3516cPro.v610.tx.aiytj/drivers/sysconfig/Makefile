# Makefile
#
#============================================================================
#	Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
#
#	File Name	: Makefile
#	Version		: 1.0
#	Author		: JABSCO Video Server Software Group
#	Created		: 2024.12.18
#	Description	: Mafile
#	History		:
#                   modify by tangjx  2024.12.18 add jco_hwtimer
#============================================================================

TARGET := sys_config.ko
obj-m += sys_config.o
sys_config-objs := sys_cfg.o clk_cfg.o sys_ctrl.o pin_mux.o

EXTRA_CFLAGS += -DCONFIG_OT_SYS_KOL_SUPPORT

ccflags-y += -I./ -I$(PWD)/../../appSrc/sdklibs/include

ifeq (${KERNEL_DIR}, )
	KERNEL_DIR = $(PWD)/../../kernel/linux-5.10.221
endif

KDIR		:= $(KERNEL_DIR)
KDIRINC		= $(KDIR)/include
INC_FLAG	= -I$(KDIRINC) 
ARCH            := arm
CROSS_COMPILE 	:= arm-v01c02-linux-musleabi-
 
CPPFLAGS	+= -v 
CPPFLAGS	+= $(INC_FLAG) 

PWD             := $(shell pwd)

.PHONY: clean debug release install default

default: release

release:
	$(MAKE) -C$(KDIR) M=$(PWD)  ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

install: 
	install $(TARGET) $(INSTALL_DIR)
	$(CROSS_COMPILE)strip --strip-debug $(INSTALL_DIR)/$(TARGET)

clean:
	-rm -rf *.o *mod* *.ko .*.cmd .tmp* Module.symvers
