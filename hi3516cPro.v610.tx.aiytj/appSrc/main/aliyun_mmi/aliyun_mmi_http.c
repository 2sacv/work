/*
 *       Filename:  aliyun_mmi_http.c
 *    Description:  阿里云百炼 MMI HTTP 透传实现
 *        Version:  1.0
 *        Created:  07/10/2026 18:17:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "g_log.h"
#include "utils.h"
#include "js_http_client.h"
#include "aliyun_mmi_http.h"

/* ================================================================
 *  内部类型: HTTP 响应透传缓冲区（userdata）
 * ================================================================ */

typedef struct {
    char *buf;
    int   size;
    int   len;
} sAliyunHttpRspBuf;

/* ================================================================
 *  错误码转字符串（单入口单出口）
 * ================================================================ */

const char *aliyun_mmi_err_str(int err_code)
{
    const char *msg = "unknown error";

    switch (err_code) {
    case E_ALIYUN_ERR_SUCCESS:          msg = "success";                                 break;
    case E_ALIYUN_ERR_NO_QUOTA:         msg = "no device quota available";               break;
    case E_ALIYUN_ERR_DEV_REGISTERED:   msg = "device already registered";               break;
    case E_ALIYUN_ERR_DEV_NOT_REG:      msg = "device not registered";                   break;
    case E_ALIYUN_ERR_TIME_EXPIRED:     msg = "request time must be within 5 minutes";   break;
    case E_ALIYUN_ERR_SIGN_EMPTY:       msg = "signature is empty";                      break;
    case E_ALIYUN_ERR_DEV_NAME_LEN:     msg = "device name max 32 characters";           break;
    case E_ALIYUN_ERR_DEV_NAME_INVALID: msg = "device name invalid";                     break;
    case E_ALIYUN_ERR_DEV_DISABLED:     msg = "device is disabled";                      break;
    case E_ALIYUN_ERR_DEV_ACTIVATED:    msg = "device already activated";                break;
    case E_ALIYUN_ERR_MODE_MISMATCH:    msg = "access mode mismatch";                    break;
    case E_ALIYUN_ERR_ACTIVATE_EXPIRED: msg = "activation quota expired";                break;
    case E_ALIYUN_ERR_QUOTA_EXHAUSTED:  msg = "quota exhausted";                         break;
    case E_ALIYUN_ERR_DAILY_LIMIT:      msg = "daily call limit reached";                break;
    case E_ALIYUN_ERR_QUOTA_EXPIRED:    msg = "quota expired";                           break;
    case E_ALIYUN_ERR_GATEWAY_FAIL:     msg = "gateway token request failed";            break;
    case E_ALIYUN_ERR_ENCRYPT_MISMATCH: msg = "encryption data mismatch";                break;
    case E_ALIYUN_ERR_DECRYPT_FAIL:     msg = "decryption failed";                       break;
    case E_ALIYUN_ERR_HTTP_FAIL:        msg = "http request failed";                     break;
    case E_ALIYUN_ERR_JSON_PARSE_FAIL:  msg = "json parse failed";                       break;
    default:                            break;
    }

    return msg;
}

/* ================================================================
 *  内部: 透传响应回调 — 将 body 拷贝到 rsp_buf
 * ================================================================ */

static void aliyun_raw_reply_cb(void *userdata, const char *body,
                                int bodysize, int isfinal)
{
    sAliyunHttpRspBuf *rsp = (sAliyunHttpRspBuf *)userdata;
    int copy_len = 0;

    if (!rsp || !rsp->buf || !body || bodysize <= 0) {
        goto exit;
    }

    copy_len = bodysize;
    if (rsp->len + copy_len >= rsp->size) {
        DBG("reset copy len %d\n", copy_len);
        copy_len = rsp->size - rsp->len - 1;
    }

    if (copy_len > 0) {
        memcpy(rsp->buf + rsp->len, body, copy_len);
        rsp->len += copy_len;
        rsp->buf[rsp->len] = '\0';
    }

    pri_mmi(LVL_DBG, "bodysize: %d, raw rsp:[%s]\n", bodysize, body);

exit:
    return;
}

/* ================================================================
 *  公开 API: 通用 HTTPS POST（透传）
 * ================================================================ */

int aliyun_mmi_http_post_raw(const char *host, int port, const char *path,
                             const char *body, int body_len,
                             char *rsp_buf, int rsp_size, int timeout_ms)
{
    jhttp_client_t *http = NULL;
    sAliyunHttpRspBuf rsp;
    int  ret = SUCCESS;

    goto_exit_if_fail(host && path && body && body_len > 0, exit, ret = FAILURE,
                      "post_raw param invalid\n");
    goto_exit_if_fail(rsp_buf && rsp_size > 0, exit, ret = FAILURE,
                      "post_raw rsp_buf invalid\n");

    memset(rsp_buf, 0, rsp_size);
    rsp.buf  = rsp_buf;
    rsp.size = rsp_size;
    rsp.len  = 0;

    /* 直接传域名，让 jhttp_client 内部做 DNS + TLS SNI = host */
    pri_mmi(LVL_DBG, "connecting to %s:%d\n", host, port);
    http = jhttp_client_create("https", host, port);
    goto_exit_if_fail(http != NULL, exit, ret = FAILURE,
                      "failed to create jhttp_client\n");

    jhttp_client_set_header(http, "Content-Type", "application/json");
    jhttp_client_set_header(http, "Host",           host);

    ret = jhttp_client_post(http, path, body, body_len, NULL,
                            aliyun_raw_reply_cb, (void *)&rsp, timeout_ms);
    goto_exit_if_fail(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to post, path=%s\n", path);

exit:
    if (http) {
        jhttp_client_destroy(http);
    }
    return ret;
}

/* ================================================================
 *  公开 API: 设备注册透传
 * ================================================================ */

int aliyun_mmi_register_raw(const char *reg_str, char *rsp_buf, int rsp_size)
{
    int ret = SUCCESS;

    goto_exit_if_fail(reg_str && strlen(reg_str) > 0, exit, ret = FAILURE,
                      "register_raw reg_str empty\n");
    goto_exit_if_fail(rsp_buf && rsp_size > 0, exit, ret = FAILURE,
                      "register_raw rsp_buf invalid\n");

    pri_mmi(LVL_DBG, "register_raw body:[%s]\n", reg_str);

    ret = aliyun_mmi_http_post_raw(ALIYUN_MMI_HOST, ALIYUN_MMI_PORT,
                                   ALIYUN_REGISTER_PATH,
                                   reg_str, (int)strlen(reg_str),
                                   rsp_buf, rsp_size,
                                   ALIYUN_MMI_HTTP_DEFAULT_TO);
    if (SUCCESS != ret) {
        ERR("failed to post register_raw, ret=%d\n", ret);
    }

exit:
    return ret;
}

/* ================================================================
 *  公开 API: 获取令牌透传
 * ================================================================ */

int aliyun_mmi_get_token_raw(const char *token_str, char *rsp_buf, int rsp_size)
{
    int ret = SUCCESS;

    goto_exit_if_fail(token_str && strlen(token_str) > 0, exit, ret = FAILURE,
                      "get_token_raw token_str empty\n");
    goto_exit_if_fail(rsp_buf && rsp_size > 0, exit, ret = FAILURE,
                      "get_token_raw rsp_buf invalid\n");

    pri_mmi(LVL_DBG, "get_token_raw body:[%s]\n", token_str);

    ret = aliyun_mmi_http_post_raw(ALIYUN_MMI_HOST, ALIYUN_MMI_PORT,
                                   ALIYUN_GET_TOKEN_PATH,
                                   token_str, (int)strlen(token_str),
                                   rsp_buf, rsp_size,
                                   ALIYUN_MMI_HTTP_DEFAULT_TO);
    if (SUCCESS != ret) {
        ERR("failed to post get_token_raw, ret=%d\n", ret);
    }

exit:
    return ret;
}

/* ================================================================
 *  全托管模式: API 信封处理（纯字符串扫描，零依赖）
 *  服务端返回 {"code":...,"data":{...}}，SDK 只需 data 内容
 * ================================================================ */

char *aliyun_mmi_extract_data_json(const char *rsp_json)
{
    const char *start = NULL;
    const char *p     = NULL;
    char       *data_str = NULL;
    int         depth    = 0;
    int         len      = 0;

    if (!rsp_json) {
        goto exit;
    }

    len = (int)strlen(rsp_json);

    /* 找 "data":{ */
    start = strstr(rsp_json, "\"data\":{");
    if (!start) {
        pri_mmi(LVL_DBG, "extract_data: no \"data\":{ found in %d bytes\n", len);
        data_str = strdup(rsp_json);
        goto exit;
    }

    start += 7;  /* 跳过 "data": */
    pri_mmi(LVL_DBG, "extract_data: found at offset %d/%d\n",
            (int)(start - rsp_json), len);

    /* 花括号配对，找到 data 对象的结束 } */
    for (p = start; *p; p++) {
        if (*p == '{') {
            depth++;
        } else if (*p == '}') {
            depth--;
            if (0 == depth) {
                p++;
                break;
            }
        }
    }

    if (0 != depth) {
        pri_mmi(LVL_DBG, "extract_data: brace mismatch, depth=%d at end\n", depth);
        data_str = strdup(rsp_json);
        goto exit;
    }

    if (p == start) {
        pri_mmi(LVL_DBG, "extract_data: empty data object\n");
        data_str = strdup(rsp_json);
        goto exit;
    }

    pri_mmi(LVL_DBG, "extract_data: extracted %d bytes\n", (int)(p - start));
    data_str = (char *)malloc(p - start + 1);
    if (data_str) {
        memcpy(data_str, start, p - start);
        data_str[p - start] = '\0';
    }

exit:
    return data_str;
}

int aliyun_mmi_get_json_code(const char *rsp_json)
{
    const char *code_str = NULL;
    int ret = -1;

    if (!rsp_json) {
        goto exit;
    }

    code_str = strstr(rsp_json, "\"code\":");
    if (!code_str) {
        goto exit;
    }

    ret = atoi(code_str + 7);  /* 跳过 "code": */

exit:
    return ret;
}
