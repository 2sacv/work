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
#ifndef _H_USR_SUPER_
#define _H_USR_SUPER_
#ifdef __cplusplus
extern "C" {
#endif

int is_usr_super(char *devid, char *user, char *passwd);
int get_usr_super(char *devid, char buf[], int buflen);
int do_verifystr(char *verifystr, char buf[], int buflen);

#ifdef __cplusplus
}
#endif

#endif


