# Makefile
#
#============================================================================
#	Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
#
#	File Name	: Makefile
#	Version		: 1.0
#	Author		: JABSCO Video Server Software Group
#	Created		: 2024.12.02
#	Description	: Mafile
#	History		:
#					Create by tangjx.2024.12.02
#============================================================================

TARGET = jco_hwtimer.ko
obj-m += jco_hwtimer.o

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
	$(CROSS_COMPILE)strip --strip-debug $(INSTALL_DIR)/jco_hwtimer.ko

clean:
	-rm -rf *.o *mod* *.ko .*.cmd .tmp* Module.symvers
