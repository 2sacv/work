/*
 *       Filename:  aliyun_mmi_websock.h
 *    Description:  WSS 客户端接口（封装 libwebsockets）
 *                  供 aliyun_mmi_dialog 调用，实现与阿里云百炼 MMI 的
 *                  WebSocket 长连接通信。
 *        Version:  1.0
 *        Created:  07/17/2026
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#ifndef __ALIYUN_MMI_WEBSOCK_H__
#define __ALIYUN_MMI_WEBSOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ================================================================
 *  WS Opcode (与 c_mmi.h WEBSOCKET_OPCODE_* 一致)
 * ================================================================ */

enum {
    WS_OPCODE_TEXT       = 0x01,
    WS_OPCODE_BINARY     = 0x02,
    WS_OPCODE_DISCONNECT = 0x08,
    WS_OPCODE_PING       = 0x09,
    WS_OPCODE_PONG       = 0x0A,
};

/* ================================================================
 *  公开接口
 * ================================================================ */

/**
 * @brief 初始化 WSS 模块（创建 lws_context）
 * @return 0 成功, 非 0 失败
 */
int aliyun_mmi_ws_init(void);

/**
 * @brief 销毁 WSS 模块（关闭连接 + 销毁 context）
 */
void aliyun_mmi_ws_deinit(void);

/**
 * @brief 断开当前 WSS 连接（不销毁 context，可再次 connect）
 */
void aliyun_mmi_ws_disconnect(void);

/**
 * @brief 向指定主机发起 WSS 连接
 * @param host    服务端域名或 IP
 * @param port    端口（443）
 * @param path    URI 路径（例如 "/api-ws/v1/inference"）
 * @param headers 额外 HTTP 头（如 Authorization），格式 "Key: Value\r\n"
 *                多条头用 "\r\n" 分隔，末尾不需要 "\r\n"
 * @return 0 成功发起握手, 非 0 失败
 */
int aliyun_mmi_ws_connect(const char *host, int port,
                          const char *path, const char *headers);

/**
 * @brief 非阻塞 service 轮询（驱动 libwebsockets 事件循环）
 *        应在定时器/主循环中定期调用。
 * @param timeout_ms 超时时间（毫秒），传 0 非阻塞立即返回
 * @return 0 正常, 非 0 失败
 */
int aliyun_mmi_ws_service(int timeout_ms);

/**
 * @brief 将 WS 数据帧排入待发送队列
 *        实际发送由 WRITEABLE 回调异步完成。
 * @param opcode WS opcode（WS_OPCODE_TEXT / WS_OPCODE_BINARY / WS_OPCODE_PING）
 * @param data   待发送数据（不含 LWS_PRE）
 * @param len    数据长度
 * @return 0 成功入队, 非 0 失败
 */
int aliyun_mmi_ws_send(uint8_t opcode, const uint8_t *data, uint32_t len);

/**
 * @brief 查询 WSS 连接是否已建立
 * @return 1 已连接, 0 未连接
 */
int aliyun_mmi_ws_is_connected(void);

/**
 * @brief 获取 lws_context 指针（供 dialog 层 lws_cancel_service 跨线程唤醒）
 * @return lws_context 指针，未初始化时返回 NULL
 */
struct lws_context *aliyun_mmi_ws_get_context(void);

#ifdef __cplusplus
}
#endif
#endif /* __ALIYUN_MMI_WEBSOCK_H__ */
