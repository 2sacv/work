/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_main.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2024-11-22
    Description  :
    History      :
                        created by tangjianxue
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mount.h>

#include "confapi.h"
#include "conf_list.h"
#include "debug.h"
#include "js_scheduler.h"
#include "system_ctrl.h"

#include "encode_main.h"
#include "encode_jpeg.h"
#include "encode_typedef.h"
#include "encode_common.h"
#include "encode_audio.h"
#include "encode_video.h"
#include "encode_osd.h"
#include "encode_base_ivx.h"
#include "encode_ivp_aidetect.h"
#include "encode_sdk.h"
#include "encode_isp.h"
#include "encode_venc.h"
#include "system_sch.h"
#include "encode_jpeg.h"

static BOOL bUninited = FALSE;
static BOOL bUninitedFinish = FALSE;
static BOOL binitedFinish = FALSE;

int encode_get_init_status()
{
    return binitedFinish;
}

int init_encode_server(void)
{
    UtilSystemCmd("echo 3 > /proc/sys/vm/drop_caches");

    encode_sdk_init();

    encode_video_init();

    encode_loop_jpeg_start();

    encode_isp_start();

    encode_venc_frame_strategy();

    encode_osd_init();

    encode_ivx_init();

    bUninited = FALSE;
    binitedFinish = TRUE;
    DBG("encode init end\n");
    return 0;
}

static void uninit_encode_server(void *data)
{
    if (bUninited) {
        return;
    }
    bUninited = TRUE;
    binitedFinish = FALSE;

    DBG("-----------uninit encode servers start\n");

    encode_ivx_uninit();

    encode_osd_uninit();

    encode_audio_uninit();

    encode_isp_stop();

    encode_loop_jpeg_stop();

    encode_video_uninit();

    encode_sdk_uninit();

    printf("___ exit /algo ___\n");
    umount("/algo");
    CompactMemo(__func__);
    UtilSystemCmd("free");

    bUninitedFinish = TRUE;

    DBG("encode uninit end\n");
    return;
}

int uninit_encode_background()
{
    return js_run_function(sch_slow, uninit_encode_server, NULL, 0);
}

int uninit_encode_wait()
{
    int nRetry = 20;
    do {
        SYSLOG("encode wait for uninit %d...\n", nRetry);
        sleep(1);
    } while (!bUninitedFinish && 0 < nRetry--);

    DropCache(__func__);
    CompactMemo(__func__);
    UtilSystemCmd("free");

    return 0;
}

int uninit_encode_done()
{
    return bUninitedFinish;
}

