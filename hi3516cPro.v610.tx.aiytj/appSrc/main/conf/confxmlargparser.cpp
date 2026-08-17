/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : confutil.cpp
 * @Created Time : 2013-10-24
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "debug.h"
#include "utf82gbk.h"

#include "confxmlargparser.h"

static int    argValueCheck_t(ArgOptS_T *opt);
static int    argValueCheck(ArgOpt *opt);
static int    argValueCheckRegion_t(ArgOptS_T *opt);
static int    argValueCheckRegion(ArgOpt *opt);
static int    checkValueRegion(ArgTypeE u32Type, char *VRBegin, void *pArgv);

static void   processSpaceXmlNode(char *dst, mxml_node_t *node);
static int    argGetDownAndUp(char* pszSHead, char* pszSTail, char *szUP, char *szDOWN);
static int    xmlParserGetInt(mxml_node_t *node, int notfound);
static double xmlParserGetDouble(mxml_node_t *node, double notfound);
static char   *xmlGetNodeString(mxml_node_t *node);
static void   xmlParserGetString(mxml_node_t *node, char* def, MapOptKey *map);
static void   xmlParserGetString_t(mxml_node_t *node, char* def, ArgOptS_T *opts);

static void   iterationAllChildNext(mxml_node_t *child);
static int    xmlParserSetInt(mxml_node_t *node, int val);
static int    xmlParserSetDouble(mxml_node_t *node, double val);
static int    xmlParserSetString(mxml_node_t *node, char *val);
static int    isAllTheZero(char *arr, int size);
static int    confAccessRIteration(ConfAct act, mxml_node_t *curNode, mxml_node_t *top,
                                   ArgOpt opts[], MapOptKey maps[]);

static int    confAccessRIteration_t(ConfAct act, mxml_node_t *curNode, mxml_node_t *top,
                                     ArgOptS_T opts[]);

int confParserStructValue_t(ArgOptS_T *opts)
{
    int ret = -1;

    for(int i = 0; opts[i].u32Type != ArgTypeEnd; i++) {
        if((opts[i].u32Type & ArgTypeTree) == ArgTypeTree) {
            if(NULL == opts[i].pValue) {
                ERR("The type of [ArgTypeTree]'s [pValue] is NULL!\n");
                return FAILURE;
            }

            if(NULL != opts[i].pCb) {
                if((opts[i].pCb)(i, opts[i].pCbArg) != SUCCESS) {
                    ERR("opts[i].pCbArg failed!\n");
                    continue;
                }
            }

            if(confParserStructValue_t((ArgOptS_T*)opts[i].pValue) != SUCCESS) {
                ERR("node <%s> errno\n", opts[i].pOpt);
                return FAILURE;
            }

            continue;
        }

        ret = argValueCheck_t(&opts[i]);
        if(ret != SUCCESS) {
            ERR("set value <%s> errno! VauleRegion is [%s]\n",
                opts[i].pOpt, opts[i].pVauleRegion);
            return ret;
        }
    }

    return SUCCESS;
}

int confParserStructValue(ArgOpt *opts)
{
    int ret = -1;

    for(int i = 0; opts[i].u32Type != ArgTypeEnd; i++) {
        if((opts[i].u32Type & ArgTypeTree) == ArgTypeTree) {
            if(NULL == opts[i].pValue) {
                ERR("The type of [ArgTypeTree]'s [pValue] is NULL!\n");
                return FAILURE;
            }

            if(NULL != opts[i].pCb) {
                if((opts[i].pCb)(i, opts[i].pCbArg) != SUCCESS) {
                    ERR("opts[i].pCbArg failed!\n");
                    continue;
                }
            }

            if(confParserStructValue((ArgOpt*)opts[i].pValue) != SUCCESS) {
                ERR("node <%s> errno\n", opts[i].pOpt);
                return FAILURE;
            }

            continue;
        }

        ret = argValueCheck(&opts[i]);
        if(ret != SUCCESS) {
            ERR("set value <%s> errno! VauleRegion is [%s]\n",
                opts[i].pOpt, opts[i].pVauleRegion);
            return ret;
        }
    }

    return SUCCESS;
}

int confAccessRoot_t(ConfAct act, mxml_node_t *root, const char *parent,
                     ArgOptS_T opts[])
{
    if(NULL == root) {
        ERR("Root node is NULL!\n");
        return FAILURE;
    }

    if(opts == NULL) {
        ERR("You must input [opts] for type check!\n");
        return FAILURE;
    }

    mxml_node_t *curnode = root;
    mxml_node_t *pNode = NULL;
    do {        //防止重名
        pNode = mxmlFindElement(curnode, root, parent, NULL, NULL, MXML_DESCEND);
        if(pNode == NULL)  {
            ERR("Can't find [%s] node!\n", parent);
            return FAILURE;
        } else if (pNode->parent->parent != root) {
            if (strncasecmp(pNode->parent->value.element.name, "platforms",strlen("platforms"))) {
                curnode = pNode;
                continue;
            }
        }
        break;
    } while(1);

    if(SUCCESS != confAccessRIteration_t(act, pNode, pNode, opts)) {
        ERR("set <%s> errno\n", parent);
        return FAILURE;
    }

    return SUCCESS;
}


int confAccessRoot(ConfAct act, mxml_node_t *root, const char *parent,
                   ArgOpt opts[], MapOptKey maps[])
{
    if(NULL == root) {
        ERR("Root node is NULL!\n");
        return FAILURE;
    }

    if(opts == NULL) {
        ERR("You must input [opts] for type check!\n");
        return FAILURE;
    }

    mxml_node_t *pNode = mxmlFindElement(root, root, parent, NULL, NULL, MXML_DESCEND);
    if(pNode == NULL) {
        ERR("Can't find [%s] node!\n", parent);
        return FAILURE;
    }

    if(SUCCESS != confAccessRIteration(act, pNode, pNode, opts, maps)) {
        return FAILURE;
    }

    return SUCCESS;
}

int confAccessRIteration_t(ConfAct act, mxml_node_t *curNode, mxml_node_t *top,
                           ArgOptS_T opts[])
{
    mxml_node_t *node = NULL;

    if(ConfGet == act) {
        for(int i = 0; opts[i].u32Type!= ArgTypeEnd; i++) {

            if(NULL == opts[i].pValue)
                continue;         //the one didn't store in the config file

            node = mxmlFindElement(curNode, top, (char*)opts[i].pOpt, NULL, NULL, MXML_DESCEND);

            if((opts[i].u32Type& ArgTypeTree) == ArgTypeTree) {
                if(SUCCESS != confAccessRIteration_t(act, node, curNode, (ArgOptS_T*)opts[i].pValue)) {
                    ERR("<%s> node is errno\n", opts[i].pOpt);
                    return FAILURE;
                }

                if(NULL != opts[i].pCb) {
                    (opts[i].pCb)(i, opts[i].pCbArg);
                    curNode = node;   //为了处理重复ArgTypeTree节点
                }

                continue;
            }

            if((opts[i].u32Type& ArgTypeInt) == ArgTypeInt) {
                *(int*)opts[i].pValue=
                    xmlParserGetInt(node, (int)opts[i].def);
            } else if((opts[i].u32Type& ArgTypeFloat) == ArgTypeFloat) {
                *(float *)opts[i].pValue=
                    (float)xmlParserGetDouble(node, 0);
            } else if((opts[i].u32Type& ArgTypeString) == ArgTypeString) {
                xmlParserGetString_t(node, (char*)opts[i].def, &opts[i]);
            }
        }
    } else if(ConfSet == act) {
        for(int i = 0; opts[i].u32Type!= ArgTypeEnd; i++) {
            if((NULL == opts[i].pValue) || (ArgTypeReadOnly == (opts[i].u32Type & ArgTypeReadOnly)))
                continue;          //the one didn't store in the config file/readonly

            node = mxmlFindElement(curNode, top, (char*)opts[i].pOpt, NULL, NULL, MXML_DESCEND);

            if((opts[i].u32Type& ArgTypeTree) == ArgTypeTree) {
                if(opts[i].pCb != NULL) {
                    (opts[i].pCb)(i, opts[i].pCbArg);
                }

                if(SUCCESS != confAccessRIteration_t(act, node, curNode, (ArgOptS_T*)opts[i].pValue)) {
                    ERR("<%s> node is errno\n", opts[i].pOpt);
                    return FAILURE;
                }

                if(opts[i].pCb != NULL) {
                    curNode = node;
                }

                continue;
            }

            if((opts[i].u32Type& ArgTypeInt) == ArgTypeInt) {
                xmlParserSetInt(node, *(int*)opts[i].pValue);
            } else if((opts[i].u32Type & ArgTypeFloat) == ArgTypeFloat) {
                xmlParserSetDouble(node, *(float*)opts[i].pValue);
            } else if((opts[i].u32Type & ArgTypeString) == ArgTypeString) {
                xmlParserSetString(node, (char *)opts[i].pValue);
            }
        }
    }

    return SUCCESS;
}



int confAccessRIteration(ConfAct act, mxml_node_t *curNode, mxml_node_t *top,
                         ArgOpt opts[], MapOptKey maps[])
{
    mxml_node_t *node = NULL;

    if(ConfGet == act) {
        for(int i = 0; maps[i].type != ArgTypeEnd; i++) {
            if((maps[i].type != opts[i].u32Type) || (maps[i].pOpt != opts[i].pOpt)) {
                ERR("[%d] of maps and opts not equal[%s/%s]\n", i, maps[i].pOpt, opts[i].pOpt);
                return FAILURE;
            }

            if(NULL == maps[i].arg)
                continue;         //the one didn't store in the config file

            node = mxmlFindElement(curNode, top, (char*)maps[i].pOpt, NULL, NULL, MXML_DESCEND);

            if((maps[i].type & ArgTypeTree) == ArgTypeTree) {
                if(SUCCESS != confAccessRIteration(act, node, curNode, (ArgOpt*)opts[i].pValue, (MapOptKey*)maps[i].arg)) {
                    ERR("<%s> node is errno\n", maps[i].pOpt);
                    return FAILURE;
                }

                if(NULL != maps[i].pCb) {
                    (maps[i].pCb)(i, maps[i].pCbArg);
                    curNode = node;   //为了处理重复ArgTypeTree节点
                }

                continue;
            }

            if((maps[i].type & ArgTypeInt) == ArgTypeInt) {
                *(int*)maps[i].arg =
                    xmlParserGetInt(node, (int)maps[i].deft);
            } else if((maps[i].type & ArgTypeFloat) == ArgTypeFloat) {
                *(float *)maps[i].arg =
                    (float)xmlParserGetDouble(node, 0);
            } else if((maps[i].type & ArgTypeString) == ArgTypeString) {
                xmlParserGetString(node, (char*)maps[i].deft, &maps[i]);
            }
        }
    } else if(ConfSet == act) {
        for(int i = 0; maps[i].type != ArgTypeEnd; i++) {
            if((maps[i].type != opts[i].u32Type) || (maps[i].pOpt != opts[i].pOpt)) {
                ERR("[%d] of maps and opts not equal\n", i);
                return FAILURE;
            }

            if((NULL == maps[i].arg) || (ArgTypeReadOnly == (maps[i].type & ArgTypeReadOnly)))
                continue;          //the one didn't store in the config file/readonly

            node = mxmlFindElement(curNode, top, (char*)maps[i].pOpt, NULL, NULL, MXML_DESCEND);

            if((maps[i].type & ArgTypeTree) == ArgTypeTree) {
                if(maps[i].pCb != NULL) {
                    (maps[i].pCb)(i, maps[i].pCbArg);
                }

                if(SUCCESS != confAccessRIteration(act, node, curNode, (ArgOpt*)opts[i].pValue, (MapOptKey*)maps[i].arg)) {
                    ERR("<%s> node is errno\n", maps[i].pOpt);
                    return FAILURE;
                }

                if(maps[i].pCb != NULL) {
                    curNode = node;
                }

                continue;
            }

            if((maps[i].type & ArgTypeInt) == ArgTypeInt) {
                xmlParserSetInt(node, *(int*)maps[i].arg);
            } else if((maps[i].type & ArgTypeFloat) == ArgTypeFloat) {
                xmlParserSetDouble(node, *(float*)maps[i].arg);
            } else if((maps[i].type & ArgTypeString) == ArgTypeString) {
                xmlParserSetString(node, (char *)maps[i].arg);
            }
        }
    }

    return SUCCESS;
}

int xmlParserGetInt(mxml_node_t *node, int notfound)
{
    char *str = xmlGetNodeString(node);

    if(NULL == str)
        return notfound;

    return atoi(str);
}

double xmlParserGetDouble(mxml_node_t *node, double notfound)
{
    char *str = xmlGetNodeString(node);

    if(NULL == str)
        return notfound;

    return atof(str);
}

void xmlParserGetString_t(mxml_node_t *node, char* def, ArgOptS_T *opts)
{
    if(NULL == node || NULL == node->child) {
        def != NULL ? strncpy((char *)opts->pValue, def, opts->szMbr- 1) : strncpy((char *)opts->pValue, "", opts->szMbr- 1);
        return ;
    }

    char *dst = (char *)malloc(4096);

    processSpaceXmlNode(dst, node);

    strncpy((char *)opts->pValue, dst, opts->szMbr - 1);

    if (dst)
		free(dst);
}


void xmlParserGetString(mxml_node_t *node, char* def, MapOptKey *map)
{
    if(NULL == node || NULL == node->child) {
        def != NULL ? strncpy((char *)map->arg, def, map->iSize - 1) : strncpy((char *)map->arg, "", map->iSize - 1);
        return ;
    }

    char *dst = (char *)malloc(4096);

    processSpaceXmlNode(dst, node);

    strncpy((char *)map->arg, dst, map->iSize - 1);

    if (dst)
		free(dst);
}




char *xmlGetNodeString(mxml_node_t *node)
{
    if(NULL == node || NULL == node->child)
        return NULL;

    return node->child->value.text.string;
}

int xmlParserSetInt(mxml_node_t *node, int val)
{
    if(NULL == node) {
        return FAILURE;
    }

    //mxmlDelete(dst);

    //dst = mxmlNewElement(pNode, keynode);
    //mxmlNewTextf(dst, 0, "%d", val);

    if(NULL != node->child)
        mxmlSetTextf(node->child, 0, "%d", val);
    else
        mxmlNewTextf(node, 0, "%d", val);

    return SUCCESS;
}

int xmlParserSetDouble(mxml_node_t *node, double val)
{
    if(NULL == node) {
        return FAILURE;
    }

    //mxmlDelete(dst);

    //dst = mxmlNewElement(pNode, keynode);
    //mxmlNewTextf(dst, 0, "%f", val);

    if(NULL != node->child)
        mxmlSetTextf(node->child, 0, "%f", val);
    else
        mxmlNewTextf(node, 0, "%f", val);

    return SUCCESS;
}

int xmlParserSetString(mxml_node_t *node, char *val)
{
    if(NULL == node) {
        return FAILURE;
    }

    //mxmlDelete(dst);

    //dst = mxmlNewElement(pNode, keynode);
    //mxmlNewTextf(dst, 0, "%s", val);

	int encoding = 0; //ENCODE_UTF8
	char *p = val;
	int utf8falg = 1;
	
	for(; p < val + strlen(val); ) {
		if(mxml_string_getc(&p, &encoding) < 0) {
			if(p == val + strlen(val))
				utf8falg = 1;
			else
				utf8falg = 0;
			break;
		}
	}
	//DBG("utf8falg : %d\n", utf8falg);

	char szutf8[1024] = {0};
	
	if(0 == utf8falg) {
		int ret = gbk2utf8((const unsigned char*)val, strlen(val), (unsigned char*)szutf8, sizeof(szutf8) -1);
		if(ret < 0) {
			ERR("gbk2utf8 failed!\n");
			return FAILURE;
		}
        snprintf(val, strlen(szutf8) + 1, "%s", szutf8);    // 非 utf-8 字符转换后更新到 outer 以便使用
	} else {
		snprintf(szutf8, sizeof(szutf8) -1, "%s", val);
	}

    if(NULL != node->child) {
        iterationAllChildNext(node->child);      //delete child's all next, reserve child

        mxmlSetTextf(node->child, 0, "%s", szutf8); //'val' with whitespace also all the
        //data in dst->child->value.text.string
        //when Reloading will become multi 'next' node
    } else {
        mxmlNewTextf(node, 0, "%s", szutf8);
    }

    return SUCCESS;
}

int argValueCheck_t(ArgOptS_T *opt)
{
    if(NULL == opt->pValue) {
        if(NULL != opt->pVauleRegion) {
            ERR("opt->pValue is NULL!\n");
            return FAILURE;
        }

        return SUCCESS;
    }

    if(argValueCheckRegion_t(opt) != SUCCESS) {
        ERR("Arg [%s] Region check failed!\n", opt->pOpt);
        return FAILURE;
    }

    return SUCCESS;
}

int argValueCheck(ArgOpt *opt)
{
    if(NULL == opt->pValue) {
        if(NULL != opt->pVauleRegion) {
            ERR("opt->pValue is NULL!\n");
            return FAILURE;
        }

        return SUCCESS;
    }

    if(argValueCheckRegion(opt) != SUCCESS) {
        ERR("Arg [%s] Region check failed!\n", opt->pOpt);
        return FAILURE;
    }

    return SUCCESS;
}

int argValueCheckRegion_t(ArgOptS_T *opt)
{
    return checkValueRegion(opt->u32Type, (char *)opt->pVauleRegion, opt->pValue);
}

int argValueCheckRegion(ArgOpt *opt)
{
    return checkValueRegion(opt->u32Type, (char *)opt->pVauleRegion, opt->pValue);
}

int checkValueRegion(ArgTypeE u32Type, char *VRBegin, void *pArgv)
{
    char *pVRBegin = VRBegin;
    char *pVREnd = pVRBegin;

    char szDOWN[64] = {0,}, szUP[64] = {0,};

    unsigned int u32Count = 0;
    unsigned int u32Rtn   = 0;

    if(VRBegin == NULL) {
        return SUCCESS;
    }

    switch(u32Type & (ArgTypeString|ArgTypeChar|ArgTypeInt|ArgTypeFloat)) {
        case ArgTypeString: {
            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                if(strlen((const char*)pArgv) == u32Count) {
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

        case ArgTypeChar: {
            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                u32Rtn = argGetDownAndUp(pVRBegin, pVREnd, szUP, szDOWN);
                if(0 == u32Rtn) { /*范围*/
                    if((isAllTheZero(szDOWN, 50) == 1) || (((char*)pArgv)[0] >= szDOWN[0])) {
                        if((isAllTheZero(szUP, 50) == 1) || (((char*)pArgv)[0] <= szUP[0])) {
                            break;
                        }
                    }
                } else { /*单值*/
                    if((isAllTheZero(szDOWN, 50) == 1) || (((char*)pArgv)[0] == szDOWN[0])) {
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

        case ArgTypeInt: {
            char *endptr = NULL;
            int s32Vlue = *((int*)pArgv);

            if (endptr && '\0' != *endptr) {
                return FAILURE;
            }

            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                u32Rtn = argGetDownAndUp(pVRBegin, pVREnd, szUP, szDOWN);
                if(0 == u32Rtn) { /*范围*/
                    if((isAllTheZero(szDOWN, 50) == 1) || (s32Vlue >= atoi(szDOWN))) {
                        if((isAllTheZero(szUP, 50) == 1) || (s32Vlue <= atoi(szUP))) {
                            break;
                        }
                    }
                } else { /*单值*/
                    if((isAllTheZero(szDOWN, 50) == 1) || (s32Vlue == atoi(szDOWN))) {
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
                ERR("set valure = %d\n", s32Vlue);
                return FAILURE;
            }

            break;
        }

        case ArgTypeFloat: {
            float eVlue = *((float*)pArgv);

            while(pVRBegin != NULL) {
                u32Count = 0;

                while((*pVREnd != '|') && (*pVREnd != '\0')) {
                    pVREnd++;
                    u32Count++;
                }

                u32Rtn = argGetDownAndUp(pVRBegin, pVREnd, szUP, szDOWN);
                if(0 == u32Rtn) { /*范围*/
                    if((isAllTheZero(szDOWN, 50) == 1) || (eVlue >= (float)atof(szDOWN))) {
                        if((isAllTheZero(szUP, 50) == 1) || (eVlue <= (float)atof(szUP))) {
                            break;
                        }
                    }
                } else { /*单值*/
                    if((isAllTheZero(szDOWN, 50) == 1) || (eVlue == (float)atof(szDOWN))) {
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

void iterationAllChildNext(mxml_node_t *child)
{
    if(child->next) {
        iterationAllChildNext(child->next);
    } else {
        return;
    }

    mxmlDelete(child->next);
}

void processSpaceXmlNode(char *dst, mxml_node_t *node)
{
    int i = 0;
    char *p = dst;
    mxml_node_t *pNode = node->child;

    while(pNode) {
        if(i)
            p += sprintf(p, " ");
        p += sprintf(p, "%s", pNode->value.text.string);
        pNode = pNode->next;
        i++;
    }  //process text with space
}

/*将"a~z,~z,a~,-1~10,-1~,~10"等形式的范围输出,返回值是单值还是范围值*/
int argGetDownAndUp(char* pszSHead, char* pszSTail, char *szUP, char *szDOWN)
{
    char*    pszVRBegin     = pszSHead;
    char*    pszVREnd       = pszVRBegin;
    unsigned int u32Count = 0;
    unsigned char u8Region = 0;

    //memset(szDOWN,0,50);
    //memset(szUP,0,50);

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

int isAllTheZero(char *arr, int size)
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
