#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "ot_common_video.h"
#include "ot_common_3a.h"
#include "ot_common_awb.h"
#include "ot_common.h"
#include "securec.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_awb.h"
#include "ss_mpi_isp.h"

#include "debug.h"
#include "system_ctrl.h"
#include "encode_common.h"
#include "encode_vi_isp.h"

static pthread_t g_isp_pid[OT_VI_MAX_PIPE_NUM] = {0};

static ot_isp_pub_attr g_isp_pub_attr_sc4336p_mipi_4m_30fps = {
    { 0, 0, 2560, 1440 },
    { 2560, 1440 },
    30,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        { 0, 0, 2560, 1440 },
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc465sl_mipi_4m_30fps = {
    { 0, 0, 2560, 1440 },
    { 2560, 1440 },
    ENCODE_SENSOR0_FRAME_RATE,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        { 0, 0, 2560, 1440 },
    },
};

static ot_isp_pub_attr g_isp_pub_attr_sc235_mipi_2m_15fps = {
    { 0, 0, 1920, 1080 },
    { 1920, 1080 },
    ENCODE_SENSOR0_FRAME_RATE,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
    0,
    0,
    {
        0,
        { 0, 0, 2304, 1296 },
    },
};

static td_s32 encode_vi_isp_get_wdr_num(ot_wdr_mode wdr_mode)
{
    td_u32 pipe_num = 1;
    switch (wdr_mode) {
        case OT_WDR_MODE_NONE:
        case OT_WDR_MODE_BUILT_IN:
            pipe_num = 1;
            break;
        case OT_WDR_MODE_2To1_LINE:
        case OT_WDR_MODE_2To1_FRAME:
            pipe_num = 2; /* WDR 2 pipe */
            break;
        case OT_WDR_MODE_3To1_LINE:
            pipe_num = 3; /* WDR 3 pipe */
            break;
        default:
            break;
    }
    return pipe_num;
}

ot_isp_sns_obj *encode_vi_isp_get_sns_obj(sns_type_t sns_type)
{
    switch (sns_type) {
#ifdef SC4336P_MIPI_4M_30FPS_10BIT_SELECT
        case SC4336P_MIPI_4M_30FPS_10BIT:
            return &g_sns_sc4336p_obj;
#endif

#ifdef SC465SL_MIPI_4M_30FPS_12BIT_SELECT
        case SC465SL_MIPI_4M_30FPS_12BIT:
            return &g_sns_sc465sl_obj;
#endif

#ifdef SC235_MIPI_2M_SELECT
        case SC235_MIPI_2M_15FPS_10BIT:
            return &g_sns_sc235hai_obj;
#endif

        default:
            return TD_NULL;
    }
}

td_s32 encode_vi_isp_sensor_regiter_callback(ot_isp_dev isp_dev, sns_type_t sns_type)
{
    td_s32 ret = -1;
    ot_isp_3a_alg_lib ae_lib = {0};
    ot_isp_3a_alg_lib thermo_lib = {0};
    ot_isp_3a_alg_lib awb_lib = {0};
    ot_isp_sns_obj *sns_obj = NULL;

    do {
        sns_obj = encode_vi_isp_get_sns_obj(sns_type);
        ENCODE_NULL_BREAK(sns_obj);

        ae_lib.id = isp_dev;
        thermo_lib.id = isp_dev;
        awb_lib.id = isp_dev;
        ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        ret = strncpy_s(thermo_lib.lib_name, sizeof(thermo_lib.lib_name), OT_THERMO_LIB_NAME, sizeof(OT_THERMO_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        if (sns_obj->pfn_register_callback != TD_NULL) {
            ret = sns_obj->pfn_register_callback(isp_dev, &ae_lib, &awb_lib);
            ENCODE_RET_BREAK(ret, "pfn_register_callback failed\n");
        } else {
            ERR("sensor_register_callback failed with TD_NULL!\n");
        }
    }while(0);

    return ret;
}

ot_isp_sns_type encode_vi_get_sns_bus_type(sns_type_t sns_type)
{
    ot_unused(sns_type);
    return OT_ISP_SNS_TYPE_I2C;
}

td_s32 encode_vi_isp_bind_sns(ot_isp_dev isp_dev, sns_type_t sns_type, td_s8 sns_dev)
{
    ot_isp_sns_commbus sns_bus_info = {0};
    ot_isp_sns_type    bus_type = OT_ISP_SNS_TYPE_I2C;
    ot_isp_sns_obj    *sns_obj = NULL;
    td_s32 ret = -1;

    do {
        sns_obj = encode_vi_isp_get_sns_obj(sns_type);
        ENCODE_NULL_BREAK(sns_obj);

        bus_type = encode_vi_get_sns_bus_type(sns_type);
        if (bus_type == OT_ISP_SNS_TYPE_I2C) {
            sns_bus_info.i2c_dev = sns_dev;
        } else {
            sns_bus_info.ssp_dev.bit4_ssp_dev = sns_dev;
            sns_bus_info.ssp_dev.bit4_ssp_cs  = 0;
        }

        if (sns_obj->pfn_set_bus_info != TD_NULL) {
            ret = sns_obj->pfn_set_bus_info(isp_dev, sns_bus_info);
            ENCODE_RET_BREAK(ret, "pfn_set_bus_info failed\n");
        } else {
            ERR("not support set sensor bus info!\n");
            break;
        }
    } while(0);

    return ret;
}

td_s32 encode_vi_isp_ae_lib_callback(ot_isp_dev isp_dev)
{
    td_s32 ret = 0;
    ot_isp_3a_alg_lib ae_lib = {0};

    do {
        ae_lib.id = isp_dev;
        ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        ret = ss_mpi_ae_register(isp_dev, &ae_lib);
        ENCODE_RET_BREAK(ret, "ss_mpi_ae_register failed\n");
    } while(0);

    return ret ;
}

td_s32 encode_vi_isp_awb_lib_callback(ot_isp_dev isp_dev)
{
    td_s32 ret = 0;
    ot_isp_3a_alg_lib awb_lib = {0};

    do {
        awb_lib.id = isp_dev;
        ret = strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        ret = ss_mpi_awb_register(isp_dev, &awb_lib);
        ENCODE_RET_BREAK(ret, "ss_mpi_awb_register failed\n");
    } while(0);

    return ret;
}

static td_s32 encode_vi_register_sensor_lib(ot_vi_pipe vi_pipe, td_u8 pipe_index, const vi_cfg_t *vi_cfg)
{
    td_s32 ret = 0;
    td_u32 bus_id = 0;
    sns_type_t sns_type = vi_cfg->sns_info.sns_type;

    do {
        ret = encode_vi_isp_sensor_regiter_callback(vi_pipe, sns_type);
        ENCODE_RET_BREAK(ret, "encode_vi_isp_sensor_regiter_callback failed\n");

        if (pipe_index > 0) {
            bus_id = -1;
        } else {
            bus_id = vi_cfg->sns_info.bus_id;
        }

        ret = encode_vi_isp_bind_sns(vi_pipe, sns_type, bus_id);
        ENCODE_RET_BREAK(ret, "encode_v_isp_bind_sns failed\n");

        ret = encode_vi_isp_ae_lib_callback(vi_pipe);
        ENCODE_RET_BREAK(ret, "encode_v_isp_ae_lib_callback failed\n");

        ret = encode_vi_isp_awb_lib_callback(vi_pipe);
        ENCODE_RET_BREAK(ret, "encode_v_isp_awb_lib_callback failed\n");
    } while(0);

    return ret;
}

static td_void encode_vi_set_isp_ctrl_param(ot_vi_pipe vi_pipe, td_bool is_isp_be_end_trigger,
    td_bool is_isp_quick_start)
{
    int ret = TD_SUCCESS;
    ot_isp_ctrl_param isp_ctrl_param = {0};

    do {
        ret = ss_mpi_isp_get_ctrl_param(vi_pipe, &isp_ctrl_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_ctrl_param failed\n");

        isp_ctrl_param.be_buf_num = 3;
        isp_ctrl_param.isp_run_wakeup_select = (is_isp_be_end_trigger) ?
        OT_ISP_RUN_WAKEUP_BE_END : OT_ISP_RUN_WAKEUP_FE_START;
        isp_ctrl_param.quick_start_en = is_isp_quick_start;

        ret = ss_mpi_isp_set_ctrl_param(vi_pipe, &isp_ctrl_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_ctrl_param failed\n");
    } while(0);
}

static void *encode_vi_isp_run_thread(td_void *param)
{
    td_s32 ret = 0;
    ot_isp_dev isp_dev = 0;
    errno_t err;
    td_char thread_name[20] = {0};

    do {
        err = snprintf_s(thread_name, sizeof(thread_name), sizeof(thread_name) - 1, "ISP%d_RUN", isp_dev); /* 20,19 chars */
        if (err < 0) {
            break;
        }
        prctl(PR_SET_NAME, thread_name, 0, 0, 0);

        isp_dev = (ot_isp_dev)(td_uintptr_t)param;

        DBG("ISP Dev %d running !\n", isp_dev);
        ret = ss_mpi_isp_run(isp_dev);
        if (ret != TD_SUCCESS) {
            ERR("OT_MPI_ISP_Run failed with %#x!\n", ret);
            break;
        }
    } while(0);

    return NULL;
}

td_s32 encode_vi_isp_run(ot_isp_dev isp_dev)
{
    td_s32 ret;
    pthread_attr_t *thread_attr = NULL;

    ret = pthread_create(&g_isp_pid[isp_dev], thread_attr, encode_vi_isp_run_thread, (td_void*)(td_uintptr_t)isp_dev);
    if (ret != 0) {
        ERR("create isp running thread failed!, error: %d\r\n", ret);
    }

    return ret;
}

td_void encode_vi_isp_run_stop(ot_isp_dev isp_dev)
{
    if (g_isp_pid[isp_dev]) {
        pthread_join(g_isp_pid[isp_dev], NULL);
        g_isp_pid[isp_dev] = 0;
        DBG("encode_common_isp_run_stop success isp_dev:%d\n", isp_dev);
    }
}

static td_s32 encode_one_pipe_isp_start(ot_vi_pipe vi_pipe, td_u8 pipe_index, const vi_cfg_t *vi_cfg)
{
    td_s32 ret = 0;

    do {
        ret = encode_vi_register_sensor_lib(vi_pipe, pipe_index, vi_cfg);
        ENCODE_RET_BREAK(ret, "encode_common_register_sensor_lib failed\n");

        encode_vi_set_isp_ctrl_param(vi_pipe, vi_cfg->pipe_info[pipe_index].isp_be_end_trigger,
        vi_cfg->pipe_info[pipe_index].isp_quick_start);

        ret = ss_mpi_isp_mem_init(vi_pipe);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_mem_init failed\n");

        ret = ss_mpi_isp_set_pub_attr(vi_pipe, &vi_cfg->pipe_info[pipe_index].isp_info);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_pub_attr failed\n");

        ret = ss_mpi_isp_init(vi_pipe);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_init failed\n");

        if ((vi_pipe < OT_VI_MAX_PHYS_PIPE_NUM ||
        (vi_cfg->pipe_info[pipe_index].isp_be_end_trigger == TD_TRUE && vi_pipe < OT_VI_MAX_PIPE_NUM)) &&
        (vi_cfg->pipe_info[pipe_index].isp_need_run == TD_TRUE)) {
            ret = encode_vi_isp_run(vi_pipe);
            ENCODE_RET_BREAK(ret, "encode_vi_isp_run failed\n");
        }
    } while(0);

    return ret;
}

int encode_vi_isp_start(const vi_cfg_t *vi_cfg)
{
    td_s8 i = 0, j = 0;
    int ret = 0;
    ot_vi_pipe vi_pipe = 0;
    ot_wdr_mode wdr_mode = vi_cfg->grp_info.fusion_grp_attr[0].wdr_mode;
    td_bool pipe_reverse = vi_cfg->grp_info.fusion_grp_attr[0].pipe_reverse;
    const ot_vi_pipe *pipe_id = vi_cfg->bind_pipe.pipe_id;

    for (i = 0; i < (td_u8)vi_cfg->bind_pipe.pipe_num; i++) {
        vi_pipe = pipe_reverse ? pipe_id[vi_cfg->bind_pipe.pipe_num - 1 - i] : pipe_id[i];

        if (vi_cfg->pipe_info[i].pipe_attr.isp_bypass == TD_TRUE) {
            continue;
        }

        if ((wdr_mode != OT_WDR_MODE_NONE) && (wdr_mode != OT_WDR_MODE_BUILT_IN) &&
            (i > 0) && (i < encode_vi_isp_get_wdr_num(wdr_mode))) {
            continue;
        }

        ret = encode_one_pipe_isp_start(vi_pipe, i, vi_cfg);
        if (ret != TD_SUCCESS) {
            for (j = i - 1; (j >= 0) && (i != 0); j--) {
                vi_pipe = pipe_reverse ? pipe_id[vi_cfg->bind_pipe.pipe_num - 1 - j] : pipe_id[j];
            }
            return ret;
        }
    }

    return TD_SUCCESS;
}


td_s32 encode_vi_isp_awb_lib_uncallback(ot_isp_dev isp_dev)
{
    td_s32 ret = 0;
    ot_isp_3a_alg_lib awb_lib = {0};

    do {
        awb_lib.id = isp_dev;
        ret = strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");
        ret = ss_mpi_awb_unregister(isp_dev, &awb_lib);
        ENCODE_RET_BREAK(ret, "ss_mpi_awb_unregister failed\n");
    } while(0);

    return ret;
}

td_s32 encode_vi_isp_ae_lib_uncallback(ot_isp_dev isp_dev)
{
    td_s32 ret = 0;
    ot_isp_3a_alg_lib ae_lib = {0};

    do {
        ae_lib.id = isp_dev;
        ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        ret = ss_mpi_ae_unregister(isp_dev, &ae_lib);
        ENCODE_RET_BREAK(ret, "ss_mpi_ae_unregister failed\n");
    } while(0);

    return ret;
}

td_s32 encode_vi_isp_sensor_unregiter_callback(ot_isp_dev isp_dev, const vi_cfg_t *vi_cfg)
{
    ot_isp_3a_alg_lib ae_lib = {0};
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
    ot_isp_3a_alg_lib thermo_lib = {0};
#endif
    ot_isp_3a_alg_lib awb_lib = {0};
    ot_isp_sns_obj *sns_obj = NULL;
    td_s32 ret = 0;

    do {
        sns_obj = encode_vi_isp_get_sns_obj(vi_cfg->sns_info.sns_type);
        ENCODE_NULL_BREAK(sns_obj);

        ae_lib.id = isp_dev;
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
        thermo_lib.id = isp_dev;
#endif
        awb_lib.id = isp_dev;
        ret = strncpy_s(ae_lib.lib_name, sizeof(ae_lib.lib_name), OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");
#ifdef CONFIG_OT_ISP_THERMO_SUPPORT
        ret = strncpy_s(thermo_lib.lib_name, sizeof(thermo_lib.lib_name), OT_THERMO_LIB_NAME, sizeof(OT_THERMO_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");
#endif
        ret = strncpy_s(awb_lib.lib_name, sizeof(awb_lib.lib_name), OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
        ENCODE_RET_BREAK(ret, "strncpy_s failed\n");

        if (sns_obj->pfn_un_register_callback != TD_NULL) {
            ret = sns_obj->pfn_un_register_callback(isp_dev, &ae_lib, &awb_lib);
            ENCODE_RET_BREAK(ret, "pfn_un_register_callback %d failed\n", isp_dev);
        } else {
            ERR("sensor_unregister_callback failed with TD_NULL!\n");
            break;
        }
    }while(0);

    return ret;
}

static td_void encode_vi_isp_deregister_sensor_lib(ot_vi_pipe vi_pipe, const vi_cfg_t *vi_cfg)
{
    // ISP use it
    encode_vi_isp_awb_lib_uncallback(vi_pipe);

    encode_vi_isp_ae_lib_uncallback(vi_pipe);

    encode_vi_isp_sensor_unregiter_callback(vi_pipe, vi_cfg);
}

static td_void encode_one_pipe_isp_stop(ot_vi_pipe vi_pipe, const vi_cfg_t *vi_cfg)
{
    ss_mpi_isp_exit(vi_pipe);

    encode_vi_isp_run_stop(vi_pipe);

    encode_vi_isp_deregister_sensor_lib(vi_pipe, vi_cfg);
}

void encode_vi_isp_stop(const vi_cfg_t *vi_cfg)
{
    td_u32     i = 0;
    td_bool    start_pipe = 0;
    ot_vi_pipe vi_pipe = 0;
    td_bool pipe_reverse = vi_cfg->grp_info.fusion_grp_attr[0].pipe_reverse;
    const ot_vi_pipe *pipe_id = vi_cfg->bind_pipe.pipe_id;

    for (i = 0; i < vi_cfg->bind_pipe.pipe_num; i++) {
        if (vi_cfg->pipe_info[i].pipe_attr.isp_bypass == TD_TRUE) {
            continue;
        }

        if ((vi_cfg->pipe_info[i].isp_info.wdr_mode == OT_WDR_MODE_NONE) ||
            (vi_cfg->pipe_info[i].isp_info.wdr_mode == OT_WDR_MODE_BUILT_IN)) {
            start_pipe = TD_TRUE;
        } else {
            start_pipe = (i > 0) ? TD_FALSE : TD_TRUE;
        }

        if (start_pipe != TD_TRUE) {
            continue;
        }

        vi_pipe = pipe_reverse ? pipe_id[vi_cfg->bind_pipe.pipe_num - 1 - i] : pipe_id[i];
        encode_one_pipe_isp_stop(vi_pipe, vi_cfg);
    }
}

int encode_vi_isp_get_pub_attr_by_sns(sns_type_t sns_type, ot_isp_pub_attr *pub_attr)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc4336p_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;

        case SC465SL_MIPI_4M_30FPS_12BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc465sl_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;

        case SC235_MIPI_2M_15FPS_10BIT:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc235_mipi_2m_15fps, sizeof(ot_isp_pub_attr));
            break;

        default:
            (td_void)memcpy_s(pub_attr, sizeof(ot_isp_pub_attr),
                &g_isp_pub_attr_sc4336p_mipi_4m_30fps, sizeof(ot_isp_pub_attr));
            break;
    }

    return TD_SUCCESS;
}
