/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_profile.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 配置文件:
                    - 保存配置信息
                    - 设置配置
                    - 获取配置
*/

#ifndef GB_PROFILE_H_
#define GB_PROFILE_H_

#include <string>

#include "jconfstruct.h"
#include "js_scheduler.h"

#include "gb_common.h"
#include "gb_socket.h"

class GbProfile {
public:
    GbProfile();

    ~GbProfile(){};

    /* 结构体初始化参数 */
    int SetGbParam(GuoBiaoS &gb_param);
    int SetGbAddress(GBAddrS &gb_location);
    int SetNetInfo(const char *ip, const char *mac, int mtu);

    /* 打印信息 */
    void PrintfGbParam();

    /* 使能 */
    bool get_enable() const {
        return enable_;
    }
    void set_enable(bool enable) {
        enable_=enable;
    }

    /* 传输协议 */
    void set_transport_protocols(NetworkProtocols transport_protocols) {
        transport_protocols_ = transport_protocols;
    }
    NetworkProtocols get_transport_protocols() const {
        return transport_protocols_;
    }
    const char *GetTcpOrUdp() {
        return transport_protocols_ == GB_TCP ? "TCP" : "UDP";
    }


    /* 设备生产厂商 */
    const char *get_manufacture() const {
        return manufacture_.c_str();
    }
    void set_manufacture(const char *manufacture) {
        manufacture_ = manufacture;
    }

    /* 平台归属 */
    const char *get_owner() const {
        return owner_.c_str();
    }
    void set_owner(const char *owner) {
        owner_ = owner;
    }

    /* 设备名称 */
    const char *get_dev_name() const {
        return dev_name_.c_str();
    }
    void set_dev_name(const char *dev_name) {
        dev_name_ = dev_name;
    }

    /* 设备型号 */
    const char *get_dev_model() const {
        return dev_model_.c_str();
    }
    void set_dev_model(const char *dev_model) {
        dev_model_ = dev_model;
    }

    /* sip 服务器 ID */
    const char *get_sip_server_id() const {
        return sip_server_id_.c_str();
    }
    void set_sip_server_id(const char *sip_server_id) {
        sip_server_id_ = sip_server_id;
    }

    /* sip 服务器域 */
    const char *get_civil_code() const {
        return sip_server_civil_code_.c_str();
    }
    void set_civil_code(const char *civil_code) {
        sip_server_civil_code_ = civil_code;
    }

    /* sip 服务器地址 */
    const char *get_sip_server_address() const {
        return sip_server_address_.c_str();
    }
    void set_sip_server_address(const char *sip_server_address) {
        sip_server_address_ = sip_server_address;
    }

    /* sip 服务器端口 */
    uint32_t get_sip_server_port() const {
        return sip_server_port_;
    }
    void set_sip_server_port(uint32_t sip_server_port) {
        sip_server_port_ = sip_server_port;
    }

    /* sip 用户名 */
    const char *get_sip_user_name() const {
        return sip_user_name_.c_str();
    }
    void set_sip_user_name(const char *sip_user_name) {
        sip_user_name_ = sip_user_name;
    }

    /* sip 用户认证 ID */
    const char *get_sip_user_auth_id() const {
        return sip_user_auth_id_.c_str();
    }
    void set_sip_user_auth_id(const char *sip_user_auth_id) {
        sip_user_auth_id_ = sip_user_auth_id;
    }

    /* sip 用户认证密码 */
    const char *get_sip_user_password() const {
        return sip_user_password_.c_str();
    }
    void set_sip_user_password(const char *sip_user_password) {
        sip_user_password_ = sip_user_password;
    }

    /* 视频通道编码 ID */
    const char *get_video_channal_id() const {
        return video_channal_id_.c_str();
    }
    void set_video_channal_id(const char *video_channal_id) {
        video_channal_id_ = video_channal_id;
    }

    /* 报警输入编码 ID */
    const char *get_alarmin_channal_id() const {
        return alarmin_channal_id_.c_str();
    }
    void set_alarmin_channal_id(const char *alarmin_channal_id) {
        alarmin_channal_id_ = alarmin_channal_id;
    }

    /* 语音输出通道编码 ID */
    const char *get_audioout_channal_id() const {
        return audioout_channal_id_.c_str();
    }
    void set_audioout_channal_id(const char *audioout_channal_id) {
        audioout_channal_id_ = audioout_channal_id;
    }

    /* 本地 sip 端口 */
    uint32_t get_sip_local_port() const {
        return sip_local_port_;
    }
    void set_sip_local_port(uint32_t local_port) {
        sip_local_port_ = local_port;
    }

    /* 注册间隔 */
    uint32_t get_register_interval() const {
        return register_interval_;
    }
    void set_register_interval(uint32_t register_interval) {
        register_interval_ = register_interval;
    }

    /* 心跳间隔 */
    uint32_t get_heartbeat_interval() const {
        return heartbeat_interval_;
    }
    void set_heartbeat_interval(uint32_t heartbeat_interval) {
        heartbeat_interval_ = heartbeat_interval;
    }

    /* 心跳超时次数 */
    uint32_t get_heartbeat_timeout_count() const {
        return heartbeat_timeout_count_;
    }
    void set_heartbeat_timeout_count(uint32_t heartbeat_timeout_count) {
        heartbeat_timeout_count_ = heartbeat_timeout_count;
    }

    /* 码流类型 */
    uint32_t get_stream_type() const {
        return stream_type_;
    }
    void set_stream_type(uint32_t stream_type) {
        stream_type_ = stream_type;
    }

    /* 注册有效期 */
    uint32_t get_term_of_register() const {
        return term_of_register_;
    }
    void set_term_of_register(uint32_t term_of_register) {
        term_of_register_ = term_of_register;
    }

    /* 位置信息 */
    const char *get_address_info() const {
        return address_info_.c_str();
    }
    void set_address_info(const char *address_info) {
        address_info_ =  address_info;
    }

    /*经度 */
    float get_longitude() const {
        return longitude_;
    }
    void set_longitude(float longitude) {
        longitude_ = longitude;
    }

    /* 纬度 */
    float get_latitude() const {
        return latitude_;
    }
    void set_latitude(float latitude) {
        latitude_ = latitude;
    }

    /* 本地ip */
    const char *get_local_ip() const {
        return local_ip_.c_str();
    }
    void set_local_ip(const char *local_ip) {
        local_ip_ =  local_ip;
    }

    /* 本地 mac */
    const char *get_local_mac() const {
        return local_mac_.c_str();
    }
    void set_local_mac(const char *local_mac) {
        local_mac_ =  local_mac;
    }

    /*mtu*/
    uint16_t get_mtu() {
        return mtu_;
    }
    void set_mtu(uint16_t mtu) {
        mtu_ = mtu;
    }

    /*发流端口*/
    uint16_t get_media_port() const {
        return media_port_;
    }
    void set_media_port(uint16_t media_port) {
        media_port_ =  media_port;
    }

    /*音频接收端口*/
    uint16_t get_broadcast_port() const {
        return broadcast_port_;
    }
    void set_broadcast_port(uint16_t broadcast_port) {
        broadcast_port_ =  broadcast_port;
    }

    /*sip 版本*/
    const char *get_sip_version() const {
        return sip_version_.c_str();
    }
    void set_sip_version(const char *sip_version) {
        sip_version_ = sip_version;
    }

    /*sip 代理*/
    const char *get_sip_agent() const {
        return sip_agent_.c_str();
    }
    void set_sip_agent(const char *sip_agent) {
        sip_agent_ = sip_agent;
    }
    /*sip 版本号，2022 版本协议添加*/
    const char *GetGbVersionNo() {
        // 获取 sip 版本号
        if (gb_version_ == 30) {
            return "3.0";
        } else {
            return "2.0";
        }
    }
    const char *GetGbVersionStr() {
        // (3.0)[2022] (2.0)[2016]  (1.0)[2011] (1.1)[2011修正]
        // 不支持低版本，3.0 以下全部按照 2016 处理
        if (gb_version_ == 30) {
            return "GB/T 28181-2022";
        } else {
            return "GB/T 28181-2016";
        }
    }
    bool IsGb2022() const {
        return gb_version_ == 30;
    }
private:
    /*状态信息*/
    bool enable_;    // 国标使能

    /*服务器配置*/
    std::string sip_server_id_; // sip 服务器 ID
    std::string sip_server_civil_code_;   // sip 服务器域
    std::string sip_server_address_; // sip 服务器 地址
    uint32_t sip_server_port_;  // sip 服务器端口
    /*设备配置*/
    std::string manufacture_; // 设备生产厂商
    std::string owner_;       // 平台归属
    std::string dev_name_;   // 设备名称
    std::string dev_model_;    // 设备型号
    std::string sip_user_name_; // sip 用户名
    std::string sip_user_auth_id_;   // sip 用户认证 ID
    std::string sip_user_password_;  // sip 用户认证密码
    std::string video_channal_id_;   // 视频通道编码 ID
    std::string alarmin_channal_id_;   // 报警输入编码 ID
    std::string audioout_channal_id_;   // 语音输出通道编码 ID
#ifdef GB28181_2022
    const int gb_version_ = 30;   // sip 版本号
#else
    const int gb_version_ = 20;   // sip 版本号
#endif
    uint32_t sip_local_port_;   // 本地 sip 端口
    uint32_t register_interval_; // 注册间隔 单位:秒 注册失败后多久尝试一次
    uint32_t heartbeat_interval_;  // 心跳间隔 单位:秒
    uint32_t heartbeat_timeout_count_; // 心跳超时次数
    uint32_t stream_type_; // 码流类型 0：主码流  1：子码流
    uint32_t term_of_register_; // 注册有效期,单位：秒  注册成功后刷新注册的时间
    NetworkProtocols transport_protocols_; // 传输协议 tcp/udp

    std::string address_info_; // 位置信息，安装位置
    float longitude_; // 经度
    float latitude_;  // 纬度

    /*ip 信息*/
    std::string local_ip_;   // 本地 IP
    std::string local_mac_;  // 本地 mac
    uint16_t mtu_; // 端口 mtu
    uint16_t reserve_; // 占位，字节对齐

    uint16_t media_port_; // 国标发流端口
    uint16_t broadcast_port_; // 国标音频接收端口

    std::string sip_version_;
    std::string sip_agent_;
};

#endif