/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_client_manager.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 客户端管理:
                    - 国标服务初始化，反初始化
                    - 报警消息处理
                    - 参数改变处理
                    - 配置参数管理
*/
#ifndef GB_CLIENT_MANAGER_H_
#define GB_CLIENT_MANAGER_H_

/*c 系统文件*/
#include <time.h>
/*c++ 系统文件*/
#include <vector>
#include <memory>
/*其它库 .h*/
#include "jconfstruct.h"
#include "js_scheduler.h"
#include "osip2/osip.h"
#include "osipparser2/osip_message.h"
#include "osipparser2/osip_parser.h"
#include "mxml.h"
/*本项目内*/
#include "gb_record_mng.h"
#include "gb_profile.h"
#include "gb_socket.h"
#include "gb_live_streaming.h"
#include "gb_broadcast.h"
#include "gb_dialog_mng.h"
#include "gb_common.h"
#include "gb_snapshot.h"

enum ClientEvent {
    EVENT_UNKNOWN = 0,
    ALARM_MOVE,       // 移动侦测
    ALARM_HUMAN,      // 人形侦测
    ALARM_LINE,       // 越线侦测
    ALARM_RECT,       // 区域侦测
    ALARM_CAB_DIS,    // 网口断开
    ALARM_CAB_NOR,    // 网口连接

    // sip 内部事件
    SNAPSHOT_END,     // 图片抓拍完成
    RECORD_END,       // 录像播放完全
};

/*ONDUTY 和 OFFDUTY 要通过 DeviceContro(设备控制) 的 GuardCmd 来切换*/
enum DutyStatus {
    ONDUTY = 0,     // 系统处于活动状态。正在监视并处理可能发生的事件或警报
    OFFDUTY,        // 系统处于非警戒状态。这种状态下监控不会报警
    ALARM,          // 设备处于报警状态。报警上报时需要设置设备为这种状态，这种状态需要通过 DeviceContro(设备控制) 的 AlarmCmd(报警清除) 来转为 ONDUTY
};

#define RECORD_NUM  2  // 一次最大回复的录像数量

#define CLIENT_LOOP_TIME 1 * 1000


class GbClientManager : public std::enable_shared_from_this<GbClientManager> {
private:
    struct TimeSegment {
        int start_hour = 0;
        int start_min = 0;
        int start_sec = 0;
        int stop_hour = 0;
        int stop_min = 0;
        int stop_sec = 0;
    };

    struct RecordWeekDayPlan {
        // 国标规定，一天的录像计划最多 8 段
        GbClientManager::TimeSegment time_segment[8];
        int num = 0;

        int AddTimeSegment(int start_hour, int start_min, int start_sec, int stop_hour, int stop_min, int stop_sec);
    };

    struct RecordWeeklyPlan {
        GbClientManager::RecordWeekDayPlan week_day[7];
        int vaild_count = 0;  // 有效的天数

        /*jco to gb record plan*/
        int JcoToGbRecordPlan(unsigned int *timestrategy);

        /*gb record plan to jco*/
        int GbRecordPlanToJco(unsigned int *timestrategy);
    };

    /*国标编码支持*/
    enum EncodingType {
        UTF_8,
        GB2312,
    };

    /*升级状态*/
    enum UpgradeStatus {
        UPGRADE_SUCCESS = 0,        // 升级成功
        TIMEOUT = 1,        // 软件下载超时
        PACKAGE_DAMAGE = 2, // 升级包损坏
        SYSTEM_ERR = 3,     // 系统异常
        OTHER_ERR = 99,     // 其它错误
    };

public:
    GbClientManager();
    ~GbClientManager();

    /*注册状态*/
    bool IsRegister() const {
        return register_;
    }

    /*停止全部录像*/
    inline void StopClientPlayback() {
        if (record_mng_ != NULL) {
            record_mng_->ClearPlayList();
        }
    }

    /*录像播放完成，发送报文通知服务器*/
    void SendPlayComplete();

    std::shared_ptr<GbClientManager> GetSharedPtr() {
        // 使用 shared_from_this() 安全地获取 shared_ptr
        try {
            auto self = shared_from_this();
            return self;
        } catch (const std::bad_weak_ptr&) {
            GB_ERR("shared_ptr is not init\n");
        }

        return nullptr;
    }

    /*获取版本字符*/
    const char *GetGbVersionStr() {
        return profile_.GetGbVersionStr();
    }

    /*事件队列*/
    static SeqQueue<ClientEvent, 16> event_queue_;

    static bool Enqueue(const ClientEvent &event) {
        return event_queue_.Enqueue(event);
    }
    static bool Dequeue(ClientEvent &event) {
        return event_queue_.Dequeue(event);
    }

    /*升级相关*/
    bool is_upgrading() { // 是否正在升级
        return upgrade_dialog_ != nullptr;
    }
private:
    /*编码转换*/
    std::string ChangeEncoding(const char *src_str, GbClientManager::EncodingType src_enc, GbClientManager::EncodingType dst_enc);

    /*状态设置和获取*/
    bool NeedStopOnce() {  // 是否需要反注册一次
        return stop_once_;
    }
    void SetStopOnce() { // 设置停止一次
        stop_once_ = true;
    }

    bool IsEnable() const {  // 国标是否使能
        return profile_.get_enable();
    }

    /*报警状态设置和获取*/
    const char *GetDutyText() const {
        if (duty_status_ == ONDUTY)
            return "ONDUTY";
        else if (duty_status_ == OFFDUTY)
            return "OFFDUTY";

        return "ALARM";
    }
    DutyStatus get_duty_status() const {
        return duty_status_;
    }
    void set_duty_status(DutyStatus duty_status) {
        duty_status_ = duty_status;
    }

    /*获取网络参数*/
    int GetNetInfo(char *ip, char *mac, int *mtu);

    /*事件处理*/
    static void HandleAlarmMessages(int id, void *p_src, int size, void *instance);
    static void HandleConfigMessages(int id, void *p_src, int size, void *instance);
    static void HandleGuoBiaoCfgChg(void *data);
    static void HandleGuoBiaoAddrCfgChg(void *data);
    static void HandleNetCfgChg(void *data);

    /*数据接收处理*/
    uint32_t HandleTcpMessage(const char *buff, uint32_t data_size);
    int HandleSipMessage(const char *buff, uint32_t data_size);
    static void HandleSocketRead(int fd, int events, void *userdata);

    /*注册，注销，心跳，刷新注册，报警上传*/
    static void ClientLoop(void *this_ptr);
    void reportAlarmInfo(int type);
    int StartSipServer();
    int StopSipServer();
    GbSocket *ConnectSipServer();
    void HandleRegister(osip_message_t *message);
    int HandleRegisterFor401(osip_message_t *message, RegisterDialog::Type type);
    int RefreshRegister();
    int DigestCalcAuth(osip_www_authenticate_t *www_auth, osip_message_t *request, char *out);
    void CvtoHex(char *Bin, char *Hex);
    int SipLogOut();
    void DoHeartbeat();
    void SyncTimeWithServer(const char* time_str);

    /*sip 发送*/
    int SipToStringAndSend(osip_message_t* message, const char *content_type = NULL, const char *body = NULL, uint32_t body_len = 0);
    void MessageRequestAddXmlAndSend(mxml_node_t *reply_xml, std::shared_ptr<MessageDialog> &ptr);
    int Send200Ok(osip_message_t *message, const char *content_type = NULL, const char *body = NULL, uint32_t body_len = 0, bool need_contact = false);
    int Send100Tring(osip_message_t *message);
    int SendAck(osip_message_t *message, const char *user);

    /*sip message 生成*/
    osip_message_t *GenerateRequest(const char *method, uint32_t serial, const char *user, bool need_contact = false, osip_call_id_t *call_id = NULL, osip_from_t *from = NULL, osip_to_t *to = NULL);
    osip_message_t *GenerateRequestForResponse(osip_message_t* response, const char *method, const char *user, bool need_contact = false);
    osip_message_t *GenerateRegesterSip(uint32_t cseq,uint32_t expires);
    osip_message_t *GenerateResponseForRequest(osip_message_t* request, int status_code, bool need_contact = false, const char *user = NULL);

    int SetUri(osip_message_t *message);
    int SetTo(osip_message_t *message, osip_to_t *to);
    int SetFrom(osip_message_t *message, const char *user, osip_from_t *from);
    int SetCseq(osip_message_t *message, const char* method, uint32_t serial_number);
    int SetCallId(osip_message_t *message, osip_call_id_t *call_id);
    int SetVia(osip_message_t *message, osip_list_t *vias);
    int SetSubject(osip_message_t *message);
    int SetContact(osip_message_t *message,  const char *user);

    /*xml 生成*/
    mxml_node_t *GenerateXmlResponse(uint32_t sn, const char *cmd_type);
    mxml_node_t *GenerateXmlResponse(mxml_node_t* root, const char *result = NULL);
    mxml_node_t *GenerateXml(const char *root_methond, uint32_t sn, const char *cmd_type, const char* result, const char *device_id);
    mxml_node_t *GenerateRecordXmlResponse(uint32_t sn, bool is_first);

    /*sip 消息不同方法处理入口函数*/
    void HandleMessage(osip_message_t *message);
    void HandleInvite(osip_message_t *message);
    void HandleAck(osip_message_t *message);
    void HandleBye(osip_message_t *message);
    void HandleSubscribe(osip_message_t *message);
    void HandleNotify(osip_message_t *message);
    void HandleInfo(osip_message_t *message);

    /*录像查询播放相关*/
    mxml_node_t *HandRecordInfo(mxml_node_t* root, uint32_t *user_data);
    mxml_node_t *HandQueryDownload(mxml_node_t* root);
    mxml_node_t* HandPresetInfo(mxml_node_t* root);
    mxml_node_t *HandQueryMobilePosition(mxml_node_t* root);
    void HandQueryMessage(mxml_node_t* root, const char *cmd_type);
    void HandNotifyBroadcast(mxml_node_t* root);
    int HandleRecordResponse(osip_message_t *response, std::shared_ptr<MessageDialog> &dialog);
    time_t TextTimeToUtc(const char *text);

    /*sip message 相关处理 */
    void HandleMessageRquest(osip_message_t *request);
    int HexStringToInt(char *buf, unsigned int seq);
    void HandPtzCmd(mxml_node_t* ptz_cmd);
    int GbJcpSend(const char *format, ...);
    int ParsePtzCmdText(const char *text, uint8_t &cmd, uint8_t &data1, uint8_t &data2, uint8_t &data3);
    void HandGuardCmd(mxml_node_t* guard_cmd);
    void HandDevideConfig(mxml_node_t* root);
    void HandSnapshot(mxml_node_t* root, std::shared_ptr<MessageDialog> &ptr);
    void SetBasicParm(mxml_node_t *root);
    void SetSVACEncodeParmJCP(mxml_node_t *root);
    void SetSVACDecodeParmJCP(mxml_node_t *root);
    void HandRecordCmd(mxml_node_t* record_cmd);
    void HandStreamNumber(mxml_node_t* stream_number);
    void SetHomePosition(mxml_node_t *root);
    void HandHomePosition(mxml_node_t* root, mxml_node_t* home_position);
    void SetSvacEncodeParm(mxml_node_t *root);
    void SetSvacDecodeParm(mxml_node_t *root);
    void HandDeviceControl(mxml_node_t* root);
    mxml_node_t *HandCatalog(mxml_node_t* root);
    mxml_node_t *HandDeviceInfo(mxml_node_t* root);
    void TimenowToServerFormat(char *buf, size_t buf_size);
    mxml_node_t *HandDeviceStatus(mxml_node_t* root);
    void HandleMessageResponse(osip_message_t *response);
    void SendSnapshotEnd();
    void HandleQuryVideoRecordPlan(mxml_node_t* response);
    void HandleQuryOSDConfig(mxml_node_t* response);
    void SetVideoParamAttribute(mxml_node_t *video_param_attribute);
    void SetVideoRecordPlan(mxml_node_t *video_record_plan);
    void SetVideoAlarmRecord(mxml_node_t *video_alarm_record);
    void SetPictrueMask(mxml_node_t *pictrue_mask);
    void SetFrameMirror(mxml_node_t *frame_mirror);
    void SetAlarmReport(mxml_node_t *alarm_report);
    void SetOSDConfig(mxml_node_t *osd_config);
    mxml_node_t *HandQuerySDCardStatus(mxml_node_t* root);
    void SendUpgradeResult(GbClientManager::UpgradeStatus status);
    void HandDeviceUpgrade(mxml_node_t* root, std::shared_ptr<MessageDialog> &ptr);

    /*sip invite 相关处理，直播、录像、对讲*/
    void HandleInviteResponse(osip_message_t *message);
    void HandleSdpPlay(SdpParse &sdp_parse, char *out_buf);
    void HandleSdpPlayback(SdpParse &sdp_parse, char *out_buf);
    void HandleSdpDownload(SdpParse &sdp_parse, char *out_buf);
    void HandleInviteRquest(osip_message_t *request);
    void HandQuerySubscribe(mxml_node_t* root_xml);
    void InitiateBroadcast();
    uint32_t GenerateSsrc(bool is_live);

private:
    /*调度*/
    JSScheduler sch_message_; // 事件和接收消息处理
    JSScheduler sch_streaming_;  // 推流处理
    JSTCHandle  hdl_loop_;
    JSRWHandle  hdl_socker_read_;

    /*定时器*/
    Timer register_timer_;
    Timer heart_timer_;
    Timer refresh_register_timer_;
    Timer log_out_timer_;  // 注销

    /*xml 编码信息，只有字符串中带中文的情况要考虑*/
    EncodingType xml_encoding_type_ = GbClientManager::EncodingType::GB2312;

    /*状态信息*/
    bool register_;
    bool stop_once_;  // 设置停止一次
    DutyStatus duty_status_; // 报警状态
    uint32_t message_cseq_;  // cseq 计数
    uint32_t register_cseq_; // 注册计数
    //uint32_t sn_; // sn 计数
    uint32_t register_nc_;  // qop = auth 认证的时候，记录 nc 值
    uint32_t heartbeat_timeout_count_;  // 心跳超时计数

    /*软件升级*/
    std::shared_ptr<MessageDialog> upgrade_dialog_;

    /*配置*/
    GbProfile profile_;

    /*网络*/
    GbSocket *socket_ctrl_;
    char recv_buf_[SIP_MESSAGE_MAX_LENGTH];
    uint32_t recv_byte_;

    /*sip 会话*/
    GbDialogMng dialog_mng_;

    /*直播流推流管理*/
    std::shared_ptr<GbLiveStreamingMng> live_streaming_mng_;

    /*录像推流*/
    std::shared_ptr<GbRecordMng> record_mng_;

    /*对讲*/
    std::shared_ptr<GbBroadcast> broadcast_ctrl_;

    /*图片抓拍*/
    std::shared_ptr<GbSnapshot> snapshot_;
};

#endif