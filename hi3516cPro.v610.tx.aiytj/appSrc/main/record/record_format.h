/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : storage_manage.h
 * Created Time : 2012-10-08
 * Version      : 1.0
 * Author       : dongzh
 * Description  :
 */

#ifndef _record_format_H_
#define _record_format_H_
#ifdef __cplusplus
extern "C" {
#endif

void format_handle_req(const char *devname);
int get_format_status();
int set_format_status(int status);

#ifdef __cplusplus
}
#endif
#endif
