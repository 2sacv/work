/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpCmdImplement.h
 * @Created Time : 2013-12-25
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _JCPCMDIMPLEMENT_H_
#define _JCPCMDIMPLEMENT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"

#define CUSTOM_CONF   "/opt/conf/custom_conf"

    int alarm_expand_jcpcmd(char *buf, int buflen, int argc, char **argv);

    int JCPCmdCA2AlarmHostCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdCA2MDCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdTestAlarm(char *buf, int buflen, int argc, char **argv);

    int JCPCmdSearchFileCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdIRCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdEthCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdUPNPCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdPortCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdOSDTxtCfg(char *buf, int buflen, int argc, char **argv);
    
    int JCPCmdDevRcrdCfg(char *buf, int buflen, int argc, char **argv);
    
    int JCPCmdDevRcrdCfg(char *buf, int buflen, int argc, char **argv);

    int record_manual_jcpcmd(char *buf, int buflen, int argc, char **argv);

    int JCPCmdEmailCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdSysCtrl(char *buf, int buflen, int argc, char **argv);

    int JCPCmdVersion(char *buf, int buflen, int argc, char **argv);

    int JCPCmdNtpCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdTimeCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdUserPasswdCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdAeAwbBlcCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdCA2Ipconflict(char *buf, int buflen, int argc, char **argv);

    int JCPCmdCA2Linkbroken(char *buf, int buflen, int argc, char **argv);

    int JCPCmdOSDCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdRoiCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdGetLog(char *buf, int buflen, int argc, char **argv);

    int JCPCmdUpdate(char *buf, int buflen, int argc, char **argv);

    int JCPCmdVeprofileCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdViCfg(char *buf, int buflen, int argc, char **argv);

    int JCPCmdXkcd(char *buf, int buflen, int argc, char **argv);

    int JCPCmdAlarmManageCfg(char *buf, int buflen, int argc, char **argv);


#ifdef __cplusplus
}
#endif
#endif

