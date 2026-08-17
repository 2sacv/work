/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-10-27
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JS_BASE64_H_
#define _JS_BASE64_H_

#ifdef __cplusplus
extern "C" {
#endif

int js_b64_encode(unsigned char *data,int dataLen,unsigned char **encData,int *encDataLen);
int js_b64_decode(unsigned char *data,int dataLen, unsigned char **decData,int *decDataLen);
void js_b64_free(unsigned char *data);

#ifdef __cplusplus
}
#endif

#endif

