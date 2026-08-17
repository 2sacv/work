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
 * @file    iv_voip.h
 * @brief   Description audio and video function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_VOIP_H__
#define __IV_VOIP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>

#include "iv_cm.h"

#define VOIP_MAX_ID_LEN 80

typedef enum voip_wxa_type {
    VOIP_WXA_TYPE_RELEASE = 0,  // 小程序正式版
    VOIP_WXA_TYPE_DEBUG = 1,    // 小程序开发版
    VOIP_WXA_TYPE_DEMO = 2,     // 小程序体验版
} voip_wxa_type_e;

typedef enum voip_recv_v_rotation {
    VOIP_RECV_V_ROTATE_NONE = 1,    // 正向流
    VOIP_RECV_V_ROTATE_ROT =  2,     // 保持发出旋转流
} voip_recv_v_rot_e;

typedef enum voip_recv_video_fps {
    VOIP_RECV_V_FPS_MIN = 3,    
    VOIP_RECV_V_FPS_3 = 3,
    VOIP_RECV_V_FPS_4 = 4,
    VOIP_RECV_V_FPS_5 = 5,
    VOIP_RECV_V_FPS_6 = 6,
    VOIP_RECV_V_FPS_7 = 7,
    VOIP_RECV_V_FPS_8 =  8,    
    VOIP_RECV_V_FPS_9 =  9,    
    VOIP_RECV_V_FPS_10 = 10,    
    VOIP_RECV_V_FPS_11 = 11,    
    VOIP_RECV_V_FPS_12 = 12,    
    VOIP_RECV_V_FPS_13 = 13,    
    VOIP_RECV_V_FPS_14 = 14,    
    VOIP_RECV_V_FPS_15 = 15,    
    VOIP_RECV_V_FPS_MAX = 15,      
} voip_recv_v_fps_e;

typedef enum _voip_called_status {
    VOIP_CALLED_STATUS_ACCEPT = 0,
    VOIP_CALLED_STATUS_BUSY   = 9,
    VOIP_CALLED_STATUS_REFUSE = 11,
    VOIP_CALLED_STATUS_BUTT,
} iv_voip_called_status_e;
/* 暂不支持 发H264,收H265 组合*/
typedef struct voip_video_info_s {
    iv_cm_venc_type_e send_v_enc;  // 设备上行发送视频编码格式
    iv_cm_venc_type_e recv_v_enc;  // 设备下行接收视频编码格式
    iv_cm_pixel_type_e recv_pixel; // 设备下行接收视频分辨率
    iv_cm_mjpg_pixel_s recv_mjpg_pixel;  // 设备下行接收视频分辨率,当视频格式为mjpg时设置生效
    iv_cm_aenc_type_e recv_aenc;   // 设备下行接收音频编码格式:voip仅支持aac和pcm格式
    voip_recv_v_fps_e u32Framerate;// 设备下行接收视频帧率,取值范围[3,15]
    voip_recv_v_rot_e   recv_v_rot;  // 设备下行接收视频旋转，1:正向，2:保持发出旋转流
} voip_video_info_s;

typedef enum {
    VOIP_ACT_TEST     = 0, // 测试激活码
    VOIP_ACT_IPC      = 1, // 家庭安防场景
    VOIP_ACT_WEARABLE = 2, // 可穿戴设备
    VOIP_ACT_LIFE     = 3, // 生活娱乐场景
    VOIP_ACT_OTHER    = 4, // 对讲和其他场景
} voip_activate_type_e;

typedef struct voip_activate_info_s {
    char model_id[VOIP_MAX_ID_LEN];
    char device_id[VOIP_MAX_ID_LEN];
    int64_t expire_time;
} voip_activate_info_s;

typedef struct voip_callback_func_s {
    int (*iv_avt_voip_update_authorize_status_cb)(char *open_id, int status);
    int (*iv_avt_voip_join_notify_cb)(char *data);
    int (*iv_avt_voip_cancel_notify_cb)(char *data);
} voip_callback_func_s;

/**
 * @brief 初始化voip功能（停止维护，请使用v2接口）
 *
 * @param type 小程序类型
 * @param data_path voip 自动生成的设置文件存储路径
 * @param model_id voip model_id
 * @param device_id voip device_id
 * @param wxa_appid voip 微信小程序 wxa_appid
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_init(voip_wxa_type_e type, const char *data_path, const char *model_id, const char *device_id,
                     const char *wxa_appid);

/**
 * @brief 检测设备是否已经注册（停止维护，请使用v2接口）
 * 需要在 iv_avt_voip_init 调用完成之后才能调用本函数
 * 
 * @param is_reg 输出设备注册情况
 * @return int 检测过程中是否出现错误, 比如数据文件夹不可读写, 
 * WXERROR_OK: 正确返回, 可以正确使用 is_reg 的值来判断设备是否注册过
 * WXERROR_FAILED_PRECONDITION: wx_init 未被调用
 * WXERROR_INVALID_ARGUMENT: 参数为空
 * WXERROR_INVALID_DEVICEID: 设备已注册，但当前 sdk 用的 deviceid、modelid 不对
 * WXERROR_UNKNOWN: 注册信息被破坏，这种情况下一般可以清理设备的 data_path 路径然后重新注册
 */
int iv_avt_voip_is_registered(int *is_reg);

/**
 * @brief 注册设备（停止维护，请使用v2接口）
 * 设备初次使用或清理 data_path 后需要调用一次此接口进行注册
 * 若设备已经注册，则立即返回
 * 若设备未注册或注册数据错误，会再次注册
 * 
 * @param ticket 小程序对应的 snticket，有效期5分钟，获取后请尽快注册
 * 获取方法请参考：
 * https://developers.weixin.qq.com/miniprogram/dev/OpenApiDoc/hardware-device/getSnTicket.html
 * @return int 
 * WXERROR_RESOURCE_EXHAUSTED: 内存资源分配失败
 * WXERROR_RESPONSE: 后台返回失败，一般是网络问题
 * WXERROR_IO: IO 失败，一般是写文件接口返回失败
 * 其它：-10008: snticket 有问题，常见原因是 snticket 过期
 */
int iv_avt_voip_register(const char *ticket);

/**
 * @brief 退出voip功能（停止维护，请使用v2接口）
 * 
 */
void iv_avt_voip_exit();

/**
 * @brief voip呼叫（停止维护，请使用v2接口）
 * 
 * @param type 呼叫类型，音视频，或仅音频
 * @param open_id voip open_id
 * @param device_id voip device_id
 * @param model_id voip model_id
 * @param wxa_appid voip 微信小程序 wxa_appid
 * @param custom_message 自定义消息，长度不超过256字节，不得含有非法URL字符，如有非法字符或传输二进制数据请自行编码处理
 * @param v_info 设置希望接收到的视频数据格式，仅当呼叫类型为音视频时生效
 * @param caller_camera_switch 主叫端摄像头开关，0关闭，1开启，如果设备端不具备摄像头或不需要开启摄像头，请设置为关闭
 * @param callee_camera_switch 被叫端摄像头开关，0关闭，1开启，如果设备端不具备屏幕或不需要查看微信用户的摄像头内容，请设置为关闭
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_call(iv_cm_stream_type_e type, const char *open_id, const char *device_id,
                     const char *model_id, const char *wxa_appid, const char *custom_message,
                     voip_video_info_s v_info, uint32_t caller_camera_switch, uint32_t callee_camera_switch);

/**
 * @brief 呼叫挂断（停止维护，请使用v2接口）
 * 
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_hang_up();

/**
 * @brief 是否占线（停止维护，请使用v2接口）
 * 
 * @return int 0无占线，1占线
 */
int iv_avt_voip_is_busy();

/**
 * @brief 初始化voip功能
 *
 * @param type 小程序类型
 * @param data_path 自动生成的设置文件存储路径
 * @param model_id 微信小程序 model_id
 * @param wxa_appid 微信小程序 wxa_appid
 * @param functions 回调函数
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_init_v2(voip_wxa_type_e type, const char *data_path, const char *model_id,
                        const char *wxa_appid, voip_callback_func_s functions);

/**
 * @brief voip呼叫
 * 
 * @param type 呼叫类型，音视频，或仅音频
 * @param open_id 微信用户 open_id
 * @param custom_message 自定义消息，长度不超过256字节，不得含有非法URL字符，如有非法字符或传输二进制数据请自行编码处理
 * @param v_info 设置希望接收到的视频数据格式，仅当呼叫类型为音视频时生效
 * @param caller_camera_switch 主叫端摄像头开关，0关闭，1开启，如果设备端不具备摄像头或不需要开启摄像头，请设置为关闭
 * @param callee_camera_switch 被叫端摄像头开关，0关闭，1开启，如果设备端不具备屏幕或不需要查看微信用户的摄像头内容，请设置为关闭
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_call_v2(iv_cm_stream_type_e type, const char *open_id, const char *custom_message,
                        voip_video_info_s v_info, uint32_t caller_camera_switch, uint32_t callee_camera_switch);

/**
 * @brief 加入voip通话
 * 
 * @param room_id 房间号
 * @param v_info 设置通话的视频数据格式 
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_join_v2(const char *room_id, voip_video_info_s v_info, iv_voip_called_status_e status);

/**
 * @brief 呼叫挂断
 * 
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_hang_up_v2();

/**
 * @brief 是否占线
 * 
 * @return int 0无占线，1占线
 */
int iv_avt_voip_is_busy_v2();

/**
 * @brief 激活设备
 * 
 * @param type 设备类型
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_activate_license_v2(voip_activate_type_e type);

/**
 * @brief 获取激活设备信息
 * 
 * @param info 设备激活信息
 * @return int 0正常，其他值异常
 */
int iv_avt_voip_get_active_device_info_v2(voip_activate_info_s *info);

/**
 * @brief 获取微信用户列表
 * 单次最多获取10个用户，超过10个用户请指定offset并分多次获取
 * 
 * @param offset 偏移量
 * @param limit 单次获取用户数
 * @param openid_list 用户openid列表，必须大于等于limit
 * @return int 大于等于0为用户总数，小于0为错误码
 */
int iv_avt_voip_get_user_list_v2(int32_t offset, int32_t limit, char openid_list[][VOIP_MAX_ID_LEN]);

/**
 * @brief 退出voip功能
 * 
 */
void iv_avt_voip_exit_v2();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_AV_H__ */
