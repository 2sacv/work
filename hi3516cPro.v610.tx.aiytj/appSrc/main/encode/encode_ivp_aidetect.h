#ifndef  __ENCODE_IVP_AIDETECT_H__
#define  __ENCODE_IVP_AIDETECT_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "encode_base_ivx.h"
#include "ot_common_aidetect.h"

#define INTV_MS_CLR_OSD (100)
#define IVX_LOOP_TIME   (50)   // ivs 定时器 loop 时间
#define CNT_RECTS_MAX   (4)

typedef struct {
    ot_aidetect_object obj_update[CNT_RECTS_MAX];
    ot_aidetect_object obj_new[CNT_RECTS_MAX];
    int update_num;
    int new_num;
} obj_filter_t;

typedef enum {
    E_SMARTAE_STATUS1   = 1,
    E_SMARTAE_STATUS2   = 2,
    E_SMARTAE_STATUS3   = 3,
    E_SMARTAE_STATUS4   = 4,
} eSmartAeStatus;

int get_ae_force();
int get_facial_convergence_status();

int encode_ivp_aidetect_set_param(struct ivx_cfg* ivx_info, struct ivx_run* ivx_run);
int encode_ivp_aidetect_process(struct ivx_cfg* ivx_info);

int encode_ivp_aidetect_init(struct ivx_cfg* ivx_info, struct ivx_run* ivx_run);
int encode_ivp_aidetect_uninit();
void send_alarm_for_hour(void);

void init_encode_ivp_aidetect_cfg(void);

#ifdef __cplusplus
}

#endif
#endif
