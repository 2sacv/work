#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    File Name    : Makefile
#    Version      : 1.0
#    Author       : JABSCO Video Server Software Group
#    Created      : 2015-03-11
#    Description  : Top Mafile
#    History      : 
#                        created by tianjun. 2015-03-11
#****************************************************************************/

SUBDIRS             += jco_hwtimer
SUBDIRS             += motor
SUBDIRS             += wifi-ws73
SUBDIRS             += sysconfig

INSTALLSUBDIRS      = $(addsuffix .install, $(SUBDIRS))
CLEANSUBDIRS        = $(addsuffix .clean, $(SUBDIRS))
DISTCLEANSUBDIRS    = $(addsuffix .distclean, $(SUBDIRS))


.PHONY : $(SUBDIRS)

all : $(INSTALLCHECK) $(KERNELCHECK) $(SUBDIRS)
install : $(INSTALLSUBDIRS)
clean : $(CLEANSUBDIRS)
distclean: $(DISTCLEANSUBDIRS)

$(SUBDIRS) : 
	@make -C $@

$(INSTALLSUBDIRS) :
	make -C $(basename $@) install
	
$(CLEANSUBDIRS) :
	make -C $(basename $@) clean
	
$(DISTCLEANSUBDIRS) :
	make -C $(basename $@) clean
#	make -C $(basename $@) distclean

include Rules.make

