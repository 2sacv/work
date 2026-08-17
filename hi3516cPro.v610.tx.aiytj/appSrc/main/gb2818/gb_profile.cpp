/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_profile.cpp
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 配置文件:
                    - 保存配置信息
                    - 设置配置
                    - 获取配置
*/

#include "gb_profile.h"

#include "gb_common.h"

GbProfile::GbProfile() :
enable_(false),
sip_server_port_(5060),
sip_local_port_(5060),
register_interval_(30),
heartbeat_interval_(60),
heartbeat_timeout_count_(3),
stream_type_(1),
term_of_register_(3600),
transport_protocols_(GB_TCP),
longitude_(0.0),
latitude_(0.0),
mtu_(1460),
media_port_(22134),
broadcast_port_(22135),
sip_version_("SIP/2.0"),
sip_agent_("IP CAMERA") {}


int GbProfile::SetGbParam(GuoBiaoS &gb_param)
{
    set_manufacture(gb_param.manufacturer);
    set_owner(gb_param.owner);
    set_civil_code(gb_param.civilcode);
    set_sip_server_address(gb_param.sip_srv_ip);
    set_sip_server_port(gb_param.port);
    set_sip_server_id(gb_param.srv_id);
    set_dev_name(gb_param.dev_sysname);
    set_dev_model(gb_param.dev_type);
    set_sip_user_name(gb_param.username);
    set_sip_user_auth_id(gb_param.authname);
    set_sip_user_password(gb_param.password);
    set_term_of_register(gb_param.reg_interval);
    set_heartbeat_interval(gb_param.hb_interval);
    set_register_interval(30);
    set_enable(gb_param.enable);
    set_video_channal_id(gb_param.video_channal_id);
    set_alarmin_channal_id(gb_param.alarm_id);
    set_audioout_channal_id(gb_param.username);  // 音频输出暂时使用用户名

    set_stream_type(gb_param.streamtype);
    set_sip_local_port(gb_param.localport);

    set_transport_protocols(gb_param.protocoltype ? GB_TCP : GB_UDP);

    GB_DBG("init gb28181 param success\n");

    return SUCCESS;
}

int GbProfile::SetGbAddress(GBAddrS &gb_location)
{
    set_address_info(gb_location.address);
    set_longitude(gb_location.longitude);
    set_latitude(gb_location.latitude);

    GB_DBG("init gb28181 address info success\n");

    return SUCCESS;
}

int GbProfile::SetNetInfo(const char *ip, const char *mac, int mtu)
{
    if (ip == nullptr || mac == nullptr) {
        GB_ERR("param error\n");
        return FAILURE;
    }

    set_local_ip(ip);
    set_local_mac(mac);
    set_mtu(mtu);

    GB_DBG("set eth info success\n");

    return SUCCESS;
}

void GbProfile::PrintfGbParam()
{
    GB_DBG("GB_info\n");
    printf("---------------------gb status---------------------\n");
    printf("enable:%d\n", enable_);
    printf("---------------------gb param ---------------------\n");
    printf("sip_server_id:%s\n", sip_server_id_.c_str());
    printf("civil_code   :%s\n", sip_server_civil_code_.c_str());
    printf("sip_server_address:%s\n",sip_server_address_.c_str());
    printf("sip_server_port:%u\n", sip_server_port_);
    printf("manufacture:%s\n", manufacture_.c_str());
    printf("owner:%s\n", owner_.c_str());
    printf("dev_name:%s\n", dev_name_.c_str());
    printf("dev_model:%s\n", dev_model_.c_str());
    printf("sip_user_name:%s\n", sip_user_name_.c_str());
    printf("sip_user_auth_id:%s\n", sip_user_auth_id_.c_str());
    printf("sip_user_password:%s\n", sip_user_password_.c_str());
    printf("video_channal_id:%s\n", video_channal_id_.c_str());
    printf("alarmin_channal_id:%s\n", alarmin_channal_id_.c_str());
    printf("audioout_channal_id:%s\n", audioout_channal_id_.c_str());
    printf("local_port:%u\n", sip_local_port_);
    printf("register_interval:%u\n", register_interval_);
    printf("heartbeat_interval:%u\n", heartbeat_interval_);
    printf("heartbeat_timeout_count:%u\n", heartbeat_timeout_count_);
    printf("stream_type:%u\n", stream_type_);
    printf("term_of_register:%u\n", term_of_register_);
    printf("transport_protocols:%s\n", transport_protocols_ == GB_TCP ? "TCP" : "UDP");
    printf("address_info:%s\n", address_info_.c_str());
    printf("longitude:%f\n", longitude_);
    printf("latitude:%f\n", latitude_);
    printf("---------------------ip info  ---------------------\n");
    printf("local_ip:%s\n", local_ip_.c_str());
    printf("local_mac:%s\n", local_mac_.c_str());
    printf("---------------------   end   ---------------------\n");
}