/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : conftypedef.h
 * @Created Time : 2013-10-24
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _CONFTYPEDEF_H_
#define _CONFTYPEDEF_H_

#ifndef SUCCESS
#define SUCCESS     0
#define FAILURE     (-1)
#endif

#define  ARG_INT_RD_ONLY      (ArgTypeE)(ArgTypeInt | ArgTypeReadOnly)
#define  ARG_FLOAT_RD_ONLY    (ArgTypeE)(ArgTypeFloat | ArgTypeReadOnly)
#define  ARG_CHAR_RD_ONLY     (ArgTypeE)(ArgTypeChar | ArgTypeReadOnly)
#define  ARG_STRING_RD_ONLY   (ArgTypeE)(ArgTypeString | ArgTypeReadOnly)
//#define  ARG_TYPE_LIST_ID     (ArgTypeE)(ArgTypeListTable | ArgTypeInt | ArgTypesID)
//#define  ARG_TYPE_LIST_MEMINT (ArgTypeE)(ArgTypeListTable | ArgTypeInt)       /* 重复成员的INT类型l*/
//#define  ARG_TYPE_LIST_MEMSTR (ArgTypeE)(ArgTypeListTable | ArgTypeString)    /* 重复成员的string类型l*/


typedef int (*cbReLabel)(int, void **);

typedef enum {
    ArgTypeReadOnly   = 1<<0, /* 只读 */
    ArgTypeString     = 1<<3, /*字符串*/
    ArgTypeChar       = 1<<4, /*字符型*/
    ArgTypeInt        = 1<<5, /*整型*/
    ArgTypeFloat      = 1<<6, /*浮点型*/
    ArgTypeTree       = 1<<7,
    //ArgTypeListTable  = 1<<8,
    //ArgTypesID          = 1<<9,

    ArgTypeEnd        = 1<<30,
} ArgTypeE;

typedef enum {
    ConfBeagin = -1,
    ConfGet,
    ConfSet,
    ConfEnd,
} ConfAct;

typedef struct {
    const char*    pOpt;
    ArgTypeE       u32Type;
    const char*    pVauleRegion;
    void*          pValue;
    cbReLabel      pCb;
    void**         pCbArg;
} ArgOpt;

typedef struct {
    const char*  pOpt;
    ArgTypeE     type;
    void*        arg;
    int          iSize;
    const void*  deft;
    cbReLabel    pCb;
    void**       pCbArg;
} MapOptKey;

typedef struct {
    const char* pOpt;               //命令参数识别字符
    ArgTypeE    u32Type;            //数据类型
    const char* pVauleRegion;       //取值范围
    void*       pValue;             //传过来的参数
    int         szMbr;              //数据空间大小
    const void* def;
    cbReLabel    pCb;
    void**       pCbArg;
} ArgOptS_T;


#endif

