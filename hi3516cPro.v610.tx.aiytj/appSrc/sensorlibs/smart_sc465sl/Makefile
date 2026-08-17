#
# sensor lib Makefile
#

3A_PATH := $(ISP_PATH)/3a
LIBPATH = $(ISP_PATH)/out

ARFLAGS = rcv
ARFLAGS_SO = $(LIBS_LD_CFLAGS)
ARFLAGS_SO += -shared -fPIC -o
CFLAGS  += -Wall -fPIC -D$(OT_FPGA)
CFLAGS  += -O2
CFLAGS  += $(LIBS_CFLAGS)

ISP_INC := $(ISP_PATH)/include
CUR_INC := ./
SNS_COM_INC := ./../common/
3A_INC := $(3A_PATH)/include
INC := -I$(ISP_INC) -I$(CUR_INC) -I$(SNS_COM_INC) -I$(3A_INC) $(COMM_INC)
DEPEND_FILE := $(foreach file,$(subst -I, ,$(CUR_INC)), $(wildcard $(file)/*.h))

COMPILE = $(CC) $(CFLAGS) $(DFLAGS)

%.o: ./%.c $(DEPEND_FILE)
	@[ -e $(LIBPATH) ] || mkdir -p $(LIBPATH)
	@[ -e $(dir $@) ] || mkdir -p $(dir $@)
	@$(COMPILE) -o $@ -c $< $(INC)

SRCS = $(wildcard ./*.c ./../common/*.c)
OBJS = $(SRCS:%.c=%.o)

LIB_NAME := libsns_sc465sl
TARGETLIB := $(LIBPATH)/$(LIB_NAME).a
TARGETLIB_SO := $(LIBPATH)/$(LIB_NAME).so

all:$(TARGETLIB)
$(TARGETLIB):$(OBJS)
	@($(AR) $(ARFLAGS) $(TARGETLIB) $(OBJS))
	@($(CC) $(ARFLAGS_SO) $(TARGETLIB_SO) $(OBJS))

clean:
	@$(RM) -rf $(TARGETLIB) $(OBJS)
	@$(RM) -rf $(LIBPATH)

aa:
	echo "$(INC)"
