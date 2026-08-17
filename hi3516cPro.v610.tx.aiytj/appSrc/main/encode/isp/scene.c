#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>
#include <securec.h>

#include "ot_scenecomm.h"
#include "scene_loadparam.h"
#include "ot_scene_setparam.h"
#include "scene.h"

#include "encode_common.h"
#include "debug.h"

ot_scene_param g_scene_param;
ot_scene_video_mode video_mode;

static td_s32 scene_set_video_mode_and_resume(td_u32 mode, const ot_scene_video_mode *video_mode)
{
    int ret;
    ot_scenecomm_expr_true_return(mode >= SCENE_MAX_VIDEOMODE || mode < 0, TD_FAILURE);
    ret = ot_scene_set_scene_mode(&(video_mode->video_mode[mode]));
    if (ret != TD_SUCCESS) {
        ERR("OT_SRDK_SCENEAUTO_Start failed\n");
        return TD_FAILURE;
    }

    ret = ot_scene_pause(TD_FALSE);
    if (ret != TD_SUCCESS) {
        ERR("OT_SCENE_Resume failed\n");
        return TD_FAILURE;
    }
    DBG("The sceneauto is started already.\n");

    return TD_SUCCESS;
}

static td_s32 scene_create_para_and_set_mode( td_u32 mode, const td_char *dir_name,
    ot_scene_video_mode *video_mode)
{
    int ret;
    ret = ot_scene_create_param(dir_name, &g_scene_param, video_mode);
    if (ret != TD_SUCCESS) {
        ERR("SCENETOOL_CreateParam failed\n");
        return TD_FAILURE;
    }

    ret = ot_scene_init(&g_scene_param);
    if (ret != TD_SUCCESS) {
        ERR("ot_scene_init failed\n");
        return TD_FAILURE;
    }

    scene_set_video_mode_and_resume(mode,(const ot_scene_video_mode *)video_mode);
    DBG("The sceneauto is created && started already.\n");
    return TD_SUCCESS;
}

td_s32 scene_main(td_char qp_file[], int mode,int choice )
{
    td_s32 ret;

    set_dir_name(qp_file);

    switch (choice) {
        case 1: /* user input 1 */
            ret = scene_create_para_and_set_mode(mode, qp_file, &video_mode);
            break;
        case 2: /* user input 3 */
            ret  = ot_scene_pause(TD_TRUE);
            ret += scene_set_video_mode_and_resume(mode, &video_mode);
            break;
        case 3: /* user input 4 */
            ret = ot_scene_deinit();
            break;
        default:
            ret = TD_SUCCESS;
            ERR("unknown input\n");
            break;
    }
    sleep(1);
    ENCODE_RET_CHECK(ret, "choice %d failed with %#x\n", choice, ret);

    return TD_SUCCESS;
}
