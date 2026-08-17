#ifndef _ALARM_SERVICE_H_
#define _ALARM_SERVICE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "system_ctrl.h"
#include "jconfig.h"

enum {
    CMD_LK_VLOSS        = 1 <<  0,
    CMD_LK_VMASK        = 1 <<  1,
    CMD_LK_MD           = 1 <<  2,
    CMD_LK_HD           = 1 <<  3,
    CMD_LK_CAR          = 1 <<  4,
    CMD_LK_PET          = 1 <<  5,
    CMD_LK_CRY          = 1 <<  6,
    CMD_LK_FACE         = 1 <<  7,
    CMD_LK_VGLINE       = 1 <<  8,
    CMD_LK_VGRECT       = 1 <<  9,
    CMD_LK_DISKFULL     = 1 << 10,
    CMD_LK_DISKERR      = 1 << 11,
    CMD_CFG_IPCFLICT    = 1 << 12,
    CMD_CFG_IPBROKEN    = 1 << 13,
    CMD_CFG_MD          = 1 << 14,
    CMD_CFG_HD          = 1 << 15,
    CMD_CFG_CAR         = 1 << 16,
    CMD_CFG_PET         = 1 << 17,
    CMD_CFG_CRY         = 1 << 18,
    CMD_CFG_FACE        = 1 << 19,
    CMD_CFG_VGLINE      = 1 << 20,
    CMD_CFG_VGRECT      = 1 << 21,
    CMD_CFG_VLOSS       = 1 << 22,
    CMD_CFG_VMASK       = 1 << 23,
    CMD_CFG_DISKFULL    = 1 << 24,
    CMD_CFG_DISKERR     = 1 << 25,
    CMD_CFG_FOLLOW      = 1 << 26,
};

typedef struct {
    JEventType         alarm_type;
    int                cmd;
}EventCmdS;

typedef struct {
    // VideoLossLinkS      lk_vloss;
    MotionDetectLinkS   lk_md;
    VglineLinkS         lk_vgline;
    VgrectLinkS         lk_vgrect;
    VMaskAlarmLinkS     lk_vmask;
    HumanDetectLinkS    lk_hd;
    CarDetectLinkS      lk_car;
    PetDetectLinkS      lk_pet;
    CryDetectLinkS      lk_cry;
    IpLinkS             lk_ipcflict;
    IpLinkS             lk_ipbroken;
    // AlarmExpLinkS       lk_alarmexp;
    MotionDetectS       cfg_md;
    HumanDetectionS     cfg_hd;
    CarDetectionS       cfg_car;
    PetDetectionS       cfg_pet;
    CryDetectionS       cfg_cry;
    VglineS             cfg_vgline;
    VgrectS             cfg_vgrect;
    // AlarmExpandS        cfg_alarmexp;
    // VideoLossS          cfg_vloss;
    VMaskAlarmS         cfg_vmask;
    AudioAlarmS         cfg_audio_alarm;
    LightAlarmS         cfg_light_alarm;
    follow_info_t       cfg_follow;
}alarm_cfg ;

typedef struct {
    time_t videolosslink_time;
    time_t motionlink_time;
    time_t vglinelink_time;
    time_t vgrectlink_time;
    time_t maskalarm_time;
    time_t humandetectlink_time;
    time_t carlink_time;
    time_t petlink_time;
    time_t scenelink_time;
    time_t crylink_time;
    time_t facelink_time;
    time_t facesnaplink_time;
    time_t alarmin_time[AIN_MAX_CHN];
    time_t ipconflictlink_time;
    time_t cablediscon_time;
    //time_t illegalvisit_time;
    time_t alarmexp_time[AEXPAND_MAX_CHN];
} last_alarm_time_t;

struct alarm_map {
    int  cfg_stat;
    alarm_cfg  cfg;
    last_alarm_time_t last_event_time;
};

typedef struct {
    JALARM_TYPE type;
    unsigned int *times;
    int *enable;
} TimeCbS;

int get_alarm_link_cfg(JEventType type, void* link_cfg, int chn);

int parse_alarm_handle(void *req, void *buf, int bufsize, int *retlen, void *arg);

int init_alarm_server(void *data);

int uninit_alarm_server(void);

#ifdef __cplusplus
}
#endif
#endif

