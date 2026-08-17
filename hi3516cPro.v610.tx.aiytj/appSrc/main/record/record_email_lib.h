/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : recordEmail.h
 * @Created Time : 2014-03-05
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _RECORD_EMAIL_LIB_H_
#define _RECORD_EMAIL_LIB_H_
#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        char    user[128];
        char    passwd[128];
        char    dstUser[128];
        char    svrAddr[128];
        char    alarmType[64];
    } MailInfoS;

    enum {
        MAIL_SUCC = 0,
        MAIL_FAIL_UNKNOW = 1,
        MAIL_FAIL_SVR = 2,
        MAIL_FAIL_SSL = 3,
        MAIL_FAIL_USER_PASSWD = 4,
        MAIL_FAIL_ANTI_SPAM = 5,
    };

    void mailSSLInit();

    int mailUpData(char *filePath, char *buf, int size, char *filename, MailInfoS mus, int timeout);

#ifdef __cplusplus
}
#endif
#endif

