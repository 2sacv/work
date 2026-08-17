/*
 *       Filename:  aliyun_mmi_http.h
 *    Description:  阿里云百炼 MMI HTTP 透传接口（全托管模式）
 *                  SDK 生成完整加密字符串 → HTTP POST raw → 响应字符串交 SDK 解析
 *        Version:  1.0
 *        Created:  07/10/2026 18:17:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 *
 *   接口列表:
 *     1. 设备注册   POST /api/device/v1/register
 *     2. 获取令牌   POST /api/token/v1/getToken
 *
 *   参考文档: https://help.aliyun.com/zh/model-studio/mmi-rtos-sdk
 */

#ifndef __ALIYUN_MMI_HTTP_H__
#define __ALIYUN_MMI_HTTP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  常量定义
 * ================================================================ */

#define ALIYUN_MMI_HOST             "bailian.multimodalagent.aliyuncs.com"
#define ALIYUN_MMI_PORT             443

#define ALIYUN_REGISTER_PATH        "/api/device/v1/register"
#define ALIYUN_GET_TOKEN_PATH       "/api/token/v1/getToken"

#define ALIYUN_MMI_HTTP_RSP_MAX     (4096)
#define ALIYUN_MMI_HTTP_DEFAULT_TO   (10000)  /* 默认超时 10s */

/* ================================================================
 *  错误码枚举（云端返回 + 本地扩展）
 * ================================================================ */

typedef enum {
    E_ALIYUN_ERR_SUCCESS           = 0,
    E_ALIYUN_ERR_NO_QUOTA          = 100007,
    E_ALIYUN_ERR_DEV_REGISTERED    = 100008,
    E_ALIYUN_ERR_DEV_NOT_REG       = 100009,
    E_ALIYUN_ERR_TIME_EXPIRED      = 100010,
    E_ALIYUN_ERR_SIGN_EMPTY        = 100011,
    E_ALIYUN_ERR_DEV_NAME_LEN      = 100012,
    E_ALIYUN_ERR_DEV_NAME_INVALID  = 100013,
    E_ALIYUN_ERR_DEV_DISABLED      = 100023,
    E_ALIYUN_ERR_DEV_ACTIVATED     = 100025,
    E_ALIYUN_ERR_MODE_MISMATCH     = 100032,
    E_ALIYUN_ERR_ACTIVATE_EXPIRED  = 100033,
    E_ALIYUN_ERR_QUOTA_EXHAUSTED   = 200014,
    E_ALIYUN_ERR_DAILY_LIMIT       = 200021,
    E_ALIYUN_ERR_QUOTA_EXPIRED     = 200022,
    E_ALIYUN_ERR_GATEWAY_FAIL      = 300002,
    E_ALIYUN_ERR_ENCRYPT_MISMATCH  = 500001,
    E_ALIYUN_ERR_DECRYPT_FAIL      = 500002,
    E_ALIYUN_ERR_HTTP_FAIL         = 900001,
    E_ALIYUN_ERR_JSON_PARSE_FAIL   = 900002,
} eAliyunMmiErrCode;

/* ================================================================
 *  函数声明
 * ================================================================ */

/**
 * @brief 错误码转可读字符串
 */
const char *aliyun_mmi_err_str(int err_code);

/**
 * @brief 通用 HTTPS POST（透传模式）
 * @param host      目标主机名
 * @param port      端口号
 * @param path      请求路径（如 /api/device/v1/register）
 * @param body      请求体（SDK 生成的完整加密字符串）
 * @param body_len  请求体长度
 * @param rsp_buf   响应缓冲区（调用方提供）
 * @param rsp_size  响应缓冲区大小
 * @param timeout_ms 超时（毫秒）
 * @return SUCCESS/FAILURE
 */
int aliyun_mmi_http_post_raw(const char *host, int port, const char *path,
                             const char *body, int body_len,
                             char *rsp_buf, int rsp_size, int timeout_ms);

/**
 * @brief 设备注册透传
 *        将 c_license_gen_register_str() 生成的字符串 POST 到云端
 * @param reg_str  SDK 生成的注册字符串
 * @param rsp_buf  响应缓冲区
 * @param rsp_size 响应缓冲区大小
 * @return SUCCESS/FAILURE
 */
int aliyun_mmi_register_raw(const char *reg_str, char *rsp_buf, int rsp_size);

/**
 * @brief 获取令牌透传
 *        将 c_license_gen_get_token_str() 生成的字符串 POST 到云端
 * @param token_str SDK 生成的令牌请求字符串
 * @param rsp_buf   响应缓冲区
 * @param rsp_size  响应缓冲区大小
 * @return SUCCESS/FAILURE
 */
int aliyun_mmi_get_token_raw(const char *token_str, char *rsp_buf, int rsp_size);

/**
 * @brief 从全托管 API 信封 JSON 中提取 "data" 字段
 *        格式: {"code":200,"data":{...}} → 返回 "{...}"（malloc, 调用方 free）
 *        若解析失败或无 data 字段，返回原字符串副本
 */
char *aliyun_mmi_extract_data_json(const char *rsp_json);

/**
 * @brief 读取 API 信封中的 "code" 字段
 * @return code 值，解析失败返回 -1
 */
int aliyun_mmi_get_json_code(const char *rsp_json);

#ifdef __cplusplus
}
#endif
#endif /* __ALIYUN_MMI_HTTP_H__ */
