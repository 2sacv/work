/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    :
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2016-04-20
    Description  :
    History      :
                        created by lsf.
******************************************************************************/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "usr_super.h"
#include "our_md5.h"

int get_usr_super(char *devid, char *buf, int buflen)
{
    time_t now;
    struct tm tm;

    char message[128] = {0,};
    MD5_CTX_OUR md5CtxA1;
    char ha1[20] = {0};

    if(devid == NULL)
        return -1;

    if(buf == NULL || buflen < 33)
        return -1;

    now = time(NULL);
    localtime_r(&now, &tm);

    snprintf(message, sizeof(message)-1, "dev:%s-%4d%2d%d[%2d]", devid,
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, (tm.tm_hour + 2)/3);

    our_MD5Init(&md5CtxA1);
    ourMD5Update(&md5CtxA1, (unsigned char *)message, strlen(message));
    our_MD5Final((unsigned char*)ha1, &md5CtxA1);

    unsigned short i;
    unsigned char j;
    for (i = 0; i < 7; i++) {
        j = (ha1[i+3] >> 4) & 0xf;
        if (j <= 9)
            buf[i * 2] = (j + '0');
        else
            buf[i * 2] = (j + 'a' - 10);

        j = ha1[i+3] & 0xf;
        if (j <= 9)
            buf[i * 2 + 1] = (j + '0');
        else
            buf[i * 2 + 1] = (j + 'a' - 10);
    };

    buf[2*(i+1)] = '\0';

    return 0;
}

int do_verifystr(char *verifystr, char *buf, int buflen)
{
    time_t now;
    struct tm tm;

    char message[128] = {0,};
    MD5_CTX_OUR md5CtxA1;
    char ha1[20] = {0};

    if(verifystr == NULL)
        return -1;

    if(buf == NULL || buflen < 33)
        return -1;

    now = time(NULL);
    localtime_r(&now, &tm);

    snprintf(message, sizeof(message)-1, "f!a@k#e:%s", verifystr);

    our_MD5Init(&md5CtxA1);
    ourMD5Update(&md5CtxA1, (unsigned char *)message, strlen(message));
    our_MD5Final((unsigned char*)ha1, &md5CtxA1);

    unsigned short i;
    unsigned char j;
    for (i = 0; i < 7; i++) {
        j = (ha1[i+3] >> 4) & 0xf;
        if (j <= 9)
            buf[i * 2] = (j + '0');
        else
            buf[i * 2] = (j + 'a' - 10);

        j = ha1[i+3] & 0xf;
        if (j <= 9)
            buf[i * 2 + 1] = (j + '0');
        else
            buf[i * 2 + 1] = (j + 'a' - 10);
    };

    buf[2*(i+1)] = '\0';

    return 0;
}

int is_usr_super(char *devid, char *user, char *passwd)
{
    char superpwd[36] = {0,};

    if(devid == NULL || user == NULL || passwd == NULL)
        return -1;

    get_usr_super(devid, superpwd, sizeof(superpwd));

    if(strcmp(superpwd, passwd) == 0)
        return 0;

    return -1;
}


