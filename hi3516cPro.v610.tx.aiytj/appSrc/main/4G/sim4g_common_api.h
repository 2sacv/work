/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_common_api.h
 * @Created Time : 2022-08-22
 * @Version      : 2.0
 * @Author       : cheby
 * @Description  :
 */

#ifndef __SIM4G_COMMON_API_H__
#define __SIM4G_COMMON_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sim4g.h"

typedef struct {
    const char *field_name;
    char *dest_str;
    size_t dest_size;
} FieldMapping;

#define SIM_VIRTUAL_CARD_TITLE  "20241"

int do_md5sum_str(const char *ptr, char *out);

int sim4g_set_serial(int fd,int baud,int nbits,int parity,int stop);

int ha_readfully(int fd, void* buf, int nbytes, struct timeval *tv);

int sim4g_get_usb0_ip(char ip[]);

int sim4g_report_cloud_server_simcard(sim_4g_t * info);

int sim4g_run_AT_expect(char *AT, char *buf, int times, int size);

int sim4g_run_AT_expect_sec(int sec, char *AT, char *buf, int times, int size);

int scanf_AT_result(char *buf, char *tag);

int scanf_AT_result2(char *buf,  char *tag, char *result, int result_len);

int sim4g_burn(sim_4g_t *info);

int get_burn_result(int *burn_ok);

int sim4g_report_location(Sim4g *sim4g_info);


#ifdef __cplusplus
}
#endif
#endif
