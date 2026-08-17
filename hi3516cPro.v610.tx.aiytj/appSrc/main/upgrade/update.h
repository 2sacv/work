/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name   : jco_update.h
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2008.11.20
    Description : update functions
    History     :
                    Create by wgy.2008.11.20
******************************************************************************/
#ifndef __UPDATE_H__
#define __UPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SYSTEM_APP_PATH         "/ipc/app/"
#define UPDATE_TMP_FILE         "/tmp/upgrade.tgz"
#define DM_FILE_PATH            "/tmp/temp"
#define DM_TMP_PATH             "/tmp/"

    /*
     * 00:00:49 | [upgrade/update_service.cpp: 84] Accept connection
     * 00:01:00 | [upgrade/update_service.cpp: 219] readBytes :0
     * 00:01:00 | [upgrade/jco_update.c: 177] do_md5sum
     * 00:01:02 | [upgrade/jco_update.c: 230] Prepare handing over to updateExt.sh
     * 00:02:02 | [upgrade/jco_update.c: 234] ------ update success ------ !!!
     */

    enum UPDATE_STATUS {
        UPDATE_BEGIN        = 3,
        UPDATE_RECV_HALF    = 8,
        UPDATE_DO_AUTHEN    = 11,
        UPDATE_DO_MD5SUM    = 13,
        UPDATE_EXTR_ENTRY   = 14,
        UPDATE_DO_BASH      = 15,       // 1/7
        UPDATE_SUCCESS      = 100,      //
        UPDATE_ERR_AUTHEN   = 101,      // encrypt
        UPDATE_ERR_MD5SUM   = 102,      // md5sum
        UPDATE_ERR_ENTRY    = 103,      // no updateExt.sh
        UPDATE_ERR_UNPACK   = 104,
        UPDATE_ERR_NON_EXIST = 105,     // pack non exitst
    };


    typedef enum {
        FIRMWARE_FILE = 0,
        MCU_FILE,
        MCU_FILE20,
        MCU_FRONT,
        MCU_CAM
    } UPDATE_FILE_TYPE;

    /*====================================================================
     discrib: update the specific file type
     param:
        int JCOUpdateBegin(     -OUT SUCCESS/FAILURE
            UPDATE_CMD_E cmd)   -IN UPDATE_CMD_E
    =====================================================================*/
    int JCOUpdateBegin();

    int JCOUpdateJoin();

    int JCOMd5sum(const char *szPath, unsigned char *md5);

    void init_server_upgrade(void *data);

    void restart_server_upgrade();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __JCO_UPDATE_H__

