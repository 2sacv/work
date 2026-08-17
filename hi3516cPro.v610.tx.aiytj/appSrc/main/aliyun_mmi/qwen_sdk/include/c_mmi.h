/*
 * Copyright 2025 Alibaba Group Holding Ltd.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __C_MMI_H__
#define __C_MMI_H__

#ifdef __cplusplus
extern "C" {
#endif

// websocket opcode
enum {
    WEBSOCKET_OPCODE_TEXT = 0x01,
    WEBSOCKET_OPCODE_BINARY = 0x02,
    WEBSOCKET_OPCODE_DISCONNECT = 0x08,
    WEBSOCKET_OPCODE_PING = 0x09,
    WEBSOCKET_OPCODE_PONG = 0x0A,
};

#include "c_utils.h"

#include "c_mmi_ringbuffer.h"
#include "lib_c_license.h"
#include "c_mmi_storage.h"

#include "cJSON.h"

#define C_MMI_ID_ARRAY_LEN          (C_LICENSE_ID_ARRAY_LEN)
#define C_MMI_APP_ID_STR_LEN        (C_LICENSE_APP_ID_STR_LEN)
#define C_MMI_DN_STR_MAX            (C_LICENSE_DN_STR_MAX)
#define C_MMI_TOKEN_LEN             (C_LICENSE_TOKEN_LEN)
#define C_MMI_DIALOG_ID_LEN         (C_LICENSE_DIALOG_ID_LEN)
#define C_MMI_IP_LEN                (C_LICENSE_IP_LEN)

#define C_MMI_ID_STRING_LEN         (C_MMI_ID_ARRAY_LEN * 2)

#define C_MMI_WORKSPACE_ID_LEN      (C_LICENSE_WORKSPACE_ID_LEN)    // workspace id 20B
#define C_MMI_API_KEY_LEN           (256)                           // 20260331-新发布

#define C_MMI_CITY_LEN              (32)    // max 31+1

enum {
    C_MMI_EVENT_USER_CONFIG,        // 用户对于sdk的配置应该在该事件回调中实现，如音频缓冲区大小、工作模式、音色等
    C_MMI_EVENT_DATA_INIT,	        // 当SDK完成初始化后触发该事件，可在该事件回调中开始建立业务连接
    C_MMI_EVENT_SPEECH_READY,	    // 当正确建立WSS连接后触发该事件，在push和tap模式下仅在该事件后才可以调用speech start
    C_MMI_EVENT_SPEECH_PREPARE,     // 当SDK已准备好可以开始新一轮对话时触发此事件
    C_MMI_EVENT_SPEECH_START,       // 当SDK开始进行音频上行时触发此事件
    C_MMI_EVENT_SPEECH_INTERRUPT,	// 当当前会话被打断时触发该事件，如对话一半按键打断，对话一半有其他请求等
    C_MMI_EVENT_DATA_DEINIT,	    // 当SDK注销后触发此事件

    C_MMI_EVENT_ASR_START,	        // 当ASR开始返回数据时触发此事件
    C_MMI_EVENT_ASR_INCOMPLETE,	    // 此事件返回尚未完成ASR的文本数据（全量）
    C_MMI_EVENT_ASR_COMPLETE,	    // 此事件返回完成ASR的全部文本数据（全量）
    C_MMI_EVENT_ASR_END,		    // 当ASR结束时触发此事件

    C_MMI_EVENT_LLM_INCOMPLETE,	    // 此事件返回尚未处理完成的LLM文本数据（全量）
    C_MMI_EVENT_LLM_COMPLETE,	    // 此事件返回处理完成的LLM全部文本数据（全量）

    C_MMI_EVENT_TTS_START,	        // 当开始音频下行时触发此事件
    C_MMI_EVENT_TTS_END,	        // 当音频完成下行时触发此事件

    C_MMI_EVENT_HEARTBEAT,	        // 当SDK收到云端心跳回复时触发此事件

    C_MMI_EVENT_ERROR,	            // 当SDK收到云端返回错误时触发此事件
};

// MAJOR STATE
enum {
    C_MMI_STATE_NO_INIT,
    C_MMI_STATE_INIT,
    C_MMI_STATE_LISTENING,
    C_MMI_STATE_THINKING,
    C_MMI_STATE_RESPONDING,
    C_MMI_STATE_RESTART,
    C_MMI_STATE_DEINIT,
};

enum {
    C_MMI_STATE_INIT_WAIT_SEND,
    C_MMI_STATE_INIT_WAIT_START,
    C_MMI_STATE_INIT_STARTED,
    C_MMI_STATE_INIT_DONE,

    C_MMI_STATE_LISTENING_IDEL,
    C_MMI_STATE_LISTENING_WAIT_SEND,
    C_MMI_STATE_LISTENING_WAIT_ASR,
    C_MMI_STATE_LISTENING_ASR,
    C_MMI_STATE_LISTENING_WAIT_END,
    C_MMI_STATE_LISTENING_ASR_END,

    C_MMI_STATE_THINKING_WAIT_LLM,
    C_MMI_STATE_THINKING_LLM,
    C_MMI_STATE_THINKING_LLM_END,

    C_MMI_STATE_RESPONDING_WAIT_AUDIO,
    C_MMI_STATE_RESPONDING_AUDIO,
    C_MMI_STATE_RESPONDING_AUDIO_END,
    C_MMI_STATE_RESPONDING_WAIT_PLAY,

    C_MMI_STATE_RESTART_WAIT_SEND,
    C_MMI_STATE_RESTART_WAIT_ACCPECT,

    C_MMI_STATE_DEINIT_WAIT_SEND,
    C_MMI_STATE_DEINIT_WAIT_STOP,
    C_MMI_STATE_DEINIT_STOPED,
};

typedef struct _mmi_err_info_t {
    int32_t code;
    char *name;
    char *msg;
} c_mmi_err_info_t;

typedef struct _mmi_prompt_pram_t { 
    char *key;
    char *value;
} c_mmi_prompt_pram_t;

typedef struct _mmi_info_t {
    c_mmi_ringbuffer_t *recorder_rb;
    c_mmi_ringbuffer_t *player_rb;

    // 云端获取
    char *header;
    char ws_id[C_MMI_WORKSPACE_ID_LEN + 1];
    char app_id[C_MMI_APP_ID_STR_LEN + 1];

    // 设备信息
    char ip[C_MMI_IP_LEN];
    double longitude;
    double latitude;
    char city[C_MMI_CITY_LEN];

    // 随机生成
    uint8_t task_id[C_MMI_ID_ARRAY_LEN];

    uint8_t major_state;
    uint8_t state;

    uint8_t speech_work;
    uint8_t auto_speech;
    uint8_t in_req2rsp;         // 0:表示没有处于req2rsp，1:表示等待accept，2:表示处于req2rsp
    uint8_t req_wait_flag;      // 当flag为1时，需要等待新收到listening才可以发送req2rsp
    uint8_t in_cancel_speech;
    uint8_t responding_flag;
    uint8_t have_spoken;        // 用于记录是否有spoken文本需要合成TTS

    uint32_t cmd_flag;
    int64_t last_req_time;

    int64_t last_hb_time;
    int64_t last_hb_recv_time;

    uint8_t audio_recv_all;
    uint32_t audio_recv_data_size;

    uint8_t takeover_mode;

    cJSON *prompt_param;
    cJSON *req2rsp;
    // cJSON *update_info;
    cJSON *images;
    cJSON *biz_params;
    cJSON *replace_words;
    char  *print_buf;   // UpdateInfo 序列化输出缓冲（C_MMI_PRINT_BUF_SIZE），懒加载，c_mmi_deinit 收口释放
} mmi_info_t;

/**
 * @brief 获取 MMI 信息结构体指针
 * 
 * @return mmi_info_t* 指向全局 mmi_info 结构体的指针
 */
mmi_info_t *c_mmi_get_info(void);

/**
 * @brief 获取 MMI 子状态
 * 
 * @return uint8_t 当前子状态值，取值为 C_MMI_STATE_xxx 系列宏定义
 */
uint8_t c_mmi_get_state(void);

/**
 * @brief 获取 MMI 主状态
 * 
 * @return uint8_t 当前主状态值，取值为 C_MMI_STATE_NO_INIT/LISTENING/THINKING/RESPONDING 等
 */
uint8_t c_mmi_get_major_state(void);

/**
 * @brief 获取WSS服务器主机名字符串。
 *
 * 此函数返回WSS服务器主机名字符串。
 * 
 * @return char* 返回指向WSS服务器主机名字符串的指针
 */
char *c_mmi_get_wss_host(void);

/**
 * @brief 获取WSS海外服务器主机名字符串。
 *
 * 此函数返回WSS海外服务器主机名字符串。
 * 
 * @return char* 返回指向WSS海外服务器主机名字符串的指针
 */
char *c_mmi_get_wss_host_global(void);

/**
 * @brief 获取WSS服务器端口字符串。
 *
 * 此函数返回WSS服务器端口字符串。
 * 
 * @return char* 返回指向WSS服务器端口字符串的指针
 */
char *c_mmi_get_wss_port(void);

/**
 * @brief 获取WSS服务API路径字符串。
 *
 * 此函数返回WSS服务API路径字符串。
 * 
 * @return char* 返回指向WSS服务API路径字符串的指针
 */
char *c_mmi_get_wss_api(void);

/**
 * @brief 获取WSS请求头信息字符串。
 *
 * 此函数返回WSS请求头信息字符串。
 * 
 * @return char* 
 *         返回指向WSS请求头信息字符串的指针
 */
char *c_mmi_get_wss_header(void);

/**
 * @brief 设置设备的IP地址信息
 *
 * 此函数用于设置设备的IP地址，该信息将被包含在客户端信息中发送给服务端
 * 
 * @param ip 指向IP地址字符串的指针
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_set_ip(char *ip);

/**
 * @brief 设置设备的地理位置坐标
 *
 * 此函数用于设置设备的经度和纬度信息，该信息将被包含在客户端信息中发送给服务端
 * 
 * @param longitude 经度值
 * @param latitude 纬度值
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_set_postion(double longitude, double latitude);

/**
 * @brief 设置设备所在城市信息
 *
 * 此函数用于设置设备所在的城市信息，该信息将被包含在客户端信息中发送给服务端
 * 
 * @param city 指向城市名称字符串的指针
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_set_city(char *city);

/**
 * @brief 初始化mmi
 * 
 * 该函数用于初始化mmi，仅后付费模式需要调用该接口
 * 
 * @param workspace WorkSpaceId，通过百炼平台获取
 * @param app_id AppId，由阿里云颁发，字符串格式
 * @param api_key 通过百炼平台获取
 * 
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_init(char *workspace, char *app_id, char *api_key);

/**
 * @brief 反初始化MMI模块
 *
 * 此函数用于释放MMI模块占用的所有资源，并重置模块状态
 * 
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_deinit(void);

/**
 * @brief 配置提示词参数
 *
 * 此函数用于配置用户自定义的提示词参数
 * 
 * @param param_list 指向提示词参数结构体数组的指针
 * @param list_size 参数数组中元素的个数
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_data_set_prompt_param(c_mmi_prompt_pram_t *param_list, uint32_t list_size);

/**
 * @brief 向录音缓冲区写入数据
 *
 * @param data 待写入的数据指针
 * @param size 数据长度（字节）
 * @return uint32_t 实际写入的字节数
 */
uint32_t c_mmi_put_recorder_data(uint8_t *data, uint32_t size);

/**
 * @brief 从播放缓冲区读取数据
 *
 * @param data 用于存储读取数据的缓冲区
 * @param size 可用缓冲区大小（字节）
 * @return uint32_t 实际读取的字节数
 */
uint32_t c_mmi_get_player_data(uint8_t *data, uint32_t size);

/**
 * @brief 清理录音和播放缓冲区中的所有音频数据
 * 
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_clean_audio_data(void);

uint8_t c_mmi_is_inited(void);

/**
 * @brief 判断 SDK 当前是否正在工作中（SDK处于初始化过程中返回0）
 * 
 * @return uint8_t 1表示正在工作中，0表示未工作
 */
uint8_t c_mmi_is_working(void);

/**
 * @brief 初始化 MMI
 * 
 * 该函数完成协议握手、设备注册、token 获取等初始化流程。
 * 
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_sdk_init(void);

/**
 * @brief 设置 Agent 接管模式开关
 * 
 * @param onoff 1表示开启 Agent 接管，0表示关闭
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_agent_takeover(uint8_t onoff);

/**
 * @brief 设置语音功能开关
 * 
 * @param enable 1表示开启语音，0表示关闭语音
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_speech_set(uint8_t enable);

/**
 * @brief 获取语音功能开关状态
 * 
 * @return uint8_t 1表示语音开启，0表示语音关闭
 */
uint8_t c_mmi_speech_is_work(void);

/**
 * @brief 更新心跳时间戳
 * 
 * @param cur_time 当前时间戳，单位毫秒
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_heartbeat_update(int64_t cur_time);

/**
 * @brief 设置自动语音模式
 * 
 * @param enable 1表示开启自动语音，0表示关闭
 * @return int32_t 返回操作结果，0表示成功，非0表示失败
 */
int32_t c_mmi_set_auto_speech(uint8_t enable);

/**
 * @brief 获取自动语音模式状态
 * 
 * @return uint8_t 1表示自动语音已开启，0表示关闭
 */
uint8_t c_mmi_get_auto_speech(void);

/**
 * @brief 将任务 ID 字节数组转换为十六进制字符串
 * 
 * @param task_id 16字节任务 ID 数组
 * @return char* 指向十六进制字符串的指针
 */
char *c_mmi_task_id_string(uint8_t task_id[16]);

#include "c_mmi_config.h"
#include "c_mmi_msg.h"

#ifdef __cplusplus
}
#endif

#endif
