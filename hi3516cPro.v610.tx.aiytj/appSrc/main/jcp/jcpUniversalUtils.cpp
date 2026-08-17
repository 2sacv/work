/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpUniversalUtils.cpp
 * @Created Time : 2013-12-26
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "jcpCmd.h"
#include "jcpUniversalUtils.h"

#define WRITE_HELP_MSG(buf, buflen, len, arg...)         \
do{                                             \
         len = snprintf(buf, buflen, "  -%s  \r\n"          \
                 "    type  : %s \r\n"            \
                 "    scope : %s \r\n"           \
                 "    desc  : %s\r\n", ##arg);  \
} while(0)

typedef struct {
    unsigned int type;
    const char*  note;
} TypeNoteS;

static int       jcpArgGetDownAndUp(char* pszSHead, char* pszSTail, char *szUP, char *szDOWN);
static int       jcpIsAllTheZero(char *arr, int size);
static int       argPrintOpt(ArgOptS *pOpt, char *buf, int buflen, HelpMsgS *helps);

static ArgOptS *argFindOptStruct(ArgOptS *opts, char *str);
static ArgOptS_Expand *argFindOptStruct_ex(ArgOptS_Expand *opts, char *str);
static int jcpArgSetValue(ArgOptS *opt, char *szArgv, int gidx , void *array, int arr_size);
static int jcpArgCheckRegion(ArgOptS *opt, char *pArgv);


int parser_jcp_arg(int argc, char **argv, ArgOptS opts[], char *buf, void *array, int str_size)
{
    int     argIdx = 0;
    int     ret = -1;
    char    *pCurArg = NULL;
    ArgOptS *pCurOpt = NULL;
    int     hasArg = FALSE;
    int     gidx = -1;

    if (1 == argc || (argc == 2 && !(argv[1][0] == '-' && argv[1][1] == '?')) || (argc > 2 && (argc % 2) == 0)) {
        sprintf(buf, "argc = %d", argc);
        return FAILURE;
    }

    //argv[0] is execute command
    for(argIdx = 1; argIdx < argc; argIdx++) {
        pCurArg = argv[argIdx];

        if ('-' == *pCurArg) {
            //不能出现两个连续带"- -"的情况?

            if (hasArg == TRUE) { //上一个没带参数
                sprintf(buf, "[%s] no value", pCurOpt == NULL? pCurArg : pCurOpt->pOpt);
                return FAILURE;
            }

            pCurArg++;

            if (0 == *pCurArg) {             //只有'-'的情况
                sprintf(buf, "need param, only [%s], the front parameter is [%s]",\
                        argv[argIdx], argv[argIdx-1]);
                return FAILURE;
            }

            pCurOpt = argFindOptStruct(opts, pCurArg);  //获取当前命令参数
            if (NULL == pCurOpt) {
                WAR("not recogonized option [%s]\n", pCurArg);
                argIdx++;
                continue;
            }

            // 不带参数
            if((pCurOpt->argType & ArgTypesNoPara) == ArgTypesNoPara) {
                hasArg = FALSE;
                pCurOpt->argType |= ArgTypesValue;
            } else {
                hasArg = TRUE;
            }
            continue;
        } else if (FALSE == hasArg) {
            sprintf(buf, "the parameter of [%s] -- not value.", argv[argIdx-1]);
            return FAILURE;
        }

        //多通道项
        if ((pCurOpt->argType & ArgTypesListTable) == ArgTypesListTable) {
            if ((pCurOpt->argType & ArgTypesID) == ArgTypesID) {
                gidx = atoi(argv[argIdx]);      //取得要设置的id
                //continue;                     //id不会改变
            }
            if (gidx == -1) {       //找不到id的情况。
                sprintf(buf, "must set [id]");
                return FAILURE;
            }

            if (gidx >= str_size) {
                sprintf(buf, "id [%d] > size [%d]", gidx, str_size);
                return FAILURE;
            }
            ret = jcpArgSetValue(pCurOpt, argv[argIdx], gidx, array, str_size);
        } else {
            ret = jcpArgSetValue(pCurOpt, argv[argIdx], 0, NULL, str_size);
        }

        if (ret != SUCCESS) {
            sprintf(buf, "the region of [%s] is [%s], or buflen is %d",
                    pCurOpt->pOpt, pCurOpt->pValueRegion, pCurOpt->nSize);
            return ret;
        }

        hasArg = FALSE;
    }

    /*检测是否有多个ArgTypeSingle被设置，或者ArgTypeSingle与非ArgTypeSingle同时被设置*/
    int singleSet = 0, nonSingleSet = 0;
    for(int i=0; (opts[i].argType & ArgTypesEnd) != ArgTypesEnd; i++) {
        if((opts[i].argType & ArgTypesValue) == ArgTypesValue) {
            ((opts[i].argType & ArgTypesSingle) == ArgTypesSingle) ? singleSet++ : nonSingleSet++;
        }
    }

    if((singleSet > 1) || ((singleSet && nonSingleSet) != 0)) {
        sprintf(buf, "the parameter of [%s] not value.", pCurOpt->pOpt);
        return FAILURE;
    }

    /* 检测有没有必须设置项， 却没有设置 */
    if(0 == singleSet) {
        for(int i=0; (opts[i].argType & ArgTypesEnd) != ArgTypesEnd; i++) {
            if(((opts[i].argType & ArgTypesValue) != ArgTypesValue) && ((opts[i].argType & ArgTypesMust) == ArgTypesMust)) {
                sprintf(buf, "the parameter of [%s] must be set.", opts[i].pOpt);
                return FAILURE;
            }
        }
    }

    return SUCCESS;

}

int parser_jcp_arg_ex(int argc, char **argv, ArgOptS_Expand opts[], char *buf, void *array, int str_size)
{
    int         argIdx = 0;
    int         ret = -1;
    char        *pCurArg = NULL;
    ArgOptS_Expand   *pCurOpt = NULL;
    int         hasArg = FALSE;
    int         gidx = -1;
    if(1 == argc) {
        sprintf(buf, "argc = %d", argc);
        return FAILURE;
    }

    //argv[0] is execute command
    for(argIdx = 1; argIdx < argc; argIdx++) {
        pCurArg = argv[argIdx];

        if('-' == *pCurArg) {
            //不能出现两个连续带"- -"的情况?

            if (hasArg == TRUE) { //上一个没带参数
                sprintf(buf, "[%s] no value", pCurOpt->st_argopts.pOpt);
                return FAILURE;
            }

            pCurArg++;

            if(0 == *pCurArg) {             //只有'-'的情况
                sprintf(buf, "need param, only [%s], the front parameter is [%s]",\
                        argv[argIdx], argv[argIdx-1]);
                return FAILURE;
            }

            pCurOpt = argFindOptStruct_ex(opts, pCurArg);  //获取当前命令参数
            if(NULL == pCurOpt) {
                sprintf(buf, "[%s] is error! not find the parameter.", pCurArg);
                return FAILURE;
            }

            // 不带参数
            if((pCurOpt->st_argopts.argType & ArgTypesNoPara) == ArgTypesNoPara) {
                hasArg = FALSE;
                pCurOpt->st_argopts.argType |= ArgTypesValue;
            } else {
                hasArg = TRUE;
            }
            continue;
        } else if(FALSE == hasArg) {
            sprintf(buf, "the parameter of [%s] -- not value.", argv[argIdx-1]);
            return FAILURE;
        }

        //多通道项
        if ((pCurOpt->st_argopts.argType & ArgTypesListTable) == ArgTypesListTable) {
            if ((pCurOpt->st_argopts.argType & ArgTypesID) == ArgTypesID) {
                gidx = atoi(argv[argIdx]);      //取得要设置的id
                //continue;                     //id不会改变
            }
            if (gidx == -1) {       //找不到id的情况。
                sprintf(buf, "must set [id]");
                return FAILURE;
            }

            if (gidx >= str_size) {
                sprintf(buf, "id [%d] > size [%d]", gidx, str_size);
                return FAILURE;
            }
            ret = jcpArgSetValue(&(pCurOpt->st_argopts), argv[argIdx], gidx, array, str_size);
        } else {
            ret = jcpArgSetValue(&(pCurOpt->st_argopts), argv[argIdx], 0, NULL, str_size);
        }
        if(ret != SUCCESS) {
            sprintf(buf, "the region of [%s] is [%s], or buflen is %d",
                    pCurOpt->st_argopts.pOpt, pCurOpt->st_argopts.pValueRegion, pCurOpt->st_argopts.nSize);
            return ret;
        }

        hasArg = FALSE;

    }

    /*检测是否有多个ArgTypeSingle被设置，或者ArgTypeSingle与非ArgTypeSingle同时被设置*/
    int singleSet = 0, nonSingleSet = 0;
    for(int i=0; (opts[i].st_argopts.argType & ArgTypesEnd) != ArgTypesEnd; i++) {
        if((opts[i].st_argopts.argType & ArgTypesValue) == ArgTypesValue) {
            ((opts[i].st_argopts.argType & ArgTypesSingle) == ArgTypesSingle) ? singleSet++ : nonSingleSet++;
        }
    }

    if((singleSet > 1) || ((singleSet && nonSingleSet) != 0)) {
        sprintf(buf, "the parameter of [%s] not value.", pCurOpt->st_argopts.pOpt);
        return FAILURE;
    }

    /* 检测有没有必须设置项， 却没有设置 */
    if(0 == singleSet) {
        for(int i=0; (opts[i].st_argopts.argType & ArgTypesEnd) != ArgTypesEnd; i++) {
            if(((opts[i].st_argopts.argType & ArgTypesValue) != ArgTypesValue) && ((opts[i].st_argopts.argType & ArgTypesMust) == ArgTypesMust)) {
                sprintf(buf, "the parameter of [%s] must be set.", opts[i].st_argopts.pOpt);
                return FAILURE;
            }
        }
    }

    return SUCCESS;

}


int arg_opt_if_set(const char *optName, ArgOptS opts[])
{
    if(NULL == optName) {
        return FAILURE;
    }

    for(int i = 0; opts[i].argType != ArgTypesEnd; i++) {
        if(strcmp(opts[i].pOpt, optName)) {
            continue;
        }

        if(ArgTypesArgFlag == (opts[i].argType & ArgTypesArgFlag)) {
            return SUCCESS;
        }
    }

    return FAILURE;
}

int arg_opt_if_set_ex(const char *optName, ArgOptS_Expand opts[])
{
    if(NULL == optName) {
        return FAILURE;
    }

    for(int i = 0; opts[i].st_argopts.argType != ArgTypesEnd; i++) {
        if(strcmp(opts[i].st_argopts.pOpt, optName)) {
            continue;
        }

        if(ArgTypesArgFlag == (opts[i].st_argopts.argType & ArgTypesArgFlag)) {
            return SUCCESS;
        }
    }

    return FAILURE;
}

int  help_jcp_arg_msg(int argc, char **argv, char *buf, int buflen,ArgOptS opts[], HelpMsgS helps[])
{
    char *p = buf;
    char tbuf[512] = {0};

    printf("jcpcmd  = %s\n", argv[0]);
    sprintf(buf, "\r\n%s \r\n功能概要:\r\n    %s "
            "\r\n\r\n参数说明: \r\n", argv[0], helps[0].phelpMsg);
    p += strlen(buf);
    int i = 0;
    for(i = 1; opts[i].argType != ArgTypesEnd; i++) {
        if (strcmp(helps[i].pOpt, opts[i].pOpt) != 0) {
            sprintf(buf, "[%d, %s]param is not match", i, helps[i].pOpt);
            return FAILURE;
        }

        argPrintOpt(&opts[i], tbuf, sizeof(tbuf), &helps[i]);
        if (int(p-buf)+(int)strlen(tbuf) >= buflen){
            ERR("jcpcmd buf not enough for cmd %s helps\n",argv[0]);
            return FAILURE;
        }
        p += sprintf(p,"%s",tbuf);
    }

    sprintf(p, "\r\n命令举例:\r\n    %s \r\n", helps[i].phelpMsg);
    return SUCCESS;
}

int  help_jcp_arg_msg_ex(int argc, char **argv, char *buf, int buflen, ArgOptS_Expand opts[], HelpMsgS helps[])
{
    char *p = buf;

    printf("jcpcmd  = %s\n", argv[0]);
    sprintf(buf, "\r\n%s \r\n功能概要:\r\n    %s "
            "\r\n\r\n参数说明: \r\n", argv[0], helps[0].phelpMsg);
    p += strlen(buf);
    int i = 0;
    for(i = 1; opts[i].st_argopts.argType != ArgTypesEnd; i++) {
        if (strcmp(helps[i].pOpt, opts[i].st_argopts.pOpt) != 0) {
            sprintf(buf, "[%d, %s]param is not match", i, helps[i].pOpt);
            return FAILURE;
        }
        p += argPrintOpt(&opts[i].st_argopts, p, buflen, &helps[i]);
    }
    sprintf(p, "\r\n命令举例:\r\n    %s \r\n", helps[i].phelpMsg);
    return SUCCESS;
}


int assembleListString(char *buf, int buflen, ArgOptS opts[])
{
    char *p = buf;
    int offset = 0;
    int left = buflen;

    if(NULL == buf) {
        ERR("buf is NULL\n");
        snprintf(p, left, "buf is NULL\n");
        return -1;
    }

    /* 0 : ?, 1 : act */
    for(int i = 2; opts[i].argType != ArgTypesEnd; i++) {
        if(NULL == opts[i].pSetValue || ((opts[i].argType & ArgTypesListTable) == ArgTypesListTable)
           || (opts[i].argType & ArgTypesSetOnly) == ArgTypesSetOnly) {
            continue;
        }

        if((opts[i].argType & ArgTypesInt) == ArgTypesInt) {
            offset = snprintf(p, left, "%s=%d;", opts[i].pOpt, *((int *)opts[i].pSetValue));
            p += offset;
            left -= offset;
        } else if((opts[i].argType & ArgTypesString) == ArgTypesString) {
            offset = snprintf(p, left, "%s=%s;", opts[i].pOpt, (char *)opts[i].pSetValue);
            p += offset;
            left -= offset;
        } else if((opts[i].argType & ArgTypesFloat) == ArgTypesFloat) {
            offset = snprintf(p, left, "%s=%f;", opts[i].pOpt, *((float *)opts[i].pSetValue));
            p += offset;
            left -= offset;
        } else if((opts[i].argType & ArgTypesChar) == ArgTypesChar) {
            offset = snprintf(p, left, "%s=%c;", opts[i].pOpt, *((char *)opts[i].pSetValue));
            p += offset;
            left -= offset;
        }

        if (left == offset) {
            snprintf(buf, buflen, "buflen[%d] is not enough\n", buflen);
            return -1;
        }
    }

    return 0;
}

int assembleListString_ex(char *buf, int buflen, ArgOptS_Expand opts[])
{
    char *p = buf;
    int offset = 0;
    int left = buflen;

    if(NULL == buf) {
        ERR("buf is NULL\n");
        snprintf(p, left, "buf is NULL\n");
        return -1;
    }

    /* 0 : ?, 1 : act */
    for(int i = 2; opts[i].st_argopts.argType != ArgTypesEnd; i++) {
        if(NULL == opts[i].st_argopts.pSetValue || ((opts[i].st_argopts.argType & ArgTypesListTable) == ArgTypesListTable)
           || (opts[i].st_argopts.argType & ArgTypesSetOnly) == ArgTypesSetOnly) {
            continue;
        }

        if((opts[i].st_argopts.argType & ArgTypesInt) == ArgTypesInt) {
            offset = snprintf(p, left, "%s:%s=%d;", opts[i].pfOpt ,opts[i].st_argopts.pOpt, *((int *)opts[i].st_argopts.pSetValue));
            p += offset;
            left -= offset;
        } else if((opts[i].st_argopts.argType & ArgTypesString) == ArgTypesString) {
            offset = snprintf(p, left, "%s:%s=%s;", opts[i].pfOpt ,opts[i].st_argopts.pOpt, (char *)opts[i].st_argopts.pSetValue);
            p += offset;
            left -= offset;
        } else if((opts[i].st_argopts.argType & ArgTypesFloat) == ArgTypesFloat) {
            offset = snprintf(p, left, "%s:%s=%f;", opts[i].pfOpt ,opts[i].st_argopts.pOpt, *((float *)opts[i].st_argopts.pSetValue));
            p += offset;
            left -= offset;
        } else if((opts[i].st_argopts.argType & ArgTypesChar) == ArgTypesChar) {
            offset = snprintf(p, left, "%s:%s=%c;", opts[i].pfOpt ,opts[i].st_argopts.pOpt, *((char *)opts[i].st_argopts.pSetValue));
            p += offset;
            left -= offset;
        }

        if (left == offset) {
            snprintf(buf, buflen, "buflen[%d] is not enough\n", buflen);
            return -1;
        }
    }

    return 0;
}


/*
 *  必备项、只读项、参数值缺失检查。
 **/
int setmustListonlyRule(char *buf, ArgOptS opts[])
{
    /* 0 : ?  1 : act */
    for(int i=2; opts[i].argType != ArgTypesEnd; i++) {
        if(((opts[i].argType & ArgTypesSetMust) == ArgTypesSetMust) &&
           ((opts[i].argType & ArgTypesValue) != ArgTypesValue)) {
            sprintf(buf, "the param of [%s] is must param!", opts[i].pOpt);
            return FAILURE;
        }

        if(((opts[i].argType & ArgTypesListOnly) == ArgTypesListOnly) &&
           ((opts[i].argType & ArgTypesValue) == ArgTypesValue)) {
            sprintf(buf, "the param of [%s] is only read, can not be set!", opts[i].pOpt);
            return FAILURE;
        }

        if((opts[i].argType & ArgTypesArgFlag) == ArgTypesArgFlag) {
            if((opts[i].argType & ArgTypesValue) != ArgTypesValue) {
                sprintf(buf, "the param of [%s] , not value!", opts[i].pOpt);
                return FAILURE;
            }
        }
    }

    return SUCCESS;
}


/*
 * list 后不能带任何参数
 **/
int checkListRule(ArgOptS opts[])
{
    /* 0 : ?  1 : act */
    for(int i=2; opts[i].argType != ArgTypesEnd; i++) {
        if((opts[i].argType & ArgTypesArgFlag) == ArgTypesArgFlag) {
            return FAILURE;
        }
    }

    return SUCCESS;
}

int checkListRule_ex(ArgOptS_Expand opts[])
{
    /* 0 : ?  1 : act */
    for(int i=2; opts[i].st_argopts.argType != ArgTypesEnd; i++) {
        if((opts[i].st_argopts.argType & ArgTypesArgFlag) == ArgTypesArgFlag) {
            return FAILURE;
        }
    }

    return SUCCESS;
}



int jcpArgSetValue(ArgOptS *opt, char *szArgv, int gidx , void *array, int arr_size)
{
    if(NULL == szArgv) {
        return FAILURE;
    }

    switch(opt->argType & (ArgTypesString|ArgTypesChar|ArgTypesInt|ArgTypesFloat)) {
        case ArgTypesString: {
            if(jcpArgCheckRegion(opt, szArgv) != SUCCESS) {
                return FAILURE;
            }

            int datalen = strlen(szArgv);
            if (datalen > (opt->nSize)-1) {       //判断是否超过存储长度
                ERR("argv len %d > buf len %d,will cut data\n", datalen, opt->nSize-1);
            }

            strncpy((char*)((long)opt->pSetValue + (long)array + gidx*arr_size), szArgv,
                    opt->nSize);
            ((char *)((long)opt->pSetValue + (long)array + gidx*arr_size))[opt->nSize-1] = '\0';
            //sprintf((char*)((int)opt->pSetValue + (int)array + gidx*arr_size), "%s", szArgv);
            opt->argType |= ArgTypesValue;
            break;
        }

        case ArgTypesChar: {
            if(jcpArgCheckRegion(opt, szArgv) != SUCCESS) {
                return FAILURE;
            }
            *(char*)((long)opt->pSetValue + (long)array + gidx*arr_size) = szArgv[0];
            opt->argType |= ArgTypesValue;
            break;
        }

        case ArgTypesInt: {
            if(jcpArgCheckRegion(opt, szArgv) != SUCCESS) {
                return FAILURE;
            }

            *(int *)((long)array + gidx*arr_size + (long)opt->pSetValue) = atoi(szArgv);
            opt->argType |= ArgTypesValue;
            break;
        }

        case ArgTypesFloat: {
            if(jcpArgCheckRegion(opt, szArgv) != SUCCESS) {
                return FAILURE;
            }
            *(float*)((long)opt->pSetValue + (long)array + gidx*arr_size) = (float)atof(szArgv);  //偏移指针
            opt->argType |= ArgTypesValue;
            break;
        }

        default: {
            return FAILURE;
        }
    }

    return SUCCESS;
}



int jcpArgCheckRegion(ArgOptS *opt, char *pArgv)
{
    char *pVRBegin = (char*)opt->pValueRegion;
    char *pVREnd = pVRBegin;

    char szDOWN[64] = {0,}, szUP[64] = {0,};

    unsigned int u32Count = 0;
    unsigned int u32Rtn   = 0;

    if(opt->pValueRegion == NULL) {
        return SUCCESS;
    }

    switch(opt->argType & (ArgTypesString|ArgTypesChar|ArgTypesInt|ArgTypesFloat)) {
        case ArgTypesString: {
            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                if(strlen(pArgv) == u32Count) {
                    int retCmp = memcmp(pVRBegin, pArgv, u32Count);
                    if(0 == retCmp) {
                        break;
                    }
                }

                if(*pVREnd != '\0') {
                    pVREnd++;
                    pVRBegin = pVREnd;
                } else {
                    pVRBegin = NULL;
                }
            }

            if(pVRBegin == NULL) {
                return FAILURE;
            }

            break;
        }

        case ArgTypesChar: {
            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                u32Rtn = jcpArgGetDownAndUp(pVRBegin, pVREnd, szUP, szDOWN);
                if(0 == u32Rtn) { /*范围*/
                    if((jcpIsAllTheZero(szDOWN, 50) == 1) || (pArgv[0] >= szDOWN[0])) {
                        if((jcpIsAllTheZero(szUP, 50) == 1) || (pArgv[0] <= szUP[0])) {
                            break;
                        }
                    }
                } else { /*单值*/
                    if((jcpIsAllTheZero(szDOWN, 50) == 1) || (pArgv[0] == szDOWN[0])) {
                        break;
                    }
                }

                if(*pVREnd != '\0') {
                    pVREnd++;
                    pVRBegin = pVREnd;
                } else {
                    pVRBegin = NULL;
                }
            }

            if(pVRBegin == NULL) {
                return FAILURE;
            }

            break;
        }

        case ArgTypesInt: {
            char *endptr = NULL;
            int s32Vlue = strtol(pArgv, &endptr, 10);

            if (endptr && '\0' != *endptr) {
                return FAILURE;
            }

            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                u32Rtn = jcpArgGetDownAndUp(pVRBegin, pVREnd, szUP, szDOWN);
                if(0 == u32Rtn) { /*范围*/
                    if((jcpIsAllTheZero(szDOWN, 50) == 1) || (s32Vlue >= atoi(szDOWN))) {
                        if((jcpIsAllTheZero(szUP, 50) == 1) || (s32Vlue <= atoi(szUP))) {
                            break;
                        }
                    }
                } else { /*单值*/
                    if((jcpIsAllTheZero(szDOWN, 50) == 1) || (s32Vlue == atoi(szDOWN))) {
                        break;
                    }
                }

                if(*pVREnd != '\0') {
                    pVREnd++;
                    pVRBegin = pVREnd;
                } else {
                    pVRBegin = NULL;
                }
            }

            if(pVRBegin == NULL) {
                return FAILURE;
            }

            break;
        }

        case ArgTypesFloat: {
            float eVlue = (float)atof(pArgv);

            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                u32Rtn = jcpArgGetDownAndUp(pVRBegin, pVREnd, szUP, szDOWN);
                if(0 == u32Rtn) { /*范围*/
                    if((jcpIsAllTheZero(szDOWN, 50) == 1) || (eVlue >= (float)atof(szDOWN))) {
                        if((jcpIsAllTheZero(szUP, 50) == 1) || (eVlue <= (float)atof(szUP))) {
                            break;
                        }
                    }
                } else { /*单值*/
                    if((jcpIsAllTheZero(szDOWN, 50) == 1) || (eVlue == (float)atof(szDOWN))) {
                        break;
                    }
                }

                if(*pVREnd != '\0') {
                    pVREnd++;
                    pVRBegin = pVREnd;
                } else {
                    pVRBegin = NULL;
                }
            }

            if(pVRBegin == NULL) {
                return FAILURE;
            }

            break;
        }

        default:
            return FAILURE;
    }

    return SUCCESS;

}


/*将"a~z,~z,a~,-1~10,-1~,~10"等形式的范围输出,返回值是单值还是范围值*/
int jcpArgGetDownAndUp(char* pszSHead, char* pszSTail, char *szUP, char *szDOWN)
{
    char*    pszVRBegin     = pszSHead;
    char*    pszVREnd       = pszVRBegin;
    unsigned int u32Count = 0;
    unsigned char u8Region = 0;

    //memset(szDOWN, 0, 50);
    //memset(szUP, 0, 50);

    while(pszVREnd != pszSTail) {
        if(*pszVREnd != '~') {
            pszVREnd++;
            u32Count++;
        } else { /* *pszVREnd== '~' */
            u8Region = 1;
            if(*pszVRBegin != '~') { /* a~..*/
                memcpy(szDOWN, pszVRBegin, u32Count);
                szDOWN[u32Count] = '\0';
            }
            //else /* ~..*/

            u32Count = 0;
            pszVREnd++;
            pszVRBegin = pszVREnd;
        }
    }

    if(u8Region == 1) { /*范围*/
        if(pszVRBegin == pszVREnd) { /* ..~ */

        } else {
            memcpy(szUP, pszVRBegin, u32Count);
            szUP[u32Count] ='\0';
        }
        return 0;
    } else { /*单值*/
        memcpy(szDOWN, pszVRBegin, u32Count);
        szDOWN[u32Count] ='\0';
    }

    return 1;
}

int jcpIsAllTheZero(char *arr, int size)
{
    int ret = 1;
    int i = 0;

    for(; i < size; i++) {
        if(arr[i] != 0x00) {
            ret = 0;
            break;
        }
    }

    return ret;
}

int argPrintOpt(ArgOptS *pOpt, char *buf, int buflen, HelpMsgS *helps)
{
    int len = 0;
    char data[128] = {0};
    char *p = data;

    TypeNoteS typeNote[] = {
        { ArgTypesMust         , " must "   },
        { ArgTypesListOnly     , " listonly "},
        { ArgTypesSetMust      , " setmust "},
        { ArgTypesNoPara       , " no_para "},
        { ArgTypesString       , " string " },
        { ArgTypesChar         , " char "   },
        { ArgTypesInt          , " int "    },
        { ArgTypesFloat        , " float "  },
        { ArgTypesSingle       , " single " },
        { ArgTypesListTable    , " list " },
        { ArgTypesID           , " ID " },
    };

    for(int i=0; i < ((int)(sizeof(typeNote) / sizeof(typeNote[0]))); i++) {
        if((pOpt->argType & typeNote[i].type) != typeNote[i].type) {
            continue;
        }

        p += sprintf(p, "%s", typeNote[i].note);

        if((ArgTypesString == typeNote[i].type) && (NULL == pOpt->pValueRegion)) {
            len = snprintf(buf, buflen, "  -%s \r\n"
                          "    type   : %s \r\n"
                          "    Maxbyte: %d \r\n"
                          "    desc   : %s \r\n",
                          pOpt->pOpt, "string",
                          pOpt->nSize,
                          helps->phelpMsg);

            return len;
        }
    }

    WRITE_HELP_MSG(buf, buflen, len, pOpt->pOpt, data, pOpt->pValueRegion, helps->phelpMsg);

    return len;
}


ArgOptS *argFindOptStruct(ArgOptS *opts, char *str)
{
    for(int i = 0; opts[i].argType!= ArgTypesEnd; i++) {
        if(strcmp(opts[i].pOpt, str) == 0) {

            /*当解析到重复参数时，清楚上次的标记位，重新
             按照规则判断。
            */
            opts[i].argType &= ~(ArgTypesValue);

            opts[i].argType |= ArgTypesArgFlag;
            return &opts[i];
        }
    }

    return NULL;
}

ArgOptS_Expand *argFindOptStruct_ex(ArgOptS_Expand *opts, char *str)
{
    for(int i = 0; opts[i].st_argopts.argType!= ArgTypesEnd; i++) {
        if(strcmp(opts[i].st_argopts.pOpt, str) == 0) {

            /*当解析到重复参数时，清楚上次的标记位，重新
             按照规则判断。
            */
            opts[i].st_argopts.argType &= ~(ArgTypesValue);

            opts[i].st_argopts.argType |= ArgTypesArgFlag;
            return &opts[i];
        }
    }

    return NULL;
}


/*
buf: 输出的缓存
arry:多通道的结构体数组
gidx:数组序号
arr_size:结构体大小。
*/
int asmListCount(char *buf, ArgOptS opts[], void *arry, int gidx, int arr_size)
{
    char *p = buf;

    for (int i = 0; opts[i].argType != ArgTypesEnd; i++) {
        if((opts[i].argType & ArgTypesListTable) != ArgTypesListTable) {
            continue;
        }

        if((opts[i].argType & ArgTypesInt) == ArgTypesInt) {
            p += sprintf(p, "%s=%d;", opts[i].pOpt, *((int *)((long)arry + gidx*arr_size + (long)opts[i].pSetValue)));
        } else if((opts[i].argType & ArgTypesString) == ArgTypesString) {
            p += sprintf(p, "%s=%s;", opts[i].pOpt, (char *)((long)arry + gidx*arr_size + (long)opts[i].pSetValue));
        } else if((opts[i].argType & ArgTypesFloat) == ArgTypesFloat) {
            p += sprintf(p, "%s=%f;", opts[i].pOpt, *((float *)((long)arry + gidx*arr_size + (long)opts[i].pSetValue)));
        } else if((opts[i].argType & ArgTypesChar) == ArgTypesChar) {
            p += sprintf(p, "%s=%c;", opts[i].pOpt, *((char *)((long)arry + gidx*arr_size + (long)opts[i].pSetValue)));
        }

    }
    strcat(p, "#");
    return strlen(buf);
}



