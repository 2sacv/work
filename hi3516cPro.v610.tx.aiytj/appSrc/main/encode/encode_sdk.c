#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mount.h>

#include "ot_buffer.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_vb.h"

#include "debug.h"
#include "encodeapi.h"
#include "confapi.h"
#include "conf_list.h"
#include "system_ctrl.h"
#include "jconfstruct.h"
#include "shm_buf_pool.h"
#include "encode_common.h"
#include "encode_sdk.h"
#include "encode_bind.h"
#include "encode_vi.h"
#include "encode_vpss.h"
#include "encode_venc.h"
#include "encode_vi_isp.h"

typedef struct {
    ot_size enc_size[CH_FS_H26X_END];
} venc_param_t;

static td_void encode_sdk_get_default_vb_cfg(ot_vb_cfg *vb_cfg, venc_param_t *enc_param,td_u32 wrap_size)
{
    td_s32 i = 0;
    ot_pic_buf_attr buf_attr = {0};

    (td_void)memset_s(vb_cfg, sizeof(ot_vb_cfg), 0, sizeof(ot_vb_cfg));
    vb_cfg->max_pool_cnt = 128; /* 128 blks */

    //vpss chn 0(main stream) chn 1(sub stream)
    for (i = 0; i < CH_FS_H26X_END; i++) {
        buf_attr.width = enc_param->enc_size[i].width;
        buf_attr.height = enc_param->enc_size[i].height;
        buf_attr.align = OT_DEFAULT_ALIGN;
        buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
        buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
        buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
        if(CH_FS_MAIN0 == i) {
            vb_cfg->common_pool[i].blk_size = wrap_size;//ot_common_get_pic_buf_size(&buf_attr);
            vb_cfg->common_pool[i].blk_cnt = 1;
        } else {
            vb_cfg->common_pool[i].blk_size = ot_common_get_pic_buf_size(&buf_attr);
            vb_cfg->common_pool[i].blk_cnt = 2;
        }
    }

    buf_attr.width = RAW_W;
    buf_attr.height = RAW_H;
    buf_attr.align = OT_DEFAULT_ALIGN;
    buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
    buf_attr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    buf_attr.compress_mode = OT_COMPRESS_MODE_NONE;
    buf_attr.video_format = OT_VIDEO_FORMAT_LINEAR;
    vb_cfg->common_pool[i].blk_size = ot_common_get_pic_buf_size(&buf_attr);
    vb_cfg->common_pool[i].blk_cnt = 3;
}

/* vb init with vb_supplement & MPI system init */
td_s32 encode_sdk_init_vb_supplement(const ot_vb_cfg *vb_conf)
{
    td_s32 ret = 0;
    ot_vb_supplement_cfg supplement_conf = {0};

    ss_mpi_sys_exit();
    ss_mpi_vb_exit();

    do {
        ret = ss_mpi_vb_set_cfg(vb_conf);
        ENCODE_RET_BREAK(ret, "ss_mpi_vb_set_cfg failed\n");

        supplement_conf.supplement_cfg = OT_VB_SUPPLEMENT_JPEG_MASK | OT_VB_SUPPLEMENT_BNR_MOT_MASK;;
        ret = ss_mpi_vb_set_supplement_cfg(&supplement_conf);
        ENCODE_RET_BREAK(ret, "ss_mpi_vb_set_supplement_cfg failed\n");

        ret = ss_mpi_vb_init();
        ENCODE_RET_BREAK(ret, "ss_mpi_vb_init failed\n");

        ret = ss_mpi_sys_init();
        ENCODE_RET_BREAK(ret, "ss_mpi_sys_init failed\n");
    } while(0);

    return ret;
}

td_void encode_sdk_sys_exit(td_void)
{
    ss_mpi_sys_exit();

    ss_mpi_vb_exit();
}

int encode_sdk_init(void)
{
    td_s32 ret = TD_SUCCESS;
    vi_cfg_t vi_cfg = {0};
    vpss_cfg_t vpss_cfg = {0};
    sys_cfg_t  sys_cfg  = {0};

    ot_vb_cfg vb_cfg = {0};
    ot_vi_pipe vi_pipe = 0;
    ot_vpss_grp vpss_grp = 0;

    venc_param_t enc_param = {0};
    VideoEncS video_param = {0};

    sns_type_t sns_type = SC465SL_MIPI_4M_30FPS_12BIT;
    ESensorType eSysCase = system_get_snsr_type();

    conf_get_realvideocfg(&video_param);
    DBG("eSysCase :%d \n",eSysCase);

    do {
        /*step 0:global params init*/
        switch (eSysCase){
        case SENSOR_SC4336P:
            sns_type = SC4336P_MIPI_4M_30FPS_10BIT;
            enc_param.enc_size[0].width = 2560;
            enc_param.enc_size[0].height = 1440;
            enc_param.enc_size[1].width = 640;
            enc_param.enc_size[1].height = 480;
            break;
        case SENSOR_SC465SL:
            sns_type = SC465SL_MIPI_4M_30FPS_12BIT;
            enc_param.enc_size[0].width = 2560;
            enc_param.enc_size[0].height = 1440;
            enc_param.enc_size[1].width = 640;
            enc_param.enc_size[1].height = 480;
            break;
        case SENSOR_SC235:
            sns_type = SC235_MIPI_2M_15FPS_10BIT;
            enc_param.enc_size[0].width = 2304;
            enc_param.enc_size[0].height = 1296;
            enc_param.enc_size[1].width = 640;
            enc_param.enc_size[1].height = 360;
            break;
        default:
            ERR("init sdk : unkown sensor error\n");
            break;
        }

        DBG("sns_type :%d \n",sns_type);

        encode_vi_get_default_vi_cfg(sns_type, &vi_cfg);

        encode_vpss_get_cfg(sns_type, &vpss_cfg, &video_param);

        sys_cfg.route_num = 1;
        sys_cfg.mode_type = OT_VI_OFFLINE_VPSS_ONLINE;
        sys_cfg.vpss_wrap_en = TD_TRUE;
        encode_vpss_wrap_cfg_init(sns_type, &sys_cfg,&vpss_cfg);

        /*step 1:sys init*/
        encode_sdk_get_default_vb_cfg(&vb_cfg, &enc_param, sys_cfg.vpss_wrap_size);
        ret = encode_sdk_init_vb_supplement(&vb_cfg);
        ENCODE_RET_BREAK(ret, "encode_sdk_init_vb_supplement failed\n");

        ret = encode_vi_set_vi_vpss_mode(sys_cfg.mode_type, OT_VI_AIISP_MODE_DEFAULT);
        ENCODE_RET_BREAK(ret, "encode_vi_set_vi_vpss_mode failed\n");

        /*step 2:vi init*/
        ret = encode_vi_init(&vi_cfg);
        ENCODE_RET_BREAK(ret, "encode_vi_init failed\n");

        ret = encode_vi_isp_start(&vi_cfg);
        ENCODE_RET_BREAK(ret, "encode_common_isp_start failed\n");

        /*step 3:vpss init*/
        ret = encode_vpss_init(vpss_grp, &vpss_cfg);
        ENCODE_RET_BREAK(ret, "encode_vpss_init failed\n");

        /*step 4:vi vpss bind*/
        ret = encode_vi_bind_vpss(vi_pipe, 0, vpss_grp, 0);
        ENCODE_RET_BREAK(ret, "encode_vi_bind_vpss failed\n");

        /*step 5:venc init*/
        ret = encode_venc_init(vpss_grp, &video_param);
        ENCODE_RET_BREAK(ret, "encode_venc_init failed\n");

        ot_mpp_version version = {0};
        ss_mpi_sys_get_version(&version);
        DBG("sdk version: %s\n", version.version);

        DBG("init sdk end\n");
    } while(0);

    return ret;
}

int encode_sdk_uninit(void)
{
    ot_vi_pipe vi_pipe = 0;
    ot_vpss_grp vpss_grp = 0;
    vi_cfg_t vi_cfg = {0};
    sns_type_t sns_type = 0;
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

    DBG("eSysCase:%d, sns_type:%d \n",eSysCase, sns_type);

    encode_vi_get_default_vi_cfg(sns_type, &vi_cfg);

    encode_venc_uninit(vpss_grp);

    encode_vi_unbind_vpss(vi_pipe, 0, vpss_grp, 0);

    encode_vpss_uninit(vpss_grp, VPSS_MAX_CHN_NUM);

    encode_vi_isp_stop(&vi_cfg);

    encode_vi_uninit(&vi_cfg);

    encode_sdk_sys_exit();

    uninit_shm_buf_pool();

    umount("/algo");
    DropCache(__func__);
    CompactMemo(__func__);
    UtilSystemCmd("free");

    DBG("uninit sdk end\n");
    return 0;
}
