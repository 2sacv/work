/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_IntelligentAnalysis.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2018-04-30 by cuiweixun
    Description  :
    History      :
******************************************************************************/


#ifndef __INTELLIGENTANALYISI_H__
#define __INTELLIGENTANALYISI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"
#include "encode_common.h"
//#include "mi_common.h"
//#include "mi_md.h"

typedef struct venc_IntelligentAnalysis_t
{
    pthread_mutex_t     	MdMutex;
    pthread_t           	pthreadIntelligentAnalysis;

}IntelligentAnalysis_t;
int encode_IntelligentAnalysis_init(void);
int encode_IntelligentAnalysis_uninit(void);


typedef struct venc_AutoLen3x_t
{
    pthread_mutex_t     	MdMutex;
    pthread_t           	pthreadAutoLen3x;

}AutoLen3x_t;
int encode_AutoLen3x_init(void);
int encode_AutoLen3x_uninit(void);


#ifdef __cplusplus
}
#endif

#endif//__ENCODE_MOTION_H__

