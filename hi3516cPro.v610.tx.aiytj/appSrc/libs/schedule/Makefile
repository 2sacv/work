#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    Version      : 1.0
#    Author       : luosf
#    Created      : 2013-03-27
#    Description  : 
#****************************************************************************/

SUBDIRS						= $(shell find . -maxdepth 2 -mindepth 2 -name "[Mm]akefile")
SUBDIRS					 := $(foreach subdir, $(SUBDIRS), $(dir $(subdir)))
SUBDIRS					 := $(sort $(SUBDIRS))
INSTALLSUBDIRS    = $(addsuffix .install, $(SUBDIRS))
CLEANSUBDIRS      = $(addsuffix .clean, $(SUBDIRS))


all     : $(SUBDIRS)
install : $(INSTALLSUBDIRS)
clean   : $(CLEANSUBDIRS)

$(SUBDIRS) :
	make -C $@

$(INSTALLSUBDIRS) :
	make -C $(basename $@) install
	
$(CLEANSUBDIRS) :
	make -C $(basename $@) clean
	
.PHONY  : $(SUBDIRS) $(INSTALLSUBDIRS) $(CLEANSUBDIRS)
