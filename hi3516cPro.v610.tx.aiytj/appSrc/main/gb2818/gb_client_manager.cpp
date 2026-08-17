/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_client_manager.cpp
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 客户端管理:
                    - 国标服务初始化，反初始化
                    - 报警消息处理
                    - 参数改变处理
                    - 配置参数管理
*/
/*c 系统文件*/
#include <sys/vfs.h>

/*c++ 系统文件*/
#include <memory>

/*其它库 .h*/
#include "jconfstruct.h"
#include "js_scheduler.h"
#include "confapi.h"
#include "jconfig.h"
#include "jevent.h"
#include "osipparser2/osip_md5.h"
#include "jcpService.h"
#include "utf82gbk.h"
#include "delay_exec.h"
#include "encodeapi.h"
#include "recordapi.h"
#include "net_config.h"
#include "net_check.h"
#include "record_disk.h"
#include "g_stat.h"
#include "g_sys.h"
#include "g_log.h"
#include "update.h"

/*本项目内*/
#include "gb_common.h"
#include "gb_client_manager.h"
#include "gb_dialog_mng.h"
#include "gb_socket.h"
#include "gb_record_mng.h"

SeqQueue<ClientEvent, 16> GbClientManager::event_queue_;

static const char *whitespace_cb(mxml_node_t *node, int where)
{
    if (where == MXML_WS_AFTER_OPEN && node->child != NULL && node->child->child != NULL)
        return "\n";
    else if (where == MXML_WS_AFTER_CLOSE)
        return "\n";

    return NULL;
}

GbClientManager::GbClientManager() :
sch_message_(NULL), sch_streaming_(NULL), hdl_loop_(NULL), hdl_socker_read_(NULL),
register_(false), stop_once_(false), duty_status_(ONDUTY), message_cseq_(1), register_cseq_(1),
register_nc_(1), heartbeat_timeout_count_(0), socket_ctrl_(NULL),
recv_byte_(0), dialog_mng_(10)
{
    int ret = 0;

    sch_message_ = js_create_scheduler((char *)"gb_message");
    if (sch_message_ == NULL) {
        GB_ERR("Init sch error\n");
    }
    GB_INFO("create message scheduler success:%p\n", sch_message_);

    sch_streaming_ = js_create_scheduler((char *)"gb_streaming");
    if (sch_streaming_ == NULL) {
        GB_ERR("Init sch error\n");
    }
    GB_INFO("create streaming scheduler success:%p\n", sch_streaming_);

    /*获取配置*/
    GuoBiaoS gb_param = {0};
    ret = conf_get_guobiaocfg(&gb_param);
    if (ret != 0) {
        GB_ERR("get guobiao config error\n");
    }

    GBAddrS gb_location = {0};
    ret = conf_get_guobiaoaddr(&gb_location);
    if (ret != 0) {
        GB_ERR("get guobiao addr error\n");
    }

    char ip[64] = {0};
    char mac[64] = {0};
    int mtu = 1500;
    GetNetInfo(ip, mac, &mtu);

    /*设置参数*/
    profile_.SetGbParam(gb_param);
    profile_.SetGbAddress(gb_location);
    profile_.SetNetInfo(ip, mac, mtu);

    /*打印参数*/
    profile_.PrintfGbParam();

    /*sip 解析初始化*/
    parser_init();

    record_mng_ = std::make_shared<GbRecordMng>(profile_, sch_streaming_);
    if (record_mng_ == NULL) {
        GB_ERR("Init record mng fail\n");
        return ;
    }

    live_streaming_mng_ = std::make_shared<GbLiveStreamingMng>(sch_streaming_, profile_);
    if (live_streaming_mng_ == NULL) {
        GB_ERR("Init live streaming fail\n");
        return ;
    }

    broadcast_ctrl_ = std::make_shared<GbBroadcast>(sch_streaming_, profile_);
    if (broadcast_ctrl_ == NULL) {
        GB_ERR("Init broard cast fail\n");
        return ;
    }

    snapshot_ = std::make_shared<GbSnapshot>(profile_);
    if (snapshot_ == NULL) {
        GB_ERR("Init snapshot fail\n");
        return ;
    }

    /*事件关注*/
    attach_event(JEvent_AlarmMD,           HandleAlarmMessages, this);
    attach_event(JEvent_Alarmhumadetect,   HandleAlarmMessages, this);
    attach_event(JEvent_AlarmVgline,       HandleAlarmMessages, this);
    attach_event(JEvent_AlarmVgrect,       HandleAlarmMessages, this);

    attach_event(JEvent_AlarmCabDis      ,     HandleConfigMessages  , this);
    attach_event(JEvent_AlarmCableNormal ,     HandleConfigMessages  , this);
    attach_config(JEvent_EthcfgChg,            HandleConfigMessages, this);
    attach_config(JEvent_DhcpNotify,           HandleConfigMessages, this);
    attach_config(JEvent_WifiCfgChg,           HandleConfigMessages, this);
    attach_config(JEvent_GuoBiaoCfg,           HandleConfigMessages, this);
    attach_config(Jevent_GuoBiaoAddrCfg,       HandleConfigMessages, this);

    js_create_timer_r(sch_message_, 20, CLIENT_LOOP_TIME, ClientLoop, this, &hdl_loop_);
}

GbClientManager::~GbClientManager()
{
    GB_DBG("start deinit client manager\n");
    /*1. 事件分离*/
    detach_event(JEvent_AlarmMD,         HandleAlarmMessages, this);
    detach_event(JEvent_Alarmhumadetect, HandleAlarmMessages, this);
    detach_event(JEvent_AlarmVgline,     HandleAlarmMessages, this);
    detach_event(JEvent_AlarmVgrect,     HandleAlarmMessages, this);

    detach_event(JEvent_AlarmCabDis,     HandleConfigMessages, this);
    detach_event(JEvent_AlarmCableNormal,HandleConfigMessages, this);
    detach_config(JEvent_EthcfgChg,      HandleConfigMessages, this);
    detach_config(JEvent_DhcpNotify,     HandleConfigMessages, this);
    detach_config(JEvent_WifiCfgChg,     HandleConfigMessages, this);
    detach_config(JEvent_GuoBiaoCfg,     HandleConfigMessages, this);
    detach_config(Jevent_GuoBiaoAddrCfg, HandleConfigMessages, this);

    /*2. 停止会话服务*/
    js_delete_reader_r(&hdl_socker_read_);
    js_delete_timer_r(&hdl_loop_);

    snapshot_->StopSnapshot();

    if (live_streaming_mng_.use_count() != 1) {
        GB_ERR("live streaming mng use count:%ld\n", live_streaming_mng_.use_count());
    }
    live_streaming_mng_.reset();

    if (broadcast_ctrl_.use_count() != 1) {
        GB_ERR("broadcast ctrl use count:%ld\n", broadcast_ctrl_.use_count());
    }
    broadcast_ctrl_.reset();

    if (record_mng_.use_count() != 1) {
        GB_ERR("record mng use count:%ld\n", record_mng_.use_count());
    }
    record_mng_.reset();

    if (socket_ctrl_) {
        delete socket_ctrl_;
        socket_ctrl_ = NULL;
    }

    /*3. 关闭调度*/
    js_delete_scheduler(sch_message_);
    sch_message_ = NULL;
    js_delete_scheduler(sch_streaming_);
    sch_streaming_ = NULL;

    GB_DBG("start deinit client success\n");
    return;
}

/*注册，注销，心跳，刷新注册，报警上传*/
void GbClientManager::ClientLoop(void *data)
{
    GbClientManager *this_ptr = (GbClientManager *)data;
    ClientEvent event = EVENT_UNKNOWN;

    while (Dequeue(event)) {
        int type = -1;
        switch(event) {
            case ALARM_MOVE:
                type = 2;
                break;
            case ALARM_HUMAN:
                type = 1;
                break;
            case ALARM_LINE:
                type = 5;
                break;
            case ALARM_RECT:
                type = 6;
                break;
            case SNAPSHOT_END:
                this_ptr->SendSnapshotEnd();
                break;
            case RECORD_END:
                this_ptr->SendPlayComplete();
                break;
            default:
                break;
        }
        if (type != -1) {
            this_ptr->reportAlarmInfo(type);
        }
    }

    if (this_ptr->NeedStopOnce()) {
        /*停止一次服务，参数刷新，或者网络变化的时候使用*/
        if (this_ptr->StopSipServer() == SUCCESS) {
            this_ptr->stop_once_ = false;
            this_ptr->register_timer_.Start(1);
        }

    } else if (this_ptr->IsEnable() && !this_ptr->IsRegister()){
        /*使能但是未注册，定时注册*/
        if (this_ptr->register_timer_.IsTimerExpired()) {
            this_ptr->StartSipServer();
            this_ptr->register_timer_.Start(this_ptr->profile_.get_register_interval());
        }

    } else if (!this_ptr->IsEnable() && this_ptr->IsRegister()) {
        /*未使能但是已经注册, 停止服务*/
        this_ptr->StopSipServer();

    } else if (this_ptr->IsEnable() && this_ptr->IsRegister()) {
        /*使能并且已经注册*/
        if (this_ptr->heart_timer_.IsTimerExpired()) { // 心跳处理
            this_ptr->DoHeartbeat();
            this_ptr->heart_timer_.Start(this_ptr->profile_.get_heartbeat_interval());
        }

        if (this_ptr->refresh_register_timer_.IsTimerExpired()) { // 刷新注册
            this_ptr->RefreshRegister();
            this_ptr->refresh_register_timer_.Start(this_ptr->profile_.get_term_of_register());
        }
    }

    if (this_ptr->is_upgrading()) {
        int progressbar = conf_get_update_progressbar();
        if (UPDATE_SUCCESS == progressbar) {
            /*升级成功*/
            this_ptr->SendUpgradeResult(GbClientManager::UpgradeStatus::UPGRADE_SUCCESS);
        } else if (UPDATE_ERR_MD5SUM == progressbar || UPDATE_ERR_ENTRY == progressbar) {
            /*升级包损坏*/
            this_ptr->SendUpgradeResult(GbClientManager::UpgradeStatus::PACKAGE_DAMAGE);
        } else if (UPDATE_ERR_NON_EXIST == progressbar) {
            /*升级包未成功下载，其它错误*/
            this_ptr->SendUpgradeResult(GbClientManager::UpgradeStatus::OTHER_ERR);
        } else if (this_ptr->upgrade_dialog_->IsTimeout()) {
            if (progressbar == 0) {
                /*超时且 progressbar 为 0 代表包没下完，软件下载超时错误*/
                this_ptr->SendUpgradeResult(GbClientManager::UpgradeStatus::TIMEOUT);
            } else {
                /*升级包下载完成，超时未升级完成，其它错误*/
                this_ptr->SendUpgradeResult(GbClientManager::UpgradeStatus::OTHER_ERR);
            }
        }
    }
}

// ip 和 mac 数组最大长度限制 64
int GbClientManager::GetNetInfo(char *ip, char *mac, int *mtu)
{
    if (ip == nullptr || mac == nullptr || mtu == nullptr) {
        GB_ERR("param error\n");
        return FAILURE;
    }

    const char *nic = "eth0";

    // 网络信息获取 eth0 优先，eth0 未连接，4g 就查 usb0，wifi 查 wlan0
    if (net_link_status("eth0") == 1) {
        nic = "eth0";
    } else if (get_g_sys(usb_4g)) {
        nic = "usb0";
    } else {
        nic = "wlan0";
    }

    GB_DBG("nic:%s\n", nic);

    int ret = net_get_ipaddr((char *)nic, ip, 64);
    if (ret < 0) {
        GB_ERR("get ip error\n");
        return FAILURE;
    }

    ret = net_get_macaddr(nic, mac);
    if (ret < 0) {
        GB_ERR("get mac error\n");
        return FAILURE;
    }

    ret = net_get_mtu(nic, mtu);
    if (ret < 0) {
        GB_ERR("get mtu error\n");
        return FAILURE;
    }

    GB_DBG("nic:%s get ip:%s mac:%s mtu:%d\n", nic, ip, mac, *mtu);

    return SUCCESS;
}

/*发送心跳报文*/
void GbClientManager::DoHeartbeat()
{
    if (heartbeat_timeout_count_ >= profile_.get_heartbeat_timeout_count()) {
        heartbeat_timeout_count_ = 0;
        GB_DBG("heartbeat timeout, reconnect\n");
        SetStopOnce(); // 达到心跳超时次数，重新建立连接
        return ;
    }

    mxml_node_t *reply_xml = GenerateXml("Notify", osip_build_random_number(), "Keepalive", NULL, profile_.get_sip_user_name());
    if (reply_xml == NULL) {
        return ;
    }
    mxml_node_t *notify_xml = mxmlFindElement(reply_xml, reply_xml, "Notify", NULL, NULL, MXML_DESCEND);
    mxml_node_t *status_xml = mxmlNewElement(notify_xml, "Status");
    mxmlNewTextf(status_xml, 0, "%s", "OK");

    // 生成心跳并且发送
    std::shared_ptr<MessageDialog> ptr = std::make_shared<MessageDialog>(MessageDialog::Type::NOTIFY_HEARTBEAT);
    MessageRequestAddXmlAndSend(reply_xml, ptr);
    heartbeat_timeout_count_++; // 发送成功，没收到回复之前，标记为超时

    mxmlDelete(reply_xml);
}

/*刷新注册*/
int GbClientManager::RefreshRegister()
{
    int ret = 0;
    osip_message_t *register_message = NULL;

    GB_DBG("refresh register\n");

    register_message = GenerateRegesterSip(register_cseq_++, profile_.get_term_of_register());
    if (register_message == NULL) {
        return FAILURE;
    }

    ret = SipToStringAndSend(register_message);
    if (ret == SUCCESS) {
        std::shared_ptr<RegisterDialog> register_ptr =
                std::make_shared<RegisterDialog>(register_message, RegisterDialog::Type::REGISTER, 10);

        ret = dialog_mng_.AddDialog(register_ptr);
        if (ret != SUCCESS) {
            GB_ERR("add refresh register dialog fail\n");
        }
    }

    osip_message_free(register_message);

    return SUCCESS;
}

void GbClientManager::HandleAlarmMessages(int id, void *p_src, int size, void *instance)
{
    if (instance == NULL) {
        GB_ERR("error, instance is NULL\n");
        return;
    }

    GbClientManager *this_ptr = (GbClientManager *)instance;
    if (this_ptr->sch_message_ == NULL || this_ptr->GetSharedPtr() == nullptr) {
        GB_ERR("sch_message or shared_ptr is NULL\n");
        return;
    }

    ClientEvent alarm_type = EVENT_UNKNOWN;

    /*报警消息判断*/
    if(id == JEvent_AlarmMD) {
        alarm_type = ALARM_MOVE;
        GB_INFO("handle messages JEvent_AlarmMD\n");

    } else if (id == JEvent_Alarmhumadetect) {
        alarm_type = ALARM_HUMAN;
        GB_INFO("handle messages JEvent_Alarmhumadetect\n");

    } else if (id == JEvent_AlarmVgline) {
        alarm_type = ALARM_LINE;
        GB_INFO("handle messages JEvent_AlarmVgline\n");

    } else if (id == JEvent_AlarmVgrect) {
        alarm_type = ALARM_RECT;
        GB_INFO("handle messages JEvent_AlarmVgrect\n");
    } else if (id == JEvent_AlarmVL) {
        // 暂不处理
        GB_INFO("handle messages JEvent_AlarmVL\n");
        return;
    } else {
        GB_INFO("Invalid message:%d\n", id);
        return;
    }

    Enqueue(alarm_type);

    return ;
}

void GbClientManager::HandleConfigMessages(int id, void *p_src, int size, void *instance)
{
    if (instance == NULL) {
        GB_ERR("error, instance is NULL\n");
        return;
    }

    GbClientManager *this_ptr = (GbClientManager *)instance;
    if (this_ptr->sch_message_ == NULL || this_ptr->GetSharedPtr() == nullptr) {
        GB_ERR("sch_message or shared_ptr is NULL\n");
        return;
    }

    // 因为国标会发送 JCP 命令配置国标参数，所以配置同步只能异步执行
    if (id == JEvent_GuoBiaoCfg) {
        SchedulerParam<GbClientManager, GuoBiaoS> *param = new SchedulerParam<GbClientManager, GuoBiaoS>(this_ptr->GetSharedPtr(), false, id);

        conf_get_guobiaocfg(param->get_data1_ptr());
        js_run_function(this_ptr->sch_message_, HandleGuoBiaoCfgChg, param, false);
    } else if (id == Jevent_GuoBiaoAddrCfg) {
        SchedulerParam<GbClientManager, GBAddrS> *param = new SchedulerParam<GbClientManager, GBAddrS>(this_ptr->GetSharedPtr(), false, id);

        conf_get_guobiaoaddr(param->get_data1_ptr());
        js_run_function(this_ptr->sch_message_, HandleGuoBiaoAddrCfgChg, param, false);
    } else if (id == JEvent_EthcfgChg || id == JEvent_DhcpNotify || id == JEvent_WifiCfgChg || id == JEvent_AlarmCabDis || id == JEvent_AlarmCableNormal) {
        SchedulerParam<GbClientManager, NetEthS> *param = new SchedulerParam<GbClientManager, NetEthS>(this_ptr->GetSharedPtr(), false, id);

        js_run_function(this_ptr->sch_message_, HandleNetCfgChg, param, false);
    }

    return ;
}

void GbClientManager::HandleGuoBiaoCfgChg(void *data)
{
    SchedulerParam<GbClientManager, GuoBiaoS> *param = (SchedulerParam<GbClientManager, GuoBiaoS> *)data;
    std::shared_ptr<GbClientManager> &this_ptr = param->get_this_ptr();

    this_ptr->profile_.SetGbParam(param->get_data1()); // 更新参数
    this_ptr->profile_.PrintfGbParam(); // 打印新参数

    GB_DBG("gb config change, reconnect\n");
    this_ptr->SetStopOnce(); // 停止一次，以进行重连

    // 异步调用，需要删除参数
    if (!param->IsSync()) {
        delete param;
    }

    return;
}

void GbClientManager::HandleGuoBiaoAddrCfgChg(void *data)
{
    SchedulerParam<GbClientManager, GBAddrS> *param = (SchedulerParam<GbClientManager, GBAddrS> *)data;
    std::shared_ptr<GbClientManager> &this_ptr = param->get_this_ptr();

    /*地址信息更新不需要重连*/
    this_ptr->profile_.SetGbAddress(param->get_data1());

    // 异步调用，需要删除参数
    if (!param->IsSync()) {
        delete param;
    }

    return;
}

void GbClientManager::HandleNetCfgChg(void *data)
{
    SchedulerParam<GbClientManager, NetEthS> *param = (SchedulerParam<GbClientManager, NetEthS> *)data;
    std::shared_ptr<GbClientManager> &this_ptr = param->get_this_ptr();

    const char *ip = this_ptr->profile_.get_local_ip();
    const char *mac = this_ptr->profile_.get_local_mac();

    char nic_ip[64] = {0};
    char nic_mac[64] = {0};
    int mtu = 1500;
    this_ptr->GetNetInfo(nic_ip, nic_mac, &mtu);

    /*ip 或者 mac 变更，重新连接服务器*/
    if ( strcmp(ip, nic_ip) != 0 || strcmp(mac, nic_mac) != 0) {
        GB_DBG("ip or mac change, reconnect\n");
        this_ptr->SetStopOnce(); // 停止一次，以进行重连
    }
    this_ptr->profile_.SetNetInfo(nic_ip, nic_mac, mtu);

    // 异步调用，需要删除参数
    if (!param->IsSync()) {
        delete param;
    }

    return;
}

/*开始 sip 服务，成功返回 SUCCESS 失败返回 FAILURE*/
int GbClientManager::StartSipServer()
{
    int ret = 0;

    if (socket_ctrl_ == NULL) {
        GB_DBG("start connect sip server\n");

        socket_ctrl_ = ConnectSipServer();
        if (socket_ctrl_ == NULL) {
            GB_DBG("Connect sip server fail\n");
            return FAILURE;
        }

        // 设置侦听
        js_create_reader_r(sch_message_, socket_ctrl_->get_socket(),
                                    JS_READABLE, HandleSocketRead, this, &hdl_socker_read_);
    }

    GB_DBG("start register\n");
    osip_message_t *register_message = NULL;
    // 发送注册消息
    register_cseq_ = 1;  // 注册序号都是从 1 开始
    register_nc_ = 1;
    register_message = GenerateRegesterSip(register_cseq_++, profile_.get_term_of_register());
    if (register_message == NULL) {
        GB_ERR("generate regester sip fail\n");
        return FAILURE;
    }

    ret = SipToStringAndSend(register_message);
    if (ret == SUCCESS) {
        /*添加会话，注册还需要后续处理*/
        std::shared_ptr<RegisterDialog> register_ptr =
                std::make_shared<RegisterDialog>(register_message, RegisterDialog::Type::REGISTER, 10);

        ret = dialog_mng_.AddDialog(register_ptr);
        if (ret != SUCCESS) {
            GB_ERR("add register dialog fail\n");
        }
    }

    osip_message_free(register_message);

    return ret;
}

/*发送注销报文*/
int GbClientManager::SipLogOut()
{
    int ret = 0;
    osip_message_t *log_out_message = NULL;

    log_out_message = GenerateRegesterSip(register_cseq_++, 0);
    if (log_out_message == NULL) {
        return FAILURE;
    }

    ret = SipToStringAndSend(log_out_message);
    if (ret == SUCCESS) {
        std::shared_ptr<RegisterDialog> logout_ptr =
                std::make_shared<RegisterDialog>(log_out_message, RegisterDialog::Type::LOG_OUT, 5);

        ret = dialog_mng_.AddDialog(logout_ptr);
        if (ret != SUCCESS) {
            GB_ERR("add logout dialog fail\n");
        }
    }

    osip_message_free(log_out_message);

    return SUCCESS;
}

/*停止 sip 服务，成功返回 SUCCESS, 失败返回 FAILURE*/
int GbClientManager::StopSipServer()
{
    /* 停止服务需要先向服务器发送 BYE，再关闭连接，可能存在服务器不响应的情况，
    *  需要定时判断，时间到了如果还不能注销，就强制关闭服务
    */
    if (IsRegister()) {
        if (!log_out_timer_.IsEnable()) {
            /*定时器未使能，发送一次注销消息，设置定时器等待 5S*/
            SipLogOut();
            log_out_timer_.Start(5);

            return FAILURE;
        } else if (!log_out_timer_.IsTimerExpired()) {
            /*定时器使能，时间未到，继续等待*/
            return FAILURE;
        }
    } else {
        if (socket_ctrl_ == NULL) {
            /*未注册，连接也未建立，返回成功*/
            return SUCCESS;
        }
    }

    /*sip 服务关闭时需要把推流服务全部关闭*/
    live_streaming_mng_->StopAllPushStreaming();
    record_mng_->ClearPlayList();

    /*关闭连接*/
    js_delete_reader_r(&hdl_socker_read_);

    delete socket_ctrl_;
    socket_ctrl_ = NULL;

    register_ = false;

    log_out_timer_.Stop();

    return SUCCESS;
}

/*创建套接字，连接服务器，成功返回套接字指针，失败返回 NULL*/
GbSocket *GbClientManager::ConnectSipServer()
{
    int ret = 0;
    GbSocket *socket = new GbSocket();
    if (socket == NULL) {
        return NULL;
    }

    ret = socket->ServerConnect(profile_.get_transport_protocols(), profile_.get_sip_local_port(),
                                        profile_.get_sip_server_address(), profile_.get_sip_server_port(), 10);
    if (ret == SUCCESS) {
        GB_DBG("connect sip server success\n");
        return socket;
    }

    delete socket;
    return NULL;
}

void GbClientManager::HandleSocketRead(int fd, int events, void *userdata)
{
    if (userdata == NULL) {
        return ;
    }

    GbClientManager *this_ptr = (GbClientManager *)userdata;
    int ret =0;

    if (this_ptr->socket_ctrl_->get_network_protocols() == GB_TCP) {
        // tcp 要考虑粘包
        ret = this_ptr->socket_ctrl_->Recv(this_ptr->recv_buf_ + this_ptr->recv_byte_, SIP_MESSAGE_MAX_LENGTH - this_ptr->recv_byte_);
        if (ret <= 0) { // = 0 对端关闭连接; < 0 接收错误
            GB_DBG("connection has been closed, reconnect\n");
            this_ptr->SetStopOnce();
            return;
        }
        this_ptr->recv_byte_ += ret;

        uint32_t consumed = 0;
        this_ptr->recv_buf_[this_ptr->recv_byte_] = 0;
        consumed = this_ptr->HandleTcpMessage(this_ptr->recv_buf_, this_ptr->recv_byte_);
        if (consumed == 0) { // 未消耗
            return ;
        } else {
            if (this_ptr->recv_byte_ > consumed) {
                memmove(this_ptr->recv_buf_, this_ptr->recv_buf_ + consumed, this_ptr->recv_byte_ - consumed);
                this_ptr->recv_byte_ -= consumed;
            } else {
                this_ptr->recv_byte_ = 0;
            }
        }
    } else {
        // udp 一次接收是一个完整的数据包
        ret = this_ptr->socket_ctrl_->Recv(this_ptr->recv_buf_, SIP_MESSAGE_MAX_LENGTH);
        if (ret > 32) { // exosip2 库的做法，大于 32 字节就处理包数据
            this_ptr->recv_buf_[ret] = 0;
            this_ptr->HandleSipMessage(this_ptr->recv_buf_, ret);
        } else if (ret < 0) {
            if (errno == 0 || errno == ERANGE) {  // exosip2 库处理，将 buff 扩大两倍，但是 tcp 那边又没处理，这里先不管
                ;
                //udp_message_max_length = udp_message_max_length*2;
                //osip_free(reserved->buf);
                //reserved->buf = (char *) osip_malloc (udp_message_max_length * sizeof (char) + 1);
            }
            if (errno == ENOTCONN) {
                GB_DBG("connection has been closed, reconnect\n");
                this_ptr->SetStopOnce();
            }
        } else {
            GB_ERR("Dummy SIP message received\n");
        }
    }

    return ;
}

/*
*  osip_message_t* 结构体转成字节流并且发送，支持添加消息体
*
* @param[message] sip 消息结构体
* @param[content_type] 默认值 NULL, 消息体的格式 例如: application/MANSCDP+xml
* @param[body] 默认值 NULL, 消息体指针
* @param[body_len] 默认值 0, 消息体长度
*
* @return 成功返回 SUCCESS 失败返回 FAILURE
*/
int GbClientManager::SipToStringAndSend(osip_message_t* message, const char *content_type, const char *body, uint32_t body_len)
{
    int ret = 0;
    char *data = NULL;
    size_t data_len = 0;

    if (content_type != NULL && body != NULL && body_len != 0) {
        if (osip_message_set_content_type(message, content_type) != OSIP_SUCCESS) {
            GB_ERR("osip set content fail\n");
            return FAILURE;
        }

        if (osip_message_set_body(message, body, (size_t)body_len)) {
            GB_ERR("osip set body fail\n");
            return FAILURE;
        }
    }

    if (osip_message_to_str(message, &data, (size_t *)&data_len) != 0) {
        GB_ERR("osip message to str fail\n");
        return FAILURE;
    }

    ret = socket_ctrl_->Send(data, data_len);
    if (ret < 0) {
        GB_ERR("send fail\n");
        ret = FAILURE;
    } else {
        GB_INFO("send bytes:%d ret:%d \n%s\n", data_len, ret, data);
        ret = SUCCESS;
    }

    osip_free(data);

    return ret;
}

/* Like strstr, but works for haystack that may contain binary data and is
 not NUL-terminated. */
static char *buffer_find(const char *haystack, size_t haystack_len, const char *needle)
{
    const char *search = haystack, *end = haystack + haystack_len;
    char *p;
    size_t len = strlen (needle);

    while (search < end && (p = (char *)memchr(search, *needle, end - search)) != NULL) {
        if (p + len > end)
            break;
        if (memcmp(p, needle, len) == 0)
            return (p);
        search = p + 1;
    }

    return (NULL);
}

#define END_HEADERS_STR "\r\n\r\n"
#define CLEN_HEADER_STR "\r\ncontent-length:"
#define CLEN_HEADER_COMPACT_STR "\r\nl:"
#define CLEN_HEADER_STR2 "\r\ncontent-length "
#define CLEN_HEADER_COMPACT_STR2 "\r\nl "
#define const_strlen(x) (sizeof((x)) - 1)

uint32_t GbClientManager::HandleTcpMessage(const char *buff, uint32_t data_size)
{
    /*  流定界方法:
    *       1)通过/r/n/r/n找到sip header，从header中找到content length
    *       2)通过content length找到sip body。sip msg len = str(sip header) + content length
    */
    int consumed = 0;
    char *buf = (char *)buff;
    size_t buflen = data_size;
    char *end_headers;

    while (buflen > 0 && (end_headers = buffer_find(buf, buflen, END_HEADERS_STR)) != NULL) {
        int clen, msglen;
        char *clen_header;

        if (buf == end_headers) {
            /* skip tcp standard keep-alive */
            GB_DBG("standard keep alive received (CRLFCRLF)\n");
            consumed += 4;
            buflen -= 4;
            buf += 4;
            continue;
        }

        /* stuff a nul in so we can use osip_strcasestr */
        *end_headers = '\0';

        /* ok we have complete headers, find content-length: or l: */
        clen_header = osip_strcasestr(buf, CLEN_HEADER_STR);
        if (!clen_header)
            clen_header = osip_strcasestr(buf, CLEN_HEADER_STR2);
        if (!clen_header)
            clen_header = osip_strcasestr(buf, CLEN_HEADER_COMPACT_STR);
        if (!clen_header)
            clen_header = osip_strcasestr(buf, CLEN_HEADER_COMPACT_STR2);
        if (clen_header != NULL) {
            clen_header = strchr(clen_header, ':');
            clen_header++;
        }
        if (!clen_header) {
            /* Oops, no content-length header.      Presume 0 (below) so we
            consume the headers and make forward progress.  This permits
            server-side keepalive of "\r\n\r\n". */
            GB_DBG("message has no content-length: <%s>\n", buf);
        }
        clen = clen_header ? atoi(clen_header) : 0;
        if (clen<0)
            return (int)data_size; /* discard data */
        /* undo our overwrite and advance end_headers */
        *end_headers = END_HEADERS_STR[0];
        end_headers += const_strlen(END_HEADERS_STR);

        /* do we have the whole message? */
        msglen = (end_headers - buf + clen);
        if ((uint32_t)msglen > buflen) {
            /* nope */
            return consumed;
        }
        /* yep; handle the message */
        HandleSipMessage(buf, msglen);
        consumed += msglen;
        buflen -= msglen;
        buf += msglen;
    }

    return consumed;
}

int GbClientManager::HandleSipMessage(const char *buff, uint32_t data_size)
{
    int ret = 0;
    osip_message_t *message;

    osip_message_init(&message);
    if(message == NULL) {
        GB_ERR("osip_message_init error\n");
        return FAILURE;
    }

    // %.*s 用来控制 %s 输出长度，解决 tcp 粘包时，会把连续好几个包的内容都打印出来的情况
    GB_INFO("handle message:\n%.*s\n", data_size, buff);

    ret = osip_message_parse(message, buff, (size_t)data_size);
    if (ret != OSIP_SUCCESS) {
        GB_ERR("parse sip message fail\n");
        osip_message_free(message);
        return FAILURE;
    }

    if (MSG_IS_RESPONSE_FOR(message, "REGISTER")) {
        HandleRegister(message);

    } else if(MSG_IS_ACK(message)) {
        HandleAck(message);

    } else if(MSG_IS_INVITE(message) || MSG_IS_RESPONSE_FOR(message, "INVITE")) {
        HandleInvite(message);

    } else if(MSG_IS_BYE(message)) {
        HandleBye(message);

    } else if(MSG_IS_SUBSCRIBE(message)) {
        HandleSubscribe(message);

    } else if(MSG_IS_MESSAGE(message) || MSG_IS_RESPONSE_FOR(message, "MESSAGE")) {
        HandleMessage(message);

    } else if(MSG_IS_NOTIFY(message)) {
        HandleNotify(message);

    } else if(MSG_IS_INFO(message)) {
        HandleInfo(message);

    } else {
        GB_DBG("*****no define MSG******\n");
    }

    if (message != NULL) {
        osip_message_free(message);
    }
    return SUCCESS;
}

void GbClientManager::CvtoHex(char *Bin, char *Hex)
{
    /*exosip 里面的函数，将二进制转换成 16 进制字符串*/
    unsigned short i;
    unsigned char j;

    for (i = 0; i < 16; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i * 2] = (j + '0');
        else
            Hex[i * 2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i * 2 + 1] = (j + '0');
        else
            Hex[i * 2 + 1] = (j + 'a' - 10);
    };

    Hex[32] = '\0';
}

/*request 是要发送出去认证的请求，需要用到里面的 methond、request-uri 以及可能用到消息体*/
/*out 大小不会超过 500, 推荐给 1024 保证安全*/
int GbClientManager::DigestCalcAuth(osip_www_authenticate_t *www_auth, osip_message_t *request, char *out)
{
    /*
    * 认证哈希值计算分为三种情况(具体计算公式参考笔记)：无 qop，qop="auth"，qop="auth-int"
    */

    if (request->sip_method == NULL || request->req_uri == NULL || www_auth->realm == NULL || www_auth->nonce == NULL) {
        GB_ERR("incomplete certification information\n");
        return FAILURE;
    }

    char ha_bin[17] = {0};
    char ha1_hex[33] = {0};
    char ha2_hex[33] = {0};
    char entity_hex[33] = {0};
    char result_hex[33] = {0};
    osip_MD5_CTX Md5Ctx;
    osip_MD5_CTX Md5Ctx_entity;

    char pszNonceCount[9] = {0};
    const char *pszCNonce = "0a4f113b";

    char *qop_auth = NULL;
    char *realm = NULL;
    char *nonce = NULL;
    osip_body_t *body = NULL;
    char *entity_str = NULL;
    size_t entity_str_len = 0;
    char *url_str = NULL;
    char *tmp_out = out;
    if (osip_uri_to_str(request->req_uri, &url_str) != OSIP_SUCCESS) {
        return FAILURE;
    }

    realm = osip_strdup_without_quote(www_auth->realm); // 解析出来 realm 是带 " 号的
    nonce = osip_strdup_without_quote(www_auth->nonce);
    /* calculate H(A1) 无论那种认证方式，ha1 都是一样的*/
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx,(unsigned char *)profile_.get_sip_user_auth_id(), strlen(profile_.get_sip_user_auth_id()));
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)realm, strlen(realm));
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)profile_.get_sip_user_password(), strlen(profile_.get_sip_user_password()));
    osip_MD5Final((unsigned char *)ha_bin, &Md5Ctx);
    CvtoHex(ha_bin, ha1_hex);

    /* calculate H(A2) */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char *)request->sip_method, (unsigned int)strlen(request->sip_method));
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)url_str, (unsigned int)strlen(url_str));

    if (www_auth->qop_options != NULL) {
        GB_DBG("qop = %s\n", www_auth->qop_options);
        qop_auth = osip_strdup_without_quote(www_auth->qop_options); // qop=auth 要去掉引号
    }


    if (qop_auth == NULL) {
        goto auth_withoutqop;
    } else if (0 == osip_strcasecmp(qop_auth, "auth-int")) {
        goto auth_withauth_int;
    } else if (0 == osip_strcasecmp(qop_auth, "auth")) {
        goto auth_withauth;
    }

auth_withoutqop:
    osip_MD5Final((unsigned char *)ha_bin, &Md5Ctx);
    CvtoHex(ha_bin, ha2_hex);

    /* calculate response */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char *) ha1_hex, 32);
    osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *) nonce, (unsigned int) strlen(nonce));
    osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);

    goto end;

auth_withauth_int:

    /*calculate H(entity)*/
    // 这里简单处理，一般我们的 request 只会有一次 body 添加
    if (osip_message_get_body(request , 1, &body) == 0) {
        if (osip_body_to_str(body, &entity_str, &entity_str_len) == 0) {
            osip_MD5Init(&Md5Ctx_entity);
            osip_MD5Update(&Md5Ctx_entity, (unsigned char*)entity_str, entity_str_len);
            osip_MD5Final((unsigned char *)ha_bin, &Md5Ctx_entity);
            CvtoHex(ha_bin, entity_hex);

            osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
            osip_MD5Update(&Md5Ctx, (unsigned char *)entity_hex, 32);
        }
    }

auth_withauth:
    snprintf(pszNonceCount, sizeof(pszNonceCount), "%08u", register_nc_++);

    osip_MD5Final((unsigned char *)ha_bin, &Md5Ctx);
    CvtoHex(ha_bin, ha2_hex);

    /* calculate response */
    osip_MD5Init(&Md5Ctx);
    osip_MD5Update(&Md5Ctx, (unsigned char *)ha1_hex, 32);
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)nonce, (unsigned int) strlen (nonce));
    osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)pszNonceCount, (unsigned int)strlen(pszNonceCount));
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)pszCNonce, (unsigned int)strlen(pszCNonce));
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
    osip_MD5Update(&Md5Ctx, (unsigned char *)qop_auth, (unsigned int)strlen(qop_auth));
    osip_MD5Update(&Md5Ctx, (unsigned char *)":", 1);
end:
    osip_MD5Update(&Md5Ctx, (unsigned char *)ha2_hex, 32);
    osip_MD5Final((unsigned char *)ha_bin, &Md5Ctx);
    CvtoHex(ha_bin, result_hex);

    /*生成认证字串*/
    if (qop_auth != NULL) {
        sprintf(out, "Digest username=\"%s\", realm=%s, nonce=%s, uri=\"%s\", response=\"%s\", algorithm=MD5, cnonce=\"%s\", qop=%s, nc=%s",
        profile_.get_sip_user_auth_id(), www_auth->realm, www_auth->nonce, url_str, result_hex, pszCNonce, qop_auth, pszNonceCount);

        osip_free(qop_auth);
        qop_auth = NULL;
    } else {
        sprintf(out, "Digest username=\"%s\", realm=%s, nonce=%s, uri=\"%s\", response=\"%s\", algorithm=MD5",
                    profile_.get_sip_user_auth_id(), www_auth->realm, www_auth->nonce, url_str, result_hex);
    }

    /*opaque 字段，如果服务器有发送，需要拷贝一份*/
    if (www_auth->opaque != NULL) {
        sprintf(out, "%s, opaque=%s", tmp_out, www_auth->opaque);
    }

    if (realm) {
        osip_free(realm);
    }

    if (nonce) {
        osip_free(nonce);
    }

    if (url_str != NULL) {
        osip_free(url_str);
    }

    if (entity_str != NULL) {
        osip_free(entity_str);
    }

    return 0;
}

int GbClientManager::HandleRegisterFor401(osip_message_t *message, RegisterDialog::Type type)
{
    osip_message_t *request_401 = GenerateRequestForResponse(message, "REGISTER", profile_.get_sip_user_name(), true);
    if (request_401 == NULL) {
        return FAILURE;
    }

    do {
        // 添加 expires
        char expires_str[32] = {0};
        if (type == RegisterDialog::Type::LOG_OUT) {
            snprintf(expires_str, sizeof(expires_str), "%u", 0); // 注销 expires 填 0，其余和注册一样
        } else {
            snprintf(expires_str, sizeof(expires_str), "%u", profile_.get_term_of_register()); // 注册 expires 非 0
        }

        if (osip_message_set_expires(request_401, expires_str) != OSIP_SUCCESS) {
            GB_ERR("register set expires\n");
            break;
        }

        osip_www_authenticate_t *www_auth = NULL;
        int ret = osip_message_get_www_authenticate(message, 0, &www_auth);
        if (ret != OSIP_SUCCESS) {
            GB_ERR("get www auth error\n");
            break;
        }

        char authorization_buf[1024] = {0};

        DigestCalcAuth(www_auth, request_401, authorization_buf);

        DBG("Authorization : %s\n", authorization_buf);
        osip_message_set_authorization(request_401, authorization_buf);

        SipToStringAndSend(request_401);
    } while(0);

    if (request_401 != NULL)
        osip_message_free(request_401);

    return SUCCESS;
}

void GbClientManager::SyncTimeWithServer(const char* time_str)
{
    if (time_str == NULL) {
        GB_ERR("time_str error\n");
        return ;
    }

    struct tm tm_local = {0};

    // 解析输入时间字符串
    if (strptime(time_str, "%Y-%m-%dT%H:%M:%S", &tm_local) == NULL) {
        GB_ERR("Failed to parse date/time string:%s\n", time_str);
        return ;
    }

    // 将 struct tm 结构转换为 time_t 类型
    // 注意：mktime 函数假设输入时间是本地时间
    time_t local_time = mktime(&tm_local);
    if (local_time == -1) {
        GB_ERR("Failed to convert to time_t\n");
        return ;
    }

    // 国标下发是中国时区的时间，需要减掉时区
    local_time -= 28800;

    GbJcpSend("timecfg -act set -time %d -timezone 29", local_time);

    return;
}

void GbClientManager::HandleRegister(osip_message_t *message)
{
    /*  客户端注册消息只用考虑回复：
    *       200 OK: 表示注册成功
    *       301 Moved Permanently: 请求的资源已永久移动到新的URI。
    *       302 Moved Temporarily: 请求的资源临时移动到新的URI。
    *       401 Unauthorized: 需要提供身份验证信息
    */
    if (!MSG_IS_RESPONSE(message)) {
        // 国标客户端只接收注册回复
        GB_ERR("This message is not a reply message\n");
        return ;
    }

    /*验证会话是否存在*/
    std::shared_ptr<RegisterDialog> register_ptr;
    int ret = dialog_mng_.PopDialog(message, register_ptr);
    if (ret != SUCCESS) {
        GB_ERR("not find Dialog for register\n");
        return ;
    }

    GB_DBG("register hanldle status code:%d\n", message->status_code);

    if (message->status_code == 200) {
        if (register_ptr->get_type() == RegisterDialog::Type::LOG_OUT)  { // 注销成功
            register_ = false;
        } else {  // 注册成功
            register_ = true;
            osip_header_t *date;
            osip_message_get_date(message, 0, &date);
            if (date != NULL)
                SyncTimeWithServer(date->hvalue); // 200 OK 会下发时间，进行对时
        }
    } else if (message->status_code == 301 || message->status_code == 302) {
        // 重定向的目标地址通常包含在 contact 字段中
        // 301 和 302 一个是临时重定向，一个是永久重定向，目前都是当临时重定向处理
        do {
            osip_contact_t* contact = NULL;

            if (osip_message_get_contact(message, 0, &contact) != OSIP_SUCCESS) {
                GB_ERR("get contact fail\n");
                break;
            }

            if (contact->url->host != NULL) {
                GB_DBG("contact->url->host:%s\n", contact->url->host);
                profile_.set_sip_server_address(contact->url->host);
            }
            if (contact->url->port != NULL) {
                GB_DBG("contact->url->port:%s\n", contact->url->port);
                profile_.set_sip_server_port(atoi(contact->url->port));
            }
            if (contact->url->username != NULL) {
                GB_DBG("contact->url->username:%s\n", contact->url->username);
                profile_.set_sip_server_id(contact->url->username);
                /*sip 域一般为服务器 id 的前十位，这里和服务器 ip 一起更新*/
                char civil_code[11] = {0};
                snprintf(civil_code, 11, "%s", contact->url->username); // snprintf 会在最后一位补 0
                profile_.set_civil_code(civil_code);
            }
            GB_DBG("redirect to reconnect\n");
            // 重新开始连接
            SetStopOnce();
        } while(0);

    } else if (message->status_code == 403 && message->reason_phrase != NULL) { // 403 密码错误或者认证失败
        GB_DBG("register fail :%s\n", message->reason_phrase);

    } else if (message->status_code == 401 || message->status_code == 407) {
        // 需要身份验证
        HandleRegisterFor401(message, register_ptr->get_type());
        dialog_mng_.AddDialog(register_ptr); // 保留对话，后续还需要处理
    }

    return ;
}

int GbClientManager::ParsePtzCmdText(const char *text, uint8_t &cmd, uint8_t &data1, uint8_t &data2, uint8_t &data3)
{
    /*ptz 指令总共 8 字节*/
    // 字节    |  字节 1  | 字节 2  | 字节 3 | 字节 4 | 字节 5 | 字节 6 | 字节 7  | 字节 8
    // -----   |-------   | ------- | ------ | ------ | ------ | ------ | ------- | -------
    // 含义    | A5H      | 组合码1 | 地址   | 指令   | 数据1  | 数据2  | 组合码2 | 校验码

    uint8_t ptz_cmd[8] = {0};
    uint64_t unmber = strtoull(text, NULL, 16); // 8 字节，对于 32 位系统来说正好是一个 ull 类型

    GB_DBG("ptzcmd:%llx\n", unmber);

    memcpy(ptz_cmd, &unmber, sizeof(ptz_cmd));
    // 字符串转整形再拷贝到数组会变成倒序，需要翻转回来
    for (int i = 0; i < 4; i++) {
        int tmp = ptz_cmd[i];
        ptz_cmd[i] = ptz_cmd[7 - i];
        ptz_cmd[7 - i] = tmp;
    }

    // 当前版本前两个字节固定为 A5 0F
    if (ptz_cmd[0] != 0xa5 || ptz_cmd[1] != 0x0f) {
        GB_ERR("byte0[%02x] || byte1[%02x] error\n", ptz_cmd[0], ptz_cmd[1]);
        return FAILURE;
    }

    // 最后一个字节为校验码,为前面的第1~7字节的算术和的低8位,即算术和对256取模后的结果
    uint8_t checksum = (ptz_cmd[0] + ptz_cmd[1] + ptz_cmd[2] + ptz_cmd[3] + ptz_cmd[4] + ptz_cmd[5] + ptz_cmd[6]) % 0x100;
    if (ptz_cmd[7] != checksum) {
        GB_ERR("checksum[%02x] error, byte7[%02x]\n", checksum, ptz_cmd[7]);
        return FAILURE;
    }

    cmd = ptz_cmd[3]; // 第四字节为指令码
    data1 = ptz_cmd[4];
    data2 = ptz_cmd[5];
    data3 = ptz_cmd[6] & 0xf0 >> 4; // 第七字节是组合码，高四位为数据3

    GB_DBG("cmd[%02x] data1[%02x] data2[%02x] data3[%02x]\n", cmd, data1, data2, data3);

    return SUCCESS;
}

// 发送 jcp 命令, 成功返回 SUCCESS 失败返回 FAILURE
int GbClientManager::GbJcpSend(const char *format, ...)
{
    char respbuf[1024] = {0};
    char cmdline[1024] = {0};
    int ret = 0;

    va_list arg_list;

    va_start(arg_list, format);
    ret = vsnprintf(cmdline, sizeof(cmdline)-1, format, arg_list);
    va_end(arg_list);

    if (ret >= (int)sizeof(cmdline)) {
        // 字符串超过了 buff 限制，命令可能不完整
        return FAILURE;
    }

    jcpcmd_sendrecv(cmdline, respbuf, sizeof(respbuf));
    if (strstr(respbuf, "Success")) {
        return SUCCESS;
    }

    return FAILURE;
}

// 国标规定的速度值是 0x00~0xff,我们的速度范围是 0~63,需要进行映射
#define DATA_TO_SPEED(a) (a * 63 / 0xff)

void GbClientManager::HandPtzCmd(mxml_node_t* ptz_cmd)
{
    uint8_t cmd = 0, data1 = 0, data2 = 0, data3 = 0;
    int ret = 0;
    int whitespace = 0;
    int set_time = 0;

    const char *ptz_cmd_text = mxmlGetText(ptz_cmd, &whitespace);
    if (ptz_cmd_text == NULL)
        return ;

    ret = ParsePtzCmdText(ptz_cmd_text + whitespace, cmd, data1, data2, data3);
    if (ret != SUCCESS)
        return ;

    switch (cmd) {
    // 国标协议上规定云台上下左右和变倍要可以组合，我们目前的指令不支持同时运动，目前策略优先变倍，无变倍才动云台
    // 国标规定变倍可以控制速度，这个我们设备也暂时不支持
    case 0x01: // 右
        GbJcpSend("pelcod20ctrl -type 1 -cmd 4 -data1 %d", DATA_TO_SPEED(data1));
        break;
    case 0x02: // 左
        GbJcpSend("pelcod20ctrl -type 1 -cmd 3 -data1 %d", DATA_TO_SPEED(data1));
        break;
    case 0x04: // 下
        GbJcpSend("pelcod20ctrl -type 1 -cmd 2 -data2 %d", DATA_TO_SPEED(data2));
        break;
    case 0x08: // 上
        GbJcpSend("pelcod20ctrl -type 1 -cmd 1 -data2 %d", DATA_TO_SPEED(data2));
        break;
    case 0x05: // 下右
        GbJcpSend("pelcod20ctrl -type 1 -cmd 6 -data1 %d -data2 %d", DATA_TO_SPEED(data1), DATA_TO_SPEED(data2));
        break;
    case 0x06: // 下左
        GbJcpSend("pelcod20ctrl -type 1 -cmd 8 -data1 %d -data2 %d", DATA_TO_SPEED(data1), DATA_TO_SPEED(data2));
        break;
    case 0x09: // 上右
        GbJcpSend("pelcod20ctrl -type 1 -cmd 5 -data1 %d -data2 %d", DATA_TO_SPEED(data1), DATA_TO_SPEED(data2));
        break;
    case 0x0A: // 上左
        GbJcpSend("pelcod20ctrl -type 1 -cmd 7 -data1 %d -data2 %d", DATA_TO_SPEED(data1), DATA_TO_SPEED(data2));
        break;
    case 0x10 ... 0x1F: // 变倍放大
        GbJcpSend("pelcod20ctrl -type 1 -cmd 10");
        break;
    case 0x20 ... 0x2F: // 变倍缩小
        GbJcpSend("pelcod20ctrl -type 1 -cmd 11");
        break;
    // 国标规定光圈和聚焦要能同时操作，目前不支持，优先光圈
    // 同时我们设备也不支持设置光圈和聚焦速度
    case 0x41: // 聚集远
        GbJcpSend("pelcod20ctrl -type 1 -cmd 14");
        break;
    case 0x42: // 聚焦近
        GbJcpSend("pelcod20ctrl -type 1 -cmd 13");
        break;
    case 0x44 ... 0x47: // 光圈放大
        GbJcpSend("pelcod20ctrl -type 1 -cmd 16");
        break;
    case 0x48 ... 0x4B: // 光圈缩小
        GbJcpSend("pelcod20ctrl -type 1 -cmd 17");
        break;
    // 国标规定预置位范围取值 0x00 ~ 0xff, 这中间有一些预置位被用做了其它功能，可能会设置失败，这里先不处理
    case 0x81: // 设置预置位
        GbJcpSend("pelcod20ctrl -type 2 -cmd 1 -data2 %d", data2);
        break;
    case 0x82: // 调用预置位
        GbJcpSend("pelcod20ctrl -type 2 -cmd 2 -data2 %d", data2);
        break;
    case 0x83: // 删除预置位
        GbJcpSend("pelcod20ctrl -type 2 -cmd 3 -data2 %d", data2);
        break;
    // 除了大球以外，其它产品巡航点应该是没用的
    case 0x84: // 加入巡航点
        GbJcpSend("pelcod20ctrl -type 3 -cmd 5 -data1 %d -data2 %d", data2, data1);
        break;
    case 0x85: // 删除一个巡航点
        GbJcpSend("pelcod20ctrl -type 3 -cmd 6 -data1 %d -data2 %d", data2, data1);
        break;
    case 0x86: // 设置巡航速度
        DBG("cmd[%d] not support\n", cmd); // 不支持
        break;
    case 0x87: // 设置巡航停留时间
        set_time = (data2 & 0xFF) | (data3 << 8);
        GB_DBG("set curise time:%d\n", set_time);
        GbJcpSend("pelcod20ctrl -type 3 -cmd 7 -data1 %d -data2 %d", set_time, data1);
        break;
    case 0x88: // 开始巡航
        GbJcpSend("pelcod20ctrl -type 3 -cmd 1 -data2 %d", data1);
        break;
    case 0x89: // 自动扫描
        DBG("cmd[%d] not support\n", cmd); // 不支持
        break;
    case 0x8a: // 设置自动扫描速度
        DBG("cmd[%d] not support\n", cmd); // 不支持
        break;
    case 0x8c: // 辅助开关：开
        DBG("cmd[%d] not support\n", cmd); // 不支持
        break;
    case 0x8d: // 辅助开关：关
        DBG("cmd[%d] not support\n", cmd); // 不支持
        break;
    case 0x00: // 停止
    case 0x40: // 停止
    case 0x80: // 停止
    default:
        GbJcpSend("pelcod20ctrl -type 1 -cmd 9");
        break;
    }
}

void GbClientManager::HandRecordCmd(mxml_node_t* record_cmd)
{
    const char *text = NULL;
    int whitespace = 0;

    text = mxmlGetText(record_cmd, &whitespace);
    if (text == NULL)
        return ;
    text += whitespace;

    // 目前这里开启只是打开手动录像，关闭只是关闭当前录像，马上又会开始接着录
    // 而且我们的关闭也只能关计划录像，这里要处理吗?
    if(!strcmp(text, "Record")) {  // 开始录像
        GbJcpSend("devrecordcfg -act set -recorden 1");
    } else if(!strcmp(text, "StopRecord")) { // 停止录像
        GbJcpSend("devrecordcfg -act set -recorden 0");
    } else {
        GB_DBG("record command not find\n");
        return ;
    }

    return ;
}

void GbClientManager::HandStreamNumber(mxml_node_t* stream_number)
{
    // 录像分辨率
    const char *text = NULL;
    int whitespace = 0;

    text = mxmlGetText(stream_number, &whitespace);
    if (text == NULL) {
        return ;
    }

    VideoEncS video_cfg = {0};
    conf_get_videocfg(&video_cfg);
    if (atoi(text) == 0) {
        GbJcpSend("devrecordcfg -act set -vencsize %d", video_cfg.enc[0].vencsize);
    } else if (atoi(text) == 1) {
        GbJcpSend("devrecordcfg -act set -vencsize %d", video_cfg.enc[1].vencsize);
    }

    return ;
}

void GbClientManager::SendUpgradeResult(GbClientManager::UpgradeStatus status)
{
    if (!IsRegister()) {
        return ;
    }

    if (upgrade_dialog_ == nullptr) {
        return ;
    }

    mxml_node_t *request_xml = NULL;

    do {
        request_xml = GenerateXml("Notify", osip_build_random_number(), "DeviceUpgradeResult", NULL, upgrade_dialog_->get_device_id());
        if (request_xml == NULL) {
            break;
        }

        mxml_node_t *notify_xml = mxmlFindElement(request_xml, request_xml, "Notify", NULL, NULL, MXML_DESCEND);
        mxml_node_t *session_id_xml = mxmlNewElement(notify_xml, "SessionID");
        mxmlNewTextf(session_id_xml, 0, "%s", upgrade_dialog_->get_session_id().c_str());

        /*升级结果*/
        mxml_node_t *upgrade_result_xml = mxmlNewElement(notify_xml, "UpgradeResult");
        mxml_node_t *firmware_xml = mxmlNewElement(notify_xml, "Firmware");
        if (status == GbClientManager::UpgradeStatus::UPGRADE_SUCCESS) {
            mxmlNewTextf(upgrade_result_xml, 0, "%s", "OK");
            mxmlNewTextf(firmware_xml, 0, "%s", upgrade_dialog_->get_firmware()); // 升级成功填写下发的版本
        } else {
            mxmlNewTextf(upgrade_result_xml, 0, "%s", "ERROR");
            mxmlNewTextf(firmware_xml, 0, "%s", get_fw_ver());     // 升级失败填写当前版本
            /*失败原因*/
            mxml_node_t *fail_xml = mxmlNewElement(notify_xml, "UpgradeFailedReason");
            mxmlNewTextf(fail_xml, 0, "%d", status);
        }

        /*xml 转成字符串*/
        char buf[4096] = {0};
        int xml_len = 0;
        xml_len = mxmlSaveString(notify_xml, buf, sizeof(buf), whitespace_cb);
        if (xml_len == 0) {
            break;
        }

        MessageRequestAddXmlAndSend(request_xml, upgrade_dialog_);
    } while (0);

    if (request_xml != NULL) {
        mxmlDelete(request_xml);
    }

    upgrade_dialog_ = nullptr; // 升级结束，置为未升级状态

    return ;
}

void GbClientManager::HandDeviceUpgrade(mxml_node_t* root, std::shared_ptr<MessageDialog> &ptr)
{
    mxml_node_t *firmware_xml = mxmlFindElement(root, root, "Firmware", NULL, NULL, MXML_DESCEND);
    mxml_node_t *file_url_xml = mxmlFindElement(root, root, "FileURL", NULL, NULL, MXML_DESCEND);
    mxml_node_t *session_id_xml = mxmlFindElement(root, root, "SessionID", NULL, NULL, MXML_DESCEND);
    mxml_node_t *sn_xml         = mxmlFindElement(root, root, "SN", NULL, NULL, MXML_DESCEND);
    mxml_node_t *device_id_xml  = mxmlFindElement(root, root, "DeviceID", NULL, NULL, MXML_DESCEND);

    int whitespace = 0;
    const char *text = NULL;
    // Firmware
    const char *firmware = NULL;
    text = mxmlGetText(firmware_xml, &whitespace);
    if (text == NULL) {
        return ;
    }
    firmware = text;

    //FileURL
    const char *file_url = NULL;
    text = mxmlGetText(file_url_xml, &whitespace);
    if (text == NULL) {
        return ;
    }
    file_url = text;

    // SessionID
    const char *session_id = NULL;
    text = mxmlGetText(session_id_xml, &whitespace);
    if (text == NULL) {
        return ;
    }
    session_id = text;

    // SN
    int sn = 0;
    text = mxmlGetText(sn_xml, &whitespace);
    if (text == NULL)
        return ;
    sn = atoi(text);

    // DeviceID
    const char *device_id = NULL;
    text = mxmlGetText(device_id_xml, &whitespace);
    if (text == NULL)
        return ;
    device_id = text;

    // 设置会话信息
    ptr->set_type(MessageDialog::Type::CONCTRL_UPGRADE);
    ptr->set_sn(sn);
    ptr->set_session_id(session_id);
    ptr->set_device_id(device_id);
    ptr->set_firmware(firmware);
    ptr->set_timeout(5 * 60); // 默认 5 分钟超时时间

    upgrade_dialog_ = ptr;

    GbJcpSend("otaUpgradeUrl -act set -url %s", file_url); // 发送升级命令

    return ;
}


void GbClientManager::HandGuardCmd(mxml_node_t* guard_cmd)
{
    const char *text = NULL;
    int whitespace = 0;

    text = mxmlGetText(guard_cmd, &whitespace);
    if (text == NULL)
        return ;

    text += whitespace;

    if(!strcmp(text, "SetGuard")) { /*打开报警布防*/
        set_duty_status(ONDUTY);
    } else if(!strcmp(text, "ResetGuard")) { /*关闭报警布防*/
        set_duty_status(OFFDUTY);
    } else {
        GB_DBG("guard command not find\n");
    }
}

void GbClientManager::SetHomePosition(mxml_node_t *root)
{
    const char *text = NULL;
    int whitespace = 0;
    uint32_t int_enable = 0;
    uint32_t int_time = 0;
    uint32_t int_index = 0;

    mxml_node_t *enable = mxmlFindElement(root, root, "Enabled", NULL, NULL, MXML_DESCEND);
    mxml_node_t *time = mxmlFindElement(root, root, "ResetTime", NULL, NULL, MXML_DESCEND);
    mxml_node_t *index = mxmlFindElement(root, root, "PresetIndex", NULL, NULL, MXML_DESCEND);
    /*Enabled : 看守位使能1:开启,0:关闭*/
    /*ResetTime : 自动归位时间间隔,开启看守位时使用,单位:秒(s)*/
    /*PresetIndex : 调用预置位编号,开启看守位时使用,取值范围0~255*/

    text = mxmlGetText(enable, &whitespace);
    if (text == NULL) {
        GB_ERR("not find enable\n");
        return;
    }

    int_enable = atoi(text);
    if (!int_enable) {  // 关闭使能，未使能其余参数不用关注
        GbJcpSend("followcfg -act set -enable 0");
    } else { // 使能，时间和预置位编号是可选的
        char jcp_cmd[128] = "followcfg -act set -enable 1 ";

        // 时间
        text = mxmlGetText(time, &whitespace);
        if (text != NULL) {
            int_time = atoi(text);
            if (int_time < 10 || int_time > 600) { // 网页上对时间的限制是 10~600
                int_time = 16;
            }
            char str[128] = {0};
            snprintf(str, sizeof(str), "-idle %u ", int_time);
            strncat(jcp_cmd, str, sizeof(jcp_cmd) - 1);
        }

        // 编号
        text = mxmlGetText(index, &whitespace);
        if (text != NULL) {
            int_index = atoi(text);
            int_index = int_index % 5 + 1;  // 网页上对看守位的限制是 1~6

            char str[128] = {0};
            snprintf(str, sizeof(str), "-preset %u ", int_index);
            strncat(jcp_cmd, str, sizeof(jcp_cmd) - 1);
        }

        GbJcpSend(jcp_cmd);
    }

}

void GbClientManager::HandHomePosition(mxml_node_t* root, mxml_node_t* home_position)
{
    const char *text = NULL;
    int whitespace = 0;

    text = mxmlGetText(home_position, &whitespace);
    if (text == NULL)
        return ;

    text += whitespace;

    if(!strcasecmp(text, "HomePosition")) {
        SetHomePosition(root);
    } else {
        GB_DBG("HomePosition action not find\n");
    }
}

void GbClientManager::SetBasicParm(mxml_node_t *root)
{
    const char *devsysname_text = NULL;
    const char *text = NULL;
    int whitespace = 0;
    uint32_t int_expiration = 0;
    uint32_t int_reginterval = 0;
    uint32_t int_heart_beat_count = 0;

    mxml_node_t *name_xml = mxmlFindElement(root, root, "Name", NULL, NULL, MXML_DESCEND);
    mxml_node_t *expiration = mxmlFindElement(root, root, "Expiration", NULL, NULL, MXML_DESCEND);
    mxml_node_t *heart_beat_interval = mxmlFindElement(root, root, "HeartBeatInterval", NULL, NULL, MXML_DESCEND);
    mxml_node_t *heart_beat_count = mxmlFindElement(root, root, "HeartBeatCount", NULL, NULL, MXML_DESCEND);//暂时没有

    /*文档上说明这些参数都是可选，这里的判断是这些参数都是必选*/
    if(name_xml == NULL || expiration  == NULL || heart_beat_interval == NULL || heart_beat_count == NULL) {
        GB_ERR("Information is incomplete!\n");
        return ;
    }
    // 设备名称，要用 UTF8 存储
    devsysname_text = mxmlGetText(name_xml, &whitespace);
    if (devsysname_text == NULL)
        return ;
    devsysname_text += whitespace;

    // 注册过期时间
    text = mxmlGetText(expiration, &whitespace);
    if (text == NULL)
        return ;
    int_expiration = atoi(text);

    // 心跳间隔
    text = mxmlGetText(heart_beat_interval, &whitespace);
    if (text == NULL)
        return ;
    int_reginterval = atoi(text);

    // 心跳超时次数
    text = mxmlGetText(heart_beat_count, &whitespace);
    if (text == NULL)
        return ;
    int_heart_beat_count = atoi(text);

    // 动态配置心跳超时次数，目前我们的配置是不会保存的
    profile_.set_heartbeat_timeout_count(int_heart_beat_count);

    GbJcpSend("guobiaocfg -act set -devsysname %s -reginterval %u -hbinterval %u ", devsysname_text,
        int_expiration, int_reginterval);
}

void GbClientManager::SetSvacEncodeParm(mxml_node_t *root)
{
    /*  svac 编码相关设置：感兴趣区域、背景区域编码质量等级、背景跳过开关、音频参数
    *       感兴趣区域：现在设备基本都不支持了，这里不处理
    *       背景区域编码质量等级和背景跳过开关设备都不支持
    *       音频参数是设置声音识别特征开关，设备也不支持
    */
    // 设置的几项都不支持，这里不做处理，要处理再添加
    return ;
}

void GbClientManager::SetSvacDecodeParm(mxml_node_t *root)
{
    /*  svac 解码相关设置：
    *       码流显示模式,取值0:基本层码流单独显示方式;1:基本层+1个增强层码流方式;2:基本层+2个增强层码流方式;3:基本层+3个增强层码流方式
    *       监控专用信息参数：绝对时间信息显示开关、监控事件信息显示开关、报警信息显示开关
    */
    // 设置的几项都不支持，这里不做处理，要处理再添加
    return ;
}

void GbClientManager::SetVideoParamAttribute(mxml_node_t *video_param_attribute)
{
    int whitespace = 0;
    const char *text = NULL;
    VideoEncS video_info = {0};
    conf_get_videocfg(&video_info);

    mxml_node_t *item_xml = video_param_attribute;

    while ((item_xml = mxmlFindElement(item_xml, video_param_attribute, "Item", NULL, NULL, MXML_DESCEND)) != NULL) {
        // 获取通道编号
        int stream_number = 0;
        mxml_node_t *stream_number_xml = mxmlFindElement(item_xml, item_xml, "StreamNumber", NULL, NULL, MXML_DESCEND);
        if (stream_number_xml == NULL) {
            continue;
        }
        text = mxmlGetText(stream_number_xml, &whitespace);
        if (text == NULL) {
            continue;
        }
        stream_number = atoi(text);
        if (stream_number < 0 || stream_number > video_info.gnum) {
            continue;
        }

        // 获取编码格式
        mxml_node_t *video_format_xml = mxmlFindElement(item_xml, item_xml, "VideoFormat", NULL, NULL, MXML_DESCEND);
        if (video_format_xml == NULL) {
            continue;
        }
        text = mxmlGetText(video_format_xml, &whitespace);
        if (text == NULL) {
            continue;
        }
        VENC_FORMAT_E codec = SdpParse::VideoFormatToCodec(static_cast<GbVideoCodecType>(atoi(text)));
        if (codec != VENC_FORMAT_BEGIN) {
            video_info.enc[stream_number].codec = codec;
        }

        // 获取分辨率
        mxml_node_t *resolution_xml = mxmlFindElement(item_xml, item_xml, "Resolution", NULL, NULL, MXML_DESCEND);
        if (resolution_xml == NULL) {
            continue;
        }
        text = mxmlGetText(resolution_xml, &whitespace);
        if (text == NULL) {
            continue;
        }
        VencSizeE vencsize = SdpParse::ResolutionToVencsize(text + whitespace);
        if (vencsize != VencSizeE_BEGIN) {
            video_info.enc[stream_number].vencsize = vencsize;
        }

        // 获取帧率
        mxml_node_t *frame_rate_xml = mxmlFindElement(item_xml, item_xml, "FrameRate", NULL, NULL, MXML_DESCEND);
        if (frame_rate_xml == NULL) {
            continue;
        }
        text = mxmlGetText(frame_rate_xml, &whitespace);
        if (text == NULL) {
            continue;
        }
        video_info.enc[stream_number].fps = atoi(text);

        // 获取码率类型
        mxml_node_t *bit_rate_type_xml = mxmlFindElement(item_xml, item_xml, "BitRateType", NULL, NULL, MXML_DESCEND);
        if (bit_rate_type_xml == NULL) {
            continue;
        }
        text = mxmlGetText(bit_rate_type_xml, &whitespace);
        if (text == NULL) {
            continue;
        }
        video_info.enc[stream_number].fixbps = SdpParse::BitRateTypeToFixbps(static_cast<GbBitrateControl>(atoi(text)));

        // 获取码率配置
        mxml_node_t *bit_rate_xml = mxmlFindElement(item_xml, item_xml, "VideoBitRate", NULL, NULL, MXML_DESCEND);
        if (bit_rate_xml == NULL) {
            continue;
        }
        text = mxmlGetText(bit_rate_xml, &whitespace);
        if (text == NULL) {
            continue;
        }
        video_info.enc[stream_number].bps = atoi(text);
    }

    conf_set_videocfg(video_info);

    return ;
}

void GbClientManager::SetVideoRecordPlan(mxml_node_t *video_record_plan)
{
    int whitespace = 0;
    const char *text = NULL;
    // 布防时间转换
    RecordCtrlS record_ctrl = {0};
    conf_get_recordcfg(&record_ctrl);
    GbClientManager::RecordWeeklyPlan weekly_plan;
    weekly_plan.JcoToGbRecordPlan(record_ctrl.timestrategy);

    do {
        // 使能判断
        mxml_node_t *record_enable_xml = mxmlFindElement(video_record_plan, video_record_plan, "RecordEnable", NULL, NULL, MXML_DESCEND);
        if (record_enable_xml == NULL) {
            break;
        }
        text = mxmlGetText(record_enable_xml, &whitespace);
        if (text == NULL) {
            break;
        }
        // 先统一关闭计划，后续有设置时间段再打开
        for (int i = 0; i < 7; ++i) {
            weekly_plan.week_day[i].num = 0;
        }
        if (atoi(text) == 0) {
            // 未使能，前面把录像计划都关闭了，这里直接退出
            break;
        }

        // 码流类型
        mxml_node_t *stream_number_xml = mxmlFindElement(video_record_plan, video_record_plan, "StreamNumber", NULL, NULL, MXML_DESCEND);
        if (stream_number_xml == NULL) {
            break;
        }
        text = mxmlGetText(stream_number_xml, &whitespace);
        if (text == NULL) {
            break;
        }
        int stream_number = atoi(text);
        VideoEncS video_cfg = {0};
        conf_get_videocfg(&video_cfg);
        if (stream_number < 0 || stream_number >= video_cfg.gnum) {
            // 参数错误
            break;
        }
        record_ctrl.rec_type = stream_number;

        // 录像计划总天数
        mxml_node_t *record_schedule_sun_num_xml = mxmlFindElement(video_record_plan, video_record_plan, "RecordScheduleSumNum", NULL, NULL, MXML_DESCEND);
        if (record_schedule_sun_num_xml == NULL) {
            break;
        }
        text = mxmlGetText(record_schedule_sun_num_xml, &whitespace);
        if (text == NULL) {
            break;
        }
        if (atoi(text) <= 0 || atoi(text) > 7) {
            // 未设置计划或者参数错误，后续不进行解析
            break;
        }

        // 解析录像计划时间段
        mxml_node_t *record_schedule_xml = video_record_plan;
        while ((record_schedule_xml = mxmlFindElement(record_schedule_xml, video_record_plan, "RecordSchedule", NULL, NULL, MXML_DESCEND)) != NULL) {
            // 解析周几
            mxml_node_t *week_day_num_xml = mxmlFindElement(record_schedule_xml, record_schedule_xml, "WeekDayNum", NULL, NULL, MXML_DESCEND);
            if (week_day_num_xml == NULL) {
                continue;
            }
            text = mxmlGetText(week_day_num_xml, &whitespace);
            if (text == NULL) {
                continue;
            }
            int week_day_num = atoi(text);
            if (week_day_num == 7) {
                // 国标取值范围是 1~7 转换成 jco 0~6，0 对应 7 表示周日
                week_day_num = 0;
            }
            if (week_day_num < 0 || week_day_num > 6) {
                continue;
            }

            // 每天录像计划总段数
            mxml_node_t *time_segment_sum_num_xml = mxmlFindElement(record_schedule_xml, record_schedule_xml, "TimeSegmentSumNum", NULL, NULL, MXML_DESCEND);
            if (time_segment_sum_num_xml == NULL) {
                continue;
            }
            text = mxmlGetText(time_segment_sum_num_xml, &whitespace);
            if (text == NULL || atoi(text) == 0) {  // 段数为 0 后面就没必要解析了
                continue;
            }

            // 时间段解析
            mxml_node_t *time_segment_xml = record_schedule_xml;
            while ((time_segment_xml = mxmlFindElement(time_segment_xml, record_schedule_xml, "TimeSegment", NULL, NULL, MXML_DESCEND)) != NULL) {
                int start_hour = -1;
                int start_min  = 0;
                int start_sec  = 0;
                int stop_hour  = 0;
                int stop_min   = 59;
                int stop_sec   = 59;
                // 解析开始小时
                mxml_node_t *start_hour_xml = mxmlFindElement(time_segment_xml, time_segment_xml, "StartHour", NULL, NULL, MXML_DESCEND);
                if (start_hour_xml == NULL) {
                    continue;
                }
                text = mxmlGetText(start_hour_xml, &whitespace);
                if (text == NULL) {
                    continue;
                }
                start_hour = atoi(text);
                // 解析开始分钟
                mxml_node_t *start_min_xml = mxmlFindElement(time_segment_xml, time_segment_xml, "StartMin", NULL, NULL, MXML_DESCEND);
                if (start_min_xml == NULL) {
                    continue;
                }
                text = mxmlGetText(start_min_xml, &whitespace);
                if (text == NULL) {
                    continue;
                }
                start_min = atoi(text);
                // 解析开始秒
                mxml_node_t *start_sec_xml = mxmlFindElement(time_segment_xml, time_segment_xml, "StartSec", NULL, NULL, MXML_DESCEND);
                if (start_sec_xml == NULL) {
                    continue;
                }
                text = mxmlGetText(start_sec_xml, &whitespace);
                if (text == NULL) {
                    continue;
                }
                start_sec = atoi(text);
                // 解析结束小时
                mxml_node_t *stop_hour_xml = mxmlFindElement(time_segment_xml, time_segment_xml, "StopHour", NULL, NULL, MXML_DESCEND);
                if (stop_hour_xml == NULL) {
                    continue;
                }
                text = mxmlGetText(stop_hour_xml, &whitespace);
                if (text == NULL) {
                    continue;
                }
                stop_hour = atoi(text);
                // 解析结束分钟
                mxml_node_t *stop_min_xml = mxmlFindElement(time_segment_xml, time_segment_xml, "StopMin", NULL, NULL, MXML_DESCEND);
                if (stop_min_xml == NULL) {
                    continue;
                }
                text = mxmlGetText(stop_min_xml, &whitespace);
                if (text == NULL) {
                    continue;
                }
                stop_min = atoi(text);
                // 解析结束秒
                mxml_node_t *stop_sec_xml = mxmlFindElement(time_segment_xml, time_segment_xml, "StopSec", NULL, NULL, MXML_DESCEND);
                if (stop_sec_xml == NULL) {
                    continue;
                }
                text = mxmlGetText(stop_sec_xml, &whitespace);
                if (text == NULL) {
                    continue;
                }
                stop_sec = atoi(text);

                weekly_plan.week_day[week_day_num].AddTimeSegment(start_hour, start_min, start_sec, stop_hour, stop_min, stop_sec);
            }
        }
    } while (0);

    weekly_plan.GbRecordPlanToJco(record_ctrl.timestrategy);
    conf_set_recordcfg(record_ctrl);

    return ;
}

void GbClientManager::SetVideoAlarmRecord(mxml_node_t *video_alarm_record)
{
    int whitespace = 0;
    const char *text = NULL;

    RecordCtrlS record_ctrl = {0};
    conf_get_recordcfg(&record_ctrl);
    VideoEncS video_cfg = {0};
    conf_get_videocfg(&video_cfg);

    do {
        // 使能，目前报警录像不支持关闭
        mxml_node_t *record_enable_xml = mxmlFindElement(video_alarm_record, video_alarm_record, "RecordEnable", NULL, NULL, MXML_DESCEND);
        if (record_enable_xml == NULL) {
            break;
        }
        text = mxmlGetText(record_enable_xml, &whitespace);
        if (text == NULL) {
            break;
        }

        // 预录制时间，可选
        mxml_node_t *record_time_xml = mxmlFindElement(video_alarm_record, video_alarm_record, "PreRecordTime", NULL, NULL, MXML_DESCEND);
        if (record_time_xml != NULL) {
            text = mxmlGetText(record_time_xml, &whitespace);
            if (text != NULL) {
                record_ctrl.prerecordtime = atoi(text);
            }
        }

        // 码流编号
        mxml_node_t *stream_number_xml = mxmlFindElement(video_alarm_record, video_alarm_record, "StreamNumber", NULL, NULL, MXML_DESCEND);
        if (stream_number_xml == NULL) {
            break;
        }
        text = mxmlGetText(stream_number_xml, &whitespace);
        if (text == NULL) {
            break;
        }
        int stream_number = atoi(text);
        if (stream_number < 0 || stream_number >= video_cfg.gnum) {
            // 参数错误
            break;
        }
        record_ctrl.rec_type = stream_number;
    } while (0);

    conf_set_recordcfg(record_ctrl);
    return ;
}

void GbClientManager::SetPictrueMask(mxml_node_t *pictrue_mask)
{
#if 0
    int whitespace = 0;
    const char *text = NULL;

    VideoMaskS video_mask = {0};
    conf_get_videomaskcfg(&video_mask);
    do {
        // 开关
        mxml_node_t *on_xml = mxmlFindElement(pictrue_mask, pictrue_mask, "On", NULL, NULL, MXML_DESCEND);
        if (on_xml == NULL) {
            break;
        }
        text = mxmlGetText(on_xml, &whitespace);
        if (text == NULL) {
            break;
        }
        // 目前有四块画面遮挡，先统一关闭，有设置计划再打开
        for (int i = 0; i < 4; ++i) {
            video_mask.mask[i].enable = 0;
        }
        int on = atoi(text);
        if (on == 0) {
            // 未使能，保持关闭状态退出
            break;
        }

        // 区域总数
        mxml_node_t *sum_num_xml = mxmlFindElement(pictrue_mask, pictrue_mask, "SumNum", NULL, NULL, MXML_DESCEND);
        if (sum_num_xml == NULL) {
            break;
        }
        text = mxmlGetText(sum_num_xml, &whitespace);
        if (text == NULL) {
            break;
        }
        if (atoi(text) <= 0) {
            break;
        }

        // 区域解析
        mxml_node_t *region_list_xml = mxmlFindElement(pictrue_mask, pictrue_mask, "RegionList", NULL, NULL, MXML_DESCEND);
        if (region_list_xml == NULL) {
            break;
        }

        mxml_node_t *item_xml = region_list_xml;
        while ((item_xml = mxmlFindElement(item_xml, region_list_xml, "Item", NULL, NULL, MXML_DESCEND)) != NULL) {
            // 区域编号
            mxml_node_t *seq_xml = mxmlFindElement(item_xml, item_xml, "Seq", NULL, NULL, MXML_DESCEND);
            if (seq_xml == NULL) {
                break;
            }
            text = mxmlGetText(seq_xml, &whitespace);
            if (text == NULL) {
                break;
            }
            int seq = atoi(text);
            if (seq < 0 || seq > 3) {
                break;
            }

            // 区域坐标
            mxml_node_t *point_xml = mxmlFindElement(item_xml, item_xml, "Point", NULL, NULL, MXML_DESCEND);
            if (point_xml == NULL) {
                break;
            }
            text = mxmlGetText(point_xml, &whitespace);
            if (text == NULL) {
                break;
            }
            int x0 = 0, y0 = 0;
            int x1 = 0, y1 = 0;
            sscanf(text + whitespace, "%d,%d,%d,%d", &x0, &y0, &x1, &y1);
            video_mask.mask[seq].x0 = x0;
            video_mask.mask[seq].y0 = y0;
            video_mask.mask[seq].x1 = x1;
            video_mask.mask[seq].y1 = y1;

            video_mask.mask[seq].enable = 1;
        }
    } while (0);

    conf_set_videomaskcfg(video_mask);
#endif
}

void GbClientManager::SetFrameMirror(mxml_node_t *frame_mirror)
{
    int whitespace = 0;
    const char *text = NULL;

    ViInfoS video_info = {0};
    conf_get_viinfocfg(&video_info);

    text = mxmlGetText(frame_mirror, &whitespace);
    if (text == NULL) {
        return;
    }

    video_info.reverse = atoi(text);
    conf_set_viinfocfg(video_info);

    return ;
}

void GbClientManager::SetAlarmReport(mxml_node_t *alarm_report)
{
    int whitespace = 0;
    const char *text = NULL;
    // 我们目前不支持多算法运行，这里直接把报警上报开关作为算法开关
    // 移动侦测
    MotionDetectS motion_detect = {0};
    conf_get_motiondetectcfg(&motion_detect);
    mxml_node_t *motion_detection_xml = mxmlFindElement(alarm_report, alarm_report, "MotionDetection", NULL, NULL, MXML_DESCEND);
    if (motion_detection_xml == NULL) {
        return ;
    }
    text = mxmlGetText(motion_detection_xml, &whitespace);
    if (text == NULL) {
        return ;
    }
    motion_detect.enable = atoi(text);
    conf_set_motiondetectcfg(motion_detect);

    // 区域入侵
    VgrectS vg_rect = {0};
    conf_get_vgrectcfg(&vg_rect);
    mxml_node_t *field_detection_xml = mxmlFindElement(alarm_report, alarm_report, "FieldDetection", NULL, NULL, MXML_DESCEND);
    if (field_detection_xml == NULL) {
        return ;
    }
    text = mxmlGetText(field_detection_xml, &whitespace);
    if (text == NULL) {
        return ;
    }
    vg_rect.enable = atoi(text);
    conf_set_vgrectcfg(vg_rect);

    return ;
}

void GbClientManager::SetOSDConfig(mxml_node_t *osd_config)
{
    int whitespace = 0;
    const char *text = NULL;

    OsdInfoS osd_info = {0};
    conf_get_osdinfocfg(&osd_info);
    OsdExpandS osd_expand = {0};
    conf_get_osdexpandcfg(&osd_expand);
    do {
        // 配置窗口长度
        mxml_node_t *length_xml = mxmlFindElement(osd_config, osd_config, "Length", NULL, NULL, MXML_DESCEND);
        if (length_xml == NULL) {
            break ;
        }
        text = mxmlGetText(length_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        int length = atoi(text);

        // 配置串口宽度
        mxml_node_t *width_xml = mxmlFindElement(osd_config, osd_config, "Width", NULL, NULL, MXML_DESCEND);
        if (width_xml == NULL) {
            break ;
        }
        text = mxmlGetText(width_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        int width = atoi(text);

        // 时间 x 坐标，我们的坐标系是 1920*1080
        mxml_node_t *time_x_xml = mxmlFindElement(osd_config, osd_config, "TimeX", NULL, NULL, MXML_DESCEND);
        if (time_x_xml == NULL) {
            break ;
        }
        text = mxmlGetText(time_x_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        osd_info.timeleft = atoi(text) * 1920 / length;

        // 时间 y 坐标
        mxml_node_t *time_y_xml = mxmlFindElement(osd_config, osd_config, "TimeY", NULL, NULL, MXML_DESCEND);
        if (time_y_xml == NULL) {
            break ;
        }
        text = mxmlGetText(time_y_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        osd_info.timetop = atoi(text) * 1080 / width;

        // 时间使能
        mxml_node_t *time_enable_xml = mxmlFindElement(osd_config, osd_config, "TimeEnable", NULL, NULL, MXML_DESCEND);
        if (time_enable_xml == NULL) {
            break ;
        }
        text = mxmlGetText(time_enable_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        osd_info.timeen = atoi(text);

        // 时间连接符
        mxml_node_t *time_type_xml = mxmlFindElement(osd_config, osd_config, "TimeType", NULL, NULL, MXML_DESCEND);
        if (time_type_xml == NULL) {
            break ;
        }
        text = mxmlGetText(time_type_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
#ifdef _OSDCODE_
        // 0 为没有中文，1 为有中文，目前只有 _OSDCODE_ 支持设置中文 osd
        if (atoi(text) == 1) {
            osd_info.dateformat = 0;
        } else {
            osd_info.dateformat = 2;
        }
#endif

        // 显示文字开关
        mxml_node_t *text_enable_xml = mxmlFindElement(osd_config, osd_config, "TextEnable", NULL, NULL, MXML_DESCEND);
        if (text_enable_xml == NULL) {
            break ;
        }
        text = mxmlGetText(text_enable_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        int enable = atoi(text);

        // 显示文字行数总数
        mxml_node_t *sum_num_xml = mxmlFindElement(osd_config, osd_config, "SumNum", NULL, NULL, MXML_DESCEND);
        if (sum_num_xml == NULL) {
            break ;
        }
        text = mxmlGetText(sum_num_xml, &whitespace);
        if (text == NULL) {
            break ;
        }
        int sum_num = atoi(text);
        if (sum_num < 0) {
            break;
        }

        // 目前限制只能设置两个文字 osd，有需要再添加
        for (int i = 0; i < 2; ++i) {
            // 先把 osd 设置为关闭，后续工具解析情况再打开
            if (i == 0) {
                osd_info.nameen = 0;
            } else {
                osd_expand.cusosd[i - 1].enable = 0;
            }
        }
        if (enable == 0) {
            // 未使能显示，后面不解析了
            break;
        }
        int no = 0;
        mxml_node_t *item_xml = osd_config;
        while ((item_xml = mxmlFindElement(item_xml, osd_config, "Item", NULL, NULL, MXML_DESCEND)) != NULL) {
            // 文字内容
            mxml_node_t *text_xml = mxmlFindElement(item_xml, item_xml, "Text", NULL, NULL, MXML_DESCEND);
            if (text_xml == NULL) {
                continue ;
            }
            text = mxmlGetText(text_xml, &whitespace);
            if (text == NULL) {
                continue ;
            }
            const char *text_str = text + whitespace;

            // X 坐标
            mxml_node_t *x_xml = mxmlFindElement(item_xml, item_xml, "X", NULL, NULL, MXML_DESCEND);
            if (x_xml == NULL) {
                continue ;
            }
            text = mxmlGetText(x_xml, &whitespace);
            if (text == NULL) {
                continue ;
            }
            int x = atoi(text);

            // Y 坐标
            mxml_node_t *y_xml = mxmlFindElement(item_xml, item_xml, "Y", NULL, NULL, MXML_DESCEND);
            if (y_xml == NULL) {
                continue ;
            }
            text = mxmlGetText(y_xml, &whitespace);
            if (text == NULL) {
                continue ;
            }
            int y = atoi(text);

            if (no == 0) {
                osd_info.nameen = 1;
                strncpy(osd_info.name, text_str, sizeof(osd_info.name) - 1);
                osd_info.nameleft = x;
                osd_info.nametop = y;
            } else if (no - 1 < ARRAY_SIZE(osd_expand.cusosd)) {
                osd_expand.cusosd[no - 1].enable = 1;
                strncpy(osd_expand.cusosd[no - 1].content, text_str, sizeof(osd_expand.cusosd[no - 1].content) - 1);
                osd_expand.cusosd[no - 1].x = x;
                osd_expand.cusosd[no - 1].y = y;
            }
            no++;
        }
    } while (0);

    conf_set_osdinfocfg(osd_info);
    conf_set_osdexpandcfg(osd_expand);
    return ;
}

void GbClientManager::HandDevideConfig(mxml_node_t* root)
{
    /*设备配置命令全部是有应答的，除了 200 OK 以外还要回复 message*/
    mxml_node_t *basic_param = mxmlFindElement(root, root, "BasicParam", NULL, NULL, MXML_DESCEND);
    mxml_node_t *svac_encode_config = mxmlFindElement(root, root, "SVACEncodeConfig", NULL, NULL, MXML_DESCEND);
    mxml_node_t *svac_decode_config = mxmlFindElement(root, root, "SVACDecodeConfig", NULL, NULL, MXML_DESCEND);
#ifdef GB28181_2022
    mxml_node_t *video_param_attribute = mxmlFindElement(root, root, "VideoParamAttribute", NULL, NULL, MXML_DESCEND);
    mxml_node_t *video_record_plan = mxmlFindElement(root, root, "VideoRecordPlan", NULL, NULL, MXML_DESCEND);
    mxml_node_t *video_alarm_record = mxmlFindElement(root, root, "VideoAlarmRecord", NULL, NULL, MXML_DESCEND);
    mxml_node_t *pictrue_mask = mxmlFindElement(root, root, "PictrueMask", NULL, NULL, MXML_DESCEND);
    mxml_node_t *frame_mirror = mxmlFindElement(root, root, "FrameMirror", NULL, NULL, MXML_DESCEND);
    mxml_node_t *alarm_report = mxmlFindElement(root, root, "AlarmReport", NULL, NULL, MXML_DESCEND);
    mxml_node_t *osd_config = mxmlFindElement(root, root, "OSDConfig", NULL, NULL, MXML_DESCEND);
    mxml_node_t *snapshot_cfg= mxmlFindElement(root, root, "SnapShotConfig", NULL, NULL, MXML_DESCEND);
#endif

    std::shared_ptr<MessageDialog> ptr = std::make_shared<MessageDialog>();
    const char *response_str = "OK";

    if(basic_param) {
        GB_DBG("BasicParam\n");
        SetBasicParm(root);
    } else if(svac_encode_config) {
        GB_DBG("SVACEncodeConfig\n");
        SetSvacEncodeParm(root);
    } else if(svac_decode_config) {
        GB_DBG("SVACDecodeConfig\n");
        SetSvacDecodeParm(root);//SVAC解码配置
#ifdef GB28181_2022
    } else if(video_param_attribute) {
        GB_DBG("VideoParamAttribute\n");
        SetVideoParamAttribute(video_param_attribute);//配置视频参数属性
    } else if(video_record_plan) {
        GB_DBG("VideoRecordPlan\n");
        SetVideoRecordPlan(video_record_plan);//计划录像配置
    } else if(video_alarm_record) {
        GB_DBG("VideoAlarmRecord\n");
        SetVideoAlarmRecord(video_alarm_record);//报警录像配置
    } else if(pictrue_mask) {
        GB_DBG("PictrueMask\n");
        SetPictrueMask(pictrue_mask);//图片遮挡
    } else if(frame_mirror) {
        GB_DBG("FrameMirror\n");
        SetFrameMirror(frame_mirror);//画面翻转
    } else if(alarm_report) {
        GB_DBG("AlarmReport\n");
        SetAlarmReport(alarm_report);//报警上报开关
    } else if(osd_config) {
        GB_DBG("OSDConfig\n");
        SetOSDConfig(osd_config);//osd 配置
    } else if (snapshot_cfg) {  // 图片抓拍
        DBG("SnapShotConfig\n");
        HandSnapshot(root, ptr);
#endif
    } else {
        response_str = "ERROR";
        GB_ERR("Unknow command!!");
    }

    mxml_node_t *reply_xml = NULL;
    reply_xml = GenerateXmlResponse(root, response_str);
    if (reply_xml != NULL) {
        MessageRequestAddXmlAndSend(reply_xml, ptr);
        mxmlDelete(reply_xml);
    }
}

void GbClientManager::HandSnapshot(mxml_node_t* root, std::shared_ptr<MessageDialog> &ptr)
{
    mxml_node_t *snap_num_xml   = mxmlFindElement(root, root, "SnapNum", NULL, NULL, MXML_DESCEND);
    mxml_node_t *interval_xml   = mxmlFindElement(root, root, "Interval", NULL, NULL, MXML_DESCEND);
    mxml_node_t *upload_url_xml = mxmlFindElement(root, root, "UploadURL", NULL, NULL, MXML_DESCEND);
    mxml_node_t *session_id_xml = mxmlFindElement(root, root, "SessionID", NULL, NULL, MXML_DESCEND);
    mxml_node_t *sn_xml         = mxmlFindElement(root, root, "SN", NULL, NULL, MXML_DESCEND);
    mxml_node_t *device_id_xml  = mxmlFindElement(root, root, "DeviceID", NULL, NULL, MXML_DESCEND);

    if (snap_num_xml == NULL || interval_xml == NULL || sn_xml == NULL ||
            upload_url_xml == NULL || session_id_xml == NULL || ptr == NULL) {
        GB_ERR("snapshot info error\n");
        return ;
    }

    uint32_t snap_num = 0, interval = 0, sn = 0;
    const char *url = NULL, *session_id = NULL, *device_id = NULL;

    const char *text = NULL;
    int whitespace = 0;
    // 抓拍图片数量
    text = mxmlGetText(snap_num_xml, &whitespace);
    if (text == NULL)
        return ;
    text += whitespace;
    snap_num = atoi(text);

    // 抓拍间隔
    text = mxmlGetText(interval_xml, &whitespace);
    if (text == NULL)
        return ;
    text += whitespace;
    interval = atoi(text);

    // url
    text = mxmlGetText(upload_url_xml, &whitespace);
    if (text == NULL)
        return ;
    text += whitespace;
    url = text;

    // session id
    text = mxmlGetText(session_id_xml, &whitespace);
    if (text == NULL)
        return ;
    text += whitespace;
    session_id = text;

    // SN
    text = mxmlGetText(sn_xml, &whitespace);
    if (text == NULL)
        return ;
    sn = atoi(text);

    // DeviceID
    text = mxmlGetText(device_id_xml, &whitespace);
    if (text == NULL)
        return ;
    text += whitespace;
    device_id = text;

    // 设置会话信息
    ptr->set_type(MessageDialog::Type::CONCTRL_SNAPSHOT);
    ptr->set_sn(sn);
    ptr->set_session_id(session_id);
    ptr->set_device_id(device_id);

    // 开始抓拍
    snapshot_->StartSnapshot(snap_num, interval, url, ptr);

    return ;
}

void GbClientManager::HandDeviceControl(mxml_node_t* root)
{
    bool need_default_reply = false;

    mxml_node_t *ptz_cmd = mxmlFindElement(root, root, "PTZCmd", NULL, NULL, MXML_DESCEND);
    mxml_node_t *record_cmd = mxmlFindElement(root, root, "RecordCmd", NULL, NULL, MXML_DESCEND);
    mxml_node_t *tele_boot = mxmlFindElement(root, root, "TeleBoot", NULL, NULL, MXML_DESCEND);
    mxml_node_t *guard_cmd = mxmlFindElement(root, root, "GuardCmd", NULL, NULL, MXML_DESCEND);
    mxml_node_t *alarm_cmd = mxmlFindElement(root, root, "AlarmCmd", NULL, NULL, MXML_DESCEND);
    mxml_node_t *i_fame_cmd = mxmlFindElement(root, root, "IFameCmd", NULL, NULL, MXML_DESCEND);//强制关键帧
    mxml_node_t *home_position = mxmlFindElement(root, root, "HomePosition", NULL, NULL, MXML_DESCEND);//看守位控制
    mxml_node_t *drag_zoom_out = mxmlFindElement(root, root, "DragZoomOut", NULL, NULL, MXML_DESCEND);//拉框放大
    mxml_node_t *drag_zoom_in = mxmlFindElement(root, root, "DragZoomIn", NULL, NULL, MXML_DESCEND);//拉框缩小
#ifdef GB28181_2022
    mxml_node_t *stream_number = mxmlFindElement(root, root, "StreamNumber", NULL, NULL, MXML_DESCEND);
    mxml_node_t *ptz_precise_ctrl= mxmlFindElement(root, root, "PTZPreciseCtrl", NULL, NULL, MXML_DESCEND);//云台精准控制
    mxml_node_t *format_sd_card= mxmlFindElement(root, root, "FormatSDCard", NULL, NULL, MXML_DESCEND);//存储卡格式化
    mxml_node_t *target_track= mxmlFindElement(root, root, "TargetTrack", NULL, NULL, MXML_DESCEND);//目标跟踪
    mxml_node_t *device_upgrade= mxmlFindElement(root, root, "DeviceUpgrade", NULL, NULL, MXML_DESCEND);//设备软件升级
#endif
    DBG("handle device control\n");

    /*  控制命令分为有应答和无应答命令，这些命令都是可选的，有概率并行
    *       无应答命令: 只返回 200 OK, 不需要返回执行结果(云台控制命令、远程启动命令、强制关键帧、拉框放大、拉框缩小命令、PTZ 精准控制、存储卡格式化、目标跟踪)
    *       有应答命令: 返回 200 OK 后，还需要发送一个 MESSAGE 上报执行结果,(录像控制、报警布防/撤防、报警复位、看守位控制、设备配置、软件升级、)
    *       注意：有应答命令回复 MESSAGE 属于新的事物，需要重新生成 CALL-ID 和 TAG
    */
    const char *response_str = "OK";
    std::shared_ptr<MessageDialog> ptr = std::make_shared<MessageDialog>();

    if(ptz_cmd) {  // 云台相关控制命令
        GB_DBG("PTZCmd\n");
        HandPtzCmd(ptz_cmd);
    }
    if(record_cmd) { // 录像控制命令
        GB_DBG("RecordCmd\n");
        HandRecordCmd(record_cmd);
        need_default_reply = true;
    }
    if(tele_boot) { // 远程重启控制命令
        GB_DBG("TeleBoot\n");
        SetStopOnce(); // 先注销
        secs_delay_reboot(15, __func__); // 延迟 15S 重启设备
    }
    if(guard_cmd) { // 报警布防/撤防命令
        GB_DBG("GuardCmd\n");
        HandGuardCmd(guard_cmd);
        need_default_reply = true;
    }
    if(alarm_cmd) { // 报警复位命令
        DBG("AlarmCmd\n");
        set_duty_status(ONDUTY);
        need_default_reply = true;
    }
    if(home_position) { // 看守位控制命令
        DBG("HomePosition\n");
        HandHomePosition(root, home_position);
        need_default_reply = true;
    }
    if(i_fame_cmd) { // 强制关键帧命令
        GB_DBG("IFameCmd\n");
        encode_immediate_iframe(CH_FS_ALL);
    }
    if(drag_zoom_out) {  // 拉框放大命令
        GB_DBG("DragZoomOut\n");  // 拉框是数字变倍使用的功能，这里先不响应
        need_default_reply = true;
    }
    if(drag_zoom_in) { // 拉框缩小命令
        GB_DBG("DragZoomIn\n");
        need_default_reply = true;
    }
#ifdef GB28181_2022
    if (stream_number) { //
        HandStreamNumber(stream_number);
        need_default_reply = true;
    }
    if (ptz_precise_ctrl) {  // 云台精准控制
        DBG("PTZPreciseCtrl\n");
        ;// 该命令是用于精准控制云台运动角度和变倍倍数，我们大部分设备不支持，暂时不实现
    }
    if (format_sd_card) {
        DBG("FormatSDCard\n");
        GbJcpSend("format -name /mnt -enable 1");
    }
    if (target_track) {
        DBG("TargetTrack\n");
        ;// 该功能手动模式，实现在屏幕上点一下，设备自动转云台到点击位置的功能，目前只有抢球有这个功能，这里先不实现
    }
    if (device_upgrade) {
        DBG("DeviceUpgrade\n");
        HandDeviceUpgrade(root, ptr);
        need_default_reply = true;// ota 功能，目前没有平台测试，先不实现
    }
#endif

    if (need_default_reply) { // 需要发送一个 xml response
        mxml_node_t *reply_xml = NULL;

        reply_xml = GenerateXmlResponse(root, response_str);
        if (reply_xml != NULL) {

            MessageRequestAddXmlAndSend(reply_xml, ptr);
            mxmlDelete(reply_xml);
        }
    }

    return ;
}
/*
*  生成 message 请求报文添加 xml 回复消息体，然后发送
*
* @param[reply_xml] xml 报文节点
* @param[ptr] 对话信息记录结构体
*
* @return
*/
void GbClientManager::MessageRequestAddXmlAndSend(mxml_node_t *reply_xml, std::shared_ptr<MessageDialog> &ptr)
{
    if (reply_xml == NULL)
        return;

    osip_message_t *request = GenerateRequest("MESSAGE", message_cseq_++, profile_.get_sip_user_name());
    if (request == NULL)
        return;

    char buf[MAX_XML_LEN] = {0};
    int xml_len = 0;

    xml_len = mxmlSaveString(reply_xml, buf, sizeof(buf), whitespace_cb);
    do {
        if (osip_message_set_content_type(request, "application/MANSCDP+xml") != OSIP_SUCCESS) {
            break;
        }

        if (osip_message_set_body(request, buf, (size_t)xml_len)) {
            break;
        }

        if (SipToStringAndSend(request) != SUCCESS) {
            GB_ERR("sip to string and send error\n");
            break;
        }
        /*添加对话管理*/
        if (ptr == NULL || ptr->SetDialogInfo(request) != SUCCESS) {
            break;
        }
        dialog_mng_.AddDialog(ptr);
    } while(0);

    if (request) {
        osip_message_free(request);
    }

    return;
}

mxml_node_t *GbClientManager::HandCatalog(mxml_node_t* root)
{
    mxml_node_t *reply_xml = GenerateXmlResponse(root, "OK");
    if (reply_xml == NULL)
        return NULL;

    mxml_node_t *response_xml = mxmlFindElement(reply_xml, reply_xml, "Response", NULL, NULL, MXML_DESCEND);

    mxml_node_t *sum_num = mxmlNewElement(response_xml, "SumNum"); // 消息总数，设备只有一路编码通道这里应该为 1
    mxmlNewTextf(sum_num, 0, "%s", "1");

    mxml_node_t *dev_list = mxmlNewElement(response_xml, "DeviceList");
    mxmlElementSetAttr(dev_list, "Num", "1");

    mxml_node_t *item = mxmlNewElement(dev_list, "Item");

    mxml_node_t *dev_id = mxmlNewElement(item, "DeviceID");
    mxmlNewTextf(dev_id, 0, "%s", profile_.get_video_channal_id());

    mxml_node_t *name_xml = mxmlNewElement(item, "Name"); // 设备/区域/系统名称
    mxmlNewTextf(name_xml, 0, "%s", profile_.get_dev_name());

    mxml_node_t *manufacturer = mxmlNewElement(item, "Manufacturer"); // 设备厂商
    mxmlNewTextf(manufacturer, 0, "%s", profile_.get_manufacture());

    mxml_node_t *model = mxmlNewElement(item, "Model"); // 设备型号
    mxmlNewTextf(model, 0, "%s", profile_.get_dev_model());

    mxml_node_t *owner = mxmlNewElement(item, "Owner");  // 设备归属
    mxmlNewTextf(owner, 0, "%s", profile_.get_owner());

    mxml_node_t *civil_code = mxmlNewElement(item, "CivilCode"); // 行政区域
    mxmlNewTextf(civil_code, 0, "%s", profile_.get_civil_code());

    mxml_node_t *address = mxmlNewElement(item, "Address"); // 安装地址
    mxmlNewTextf(address, 0, "%s", profile_.get_address_info());

    mxml_node_t *parental = mxmlNewElement(item, "Parental"); // 是否有子设备(必选)1有,0没有
    mxmlNewTextf(parental, 0, "%s", "0");

    mxml_node_t *parent_id = mxmlNewElement(item, "ParentID");// 父设备ID
    mxmlNewTextf(parent_id, 0, "%s", profile_.get_sip_server_id());

    mxml_node_t *safety_way = mxmlNewElement(item, "SafetyWay"); // 信令安全模式 0:不采用
    mxmlNewTextf(safety_way, 0, "%s", "0");

    mxml_node_t *register_way = mxmlNewElement(item, "RegisterWay"); // 注册方式(必选)缺省为1; 1:符合IETFRFC3261标准的认证注册模式;
    mxmlNewTextf(register_way, 0, "%s", "1");

    mxml_node_t *secrecy = mxmlNewElement(item, "Secrecy"); // 保密属性(必选)缺省为0;0:不涉密,1:涉密
    mxmlNewTextf(secrecy, 0, "%s", "0");

    mxml_node_t *status = mxmlNewElement(item, "Status"); // 设备状态
    mxmlNewTextf(status, 0, "%s", "ON");

    // 这里应该还有报警输出设备和音频输出设备，目前我们不支持设置

    return reply_xml;
}

mxml_node_t *GbClientManager::HandDeviceInfo(mxml_node_t* root)
{
    mxml_node_t *reply_xml = GenerateXmlResponse(root, "OK");
    if (reply_xml == NULL)
        return NULL;
    mxml_node_t *response_xml = mxmlFindElement(reply_xml, reply_xml, "Response", NULL, NULL, MXML_DESCEND);

    mxml_node_t *devname = mxmlNewElement(response_xml, "DeviceName"); // 设备名称
    mxml_node_t *devcompany = mxmlNewElement(response_xml, "Manufacturer");// 设备生产厂商
    mxml_node_t *model = mxmlNewElement(response_xml, "Model");  // 设备型号
    mxml_node_t *firmware = mxmlNewElement(response_xml, "Firmware");  // 固件版本
    mxml_node_t *channel = mxmlNewElement(response_xml, "Channel"); // 设备输入通道数
    mxml_node_t *maxalarm = mxmlNewElement(response_xml, "MaxAlarm"); // 协议没有规定，属于扩展添加

    mxmlNewTextf(devname, 0, "%s", profile_.get_dev_name());
    mxmlNewTextf(devcompany, 0, "%s", profile_.get_manufacture());
    mxmlNewTextf(model, 0, "%s", profile_.get_dev_model());
    mxmlNewTextf(firmware, 0, "%s", get_fw_ver()); // 这里使用 OTA 版本号作为设备的固件版本
    mxmlNewTextf(channel, 0, "%d", 1);
    mxmlNewTextf(maxalarm, 0, "%d", 1);

    return reply_xml;
}

void GbClientManager::TimenowToServerFormat(char *buf, size_t buf_size)
{
    time_t timenow;
    struct tm tml;
    struct tm *p = &tml;

    /* AlarmTime 2009-12-04T16:23:32 */
    time(&timenow);
    //timenow += 3600*8;
    localtime_r(&timenow, &tml);

    snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
        p->tm_hour, p->tm_min, p->tm_sec);

    return ;
}

mxml_node_t *GbClientManager::HandDeviceStatus(mxml_node_t* root)
{
    mxml_node_t *reply_xml = GenerateXmlResponse(root, "OK");
    if (reply_xml == NULL)
        return NULL;
    mxml_node_t *response_xml = mxmlFindElement(reply_xml, reply_xml, "Response", NULL, NULL, MXML_DESCEND);

    mxml_node_t *online = mxmlNewElement(response_xml, "Online"); // 设备是否在线
    mxml_node_t *status = mxmlNewElement(response_xml, "Status"); // 是否正常工作
    mxml_node_t *encode = mxmlNewElement(response_xml, "Encode"); // 是否编码
    mxml_node_t *record = mxmlNewElement(response_xml, "Record"); // 是否录像
    mxml_node_t *time = mxmlNewElement(response_xml, "DeviceTime"); // 设备时间和日期

    mxmlNewTextf(online, 0, "%s", "ONLINE"); //OFFLINE
    mxmlNewTextf(status, 0, "%s", "OK");
    mxmlNewTextf(encode, 0, "%s", "ON");
    if(record_get_currec_status() != RECORD_STATUS_STOP)//RECORD_STATUS_E
        mxmlNewTextf(record, 0, "%s", "ON");
    else
        mxmlNewTextf(record, 0, "%s", "OFF");

    char timeBuf[32] = {0};
    TimenowToServerFormat(timeBuf, sizeof(timeBuf));
    mxmlNewTextf(time, 0, "%s", timeBuf);

    //Alarmstatus
    mxml_node_t *alarm_status = mxmlNewElement(response_xml,"Alarmstatus"); // 报警设备状态列表
    mxmlElementSetAttr(alarm_status, "Num", "1");
    mxml_node_t *item = mxmlNewElement(alarm_status, "Item");
    mxml_node_t *alarm_devid = mxmlNewElement(item, "DeviceID"); // 报警设备编码
    mxml_node_t *duty_status = mxmlNewElement(item, "DutyStatus"); // 报警设备状态

    mxmlNewTextf(alarm_devid, 0, "%s", profile_.get_alarmin_channal_id());
    mxmlNewTextf(duty_status, 0, "%s", GetDutyText()); //OFFDUTY  ALARM  ONDUTY

    return reply_xml;
}

/*把 UTC 时间转换成中国本地时间 格式: 2021-10-19T14:49:34*/
const char *Utf8ToGbkTime(time_t raw_time, char *buff)
{
    raw_time += 8 * 3600; // 加上 8 小时的偏移量
    struct tm* gbk_tm = gmtime(&raw_time);

    strftime(buff, 20, "%Y-%m-%dT%H:%M:%S", gbk_tm); // 格式化时间字符串

    return buff;
}

/*
* 生成录像查询回复 xml 报文，
*
* @param[sn] xml 会话的 sn 号
* @param[is_first] 是否是第一次生成，如果是该次查询的第一次报文，没有录像也需要回复。后续继承查找就不需要了
*
* @return 成功返回 xml 节点，失败或者查询结束返回 NULL
*/
mxml_node_t *GbClientManager::GenerateRecordXmlResponse(uint32_t sn, bool is_first)
{
    /*首先根据 sn 号查找是否存在查询任务*/
    int pos = record_mng_->FindQureyBySn(sn);
    if (pos < 0) {
        GB_ERR("not find qurey\n");
        return NULL;
    }

    mxml_node_t *reply_xml = GenerateXmlResponse(sn, "RecordInfo");
    if (reply_xml == NULL)
        return NULL;

    mxml_node_t *response_xml = mxmlFindElement(reply_xml, reply_xml, "Response", NULL, NULL, MXML_DESCEND);

    mxml_node_t *name_xml = mxmlNewElement(response_xml, "Name");
    mxml_node_t *sum_num_xml = mxmlNewElement(response_xml, "SumNum");
    mxmlNewTextf(name_xml, 0, "%s", profile_.get_dev_name());
    mxmlNewTextf(sum_num_xml, 0, "%u", record_mng_->GetQureyTotal(pos));

    mxml_node_t *record_list_xml = mxmlNewElement(response_xml, "RecordList");

    int add_count = 0;
    for (; add_count < RECORD_NUM; add_count++) {
        sRec1File *record_info = record_mng_->ResumeGetRcordInfo(pos);
        if (record_info == NULL) {  // 返回空代表取完了
            break;
        }

        const char *record_type = "time";

        mxml_node_t *item_xml = mxmlNewElement(record_list_xml, "Item");
        mxml_node_t *device_xml = mxmlNewElement(item_xml, "DeviceID");
        mxml_node_t *name_xml = mxmlNewElement(item_xml, "Name");
        mxml_node_t *file_path_xml = mxmlNewElement(item_xml, "FilePath");
        mxml_node_t *addr_xml = mxmlNewElement(item_xml, "Address");
        mxml_node_t *start_time_xml = mxmlNewElement(item_xml, "StartTime");
        mxml_node_t *end_time_xml = mxmlNewElement(item_xml, "EndTime");
        mxml_node_t *secrecy_xml = mxmlNewElement(item_xml, "Secrecy");
        mxml_node_t *type_xml = mxmlNewElement(item_xml, "Type");
        // 正常返回的 file_size 是录像长度，这里先屏蔽 filesize 上报
        //mxml_node_t *file_size_xml = mxmlNewElement(item_xml, "FileSize");

        mxmlNewTextf(device_xml, 0, "%s", profile_.get_sip_user_name());
        mxmlNewTextf(name_xml, 0, "%s", profile_.get_dev_name());
        mxmlNewTextf(file_path_xml, 0, "%s", record_info->file_name);
        mxmlNewTextf(addr_xml, 0, "%s", profile_.get_address_info());
        char time_str[30] = {0};
        mxmlNewTextf(start_time_xml, 0, "%s", Utf8ToGbkTime(record_info->start_time, time_str));
        mxmlNewTextf(end_time_xml, 0, "%s", Utf8ToGbkTime(record_info->stop_time, time_str));
        mxmlNewTextf(secrecy_xml, 0, "%d", 0);
        mxmlNewTextf(type_xml, 0, "%s", record_type);
        //mxmlNewTextf(file_size_xml, 0, "%d", record_info->file_size);
    }

    mxmlElementSetAttrf(record_list_xml, "Num", "%d", add_count);

    if (add_count == 0 && !is_first) {
        // 第一次没有录像也要返回数据告诉服务器
        // 不是第一次就不需要上报数据给服务器
        mxmlDelete(reply_xml);
        reply_xml = NULL;
    }

    return reply_xml;
}

time_t GbClientManager::TextTimeToUtc(const char *text)
{
    if (text == NULL) {
        GB_ERR("text is error\n");
        return 0;
    }

    GB_DBG("time text:%s\n", text);

    struct tm tm_time = {0};
    if (strptime(text, "%Y-%m-%dT%H:%M:%S", &tm_time) == NULL) {
        GB_ERR("Failed to parse time string.\n");
        return 0;
    }

    time_t utc_time = mktime(&tm_time) - 28800; // 写死东八区

    GB_DBG("utc time:%lld\n", utc_time);

    return utc_time;
}

mxml_node_t *GbClientManager::HandRecordInfo(mxml_node_t* root, uint32_t *user_data)
{
    mxml_node_t *startt_xml = mxmlFindElement(root, root, "StartTime", NULL, NULL, MXML_DESCEND);
    mxml_node_t *endt_xml = mxmlFindElement(root, root, "EndTime", NULL, NULL, MXML_DESCEND);
    mxml_node_t *devid_xml = mxmlFindElement(root, root, "DeviceID", NULL, NULL, MXML_DESCEND);
    mxml_node_t *sn_xml = mxmlFindElement(root, root, "SN", NULL, NULL, MXML_DESCEND);
    mxml_node_t *cmd_type_xml = mxmlFindElement(root, root, "CmdType", NULL, NULL, MXML_DESCEND);

    if (startt_xml == NULL || endt_xml == NULL || devid_xml == NULL || sn_xml == NULL || cmd_type_xml == NULL) {
        GB_ERR("Information is incomplete!\n");
        return NULL;
    }

    int whitespace = 0;
    const char *text = NULL;
    time_t start_time = 0, end_time = 0;
    uint32_t sn = 0;

    // 开始时间
    text = mxmlGetText(startt_xml, &whitespace);
    if (text == NULL)
        return NULL;
    start_time = TextTimeToUtc(text + whitespace);

    // 结束时间
    text = mxmlGetText(endt_xml, &whitespace);
    if (text == NULL)
        return NULL;
    end_time = TextTimeToUtc(text + whitespace);

    // SN
    text = mxmlGetText(sn_xml, &whitespace);
    if (text == NULL)
        return NULL;
    sn = atoi(text);

    *user_data = sn;

    record_mng_->AddRecordQuery(sn, start_time, end_time);  // 添加查询任务

    return GenerateRecordXmlResponse(sn, true);  // 生成回复报文，并返回
}

/*返回临时字符串，建议直接使用 std::string str = ChangeEncodingTo(...) 来构造初始化*/
/*src_str 的长度不能超过buf 的一半*/
std::string GbClientManager::ChangeEncoding(const char *src_str, GbClientManager::EncodingType src_enc, GbClientManager::EncodingType dst_enc)
{
    unsigned char out_buf[2048] = {0};
    int ret = 0;

    if (src_str == NULL || (strlen(src_str) > (sizeof(out_buf) - 1) / 2)) {
        GB_ERR("src_str:%p strlen:%d\n", src_str, strlen(src_str));
        return src_str;
    }

    if (src_enc == UTF_8) {
        if (dst_enc == GB2312) {
            ret = utf82gbk(reinterpret_cast<unsigned char*>(const_cast<char*>(src_str)), out_buf, sizeof(out_buf) - 1);
            if (ret < 0) {
                GB_ERR("utf82gbk failed!\n");
            } else {
                return reinterpret_cast<const char*>(out_buf);
            }
        }
    } else if (src_enc == GB2312) {
        if (dst_enc == UTF_8) {
            ret = gbk2utf8(reinterpret_cast<unsigned char*>(const_cast<char*>(src_str)), strlen(src_str), out_buf, sizeof(out_buf) - 1);
            if (ret < 0) {
                GB_ERR("gbk2utf8 failed!\n");
            } else {
                return reinterpret_cast<const char*>(out_buf);
            }
        }
    }

    // 不支持的转换，或者转换失败直接原字符返回
    return src_str;
}

int GbClientManager::RecordWeekDayPlan::AddTimeSegment(int start_hour, int start_min, int start_sec, int stop_hour, int stop_min, int stop_sec)
{
    if (num >= 8 ||
        start_hour < 0  || start_hour > 23 ||
        start_min < 0 || start_min > 59 ||
        start_sec < 0 || start_sec > 59 ||
        stop_hour < 0 || stop_hour > 23 ||
        stop_min < 0 || stop_min > 59 ||
        stop_sec < 0 || stop_sec > 59 ||
        start_hour > stop_hour ||
        (start_hour == stop_hour && start_min > stop_min)) {

        GB_ERR("add time segment error\n");
        return num;
    }

    time_segment[num].start_hour = start_hour;
    time_segment[num].start_min  = start_min;
    time_segment[num].start_sec  = start_sec;
    time_segment[num].stop_hour  = stop_hour;
    time_segment[num].stop_min   = stop_min;
    time_segment[num].stop_sec   = stop_sec;

    return ++num;
}

int GbClientManager::RecordWeeklyPlan::JcoToGbRecordPlan(unsigned int *timestrategy)
{
    if (timestrategy == NULL) {
        GB_ERR("param error\n");
        return FAILURE;
    }

    for (int i = 0; i < 7; ++i) {
        // 判断当天有没有计划
        if (timestrategy[i] == 0) {
            continue;
        }

        int start_hour = -1;
        int start_min  = 0;
        int start_sec  = 0;
        int stop_hour  = 0;
        int stop_min   = 59;
        int stop_sec   = 59;
        // 一天 24 小时, jco 录像时间段只精确到小时
        for (int j = 0; j < 24; ++j) {
            if (timestrategy[i] & (1 << j)) {
                // 当前小时有计划，未初始化开始时间就初始化开始和结束时间，已初始化就更新结束时间
                if (start_hour == -1) {
                    start_hour = stop_hour = j;
                } else {
                    stop_hour = j;
                }
            } else {
                // 当前小时无计划，已初始化开始时间就记录一条
                if (start_hour != -1) {
                    week_day[i].AddTimeSegment(start_hour, start_min, start_sec, stop_hour, stop_min, stop_sec);
                    start_hour = -1;
                }
            }
        }
        // 处理最后一个小时有计划的情况
        if (start_hour != -1) {
            week_day[i].AddTimeSegment(start_hour, start_min, start_sec, stop_hour, stop_min, stop_sec);
        }

        ++vaild_count; // 有效天数加一
    }

    return SUCCESS;
}

int GbClientManager::RecordWeeklyPlan::GbRecordPlanToJco(unsigned int *timestrategy)
{
    if (timestrategy == NULL) {
        GB_ERR("param error\n");
        return FAILURE;
    }

    for (int i = 0; i < 7; ++i) {
        // 判断当天有没有计划
        if (week_day[i].num == 0) {
            timestrategy[i] = 0;
            continue;
        }

        // 处理时间段
        for (int j = 0; j < week_day[i].num; ++j) {
            // jco 录像计划只要求精确到小时，只要当前小时有计划，无论分钟数都视为录满一个小时
            for (int k = week_day[i].time_segment[j].start_hour; k <= week_day[i].time_segment[j].stop_hour; ++k) {
                timestrategy[i] |= 1 << k;
                timestrategy[i] |= 1 << 31; // 最高位置 1 表示当天有计划
                GB_INFO("k:%d timestrategy:%u\n", k, timestrategy[i]);
            }
        }
    }

    return SUCCESS;
}

void GbClientManager::HandleQuryVideoRecordPlan(mxml_node_t* response)
{
    RecordCtrlS record_ctrl = {0};
    conf_get_recordcfg(&record_ctrl);

    GbClientManager::RecordWeeklyPlan weekly_plan;
    weekly_plan.JcoToGbRecordPlan(record_ctrl.timestrategy);

    mxml_node_t *video_record_plan_xml = mxmlNewElement(response, "VideoRecordPlan");
    mxml_node_t *record_enable_xml = mxmlNewElement(video_record_plan_xml, "RecordEnable");
    mxml_node_t *record_schedule_sun_num_xml = mxmlNewElement(video_record_plan_xml, "RecordScheduleSumNum");

    // 计划录像是否启用
    if (weekly_plan.vaild_count > 0) {
        mxmlNewTextf(record_enable_xml, 0, "1");
        mxmlNewTextf(record_schedule_sun_num_xml, 0, "%d", weekly_plan.vaild_count);

        for (int i = 0; i < 7; ++i) {
            if (weekly_plan.week_day[i].num == 0) {
                // 当天无计划，跳过
                continue;
            }

            mxml_node_t *record_schedule_xml = mxmlNewElement(video_record_plan_xml, "RecordSchedule");
            mxml_node_t *week_day_num_xml = mxmlNewElement(record_schedule_xml, "WeekDayNum");
            mxmlNewTextf(week_day_num_xml, 0, "%d", i + 1); // 周几取值范围 1~7

            // 时间总段数
            mxml_node_t *time_segment_sum_num_xml = mxmlNewElement(record_schedule_xml, "TimeSegmentSumNum");
            mxmlNewTextf(time_segment_sum_num_xml, 0, "%d", weekly_plan.week_day[i].num);

            for (int j = 0; j < weekly_plan.week_day[i].num; ++j) {
                mxml_node_t *time_segment_xml = mxmlNewElement(record_schedule_xml, "TimeSegment");

                mxml_node_t *start_hour_xml = mxmlNewElement(time_segment_xml, "StartHour");
                mxmlNewTextf(start_hour_xml, 0, "%d", weekly_plan.week_day[i].time_segment[j].start_hour);
                mxml_node_t *start_min_xml = mxmlNewElement(time_segment_xml, "StartMin");
                mxmlNewTextf(start_min_xml, 0, "%d", weekly_plan.week_day[i].time_segment[j].start_min);
                mxml_node_t *start_sec_xml = mxmlNewElement(time_segment_xml, "StartSec");
                mxmlNewTextf(start_sec_xml, 0, "%d", weekly_plan.week_day[i].time_segment[j].start_sec);
                mxml_node_t *stop_hour_xml = mxmlNewElement(time_segment_xml, "StopHour");
                mxmlNewTextf(stop_hour_xml, 0, "%d", weekly_plan.week_day[i].time_segment[j].stop_hour);
                mxml_node_t *stop_min_xml = mxmlNewElement(time_segment_xml, "StopMin");
                mxmlNewTextf(stop_min_xml, 0, "%d", weekly_plan.week_day[i].time_segment[j].stop_min);
                mxml_node_t *stop_sec_xml = mxmlNewElement(time_segment_xml, "StopSec");
                mxmlNewTextf(stop_sec_xml, 0, "%d", weekly_plan.week_day[i].time_segment[j].stop_sec);
            }
        }
    } else {
        mxmlNewTextf(record_enable_xml, 0, "0");
        mxmlNewTextf(record_schedule_sun_num_xml, 0, "0");
    }

    // 录制码流类型
    mxml_node_t *stream_number_xml = mxmlNewElement(video_record_plan_xml, "StreamNumber");
    if (record_ctrl.rec_type == 1) {
        // 子码流尺寸匹配
        mxmlNewTextf(stream_number_xml, 0, "1");
    } else {
        // 默认录制主码流
        mxmlNewTextf(stream_number_xml, 0, "0");
    }

    return ;
}

void GbClientManager::HandleQuryOSDConfig(mxml_node_t* response)
{
    OsdInfoS osd_info = {0};
    conf_get_osdinfocfg(&osd_info);

    mxml_node_t *osd_config_xml = mxmlNewElement(response, "OSDConfig");
    // 配置参考坐标系宽高，目前我们是以 1080P 作为标准坐标系
    mxml_node_t *length_xml = mxmlNewElement(osd_config_xml, "Length");
    mxmlNewTextf(length_xml, 0, "1920");
    mxml_node_t *width_xml = mxmlNewElement(osd_config_xml, "Width");
    mxmlNewTextf(width_xml, 0, "1080");
    // 时间 osd 信息
    mxml_node_t *time_x_xml = mxmlNewElement(osd_config_xml, "TimeX");
    mxmlNewTextf(time_x_xml, 0, "%d", osd_info.timeleft);
    mxml_node_t *time_y_xml = mxmlNewElement(osd_config_xml, "TimeY");
    mxmlNewTextf(time_y_xml, 0, "%d", osd_info.timetop);
    mxml_node_t *time_enable_xml = mxmlNewElement(osd_config_xml, "TimeEnable");
    mxmlNewTextf(time_enable_xml, 0, "%d", osd_info.timeen);
    // 时间 type 0-年月日使用符号'-'连接  1-年月日使用中文
    mxml_node_t *time_type_xml = mxmlNewElement(osd_config_xml, "TimeType");
#ifndef _OSDCODE_
    // 不定义宏的时候，没有中文，写死 0
    mxmlNewTextf(time_type_xml, 0, "0");
#else
    // 定义宏时，0、1、6 是带中文的，其它使用连字符
    mxmlNewTextf(time_type_xml, 0, "%d", osd_info.dateformat == 0 || osd_info.dateformat == 1 || osd_info.dateformat == 6);
#endif
    mxml_node_t *text_enable_xml = mxmlNewElement(osd_config_xml, "TextEnable");
    mxml_node_t *sum_num_xml = mxmlNewElement(osd_config_xml, "SumNum");

    int enable = 0;
    int sum_num = 0;

    // 扩展文字最多 8 行，前一二行使用名称和字母，后面需要再填扩展字幕
    if (osd_info.nameen) {
        mxml_node_t *item_xml = mxmlNewElement(osd_config_xml, "Item");
        mxml_node_t *text_xml = mxmlNewElement(item_xml, "Text");
        // 国标规定长度最长 32，我们的 OSD 是 UTF8 编码，转换成 xml 使用的编码
        mxmlNewTextf(text_xml, 0, "%.*s", 32, ChangeEncoding(osd_info.name, GbClientManager::EncodingType::UTF_8, xml_encoding_type_).c_str());
        mxml_node_t *x_xml = mxmlNewElement(item_xml, "X");
        mxmlNewTextf(x_xml, 0, "%d", osd_info.nameleft);
        mxml_node_t *y_xml = mxmlNewElement(item_xml, "Y");
        mxmlNewTextf(y_xml, 0, "%d", osd_info.nametop);
        enable = 1;
        ++sum_num;
    }

    OsdExpandS osd_expand = {0};
    conf_get_osdexpandcfg(&osd_expand);
    for (int i = 0; i < 1; ++i) { // 这里只使用第一个名称的位置，后续需要增加再扩展
        if (osd_expand.cusosd[i].enable) {
            mxml_node_t *item_xml = mxmlNewElement(osd_config_xml, "Item");
            mxml_node_t *text_xml = mxmlNewElement(item_xml, "Text");
            mxmlNewTextf(text_xml, 0, "%.*s", 32, ChangeEncoding(osd_expand.cusosd[i].content, GbClientManager::EncodingType::UTF_8, xml_encoding_type_).c_str());
            mxml_node_t *x_xml = mxmlNewElement(item_xml, "X");
            mxmlNewTextf(x_xml, 0, "%d", osd_expand.cusosd[i].x);
            mxml_node_t *y_xml = mxmlNewElement(item_xml, "Y");
            mxmlNewTextf(y_xml, 0, "%d", osd_expand.cusosd[i].y);
            enable = 1;
            ++sum_num;
        }
    }

    mxmlNewTextf(text_enable_xml, 0, "%d", enable);
    mxmlNewTextf(sum_num_xml, 0, "%d", sum_num);

    return ;
}

mxml_node_t *GbClientManager::HandQueryDownload(mxml_node_t* root)
{
    mxml_node_t *reply_xml = NULL;

    mxml_node_t *cmd_type_xml = mxmlFindElement(root, root, "CmdType", NULL, NULL, MXML_DESCEND);
    mxml_node_t *sn_xml = mxmlFindElement(root, root, "SN", NULL, NULL, MXML_DESCEND);
    mxml_node_t *config_type_xml = mxmlFindElement(root, root, "ConfigType", NULL, NULL, MXML_DESCEND);

    /*
        ConfigType: 查询配置参数类型,可查询的配置类型包括基本参数配置:BasicParam,视频参数范围:VideoParamOpt,SVAC
                    编码配置:SVACEncodeConfig,SVAC 解码配置:SVACDecodeConfig。可同时查询多个配置类型,各类型以“/”分隔,
                    可返回与查询SN 值相同的多个响应,每个响应对应一个配置类型。-
    */
    if (cmd_type_xml == NULL || sn_xml == NULL || config_type_xml == NULL)
        return NULL;

    const char *config_type = mxmlGetText(config_type_xml, NULL);
    if (config_type == NULL) {
        GB_ERR("not find config_type\n");
        return NULL;
    }

    GB_DBG("config type:%s\n", config_type);

    reply_xml = GenerateXmlResponse(root, "OK");
    if (reply_xml == NULL)
        return NULL;
    mxml_node_t *response_xml = mxmlFindElement(reply_xml, reply_xml, "Response", NULL, NULL, MXML_DESCEND);

    if (strstr(config_type, "BasicParam")) { // 基本参数配置
        GB_DBG("*******************BasicParam!\n");
        mxml_node_t *basic_param_xml = mxmlNewElement(response_xml, "BasicParam");

        mxml_node_t *name_xml = mxmlNewElement(basic_param_xml, "Name"); // 设备名称
        mxmlNewTextf(name_xml, 0, "%s", profile_.get_dev_name());
        mxml_node_t *expiration_xml = mxmlNewElement(basic_param_xml, "Expiration"); // 注册过期时间
        mxmlNewTextf(expiration_xml, 0, "%d", profile_.get_term_of_register());
        mxml_node_t *heart_beat_interval_xml = mxmlNewElement(basic_param_xml, "HeartBeatInterval"); // 心跳间隔时间
        mxmlNewTextf(heart_beat_interval_xml, 0, "%d", profile_.get_heartbeat_interval());
        mxml_node_t *heart_beat_count_xml = mxmlNewElement(basic_param_xml, "HeartBeatCount"); // 心跳超时次数
        mxmlNewTextf(heart_beat_count_xml, 0, "%d", profile_.get_heartbeat_timeout_count());
        mxml_node_t *position_capability_xml = mxmlNewElement(basic_param_xml, "PositionCapability"); // 定位功能支持情况。取值:0-不支持;1-支持 GPS定位;2-支持北斗定位
        mxmlNewTextf(position_capability_xml, 0, "%d", 0);
        /*mxml_node_t *longitude_xml = mxmlNewElement(basic_param_xml, "Longitude"); // 经度
        mxmlNewTextf(longitude_xml, 0, "%f", profile_.get_longitude());
        mxml_node_t *latitude_xml = mxmlNewElement(basic_param_xml, "Latitude"); // 纬度
        mxmlNewTextf(latitude_xml, 0, "%f", profile_.get_latitude());*/
    }

    if (strstr(config_type, "VideoParamOpt")) { // 视频参数范围
        GB_DBG("*******************videoParamOpt!\n");

        mxml_node_t *video_param_opt_xml = mxmlNewElement(response_xml, "VideoParamOpt");

        mxml_node_t *download_speed_xml = mxmlNewElement(video_param_opt_xml, "DownloadSpeed");   //下载倍速范围(可选),各可选参数以“/”分隔,如设备支持1,2,4倍速下载则应写为“1/2/4
        mxmlNewTextf(download_speed_xml, 0, "%s", "1/2/4");

        mxml_node_t *Resolution = mxmlNewElement(video_param_opt_xml, "Resolution");     //摄像机支持的分辨率(可选),可有多个分辨率值,各个取值间以“/”分隔。分辨率取值参见附录 F中SDPf字段规定
#ifdef GB28181_2022
        // gb2022 扩展了分辨率范围，这里增加一些选项
        // 由于当前获取分辨率范围比较麻烦，这里直接写死给一个较大的范围
        mxmlNewTextf(Resolution, 0, "%s/%s/%s/%s/%s/%s", SdpParse::VencsizeToResolution(VencSizeE_4M),
                                                         SdpParse::VencsizeToResolution(VencSizeE_3M),
                                                         SdpParse::VencsizeToResolution(VencSizeE_1080P),
                                                         SdpParse::VencsizeToResolution(VencSizeE_720P),
                                                         SdpParse::VencsizeToResolution(VencSizeE_VGA),
                                                         SdpParse::VencsizeToResolution(VencSizeE_CIF));
#else
        mxmlNewTextf(Resolution, 0, "6/5");  // 1—QCIF; 2—CIF; 3—4CIF; 4—D1; 5—720P; 6—1080P/I
#endif
    }

    if (strstr(config_type, "SVACEncodeConfig")) { // SVAC 编码配置
        GB_DBG("*******************sVACEncodeConfig!\n");
        // 不支持，回复 200 ok 就行，目前海康也是这么做的
    }

    if (strstr(config_type, "SVACDecodeConfig")) { // SVAC解码配置
        DBG("*******************sVACDecodeConfig!\n");
        // 不支持 SVAC 编码，这里不处理
    }
#ifdef GB28181_2022
    if (strstr(config_type, "VideoParamAttribute")) { // 视频参数属性配置
        DBG("*******************VideoParamAttribute!\n");
        VideoEncS video_info = {0};
        conf_get_videocfg(&video_info);

        mxml_node_t *video_param_arribute_xml = mxmlNewElement(response_xml, "VideoParamAttribute");

        for (int i = 0; i < video_info.gnum; ++i) {
            mxml_node_t *item_xml = mxmlNewElement(video_param_arribute_xml, "Item");
            // 视频流编号，0-主码流 1-子码流，类推
            mxml_node_t *stream_number_xml = mxmlNewElement(item_xml, "StreamNumber");
            mxmlNewTextf(stream_number_xml, 0, "%d", i);
            // 视频编码格式，符合 SDP f 字段规定
            mxml_node_t *video_format_xml = mxmlNewElement(item_xml, "VideoFormat");
            mxmlNewTextf(video_format_xml, 0, "%d", SdpParse::CodecToVideoFormat(video_info.enc[i].codec));
            // 分辨率当前值
            mxml_node_t *resolution_xml = mxmlNewElement(item_xml, "Resolution");
            mxmlNewTextf(resolution_xml, 0, "%s", SdpParse::VencsizeToResolution(video_info.enc[i].vencsize));
            // 帧率当前配置
            mxml_node_t *frame_rate_xml = mxmlNewElement(item_xml, "FrameRate");
            mxmlNewTextf(frame_rate_xml, 0, "%d", video_info.enc[i].fps);
            // 码率类型配置值
            mxml_node_t *bit_rate_type_xml = mxmlNewElement(item_xml, "BitRateType");
            mxmlNewTextf(bit_rate_type_xml, 0, "%d", SdpParse::FixbpsToBitRateType(video_info.enc[i].fixbps));
            // 视频码率配置值
            mxml_node_t *video_bit_rate_xml = mxmlNewElement(item_xml, "VideoBitRate");
            mxmlNewTextf(video_bit_rate_xml, 0, "%d", video_info.enc[i].bps);
        }

        mxmlElementSetAttrf(video_param_arribute_xml, "Num", "%d", video_info.gnum);
    }

    if (strstr(config_type, "VideoRecordPlan")) { // 录像计划
        DBG("*******************VideoRecordPlan!\n");
        HandleQuryVideoRecordPlan(response_xml);
    }

    if (strstr(config_type, "VideoAlarmRecord")) { // 报警录像
        DBG("*******************VideoAlarmRecord!\n");
        RecordCtrlS record_ctrl = {0};
        conf_get_recordcfg(&record_ctrl);
        VideoEncS video_cfg = {0};
        conf_get_realvideocfg(&video_cfg);

        mxml_node_t *video_alarm_record_xml = mxmlNewElement(response_xml, "VideoAlarmRecord");
        mxml_node_t *record_enable_xml = mxmlNewElement(video_alarm_record_xml, "RecordEnable");
        mxmlNewTextf(record_enable_xml, 0, "1");  // 报警录像使能，目前我们的代码无法关闭
        mxml_node_t *pre_record_xml = mxmlNewElement(video_alarm_record_xml, "PreRecordTime");
        mxmlNewTextf(pre_record_xml, 0, "%d", record_ctrl.prerecordtime);  // 预录制时间
        mxml_node_t *stream_number_xml = mxmlNewElement(video_alarm_record_xml, "StreamNumber");
        if (record_ctrl.rec_type == 1) {
            // 子码流尺寸匹配
            mxmlNewTextf(stream_number_xml, 0, "1");
        } else {
            // 默认录制主码流
            mxmlNewTextf(stream_number_xml, 0, "0");
        }
    }

    // if (strstr(config_type, "PictrueMask")) { // 视频画面遮挡
    //     DBG("*******************PictrueMask!\n");
    //     VideoMaskS video_mask = {0};
    //     conf_get_videomaskcfg(&video_mask);

    //     mxml_node_t *pictrue_mask_xml = mxmlNewElement(response_xml, "PictrueMask");
    //     mxml_node_t *on_xml = mxmlNewElement(pictrue_mask_xml, "On");
    //     mxml_node_t *sum_num_xml = mxmlNewElement(pictrue_mask_xml, "SumNum");
    //     mxml_node_t *region_list_xml = mxmlNewElement(pictrue_mask_xml, "RegionList");

    //     int on = 0, sum_num = 0;
    //     for (int i = 0; i < 4 && i < video_mask.gnum; ++i) {  // 网页上只放开 4 个区域，这里同样
    //         if (video_mask.mask[i].enable) {
    //             on = 1;
    //             ++sum_num;
    //             mxml_node_t *item_xml = mxmlNewElement(region_list_xml, "Item");
    //             mxml_node_t *seq_xml = mxmlNewElement(item_xml, "Seq");
    //             mxmlNewTextf(seq_xml, 0, "%d", sum_num); // 编号 1~4
    //             mxml_node_t *point_xml = mxmlNewElement(item_xml, "Point");
    //             mxmlNewTextf(point_xml, 0, "%d,%d,%d,%d", video_mask.mask[i].x0, video_mask.mask[i].y0,
    //                                                        video_mask.mask[i].x1, video_mask.mask[i].y1); // 左上角，右下角坐标
    //         }
    //     }

    //     mxmlNewTextf(on_xml, 0, "%d", on);
    //     mxmlNewTextf(sum_num_xml, 0, "%d", sum_num);
    //     mxmlElementSetAttrf(region_list_xml, "Num", "%d", sum_num);
    // }

    if (strstr(config_type, "FrameMirror")) { // 画面翻转
        DBG("*******************FrameMirror!\n");
        ViInfoS video_info = {0};
        conf_get_viinfocfg(&video_info);

        // 翻转 0:正常 1:水平 2:上下 3:中心
        mxml_node_t *frame_mirror_xml = mxmlNewElement(response_xml, "FrameMirror");
        mxmlNewTextf(frame_mirror_xml, 0, "%d", video_info.reverse);
    }

    if (strstr(config_type, "AlarmReport")) { // 报警上报开关
        DBG("*******************AlarmReport!\n");
        // 目前文档只规定了移动侦测和区域入侵
        // 我们大部分不支持同时开多个算法，这里就把算法开关作为报警上报开关
        MotionDetectS motion_detect = {0};
        conf_get_motiondetectcfg(&motion_detect);
        VgrectS vg_rect = {0};
        conf_get_vgrectcfg(&vg_rect);

        mxml_node_t *alarm_report_xml = mxmlNewElement(response_xml, "AlarmReport");
        mxml_node_t *motion_detection_xml = mxmlNewElement(alarm_report_xml, "MotionDetection");
        mxmlNewTextf(motion_detection_xml, 0, "%d", motion_detect.enable);
        mxml_node_t *field_detection_xml = mxmlNewElement(alarm_report_xml, "FieldDetection");
        mxmlNewTextf(field_detection_xml, 0, "%d", vg_rect.enable);
    }

    if (strstr(config_type, "OSDConfig")) { // osd 设置
        DBG("*******************OSDConfig!\n");
        HandleQuryOSDConfig(response_xml);
    }

    if (strstr(config_type, "SnapShotConfig")) { // 图像抓拍配置
        DBG("*******************SnapShotConfig!\n");
        // 目前参数不会保存，图片抓拍查询暂时先不做
    }
#endif

    return reply_xml;
}

mxml_node_t* GbClientManager::HandPresetInfo(mxml_node_t* root)
{
    // 预置位查询先不支持，有需要再支持

    return NULL;
}

mxml_node_t *GbClientManager::HandQueryMobilePosition(mxml_node_t* root)
{
    // 不支持，回复 200 OK

    return NULL;
}

mxml_node_t *GbClientManager::HandQuerySDCardStatus(mxml_node_t* root)
{
    mxml_node_t *reply_xml = GenerateXmlResponse(root, NULL);
    if (reply_xml == NULL)
        return NULL;
    mxml_node_t *response_xml = mxmlFindElement(reply_xml, reply_xml, "Response", NULL, NULL, MXML_DESCEND);

    int sum_num = 0;

    char cur_dev_path[128] = {0};
    int ret = storage_get_mmcpath(cur_dev_path);
    if(ret == 0) {
        sum_num = 1;
    }

    // 存储卡总数
    mxml_node_t *sum_num_xml = mxmlNewElement(response_xml, "SumNum");
    mxmlNewTextf(sum_num_xml, 0, "%d", sum_num);

    // 储存卡列表
    mxml_node_t *sdcard_status_info_xml = mxmlNewElement(response_xml, "SDCardStatusInfo");
    mxmlElementSetAttrf(sdcard_status_info_xml, "Num", "%d", sum_num);

    for (int i = 0; i < sum_num; ++i) {
        // Item
        mxml_node_t *item_xml = mxmlNewElement(sdcard_status_info_xml, "Item");

        // sd 卡编号
        mxml_node_t *id_xml = mxmlNewElement(item_xml, "ID");
        mxmlNewTextf(id_xml, 0, "%d", i + 1); // 根据格式化那边对 ID 的规定，这里应该从 1 开始，0 代表所有 sd 卡

        // SD 卡名称
        mxml_node_t *hdd_name_xml = mxmlNewElement(item_xml, "HddName");
        mxmlNewTextf(hdd_name_xml, 0, "%s", cur_dev_path);
        // 状态，ok-正常，formatting-格式化，unformatted-未格式化，idle-空闲，error-错误
        mxml_node_t *status_xml = mxmlNewElement(item_xml, "Status");
        RecStatusE rec_status = record_get_currec_status();
        if (get_g_stat(record, SD_REC_FORMAT)) {
            // 格式化
            mxmlNewTextf(status_xml, 0, "formatting");
        } else if (get_g_stat(record, SD_ERR_UNAUTH)) {
            // 未格式化
            mxmlNewTextf(status_xml, 0, "unformatted");
        } else if (get_g_stat(record, SD_ERR_READ | SD_ERR_WRITE)) {
            // 读写错误
            mxmlNewTextf(status_xml, 0, "error");
        } else if (rec_status == RECORD_STATUS_STOP) {
            // 未录像，空闲
            mxmlNewTextf(status_xml, 0, "idle");
        } else {
            // 正常
            mxmlNewTextf(status_xml, 0, "ok");
        }

        struct statfs stfs;
        ret = statfs(cur_dev_path, &stfs);
        if(ret == 0) {
            long free = ((stfs.f_bfree >> 10) * stfs.f_bsize) >> 10; // m
            long total = ((stfs.f_blocks >> 10) * stfs.f_bsize) >> 10; // m

            // 存储容量 单位:MB
            mxml_node_t *capacity_xml = mxmlNewElement(item_xml, "Capacity");
            mxmlNewTextf(capacity_xml, 0, "%ld", total);

            // 剩余存储容量，单位:MB
            mxml_node_t *free_space_xml = mxmlNewElement(item_xml, "FreeSpace");
            mxmlNewTextf(free_space_xml, 0, "%ld", free);
        }
    }

    return reply_xml;
}

void GbClientManager::HandQueryMessage(mxml_node_t* root, const char *cmd_type)
{
    mxml_node_t *reply_xml = NULL;

    DBG("***cmd type = %s***\n", cmd_type);
    /*
    DeviceStatus:设备状态查询请求
    Catalo:设备目录信息查询请求
    DeviceInf:设备信息查询请求
    RecordInf:文件目录检索请求
    Alarm:报警查询
    ConfigDownloa:设备配置查询
    PresetQuer:设备预置位查询
    MobilePositio:移动设备位置数据查询
    */
    std::shared_ptr<MessageDialog> ptr = std::make_shared<MessageDialog>();
    /*查询消息都属于需要回复的*/
    if(!strcasecmp(cmd_type, "Catalog")) { // 设备目录信息查询，主要是查询设备信息，行政区域之类的
        reply_xml = HandCatalog(root);

    } else if(!strcasecmp(cmd_type, "DeviceInfo")) { // 设备信息查询
        reply_xml = HandDeviceInfo(root);

    } else if(!strcasecmp(cmd_type, "DeviceStatus")) { // 设备状态查询
        reply_xml = HandDeviceStatus(root);

    } else if(!strcasecmp(cmd_type, "RecordInfo")) { // 录像查询
        uint32_t sn = 0;
        reply_xml = HandRecordInfo(root, &sn);

        ptr->set_type(MessageDialog::Type::QUREY_RECORD);
        ptr->set_sn(sn);
        ptr->set_timeout(20);
    } else if(!strcasecmp(cmd_type, "Alarm")) { // 报警查询
        ;// 海康只回复一个 200ok
    } else if(!strcasecmp(cmd_type, "ConfigDownload")) { // 设备配置查询
        DBG("***ConfigDownload***\n");
        reply_xml = HandQueryDownload(root);
    } else if(!strcasecmp(cmd_type, "PresetQuery")) { // 设备预置位查询
        DBG("***PresetQuery***\n");
        reply_xml = HandPresetInfo(root);
    } else if(!strcasecmp(cmd_type, "MobilePosition")) { // 移动设备位置数据查询
        DBG("***MobilePosition***\n");
        reply_xml = HandQueryMobilePosition(root);
#ifdef GB28181_2022
    } else if(!strcasecmp(cmd_type, "SDCardStatus")) { // 存储卡状态查询
        DBG("***SDCardStatus***\n");
        reply_xml = HandQuerySDCardStatus(root);
#endif
    } else {
        DBG("*******cmd_type no match*****\n");
    }

    if (reply_xml != NULL) {
        MessageRequestAddXmlAndSend(reply_xml, ptr);
        mxmlDelete(reply_xml);
    }

    return ;
}

void GbClientManager::HandNotifyBroadcast(mxml_node_t* root)
{
    mxml_node_t *cmd_type_xml = mxmlFindElement(root, root, "CmdType", NULL, NULL, MXML_DESCEND);
    mxml_node_t *sn_xml = mxmlFindElement(root, root, "SN", NULL, NULL, MXML_DESCEND);
    mxml_node_t *source_id_xml = mxmlFindElement(root, root, "SourceID", NULL, NULL, MXML_DESCEND);
    mxml_node_t *target_id_xml = mxmlFindElement(root, root, "TargetID", NULL, NULL, MXML_DESCEND);
  //mxml_node_t *device_id_xml = mxmlFindElement(root, root, "DeviceID", NULL, NULL, MXML_DESCEND);

    if (cmd_type_xml == NULL || sn_xml == NULL || source_id_xml == NULL || target_id_xml == NULL) {
        GB_DBG("broadcast info error\n");
        return ;
    }

    // SN
    int whitespace = 0;
    const char *text = mxmlGetText(sn_xml, &whitespace);
    if (text == NULL)
        return ;
    uint32_t sn = atoi(text);

    // device id
    const char *device_id = NULL;
    text = mxmlGetText(target_id_xml, &whitespace);
    if (text == NULL) {
        GB_ERR("get device id error\n");
        return ;
    }
    device_id = text + whitespace;

    mxml_node_t *reply_xml = GenerateXml("Response", sn, "Broadcast", "OK", device_id);
    if (reply_xml == NULL)
        return ;

    std::shared_ptr<MessageDialog> ptr = std::make_shared<MessageDialog>(MessageDialog::Type::NOTIFY_BROADCAST);
    MessageRequestAddXmlAndSend(reply_xml, ptr);
    mxmlDelete(reply_xml);

    return ;
}

void GbClientManager::HandleMessageRquest(osip_message_t *request)
{
    mxml_node_t *root = NULL;
    int whitespace = 0;
    osip_body_t *body = NULL;

    do {
        osip_message_get_body(request, 0, &body);
        if(body == NULL || request->content_type == NULL ||
                request->content_type->type == NULL || request->content_type->subtype == NULL) {
            GB_ERR("request body error or content type error\n");
            break;
        }

        GB_DBG("contect type: %s/%s\n", request->content_type->type, request->content_type->subtype);
        if (strcasecmp(request->content_type->type, "application") != 0 ||
                strcasecmp(request->content_type->subtype, "MANSCDP+xml")) {
            GB_ERR("content type error\n");
            break;
        }

        // mxml 解析文本中如果包含 gb2312 编码的中文会导致失败，这里统一转码为 UTF_8
        if (strstr(body->body, "encoding=\"GB2312\"")) {
            GB_DBG("xml encoding: GB2312\n");

            std::string body_utf8 = ChangeEncoding(body->body, GbClientManager::EncodingType::GB2312,
                                                                GbClientManager::EncodingType::UTF_8);
            //GB_INFO("utf8 message:\n%s\n", body_utf8.c_str());
            root = mxmlLoadString(NULL, body_utf8.c_str(), MXML_TEXT_CALLBACK);
            if(root == NULL) {
                GB_ERR("mxmlLoadString error\n");
                break;
            }
        } else {
            // xml 中，缺省 encoding 表示使用 UTF_8 编码
            // 不匹配的编码，全部当成 UTF-8 处理
            GB_DBG("xml encoding: UTF-8\n");
            //GB_INFO("utf8 message:\n%s\n", body->body);
            root = mxmlLoadString(NULL, body->body, MXML_TEXT_CALLBACK);
            if(root == NULL) {
                GB_ERR("mxmlLoadString error\n");
                break;
            }
        }

        mxml_node_t *control = mxmlFindElement(root, root, "Control", NULL, NULL, MXML_DESCEND);
        mxml_node_t *query = mxmlFindElement(root, root, "Query", NULL, NULL, MXML_DESCEND);
        mxml_node_t *response_xml = mxmlFindElement(root, root, "Response", NULL, NULL, MXML_DESCEND);
        mxml_node_t *notify   = mxmlFindElement(root, root, "Notify", NULL, NULL, MXML_DESCEND);

        mxml_node_t *cmd_type_xml = mxmlFindElement(root, root, "CmdType", NULL, NULL, MXML_DESCEND);

        if (control && cmd_type_xml) {  // control 会有一个子节点 CmdType 指示命令类型，这个是必要的
            const char *cmd_type_text = NULL;
            cmd_type_text = mxmlGetText(cmd_type_xml, &whitespace);
            if(cmd_type_text != NULL) {
                if (0 == strcmp(cmd_type_text + whitespace, "DeviceControl")) {
                    HandDeviceControl(root);
                } else if (0 == strcmp(cmd_type_text, "DeviceConfig")) {
                    HandDevideConfig(root);
                }
            }
        } else if (query && cmd_type_xml) { // query 会有一个子节点 CmdType 指示命令类型，这个是必要的
            const char *cmd_type_text = NULL;

            cmd_type_text = mxmlGetText(cmd_type_xml, &whitespace);
            if (cmd_type_text != NULL) {
                HandQueryMessage(root, cmd_type_text + whitespace);
            }

        } else if (notify) {
            // 对于设备来讲，目前 message 的 notify 只有服务器通知设备要发起广播
            const char *cmd_type_text = NULL;

            cmd_type_text = mxmlGetText(cmd_type_xml, &whitespace);
            if(cmd_type_text != NULL && 0 == strcmp(cmd_type_text + whitespace, "Broadcast")) {
                HandNotifyBroadcast(notify);
            }
        } else if (response_xml) {
            ; // 报警命令会收到 xml response ，这个设备不需要处理，只要返回 200 OK 就行
        }
    } while(0);

    if (root != NULL) {
        mxmlDelete(root);
    }

    return ;
}
/*
* 生成并发送 200 ok 报文，可以携带消息体
*
* @param[message] 请求的 sip 报文，200 ok 需要的 from methond call_id 都从这里面解析
* @param[content_type] 默认值 NULL，消息体的类型
* @param[body] 默认值 NULL, 消息体的内容
* @param[body_len] 默认值 0 消息体的长度
* @param[need_contact] 是否需要 contact ，只有几类特殊的报文需要携带 默认值 false
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::Send200Ok(osip_message_t *message, const char *content_type, const char *body, uint32_t body_len, bool need_contact)
{
    int ret = 0;
    osip_message_t *response = NULL;
    do {
        osip_message_t *response = GenerateResponseForRequest(message, 200, need_contact);
        if (response == NULL) {
            ret = FAILURE;
            break;
        }

        if (content_type != NULL && body != NULL && body_len != 0) {
            if (osip_message_set_content_type(response, content_type) != OSIP_SUCCESS) {
                GB_ERR("add content type error\n");
                ret = FAILURE;
                break;
            }

            if (osip_message_set_body(response, body, (size_t)body_len) != OSIP_SUCCESS) {
                GB_ERR("add message body error\n");
                ret = FAILURE;
                break;
            }
        }

        ret = SipToStringAndSend(response);

    } while(0);

    if (response) {
        osip_message_free(response);
    }

    return ret;
}
/*
* 生成并发送 100 tring 报文
*
* @param[message] 请求的 sip 报文，100 tring 需要的 from methond call_id 都从这里面解析
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::Send100Tring(osip_message_t *message)
{
    int ret = FAILURE;

    osip_message_t *response = GenerateResponseForRequest(message, 100);
    if (response != NULL) {
        ret = SipToStringAndSend(response);

        osip_message_free(response);
    }

    return ret;
}

/*
* 生成并发送 ack 报文
*
* @param[message] 请求的 sip 报文 ack 需要的 from methond call_id 都从这里面解析
* @param[user] sip 设备编码
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SendAck(osip_message_t *message, const char *user)
{
    int ret = FAILURE;

    osip_message_t *ack = GenerateRequestForResponse(message, "ACK", user, true);
    if (ack != NULL) {
        ret = SipToStringAndSend(ack);

        osip_message_free(ack);
    }

    return ret;
}
/**
 * 处理录像回复 200 OK
 *
 * @param[response] 回复 sip 报文解析
 * @param[dialog] sip 会话信息
 *
 * @return 成功发送回复报文返回 SUCCESS，录像查询完成或者发送失败返回 FAILURE
 */
int GbClientManager::HandleRecordResponse(osip_message_t *response, std::shared_ptr<MessageDialog> &dialog)
{
    mxml_node_t *reply_xml = NULL;
    osip_message_t *request = NULL;
    int ret = FAILURE;

    do {
        reply_xml = GenerateRecordXmlResponse(dialog->get_sn(), false);
        if (reply_xml == NULL) { // 出错或者数据已经发完
            record_mng_->DeleteQureyBySn(dialog->get_sn());
            break;
        }

        // 录像剩余数据发送应该属于同一个会话
        request = GenerateRequestForResponse(response, "MESSAGE", profile_.get_sip_user_name());
        if (request == NULL) {
            break;
        }

        char buf[4096] = {0};
        int xml_len = 0;

        xml_len = mxmlSaveString(reply_xml, buf, sizeof(buf), whitespace_cb);

        if (osip_message_set_content_type(request, "application/MANSCDP+xml") != OSIP_SUCCESS) {
            GB_ERR("add content type error\n");
            break;
        }

        if (osip_message_set_body(request, buf, (size_t)xml_len) != OSIP_SUCCESS) {
            GB_ERR("add message body error\n");
            break;
        }

        ret = SipToStringAndSend(request);
        if (ret != SUCCESS)
            break;

        /*添加对话，等待后续处理*/
        dialog_mng_.AddDialog(dialog);
    } while(0);

    if (reply_xml) {
        mxmlDelete(reply_xml);
    }

    if (request) {
        osip_message_free(request);
    }

    return ret;
}

uint32_t GbClientManager::GenerateSsrc(bool is_live)
{
    // ssrc 为十位十进制整数字符串
    char ssrc[11] = {0};
    char *p = ssrc;
    // 第 0 位: 0 为实时 1 为历史
    if (!is_live) {
        p[0] = '1';
    } else {
        p[0] = '0';
    }
    p += 1;
    // 第 1~5 位: 取 sip 服务器 id 的 3~7
    const char *server_id = profile_.get_sip_server_id();
    server_id += 3;
    for (int i = 0; i < 5; ++i) {
        p[i] = server_id[i];
    }
    p += 5;
    // 最后四位随机值
    unsigned int number = osip_build_random_number();
    for (int i = 0; i < 4; ++i) {
        p[i] = (char)((number >> i) & 0x1) + '0';
    }

    return atoi(ssrc);

}

void GbClientManager::InitiateBroadcast()
{
    osip_message_t *invite_message = GenerateRequest("INVITE", message_cseq_++, profile_.get_audioout_channal_id(), true);
    if (invite_message == NULL) {
        GB_ERR("generate request fail\n");
        return ;
    }

    char sdp_buf[1024] = {0};
    char *p = sdp_buf;
    uint32_t ssrc = GenerateSsrc(true);

    p += sprintf(p, "v=0\r\n");
    p += sprintf(p, "o=%s 0 0 IN IP4 %s\r\n", profile_.get_sip_user_name(), profile_.get_local_ip());
    p += sprintf(p, "s=Play\r\n");
    p += sprintf(p, "c=IN IP4 %s\r\n", profile_.get_local_ip());
    p += sprintf(p, "t=0 0\r\n");
    p += sprintf(p, "m=audio %d TCP/RTP/AVP 8 96\r\n", profile_.get_broadcast_port());
    p += sprintf(p, "a=recvonly\r\n");
    /*这里支持两种格式，PCMA 或者 PS，RTP 负载音频只有 PCMA，PS 里面才有 PCMU*/
    p += sprintf(p, "a=rtpmap:8 PCMA/8000\r\n");
    p += sprintf(p, "a=rtpmap:96 PS/90000\r\n");
    p += sprintf(p, "a=setup:active\r\n");
    p += sprintf(p, "a=connection:new\r\n");
    p += sprintf(p, "y=%010d\r\n", ssrc);
     /*1: G711       8: 8kbps     1: 8kHZ*/
    p += sprintf(p, "f=v/////a/1/8/1\r\n");
    p += sprintf(p, "a=y:%010d\r\n", ssrc);
    p += sprintf(p, "a=f:v/////a/1/8/1\r\n");

    int ret = SipToStringAndSend(invite_message, "application/sdp", sdp_buf, p - sdp_buf);
    if (ret == SUCCESS) {
        std::shared_ptr<InviteDialog> ptr = std::make_shared<InviteDialog>(invite_message);
        dialog_mng_.AddDialog(ptr);
    }

    osip_message_free(invite_message);

    return ;
}

void GbClientManager::HandleMessageResponse(osip_message_t *response)
{
    GB_DBG("message hanldle status code:%d\n", response->status_code);

    /*验证会话是否存在*/
    std::shared_ptr<MessageDialog> message_ptr;
    int ret = dialog_mng_.PopDialog(response, message_ptr);
    if (ret != SUCCESS) {
        GB_DBG("not find Dialog for response\n");
        return ;
    }

    /* message 的回复一般是 200 OK，表示对端收到了你发送的消息 */
    if (response->status_code == 200) {

        if (message_ptr->get_type() == MessageDialog::Type::QUREY_RECORD) {
            /* 录像需要判断录像记录是否发送完毕 */
            HandleRecordResponse(response, message_ptr);
        } else if (message_ptr->get_type() == MessageDialog::Type::NOTIFY_BROADCAST) {
            /* 广播需要发起 INVITE 请求，这个需要新开一个会话*/
            InitiateBroadcast();
        } else if (message_ptr->get_type() == MessageDialog::Type::NOTIFY_HEARTBEAT) {
            /*收到心跳回复，清除心跳超时计数*/
            heartbeat_timeout_count_ = 0;
        } else {
            ;
        }
    } else {
        GB_ERR("unhandled status code:%d\n", response->status_code);
    }

    return ;
}

void GbClientManager::HandleMessage(osip_message_t *message)
{
    /*  MESSAGE消息要考虑回复和请求:
             1. Rquest: 服务器会通过内置的 xml 里面的内容描述请求内容，先回一个 200 OK, 这里分为有应答和无应答消息
                        有应答消息需要发送一个 MESSAGE 里面 xml 标注 response 返回服务器查询的内容
             2. Response: 这个是服务器对我们发送的 message 的回复，大部分收到回复会话就结束了，小部分还需要继续回复消息
    */

    if (MSG_IS_RESPONSE(message)) {
        HandleMessageResponse(message);
    } else {
        // 设备收到请求消息后要立即返回 200 OK，告诉服务器消息收到了
        Send200Ok(message);
        HandleMessageRquest(message);
    }

}

void GbClientManager::HandleAck(osip_message_t *message)
{
    /*验证会话是否存在*/
    std::shared_ptr<InviteDialog> invite_ptr;
    int ret = dialog_mng_.PopDialog(message, invite_ptr);
    if (ret != SUCCESS) {
        GB_ERR("not find Dialog for ack\n");
        return ;
    }

    do {
        if (invite_ptr->get_type() == InviteDialog::Type::PLAY) {
            live_streaming_mng_->StartPushStreaming(&invite_ptr->get_sdp_parse(), invite_ptr->get_call_id());
        } else if (invite_ptr->get_type() == InviteDialog::Type::PLAYBACK) {
            record_mng_->StartPushRecord(&invite_ptr->get_sdp_parse(), invite_ptr);
        } else if (invite_ptr->get_type() == InviteDialog::Type::DOWNLOAD) {
            record_mng_->StartPushRecord(&invite_ptr->get_sdp_parse(), invite_ptr); // 下载录像和推流走一个接口
        }
    } while (0);

    return;
}

void GbClientManager::HandleSdpPlay(SdpParse &sdp_parse, char *out_buf)
{
    char *p = out_buf;

    /*外部给的 buf 大小是 1024，内容大小不能超过该值*/
    p += sprintf(p, "v=0\r\n");
    p += sprintf(p, "o=%s 0 0 IN IP4 %s\r\n", profile_.get_video_channal_id(), profile_.get_local_ip());
    p += sprintf(p, "s=Play\r\n");
    p += sprintf(p, "c=IN IP4 %s\r\n", profile_.get_local_ip());
    p += sprintf(p, "t=0 0\r\n");

    // 一般设备要从服务器下发的负载类型中选择一种，但是国标基本都是 "96 PS/90000" 这里就不解析了
    p += sprintf(p, "m=video %d %s %d\r\n", profile_.get_media_port(), sdp_parse.get_transport().c_str() , 96);
    if (sdp_parse.get_setup() == GB_ACTIVE) {  // 这个是建立链接的方式，如果服务器那边表示主动连接，我门就要回被动接受
        p += sprintf(p, "a=setup:passive\r\n");
    } else {
        p += sprintf(p, "a=setup:active\r\n");
    }
    if (sdp_parse.get_connect().size() != 0 ) {
        p += sprintf(p, "a=connection:%s\r\n", sdp_parse.get_connect().c_str());
    }
    p += sprintf(p, "a=sendonly\r\n");  // 推流只发送
    p += sprintf(p, "a=rtpmap:96 PS/90000\r\n");
    p += sprintf(p, "y=%010u\r\n", sdp_parse.get_ssrc());
    if (sdp_parse.get_encode_proterty().size() != 0 ) {
        p += sprintf(p, "f=%s\r\n", sdp_parse.get_encode_proterty().c_str());
    } /*else {
        VideoEncS encode_cfg = {0};
        conf_get_realvideocfg(&encode_cfg);
        p += sprintf(p, "f=v/2/6/%d/2/a/1/8/1\r\n", encode_cfg.enc[0].fps);
    }*/

    return ;
}

void GbClientManager::HandleSdpPlayback(SdpParse &sdp_parse, char *out_buf)
{
    char *p = out_buf;

    /*外部给的 buf 大小是 1024，内容大小不能超过该值*/
    p += sprintf(p, "v=0\r\n");
    p += sprintf(p, "o=%s 0 0 IN IP4 %s\r\n", profile_.get_video_channal_id(), profile_.get_local_ip());
    p += sprintf(p, "s=Playback\r\n");
    p += sprintf(p, "c=IN IP4 %s\r\n", profile_.get_local_ip());
    p += sprintf(p, "t=0 0\r\n");

    // 一般设备要从服务器下发的负载类型中选择一种，但是国标基本都是 "96 PS/90000" 这里就不解析了
    p += sprintf(p, "m=video %d %s %d\r\n", profile_.get_media_port(), sdp_parse.get_transport().c_str() , 96);
    if (sdp_parse.get_setup() == GB_ACTIVE) {  // 这个是建立链接的方式，如果服务器那边表示主动连接，我门就要回被动接受
        p += sprintf(p, "a=setup:passive\r\n");
    } else {
        p += sprintf(p, "a=setup:active\r\n");
    }
    if (sdp_parse.get_connect().size() != 0 ) {
        p += sprintf(p, "a=connection:%s\r\n", sdp_parse.get_connect().c_str());
    }
    p += sprintf(p, "a=sendonly\r\n");  // 推流只发送
    p += sprintf(p, "a=rtpmap:96 PS/90000\r\n");
    p += sprintf(p, "y=%010u\r\n", sdp_parse.get_ssrc());
    if (sdp_parse.get_encode_proterty().size() != 0 ) {
        p += sprintf(p, "f=%s\r\n", sdp_parse.get_encode_proterty().c_str());
    }
    return ;
}

void GbClientManager::HandleSdpDownload(SdpParse &sdp_parse, char *out_buf)
{
    char *p = out_buf;
    /*外部给的 buf 大小是 1024，内容大小不能超过该值*/
    p += sprintf(p, "v=0\r\n");
    p += sprintf(p, "o=%s 0 0 IN IP4 %s\r\n", profile_.get_video_channal_id(), profile_.get_local_ip());
    p += sprintf(p, "s=Download\r\n");
    p += sprintf(p, "c=IN IP4 %s\r\n", profile_.get_local_ip());
    p += sprintf(p, "t=0 0\r\n");

    // 一般设备要从服务器下发的负载类型中选择一种，但是国标基本都是 "96 PS/90000" 这里就不解析了
    p += sprintf(p, "m=video %d %s %d\r\n", profile_.get_media_port(), sdp_parse.get_transport().c_str() , 96);
    if (sdp_parse.get_setup() == GB_ACTIVE) {  // 这个是建立链接的方式，如果服务器那边表示主动连接，我门就要回被动接受
        p += sprintf(p, "a=setup:passive\r\n");
    } else {
        p += sprintf(p, "a=setup:active\r\n");
    }
    if (sdp_parse.get_connect().size() != 0 ) {
        p += sprintf(p, "a=connection:%s\r\n", sdp_parse.get_connect().c_str());
    }
    p += sprintf(p, "a=sendonly\r\n");  // 推流只发送
    p += sprintf(p, "a=rtpmap:96 PS/90000\r\n");
    p += sprintf(p, "a=filesize:%d\r\n", record_mng_->DownloadGetFileSize(sdp_parse.get_start_time(), sdp_parse.get_end_time()));
    p += sprintf(p, "y=%010u\r\n", sdp_parse.get_ssrc());
    if (sdp_parse.get_encode_proterty().size() != 0 ) {
        p += sprintf(p, "f=%s\r\n", sdp_parse.get_encode_proterty().c_str());
    }
    return ;
}

void GbClientManager::HandleInviteRquest(osip_message_t *request)
{
    do {
        std::shared_ptr<InviteDialog> invite_dialog = std::make_shared<InviteDialog>(request);

        SdpParse &sdp_parse = invite_dialog->get_sdp_parse();

        char sdp_buf[1024] = {0};
        switch (invite_dialog->get_type()) {
            case InviteDialog::Type::PLAY:
                HandleSdpPlay(sdp_parse, sdp_buf);
                break;
            case InviteDialog::Type::PLAYBACK:
                HandleSdpPlayback(sdp_parse, sdp_buf);
                break;
            case InviteDialog::Type::DOWNLOAD:
                HandleSdpDownload(sdp_parse, sdp_buf);
                break;
            default:
                GB_DBG("not find name:%s\n", sdp_parse.get_name().c_str());
                break;
        }

        // 发送 200 OK 回复携带 sdp 消息体
        int ret = Send200Ok(request, "application/sdp", sdp_buf, strlen(sdp_buf), true);
        if (ret == SUCCESS) {
            ret = dialog_mng_.AddDialog(invite_dialog);
            if (ret != SUCCESS) {
                GB_ERR("Invite add dialog fail\n");
                break;
            }
        }
    } while(0);

    return;
}

void GbClientManager::HandleInviteResponse(osip_message_t *message)
{
    /*验证会话是否存在*/
    std::shared_ptr<InviteDialog> invite_ptr;
    int ret = dialog_mng_.PopDialog(message, invite_ptr);
    if (ret != SUCCESS) {
        GB_ERR("not find Dialog for invite\n");
        return ;
    }

    GB_DBG("Invite hanldle status code:%d dialog:%p\n", message->status_code, invite_ptr.get());

    if (message->status_code == 100) {
        dialog_mng_.AddDialog(invite_ptr);
        goto exit_; // 100 tring 不用处理
    } else if (message->status_code == 200 && invite_ptr->get_type() == InviteDialog::Type::PLAY) {
        /*服务器发给设备的 200 ok 中 play 表示语音广播*/
        osip_body_t *body = NULL;
        do {
            osip_message_get_body(message, 0, &body);
            if (body == NULL || message->content_type == NULL || message->content_type->type == NULL || message->content_type->subtype == NULL ||
                    strcasecmp(message->content_type->type, "application") != 0 || strcasecmp(message->content_type->subtype, "sdp") != 0) {
                GB_ERR("content type error\n");
                break;
            }
            /*原始的 sdp parse 是设备生成的，实际要使用服务器下发的，这里重新解析*/
            int ret = invite_ptr->get_sdp_parse().Decode(body->body);
            if (ret != SUCCESS) {
                break;
            }

            SendAck(message, profile_.get_audioout_channal_id());

            broadcast_ctrl_->RrepareRecvAudio(&invite_ptr->get_sdp_parse(), message->call_id);
        } while (0);
    }
exit_:
    return ;
}

void GbClientManager::HandleInvite(osip_message_t *message)
{
    if (MSG_IS_RESPONSE(message)) {
        HandleInviteResponse(message); // response 主要处理广播
    } else {
        Send100Tring(message); // 发送 100 tring 表示收到了 Invite
        HandleInviteRquest(message); // 直播，录像，文件下载
    }

    return ;
}

void GbClientManager::HandleBye(osip_message_t *message)
{
    Send200Ok(message); // 发送 200 OK 表示消息收到了

    /*
     *bye 是无特征的，并不能通过报文判断是属于 PLAY PLAYBACK 或者是 DOWNLOAD 中的哪一种
     *这里全部的推流会话都判断一遍，匹配的就停止
    */
    live_streaming_mng_->StopPushStreaming(message->call_id);
    record_mng_->DeleteRecordPlayByCallId(message->call_id);
    broadcast_ctrl_->StopRecvAduio(message->call_id);

    return ;
}

void  GbClientManager::HandQuerySubscribe(mxml_node_t* root_xml)
{
    /*这块逻辑暂时不处理*/
    char buf[32] = {0};

    mxml_node_t *cmd_type_xml = mxmlFindElement(root_xml, root_xml, "CmdType", NULL, NULL, MXML_DESCEND);
    if(cmd_type_xml == NULL || cmd_type_xml->child->value.text.string == NULL)
        return ;

    strcpy(buf, cmd_type_xml->child->value.text.string);
    if(!strcmp(buf, "Alarm")) { //Alarm Subscribe
        GB_DBG("alarm subscribe\n");
    } else if(!strcmp(buf, "Catalog")) { // 目录订阅
        GB_DBG("Directory SUBSCRIBE!\n");
    } else {
        GB_DBG("To do...\n");
    }

}

void GbClientManager::HandleSubscribe(osip_message_t *message)
{
    // subscribe 是无应答的，只需要回复 200 OK 就行，200 OK 中需要附加回复消息体
    // subscribe 分为事件订阅(包括报警事件、移动设备位置通知事件等)，目录订阅
    // 目录订阅(catalog) 需要在目录变更后通知订阅者

    osip_body_t *body = NULL;
    mxml_node_t *root_xml = NULL;

    do {
        osip_message_get_body(message, 0, &body);
        if(body == NULL)
            break;

        root_xml = mxmlLoadString(NULL, body->body, MXML_NO_CALLBACK);
        if( root_xml == NULL)
            break;

        mxml_node_t *query_xml = mxmlFindElement(root_xml, root_xml, "Query", NULL, NULL, MXML_DESCEND);
        if(query_xml) {
            HandQuerySubscribe(root_xml);
        }

        mxml_node_t *response_xml = GenerateXmlResponse(root_xml, "OK");
        if (response_xml == NULL)
            break;

        char buf[4096] = {0};
        int xml_len = 0;
        xml_len = mxmlSaveString(response_xml, buf, sizeof(buf), whitespace_cb);

        Send200Ok(message, "Application/MANSCDP+XML", buf, xml_len, true);

        mxmlDelete(response_xml);
    } while (0);

    if (root_xml) {
        mxmlDelete(root_xml);
    }

    return ;
}

void GbClientManager::HandleNotify(osip_message_t *message)
{
    // 对于设备来讲，notify 主要是服务器回复的 200 OK，基本不需要处理
    ;
}

void GbClientManager::HandleInfo(osip_message_t *message)
{
    /*设备收到 info 信息，目前只应用于录像回放过程中的控制*/
    /*这个会话回复 200 OK 就结束了，不需要加入会话管理*/

    Send200Ok(message, NULL, NULL, 0, true); // 发送 200 OK 表示消息收到了

    osip_body_t *body = NULL;

    osip_message_get_body(message, 0, &body);
    if(body == NULL || message->content_type == NULL ||
            message->content_type->type == NULL || message->content_type->subtype == NULL) {
        GB_ERR("message body error or content type error\n");
        return;
    }

    // 录像控制，Content-Type: Application/MANSRTSP
    if (strcasecmp(message->content_type->type, "Application") == 0 &&
            strcasecmp(message->content_type->subtype, "MANSRTSP") == 0) {

        if (strstr(body->body, "PLAY")) {
            GB_DBG("info handle play\n");
            record_mng_->SetRecordPlayStatus(message->call_id, PLAY);

            const char *p = NULL;
            if ((p = strstr(body->body, "Scale:")) != NULL) {
                p += strlen("Scale:");
                double scale = atof(p);

                record_mng_->SetRecordPlayScale(message->call_id, scale);
                GB_DBG("set record play scale:%lf\n", scale);

            }

            if ((p = strstr(body->body, "Range:")) != NULL) {
                // Range 值有两种
                //      1. now 表示从当前时间开始播放，这种不用管
                //      2. ntp=xx 表示从 xx 时间开始播放，偏移相对于文件开始
                if ((p = strstr(body->body, "npt=")) != NULL) {
                    p += strlen("ntp=");
                    time_t offset = atoi(p);

                    record_mng_->SetRecordPlayOffset(message->call_id, offset);
                    GB_DBG("set record play offset:%lld\n", offset);
                }
            }

        } else if (strstr(body->body, "PAUSE")) {
            GB_DBG("info handle pause\n");
            record_mng_->SetRecordPlayStatus(message->call_id, PAUSE);
        } else if (strstr(body->body, "TEARDOWN")) {
            GB_DBG("info handle teardown\n");
            record_mng_->SetRecordPlayStatus(message->call_id, TEARDOWN);
        }
    }
}

/*
* 生成注册报文，注册报文特殊的地方是需要添加 expires 并且大于 0 表示注册有效期，等于 0 表示注销
*
* @param[cseq] 请求序列号
* @param[expires] 有效期
*
* @return 成功返回 结构体指针，失败返回 NULL
*/
osip_message_t *GbClientManager::GenerateRegesterSip(uint32_t cseq,uint32_t expires)
{
    osip_message_t *register_message = NULL;

    register_message = GenerateRequest("REGISTER", cseq, profile_.get_sip_user_name(), true);
    if (register_message != NULL) {
        // REGISTER 需要带 expires
        char expires_str[32] = {0};
        snprintf(expires_str, sizeof(expires_str), "%u", expires);
        if (osip_message_set_expires(register_message, expires_str) != OSIP_SUCCESS) {
            osip_message_free(register_message);
            register_message = NULL;
            GB_ERR("register set expires");
        }
    }

    /*添加国标版本字段*/
    int ret = osip_message_set_header(register_message, "X-GB-Ver", profile_.GetGbVersionNo());
    if (ret != OSIP_SUCCESS) {
        GB_ERR("Failed to add ver header\n");
    }

    return register_message;
}

/*
*  根据 xml 报文生成 response 报文，cmd_type 和 sn 从 root 报文里面复制，device_id 使用设备用户 ID
*
* @param[root] xml 根节点
* @param[result] result 值
*
* @return 成功返回 xml 节点，失败返回 NULL
*/
mxml_node_t *GbClientManager::GenerateXmlResponse(mxml_node_t* root, const char *result)
{
    mxml_node_t *cmd_type_xml = mxmlFindElement(root, root, "CmdType", NULL, NULL, MXML_DESCEND);
    mxml_node_t *sn_xml = mxmlFindElement(root, root, "SN", NULL, NULL, MXML_DESCEND);
    mxml_node_t *device_id_xml = mxmlFindElement(root, root, "DeviceID", NULL, NULL, MXML_DESCEND);

    // 请求 xml 中 命令类型(CmdType)、命令序列号(SN)、设备编码(DeviceID) 是必须的
    if (cmd_type_xml == NULL || sn_xml == NULL || device_id_xml == NULL) {
        GB_ERR("XML error\n");
        return NULL;
    }

    const char *text = NULL, *cmd_type = NULL, *device_id = NULL;
    uint32_t sn = 0;
    int whitespace = 0;

    // cmd_type
    text = mxmlGetText(cmd_type_xml, &whitespace);
    if (text == NULL) {
        GB_ERR("get cmd type error\n");
        return NULL;
    }
    cmd_type = text + whitespace;

    // sn
    text = mxmlGetText(sn_xml, &whitespace);
    if (text == NULL) {
        GB_ERR("get sn error\n");
        return NULL;
    }
    sn = atoi(text);

    // device id
    text = mxmlGetText(device_id_xml, &whitespace);
    if (text == NULL) {
        GB_ERR("get device id error\n");
        return NULL;
    }

    device_id = text + whitespace;

    // 匹配 device id
    /* 这里 device_id 不应该限制等于 sip ID，设备下可能会有多个通道，
       比如视频编码通道(双目会有两个视频编码通道 ID)，音频编码通道，然后平台会通过不同的通道 ID 来查询对应编码通道的参数，
       所以 device_id 并不严格等于 sip ID
    if (strcmp(device_id, profile_.get_sip_user_name()) != 0) {
        GB_ERR("device ID error in xml\n");
        return NULL;
    }*/

    return GenerateXml("Response", sn, cmd_type, result, device_id);
}

/*
* 根据 sn 报文生成 xml response 报文，带 device id 不带 result 节点
*
* @param[sn] xml 会话的 sn 号
* @param[cmd_type] 命令类型
*
* @return 成功返回 xml 节点，失败返回 NULL
*/
mxml_node_t *GbClientManager::GenerateXmlResponse(uint32_t sn, const char *cmd_type)
{
    if (cmd_type == NULL) {
        GB_ERR("cmd type is NULL\n");
        return NULL;
    }

    return GenerateXml("Response", sn, cmd_type, NULL, profile_.get_sip_user_name());
}

/*
* 生成 xml response 报文
*
* @param[root_methond] 根节点方法 ex:response、query
* @param[sn] xml 会话 sn 号，为 0 表示不带该节点
* @param[cmd_type] 命令类型，为 NULL 表示不带该节点
* @param[result] 结果字段，为 NULL 表示不带该节点
* @param[need_add_device_id] 是否需要携带 device id
*
* @return 成功返回 xml 节点，失败返回 NULL
*/
mxml_node_t *GbClientManager::GenerateXml(const char *root_methond, uint32_t sn, const char *cmd_type, const char* result, const char *device_id)
{
    if (cmd_type == NULL){
        GB_ERR("cmd type is NULL\n");
        return NULL;
    }
    const char *head = NULL;

    if (xml_encoding_type_ == GB2312) {
        head = "?xml version=\"1.0\" encoding=\"GB2312\"?";
    } else {
        head = "?xml version=\"1.0\" encoding=\"UTF-8\"?";
    }

    mxml_node_t *reply_xml = mxmlNewElement(NULL, head);
    if (reply_xml == NULL) {
        GB_ERR("create reply xml fail\n");
        return NULL;
    }

    mxml_node_t *response_xml = mxmlNewElement(reply_xml, root_methond); // 创建根节点

    if (cmd_type != NULL) {
        mxml_node_t *cmd_type_xml = mxmlNewElement(response_xml, "CmdType");
        mxmlNewTextf(cmd_type_xml, 0, "%s", cmd_type);
    }

    if (sn != 0) {
        mxml_node_t *sn_xml = mxmlNewElement(response_xml, "SN");
        mxmlNewTextf(sn_xml, 0, "%d", sn);
    }

    if (device_id) {
        mxml_node_t *device_id_xml = mxmlNewElement(response_xml, "DeviceID");
        mxmlNewTextf(device_id_xml, 0, "%s", device_id);
    }

    if (result != NULL) {
        mxml_node_t *result_xml = mxmlNewElement(response_xml, "Result");
        mxmlNewTextf(result_xml, 0, "%s", result);
    }

    return reply_xml;
}

/*
* 生成请求消息的回复报文
*
* @param[request] 请求报文
* @param[status_code] 回复报文状态码
* @param[need_contact] 是否需要 contact ，只有几类特殊的报文需要携带 默认值 false
* @param[user] sip 设备编码，当携带 contact 才有效 默认值为 NULL
*
* @return 成功返回 结构体指针，失败返回 NULL
*/
osip_message_t *GbClientManager::GenerateResponseForRequest(osip_message_t* request, int status_code, bool need_contact, const char *user)
{
    osip_message_t *response = NULL;

    do {
        osip_message_init(&response);
        if (response == NULL) {
            GB_ERR("osip_message_init error!\n");
            break;
        }

        request->sip_version = osip_strdup(profile_.get_sip_version());
        if (request->sip_version == NULL) {
            GB_ERR("set version fail\n");
            break;
        }

        osip_message_set_status_code(response, status_code);

        response->reason_phrase = osip_strdup(osip_message_get_reason(status_code));
        if(response->reason_phrase == NULL) {
            if (response->status_code == 101)
                response->reason_phrase = osip_strdup("Dialog Establishement");
            else
                response->reason_phrase = osip_strdup("Unknown code");
        }
        response->req_uri = NULL;
        response->sip_method = NULL;

        if (response->reason_phrase == NULL) {
            GB_ERR("set reason phrase fail\n");
            break;
        }

        if (SetTo(response, request->to) != SUCCESS) {
            GB_ERR("set to fail\n");
            break;
        }

        if (SetFrom(response, NULL, request->from) != SUCCESS) {
            GB_ERR("set from fail\n");
            break;
        }

        if (osip_cseq_clone(request->cseq, &response->cseq) != OSIP_SUCCESS) {
            GB_ERR("clone cseq fail\n");
            break;
        }

        if (SetCallId(response, request->call_id) != SUCCESS) {
            GB_ERR("set call id fail\n");
            break;
        }

        if (SetVia(response, &request->vias) != SUCCESS) {
            GB_ERR("set via fail\n");
            break;
        }

        if (need_contact && SetContact(response, profile_.get_sip_user_name()) != SUCCESS) {
            GB_ERR("set contact fail\n");
            break;
        }

        if (osip_message_set_user_agent(response, profile_.get_sip_agent()) != OSIP_SUCCESS) {
            GB_ERR("set message fail\n");
            break;
        }

        /*record_route 克隆, 正常来说回复不需要带 record route 这里先屏蔽*/
        /*osip_record_route_t *record_route = NULL;
        int loop = 0;
        char *hvalue = NULL;

        while(osip_message_get_record_route(request,loop++,&record_route) != OSIP_UNDEFINED_ERROR){
            osip_record_route_to_str(record_route,&hvalue);
            DBG("recordGenerateRequest route:%s",hvalue);
            osip_message_set_record_route(response,hvalue);
            osip_free(hvalue);
            hvalue = NULL;
        }*/

        return response;
    } while(0);

    if (response) {
        osip_message_free(response);
    }

    GB_ERR("Init response fail\n");

    return NULL;
}

/*
* 生成对应回复消息的请求报文，属于一个会话中下一个事务的开始报文，例如 INVITE 中的 ack
*
* @param[response] 回复报文
* @param[method] 请求报文的方法
* @param[user] sip 设备编码
* @param[need_contact] 是否需要 contact ，只有几类特殊的报文需要携带 默认值 false
*
* @return 成功返回 结构体指针，失败返回 NULL
*/
osip_message_t *GbClientManager::GenerateRequestForResponse(osip_message_t* response, const char *method, const char *user, bool need_contact)
{
    uint32_t serial = 1;

    if (response->cseq->number != NULL) {
        serial = atoi(response->cseq->number) + 1;
    }

    return GenerateRequest(method, serial, user, need_contact, response->call_id, response->from, NULL);
}

/*
* 生成请求报文
*
* @param[method] 请求报文的方法
* @param[serial] 请求报文序列号
* @param[user] sip 设备编码
* @param[need_contact] 是否需要 contact 默认值 false
* @param[call_id] sip call_id 结构体 默认值 NULL
* @param[from] sip from 结构体 默认值 NULL
* @param[to] sip to 结构体 默认值 NULL
*
* @return 成功返回 结构体指针，失败返回 NULL
*/
osip_message_t *GbClientManager::GenerateRequest(const char *method, uint32_t serial, const char *user, bool need_contact, osip_call_id_t *call_id, osip_from_t *from, osip_to_t *to)
{
    /*设备主动生成请求的情况: 注册 注销 报警消息报送 */
    osip_message_t *request = NULL;

    do {
        if (method == NULL)
            break;

        osip_message_init(&request);
        if (request == NULL) {
            GB_DBG("osip_message_init error!\n");
            break;
        }

        request->sip_version = osip_strdup(profile_.get_sip_version());
        if (request->sip_version == NULL) {
            GB_ERR("set version fail\n");
            break;
        }

        request->sip_method = osip_strdup(method);
        if (request->sip_method == NULL) {
            GB_ERR("set method fail\n");
            break;
        }

        if (SetUri(request) != SUCCESS) {
            GB_ERR("set uri fail\n");
            break;
        }

        if (SetTo(request, to) != SUCCESS) {
            GB_ERR("set to fail\n");
            break;
        }

        if (SetFrom(request, user, from) != SUCCESS) {
            GB_ERR("set from fail\n");
            break;
        }

        if (SetCseq(request, method, serial) != SUCCESS) {
            GB_ERR("set cseq fail\n");
            break;
        }

        if (SetCallId(request, call_id) != SUCCESS) {
            GB_ERR("set call id fail\n");
            break;
        }

        /*请求的 via 都是自己生成，回复报文就需要复制*/
        if (SetVia(request, NULL) != SUCCESS) {
            GB_ERR("set via fail\n");
            break;
        }

        if (MSG_IS_INVITE(request) && SetSubject(request) != SUCCESS) {
            GB_ERR("set subject fail\n");
            break;
        }

        if (need_contact && SetContact(request, profile_.get_sip_user_name()) != SUCCESS) {
            GB_ERR("set contact fail\n");
            break;
        }

        /*max forward 请求都需要设置，回复不需要*/
        if (osip_message_set_max_forwards(request, "70") != OSIP_SUCCESS) {
            GB_ERR("set max forwards fail\n");
            break;
        }

        if (osip_message_set_user_agent(request, profile_.get_sip_agent()) != OSIP_SUCCESS) {
            GB_ERR("set user agent fail\n");
            break;
        }

        return request;
    } while(0);

    if (request) {
        osip_message_free(request);
    }

    GB_ERR("Init request message fail\n");

    return NULL;
}

/*
* 设置请求消息的 uri，格式基本固定，不需要外部传其它参数
* sip 请求消息必须携带
*
* @param[message] sip 消息结构体
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetUri(osip_message_t *message)
{
    if (message == NULL || message->req_uri != NULL) {
        GB_ERR("set uri error\n");
        return FAILURE;
    }

    /*格式 sip:SIP服务器编码@目的域名或IP地址端口*/
    /*sip:61011300490000000001@6101130049 SIP/2.0*/
    osip_uri_t *req_uri;

    osip_uri_init(&req_uri);
    if(NULL == req_uri)
        return FAILURE;

    osip_uri_set_scheme(req_uri, osip_strdup("sip"));
    osip_uri_set_username(req_uri, osip_strdup(profile_.get_sip_server_id()));
    osip_uri_set_host(req_uri, osip_strdup(profile_.get_civil_code()));

    message->req_uri = req_uri;
    return SUCCESS;
}

/*
* 设置 to, 会话发起者自己生成，会话接收者就需要复制。
* sip 消息必须携带
*
* @param[message] sip 消息结构体
* @param[to] 需要克隆的 to 消息体，为 NULL 的话就自己生成
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetTo(osip_message_t *message, osip_to_t *to)
{
    if (message == NULL || message->to != NULL) {
        GB_ERR("set to error\n");
        return FAILURE;
    }

    if (to != NULL) {
        /*克隆 to，回复消息需要添加 tag*/
        if (osip_to_clone(to, &(message->to)) != OSIP_SUCCESS)
            return FAILURE;

        osip_generic_param_t *tag = NULL;
        osip_to_get_tag(message->to, &tag);
        if (tag == NULL && MSG_IS_RESPONSE(message)) {
            char tag_str[64] = {0};
            snprintf(tag_str, sizeof(tag_str), "%u", osip_build_random_number());
            osip_to_set_tag(message->to, osip_strdup(tag_str));
        }
    } else {
        /*主动生成不需要带 tag ，to tag 是由接收端产生*/
        /*格式：To:<sip:SIP设备编码@源域名>*/
        /*to 的 sip 设备编码是服务器 ID*/
        /*<sip:34020000001320000001@6101130049>*/
        char to_buf[256] = {0};
        snprintf(to_buf, sizeof(to_buf), "<sip:%s@%s>",  profile_.get_sip_server_id(), profile_.get_civil_code());

        if (osip_message_set_to(message, to_buf) != OSIP_SUCCESS) {
            return FAILURE;
        }
    }

    return  SUCCESS;
}

/*
* 设置 from, 会话发起者自己生成，会话接收者就需要复制。请求生成的时候需要区分不同的设备，sip 消息设备，视频输出，音频输出，报警输入。
* sip 消息必须携带
*
* @param[message] sip 消息结构体
* @param[user] sip 设备编码
* @param[from] 需要克隆的 from 消息体，为 NULL 的话就自己生成
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetFrom(osip_message_t *message, const char *user, osip_from_t *from)
{
    if (message == NULL || message->from != NULL) {
        GB_ERR("set from error\n");
        return FAILURE;
    }

    if (from != NULL) {
        if (osip_from_clone(from, &(message->from)) != OSIP_SUCCESS) {
            return FAILURE;
        }
    } else {
        if (user == NULL) {
            return FAILURE;
        }
        /*格式： From:<sip:SIP设备编码@源域名>;tag=xxxxxxx*/
        /*<sip:34020000001320000001@6101130049>;tag=1523268668*/
        char from_buf[256] = {0};
        snprintf(from_buf, sizeof(from_buf), "<sip:%s@%s>;tag=%u",  user, profile_.get_civil_code(), osip_build_random_number());

        if (osip_message_set_from(message, from_buf) != OSIP_SUCCESS) {
            return FAILURE;
        }
    }

    return SUCCESS;
}

/*
* 设置 cseq
* sip 消息必须携带
*
* @param[message] sip 消息结构体
* @param[method] sip 方法
* @param[serial_number] cseq 序列号
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetCseq(osip_message_t *message, const char* method, uint32_t serial_number)
{
    if (message == NULL || message->cseq != NULL || method == NULL || serial_number == 0) {
        GB_ERR("set cseq error\n");
        return FAILURE;
    }

    char number_str[12] = {0};
    osip_cseq_t *sip_cseq;

    osip_cseq_init(&sip_cseq);
    if(sip_cseq == NULL)
        return FAILURE;

    snprintf(number_str, sizeof(number_str), "%u", serial_number);
    osip_cseq_set_number(sip_cseq, osip_strdup(number_str));

    osip_cseq_set_method(sip_cseq, osip_strdup(method));

    message->cseq = sip_cseq;

    return SUCCESS;
}

/*
* 设置 call_id，会话发起者自己生成，会话接收者就需要复制
* sip 消息必须携带
*
* @param[message] sip 消息结构体
* @param[call_id] sip call_id 结构体
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetCallId(osip_message_t *message, osip_call_id_t *call_id)
{
    if (message == NULL || message->call_id != NULL) {
        GB_ERR("set call id error\n");
        return FAILURE;
    }

    if (call_id != NULL) {
       if(osip_call_id_clone(call_id, &(message->call_id)) != OSIP_SUCCESS) {
            return FAILURE;
       }
    } else {
        char call_id[64] = {0};
        snprintf(call_id, sizeof(call_id), "%u", osip_build_random_number());

        if (osip_message_set_call_id(message, call_id) != OSIP_SUCCESS) {
            return FAILURE;
        }
    }

    return SUCCESS;
}

/*
* 设置 via，描述了消息从发送者到接收者的路径，事务的发起者自己生成，事务的接收者就需要复制。
* sip 消息必须携带
*
* @param[message] sip 消息结构体
* @param[vias] sip via 列表指针
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetVia(osip_message_t *message, osip_list_t *vias)
{
    if (message == NULL) {
        GB_ERR("set via error\n");
        return FAILURE;
    }

    if (vias != NULL) {
        /*如果有多个代理服务器的话，会有多个 via*/
        int pos = 0;
        while(!osip_list_eol(vias, pos)) {
            osip_via_t *via;
            osip_via_t *via2;

            via = (osip_via_t *)osip_list_get(vias, pos);
            if(osip_via_clone(via, &via2) != OSIP_SUCCESS) {
                break;
            }
            if (pos == 0 && via2->host != NULL) { // received 设置最前面的，也就是发送给我们的那个服务器就行
                osip_generic_param_t *rport;
                osip_via_param_get_byname (via2, (char *)"rport", &rport);  //从via 携带的参数中找rport
                if (rport != NULL && rport->gvalue == NULL) {   //如果rport的值为NULL，则用实际对端发送消息的端口给rport赋值
                    rport->gvalue = (char *)osip_malloc(9);
                    if (rport->gvalue != NULL) {
                        snprintf(rport->gvalue, 9, "%u", profile_.get_sip_local_port());
                    }
                }
                osip_via_set_received(via2, osip_strdup(profile_.get_local_ip()));
            }
            osip_list_add(&message->vias, via2, -1);

            pos++;
        }
    } else {
        /*格式： Via:SIP/2.0/UDP 源域名或IP地址端口*/
        /*设备上报：SIP/2.0/TCP 192.168.1.104:58667;rport;branch=z9hG4bK1731546947*/

        /*branch 由请求者生成, 每一个事务的 branch 都不能相同*/
        /*  branch参数的生成规则在RFC 3261中定义如下：
        *       1. branch参数必须以z9hG4bK开头
        *       2. branch 参数的长度不能超过 768 个字符
        *       3. branch 不能包含 单/双引号 反斜杠 百分号
        */
        char via_buf[256] = {0};

        snprintf(via_buf, sizeof(via_buf), "SIP/2.0/%s %s:%d;rport;branch=z9hG4bK%u",  profile_.GetTcpOrUdp(),
                            profile_.get_local_ip(), profile_.get_sip_local_port(), osip_build_random_number());

        if (osip_message_set_via(message, via_buf) != OSIP_SUCCESS) {
            return FAILURE;
        }
    }

    return SUCCESS;
}

/*
* 设置 subject ，字段用于提供一个简短的描述，描述会话的主题或内容。
* 只有 Invite 消息发起者需要携带，其它都不需要
*
* @param[message] sip 消息结构体
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetSubject(osip_message_t *message)
{
    if (message == NULL) {
        GB_ERR("set subject error\n");
        return FAILURE;
    }
    /*格式 Subject:媒体流发送者ID:发送方媒体流序列号,媒体流接收者ID:接收方媒体流序列号*/
    /*34010000002000000101:1,35010101001320000139:2*/
    /*对于客户端老说只有广播的时候会发 invite, 这里写死音频输出通道的 id*/
    char subject_buf[256] = {0};
    snprintf(subject_buf, sizeof(subject_buf), "%s:1,%s:2", profile_.get_sip_server_id(), profile_.get_audioout_channal_id());
    if (osip_message_set_subject(message, subject_buf) != OSIP_SUCCESS) {
        return FAILURE;
    }

    return SUCCESS;
}

/*
* 设置 contact ，用于指定请求或响应中的用户地址
* register 相关 Invite 相关(包括 ack 200ok 录像控制的 info message 不包括 bye 和 100 tring) 需要带 200 ok
* 另外就是 subject 和 notify 需要带，不过这两个设备目前没有支持
*
* @param[message] sip 消息结构体
* @param[user] 源设备编码，有 sip用户名，报警输入 ID，音频输出 ID, 视频通道 ID 等
*
* @return 成功返回 SUCCESS，失败返回 FAILURE
*/
int GbClientManager::SetContact(osip_message_t *message,  const char *user)
{
    if (message == NULL) {
        GB_ERR("set contact error\n");
        return FAILURE;
    }

    /*规则 Contact:<sip:SIP设备编码@源IP地址端口>*/
    /*<sip:34020000001320000001@192.168.1.104:5060>*/
    char contact_buf[256] = {0};

    snprintf(contact_buf, sizeof(contact_buf), "<sip:%s@%s:%d>", user, profile_.get_local_ip(), profile_.get_sip_local_port());

    if (osip_message_set_contact(message, contact_buf)) {
        return FAILURE;
    }

    return SUCCESS;
}

/*发送 message 通知服务器录像播放完成*/
void GbClientManager::SendPlayComplete()
{
    std::shared_ptr<InviteDialog> invite_ptr = record_mng_->GetCompletPtr();
    if (invite_ptr == nullptr) {
        GB_ERR("not find complete record\n");
        return ;
    }

    osip_call_id_t *call_id = invite_ptr->get_call_id();
    osip_from_t *from = invite_ptr->get_from();
    osip_from_t *to = invite_ptr->get_to();

    // 这个和 INVITE 属于同一个会话，需要覆盖 call-id from 和 to
    osip_message_t *message = NULL;
    message = GenerateRequest("MESSAGE", message_cseq_++, profile_.get_video_channal_id(),
                                            true, call_id, from, to);
    if (message == NULL) {
        return ;
    }

    mxml_node_t *request_xml = NULL;
    do {
        request_xml = GenerateXml("Notify", osip_build_random_number(), "MediaStatus", NULL, profile_.get_video_channal_id());
        if (request_xml == NULL) {
            break;
        }

        mxml_node_t *notify_xml = mxmlFindElement(request_xml, request_xml, "Notify", NULL, NULL, MXML_DESCEND);
        mxml_node_t *type_xml = mxmlNewElement(notify_xml, "NotifyType");
        mxmlNewTextf(type_xml, 0, "%d", 121);

        char buf[4096] = {0};
        int xml_len = 0;

        xml_len = mxmlSaveString(notify_xml, buf, sizeof(buf), whitespace_cb);

        SipToStringAndSend(message, "application/MANSCDP+xml", buf, xml_len);
    } while (0);

    if (request_xml != NULL) {
        mxmlDelete(request_xml);
    }

    if (message != NULL) {
        osip_message_free(message);
    }

    return ;
}

/*上传报警消息*/
void GbClientManager::reportAlarmInfo(int type)
{
    if (!IsRegister()) {
        return ;
    }

    if (get_duty_status() == OFFDUTY) {
        GB_DBG("device is offduty\n");
        return ;
    }

    mxml_node_t *request_xml = NULL;
    do {
        request_xml = GenerateXml("Notify", osip_build_random_number(), "Alarm", NULL, profile_.get_sip_user_name());
        if (request_xml == NULL) {
            break;
        }
        char timeBuf[32] = {0};
        TimenowToServerFormat(timeBuf, sizeof(timeBuf));

        mxml_node_t *notify_xml = mxmlFindElement(request_xml, request_xml, "Notify", NULL, NULL, MXML_DESCEND);

        mxml_node_t *priority_xml = mxmlNewElement(notify_xml, "AlarmPriority");//报警类型
        mxml_node_t *alarm_time_xml = mxmlNewElement(notify_xml, "AlarmTime");
        mxml_node_t *alarm_method_xml = mxmlNewElement(notify_xml, "AlarmMethod"); //联动方式

        mxml_node_t *info_xml = mxmlNewElement(notify_xml, "Info"); //联动方式
        mxml_node_t *alarm_type_xml = mxmlNewElement(info_xml, "AlarmType"); //检测报警

        mxmlNewTextf(priority_xml, 0, "%d", 4);  // 4 四级警情
        mxmlNewTextf(alarm_time_xml, 0, "%s", timeBuf);
        mxmlNewTextf(alarm_method_xml, 0, "%d", 5); // 5 视频报警

        mxmlNewTextf(alarm_type_xml, 0, "%d", type); // 报警类型

        std::shared_ptr<MessageDialog> ptr = std::make_shared<MessageDialog>();
        MessageRequestAddXmlAndSend(request_xml, ptr);

        set_duty_status(ALARM);  // 设置报警状态
    } while (0);

    if (request_xml != NULL) {
        mxmlDelete(request_xml);
    }
}

void GbClientManager::SendSnapshotEnd()
{
    if (!IsRegister()) {
        return ;
    }

    std::shared_ptr<MessageDialog> &ptr = snapshot_->get_message_ptr();
    if (ptr == nullptr || ptr->get_session_id().size() == 0) {
        GB_ERR("snapshot error\n");
        return;
    }

    mxml_node_t *request_xml = NULL;
    do {
        request_xml = GenerateXml("Notify", osip_build_random_number(), "UploadSnapShotFinished", NULL, ptr->get_device_id());
        if (request_xml == NULL) {
            break;
        }

        mxml_node_t *notify_xml = mxmlFindElement(request_xml, request_xml, "Notify", NULL, NULL, MXML_DESCEND);
        mxml_node_t *session_id_xml = mxmlNewElement(notify_xml, "SessionID");
        mxmlNewTextf(session_id_xml, 0, "%s", ptr->get_session_id().c_str());

        /*添加 id 列表*/
        mxml_node_t *snapshot_list_xml = mxmlNewElement(notify_xml, "SnapShotList");
        for (std::string &file_id : snapshot_->get_file_id()) {
            mxml_node_t *file_id_xml = mxmlNewElement(snapshot_list_xml, "SnapShotFileID");
            mxmlNewTextf(file_id_xml, 0, "%s", file_id.c_str());
        }
        mxmlElementSetAttrf(snapshot_list_xml, "Num", "%d", snapshot_->get_file_id().size());

        snapshot_->StopSnapshot();

        /*xml 转成字符串*/
        char buf[4096] = {0};
        int xml_len = 0;
        xml_len = mxmlSaveString(notify_xml, buf, sizeof(buf), whitespace_cb);
        if (xml_len == 0) {
            break;
        }

        MessageRequestAddXmlAndSend(request_xml, ptr);
    } while (0);

    if (request_xml != NULL) {
        mxmlDelete(request_xml);
    }

    return ;
}
