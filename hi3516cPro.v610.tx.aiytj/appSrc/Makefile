#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    Version      : 1.0
#    Author       : tianjun
#    Created      : 2015-02-10
#    Description  : 
#****************************************************************************/

include cc.rule
include dir.rule

#SUBDIRS    = $(shell find . -maxdepth 2 -mindepth 2 -name "[Mm]akefile")
#SUBDIRS    := $(foreach subdir, $(SUBDIRS), $(dir $(subdir)))
#SUBDIRS    := $(sort $(SUBDIRS))

SUBDIRS     = libs main


SUBDIRS         := $(foreach dir, $(SUBDIRS), $(shell [ -d $(dir) ] && echo $(dir)))										
INSTALLSUBDIRS  = $(addsuffix .install, $(SUBDIRS))
CLEANSUBDIRS    = $(addsuffix .clean, $(SUBDIRS))
SUBDIRSLIB      = $(addsuffix .lib, $(SUBDIRS))

all     : $(SUBDIRS)
install : $(INSTALLSUBDIRS)
clean   : $(CLEANSUBDIRS)
lib	: $(SUBDIRSLIB)

$(SUBDIRS) :
	make -C $@

$(INSTALLSUBDIRS) :
	make -C $(basename $@) install
	
$(CLEANSUBDIRS) :
	make -C $(basename $@) clean

$(SUBDIRSLIB) :
	make -C $(basename $@) lib

i:
	@du -sk	main/jco_server
	install	main/jco_server ../filesys/filesys_normal/ipc/app/jco_server
	install main/jco_server main/jco_server.`strings main/jco_server | awk '/verdttm/{print $$2;exit}'`
	#$(ARM_PREFIX)strip main/jco_server

simple:
	$(ARM_PREFIX)strip main/jco_server

dump:
	$(OBJDUMP) -d main/jco_server > a.asm
	
.PHONY  : i $(SUBDIRS) $(INSTALLSUBDIRS) $(CLEANSUBDIRS) $(SUBDIRSLIB)
