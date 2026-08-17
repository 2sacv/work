#ifndef __JABSCO_XTM5809_H__
#define __JABSCO_XTM5809_H__
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void init_xtm5809(void);

void motor_stop(void);

void step_motor(int motor_no,uint32_t step);
void ircut_send_data(void);
void ir_cut(int cut_type);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
