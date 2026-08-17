/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2015-02-06
 * Version      : 1.0
 * Author       : cheby
 * Description  :
 */
#ifndef _ALARM_CANCEL_H_
#define _ALARM_CANCEL_H_

#ifdef __cplusplus
extern "C" {
#endif

    int init_handle_alarm_cancel(void* data) ;

    void uninit_handle_alarm_cancel(void);

    int handle_alarm_cancel(int id);

#ifdef __cplusplus
}
#endif

#endif

