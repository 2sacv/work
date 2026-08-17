#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    Version      : 1.0
#    Author       : luosf
#    Created      : 2013-03-27
#    Description  : 
#****************************************************************************/

include cc.rule

SUBDIRS						= $(shell find . -maxdepth 3 -mindepth 2 -name "[Mm]akefile" | grep -v -w test)
SUBDIRS					 := $(foreach subdir, $(SUBDIRS), $(dir $(subdir)))
SUBDIRS					 := $(sort $(SUBDIRS))
#SUBDIRS					 += $(shell test -d test && echo test)
INSTALLSUBDIRS    = $(addsuffix .install, $(SUBDIRS))
CLEANSUBDIRS      = $(addsuffix .clean, $(SUBDIRS))

all     : $(SUBDIRS)
install : $(INSTALLSUBDIRS)
clean   : $(CLEANSUBDIRS)

$(SUBDIRS):
	make -C $@

$(INSTALLSUBDIRS) :
	@echo libs install do nothing
	
$(CLEANSUBDIRS) :	
	make -C $(basename $@) clean
	
.PHONY  : $(SUBDIRS) $(INSTALLSUBDIRS) $(CLEANSUBDIRS)
