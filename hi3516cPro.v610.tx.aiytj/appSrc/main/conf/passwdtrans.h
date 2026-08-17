/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : passwdtrans.h
 * @Created Time : 2014-04-23
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _PASSWD_TRANS_H_
#define _PASSWD_TRANS_H_
#ifdef __cplusplus
extern "C" {
#endif

int passwd_trans_encode(char *dst, char *src, int len);

int passwd_trans_decode(char *dst, char *src, int len);

#ifdef __cplusplus
}
#endif
#endif

