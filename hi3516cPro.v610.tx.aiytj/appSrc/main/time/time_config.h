#ifndef  __TIME_CONFIG_H__
#define  __TIME_CONFIG_H__
#include "confapi.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define TZ_FILE "/opt/etc/TZ"

    typedef struct {
        unsigned int Xl_i;
        unsigned int Xl_f;
    } l_fp;

    typedef struct {
        unsigned char li_vn_mode;       // contains leap indicator, version and mode
        unsigned char stratum;          // peer's stratum
        unsigned char ppoll;            // the peer polling interval
        char  precision;                // peer clock precision
        int  rootdelay;                 // distance to primary clock
        unsigned int rootdispersion;    // clock dispersion
        unsigned int refid;             // reference clock ID
        l_fp reftime;                   // time peer clock was last updated
        l_fp org;                       // originate time stamp
        l_fp rec;                       // receive time stamp
        l_fp xmt;                       // transmit time stamp
    } NtpPacket;

#define PKT_LI_VN_MODE(li, vn, md) \
    ((unsigned char)((((li) << 6) & 0xc0) | (((vn) << 3) & 0x38) | ((md) & 0x7)))

/*
 * NTP
 **/
int init_client_ntp_update(void *data);
void uninit_client_ntp_update(void);
int time_ntp_server_time(char *server_ip, int server_port);
time_t get_ntp_epoche(char *server_ip, int server_port);
void pri_ntp_epoche(void *data);

/*
 * system time & TZ
 **/
int init_system_zone();
int init_system_time();
int dump_system_time(time_t epoch);
int get_tz_seceast(void);
int get_tz_idx_by_seceast(int sec_east);

time_t mktime_utc(struct tm *ts);
int dump_tz_idx(int idx);

/*
 * RTC
 **/
int is_board_cost_effective();
int get_rtcstat(int *rtcstat);
int hwclock_get(time_t *tmGet);

#ifdef __cplusplus
}

#endif
#endif
