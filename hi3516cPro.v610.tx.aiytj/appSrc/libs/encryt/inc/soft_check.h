/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2015-06-05
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _SOFTCHECH_H_
#define _SOFTCHECH_H_
#ifdef __cplusplus
extern "C" {
#endif

int check_device_info_isok(const char *uid, const char *mac, const char *devid, const char *devinfo);
int get_device_info_new(const char *uid, const char *newmac, const char *newid, char *buf, int bufsize);

#ifdef __cplusplus
}
#endif
#endif

