#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    Version      : 1.0
#    Author       : luosf
#    Created      : 2013-03-27
#    Description  : 
#****************************************************************************/
MVTOOL_PREFIX = arm-v01c02-linux-musleabi-
CROSS_COMPILE 			=	$(MVTOOL_PREFIX)
C_COMPILER 					=	$(CROSS_COMPILE)gcc
CPLUSPLUS_COMPILER	=	$(CROSS_COMPILE)g++
LINK 								=	$(CROSS_COMPILE)g++ -o

INC_DIRS   = -I./
INC_DIRS   += -I../include
INC_DIRS   += -I../utils/inc
INC_DIRS   += -I../schedule/BasicUsageEnvironment/include
INC_DIRS   += -I../schedule/UsageEnvironment/include
INC_DIRS   += -I../schedule/groupsock/include
INC_DIRS   += -I../socket/inc

C_FLAGS 	 			+= $(INC_DIRS)
CPLUSPLUS_FLAGS	+= $(INC_DIRS)

C 	=	c
CPP =	cpp
OBJ =	o
EXE = 

LIBS	= $(shell find ../ -name "*.a")
APPS 	= test_time_call$(EXE) test_pthread_manage$(EXE) test_thread_pool$(EXE) cli ser
ALL 	= $(APPS)
all: $(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
	
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<
	
test_time_call_OBJS = test_time_call.$(OBJ)
test_pthread_manage_OBJS = test_pthread_manage.$(OBJ)
test_thread_pool_OBJS = test_thread_pool.$(OBJ)
cli_OBJS = cli.$(OBJ)
ser_OBJS = ser.$(OBJ)

test_time_call$(EXE):	$(test_time_call_OBJS) $(LIBS)
	$(LINK)$@ $(test_time_call_OBJS) $(LIBS) -lpthread

test_pthread_manage$(EXE) : $(test_pthread_manage_OBJS) $(LIBS)
	$(LINK) $@ $^ -lpthread
	
test_thread_pool$(EXE) : $(test_thread_pool_OBJS) $(LIBS)
	$(LINK) $@ $^ -lpthread
	
cli: $(cli_OBJS) $(LIBS)
	$(LINK) $@ $^

ser: $(ser_OBJS) $(LIBS)
	$(LINK) $@ $^ 
	
clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ 

##### Any additional, platform-specific rules come here:
