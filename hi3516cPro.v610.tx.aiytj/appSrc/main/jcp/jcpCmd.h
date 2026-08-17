/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpCmd.h
 * @Created Time : 2013-12-24
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _JCPCMD_H_
#define _JCPCMD_H_
#ifdef __cplusplus
extern "C" {
#endif

//#include "jcpCmdImplement.h"

#define ARG_TYPE_ASK      (ArgTypesE)(ArgTypesNoPara | ArgTypesSingle | ArgTypesString)
#define ARG_TYPE_ACT      (ArgTypesE)(ArgTypesMust   | ArgTypesString)
#define ARG_TYPE_MUSTINT  (ArgTypesE)(ArgTypesMust | ArgTypesInt)


#define ARG_TYPE_LISTS    (ArgTypesE)(ArgTypesListOnly | ArgTypesString)
#define ARG_TYPE_LISTI    (ArgTypesE)(ArgTypesListOnly | ArgTypesInt)
#define ARG_TYPE_SETI     (ArgTypesE)(ArgTypesSetOnly | ArgTypesInt)
#define ARG_TYPE_SETS     (ArgTypesE)(ArgTypesSetOnly | ArgTypesString)

#define ARG_TYPE_LIST_MEMINT (ArgTypesE)(ArgTypesListTable | ArgTypesInt)       /* 重复成员的INT类型l*/
#define ARG_TYPE_LIST_MEMSTR (ArgTypesE)(ArgTypesListTable | ArgTypesString)    /* 重复成员的string类型l*/
#define ARG_TYPE_LIST_ID (ArgTypesE)(ArgTypesListTable | ArgTypesInt | ArgTypesID)  /* 重复成员组的ID*/

    typedef enum
    {
        ArgTypesMust       = 1<<0,
        ArgTypesSetMust    = 1<<1,
        ArgTypesListOnly   = 1<<2,
        ArgTypesSetOnly    = 1<<3,
        ArgTypesString     = 1<<7,  /*字符串*/
        ArgTypesChar       = 1<<8,  /*字符型*/
        ArgTypesInt        = 1<<9,  /*整型*/
        ArgTypesFloat      = 1<<10, /*浮点型*/
        ArgTypesSingle     = 1<<11, /*存在该选项，则不能设置其他选项，否则返回解析错误*/
        ArgTypesNoPara     = 1<<12, /*只有?帮助选项是这种类型*/
        ArgTypesListTable  = 1<<13, /*多通道成员标志位*/
        ArgTypesID         = 1<<14, /*id标记位*/
        ArgTypesArgFlag    = 1<<28, /*选项存在标志位*/
        ArgTypesValue      = 1<<29, /*参数值存在标志位*/
        ArgTypesEnd        = 1<<30,

    }
    ArgTypesE;

    typedef struct {
        const char* pOpt;               //命令参数识别字符
        unsigned int   argType;         //数据类型
        const char* pValueRegion;       //取值范围
        void*       pSetValue;          //传过来的参数
        int         nSize;              //数据空间大小
        const char* phelpMsg;
    } ArgOptS;

	typedef struct {
		const char* pfOpt;              //功能项识别字符
		ArgOptS     st_argopts;         //功能项参数结构体
	}ArgOptS_Expand;	

    typedef struct {
        const char*   pOpt;             // 参数选项
        const char*   phelpMsg;         //参数描述信息
    } HelpMsgS;

    typedef struct {
        unsigned short  version;      //0x01
        unsigned short  fixedFlag;    //0xFFFF
        int             cmdLen;       //jcp cmd length
        char            buf[0];
    } JCPDataHead;

    typedef int (*JcpPrcessFunc)(char*, int, int, char **);

    struct JcpCmdMap {
        const char          *cmd;
        JcpPrcessFunc       func;
    };

    extern struct JcpCmdMap JcpCmdAll[];

#ifdef __cplusplus
}
#endif
#endif


