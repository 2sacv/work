#****************************************************************************
#    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
# 
#    File Name    : Makefile
#    Version      : 1.0
#    Author       : JABSCO Video Server Software Group
#    Created      : 2013-03-14
# 
#                        created by zengy. 2013-03-14
#****************************************************************************/

include ../libs.rule

HTTPDEFS                    = -DHAVE__PROGNAME=1 -DHAVE_FCNTL_H=1 -DHAVE_GRP_H=1\
                                          -DHAVE_MEMORY_H=1 -DHAVE_PATHS_H=1 -DHAVE_POLL_H=1\
                                          -DHAVE_SYS_POLL_H=1 -DTIME_WITH_SYS_TIME=1 \
                                          -DHAVE_DIRENT_H=1 -DHAVE_LIBCRYPT=1 -DHAVE_STRERROR=1\
                                          -DHAVE_WAITPID=1 -DHAVE_VSNPRINTF=1 -DHAVE_DAEMON=1\
                                          -DHAVE_SETSID=1 -DHAVE_GETADDRINFO=1 -DHAVE_GETNAMEINFO=1\
                                          -DHAVE_GAI_STRERROR=1 -DHAVE_ATOLL=1\
                                          -DHAVE_UNISTD_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_MMAP=1\
                                          -DHAVE_SELECT=1 -DHAVE_POLL=1 -DHAVE_TM_GMTOFF=1\
                                          -DHAVE_INT64T=1 -DHAVE_SOCKLENT=1 -DWITHONVIF=1
HTTPDEFS      += -D__TWO_CHN_COUND_NOT_BE_SAME__=1
CFLAGS        += $(LIBS_INCFLAGS) $(HTTPDEFS)
CFLAGS        += $(SDK_LIBS_INCFLAGS) # for encode
LDFLAGS       += -L./ -lhttp -lonvif -lrecord #-lencode -lstream -lhanbang
LDFLAGS       += $(LIBS_LDFLAGS) # -lev
LDFLAGS       += $(SDK_LIBS_LDFLAGS)	# for encode

INC_DIRS      = $(shell find . -name "*.h" | grep -v "http//*" | grep -v "record//*" | grep -v "encode//*" | grep -v "stream//*")

INC_DIRS     := $(foreach incdir, $(INC_DIRS), $(dir $(incdir)))
INC_DIRS     := $(sort $(INC_DIRS))
INC_DIRS     := $(foreach incdir, $(INC_DIRS), -I$(incdir))
CFLAGS       += $(INC_DIRS)
EPOCH        := $(shell date +%s)
COMPILE_PKTM := $(shell date +%F.%H%M%S -d @$(EPOCH))
COMPILE_DTTM := $(shell date +%H%M.%Y%m%d -d @$(EPOCH))
COMPILE_DATE := $(shell date +%Y%m%d -d @$(EPOCH))
CFLAGS       += -DCOMPILE_DATE=\"$(COMPILE_DATE)\" -DCOMPILE_DTTM=\"$(COMPILE_DTTM)\" -DCOMPILE_PKTM=\"$(COMPILE_PKTM)\"
CFLAGS       += -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
CFLAGS       += -fmessage-length=0 -DWITH_OPENSSL

C_SRC       = $(shell find . -name "*.c" | grep -v "http//*" | grep -v "record//*" | grep -v "encode//*" | grep -v "stream//*" | grep -v "hxht//*" | grep -v "hanbang  //*")
C_OBJS      = $(C_SRC:%.c=%.o)

CPP_SRC     = $(shell find . -name "*.cpp" | grep -v "http//*" | grep -v "record//*" | grep -v "encode//*" | grep -v "stream//*" | grep -v "hxht//*" | grep -v "hanbang  //*")
CPP_OBJS    = $(CPP_SRC:%.cpp=%.o)

httpExist = $(shell if [ -d http ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(httpExist), exist)
HTTP_INC_DIRS      = $(shell find ./http/ -name "*.h")
HTTP_INC_DIRS      := $(foreach incdir, $(HTTP_INC_DIRS), $(dir $(incdir)))
HTTP_INC_DIRS      := $(sort $(HTTP_INC_DIRS))
HTTP_INC_DIRS      := $(foreach incdir, $(HTTP_INC_DIRS), -I$(incdir))

HTTP_C_SRC       = $(shell find ./http/ -name "*.c")
HTTP_C_OBJS      = $(HTTP_C_SRC:%.c=%.o)

HTTP_CPP_SRC     = $(shell find ./http/ -name "*.cpp")
HTTP_CPP_OBJS    = $(HTTP_CPP_SRC:%.cpp=%.o)

HTTP_O = $(HTTP_C_OBJS) $(HTTP_CPP_OBJS)

CFLAGS += $(HTTP_INC_DIRS)
else
EXTRA_LIBS   += ./libhttp.a
endif

onvifExist = $(shell if [ -d onvif_new ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(onvifExist), exist)
ONVIF_INC_DIRS      = $(shell find ./onvif_new/ -name "*.h")
ONVIF_INC_DIRS      := $(foreach incdir, $(ONVIF_INC_DIRS), $(dir $(incdir)))
ONVIF_INC_DIRS      := $(sort $(ONVIF_INC_DIRS))
ONVIF_INC_DIRS      := $(foreach incdir, $(ONVIF_INC_DIRS), -I$(incdir)) 

ONVIF_C_SRC       = $(shell find ./onvif_new/ -name "*.c")
ONVIF_C_OBJS      = $(ONVIF_C_SRC:%.c=%.o)

ONVIF_CPP_SRC     = $(shell find ./onvif_new/ -name "*.cpp")
ONVIF_CPP_OBJS    = $(ONVIF_CPP_SRC:%.cpp=%.o)

ONVIF_O = $(ONVIF_C_OBJS) $(ONVIF_CPP_OBJS)

CFLAGS += $(ONVIF_INC_DIRS)
else
EXTRA_LIBS   += ./libonvif.a
endif

recordExist = $(shell if [ -d record ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(recordExist), exist)
RECORD_INC_DIRS      = $(shell find ./record/ -name "*.h")
RECORD_INC_DIRS      := $(foreach incdir, $(RECORD_INC_DIRS), $(dir $(incdir)))
RECORD_INC_DIRS      := $(sort $(RECORD_INC_DIRS))
RECORD_INC_DIRS      := $(foreach incdir, $(RECORD_INC_DIRS), -I$(incdir))

RECORD_C_SRC       = $(shell find ./record/ -name "*.c")
RECORD_C_OBJS      = $(RECORD_C_SRC:%.c=%.o)

RECORD_CPP_SRC       = $(shell find ./record/ -name "*.cpp")
RECORD_CPP_OBJS      = $(RECORD_CPP_SRC:%.cpp=%.o)

RECORD_O = $(RECORD_C_OBJS) $(RECORD_CPP_OBJS)

CFLAGS += $(RECORD_INC_DIRS)
else
EXTRA_LIBS   += ./librecord.a
endif

encodeExist = $(shell if [ -d encode ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(encodeExist), exist)
ENCODE_INC_DIRS      = $(shell find ./encode/ -name "*.h")
ENCODE_INC_DIRS      := $(foreach incdir, $(ENCODE_INC_DIRS), $(dir $(incdir)))
ENCODE_INC_DIRS      := $(sort $(ENCODE_INC_DIRS))
ENCODE_INC_DIRS      := $(foreach incdir, $(ENCODE_INC_DIRS), -I$(incdir))

ENCODE_C_SRC       = $(shell find ./encode/ -name "*.c")
ENCODE_C_OBJS      = $(ENCODE_C_SRC:%.c=%.o)

ENCODE_CPP_SRC       = $(shell find ./encode/ -name "*.cpp")
ENCODE_CPP_OBJS      = $(ENCODE_CPP_SRC:%.cpp=%.o)

ENCODE_O = $(ENCODE_C_OBJS) $(ENCODE_CPP_OBJS)

CFLAGS += $(ENCODE_INC_DIRS)
endif

tencentExist = $(shell if [ -d tencent ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(tencentExist), exist)
TENCENT_INC_DIRS      = $(shell find ./tencent/ -name "*.h")
TENCENT_INC_DIRS      := $(foreach incdir, $(TENCENT_INC_DIRS), $(dir $(incdir)))
TENCENT_INC_DIRS      := $(sort $(TENCENT_INC_DIRS))
TENCENT_INC_DIRS      := $(foreach incdir, $(TENCENT_INC_DIRS), -I$(incdir))

TENCENT_C_SRC       = $(shell find ./tencent/ -name "*.c")
TENCENT_C_OBJS      = $(TENCENT_C_SRC:%.c=%.o)

TENCENT_CPP_SRC       = $(shell find ./tencent/ -name "*.cpp")
TENCENT_CPP_OBJS      = $(TENCENT_CPP_SRC:%.cpp=%.o)

TENCENT_O = $(TENCENT_C_OBJS) $(TENCENT_CPP_OBJS)

CFLAGS += $(TENCENT_INC_DIRS)
endif

streamExist = $(shell if [ -d stream ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(streamExist), exist)
STREAM_INC_DIRS      = $(shell find ./stream/ -name "*.h")
STREAM_INC_DIRS      := $(foreach incdir, $(STREAM_INC_DIRS), $(dir $(incdir)))
STREAM_INC_DIRS      := $(sort $(STREAM_INC_DIRS))
STREAM_INC_DIRS      := $(foreach incdir, $(STREAM_INC_DIRS), -I$(incdir))

STREAM_C_SRC       = $(shell find ./stream/ -name "*.c")
STREAM_C_OBJS      = $(STREAM_C_SRC:%.c=%.o)

STREAM_CPP_SRC       = $(shell find ./stream/ -name "*.cpp")
STREAM_CPP_OBJS      = $(STREAM_CPP_SRC:%.cpp=%.o)
#STREAM_CPP_OBJS      += ./stream/mediaServer/RecordInput.o

STREAM_O = $(STREAM_C_OBJS) $(STREAM_CPP_OBJS)

CFLAGS += $(STREAM_INC_DIRS)
else
EXTRA_LIBS   += ./libstream.a
endif

gb28181Exist = $(shell if [ -d gb2818 ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(gb28181Exist), exist)
GB28181_INC_DIRS      = $(shell find ./gb2818/ -name "*.h")
GB28181_INC_DIRS      := $(foreach incdir, $(GB28181_INC_DIRS), $(dir $(incdir)))
GB28181_INC_DIRS      := $(sort $(GB28181_INC_DIRS))
GB28181_INC_DIRS      := $(foreach incdir, $(GB28181_INC_DIRS), -I$(incdir))

GB28181_C_SRC       = $(shell find ./gb2818/ -name "*.c")
GB28181_C_OBJS      = $(GB28181_C_SRC:%.c=%.o)

GB28181_CPP_SRC       = $(shell find ./gb2818/ -name "*.cpp")
GB28181_CPP_OBJS      = $(GB28181_CPP_SRC:%.cpp=%.o)

GB28181_O = $(GB28181_C_OBJS) $(GB28181_CPP_OBJS)

CFLAGS += $(GB28181_INC_DIRS)
else
EXTRA_LIBS   += ./libgb28181.a
endif

EXTRA_OBJ   += ./jcp/cmdline_parse.o ./lib/qrEncode.o 
ifeq ($(streamExist), exist)
EXTRA_OBJ   += ./stream/JRTSPSender.o 
endif

ifeq ($(MIPS_VALGRIND), 1)
ifeq (,$(wildcard ./valgrind/valgrind_thread.c))
EXTRA_OBJ   += lib/valgrind.oo
endif
endif

ggwaveExist = $(shell if [ -d ggwave ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(ggwaveExist), exist)
GGWAVE_INC_DIRS      = $(shell find ./ggwave/ -name "*.h")
GGWAVE_INC_DIRS      := $(foreach incdir, $(GGWAVE_INC_DIRS), $(dir $(incdir)))
GGWAVE_INC_DIRS      := $(sort $(GGWAVE_INC_DIRS))
GGWAVE_INC_DIRS      := $(foreach incdir, $(GGWAVE_INC_DIRS), -I$(incdir))

GGWAVE_C_SRC       = $(shell find ./ggwave/ -name "*.c")
GGWAVE_C_OBJS      = $(GGWAVE_C_SRC:%.c=%.o)

GGWAVE_CPP_SRC       = $(shell find ./ggwave/ -name "*.cpp")
GGWAVE_CPP_OBJS      = $(GGWAVE_CPP_SRC:%.cpp=%.o)

GGWAVE_O = $(GGWAVE_C_OBJS) $(GGWAVE_CPP_OBJS)

CFLAGS += $(GGWAVE_INC_DIRS)
else
EXTRA_LIBS   += ./libggwave.a
endif

bleExist = $(shell if [ -d ble ]; then echo "exist"; else echo "notexist"; fi;)
ifeq ($(bleExist), exist)
BLE_INC_DIRS  = $(shell find ./ble/ -name "*.h")
BLE_INC_DIRS := $(foreach incdir, $(BLE_INC_DIRS), $(dir $(incdir)))
BLE_INC_DIRS := $(sort $(BLE_INC_DIRS))
BLE_INC_DIRS := $(foreach incdir, $(BLE_INC_DIRS), -I$(incdir))

BLE_C_SRC     = $(shell find ./ble/ -name "*.c")
BLE_C_OBJS    = $(BLE_C_SRC:%.c=%.o)
BLE_CPP_SRC   = $(shell find ./ble/ -name "*.cpp")
BLE_CPP_OBJS  = $(BLE_CPP_SRC:%.cpp=%.o)
BLE_O   = $(BLE_C_OBJS) $(BLE_CPP_OBJS)
CFLAGS += $(BLE_INC_DIRS)
else
EXTRA_LIBS += ./libble.a
endif

CPP_OBJS1   = $(filter-out $(no_need_obj),$(CPP_OBJS))

EXEC        = jco_server
all  : upversion lib $(EXEC)

$(EXTRA_OBJ) $(EXTRA_LIBS) : 
	cp ./lib/cmdline_parse.oo ./jcp/cmdline_parse.o
	cp ./lib/qrEncode.oo ./lib/qrEncode.o
	if [ -d "stream" ] ; then cp ./stream/JRTSPSender.oo ./stream/JRTSPSender.o; fi

lib:$(HTTP_O) $(ONVIF_O) $(RECORD_O) $(ENCODE_O) $(STREAM_O) $(TENCENT_O) $(EXTRA_OBJ) $(GB28181_O) $(GGWAVE_O) ${BLE_O} $(ALIYUN_MMI_O)
	if [ -d "http" ] ; then $(AR) -cr libhttp.a $(HTTP_O); fi
	if [ -d "onvif_new" ] ; then $(AR) -cr libonvif.a $(ONVIF_O); fi
	if [ -d "record" ] ; then $(AR) -cr librecord.a $(RECORD_O); fi
	if [ -d "encode" ] ; then $(AR) -cr libencode.a $(ENCODE_O) $(EXTRA_OBJ); fi
	if [ -d "stream" ] ; then $(AR) -cr libstream.a $(STREAM_O) $(EXTRA_OBJ); fi
	#if [ -d "tencent" ]; then $(AR) -cr libtencent.a $(TENCENT_O) $(EXTRA_OBJ); fi
	if [ -d "gb2818" ]; then $(AR) -cr libgb2818.a $(GB28181_O) $(EXTRA_OBJ); fi
	if [ -d "ggwave" ]; then $(AR) -cr libggwave.a $(GGWAVE_O) $(EXTRA_OBJ); fi
	if [ -d "ble" ] ; then $(AR) -cr libble.a $(BLE_O); fi

upversion:
	@echo ___________ up version ___________
	@rm -f conf/jconfig.o jcp/jcpCmdImplement.o

APP_LIB = libhttp.a librecord.a libencode.a libstream.a libggwave.a libble.a # libtencent.a

$(EXEC):$(C_OBJS) $(CPP_OBJS1) $(EXTRA_OBJ) $(APP_LIB)
	$(C++) -o $(EXEC) $^ $(LDFLAGS) -lssl -lcrypto -pthread -lcrypt
install:lib $(EXEC)
	install $(EXEC) $(INSTALL_DIR)
	@echo    backup jco_server succ

clean:
	find . -name "*.o" | xargs rm -fr
	find . -name "*~" | xargs rm -fr
	find . -name "*.orig" | xargs rm -fr
	find . -name "*.tmp" | xargs rm -fr
	rm -f *.o *~ *.orig *.tmp
	@if [ -d "http" ] ; then rm libhttp.a -f; fi
	@if [ -d "onvif_new" ] ; then rm libonvif.a -f; fi
	@if [ -d "record" ] ; then rm librecord.a -f; fi
	@if [ -d "encode" ] ; then rm libencode.a -f; fi
	@if [ -d "stream" ] ; then rm libstream.a -f; fi
	#@if [ -d "tencent" ] ; then rm libtencent.a -f; fi
	@if [ -d "gb2818" ] ; then rm libgb2818.a -f; fi
	@if [ -d "ble" ] ; then rm libble.a -f; fi
	rm -f $(EXEC)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

.cpp.o:
	$(C++) -c $(CFLAGS) $< -o $@

.PHONY: all clean install upversion

