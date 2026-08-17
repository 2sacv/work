
#ifdef PLATFORM_TENCENT

#ifndef __TENCENT_HTTP_SERVICE_H__
#define __TENCENT_HTTP_SERCICE_H__
#include "sim4g.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cpluscplus */
/*
    添加新接口说明:
    1. .c中新增方法 report_***(void *info){}
    2. .h中新增声明 report_***(void *info);
    3. 组装body时,构建sFieldMap结构体
    4. post时, 构建sPostMsg结构体
    5. 在eMethedType中增加E_***
    6. post回复函数命名规范：hdl_***_reply
*/

#define HEAD_MAX_SIZE 1024
#define BODY_MAX_SIZE 2048

// 定义字段类型枚举
typedef enum {
    E_FIELD_STRING,
    E_FIELD_INT
} eFieldType;

// 定义键值对结构体
typedef struct {
    const char *key;
    eFieldType type;
    void *val;
} sFieldMap;

// 定义方法类型枚举
typedef enum {
    E_ACTION_4G_AIRBURN,
    E_ACTION_DEV_BIND,
    E_ACTION_DEV_UNBIND,
    E_ACTION_SIM4G,
    E_ACTION_4G_LOCATION,
    E_ACTION_PRIVCTRL,
    E_ACTION_TWECALL,
    E_ACTION_ABNORMAL,
    E_ACTION_SDSTAT,
} eActionType;

typedef struct {
    char head_buf[HEAD_MAX_SIZE];
    char body_buf[BODY_MAX_SIZE];
    const char *host;
    const char *method;
    int port;
    int timeoutms;
    int action;
} sPostMsg;

int report_4g_airburn(sim_4g_t *sim4g_info, sBurnArg *dev);

int report_dev_bind(char *token);

int report_dev_unbind();

int report_twecall();

int report_4g_info(sim_4g_t * info);

int report_4g_location(Sim4g *sim4g_info);

int report_privctrl(priv_ctrl_t info);

int report_exception_record(int type, char *old_value, char *new_value);

int report_sd_stat();

#ifdef __cplusplus
}
#endif /* __cpluscplus */

#endif /* __TENCENT_HTTP_SERVICE_H__ */
#endif //PLATFORM_TENCENT

