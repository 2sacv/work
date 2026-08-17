/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : confextcb.h
 * @Created Time : 2014-01-07
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _CONF_EXTCB_H_
#define _CONF_EXTCB_H_
#ifdef __cplusplus
extern "C" {
#endif
    int  multiDevMapsCbFunc(int i, void *arg[]);

    int  multiDevOptsCbFunc(int i, void *arg[]);

    int  sysUserMapsCbFunc(int i, void *arg[]);

    int  sysUserOptsCbFunc(int i, void *arg[]);

    int osdExpandMapsCbFunc(int i, void *arg[]);

    int osdExpandOptsCbFunc(int i, void *arg[]);

    int videoEncMapsCbFunc(int i, void *arg[]);

    int videoEncOptsCbFunc(int i, void *arg[]);

    int videoMaskMapsCbFunc(int i, void *arg[]);

    int videoMaskOptsCbFunc(int i, void *arg[]);
    
    int roiListMapsCbFunc(int i, void *arg[]);

    int roiListOptsCbFunc(int i, void *arg[]);

    int profileListMapsCbFunc(int i, void *arg[]);

    int profileListOptsCbFunc(int i, void *arg[]);

    int PresetlistMapsCbFunc(int i, void *arg[]);

    int PresetlistOptsCbFunc(int i, void *arg[]);

#ifdef __cplusplus
}
#endif
#endif


