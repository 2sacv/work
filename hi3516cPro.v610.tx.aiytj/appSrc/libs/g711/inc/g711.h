/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : g711.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2009-07-13
    Description  : 
    History      : 
                        created by lsf. 2009-07-13
******************************************************************************/


#if !defined(_G711_H_)
#define _G711_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned char jco_linear2alaw(short pcm_val);
short jco_alaw2linear(unsigned char	a_val);

unsigned char jco_linear2ulaw(short	pcm_val);
short jco_ulaw2linear(unsigned char	u_val);

unsigned char jco_alaw2ulaw(unsigned char aval);
unsigned char jco_ulaw2alaw(unsigned char uval);

#ifdef __cplusplus
}
#endif

#endif
