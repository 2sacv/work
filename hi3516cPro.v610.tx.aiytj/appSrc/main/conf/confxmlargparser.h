/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : confxmlargparser.h
 * @Created Time : 2013-10-24
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _CONFXMLARGPARSER_H_
#define _CONFXMLARGPARSER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mxml.h"

#include "conftypedef.h"

    int confParserStructValue_t(ArgOptS_T *opts);

    int confParserStructValue(ArgOpt *opts);

    int confAccessRoot(ConfAct act, mxml_node_t *root, const char *parent,
                       ArgOpt opts[], MapOptKey maps[]);

    int confAccessRoot_t(ConfAct act, mxml_node_t *root, const char *parent,
                         ArgOptS_T opts[]);

    int ali_conf_access_root_t( mxml_node_t *root, const char *strstr, const char *str, ArgOptS_T *opts);

#ifdef __cplusplus
}
#endif
#endif

