/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : recordapi.h
 * @Created Time : 2014-03-03
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _RECORDAPI_H_
#define _RECORDAPI_H_

#include "record_watch.h"
#include "alarm_event.h"
#include "jconfstruct.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Name        : record_get_storage_info
 * Description : 
 * Param       : 
 * Returns     : suceess return 0, error return -1
 */
int record_get_storage_info(RecordCtrlS *ctrls, char *buf, int bufsize);

/**
 * Name        : record_request_rec
 * Description : 
 * Param       : 
 * Returns     : suceess return 0, error return -1
 */
int record_request_rec(eJRecType itype);
int record_request_stop(void);
RecStatusE record_get_currec_status(void);
int record_email_text(JALARM_TYPE type);


/**
 * Name        : record_storage_dev_add
 * Description : 
 * Param       : 
 * Returns     : suceess return 0, error return -1
 */
int record_storage_dev_add(char *path);


/**
 * Name        : record_storage_dev_remove
 * Description : 
 * Param       : 
 * Returns     : suceess return 0, error return -1
 */
int record_storage_dev_remove(char *path);

/**
 * Name        : record_request_format
 * Description : 
 * Param       : 
 * Returns     : suceess return 0, error return -1
 */
int record_request_format(const char *devname);


/**
 * Name        : record_get_format_status
 * Description : 
 * Param       : 
 * Returns     : suceess return 0, error return -1
 */
int record_get_format_status(int *isok);


#ifdef __cplusplus
}
#endif
#endif

