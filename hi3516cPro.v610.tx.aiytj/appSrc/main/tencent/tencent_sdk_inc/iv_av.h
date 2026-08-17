/*****************************************************************************
 * Tencent is pleased to support the open source community by making IoT Video available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 *is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 *or implied. See the License for the specific language governing permissions and limitations under
 *the License.
 *
 * @file    iv_av.h
 * @brief   Description audio and video function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_AV_H__
#define __IV_AV_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>

#include "iv_cm.h"
#include "iv_sys.h"

/***********************Audio And Video Transmission API**************************/

/*Defines the type of avt */
typedef enum
{
    IV_AVT_STREAM_TYPE_AUDIO = 0, /*audio*/
    IV_AVT_STREAM_TYPE_VIDEO = 1, /*video*/
    IV_AVT_STREAM_TYPE_AV    = 2, /*audio+video*/
    IV_AVT_STREAM_TYPE_MJPG  = 3, /*mjpg stream*/

    IV_AVT_STREAM_TYPE_BUTT
} iv_avt_stream_type_e;

/*device status check */
typedef enum
{
    IV_AVT_DEV_ACCEPT = 0, /* device accept app request */
    IV_AVT_DEV_REFUSE = 1, /* device refuse app request*/
    /*
         IV_AVT_DEV_OTHER*
    */
    IV_AVT_DEV_STATUS_BUTT = 10
} iv_avt_device_status_e;

/*device status check */
typedef enum
{
    IV_AVT_REQUEST_SEND_STREAM = 0, /* app request device to send stream */
    IV_AVT_REQUEST_RECV_STREAM = 1, /* app request device to receive stream, only support audio*/
    IV_AVT_REQUEST_BUTT
} iv_avt_request_type_e;

/*Defines the type of request*/
typedef enum
{
    IV_AVT_VIDEO_RES_FL = 0,  // fluency stream
    IV_AVT_VIDEO_RES_SD = 1,  // standard stream
    IV_AVT_VIDEO_RES_HD = 2,  // high stream
    IV_AVT_VIDEO_RES_PB = 3,  // playback stream
    IV_AVT_VIDEO_AUDIO  = 4,  // audio stream
    IV_AVT_VIDEO_RES_BUTT
} iv_avt_video_res_type_e;

/*Defines usr data parameter*/
typedef struct {
    char *src;           // input data
    uint32_t src_len;    // input data length
    iv_cm_memory_s dst;  // output data
} iv_avt_usr_data_parm_s;

/* Defines stream request args*/
typedef struct {
    char *user_args;
    char *requester;
    iv_cm_time_fragment_s *pb_time;  // if res_type is not IV_AVT_VIDEO_RES_PB, the value is NULL.
} iv_avt_req_stream_info_s;

/*Defines stream request parameter*/
typedef struct {
    iv_avt_request_type_e request_type;     // input data
    iv_avt_device_status_e request_result;  // output data
} iv_avt_req_stream_param_s;

/*Defines channel name and status*/
typedef struct {
    iv_cm_memory_s name;
    uint8_t is_online;
} iv_avt_chn_name_param_s;

/*Defines the event type of p2p*/
typedef enum
{
    IV_AVT_EVENT_P2P_PEER_READY        = 0,  // p2p peer ready
    IV_AVT_EVENT_P2P_PEER_CONNECT_FAIL = 1,  // p2p stun connect failed
    IV_AVT_EVENT_P2P_PEER_ERROR        = 2,  // p2p peer network error
    IV_AVT_EVENT_P2P_PEER_ADDR_CHANGED = 3,  // p2p peer addr is changed
    IV_AVT_EVENT_P2P_WATERMARK_LOW     = 4,  // send buffer watermark warning
    IV_AVT_EVENT_P2P_WATERMARK_WARN    = 5,  // send buffer watermark high, stop sending
    IV_AVT_EVENT_P2P_WATERMARK_HIGH    = 6,  // send buffer watermark low, resume sending
    IV_AVT_EVENT_P2P_LOCAL_NET_READY   = 7,  // p2p local network ready
    IV_AVT_EVENT_P2P_LINK_DISCONNECT   = 8,  // p2p client link with remote peer is disconnected
    IV_AVT_EVENT_P2P_CLI_SEND_CLOSE    = 9,  // p2p client send stream closed
    IV_AVT_EVENT_P2P_BUTT
} iv_avt_event_e;

/*Defines the command type*/
typedef enum
{
    IV_AVT_COMMAND_USR_DATA   = 0,  // usr define data, for example ptz
    IV_AVT_COMMAND_REQ_STREAM = 1,  // request stream (send or receive)
    IV_AVT_COMMAND_CHN_NAME   = 2,  // get channel name
    IV_AVT_COMMAND_REQ_IFRAME = 3,  // request I frame

    // playback command
    IV_AVT_COMMAND_PLAYBACK_PAUSE,        // pause playback
    IV_AVT_COMMAND_PLAYBACK_RESUME,       // resume playback
    IV_AVT_COMMAND_PLAYBACK_QUERY_MONTH,  // query record info of month
    IV_AVT_COMMAND_PLAYBACK_QUERY_DAY,    // query record info of day
    IV_AVT_COMMAND_PLAYBACK_SEEK,         // seek record position
    IV_AVT_COMMAND_PLAYBACK_FF,           // playback fast forward(快进)
    IV_AVT_COMMAND_PLAYBACK_SPEED,        // playback speed(快放 or 慢放)
    IV_AVT_COMMAND_PLAYBACK_REWIND,       // rewind record(倒放)
    IV_AVT_COMMAND_PLAYBACK_PROGRESS,     // playback progress

    // file list command
    IV_AVT_COMMAND_QUERY_FILE_LIST,  // file list

    // voip call command
    IV_AVT_COMMAND_CALL_ANSWER,  // answer the call
    IV_AVT_COMMAND_CALL_HANG_UP, // hang up after connected
    IV_AVT_COMMAND_CALL_REJECT,  // reject before connected
    IV_AVT_COMMAND_CALL_CANCEL,  // cancel the call before connected
    IV_AVT_COMMAND_CALL_BUSY,    // busy
    IV_AVT_COMMAND_CALL_TIMEOUT, // timeout

    // voip av status
    IV_AVT_COMMAND_CALL_AV_CLOSE,       // wx mini program close microphone and camera
    IV_AVT_COMMAND_CALL_AV_AUDIO_ONLY,  // wx mini program open microphone close camera
    IV_AVT_COMMAND_CALL_AV_VIDEO_ONLY,  // wx mini program open camera close microphone
    IV_AVT_COMMAND_CALL_AV_AUDIO_VIDEO, // wx mini program open microphone and camera

    IV_AVT_COMMAND_TYPE_BUTT
} iv_avt_command_type_e;

/*Defines the download status*/
typedef enum
{
    IV_AVT_DOWNLOAD_STATUS_START,  // start to download
    IV_AVT_DOWNLOAD_STATUS_RUNNING,
    IV_AVT_DOWNLOAD_STATUS_STOP,  // stop to download
    IV_AVT_DOWNLOAD_STATUS_BUTT
} iv_avt_download_status_e;

typedef enum
{
    IV_AVT_P2P_LOG_DISABLE = 0,
    IV_AVT_P2P_LOG_ERROR   = 1,
    IV_AVT_P2P_LOG_WARN    = 2,
    IV_AVT_P2P_LOG_INFO    = 3,
    IV_AVT_P2P_LOG_DEBUG   = 4,
    IV_AVT_P2P_LOG_VERBOSE = 5
} iv_avt_p2p_log_level_e;

typedef enum
{
    IV_AVT_P2P_NIC_NORMAL = 0,
    IV_AVT_P2P_NIC_NAT_EX = 1,
} iv_avt_p2p_nic_type_e;

typedef enum
{
    IV_AVT_P2P_UDP = 0,
    IV_AVT_P2P_TCP = 1,
    IV_AVT_P2P_AUTO = 2,
} iv_avt_p2p_tp_type_e;

/*Defines the parameter of congestion ctrl*/
typedef struct {
    size_t low_mark;
    size_t warn_mark;
    size_t high_mark;
    bool enable;
} congestion_ctrl_s;

/*Defines the parameter of p2p keep alive ctrl*/
typedef struct {
    uint8_t time_inter_s;
    uint8_t max_attempt_num;
} p2p_keep_alive_s;

typedef struct {
    // 瞬时发送速率，随网速变化会有较大波动，单位：字节/秒
    uint32_t inst_net_rate;
    // 过去一秒内累计发送并得到对端确认的数据，即平均发送速率，单位：字节/秒
    uint32_t ave_sent_rate;
    // 累计发送并得到对端确认的数据， 单位：字节
    uint64_t sum_sent_acked;
    // 当前P2P链路的连接模式：0 无效；1 直连；2 转发
    uint32_t link_mode;
} iv_p2p_send_stats_s;

/*
 * P2P初始化参数
 * log_level： P2P模块日志级别，开发阶段，如果配置了日志文件，建议配置在DEBUG级别，否则用INFO
 * log_file_path： P2P模块日志文件路径，若为NULL则输出到stderr或串口终端，建议配置在"/tmp"路径
 * log_file_size： P2P模块日志文件大小限制，实际占用空间最大为该值两倍，SDK会自动回滚
 * mtu_size：P2P模块UDP MTU大小，范围 500-1460，若为0则采用默认值1200
 * nic_type：一般置为0，当设备采用某些特殊的NAT网卡时需要进行设置
 * sender_interval_ms: 发送数据的timer间隔，默认为10毫秒，在码率较低且对功耗要求较高时可以适当改大
 * keepalive_interval_s：P2P与STUN服务器的保活间隔，默认为10秒
 * access_retry_times：连接STUN服务器的重试次数，默认为10，实际的重试次数为该值的3倍
 * protocol: 一般置为0, 当设备处于不支持UDP包的网络环境时, 可以设置该参数
 * proxy_ip&proxy_port: 低功耗场景在WiFi模块运行proxy服务的IP和端口，两者都有效时会进入proxy模式
 * thread_policy: P2P线程调度策略：0=SCHED_OTHER 1=SCHED_FIFO 2=SCHED_RR
 * thread_priority: P2P线程优先级：0~99, 99为最高优先级，0为系统默认
 * need_pre_connect ： 是否需要预先处理连接，0：不需要，1：需要, 默认为0;(针对物联网卡ip限制通过问题，需要预先处理连接)
 * */
typedef struct {
    iv_avt_p2p_log_level_e log_level;
    const char *log_file_path;
    uint32_t log_file_size;
    uint32_t mtu_size;
    iv_avt_p2p_nic_type_e nic_type;
    uint32_t sender_interval_ms;
    uint32_t keepalive_interval_s;
    uint32_t access_retry_times;
    iv_avt_p2p_tp_type_e protocol; // 该参数目前已废弃，无需配置
    const char *proxy_ip;
    uint32_t proxy_port;
    uint32_t loopback_port;
    int thread_policy;
    int thread_priority;
    uint32_t need_pre_connect;
} p2p_init_params_s;

typedef struct {
    int probe_port;       //局域网探测的端口
    int trans_port;       //局域网数据传输的端口
    char local_addr[64];  //设备在局域网的IP地址
    char *vendor_id;      //厂商ID,用于区分不同厂商的设备
    char *device_id;      //设备ID,用于区分设备
} local_net_info_s;

/*Defines the parameter of av talk init */
typedef struct iv_avt_init_parm_s {
    /*max size of stream frame (kB)*/
    uint32_t max_frame_size;
    /*max connect number, means max connect number at the same moment, for example APP*/
    uint32_t max_connect_num;
    /*the callback function of start av talk*/
    void (*iv_avt_get_av_enc_info_cb)(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type,
                                      iv_cm_av_data_info_s *av_data_info, void *args);

    /*the callback function of start av talk*/
    void (*iv_avt_start_real_play_cb)(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type,
                                      void *args);

    /*the callback function of stop av talk*/
    void (*iv_avt_stop_real_play_cb)(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type);

    /*the callback function of recv stream notice*/
    int (*iv_avt_start_recv_stream_cb)(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type,
                                       iv_cm_av_data_info_s *p_av_data_info);

    /*the callback function of recv stream notice*/
    int (*iv_avt_stop_recv_stream_cb)(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type);

    /*the callback function of recv stream*/
    int (*iv_avt_recv_stream_cb)(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type, void *p_stream);

    /*已废弃，通过IV_AVT_COMMAND_USR_DATA实现*/
    void (*iv_avt_recv_user_data_cb)(uint32_t visitor, uint32_t channel, const char *src, uint32_t src_len,
                                     iv_cm_memory_s *dst);

    /*the callback function of notify*/
    void (*iv_avt_notify_cb)(iv_avt_event_e event, uint32_t visitor, uint32_t channel,
                             iv_avt_video_res_type_e video_res_type);

    /*the callback function of command*/
    int (*iv_avt_recv_command_cb)(iv_avt_command_type_e command, uint32_t visitor, uint32_t channel,
                                  iv_avt_video_res_type_e video_res_type, void *args);

    /*the callback function of download*/
    int (*iv_avt_download_file_cb)(iv_avt_download_status_e status, uint32_t visitor, uint32_t channel, void *args);

    /*the callback function of get peer outer net info*/
    void (*iv_avt_get_peer_outer_net_cb)(uint32_t visitor, uint32_t channel, char *net_info);
    /*congestion control*/
    congestion_ctrl_s congestion;
    /*p2p keep alive parameter*/
    p2p_keep_alive_s p2p_keep_alive;
    /*p2p init parameters*/
    p2p_init_params_s p2p_init_params;
    /*local network config*/
    local_net_info_s net_info;
} iv_avt_init_parm_s;

/********************************API declaration******************************/

/**
 * @brief init audio and video transmission module
 *
 * @param pstInitParm:av transmission interface
 * @return int: error code
 */
int iv_avt_init(const iv_avt_init_parm_s *pstInitParm);

/**
 * @brief exit audio and video transmission module
 *
 * @return int：error code
 */
int iv_avt_exit(void);

/**
 * @brief send audio and video stream
 *
 * @param visitor:client id
 * @param channel:current stream come from device id
 * @param video_res_type:stream resolution flag
 * @param stream_type: stream type flag
 * @param p_stream:stream data
 * @return int:error code
 */
int iv_avt_send_stream(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type,
                       iv_avt_stream_type_e stream_type, void *p_stream);

/**
 * @brief get send stream real status
 *
 * @param visitor:client id
 * @param channel:send stream source
 * @param video_res_type:stream resolution flag
 * @param stream_send_status:send stream real status
 * @return int:error code
 */
int iv_avt_get_send_stream_status(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type,
                                  iv_p2p_send_stats_s *stream_send_status);
/**
 * @brief 获取相关P2P视频流传输通道的发送缓冲区大小，用于用户自定义的P2P链路拥塞控制
 *
 * @param visitor:client id
 * @param channel:send stream source
 * @param video_res_type :stream resolution flag
 * @return size_t
 */
size_t iv_avt_get_send_stream_buf(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type);

/**
 * @brief 设备端主动结束P2P视频传输
 *
 * @param visitor:client id
 * @param channel:send stream source
 * @param video_res_type:stream resolution flag
 * @return int:error code
 */
int iv_avt_send_finish_stream(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type);

/**
 * @brief 设备端主动停止接收对端发过来的流
 *
 * @param visitor:client id
 * @param channel:recv stream source
 * @return int:error code
 */
int iv_avt_recv_finish_stream(uint32_t visitor, uint32_t channel);

/**
 * @brief 设备端主动发送P2P视频传输结束的消息，需要在 iv_avt_send_finish_stream 之前使用
 *
 * @param visitor:client id
 * @param channel:send stream source
 * @param video_res_type:stream resolution flag
 * @return int:error code
 */
int iv_avt_send_finish_stream_msg(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type);

/**
 * @brief 设备端在直播或者回看期间主动向对端发送信令,是一个同步接口
 *
 * @param visitor:client id
 * @param msg:send messge content
 * @param msg_len:send message length
 * @param recv_buf:receive message buffer
 * @param recv_len:receive message length
 * @param timeout_us:commnad timeout, unit: ms
 * @return int
 */
int iv_avt_send_command(uint32_t visitor, const char *msg, size_t msg_len, unsigned char *recv_buf, size_t *recv_len,
                        uint32_t timeout_ms);

/**
 * @brief get p2p peerinfo
 *
 * @return const char*: peer information
 */
const char *iv_avt_get_peerinfo(void);


// 注意以下Raw P2P API不再维护，不建议使用!!
/********************************Raw P2P API ******************************/
/* Audio And Video Transmission API and Raw P2P API  are mutually exclusive. Only one of them
 * can be used*/
/*
 * P2P框架回调，非阻塞操作
 *
 * p2p_handle_send_init_cb
 *       当收到app发起的观看视频（实时流或回看）或文件下载请求时，会调用该回调进行相关初始化
 *       并将对端的peername以及请求参数传入，请求参数采用 key1=value&key2=value2 形式，
 *       如action=playback&start=1611837000&end=1611837100可表示请求回看，包含起始和结束时间
 *       回调应该返回唯一的视频通道句柄指针handle，P2P在后续获取数据时会传入
 * p2p_handle_send_get_cb:
 *       P2P会以定时器驱动（10毫秒）的方式通过该回调获取数据并通过P2P通道进行发送
 *       如果没有数据，可将frame_buf置为NULL并将frame_size置为0
 *       如果出现异常，可将frame_buf置为NULL并将frame_size置为-1，则会停止取数据
 * p2p_handle_send_stop_cb:
 *       当视频流接收端（app）停止观看或断开时，会通过该回调进行通知
 *
 * p2p_handle_recv_init_cb:
 *       当app向camera发起语音或数据发送请求时，会调用该回调进行相关初始化
 *       并将对端的peername以及请求参数传入，请求参数采用 key1=value&key2=value2 形式，
 *       回调应该返回唯一的接收句柄指针handle，P2P在后续接收数据回调时会传入
 * p2p_handle_recv_voice_cb：当app向camera发送语音数据时，可通过该回调获取数据，并传入句柄指针
 *          语音数据为流式传输，当app停止发送或断开时，回调会传入recv_buf=NULL，recv_len=0作为结束事件
 *
 * p2p_handle_event_notify_cb:
 *      P2P事件通知回调，具体事件类型参考iv_avt_event_e
 *
 * p2p_handle_command_cb：
 *       当收到app发起的信令操作时，通过该回调进行处理并返回相关数据
 *       比如可通过该接口获取录像列表和进行云台控制
 *       当有新的信令请求到达时，如果该信令的回复数据小于16KB，会优先处理并回复
 *       因此建议回复的数据长度在16KB以内，否则会影响实时性
 *
 * 注意：回调里面的peer信息，存放于临时内存，如果要持久化使用的话需要在回调里进行拷贝
 *
 * */
typedef struct iv_p2p_parm_s {
    void *(*p2p_handle_send_init_cb)(const char *params, const char *peer);
    iv_cm_memory_s (*p2p_handle_send_get_cb)(void *handle);
    int (*p2p_handle_send_stop_cb)(void *handle);
    void (*p2p_handle_event_notify_cb)(iv_avt_event_e, void *handle);
    void *(*p2p_handle_recv_init_cb)(const char *params, const char *peer);
    void (*p2p_handle_recv_voice_cb)(void *handle, uint8_t *recv_buf, size_t recv_len, uint8_t *stop);
    iv_cm_memory_s (*p2p_handle_command_cb)(const char *command, size_t cmd_len, const char *peer);
    /*p2p keep alive parameter*/
    p2p_keep_alive_s p2p_keep_alive;
    /*p2p init parameters*/
    p2p_init_params_s p2p_init_params;
    /*local network config*/
    local_net_info_s net_info;
} iv_p2p_parm_s;

/**
 * @brief:initial p2p resource
 *
 * @param pstInitParm:p2p initial parameter
 * @return int:error code
 */
int iv_avt_p2p_init(const iv_p2p_parm_s *pstInitParm);

/**
 * @brief destroy p2p resource
 *
 * @return int:error code
 */
int iv_avt_p2p_exit(void);

/*
 * 设置P2P发送缓冲区的水位告警值(单位：字节)，用于链路拥塞感知和码率变化控制
 * 当发送缓冲区存在未发送的数据超过warn_mark时，会触发一次IPC_P2P_EVENT_BUF_WATERMARK_WARN事件
 * 当数据超过high_mark时，会触发一次IPC_P2P_EVENT_BUF_WATERMARK_HIGH事件，并暂停通过回调取数据
 * 直到数据低于low_mark时，会触发一次IPC_P2P_EVENT_BUF_WATERMARK_LOW，并重新取数据
 * 可根据视频流码率和可用内存做动态调整
 * 目前该设置是全局的，即对所有的视频流推送都使用同样的设置
 * 如果三个值都设置为0， 则不进行告警，用户可以通过iv_avt_p2p_get_send_buf自行做拥塞控制
 * */
int iv_avt_p2p_set_buf_watermark(size_t low_mark, size_t warn_mark, size_t high_mark);

/*
 * 获取相关P2P视频流传输通道的发送缓冲区大小，用于用户自定义的P2P链路拥塞控制
 * 传入由p2p_handle_send_init_cb回调创建的视频通道句柄指针handle
 * */
size_t iv_avt_p2p_get_send_buf(void *handle);

/*
 * 获取相关P2P视频流传输通道的发包统计数据如发送速率等，用于用户自定义的P2P链路拥塞控制
 * 传入由p2p_handle_send_init_cb回调创建的视频通道句柄指针handle
 * */
iv_p2p_send_stats_s iv_avt_p2p_get_send_status(void *handle);

/*
 * 在看直播期间，设备端可以通过该接口主动发送反馈消息给对端，并可以接收对端回复
 * 该调用为同步阻塞方式，如果设置了较短的超时时间，则在网络繁忙或不稳定时候，该消息可能会因为超时而被丢弃
 * peername: 对端的peername，在设备端收到app音视频传输请求时会获得该值
 * msg可以为任意格式字符或二进制数据，长度由msg_len提供，建议在16KB以内，否则会影响实时性
 * recv_buf必须是事先分配的足够大的内存用于存放回复的数据，实际数据长度根据recv_len获取
 * timeout_us: 命令超时时间，单位为毫秒，值为0时采用默认超时(7500ms左右)
 */
int iv_avt_p2p_send_command(const char *peer, const char *msg, size_t msg_len, unsigned char *recv_buf,
                            size_t *recv_len, uint32_t timeout_ms);
/*
 * 设置P2P模块MTU大小
 * 当播放端不出图时，建议调用该接口，逐渐递减MTU值。若仍然不能出图，则建议P2P初始化时改为IV_AVT_P2P_TCP
 * mtu_size：范围 500-1200
 * */
void iv_avt_set_mtu(uint32_t mtu_size);

/*
 * 获取2P模块MTU大小
 * */
uint32_t iv_avt_get_mtu(void);

/*
 * 设置P2P模块的日志输出级别
 * 
 * log_level: 
 * */
void iv_avt_set_p2p_log_level(iv_sys_log_level_type_e log_level);

/*
 * 获取2P模块日志输出级别
 * */
iv_sys_log_level_type_e iv_avt_get_p2p_log_level(void);



typedef struct {
    char * LLMConfig; // 大模型配置
    int LLMLen;       // 大模型长度
    char *TTSConfig;  // TTS配置
    int TTSLen;       // TTS长度
    char *STTConfig;  // 语音识别配置
    int STTLen;       // 语音识别配置长度
    char *AgentConfig; // 代理配置
    int AgentConfigLen; // 长度
}iv_ai_start_s;

/**
 * @brief 开启AI对话
 * 
 * @param params @see iv_ai_start_s
 * @return int 
 */
int iv_avt_ai_start(iv_ai_start_s *params);

/**
 * @brief 停止AI对话
 * 
 * @return 0 successed;other:failed 
 */
int iv_avt_ai_stop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_AV_H__ */
