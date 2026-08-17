/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : delay_handle.h
 * Created Time : 2012-10-08
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _delay_handle_H_
#define _delay_handle_H_
#ifdef __cplusplus
extern "C" {
#endif

    typedef void (*JDelayFuncPtr)(void *data);

    int init_delay_handle(void);
    void uninit_delay_handle(void);

    int delay_handle_add(JDelayFuncPtr func, void *data, const char *str);

#ifdef __cplusplus
}
#endif
#endif
