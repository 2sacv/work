#ifndef _TIME_SYNC_H
#define _TIME_SYNC_H
#ifdef __cplusplus 
extern "C" {
#endif

int start_timesync(time_t utc_secs);
int clear_timesync();
int check_timesync(time_t utc_secs);


#ifdef __cplusplus
}
#endif
#endif
