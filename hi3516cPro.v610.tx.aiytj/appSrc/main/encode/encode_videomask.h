#ifndef __ENCODE_VIDEOMASK_H__
#define __ENCODE_VIDEOMASK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cpluscplus */

#define VIDEOMASK_SWITCH_OFF                "videomaskcfg -act set -maskid 3  -color 0  -left 0 -top 0 -right 0 -bottom 0 -masken 0"
#define VIDEOMASK_SWITCH_ON                 "videomaskcfg -act set -maskid 3  -color 0  -left 0 -top 0 -right 1919 -bottom 1079 -masken 1"
#define VIDEOMASK_EN                        "Lens Masking"
#define VIDEOMASK_CN                        "视频遮蔽中"  //注意必须按utf-8格式保存，不能用GB字体保存，否则最后一个字符会显示不出来
#define VIDEOMASK_JA                        "プライバシーモード"
#define VIDEOMASK_LENS_BOTTOM               "pelcod20ctrl -type 1 -cmd 2 -data2 63"
#define ID_VIDEO_MASK                       (3)

enum {
    CMD_VIDEOMASK = 1 << 0,
    CMD_VM_ENABLE = 1 << 1,
};

struct videomask_cfg {
    VideoMaskS vm;
    videomask_plan_t vm_plan;
};

struct videomask_run {
    int status;                //隐私遮挡有没有真实开启标志位
    JSTCHandle hdl_videomask;
    JSScheduler sch_videomask;
    struct cmdstat *ctx;
};

int videomask_init(void);
int videomask_uninit(void);
int videomask_enabled(void);

#ifdef __cplusplus
}
#endif /* __cpluscplus */

#endif
