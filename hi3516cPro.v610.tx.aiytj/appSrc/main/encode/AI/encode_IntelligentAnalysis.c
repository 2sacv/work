/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_IntelligentAnalysis.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2018-04-30 by cuiweixun
    Description  :
    History      :
******************************************************************************/
#include "encode_IntelligentAnalysis.h"
#include "utils.h"
#include "debug.h"

extern  int encode_AutoLen_watch(void);

extern  void millisecond_nanosleep(long millisecond);
extern  unsigned short AutoFocus_Get_SysTimer(void);


//#define AutoLen3x_Time
static AutoLen3x_t gtVencAutoLen;
static void *encode_AutoLen3x_process(HANDLE hInstance)
{
    #ifdef AutoLen3x_Time
    double T0,T1;
    #endif

    DBG("Now encode_AutoLen3x_process is start \n");
    while(encode_thread_get_run())
    {
        #ifdef AutoLen3x_Time
        T0=get_usec_of_day();
        #endif
        millisecond_nanosleep(AutoFocus_Get_SysTimer());
        encode_AutoLen_watch();
        #ifdef AutoLen3x_Time
        T1=get_usec_of_day();
        fprintf(stderr,"\033[1;31m""T=%.1f T0=%d \n""\033[0000m",(T1-T0)/1000,AutoFocus_Get_SysTimer());
        #endif
    }
    return NULL;
}

int encode_AutoLen3x_init(void)
{
    int ret = S_OK;

    memset(&gtVencAutoLen, 0, sizeof(AutoLen3x_t));
    pthread_mutex_init(&gtVencAutoLen.MdMutex, NULL);


    if ((gtVencAutoLen.pthreadAutoLen3x = create_pthread("AutoLen3x",encode_AutoLen3x_process,NULL,NULL)) == NULL)
    {
        ERR("AutoLen3x create fail \n");
        return S_FAIL;
    }
    return ret;
}

int encode_AutoLen3x_uninit(void)
{
    int ret = S_OK;

    if (NULL != gtVencAutoLen.pthreadAutoLen3x)
    {
       join_pthread(gtVencAutoLen.pthreadAutoLen3x);
       gtVencAutoLen.pthreadAutoLen3x = NULL;
    }
    pthread_mutex_destroy(&gtVencAutoLen.MdMutex);
    DBG("Now AutoLen3x uninit \n");
    return ret;
}

