#include <string.h>
#include<math.h>
#include <stdio.h>
#include "jconfstruct.h"
#include "js_scheduler.h"
#include "confapi.h"
#include "debug.h"
#include "js_event.h"
#include "jconfig.h"
#include "utils.h"
#include "ptz_ctrl.h"
#include "confapi.h"
#include "cmdstat.h"
#include "system_sch.h"
#include "encode_videomask.h"

static struct videomask_cfg cfg = {{0}};
static struct videomask_cfg raw = {{0}};
static struct videomask_run run = {
    .status = -1
};
static struct videomask_cfg *p_cfg = &cfg;
static struct videomask_cfg *p_raw = &raw;
static struct videomask_run *p_run = &run;

/**
 * @brief 判断当前时间是否在布防时间范围内
 * @param now_mins    当前时间(从 0 点到现在的分钟)
 * @param begin_mins  布防开始时间(从 0 点到现在的分钟)
 * @param end_mins    布防结束时间(从 0 点到现在的分钟)
 * @return 1:在布防时间 0:不在布防时间
 */
static int is_time_in_range(int now_mins, int begin_mins, int end_mins)
{
    // 判断是否跨天（结束时间早于开始时间）
    if (begin_mins < end_mins) {
        // 非跨天：判断当前时间是否在 [begin_mins, end_mins] 区间
        return (now_mins >= begin_mins) && (now_mins <= end_mins);
    } else if (begin_mins > end_mins) {
        // 跨天：判断当前时间是否在 [begin_mins, 1440] 或 [0, end_mins] 区间
        return (now_mins >= begin_mins) || (now_mins <= end_mins);
    // 开始和结束时间相同，视为不布防
    } else {
        return 0;
    }
}

static int videomask_set_audioinfo(int mask_enable)
{
    AudioCfgS audioinfo = {0,};

    DBG("videomask_set_audioinfo :%d\n", mask_enable);

    conf_get_audiocfg(&audioinfo);
    if (mask_enable) {
        audioinfo.inenable = 0;
    } else {
        audioinfo.inenable = 1;
    }
    conf_set_audiocfg(audioinfo);

    return 0;
}

static int videomask_set_osdinfo(int mask_enable)
{
    int current_language = 0;
    OsdExpandS osdinfo = {0,};
    OsdInfoS tConfigParam = {0,};

    DBG("videomask_set_osdinfo mask_enable=%d\n", mask_enable);
    conf_get_osdexpandcfg(&osdinfo);
    conf_get_osdinfocfg(&tConfigParam);

    if (mask_enable) {
        //关闭常规字幕,开启提示字幕
        current_language = 0;
        DBG("current_language=%d\n", current_language);
        if(0 != osdinfo.cusosd[6].enable){
            osdinfo.cusosd[6].enable = 0;
        }
        osdinfo.cusosd[6].enable = 1;
        osdinfo.cusosd[6].id = 0;
        osdinfo.cusosd[6].x = 820;
        osdinfo.cusosd[6].y = 500;
        memset(&osdinfo.cusosd[6].content, 0, sizeof(osdinfo.cusosd[6].content));
        if(LANGUAGE_ENGLISH == current_language){
            snprintf(osdinfo.cusosd[6].content,128,"%s",VIDEOMASK_EN);
        }else{
            snprintf(osdinfo.cusosd[6].content,128,"%s",VIDEOMASK_CN);
        }

        DBG("osdinfo.cusosd[6].content=[%s]\n", osdinfo.cusosd[6].content);
        conf_set_osdexpandcfg(osdinfo);
    } else {
        osdinfo.cusosd[6].enable = 0;
        conf_set_osdexpandcfg(osdinfo);
    }

    return 0;
}

static int set_video_mask(int enable)
{
    static int enabled = FALSE;
    VideoMaskS vm = {0};
    char jcpcmd[256] = {0};
    char response[256] ={0};

    DBG("set_video_mask enable=%d\n", enable);
    memcpy(&vm, &p_cfg->vm, sizeof(vm));

    if (enable) {
        if (!enabled) {
            ptz_save_preset(PRESET_VIDEO_MASK, TRUE);
            DBG("shake_SavePreset shake_PresetNo!\n");
        } else {
            WAR("already enabled, won't save preset\n");
        }

        //VIDEOMASK_SWITCH_ON
        vm.mask[ID_VIDEO_MASK].color = 0;
        vm.mask[ID_VIDEO_MASK].x0 = 0;
        vm.mask[ID_VIDEO_MASK].x1 = 1919;
        vm.mask[ID_VIDEO_MASK].y0 = 0;
        vm.mask[ID_VIDEO_MASK].y1 = 1079;
        vm.mask[ID_VIDEO_MASK].enable = 1;
        conf_set_videomaskcfg(vm);

        memset(jcpcmd, 0,sizeof(jcpcmd));
        memset(response, 0,sizeof(response));

        videomask_set_audioinfo(1);
        videomask_set_osdinfo(1);

        if (0 == p_cfg->vm_plan.cover_direction) {
            ptz_move_motor(E_MOVE_UP, 0, TRUE);
        } else {
            ptz_move_motor(E_MOVE_DOWN, 0, TRUE);
        }

        DBG("exec video mask on success!\n"); 
    } else {
        //VIDEOMASK_SWITCH_OFF
        vm.mask[ID_VIDEO_MASK].color = 0;
        vm.mask[ID_VIDEO_MASK].x0 = 0;
        vm.mask[ID_VIDEO_MASK].x1 = 0;
        vm.mask[ID_VIDEO_MASK].y0 = 0;
        vm.mask[ID_VIDEO_MASK].y1 = 0;
        vm.mask[ID_VIDEO_MASK].enable = 0;
        conf_set_videomaskcfg(vm);

        memset(jcpcmd, 0, sizeof(jcpcmd));
        memset(response, 0, sizeof(response));

        videomask_set_audioinfo(0);
        videomask_set_osdinfo(0);

        ptz_call_preset(PRESET_VIDEO_MASK, TRUE);

        DBG("exec video mask off success!\n"); 
    }

    enabled = enable;

    return 0;
}

/**
 * @brief 判断当前星期几是否在布防计划中
 * @param week_str 周计划字符串（如"1010000"表示启用周一和周三）
 * @param tm 当前时间结构体
 * @return 1:启用 0:不启用
 */
static int is_weekday_enabled(const char *week_str, const struct tm *tm)
{
    if (!week_str || strlen(week_str) != 7) {
        return 0;
    }

    return (week_str[tm->tm_wday] == '1');
}

/**
 * @brief 判断当前是否在视频遮挡布防时间内
 * @return 1:在布防时间 0:不在布防时间
 */
int videomask_plan_time_judge(struct tm *p_tm, int now_mins, int begin_mins,
                              int end_mins)
{
    // 若未启用时间计划, 直接返回FALSE
    if (!p_cfg->vm_plan.enable) {
        return FALSE;
    }

    // 调试日志
    pri_vidmask(LVL_DBG, "Now: %02d:%02d:%02d, Begin: %02d:%02d, End: %02d:%02d\n",
                p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec,
                p_cfg->vm_plan.beginhour, p_cfg->vm_plan.beginmin,
                p_cfg->vm_plan.endhour, p_cfg->vm_plan.endmin);

    // 检查周计划是否匹配
    if (!is_weekday_enabled(p_cfg->vm_plan.week, p_tm)) {
        pri_vidmask(LVL_DBG, "weekday not enable\n");
        return FALSE;
    }

    return is_time_in_range(now_mins, begin_mins, end_mins);

}

void videomask_watch_cb(void *ctx)
{
    static int vmask_init = FALSE;
    static int vmask_prev = -1;
    static int mins_skip = -1;

    // 获取当前时间
    struct tm cur_tm;
    time_t cur_time = 0;
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;
    int cmd = cmd_get_command(p_cmd);
    int mask_enable = FALSE;
    int begin_mins = p_cfg->vm_plan.beginhour * 60 + p_cfg->vm_plan.beginmin;
    int end_mins = p_cfg->vm_plan.endhour * 60 + p_cfg->vm_plan.endmin;;

    cur_time = time(NULL);
    localtime_r(&cur_time, &cur_tm);

    int now_mins = cur_tm.tm_hour * 60 + cur_tm.tm_min;
    int is_period_edge = p_cfg->vm_plan.enable &&
                         (now_mins == begin_mins || now_mins == end_mins);

    if (cmd & CMD_VM_ENABLE) {
        set_video_mask(p_cfg->vm_plan.mask_enable);
        p_run->status = p_cfg->vm_plan.mask_enable;

        //如果在布防周期开始/结束的那一分钟临时取消/开启，这一分钟不再进行下一周期判断，否则开/关了就会立刻关/开
        if (is_period_edge) {
            pri_vidmask(LVL_DBG, "period edge %s videomask\n",
                        p_run->status ? "enable" : "disable");
            mins_skip = now_mins;
        }

        //1S 缓冲时间让电机先动起来
        goto exit;
    }

    if (mins_skip > 0) {
        if (now_mins != mins_skip) {
            mins_skip = -1;
        } else {
            pri_vidmask(LVL_DBG, "wait curr minute passed, Now: %02d:%02d:%02d\n",
                        cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec);
            goto exit;
        }
    }

    //因为开启隐私遮挡需要将电机转到最上/下，电机初始化完之后，才检测隐私遮挡
    //开启/关闭隐私遮挡时，需要等电机转完后再做判断，不然有可能叠加状态，
    //导致电机在转动/还未开始转动时记录坐标
    if (!is_ptz_init() || ptz_is_run()) {
        goto exit;
    }

    if (vmask_init) {
        mask_enable = videomask_plan_time_judge(&cur_tm, now_mins,
                                                begin_mins, end_mins);
        pri_vidmask(LVL_DBG, "placed time: %d, vmask enable: %d\n",
                    mask_enable, p_run->status);

        //进入/退出布防时间
        //或者全天布防时间，临时取消后，下个开始/结束时间点，需要刷新
        if (mask_enable != vmask_prev ||
            (is_period_edge && p_run->status != mask_enable)) {
            p_cfg->vm_plan.mask_enable = p_run->status = vmask_prev = mask_enable;
            set_video_mask(mask_enable);
            conf_set_videomaskplan_cfg(p_cfg->vm_plan);

            COLOR_G("vmask enable change to %d\n", mask_enable);
        }
    } else {
        vmask_prev = videomask_plan_time_judge(&cur_tm, now_mins, begin_mins, end_mins);

        if (p_cfg->vm_plan.mask_enable &&
            p_cfg->vm.mask[ID_VIDEO_MASK].enable) {
            mask_enable = TRUE;
        } else {
            mask_enable = FALSE;
        }

        set_video_mask(mask_enable);
        p_run->status = mask_enable;
        vmask_init = TRUE;

        COLOR_G("init vmask enable %d, vmask time: %d\n", mask_enable, vmask_prev);
    }

exit:
    return;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage & CMD_VIDEOMASK) {
        DBG("cmd_stat:%d\n", p_cmd->cmd_stage);

        if (p_cfg->vm_plan.mask_enable != p_raw->vm_plan.mask_enable) {
            DBG("video mask plan enable: %d\n", p_cfg->vm_plan.mask_enable);
            cmd_set_command(p_cmd, CMD_VM_ENABLE);
        }

        memcpy(&p_cfg->vm_plan, &p_raw->vm_plan, sizeof(p_raw->vm_plan));
    }

    return;
}

static void cb_videomask_plan(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VIDEOMASK, &p_raw->vm_plan, p_src, size);
}

int videomask_init(void)
{
    DBG("videomask init...\n");
    /* STEP 1 */
    static struct cmdstat cmdstat_videomask = {0};
    cmdstat_videomask.diff_cfg2cmd = diff_cfg2cmd;

    p_run->ctx = &cmdstat_videomask;

    conf_get_videomaskcfg(&p_cfg->vm);
    conf_get_videomaskplan_cfg(&p_cfg->vm_plan);

    //提前更新当前隐私遮挡开关配置
    p_run->status = p_cfg->vm_plan.mask_enable &&
                    p_cfg->vm.mask[ID_VIDEO_MASK].enable;

    memcpy(&p_raw->vm_plan, &p_cfg->vm_plan, sizeof(p_raw->vm_plan));

    /* STEP 2 */
    attach_config(JEvent_VideoMaskPlanCfg, cb_videomask_plan, p_run->ctx);

    /* STEP 3 */
    p_run->sch_videomask = sch_slow;

    js_create_timer_r(p_run->sch_videomask, 10, 1*1000, videomask_watch_cb,
                      p_run->ctx, &p_run->hdl_videomask);

    return 0;
}

int videomask_uninit(void)
{
    DBG("videomask uninit...\n");
    js_delete_timer_r(&p_run->hdl_videomask);
    p_run->sch_videomask = NULL;

    detach_config(JEvent_VideoMaskPlanCfg, cb_videomask_plan, p_run->ctx);

    return 0;
}

int videomask_enabled(void)
{
    if (-1 != p_run->status) {
        return p_run->status;
    } else {
        return FALSE;
    }
}
