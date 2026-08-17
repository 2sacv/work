/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : confutils.h
 * @Created Time : 2013-11-08
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _CONFUTILS_H_
#define _CONFUTILS_H_
#ifdef  __cplusplus
extern "C" {
#endif

#include "conftypedef.h"

    int ipMaskGatewayCheck(char *ethip, char *ethmask, char *ethgw);

    int ipMaskGatewayRuleCheck(char *ethip, char *ethmask, char *ethgw);

    int processEthMac(ConfAct act, const char *pNic, char *szMac);

#ifdef __cplusplus
}
#endif
#endif

