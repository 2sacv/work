/******************************************************************************
    Copyright (C), 2025-2035, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_isp.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2025-01-08
    Description  :
    History      :
                        created by tangjianxue
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ot_type.h"
#include "ot_mpi_isp.h"
#include "ot_common_vpss.h"
#include "ot_sns_ctrl.h"
#include "ot_mpi_awb.h"
#include "ss_mpi_vi.h"
#include "ss_mpi_isp.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_awb.h"

#include "debug.h"
#include "utils.h"
#include "js_scheduler.h"
#include "conf_list.h"
#include "jconfig.h"
#include "confapi.h"
#include "system_ctrl.h"
#include "g_log.h"
#include "cmdstat.h"

#include "encodeapi.h"
#include "encode_video.h"
#include "encode_isp.h"
#include "encode_common.h"
#include "encode_vi.h"
#include "encode_vi_isp.h"

#include "system_ctrl.h"
#include "jevent.h"

#include "scene.h"
#include "system_sch.h"

enum {
    CMD_ISP_3AS         = 1 << 0,
    CMD_ISP_3AS_AE      = 1 << 1,   // 快门
    CMD_ISP_3AS_AWB     = 1 << 2,   // 白平衡（红蓝增益）
    CMD_ISP_3AS_BLC     = 1 << 3,   // 背光补偿
    CMD_ISP_3AS_LOWLH   = 1 << 4,   // 低照度增强
    CMD_ISP_3AS_NIGHTFM = 1 << 5,   // 夜视人脸模式

    CMD_ISP_ViInfoS     = 1 << 7,
    CMD_ISP_ViInfoS_NIG = 1 << 8,   // 夜视亮度
    CMD_ISP_ViInfoS_BRI = 1 << 9,   // 亮度
    CMD_ISP_ViInfoS_CON = 1 << 10,  // 对比度
    CMD_ISP_ViInfoS_HUE = 1 << 11,  // 色调
    CMD_ISP_ViInfoS_SAT = 1 << 12,  // 饱和度
    CMD_ISP_ViInfoS_SHA = 1 << 13,  // 锐度
    CMD_ISP_ViInfoS_LAM = 1 << 14,  // 光源频率
    CMD_ISP_ViInfoS_REV = 1 << 15,  // 视频镜像
    CMD_ISP_ViInfoS_GAI = 1 << 16,  // 增益
    CMD_ISP_ViInfoS_BRL = 1 << 17,  // 强度等级
    CMD_ISP_ViInfoS_SUP = 1 << 18,  // 强光抑制

    CMD_ISP_OUTDOOR_ISP = 1 << 20,  // 室外isp动态调整
    CMD_ISP_DENOISE     = 1 << 21,  // 降噪
};

typedef enum {
    ISP_SCENE_INIT   = 1,
    ISP_SCENE_SWITCH = 2,
    ISP_SCENE_UNINT  = 3,
}isp_params_scene_type_e;

static struct isp_cfg cfg = {0};
static struct isp_cfg raw = {0};
static struct isp_run run = {0};

static struct isp_cfg *g_cfg_isp = &cfg;
static struct isp_cfg *g_raw_isp = &raw;
static struct isp_run *g_run_isp = &run;

static int ISP_MODE = ISP_DAY;
static int high_temp = 0; //高温标志位

int get_isp_mode(void)
{
    return ISP_MODE;
}

static int encode_isp_video_set_brightness(ot_vi_pipe pipe, unsigned char bright)
{
    int ret = 0;
    ot_isp_csc_attr csc_attr = {0};

    do {
        ret= ss_mpi_isp_get_csc_attr(pipe, &csc_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
        csc_attr.luma = bright*100/255;
        ret = ss_mpi_isp_set_csc_attr(pipe, &csc_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
        DBG("bright:%d, csc_attr.luma:%d\n", bright, csc_attr.luma);
    } while (0);

    return ret;
}

static int encode_isp_video_set_contrast(unsigned char pipe, unsigned char contrast)
{
    int ret = 0;
    ot_isp_csc_attr csc_attr = {0};
    ESensorType eSysCase = system_get_snsr_type();

    do {
        ret= ss_mpi_isp_get_csc_attr(pipe, &csc_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);        
        csc_attr.contr = contrast*100/255;
        if(eSysCase == SENSOR_SC235){
                if(ISP_MODE == 2){
                    csc_attr.contr = csc_attr.contr + 5;
                    if(csc_attr.contr > 255){
                        csc_attr.contr = 255;
                    }
                }
        }
        ret = ss_mpi_isp_set_csc_attr(pipe, &csc_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
        DBG("contrast:%d, csc_attr.contr:%d\n", contrast, csc_attr.contr);
    } while (0);

    return ret;
}

static int encode_isp_video_set_saturation(unsigned char pipe, unsigned char saturation)
{
    int ret = 0;
    ot_isp_csc_attr csc_attr = {0};
    
    do {
         ret= ss_mpi_isp_get_csc_attr(pipe, &csc_attr);
         ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
         csc_attr.satu = saturation*100/255;
         ret = ss_mpi_isp_set_csc_attr(pipe, &csc_attr);
         ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
         DBG("saturation:%d, csc_attr.satu:%d\n", saturation, csc_attr.satu);
    } while (0);

    return ret;
}

static int encode_isp_video_set_hue(unsigned char pipe, unsigned char hue)
{
    int ret = 0;
    ot_isp_csc_attr csc_attr = {0};
    
    do {
         ret= ss_mpi_isp_get_csc_attr(pipe, &csc_attr);
         ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
         csc_attr.hue = hue*100/255;
         ret = ss_mpi_isp_set_csc_attr(pipe, &csc_attr);
         ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_csc_attr failed vi_pipe:%d\n", pipe);
         DBG("hue:%d, csc_attr.hue:%d\n", hue, csc_attr.hue);
    } while (0);

    return ret;
}


static int encode_isp_video_set_sharpness(unsigned char pipe, unsigned char sharpness)
{
    int ret = 0;
    ot_isp_sharpen_attr shp_attr = {0};

    do {
        ret= ss_mpi_isp_get_sharpen_attr(pipe, &shp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_sharpen_attr failed vi_pipe:%d\n", pipe);
        if(128 == sharpness) {
            shp_attr.op_type = OT_OP_MODE_AUTO;
        } else {
            shp_attr.op_type = OT_OP_MODE_MANUAL;
            shp_attr.manual_attr.detail_ctrl = sharpness;
        }
        ret= ss_mpi_isp_set_sharpen_attr(pipe, &shp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_sharpen_attr failed vi_pipe:%d\n", pipe);
    } while (0);

    DBG("set_sharpness sharpness:%d return ret:0x%x\n", sharpness, ret);
    return ret;
}

static int encode_isp_video_set_hiLightDepress(unsigned char pipe, unsigned short suppress)
{
    int ret = 0;
    int isp_suppress = 0;

    do {

    } while (0);

    DBG("set_hiLightDepress suppress:%d return ret:0x%x\n", isp_suppress, ret);
    return ret;
}

static int encode_isp_video_set_reverse(int pipe,int reverse)//镜像
{
    ot_isp_sns_obj    *sns_obj = NULL;
    ot_isp_sns_mirrorflip_type sns_mirror_flip = ISP_SNS_NORMAL;
    sns_type_t sns_type = SC465SL_MIPI_4M_30FPS_12BIT;
    ESensorType eSysCase = system_get_snsr_type();

    switch (eSysCase){
    case SENSOR_SC4336P:
        sns_type = SC4336P_MIPI_4M_30FPS_10BIT;
        break;
    case SENSOR_SC465SL: 
        sns_type = SC465SL_MIPI_4M_30FPS_12BIT;
        break;
    case SENSOR_SC235: 
        sns_type = SC235_MIPI_2M_15FPS_10BIT;
        break;
    default:
        ERR("init sdk : unkown sensor error\n");
        return -1;
    }

    do {
        sns_obj = encode_vi_isp_get_sns_obj(sns_type);
        ENCODE_NULL_BREAK(sns_obj);

        switch (reverse%4)
        {
            case 0:
                    sns_mirror_flip = ISP_SNS_NORMAL;
                    break;
            case 1:
                    sns_mirror_flip = ISP_SNS_MIRROR;
                    break;
            case 2:
                    sns_mirror_flip = ISP_SNS_FLIP;
                    break;
            case 3:
                    sns_mirror_flip = ISP_SNS_MIRROR_FLIP;
                    break;
        }

        if (sns_obj->pfn_mirror_flip != TD_NULL) {
            sns_obj->pfn_mirror_flip(pipe,sns_mirror_flip);
        } else {
            ERR("pfn_mirror_flip failed with TD_NULL!\n");
        }
    }while(0);

    DBG("set_reverse reverse:%d sns_mirror_flip:%d\n", reverse, sns_mirror_flip);
    return 0;
}

static int encode_isp_set_ae(int pipe, int ae)
{
    int ret = 0;
    ot_isp_exposure_attr exp_attr = {0};

    do {
        ret = ss_mpi_isp_get_exposure_attr(pipe, &exp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_exposure_attr failed vi_pipe:%d\n", pipe);

        exp_attr.bypass= TD_FALSE;
        exp_attr.op_type = OT_OP_MODE_MANUAL;
        exp_attr.manual_attr.exp_time_op_type = OT_OP_MODE_MANUAL;
        switch(ae) {
            case 0:
                exp_attr.op_type = OT_OP_MODE_AUTO;
                break;
            case 1:
                exp_attr.manual_attr.exp_time = 100;
                break;
            case 2:
                exp_attr.manual_attr.exp_time = 200;
                break;
            case 3:
                exp_attr.manual_attr.exp_time = 500;
                break;
            case 4:
                exp_attr.manual_attr.exp_time = 1000;
                break;
            case 5:
                exp_attr.manual_attr.exp_time = 2000;
                break;
            case 6:
                exp_attr.manual_attr.exp_time = 4000;
                break;
            case 7:
                exp_attr.manual_attr.exp_time = 5000;
                break;
            case 8:
                exp_attr.manual_attr.exp_time = 8000;
                break;
            case 9:
                exp_attr.manual_attr.exp_time = 10000;
                break;
            case 10:
                exp_attr.manual_attr.exp_time = 20000;
                break;
            case 11:
                exp_attr.manual_attr.exp_time = 40000;
                break;
            default:
                exp_attr.op_type = OT_OP_MODE_AUTO;
                break;
        }

        DBG("op_type:%d exp_time:%d\n", exp_attr.op_type, exp_attr.manual_attr.exp_time);
        ret = ss_mpi_isp_set_exposure_attr(pipe, &exp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_exposure_attr failed vi_pipe:%d\n", pipe);
    } while(0);

    return 0;
}

static int encode_isp_set_awb(int pipe, int awb, int redgain, int bluegain)
{
    int ret = 0;
    ot_isp_wb_attr wb_attr = {0};
    do {
        ret = ss_mpi_isp_get_wb_attr(pipe, &wb_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_wb_attr failed pipe:%d\n", pipe);
        wb_attr.op_type = awb;
        wb_attr.manual_attr.r_gain = redgain;
        wb_attr.manual_attr.b_gain = bluegain;
        ret = ss_mpi_isp_set_wb_attr(pipe, &wb_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_wb_attr failed pipe:%d\n", pipe);
    } while(0);

    return 0;
}

static int encode_isp_set_gain(unsigned char pipe, unsigned char gain)
{

    return 0;
}

static int encode_isp_video_set_shutter(unsigned char pipe, int time)
{
    int ret = 0;
    
    ret = encode_isp_set_ae(pipe, g_cfg_isp->v3as.ae);

    DBG("set_shutter time:%d return ret:0x%x\n", time, ret);
    return ret;
}

static int encode_isp_video_set_manual_awb(unsigned char pipe, Video3aS *video3acfg)
{
    int ret = 0;

    ret = encode_isp_set_awb(pipe, g_cfg_isp->v3as.awb, g_cfg_isp->v3as.redgain, g_cfg_isp->v3as.bluegain);

    return ret;
}

static int encode_isp_video_set_lampfrequency(unsigned char pipe, unsigned char value)
{
    int ret = 0;
    ot_isp_exposure_attr exp_attr = {0};
    
      do {
          ret= ss_mpi_isp_get_exposure_attr(pipe, &exp_attr);
          ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_exposure_attr failed vi_pipe:%d\n", pipe);
          exp_attr.auto_attr.antiflicker.enable = 1;
          exp_attr.auto_attr.antiflicker.mode = OT_ISP_ANTIFLICKER_AUTO_MODE;
          if(1 == value) {
            exp_attr.auto_attr.antiflicker.frequency = 50;
          } else {
            exp_attr.auto_attr.antiflicker.frequency = 60;
          }
          ret = ss_mpi_isp_set_exposure_attr(pipe, &exp_attr);
          ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_exposure_attr failed vi_pipe:%d\n", pipe);
          DBG("value:%d, frequency:%d\n", value, exp_attr.auto_attr.antiflicker.frequency);
      } while (0);

    return ret;
}

static int encode_isp_image_setting(void)
{
    int ret = 0;
    unsigned char pipe = 0;
    ViInfoS *viinfocfg = &g_cfg_isp->vinfos;
    Video3aS *video3acfg = &g_cfg_isp->v3as;

    ret = encode_isp_video_set_brightness(pipe, viinfocfg->bright);

    ret += encode_isp_video_set_contrast(pipe, viinfocfg->contrast);

    ret += encode_isp_video_set_saturation(pipe, viinfocfg->saturation);
    
    ret += encode_isp_video_set_hue(pipe, viinfocfg->hue);

    ret += encode_isp_video_set_sharpness(pipe, viinfocfg->sharpness);

    ret += encode_isp_video_set_reverse(pipe,viinfocfg->reverse);

    ret += encode_isp_video_set_hiLightDepress(pipe, viinfocfg->suppress);

    ret += encode_isp_set_gain(pipe, viinfocfg->gain);

    ret += encode_isp_video_set_lampfrequency(pipe, viinfocfg->lampfrequency);

    ret += encode_isp_video_set_shutter(pipe, video3acfg->ae);

    ret += encode_isp_video_set_manual_awb(pipe,video3acfg);
    return ret;
}

static int encode_isp_param(unsigned char pipe, int  mode,int stat)
{
    static int iPreMode = ISP_DAY; //初始化AX_ISP_LoadBinParams白天效果文件

    char pFile[256] = {0};
    char sensor_name[256] = {0};
    int ret = -1;
    int len = 0;
    LightExtCfg stLightExtcfg = {0};
    conf_get_lightext_cfg(&stLightExtcfg);

    ot_isp_awb_attr_ex awb_attr_ex = {0};
    ot_isp_wb_attr wb_attr = {0};

    if(ISP_SCENE_SWITCH == stat && iPreMode == mode && high_temp == 0) { //与上次load的效果文件相同将不在load
        return 0;
    }
    iPreMode = mode;
    ESensorType eSysCase = system_get_snsr_type();

    if(eSysCase == SENSOR_GC5603){
        strncpy(sensor_name, "gc5603", sizeof(sensor_name) - 1);
    }else if (eSysCase == SENSOR_SC465SL){
        strncpy(sensor_name, "sc465sl", sizeof(sensor_name) - 1);
    }else if (eSysCase == SENSOR_SC235){
        strncpy(sensor_name, "sc235", sizeof(sensor_name) - 1);
    }

    do{
        len = snprintf(pFile, sizeof(pFile), "/ipc/sensor/sensor_%s/",sensor_name);
        if(len < 0 || len >= sizeof(pFile)) {
            ERR("snprintf failed len:%d\n", len);
            break;
        }

        switch (mode) {
        case ISP_DAY:
            ISP_MODE = 0;
            break;
        case ISP_COLOR_NIGHT:
            ISP_MODE = 1;
            break;
        case ISP_INFRARED_NIGHT:
            ISP_MODE = 2;
            break;
        default:
            ERR("%d mode is invalid\n", mode);
            break;
        }

        //stat : 1 初始化scene场景模块及参数，2 切换scene场景 3 反初始化scene场景调用
        ret = scene_main(pFile, ISP_MODE, stat);
        break_if_fail(SUCCESS == ret, ret);
        usleep(200*1000);
        if(eSysCase == SENSOR_SC465SL){
            ret = ot_mpi_isp_get_awb_attr_ex(pipe, &awb_attr_ex);
            break_if_fail(SUCCESS == ret, ret);            
            ret = ot_mpi_isp_get_wb_attr(pipe, &wb_attr);
            break_if_fail(SUCCESS == ret, ret);

            awb_attr_ex.in_or_out.low_start = 4500;
            if(mode == ISP_DAY){
                wb_attr.auto_attr.luma_hist.hist_wt[0] = 32;
                wb_attr.auto_attr.luma_hist.hist_wt[1] = 128;
                //wb_attr.auto_attr.natural_cast_en = 0;
            }else{
                wb_attr.auto_attr.luma_hist.hist_wt[0] = 100;
                wb_attr.auto_attr.luma_hist.hist_wt[1] = 180;
                //wb_attr.auto_attr.natural_cast_en = 1;
            }

            ret = ot_mpi_isp_set_awb_attr_ex(pipe, &awb_attr_ex);
            break_if_fail(SUCCESS == ret, ret);
            ret = ot_mpi_isp_set_wb_attr(pipe, &wb_attr);
            break_if_fail(SUCCESS == ret, ret);
        }else if (eSysCase == SENSOR_SC235){
            //SC235
            ret = ot_mpi_isp_get_awb_attr_ex(pipe, &awb_attr_ex);
            break_if_fail(SUCCESS == ret, ret);            
            ret = ot_mpi_isp_get_wb_attr(pipe, &wb_attr);
            break_if_fail(SUCCESS == ret, ret);

            awb_attr_ex.in_or_out.low_start = 4500;
            if(mode == ISP_DAY ){
                wb_attr.auto_attr.bg_strength = 128;
                wb_attr.auto_attr.alg_type = OT_ISP_AWB_ALG_LOWCOST;
            }else if(mode == ISP_COLOR_NIGHT){
                wb_attr.auto_attr.bg_strength = 128;
                wb_attr.auto_attr.alg_type = OT_ISP_AWB_ALG_LOWCOST;
            }else {
                wb_attr.auto_attr.bg_strength = 128;
                wb_attr.auto_attr.alg_type = OT_ISP_AWB_ALG_ADVANCE;
            }

            ret = ot_mpi_isp_set_awb_attr_ex(pipe, &awb_attr_ex);
            break_if_fail(SUCCESS == ret, ret);
            ret = ot_mpi_isp_set_wb_attr(pipe, &wb_attr);
            break_if_fail(SUCCESS == ret, ret);
        }
        high_temp = 0;
        encode_isp_image_setting();
    }while(0);

    return ret;
}

static void cb_event_video3acfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_ISP_3AS, &g_raw_isp->v3as, p_src, size);
}

static void cb_event_videoincfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_ISP_ViInfoS, &g_raw_isp->vinfos, p_src, size);
}

static void cb_event_denoisecfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_ISP_DENOISE, &g_raw_isp->dnrcfgs, p_src, size);
}

static void cb_event_ispcolor(int id, void *p_src, int size, void *ctx)
{
    eIspColor isp_color = ISP_MAX;
    if (p_src) {
        isp_color = *(int *)p_src;
        encode_isp_param(0, isp_color, ISP_SCENE_SWITCH);
    }
}

static int exec_cmd_3acfg(int cmd)
{
    int ret = S_OK;
    ot_vi_pipe vi_pipe = 0;
    if (cmd & CMD_ISP_3AS_AE) {
        encode_isp_set_ae(vi_pipe, g_cfg_isp->v3as.ae);
    }

    if (cmd & CMD_ISP_3AS_AWB) {
        encode_isp_set_awb(vi_pipe, g_cfg_isp->v3as.awb, g_cfg_isp->v3as.redgain, g_cfg_isp->v3as.bluegain);
    }

    if (cmd & CMD_ISP_3AS_BLC) {
        
    }

    if (cmd & CMD_ISP_3AS_LOWLH) {
        
    }

    if (cmd & CMD_ISP_3AS_NIGHTFM) {
        
    }
    return ret;
}

static int exec_cmd_viinfo(int cmd)
{
    int ret = S_OK;
    unsigned char value = 0;
    int pipe = 0;

    if (cmd & CMD_ISP_ViInfoS_BRI) {
        value = g_cfg_isp->vinfos.bright;
        ret = encode_isp_video_set_brightness(pipe, value);
    }

    if (cmd & CMD_ISP_ViInfoS_CON) {
        value = g_cfg_isp->vinfos.contrast;
        ret += encode_isp_video_set_contrast(pipe, value);
    }

    if (cmd & CMD_ISP_ViInfoS_HUE) {
        value = g_cfg_isp->vinfos.hue;
        ret += encode_isp_video_set_hue(pipe, value);
    }

    if (cmd & CMD_ISP_ViInfoS_SAT) {
        value = g_cfg_isp->vinfos.saturation;
        ret += encode_isp_video_set_saturation(pipe, value);
    }

    if (cmd & CMD_ISP_ViInfoS_SHA) {
        value = g_cfg_isp->vinfos.sharpness;
        ret += encode_isp_video_set_sharpness(pipe, value);
    }

    if (cmd & CMD_ISP_ViInfoS_LAM) {
        value = g_cfg_isp->vinfos.lampfrequency;
        ret += encode_isp_video_set_lampfrequency(0, value);
    }

    if (cmd & CMD_ISP_ViInfoS_REV) {
        ret += encode_isp_video_set_reverse(0, g_cfg_isp->vinfos.reverse);
    }

    if (cmd & CMD_ISP_ViInfoS_SUP) {
        ret += encode_isp_video_set_hiLightDepress(0, g_cfg_isp->vinfos.suppress);
    }

    if (cmd & CMD_ISP_ViInfoS_GAI) {
        //ret += encode_isp_video_set_gain(0, g_cfg_isp->vinfos.gain, MaxAGain);
    }

    return ret;
}

static int exec_cmd_denoise()
{
    int ret = S_OK;

    return ret;
}

static void isp_high_temp(ESensorType stype)
{   
    ot_vi_pipe vi_pipe = 0;
    ot_isp_sns_obj    *sns_obj = NULL;
    ot_isp_exp_info  exp_info = {0};
    sns_type_t sns_type = SC465SL_MIPI_4M_30FPS_12BIT;
    td_u32 value_h = 0,value_l = 0,addr_h = 0,addr_l = 0;
    int value = 0,ret = 0;
    char pFile[256] = {0};
    char sensor_name[256] = {0};
    int len = 0;
    static int high_temp_true = 0;

    switch (stype){
        case SENSOR_SC4336P:
            sns_type = SC4336P_MIPI_4M_30FPS_10BIT;
            strncpy(sensor_name, "sc4336p", sizeof(sensor_name) - 1);
            break;
        case SENSOR_SC465SL: 
            sns_type = SC465SL_MIPI_4M_30FPS_12BIT;
            strncpy(sensor_name, "sc465sl", sizeof(sensor_name) - 1);
            break;
        case SENSOR_SC235: 
            sns_type = SC235_MIPI_2M_15FPS_10BIT;
            strncpy(sensor_name, "sc235", sizeof(sensor_name) - 1);
            break;
        default:
            ERR("init sdk : unkown sensor error\n");
            return ;
    }

    do{
        sns_obj = encode_vi_isp_get_sns_obj(sns_type);
        ENCODE_NULL_BREAK(sns_obj);

        if(stype == SENSOR_SC235){
            len = snprintf(pFile, sizeof(pFile), "/ipc/sensor/sensor_%s/",sensor_name);
            if(len < 0 || len >= sizeof(pFile)) {
                ERR("snprintf failed len:%d\n", len);
                break;
            }

            addr_h = 0x3998;
            addr_l = 0x3999;
            if (sns_obj->pfn_read_reg != TD_NULL) {
                value_h = sns_obj->pfn_read_reg(vi_pipe,addr_h);
                value_l = sns_obj->pfn_read_reg(vi_pipe,addr_l);
                value = (value_h << 8) + value_l;
            } else {
                ERR("pfn_mirror_flip failed with TD_NULL!\n");
            };

            ret = ss_mpi_isp_query_exposure_info(vi_pipe ,&exp_info);         
            ENCODE_RET_BREAK(ret, "ss_mpi_isp_query_exposure_info failed vi_pipe:%d\n",vi_pipe);

            if(value > 0x0108 && exp_info.iso > 25271){
                high_temp_true = 1;
            }else if(exp_info.iso < 23260 || (value < 0x102)){
                high_temp_true = 0;
            }

            if(high_temp_true){
                if(high_temp_true != high_temp && ISP_MODE == ISP_DAY){//进入高温
                    ret = scene_main(pFile, 3, 2);
                    break_if_fail(SUCCESS == ret, ret);
                    high_temp = 1;
                    //encode_isp_config_init();
                    encode_isp_image_setting();
                }
            }else if(high_temp_true != high_temp){          
                encode_isp_param(vi_pipe, ISP_MODE, ISP_SCENE_SWITCH);
                encode_isp_image_setting();
            }
        }

    }while(0);
    
    return;
}

static void media_isp_hightemp_watch()
{
    switch (system_get_snsr_type()) {
    case SENSOR_SC465SL:
        isp_high_temp(SENSOR_SC465SL);
        break;
    case SENSOR_SC235:
        isp_high_temp(SENSOR_SC235);
        break;
    default:
        break;
    }
}

static void loop_isp(void *ctx)
{
    static int call_cnt = 0;
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd) {
        if (cmd & CMD_ISP_3AS) {
            exec_cmd_3acfg(cmd);
        }
        
        if (cmd & CMD_ISP_ViInfoS) {
            exec_cmd_viinfo(cmd);
        }

        if (cmd & CMD_ISP_DENOISE) {
            exec_cmd_denoise();
        }
    }

    if (++call_cnt >= HIGHTEMP_WATCH_PERIOD) {    // 5s
        call_cnt = 0;
        // 高温逻辑
        media_isp_hightemp_watch();
    } 
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;
    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_ISP_3AS) {
            if (g_cfg_isp->v3as.ae != g_raw_isp->v3as.ae) {
                cmd_set_command(p_cmd, CMD_ISP_3AS_AE);
            }

            if (g_cfg_isp->v3as.awb != g_raw_isp->v3as.awb ||
               g_cfg_isp->v3as.redgain != g_raw_isp->v3as.redgain ||
               g_cfg_isp->v3as.bluegain != g_raw_isp->v3as.bluegain) {
                cmd_set_command(p_cmd, CMD_ISP_3AS_AWB);
            }

            if (g_cfg_isp->v3as.blc != g_raw_isp->v3as.blc) {
                cmd_set_command(p_cmd, CMD_ISP_3AS_BLC);
            }

            if (g_cfg_isp->v3as.lowlightEnhance != g_raw_isp->v3as.lowlightEnhance) {
                cmd_set_command(p_cmd, CMD_ISP_3AS_LOWLH);
            }

            if (g_cfg_isp->v3as.nightfacemode != g_raw_isp->v3as.nightfacemode) {
                cmd_set_command(p_cmd, CMD_ISP_3AS_NIGHTFM);
            }

            memcpy(&g_cfg_isp->v3as, &g_raw_isp->v3as, sizeof(g_cfg_isp->v3as));
        }

        if (p_cmd->cmd_stage & CMD_ISP_ViInfoS) {
            if (g_cfg_isp->vinfos.nightluma != g_raw_isp->vinfos.nightluma) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_NIG);
            }

            if (g_cfg_isp->vinfos.bright != g_raw_isp->vinfos.bright) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_BRI);
            }

            if (g_cfg_isp->vinfos.contrast != g_raw_isp->vinfos.contrast) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_CON);
            }

            if (g_cfg_isp->vinfos.hue != g_raw_isp->vinfos.hue) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_HUE);
            }

            if (g_cfg_isp->vinfos.saturation != g_raw_isp->vinfos.saturation) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_SAT);
            }

            if (g_cfg_isp->vinfos.sharpness != g_raw_isp->vinfos.sharpness) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_SHA);
            }

            if (g_cfg_isp->vinfos.lampfrequency != g_raw_isp->vinfos.lampfrequency) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_LAM);
            }

            if (g_cfg_isp->vinfos.reverse != g_raw_isp->vinfos.reverse) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_REV);
            }

            if (g_cfg_isp->vinfos.suppress != g_raw_isp->vinfos.suppress) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_SUP);
            }

            if(g_cfg_isp->vinfos.gain != g_raw_isp->vinfos.gain) {
                cmd_set_command(p_cmd, CMD_ISP_ViInfoS_GAI);
            }

            memcpy(&g_cfg_isp->vinfos, &g_raw_isp->vinfos, sizeof(g_cfg_isp->vinfos));
        }

        if (p_cmd->cmd_stage & CMD_ISP_DENOISE) {
            memcpy(&g_cfg_isp->dnrcfgs, &g_raw_isp->dnrcfgs, sizeof(g_cfg_isp->dnrcfgs));
        }
        
    }
}

int encode_isp_start()
{
    DBG("encode isp start...\n");
    static struct cmdstat cmdstat_isp; 
    struct cmdstat *ctx = &cmdstat_isp;
    cmdstat_isp.diff_cfg2cmd = diff_cfg2cmd;

    g_run_isp->sch = sch_slow;

    conf_get_video3acfg(&g_raw_isp->v3as);
    conf_get_viinfocfg(&g_raw_isp->vinfos);
    conf_get_denoisecfg(&g_raw_isp->dnrcfgs);
    memcpy(&g_cfg_isp->v3as, &g_raw_isp->v3as, sizeof(g_cfg_isp->v3as));
    memcpy(&g_cfg_isp->vinfos, &g_raw_isp->vinfos, sizeof(g_cfg_isp->vinfos));
    memcpy(&g_cfg_isp->dnrcfgs, &g_raw_isp->dnrcfgs, sizeof(g_cfg_isp->dnrcfgs));

    encode_isp_param(0, ISP_MODE = ISP_DAY, ISP_SCENE_INIT);
    encode_isp_image_setting();

    g_run_isp->p_ctx = ctx;
    attach_config(JEvent_Video3aCfgChg   , cb_event_video3acfg, (void *)ctx);
    attach_config(JEvent_ViinfoCfgChg    , cb_event_videoincfg, (void *)ctx);
    attach_config(JEvent_DnrCfgChg       , cb_event_denoisecfg, (void *)ctx);
    attach_event(JEvent_RunIspColor      , cb_event_ispcolor  , (void *)ctx);

    js_create_timer_r(g_run_isp->sch, ISP_PERIOD, ISP_PERIOD, loop_isp, ctx, &g_run_isp->hdl_loop);

    DBG("---- start isp success ----\n");
    return 0;
}

int encode_isp_stop()
{
    DBG("encode isp stop...\n");

    js_delete_timer_r(&g_run_isp->hdl_loop);

    encode_isp_param(0, ISP_MODE = ISP_DAY, ISP_SCENE_UNINT);

    detach_config(JEvent_Video3aCfgChg   , cb_event_video3acfg, g_run_isp->p_ctx);
    detach_config(JEvent_ViinfoCfgChg    , cb_event_videoincfg, g_run_isp->p_ctx);
    detach_config(JEvent_DnrCfgChg       , cb_event_denoisecfg, g_run_isp->p_ctx);
    detach_event(JEvent_RunIspColor      , cb_event_ispcolor  , g_run_isp->p_ctx);

    DBG("---- stop isp success ----\n");
    return 0;	
}
