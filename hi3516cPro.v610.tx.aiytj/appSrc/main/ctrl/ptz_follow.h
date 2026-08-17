#ifndef _PTZ_FOLLOW_H
#define _PTZ_FOLLOW_H
#ifdef __cplusplus 
extern "C" {
#endif

#include "ptz_ctrl.h"
#define SZ_MVBUF 8

struct mv_snap {
    int tik;
    int valid;              // buf[i] 数据是否有效
    int x;                  // 人形框
    int y;
    int w;                  // 宽
    int h;
    int p;                  // 云台水平位置
    int t;
};

struct mvbuf {
    int trkid;                       // 从人形算法中获取的真实 track_id
    int i;                          // buf当前索引
    struct mv_snap snap[SZ_MVBUF];  //
};

void clear_item(void);
void add_item(int tik, int trkid, MotorStatus *status, int x, int y, int w, int h);

int x_zoom2origin(int x);
int y_zoom2origin(int y);
int w_zoom2origin(int w);
int h_zoom2origin(int h);

int x_origin2zoom(int x);
int y_origin2zoom(int y);
int w_origin2zoom(int w);
int h_origin2zoom(int h);

int is_leaving_center(PtzCfg *cfg, PtzRun *run, FollowPreset *f_preset);

#ifdef __cplusplus
}
#endif
#endif
