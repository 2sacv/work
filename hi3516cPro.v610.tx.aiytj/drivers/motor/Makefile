# Makefile
#
#============================================================================
#	Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
#
#	File Name	: Makefile
#	Version		: 1.0
#	Author		: JABSCO Video Server Software Group
#	Created		: 2014.03.11
#	Description	: Mafile
#	History		:
#					Create by tianjun.2014.03.11
#                   modify by tangjx  2024.12.03 add jco_hwtimer
#============================================================================

TARGET := motor.ko
obj-m += motor.o ../jco_hwtimer/jco_hwtimer.o
motor-objs := mot.o xtm5809.o

ccflags-y += -I$(PWD)/../jco_hwtimer
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
