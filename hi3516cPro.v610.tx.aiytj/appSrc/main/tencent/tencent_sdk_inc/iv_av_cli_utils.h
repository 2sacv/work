//
// Created by monktan on 2024/3/20.
//

#ifndef IV_AV_CLI_UTILS_H
#define IV_AV_CLI_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


/* ########################################## 类型定义 ########################################################## */
typedef enum
{
    XP2PTypeClose            = 1000,  // 数据传输完成
    XP2PTypeLog              = 1001,  // 日志输出
    XP2PTypeCmd              = 1002,  // command json
    XP2PTypeDisconnect       = 1003,  // p2p链路断开
    XP2PTypeDetectReady      = 1004,  // p2p链路初始化成功
    XP2PTypeDetectError      = 1005,  // p2p链路初始化失败
    XP2PTypeDeviceMsgArrived = 1006,  // 设备端向App发消息
    XP2PTypeCmdNOReturn      = 1007,  // 设备未返回app自定义信令
    XP2PTypeStreamEnd        = 1008,  // 设备停止推流，或者由于达到设备最大连接数，拒绝推流
    XP2PTypeDownloadEnd      = 1009,  // 设备停止推流，下载结束
    XP2PTypeSaveFileOn       = 8000,  // 获取保存音视频流开关状态
    XP2PTypeSaveFileUrl      = 8001   // 获取音视频流保存路径
} XP2PType;

typedef enum
{
    XP2P_ERR_NONE             = 0,      // 成功
    XP2P_ERR_INIT_PRM         = -1000,  // 入参为空
    XP2P_ERR_GET_XP2PINFO     = -1001,  // SDK内部请求xp2p info失败
    XP2P_ERR_PROXY_INIT       = -1002,  // 本地p2p代理初始化失败
    XP2P_ERR_UNINIT           = -1003,  // 数据接收/发送服务未初始化
    XP2P_ERR_ENCRYPT          = -1004,  // 数据加密失败
    XP2P_ERR_TIMEOUT          = -1005,  // 请求超时
    XP2P_ERR_REQUEST_FAIL     = -1006,  // 请求错误
    XP2P_ERR_VERSION          = -1007,  // 设备版本过低，请升级设备固件
    XP2P_ERR_APPLICATION      = -1008,  // application初始化失败
    XP2P_ERR_REQUEST          = -1009,  // request初始化失败
    XP2P_ERR_DETECT_NOTREADY  = -1010,  // p2p探测未完成
    XP2P_ERR_P2P_ININED       = -1011,  // 当前id对应的p2p已完成初始化
    XP2P_ERR_P2P_UNININ       = -1012,  // 当前id对应的p2p未初始化
    XP2P_ERR_NEW_MEMERY       = -1013,  // 内存申请失败
    XP2P_ERR_XP2PINFO_RULE    = -1014,  // 获取到的xp2p info格式错误
    XP2P_ERR_XP2PINFO_DECRYPT = -1015,  // 获取到的xp2p info解码失败
    XP2P_ERR_PROXY_LISTEN     = -1016,  // 本地代理监听端口失败
    XP2P_ERR_CLOUD_EMPTY      = -1017,  // 云端返回空数据
    XP2P_ERR_JSON_PARSE       = -1018,  // json解析失败
    XP2P_ERR_SERVICE_NOTRUN   = -1019,  // 当前id对应的服务(语音、直播等服务)没有在运行
    XP2P_ERR_CLIENT_NULL      = -1020   // 从map中取出的client为空
} XP2PErrCode;

typedef enum
{
    XP2PVoiceServiceClose  = 2000,  // 语音对讲服务关闭
    XP2PStreamServiceClose = 2001   // 音视频流接收服务关闭
} XP2PCloseSubType;

#ifdef __cplusplus
}
#endif

#endif //IV_AV_CLI_UTILS_H
