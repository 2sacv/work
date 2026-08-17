/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : debug.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2009-03-10
    Description  : 
    History      : 
                        created by lsf. 2009-03-10
******************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H


#ifdef __cplusplus
extern "C"{
#endif 

#include <syslog.h>
#include <libgen.h>
#include <stdio.h>

#include "g_sys.h"
#include "g_run.h"
#include "g_log.h"
#include "g_stat.h"

#include "logapi.h"

#ifndef MODULE
#define MODULE "server"
#endif

#define BIAS_P(a,b) ((MAX(a,b)-MIN(a,b))*1.0/((a+b)/2))

#define eq_bit_and(var, b)  (((var) & (b)) == (b))
#define is_mod0(var, c)     ((var)%(c) == 0)
#define is_inc_mod0(var, c) ((var++)%(c) == 0)
#define is_inc_modc(var, c) ((var++)%(c) == c-1)  // exec c run
#define is_modc(var, c) ((var)%(c) == (c)-1)  // exec c run

#ifndef is_float0
#define is_float0(v) (v >= -0.000001 && v <= 0.000001)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MIN3
#define MIN3(a, b, c) MIN(MIN(a,b),c)
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MAX3
#define MAX3(a, b, c) MAX(MAX(a,b),c)
#endif

#define RANGE(dig, min, max) (MIN(MAX(dig, min), max))

#ifndef BOOL
typedef unsigned int BOOL;
#endif

#ifndef FALSE
#define FALSE		0
#define TRUE		1
#endif

#ifndef SUCCESS
#define SUCCESS		0
#endif

#ifndef FAILURE
#define FAILURE		(-1)
#endif

#ifndef NULL
#define NULL		0L
#endif

#ifndef CONSOLE
#define CONSOLE stdout
#endif

#ifndef RETVOID
#define RETVOID free(NULL)
#endif

#define SEND_SEGV_SELF() (*((int *)NULL) = 0XBADF00D)

#ifndef XSTR
#define STR(s) #s
#define XSTR(s) STR(s)
#endif

#ifndef return_if_fail
#define return_if_fail(condi) do {                                         \
  if (!(condi)) {                                                          \
      fprintf(stderr, "\033[1;31m""%s:%5d fail\n", __FILE__,__LINE__);    \
      return;                                                              \
  }                                                                        \
} while (0)
#endif

#ifndef return_val_if_fail
#define return_val_if_fail(condi, ret) do {                                         \
  if (!(condi)) {                                                                   \
      fprintf(stderr, "\033[1;31m""%s:%5d fail(" #condi ")\n", __FILE__,__LINE__);  \
      return ret;                                                                   \
  }                                                                                 \
} while (0)
#endif

#ifndef break_if_fail
#define break_if_fail(condi, ret) {                                                      \
  if (!(condi)) {                                                                       \
      fprintf(stderr, "\033[1;31m""%s:%5d fail, ret %d\n", __FILE__,__LINE__, ret);       \
      break;                                                                            \
  }                                                                                     \
  }
#endif

#ifndef goto_exit_if_fail
#define goto_exit_if_fail(condi, tag, ret, fmt, args...) do {                               \
  if (!(condi)) {                                                                           \
    ret;                                                                                    \
    fprintf(stderr, "\033[1;31m""[*ERR*] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);\
    goto tag;                                                                               \
  }                                                                                         \
} while (0)
#endif

#ifndef goto_tag_if_fail
#define goto_tag_if_fail(condi, tag) do {                                           \
  if (!(condi)) {                                                                   \
    fprintf(CONSOLE, "%s|%d| fail (" #condi ")\n", __FILE__, __LINE__);             \
    goto tag;                                                                       \
  }                                                                                 \
} while (0)
#endif

#ifndef goto_if_4gfail
#define goto_if_4gfail(condi, tag) do {                                             \
          if (!(condi)) {                                                               \
            syslog(LOG_INFO, "\x1b[1;31m%s|%d| fail (" #condi ")\n\x1b[0m", __FILE__, __LINE__);       \
            goto tag;                                                                   \
          }                                                                             \
        } while (0)
#endif

#define __FG_R "31"
#define __FG_G "32"
#define __FG_Y "33"
#define __BG   "0"


#define COLOR_R(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_R, ##args); \
} while(0)

#define COLOR_G(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_G, ##args); \
} while(0)

#define COLOR_Y(fmt, args...) do { \
    printf("[%s:%5d] \x1b[%s;%sm" fmt "\x1b[0m\n", \
            (char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
} while(0)


#define DBG(fmt, args...) do { \
    fprintf(stdout, "\033[m""[-DBG-] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);    \
} while(0)

#define ERR(fmt, args...) do { \
    fprintf(stderr, "\033[1;31m""[*ERR*] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);    \
} while(0)

#define LOG(fmt, args...) do { \
	log_record(1, 1, 1, MODULE, "[%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);\
} while(0)

#ifndef SYSLOG
#define SYSLOG(fmt, args...) do { \
        fprintf(stdout, "[%s:%5d] \x1b[%s;%sm [-SYS-]" fmt "\x1b[0m\n", \
                (char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
        syslog(LOG_INFO,"[%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);            \
    } while(0)
#endif

#ifndef goto_if_fatal_err
#define goto_if_fatal_err(condi, tag, ret, fmt, args...) do { \
    if (!(condi)) {                                           \
        ret;                                                  \
        SYSLOG(fmt, ##args);                                  \
        goto tag;                                             \
    }                                                         \
} while (0)
#endif

#define WAR(fmt, args...) do { \
			fprintf(stdout, "[%s:%5d] \x1b[%s;%sm [-WAR-]" fmt "\x1b[0m\n", \
					(char *)__FILE__,__LINE__, __BG, __FG_Y, ##args); \
		} while(0)

#ifndef ASSERT
#define ASSERT(x) do { \
    if((x)){}else{printf("### ASSERT Fail:%s @[%s,%d])\n",#x,__FILE__,__LINE__);} \
}while(0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)	((int)(sizeof(array) / sizeof(array[0])))
#endif

#define RET_JUDGE(ret) do{  \
    if (S_OK != ret) { \
        ERR("[%s] bRet[0x%x] is ERROR! Please Check Code \n",__func__, ret);} \
    }while(0)

#define RET_PRINT(ret, fmt, args...) do{  \
    if (S_OK != ret) { \
        ERR("ret[0x%x]" fmt "\n", ret, ## args);} \
    }while(0)

#define RET_BREAK(ret, fmt, args...) \
    if (S_OK != ret) { \
        ERR("ret[0x%x]" fmt "\n", ret, ## args);\
        break;\
    }

#ifndef SYSLOG_RECORD
#define SYSLOG_RECORD(fmt, args...) do { \
    syslog(LOG_INFO,"[record][%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);            \
} while(0)
#endif


#ifndef PTR2INT
#define PTR2INT(ptr) ((ptr==NULL) ? 0 : *((int *)ptr))
#endif

#ifdef __cplusplus
}
#endif 

#endif

