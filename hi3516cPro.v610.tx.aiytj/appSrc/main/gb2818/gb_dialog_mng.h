/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_dialog.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 sip 会话管理
*/

#ifndef GB_DIALOG_H_
#define GB_DIALOG_H_

#include <stdint.h>

#include <vector>

#include "osipparser2/osip_message.h"

#include "gb_sdp_parse.h"

/*  sip 消息分为请求和回复:
*       请求一共有：INVITE,ACK,BYE,CANCEL,OPTIONS,REGISTER,MESSAGE,INFO,SUBSCRIBE,NOTIFY,REFER,REDIRECT
*       回复消息  : 1XX,2XX,3XX,4XX,5XX,6XX
*/

/*基类*/
class Dialog {
public:
    osip_call_id_t *get_call_id() {
        return call_id_;
    }

    osip_from_t *get_from() {
        return from_;
    }

    osip_to_t *get_to() {
        return to_;
    }

    bool IsTimeout() {
        time_t current_time = mono_sec();
        return (current_time - initial_time_ > timeout_) ? true : false;
    }

    bool IsMatch(osip_message_t* message);

    /*from 和 to 初始化的时候不会被设置，提供函数给外部设置*/
    int set_from(osip_from_t *from);
    int set_to(osip_to_t *to);

    void set_timeout(time_t timeout) {
        timeout_ = timeout;
    }

protected:
    Dialog(osip_message_t* message, time_t timeout_s);
    Dialog(time_t timeout_s);
    virtual ~Dialog() { /*子类如果有特殊资源需要释放需要重写析构函数*/
        Clear();
    }

    void Clear();/*基类资源释放函数*/

    int set_call_id(osip_call_id_t *call_id);
private:
    /*对话由 Call-ID, From-tag 和 To-tag 确定，但是大部分不会严格要求，经测试海康只保证 call_id_ 匹配*/
    osip_call_id_t *call_id_ = NULL;
    osip_from_t *from_ = NULL;
    osip_to_t *to_ = NULL;

    /*对话超时时间，应对意外情况对话不能正常结束时强制结束*/
    time_t timeout_ = 20;    /*对话超时时间 单位：秒*/
    time_t initial_time_ = 0; /*对话添加时间 单位：秒*/
};

/*Invite 对话子类*/
class InviteDialog : public Dialog {
public:
    enum class Type{
        UNKOWN = 0,
        PLAY,      // 直播或者广播
        PLAYBACK,  // 回放
        DOWNLOAD,  // 下载
        BROADCAST, // 广播
    };
public:
    InviteDialog(osip_message_t* message);
    ~InviteDialog() { /*子类的析构函数一定要被实现，这样才能保证多态的时候，聚合类的资源被释放*/
        Clear();
    }

    void set_type(Type type) {
        type_ = type;
    }
    Type get_type() const {
        return type_;
    }

    SdpParse &get_sdp_parse() {
        return sdp_parse_;
    }
private:
    SdpParse sdp_parse_;  // sdp 解析，每一个 Invite 对话都包含
    Type type_ = Type::UNKOWN;
};

/*Message 对话子类*/
class MessageDialog : public Dialog {
public:
    enum class Type{
        UNKOWN = 0,
        QUREY_RECORD,      // 录像列表查询
        NOTIFY_BROADCAST,  // 广播通知
        NOTIFY_HEARTBEAT,  // 心跳
        CONCTRL_SNAPSHOT,  // 抓拍
        CONCTRL_UPGRADE,   // 软件升级
    };
public:
    MessageDialog(osip_message_t* message) : Dialog(message, 20) {};
    MessageDialog(Type type) : Dialog(10), type_(type) {};
    MessageDialog() : Dialog(10) {};

    ~MessageDialog() = default;

    int SetDialogInfo(osip_message_t* message);

    void set_type(Type type) {
        type_ = type;
    }
    Type get_type() const {
        return type_;
    }

    void set_sn(uint32_t sn) {
        sn_ = sn;
    }
    uint32_t get_sn() const {
        return sn_;
    }

    void set_session_id(const char *session_id) {
        session_id_ = session_id;
    }
    std::string &get_session_id() {
        return session_id_;
    }
    void set_device_id(const char *device_id) {
        device_id_ = device_id;
    }
    const char *get_device_id() {
        return device_id_.c_str();
    }

    void set_firmware(const char *firmware) {
        firmware_ = firmware;
    }
    const char *get_firmware() {
        return firmware_.c_str();
    }
private:
    uint32_t sn_ = 0;      //命令序列号，标记 xml 对话
    std::string device_id_;  // device_id 根据会话内容，可能为设备 id 或者 编码通道 id 等
    std::string session_id_;  // 抓拍使用，包含在 xml 中
    std::string firmware_;     // 固件版本，软件升级使用
    Type type_ = Type::UNKOWN;
};

/*Register 对话子类*/
class RegisterDialog : public Dialog {
public:
    enum class Type{
        UNKOWN = 0,
        REGISTER,  // 注册
        LOG_OUT,   // 注销
    };
public:
    RegisterDialog(osip_message_t* message, Type type, time_t timeout) : Dialog(message, timeout), type_(type) {};
    ~RegisterDialog()  = default;

    void set_type(Type type) {
        type_ = type;
    }
    Type get_type() const {
        return type_;
    }
private:
    Type type_ = Type::UNKOWN;
};

class GbDialogMng {
public:
    GbDialogMng(int max) : dialog_arr_(max) {};
    ~GbDialogMng() {PopAllDialog();};

    /*添加对话*/
    int AddDialog(std::shared_ptr<InviteDialog> &ptr);
    int AddDialog(std::shared_ptr<MessageDialog> &ptr);
    int AddDialog(std::shared_ptr<RegisterDialog> &ptr);
    /*弹出对话*/
    int PopDialog(osip_message_t* message, std::shared_ptr<InviteDialog> &ptr);
    int PopDialog(osip_message_t* message, std::shared_ptr<MessageDialog> &ptr);
    int PopDialog(osip_message_t* message, std::shared_ptr<RegisterDialog> &ptr);
    /*弹出所有对话*/
    void PopAllDialog();

private:
    std::vector<std::shared_ptr<Dialog>> dialog_arr_;
};

#endif