/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : utf82gbk.h
 * @Created Time : 2014-09-11
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  : 
 */

#ifndef __UTF82GBK_H__
#define __UTF82GBK_H__

#ifdef __cplusplus
extern "C" {
#endif

int gbk2utf8(const unsigned char *pgbk, int inlen, unsigned char *putf8, int outlen);

int utf82gbk(unsigned char *in, unsigned char *out, int outlen);

int gbk2unicode(const unsigned short *pgbk, unsigned short *punicode);

#ifdef __cplusplus
}
#endif 

#endif

