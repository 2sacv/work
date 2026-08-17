/*
 *       Filename:  ptz_follow.c
 *    Description:
 *        Version:  1.0
 *        Created:  2025年03月27日 10时30分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (),
 *   Organization:
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "debug.h"
#include "ptz_follow.h"
#include "ptz_ctrl.h"
#include "conf_list.h"
#include "jconfig.h"

struct mvbuf mvtrk = {0};

#define clr_mvbuf() memset(&mvtrk, 0, sizeof(mvtrk))

/* relative_i = 0, 最新
 * relative_i = 1, 次新
 **/
#define SNAP(relative_i) (mvtrk.snap[((mvtrk.i - relative_i + SZ_MVBUF) % SZ_MVBUF)])

void trk_osd(int p1, int t1, int speed, struct mv_snap *s, int up, int down, int left, int right, int x_slow, int y_slow)
{
    static OsdExpandS osdExpand;
    static int osdinit = 0;
    if (osdinit == 0) {
        get_config(handleOsdExpandCfg, osdExpand);
        osdinit = 1;
    }

    int osdenable = 1;
    snprintf(osdExpand.cusosd[1].content, sizeof(osdExpand.cusosd[1].content) - 1,
            "p: %d t:%d -> %d %d, s: %d", s->p, s->t, p1, t1, speed);
    osdExpand.cusosd[1].enable = osdenable;
    osdExpand.cusosd[1].x = 10;
    osdExpand.cusosd[1].y = 140;

    snprintf(osdExpand.cusosd[2].content, sizeof(osdExpand.cusosd[2].content) - 1,
            "x,y: %d %d w,h: %d %d", s->x, s->y, s->w, s->h);
    osdExpand.cusosd[2].enable = osdenable;
    osdExpand.cusosd[2].x = 10;
    osdExpand.cusosd[2].y = 200;

    snprintf(osdExpand.cusosd[3].content, sizeof(osdExpand.cusosd[3].content) - 1,
            "U:%d,D:%d,L:%d,R:%d,S:%d/%d", up, down, left, right, x_slow, y_slow);
    osdExpand.cusosd[3].enable = osdenable;
    osdExpand.cusosd[3].x = 10;
    osdExpand.cusosd[3].y = 260;

    send_conf_data(JEvent_OsdExpandCfgChg, &osdExpand, sizeof(OsdExpandS));
}

void clear_item(void)
{
    clr_mvbuf();
}

void add_item(int tik, int trkid, MotorStatus *status, int x, int y, int w, int h)
{
    if (trkid != mvtrk.trkid) {
        mvtrk.trkid = trkid;
    }

    mvtrk.i = (mvtrk.i+1) % SZ_MVBUF;

    mvtrk.snap[mvtrk.i].tik = tik;
    mvtrk.snap[mvtrk.i].valid = TRUE;
    mvtrk.snap[mvtrk.i].x = x;
    mvtrk.snap[mvtrk.i].y = y;
    mvtrk.snap[mvtrk.i].w = w;
    mvtrk.snap[mvtrk.i].h = h;
    mvtrk.snap[mvtrk.i].p = status->steps[PAN_MOTOR];
    mvtrk.snap[mvtrk.i].t = status->steps[TILT_MOTOR];

    return;
}

/* 水平视角 80°
 * 水平步数 = 步/像素 * 像素N
 *          = (80度对应的步数 / 1920像素) * 像素N
 *          =  80 * (一圈的步数 / 270度) / 1920 * 像素N
 *          =  N * 80 * 一圈的步数 / 270 / 1920
 **/
static float x_pix2step(PtzCfg *cfg, int x_pix)
{
    return 1.0 * x_pix *
        cfg->motor_cfg.max_h_ViewAngle * cfg->motor_cfg.h_maxstep
        / cfg->motor_cfg.max_h_Angle / BASE_WIDTH;
}

static float y_pix2step(PtzCfg *cfg, int y_pix)
{
    return 1.0 * y_pix *
        cfg->motor_cfg.max_v_ViewAngle * cfg->motor_cfg.v_maxstep
        / cfg->motor_cfg.max_v_Angle / BASE_HEIGHT;
}

/**
 * 将变倍后的坐标还原为变倍之前的坐标
 * 计算显示区域参数
 * 左边界坐标: origin_x = (原始宽度 - 原始宽度 / 变倍系数) / 2
 *
 * 中心点坐标还原
 * x = origin_x + 变倍后x / 变倍系数
 * y = origin_y + 变倍后y / 变倍系数
 *
 * 还原宽高 (直接除以缩放系数)
 * w = 变倍后w / 变倍系数
 * h = 变倍后h / 变倍系数
*/
int x_zoom2origin(int x)
{
    int dzoom_pos = dzoom_get_cur_zoom();
    int origin_x = (BASE_WIDTH - BASE_WIDTH * DZOOM_MIN_ZOOM / dzoom_pos) / 2;
    return origin_x + x * DZOOM_MIN_ZOOM / dzoom_pos;
}

int y_zoom2origin(int y)
{
    int dzoom_pos = dzoom_get_cur_zoom();
    int origin_y = (BASE_HEIGHT - BASE_HEIGHT * DZOOM_MIN_ZOOM / dzoom_pos) / 2;
    return origin_y + y * DZOOM_MIN_ZOOM / dzoom_pos;
}

int w_zoom2origin(int w)
{
    return w * DZOOM_MIN_ZOOM / dzoom_get_cur_zoom();
}

int h_zoom2origin(int h)
{
    return h * DZOOM_MIN_ZOOM / dzoom_get_cur_zoom();
}

/**
 * 将还原后的坐标转换为变倍后的坐标
 * 计算显示区域参数
 * 视口左上角x: viewport_x = 原图中心x - (原图宽度 / (2 * 缩放倍数))
 * 视口左上角y: viewport_y = 原图中心y - (原图高度 / (2 * 缩放倍数))
 * 变倍后x = (原始x - 视口左上角x) * 缩放因子
 * 变倍后y = (原始y - 视口左上角y) * 缩放因子
 *
 * 还原宽高 (直接乘以缩放系数)
 * 变倍后w = 原始宽度 * 缩放因子
 * 变倍后h = 原始高度 * 缩放因子
*/
int x_origin2zoom(int x)
{
    int dzoom_pos = dzoom_get_cur_zoom();
    int viewport_x = (BASE_WIDTH / 2) - (BASE_WIDTH * DZOOM_MIN_ZOOM / dzoom_pos / 2);
    return (x - viewport_x) * dzoom_pos / DZOOM_MIN_ZOOM;
}

int y_origin2zoom(int y)
{
    int dzoom_pos = dzoom_get_cur_zoom();
    int viewport_y = (BASE_HEIGHT / 2) - (BASE_HEIGHT * DZOOM_MIN_ZOOM / dzoom_pos / 2);
    return (y - viewport_y) * dzoom_pos / DZOOM_MIN_ZOOM;
}

int w_origin2zoom(int w)
{
    return w * dzoom_get_cur_zoom() / DZOOM_MIN_ZOOM;
}

int h_origin2zoom(int h)
{
    return h * dzoom_get_cur_zoom() / DZOOM_MIN_ZOOM;
}

/* #  4 种状态:
 * 1. 靠近中心
 * 2. 远离中心
 * 3. 在中心缓冲区
 * 4. 静止
 *
 * #  2 个运动方向
 * 0. 成人每秒二步
 * x. 左右运动，每 STEP 就是 width
 * y. 上下运动，将 H 分为 上top 中mid 下bot 3区，每秒距离分别 t=h/9 m=h/6 b=h/3
 *    从 Center 到 Bot，只需要 3 秒。
 *
 * in : @mvtrk      快照缓冲
 * out: @           动力速度
 * ret: @leaving    1:远离中心 0:其它
 *
 **/

#define AMAX(x,y)   (abs(x)>abs(y)?(x):(y))
#define C_R(w)      (1920/2 + RANGE(w,60 ,150)/2)     // 5m  150, Buffer是一个人的宽度
#define C_L(w)      (1920/2 - RANGE(w,60 ,150)/2)     // 13m 60
#define C_U(h)      (1080/2 - RANGE(h,180,360)/2)     // 5m  400, Buffer是一个人的高度
#define C_D(h)      (1080/2 + RANGE(h,180,360)/2)     // 13m 170

int is_leaving_center(PtzCfg *cfg, PtzRun *run, FollowPreset *f_preset)
{
    struct mv_snap *snap5 = &SNAP(5);
    struct mv_snap *snap4 = &SNAP(4);
    struct mv_snap *snap2 = &SNAP(2);
    struct mv_snap *snap1 = &SNAP(1);
    struct mv_snap *snap0 = &SNAP(0);
    int speed = 0;
    int leaving = TRUE, x_slow = FALSE, y_slow = FALSE;
    int up = 0, down = 0, left = 0, right = 0;
    int H_STEP = RANGE(snap0->w, 32 , 480); // width  of body when distance=1m , distance=30m
    int V_STEP = RANGE(snap0->h, 100, 800); // height of body when distance=1m , distance=30m
    int max_speed = 44;
    int min_speed = 10;
    dbg_ptz("max_speed: %d, min_speed: %d\n", max_speed, min_speed);
    //set_g_run(ivs, RUN_IVS_TRACK);

    /*计算到目标的像素距离*/
    int base_center_x = BASE_WIDTH / 2;
    int base_center_y = BASE_HEIGHT / 2;
    int x_pix = base_center_x - snap0->x;
    int y_pix = base_center_y - snap0->y;
    dbg_ptz("x_pix: %d, y_pix: %d\n", x_pix, y_pix);

    /*计算一个像素对应水平垂直的步数*/
    float x_step_of_1p = x_pix2step(cfg, 1);
    float y_step_of_1p = y_pix2step(cfg, 1);

#if defined(DIGITAL_ZOOM)
    // 数字变倍之后视角会相应的缩小
    x_step_of_1p = x_step_of_1p * 100 / dzoom_get_cur_zoom();
    y_step_of_1p = y_step_of_1p * 100 / dzoom_get_cur_zoom();
#endif
    dbg_ptz("x_step_of_1p: %f, y_step_of_1p: %f\n", x_step_of_1p, y_step_of_1p);

    /*计算水平垂直马达需要运动的步数*/
    int x_steps = (int)(x_pix * x_step_of_1p);
    int y_steps = (int)(y_pix * y_step_of_1p);
    dbg_ptz("x_steps: %d, y_steps: %d\n", x_steps, y_steps);

    /*根据马达翻转计算运动方向*/
    Direction_t x_dir = handle_pan_reverse(DIRECTION_DOWN);
    Direction_t y_dir = handle_tilt_reverse(DIRECTION_DOWN);
    dbg_ptz("x_dir: %d, y_dir: %d\n", x_dir, y_dir);

    /*计算目标马达位置*/
    int dest_x = snap0->p + x_steps * x_dir;
    int dest_y = snap0->t + y_steps * y_dir;
    int y_max_step = cfg->motor_cfg.v_maxstep;
    int x_max_step = cfg->motor_cfg.h_maxstep;
    f_preset->preset.steps[PAN_MOTOR]  = RANGE(dest_x, 0, x_max_step);
    f_preset->preset.steps[TILT_MOTOR] = RANGE(dest_y, 0, y_max_step);
    dbg_ptz("p: %d, t: %d, dest_pan_steps: %d, dest_tilt_steps: %d\n",
        snap0->p, snap0->t,
        f_preset->preset.steps[PAN_MOTOR], f_preset->preset.steps[TILT_MOTOR]);

#if defined(DIGITAL_ZOOM)
    /*数字变倍处理*/
    f_preset->preset.dzoom_pos = dzoom_get_cur_zoom();
    if (is_dzoom_enable()) {
        if (w_origin2zoom(snap0->w) > ZOOM_IN_LIMIT) { // 缩小
            f_preset->preset.dzoom_pos -= FOLLOW_DZOOM_STEP;
            if (f_preset->preset.dzoom_pos < DZOOM_MIN_ZOOM) {
                f_preset->preset.dzoom_pos = DZOOM_MIN_ZOOM;
            }
        } else if (w_origin2zoom(snap0->w) < ZOOM_OUT_LIMIT) { // 放大
            // 只有人在中间区域的时候才放大, 考虑变倍的影响
            if (x_origin2zoom(snap0->x) > BASE_WIDTH / 4 &&
                x_origin2zoom(snap0->x) < BASE_WIDTH * 3 / 4 &&
                y_origin2zoom(snap0->y) > BASE_HEIGHT / 4 &&
                y_origin2zoom(snap0->y) < BASE_HEIGHT * 3 / 4) {
                f_preset->preset.dzoom_pos += FOLLOW_DZOOM_STEP;
                if (f_preset->preset.dzoom_pos > DZOOM_MAX_ZOOM) {
                    f_preset->preset.dzoom_pos = DZOOM_MAX_ZOOM;
                }
            }
        }
    }
#endif

    if (snap0->p == f_preset->preset.steps[PAN_MOTOR] &&
        snap0->t == f_preset->preset.steps[TILT_MOTOR]
#if defined(DIGITAL_ZOOM)
        && f_preset->preset.dzoom_pos == dzoom_get_cur_zoom()
#endif
    ) {
        /*位置没变并且变倍倍数没变，不需要跟踪*/
        leaving = FALSE;
        goto __exit;
    }

    /*
     * 判断是否 leaving, 4帧走1/2步, 2帧走1/4步
     *
     **/
    if (!snap2->valid) {
        return FALSE;
    }

    if (snap0->x > C_R(snap0->w) && snap0->x - snap2->x > H_STEP/6) { // right
        right = TRUE;
    } else if (snap0->x < C_L(snap0->w) && snap0->x - snap2->x < -H_STEP/6) { // left
        left = TRUE;
    }

    // 靠边进入, 缓慢运动, 人与云台的运动方向相对
    if (!(right || left || up || down) && snap4->valid) {
        if (snap0->x > C_R(snap0->w) && snap0->x - snap4->x < -H_STEP/6) { // 人在右, 向左走
            left = TRUE; x_slow = 2; speed = 5;
        } else if (snap0->x < C_L(snap0->w) && snap0->x - snap4->x > H_STEP/6) { // 人在左, 向右走
            right = TRUE; x_slow = 2; speed = 5;
        }

        if (snap0->y < C_U(snap0->h) && snap0->y - snap4->y > V_STEP/2) { // 人在上, 向下走
            down = TRUE; y_slow = 2; speed = 5;
        } else if (snap0->y > C_D(snap0->h) && snap0->y - snap4->y < -V_STEP/2) { // 人在下, 向上走
            up = TRUE; y_slow = 2; speed = 5;
        }
    }

    // 解决人形非常慢的情况, 直接用距离判断
    if (!(right || left) && snap4->valid) {
        if (snap0->x > C_R(snap0->w)) {
            if (snap0->x - snap4->x > H_STEP/6) { // right
                right = TRUE;
            } else {
                int d_right = x_origin2zoom(snap0->x) + w_origin2zoom(snap0->w) / 2;
                if (d_right > BASE_WIDTH * 2 / 3) {
                    right = TRUE; x_slow = TRUE;
                }
            }
        } else if (snap0->x < C_L(snap0->w)) {
            if (snap0->x - snap4->x < -H_STEP/6) { // left
                left = TRUE;
            } else {
                int d_left = x_origin2zoom(snap0->x) - w_origin2zoom(snap0->w) / 2;
                if (d_left < BASE_WIDTH / 3) {
                    left = TRUE; x_slow = TRUE;
                }
            }
        }
    }

    /* 垂直方向半秒距离 */
    if (snap5->valid) {
        if (snap0->y < C_U(snap0->h)) {                     // top
            V_STEP = snap0->h/27;                           /* 真实步的 0.6 倍 */
            if (snap0->y - snap5->y < -V_STEP) {
                up = TRUE;
            } else {
                int d_top = 540 - (h_origin2zoom(snap0->h)*1.5);    // Buf区偏移一个 h
                if (snap0->y < d_top) {
                    up = TRUE; y_slow = TRUE;               // 电机.v 比真实有人移动速度要降
                }
            }
        } else if (snap0->y > C_D(snap0->h)) {              // bot
            V_STEP = snap0->h/18;
            if (snap0->y - snap5->y > V_STEP) {             // 移动
                down = TRUE;
            } else {
                // bot=360 分三份，直接用距离判断
                int d_bot = 1080 - (y_origin2zoom(snap0->y)+h_origin2zoom(snap0->h)/2);
                if (d_bot < 240) {                          // 脚触到 bot 的 2/3
                    down = TRUE; y_slow = TRUE;
                }
            }
        } else {
            V_STEP = snap0->h/14;
        }
    }

    if (speed == 0) {
        if (snap0->w <= 200 && abs(x_steps) <= snap0->w) {
            speed = 5;
        } else {
            speed =  abs(x_steps) * 7 * cfg->motor_cfg.max_h_ViewAngle / BASE_WIDTH;
        }
    }

    // 单电机旋转时, 要判断水平垂直电机的优先级, 垂直电机在极限位置时不优先
    if (snap0->t != f_preset->preset.steps[TILT_MOTOR]) {
        // 垂直电机优先
        int y_priority = FALSE;
        if (abs(f_preset->preset.steps[TILT_MOTOR]-snap0->t) * 2 > abs(f_preset->preset.steps[PAN_MOTOR]-snap0->p)) {
            y_priority = TRUE;
        }

        // 当人形框位于上1/3, 下1/4时, 优先垂直电机
        if (y_origin2zoom(snap0->y) < BASE_HEIGHT / 3 ||
            y_origin2zoom(snap0->y) > BASE_HEIGHT * 3 / 4) {
            y_priority = TRUE;
        }

        if (y_priority) {
            f_preset->preset.steps[PAN_MOTOR] = snap0->p;
            dest_y = snap0->t + y_steps * y_dir / 3;
            f_preset->preset.steps[TILT_MOTOR] = RANGE(dest_y, 0, y_max_step);
            speed = 11;
            down = TRUE;
        }
    }

    /* 判断移动的角速度，尽量平滑以避免运动马赛克，依 tik 考虑不连续检到
     * °/s 与 speed[1~63] 是接近的
     * 1秒内的像素 = 2帧像素*7
     * speed ~= 多少°/s
     *       ~= 1秒内的像素 / 1度的像素
     *        = diff_x_pix * 7 / (BASE_WIDTH / g_cfg_ptz->motor_cfg.max_h_ViewAngle)
     *        = diff_x_pix * 7 * g_cfg_ptz->motor_cfg.max_h_ViewAngle / BASE_WIDTH
     **/
    //speed =  abs(diff_x_pix) * 7 * cfg->motor_cfg.max_h_ViewAngle / BASE_WIDTH;
    speed = speed * DZOOM_MIN_ZOOM / f_preset->preset.dzoom_pos; // 根据变倍计算速度
    f_preset->speed = RANGE(speed, min_speed, max_speed);        // 40°/s ~ 8°/s

__exit:
    dbg_ptz("tik: %d, leaving: %d up: %d down: %d left: %d right: %d\n"
            "x ,y,w,h : [%d, %d, %d, %d], H_STEP[%d]\n"
            "x0,1,2,4 : [%d, %d, %d, %d] @ %d in [%d ~ %d]\n"
            "old p t: [%d, %d] new [%d %d] -> %d %d\n"
            "x_slow: %d, speed: %d -> %d\n",
            snap0->tik, leaving, up, down, left, right,
            snap0->x, snap0->y, snap0->w, snap0->h, H_STEP,
            snap0->x, snap1->x, snap2->x, snap4->x, (snap0->x - snap1->x), H_STEP/3, H_STEP/5,
            snap0->p, snap0->t, dest_x, dest_y, f_preset->preset.steps[PAN_MOTOR], f_preset->preset.steps[TILT_MOTOR],
            x_slow, speed, f_preset->speed
    );

    // static int moving_p = FALSE;
    int moving_c = (up||down||left||right);

    /*if (get_g_run(ivs, RUN_IVS_FOLLOW_OSD) ) {
        // 停止移动，数据要清0
        if ((!moving_c && moving_p != moving_c) || (leaving && moving_c)) {
            trk_osd(
                f_preset->preset.steps[PAN_MOTOR], f_preset->preset.steps[TILT_MOTOR], speed,
                snap0, up, down, left, right, x_slow, y_slow
                    );
        }
    }*/
    // moving_p = moving_c;

    return (leaving && moving_c);
}

