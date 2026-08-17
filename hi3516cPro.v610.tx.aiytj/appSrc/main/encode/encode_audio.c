#include "ot_type.h"
#include "ot_common_aio.h"
#include "ot_vqe_register.h"
#include "ss_mpi_audio.h"

#include "debug.h"
#include "encode_common.h"
#include "encode_audio_queue.h"
#include "encode_audio_input.h"
#include "encode_audio_output.h"
#include "encode_audio.h"
#include "encode_aac.h"
#include "encode_cry.h"

static td_s32 encode_audio_register_vqe_module(td_void)
{
    td_s32 ret = TD_SUCCESS;
    ot_audio_vqe_register vqe_reg_cfg = {0};

    /* RecordVQE */
    //vqe_reg_cfg.record_mod_cfg.handle = ot_vqe_record_get_handle();

    /* Resample */
    vqe_reg_cfg.resample_mod_cfg.handle = ot_vqe_resample_get_handle();

    /* TalkVQE */
    vqe_reg_cfg.hpf_mod_cfg.handle = ot_vqe_hpf_get_handle();
    vqe_reg_cfg.aec_mod_cfg.handle = ot_vqe_aec_get_handle();
    vqe_reg_cfg.agc_mod_cfg.handle = ot_vqe_agc_get_handle();
    vqe_reg_cfg.anr_mod_cfg.handle = ot_vqe_anr_get_handle();
    vqe_reg_cfg.eq_mod_cfg.handle = ot_vqe_eq_get_handle();

    /* TalkV2VQE */
    vqe_reg_cfg.talkv2_mod_cfg.handle = ot_vqe_talkv2_get_handle();

    ret = ss_mpi_audio_register_vqe_mod(&vqe_reg_cfg);
    ENCODE_RET_CHECK(ret, "ss_mpi_audio_register_vqe_mod failed!\n");

    return ret;
}

int encode_audio_sys_init(void)
{
    td_s32 ret = TD_SUCCESS;
    static int init = FALSE;
    ot_audio_mod_param audio_mod_param = {0};

    do {
        ret = ss_mpi_audio_exit();
        ENCODE_RET_BREAK(ret, "ss_mpi_audio_exit failed!\n");

        if(!init) {
            init = TRUE;
            ret = encode_audio_register_vqe_module();
            ENCODE_RET_BREAK(ret, "encode_audio_register_vqe_module failed!\n");
        }

        ret = ss_mpi_audio_init();
        ENCODE_RET_BREAK(ret, "ss_mpi_audio_init failed!\n");

        ret = ss_mpi_audio_get_mod_param(&audio_mod_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_audio_get_mod_param failed!\n");

        audio_mod_param.clk_select = OT_AUDIO_CLK_SELECT_BASE;
        /* set audio mode parameter. */
        ret = ss_mpi_audio_set_mod_param(&audio_mod_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_audio_set_mod_param failed!\n");
    } while(0);

    return ret;
}

int encode_audio_sys_uninit(void)
{
    ss_mpi_audio_exit();

    return TD_SUCCESS;
}

int encode_audio_init(void)
{
    encode_audio_sys_init();

    encode_audio_queue_start();

    aac_encode_init();

    encode_audio_in_init();

    encode_audio_out_init();

    encode_crydet_start();

    return TD_SUCCESS;
}

int encode_audio_uninit(void)
{
    encode_crydet_stop();

    encode_audio_queue_stop();

    encode_audio_out_uninit();

    encode_audio_in_uninit();

    aac_encode_uninit();

    encode_audio_sys_uninit();

    return TD_SUCCESS;
}

void amplify_pcm_volume(td_u8 *data, td_u32 bytes_data,
                        ot_audio_bit_width bitwidth, float multiplier)
{
    return_if_fail(NULL != data);
    return_if_fail(bytes_data > 0);

    int32_t samples = 0;
    int32_t idx = 0, smp_point_amped = 0;
    int8_t *pcm_i8 = (int8_t *)data;
    int16_t *pcm_i16 = (int16_t *)data;

    switch (bitwidth) {
    case OT_AUDIO_BIT_WIDTH_8: {
        samples = bytes_data;
        for (idx = 0; idx < samples; idx++) {
            smp_point_amped = RANGE((int32_t)(pcm_i8[idx] * multiplier), INT8_MIN, INT8_MAX);
            pcm_i8[idx] = (int8_t)smp_point_amped;
        }
        break;
    }
    case OT_AUDIO_BIT_WIDTH_16: {
        samples = bytes_data / 2;
        for (idx = 0; idx < samples; idx++) {
            smp_point_amped = RANGE((int32_t)(pcm_i16[idx] * multiplier), INT16_MIN, INT16_MAX);
            pcm_i16[idx] = (int16_t)smp_point_amped;
        }
        break;
    }
    default: {
        ERR("not supported bitwidth %d\n", bitwidth);
        break;
    }
    }
}

