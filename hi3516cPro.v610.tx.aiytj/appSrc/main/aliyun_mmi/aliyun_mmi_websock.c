/*
 *       Filename:  aliyun_mmi_websock.c
 *    Description:  WSS 客户端实现（基于 libwebsockets v4.5.8）
 *
 *                  功能:
 *                    - lws_context 创建/销毁
 *                    - WSS 连接握手（SNI + 自定义 Header）
 *                    - WS 帧发送（TEXT / BINARY / PING / PONG）
 *                    - WS 帧接收 → 环形缓冲区 → 上层逐帧取走
 *                    - 非阻塞 service 轮询
 *
 *                  发送链路:
 *                    aliyun_mmi_ws_send → 入待发队列
 *                    → lws_callback_on_writable
 *                    → WRITEABLE 回调: lws_write 排空队列
 *
 *                  接收链路:
 *                    LWS CLIENT_RECEIVE 回调 → 写入环形缓冲区
 *                    → 上层 aliyun_mmi_ws_recv 逐帧取走
 *        Version:  1.0
 *        Created:  07/17/2026
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>

#include "g_log.h"
#include "utils.h"
#include "debug.h"
#include "c_mmi_msg.h"
#include "factory_db.h"
#include "fifo_queue.h"
#include "aliyun_mmi_cb.h"
#include "aliyun_mmi_dialog.h"
#include "aliyun_mmi_websock.h"

/* ================================================================
 *  内部常量
 * ================================================================ */

#define WS_RECV_RING_SIZE   (64 * 1024)  /* 接收环形缓冲区 */
#define WS_SEND_BUF_SIZE    (64 * 1024)  /* 单帧发送缓冲（含 LWS_PRE） */
#define WS_MAX_SEND_QUEUE   (8)          /* 发送队列最大深度 */
#define WS_CERT_PATH        F_WS_CERT

/* close 握手兜底超时（秒）：服务端在此时间内不回 close 帧则强制关闭 */
#define WS_CLOSE_ACK_TIMEOUT_SEC  (3)

/* LWS 协议名 */
#define WS_PROTOCOL_NAME    "mmi-ws"

/* ================================================================
 *  内部类型
 * ================================================================ */

/* 待发帧队列节点（由 fifo_queue 管理出队/入队） */
typedef struct ws_send_node {
    uint8_t  opcode;
    uint32_t len;
    uint8_t  data[LWS_PRE + WS_SEND_BUF_SIZE];  /* LWS_PRE 前导 */
} ws_send_node_t;

/* 接收环形缓冲区 */
typedef struct {
    uint8_t  buf[WS_RECV_RING_SIZE];
    uint32_t head;       /* 写指针 */
    uint32_t tail;       /* 读指针 */
    uint32_t count;      /* 已缓存字节数 */
} ws_recv_ring_t;

/* WS 回调 handler 类型 + 分发表条目 */
typedef int (*cbWsHandler)(struct lws *wsi, void *in, size_t len);

typedef struct {
    enum lws_callback_reasons reason;
    cbWsHandler              handler;
} sWsCallbackMap;

/* ================================================================
 *  全局运行时状态
 * ================================================================ */

typedef struct {
    char                 headers[4096];    /* 暂存握手头 */
    struct lws_context  *ctx;              /* lws_context */
    struct lws          *wsi;              /* 当前 WSI */
    int                  connected;        /* 连接是否已建立 */
    int                  closing;          /* 是否正在等待服务端 close 完成 */

    queue_t             *send_queue;       /* 待发送帧队列 (fifo_queue) */
    int                  cnt_queue;        /* 队列当前深度 */

    ws_recv_ring_t       recv_ring;        /* 接收环形缓冲区 */

    /* 分片接收累积 */
    uint8_t             *frag_buf;         /* 分片累积缓冲（realloc 动态分配） */
    uint32_t             frag_len;         /* 已累积字节数 */
    uint8_t              frag_opcode;      /* 当前消息的 opcode（首 fragment 记录） */
} sMmiWSRun;

static sMmiWSRun g_ws = {0};

/* ================================================================
 *  内部: 发送队列操作
 * ================================================================ */

static void send_queue_push(uint8_t opcode, const uint8_t *data, uint32_t len)
{
    ws_send_node_t *old = NULL;
    ws_send_node_t *node = NULL;

    if (len > WS_SEND_BUF_SIZE) {
        ERR("ws send oversized, drop len=%u\n", len);
        goto exit;
    }

    node = (ws_send_node_t *)malloc(sizeof(ws_send_node_t));
    if (!node) {
        ERR("OOM for ws send node\n");
        goto exit;
    }

    memset(node, 0, sizeof(*node));
    node->opcode = opcode;
    node->len    = len;
    memcpy(node->data + LWS_PRE, data, len);

    /* 队列满时抛弃最旧成员 */
    if (g_ws.cnt_queue >= WS_MAX_SEND_QUEUE) {
        old = (ws_send_node_t *)fifo_queue_pop_unblock(g_ws.send_queue);
        if (old) {
            free(old);
            g_ws.cnt_queue--;
        }
    }

    fifo_queue_push(g_ws.send_queue, node);
    g_ws.cnt_queue++;

exit:
    return;
}

static ws_send_node_t *send_queue_pop(void)
{
    ws_send_node_t *node = NULL;

    if (NULL != g_ws.send_queue) {
        node = (ws_send_node_t *)fifo_queue_pop_unblock(g_ws.send_queue);
        if (node) {
            g_ws.cnt_queue--;
        }
    }

    return node;
}

static void send_queue_clear(void)
{
    ws_send_node_t *node;
    while ((node = send_queue_pop()) != NULL) {
        free(node);
    }
}

/* ================================================================
 *  内部: WS opcode → lws_write_protocol 映射
 * ================================================================ */

static enum lws_write_protocol ws_opcode_to_lws(uint8_t opcode)
{
    switch (opcode) {
    case WS_OPCODE_TEXT:   return LWS_WRITE_TEXT;
    case WS_OPCODE_BINARY: return LWS_WRITE_BINARY;
    case WS_OPCODE_PING:   return LWS_WRITE_PING;
    case WS_OPCODE_PONG:   return LWS_WRITE_PONG;
    default:               return LWS_WRITE_BINARY;
    }
}

/* ---------- 各事件 handler ---------- */

static int cb_ws_append_header(struct lws *wsi, void *in, size_t len)
{
    char **p     = (char **)in;
    char  *end   = (*p) + len;
    char  *hdr   = g_ws.headers;
    char  *line  = NULL;
    char  *save  = NULL;
    int   ret = 0;

    if (!hdr[0]) {
        goto exit;
    }

    /* 逐行注入 Key: Value */
    line = strtok_r(hdr, "\r\n", &save);
    while (line) {
        /* 跳过前导空白 */
        while (*line == ' ' || *line == '\t') line++;
        if (!*line) {
            line = strtok_r(NULL, "\r\n", &save);
            continue;
        }

        char *colon = strchr(line, ':');
        if (colon) {
            int  key_len = (int)(colon - line);
            char *val    = colon + 1;
            while (*val == ' ') val++;

            if (*val && *p + key_len + strlen(val) + 4 < end) {
                if (lws_add_http_header_by_name(wsi,
                        (const unsigned char *)line,
                        (const unsigned char *)val,
                        (int)strlen(val), (unsigned char **)p,
                        (unsigned char *)end)) {
                    ERR("ws add header failed\n");
                }
            }
        }
        line = strtok_r(NULL, "\r\n", &save);
    }

exit:
    return ret;
}

static int cb_ws_established(struct lws *wsi, void *in, size_t len)
{
    g_ws.connected = 1;
    g_ws.wsi       = wsi;
    g_ws.closing   = FALSE;
    pri_mmi(LVL_DBG, "ws established\n");
    /* 连接建立后立即请求可写，排空可能积压的发送队列 */
    lws_callback_on_writable(wsi);

    return 0;
}

static int cb_ws_writeable(struct lws *wsi, void *in, size_t len)
{
    ws_send_node_t *node = NULL;
    int ret = 0;

    node = send_queue_pop();
    if (!node) {
        goto exit;
    }

    enum lws_write_protocol wp = ws_opcode_to_lws(node->opcode);
    int n = lws_write(wsi, node->data + LWS_PRE, node->len, wp);
    if (n < (int)node->len) {
        ERR("ws write fail: %d/%u\n", n, node->len);
        free(node);
        ret = -1;
        goto exit;
    }

    pri_mmi(LVL_LOOP, "ws opcode=%u len=%u\n", node->opcode, node->len);
    free(node);

    /* 队列可能还有数据，继续请求可写 */
    lws_callback_on_writable(wsi);

exit:
    return ret;
}

static int cb_ws_receive(struct lws *wsi, void *in, size_t len)
{
    uint8_t opcode = WS_OPCODE_BINARY;
    int ret = 0, bytes_analysed = 0, is_text = FALSE;

    /* 仅用 lws_frame_is_binary 判断 opcode 即可。
     * lws_frame_is_binary 对分片帧基于首帧 opcode 判断，每个分片
     * 都能正确区分 TEXT/BINARY。切勿用 lws_is_final_fragment 参与
     * 判断，否则中间分片会被误判为 BINARY，导致 SDK 解析失败
     * （parse wss text failed）。 */
    if (!lws_frame_is_binary(wsi)) {
        opcode = WS_OPCODE_TEXT;
    }

    pri_mmi(LVL_LOOP, "ws recv opcode=%u len=%zu first=%d final=%d\n",
            opcode, len, lws_is_first_fragment(wsi), lws_is_final_fragment(wsi));

    /* 首 fragment：记录 opcode，重置累积长度 */
    if (lws_is_first_fragment(wsi)) {
        pri_mmi(LVL_DBG, "first fragment\n");
        g_ws.frag_opcode = opcode;
        g_ws.frag_len = 0;
    }

    /* realloc 累积当前 fragment 数据，统一多预留 1 字节：
     * TEXT 帧 final 时用于写 '\0'，BINARY 帧则忽略。 */
    uint8_t *p_buf = (uint8_t *)realloc(g_ws.frag_buf, g_ws.frag_len + len + 1);
    if (NULL == p_buf) {
        ERR("OOM for frag buf\n");
        goto free_and_exit;
    }

    g_ws.frag_buf = p_buf;
    memcpy(&g_ws.frag_buf[g_ws.frag_len], in, len);
    g_ws.frag_len += (uint32_t)len;

    pri_mmi(LVL_LOOP, "ws frag accumulate, total=%u\n", g_ws.frag_len);

    /* 非 final：继续等待后续 fragment，保留累积状态 */
    if (!lws_is_final_fragment(wsi)) {
        goto exit;
    }

    pri_mmi(LVL_DBG, "final fragment\n");

    /* final：opcode 以首 fragment 记录的为准 */
    opcode  = g_ws.frag_opcode;
    is_text = (WS_OPCODE_TEXT == opcode);

    if (is_text) {
        /* TEXT 帧需 '\0' 结尾（SDK 用 require_null_terminated 解析） */
        g_ws.frag_buf[g_ws.frag_len] = '\0';
    }

    bytes_analysed = c_mmi_analyze_recv_data(opcode, g_ws.frag_buf,
                                             (uint32_t)g_ws.frag_len);

    if (bytes_analysed <= 0) {
        ERR("failed to analyse opcode %d, %.*s\n",
            opcode, g_ws.frag_len, g_ws.frag_buf);
        mmi_dialog_reset_played_done();
        mmi_cb_reset_ai_talking();
    }

free_and_exit:
    if (NULL != g_ws.frag_buf) {
        free(g_ws.frag_buf);
        g_ws.frag_buf = NULL;
    }
    g_ws.frag_len = 0;

exit:

    return ret;
}

static int cb_ws_pong(struct lws *wsi, void *in, size_t len)
{
    pri_mmi(LVL_DBG, "ws recv pong\n");
    return 0;
}

static int cb_ws_closed(struct lws *wsi, void *in, size_t len)
{
    pri_mmi(LVL_DBG, "ws closed (wsi=%p cur=%p)\n", (void *)wsi, (void *)g_ws.wsi);

    /* 仅当关闭的是当前活跃连接时才更新全局状态，
     * 避免旧连接的延迟 CLOSED 回调污染新连接状态 */
    if (wsi != g_ws.wsi) {
        goto exit;
    }

    g_ws.connected = 0;
    g_ws.wsi       = NULL;
    g_ws.closing   = FALSE;
    send_queue_clear();

exit:
    return 0;
}

static int cb_ws_conn_error(struct lws *wsi, void *in, size_t len)
{
    ERR("ws conn error: %s (len=%zu)\n", in ? (char *)in : "unknown", len);

    /* 仅当出错的是当前活跃连接时才更新全局状态 */
    if (wsi != g_ws.wsi) {
        goto exit;
    }

    g_ws.connected = 0;
    g_ws.wsi       = NULL;
    g_ws.closing   = FALSE;

    send_queue_clear();

exit:
    return 0;
}

/* ---------- 事件分发表 ---------- */

static sWsCallbackMap g_ws_cb_maps[] = {
    {LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER, cb_ws_append_header},
    {LWS_CALLBACK_CLIENT_ESTABLISHED,             cb_ws_established  },
    {LWS_CALLBACK_CLIENT_WRITEABLE,               cb_ws_writeable    },
    {LWS_CALLBACK_CLIENT_RECEIVE,                 cb_ws_receive      },
    {LWS_CALLBACK_CLIENT_RECEIVE_PONG,            cb_ws_pong         },
    {LWS_CALLBACK_CLIENT_CLOSED,                  cb_ws_closed       },
    {LWS_CALLBACK_CLIENT_CONNECTION_ERROR,        cb_ws_conn_error   },
};

/* ---------- LWS 协议回调入口 ---------- */

static int cb_mmi_ws(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len)
{
    int ret = 0, idx = 0;

    for (idx = 0; idx < ARRAY_SIZE(g_ws_cb_maps); idx++) {
        if (g_ws_cb_maps[idx].reason == reason) {
            ret = g_ws_cb_maps[idx].handler(wsi, in, len);
            goto exit;
        }
    }

exit:
    return ret;
}

/* ================================================================
 *  LWS 协议表
 * ================================================================ */
static struct lws_protocols g_ws_protocols[] = {
    {
        .name                 = WS_PROTOCOL_NAME,
        .callback             = cb_mmi_ws,
        .per_session_data_size = 0,
        .rx_buffer_size       = WS_RECV_RING_SIZE,
        .id                   = 0,
        .user                 = NULL,
        .tx_packet_size       = 0,
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 }  /* 哨兵 */
};

/* ================================================================
 *  公开 API
 * ================================================================ */
int aliyun_mmi_ws_init(void)
{
    struct lws_context_creation_info info;
    int ret = SUCCESS;

    if (g_ws.ctx) {
        pri_mmi(LVL_DBG, "ws ctx already inited\n");
        /* ctx 保留的场景下，确保 send_queue 可用 */
        if (!g_ws.send_queue) {
            g_ws.send_queue = create_fifo_queue();
            goto_exit_if_fail(NULL != g_ws.send_queue, exit, ret = FAILURE,
                              "failed to create send fifo_queue\n");
        }
        goto exit;
    }

    g_ws.send_queue = create_fifo_queue();
    goto_exit_if_fail(NULL != g_ws.send_queue, exit, ret = FAILURE,
                      "failed to create send fifo_queue\n");

    memset(&info, 0, sizeof(info));

    info.options     = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port        = CONTEXT_PORT_NO_LISTEN;    /* 纯客户端 */
    info.protocols   = g_ws_protocols;
    info.gid         = -1;
    info.uid         = -1;
    info.connect_timeout_secs = 10;

#if defined(LWS_WITH_TLS)
    info.client_ssl_ca_filepath = WS_CERT_PATH;
#endif

    /* 设置日志级别 */
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

    g_ws.ctx = lws_create_context(&info);
    goto_exit_if_fail(NULL != g_ws.ctx, exit, ret = FAILURE,
                      "failed to create lws context\n");

    DBG("ws context created\n");

exit:
    return ret;
}

void aliyun_mmi_ws_deinit(void)
{
    send_queue_clear();

    if (g_ws.send_queue) {
        release_fifo_queue(g_ws.send_queue);
        g_ws.send_queue = NULL;
    }

    if (g_ws.frag_buf) {
        free(g_ws.frag_buf);
        g_ws.frag_buf = NULL;
    }
    g_ws.frag_len = 0;

    g_ws.connected = 0;
    g_ws.wsi       = NULL;

    if (g_ws.ctx) {
        lws_context_destroy(g_ws.ctx);
        g_ws.ctx = NULL;
    }

    memset(&g_ws.recv_ring, 0, sizeof(g_ws.recv_ring));
    DBG("ws deinited\n");
}

int aliyun_mmi_ws_connect(const char *host, int port,
                          const char *path, const char *headers)
{
    struct lws *p_lws = NULL;
    struct lws_client_connect_info cci;
    int ret = SUCCESS;

    goto_exit_if_fail(NULL != g_ws.ctx, exit, ret = FAILURE,
                      "ws not inited\n");

    if (g_ws.connected) {
        pri_mmi(LVL_DBG, "ws already connected\n");
        goto exit;
    }

    /* 暂存 headers 供 CLIENT_APPEND_HANDSHAKE_HEADER 使用 */
    if (headers && headers[0]) {
        snprintf(g_ws.headers, sizeof(g_ws.headers), "%s", headers);
    } else {
        g_ws.headers[0] = '\0';
    }

    memset(&cci, 0, sizeof(cci));
    cci.context        = g_ws.ctx;
    cci.address        = host;
    cci.port           = port;
    cci.path           = path;
    cci.host           = host;
    cci.origin         = "origin";
    cci.protocol       = WS_PROTOCOL_NAME;
    cci.local_protocol_name = WS_PROTOCOL_NAME;
    cci.ssl_connection = LCCSCF_USE_SSL
                        | LCCSCF_ALLOW_SELFSIGNED
                        | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    cci.ietf_version_or_minus_one = -1;
    cci.pwsi           = &g_ws.wsi;

    DBG("ws connecting to %s:%d%s\n", host, port, path);

    p_lws = lws_client_connect_via_info(&cci);
    goto_exit_if_fail(NULL != p_lws, exit, ret = FAILURE,
                      "failed to start ws connect\n");

exit:
    return ret;
}

int aliyun_mmi_ws_service(int timeout_ms)
{
    if (!g_ws.ctx) {
        pri_mmi(LVL_LOOP, "ws ctx is null\n");
        return SUCCESS;
    }

    return lws_service(g_ws.ctx, timeout_ms);
}

int aliyun_mmi_ws_send(uint8_t opcode, const uint8_t *data, uint32_t len)
{
    int ret = SUCCESS;

    goto_exit_if_fail(g_ws.connected && g_ws.wsi && data && len > 0,
                      exit, ret = FAILURE,
                      "ws send param invalid\n");

    send_queue_push(opcode, data, len);

    /* 请求可写回调 */
    lws_callback_on_writable(g_ws.wsi);

exit:
    return ret;
}

int aliyun_mmi_ws_is_connected(void)
{
    return g_ws.connected;
}

struct lws_context *aliyun_mmi_ws_get_context(void)
{
    return g_ws.ctx;
}

void aliyun_mmi_ws_disconnect(void)
{
    if (NULL != g_ws.send_queue) {
        send_queue_clear();
    }

    if (NULL != g_ws.wsi) {
        COLOR_G("wss normal close\n");
        g_ws.closing = TRUE;
        lws_close_reason(g_ws.wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);

        /* 兜底超时：若服务端在 WS_CLOSE_ACK_TIMEOUT_SEC 内不回 close 帧，
         * libwebsockets 会强制触发 CLIENT_CONNECTION_ERROR / CLIENT_CLOSED
         * 回调，从而唤醒阻塞的 lws_service，避免永久卡死。 */
        lws_set_timeout(g_ws.wsi, PENDING_TIMEOUT_CLOSE_ACK,
                        WS_CLOSE_ACK_TIMEOUT_SEC);

        /* 阻塞同步等待服务端 close 回包：
         * lws v4.5 中 lws_service 会阻塞到有事件到达才返回。
         * 服务端回 close 帧后触发 cb_ws_closed，清除 closing 标志，
         * lws_service 随即返回，循环退出；若超时则靠上面的
         * lws_set_timeout 兜底强制关闭。
         * 前提：调用前 service 线程已删除，本线程独占 context。 */
        while (g_ws.closing) {
            lws_service(g_ws.ctx, 0);
        }
    }

    g_ws.connected = 0;
    g_ws.wsi       = NULL;
    g_ws.closing   = FALSE;
    g_ws.headers[0] = '\0';

    DBG("ws disconnected (context kept)\n");
}
