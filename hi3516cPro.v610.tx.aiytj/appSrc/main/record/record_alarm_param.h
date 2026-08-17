/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2024-06-27
 * Version      : 1.0
 * Author       : tangjx
 * Description  :
 */

#ifndef _RECORD_ALARM_PARAM_h_
#define _RECORD_ALARM_PARAM_h_

#ifdef __cplusplus
    extern "C" {
#endif

#define MINS_OF_1DAY 1440
#define MINS_ALARM_RECORD 3

#define REC_ALARM_FILE      "rec.index"

typedef struct
{
    int  is_init;
    int  need_save;
    int  is_cross_day;
    int  startday;
    char recalarm_file[256];
    char recalarm_flag[MINS_OF_1DAY + 1];
    char second_day_recflag[MINS_OF_1DAY + 1];
} RecAlarmParam_t;

int init_recalarm_param(int recalarm);
void uinit_recalarm_flag(void);
void set_recalarm_flag(int alarm_record, int time_record);
void get_recalarm_flag(int day, char *flag_buf, int buflen);
void get_curday_recalarm_flag(char *flagbuf, int buflen);

#ifdef __cplusplus
}
#endif
#endif

