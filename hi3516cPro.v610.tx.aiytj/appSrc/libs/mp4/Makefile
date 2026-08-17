#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    Version      : 1.0
#    Author       : luosf
#    Created      : 2013-03-27
#    Description  : 
#****************************************************************************/

MVTOOL_PREFIX 			=$(ARM_PREFIX)

CROSS_COMPILE 			=	$(MVTOOL_PREFIX)
C_COMPILER 					=	$(CROSS_COMPILE)gcc
CPLUSPLUS_COMPILER	=	$(CROSS_COMPILE)g++

INC_DIRS		= $(shell find . -name "*.h")
INC_DIRS 	 := $(foreach incdir, $(INC_DIRS), $(dir $(incdir)))
INC_DIRS   := $(sort $(INC_DIRS))
INC_DIRS 	 := $(foreach incdir, $(INC_DIRS), -I$(incdir))


CFLAGS 	 	 += $(INC_DIRS) -I../include
EXTRA_LIB   = 

TARGET 			= lib$(notdir $(shell pwd)).a
all : $(TARGET)

C_SRC 			= $(shell find . -name "*.c")
C_OBJS 			= $(C_SRC:%.c=%.o)

CPP_SRC 		= $(shell find . -name "*.cpp")
CPP_OBJS 		= $(CPP_SRC:%.cpp=%.o)

ALL_OBJS		= $(C_OBJS) $(CPP_OBJS)

$(TARGET) : $(ALL_OBJS) $(EXTRA_LIB)
	$(AR) -cr $@ $^  
	
clean:
	find . -name "*.o" | xargs rm -fr
	find . -name "*~" | xargs rm -fr
	find . -name "*.orig" | xargs rm -fr
	find . -name "*.tmp" | xargs rm -fr
	rm -f *.o *~ *.orig *.tmp
	rm -f $(TARGET)

.c.o:
	$(C_COMPILER) -c $(CFLAGS) $< -o $@

.cpp.o:
	$(CPLUSPLUS_COMPILER) -c $(CFLAGS) $< -o $@
