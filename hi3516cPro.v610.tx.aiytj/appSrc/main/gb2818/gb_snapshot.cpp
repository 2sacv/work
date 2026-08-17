/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_snapshot.cpp
* @Created Time : 2024-07-29
* @Version      : 1.0
* @Author       :
* @Description  : 国标图像抓拍 支持定时连拍 一次最多运行一个任务
*/

#include <sys/time.h>

#include <iostream>
#include <sstream>

#include "encode_jpeg.h"
#include "js_http_client.h"
#include "js_scheduler.h"
#include "JFile.h"

#include "gb_snapshot.h"
#include "gb_client_manager.h"
#include "gb_common.h"

int jhttp_update_jpg(const std::string &url, const char *filename, const char *data, int data_size)
{
    /*
        2024/12/17 上海公安三所测试，post 的 url 内不能携带 ip 和端口，ip 和端口需要通过 host 携带
    */
    int err = -1;
    jhttp_client_t* httpclient = NULL;

    /*解析 url*/
    std::string protocol, host, address, port, path;
    auto protocolEnd = url.find("://");
    if (protocolEnd != std::string::npos) {
        protocol = url.substr(0, protocolEnd);
        // 剩余部分
        std::string remaining = url.substr(protocolEnd + 3);

        // 找到端口和路径分隔符
        auto portStart = remaining.find(':');
        auto pathStart = remaining.find('/');

        if (portStart != std::string::npos) {
            // 有端口
            address = remaining.substr(0, portStart);
            port = remaining.substr(portStart + 1, pathStart - portStart - 1);
        } else {
            // 没有端口
            address = remaining.substr(0, pathStart);

            if (protocol == "https") {
                port = "443";
            } else {
                port = "80";
            }
        }
        path = remaining.substr(pathStart);
        host = remaining.substr(0, pathStart);
        GB_INFO("protocol[%s] address[%s] port[%s] path[%s]\n", protocol.c_str(), address.c_str(), port.c_str(), path.c_str());
    } else {
        GB_ERR("Invalid URL format.");
    }

    do {
        httpclient = jhttp_client_create(protocol.c_str(), (char *)address.c_str(), atoi(port.c_str()));
        if(httpclient == NULL)
            break;

        jhttp_client_set_Param(httpclient, "Host", host.c_str());
        //jhttp_client_set_Param(httpclient, "filename", filename);
        jhttp_client_set_formdata(httpclient, "fileName", filename, (char *)data, data_size);

        err = jhttp_client_post(httpclient, path.c_str(), NULL, 0, NULL, NULL, NULL, 6*1000);
    } while(0);

    jhttp_client_destroy(httpclient);

    return err;
}

GbSnapshot::GbSnapshot(GbProfile &profile) : profile_(profile)
{
#ifndef GB28181_2022
    GB_DBG("not support gb2022\n");
    return ;
#endif

    // 抓拍要进行 http 传输，可能会连接超时，不能和直播流用一个线程
    sch_ = js_create_scheduler((char *)"gb_snapshot");
    if (sch_ == NULL) {
        GB_ERR("Init snapshot error\n");
    }
}

GbSnapshot::~GbSnapshot()
{
    StopSnapshot();

    js_delete_scheduler(sch_);
    sch_ = NULL;
}

int GbSnapshot::StartSnapshot(uint32_t snap_num, uint32_t interval, const char *url, std::shared_ptr<MessageDialog> &message_ptr)
{
    if (sch_ == nullptr) {
        GB_DBG("snapshot not init\n");
        return FAILURE;
    }

    if (url == nullptr || message_ptr == nullptr) {
        GB_ERR("url or ptr error\n");
        return FAILURE;
    }

    if (hdl_snap_ != nullptr) {
        StopSnapshot();
    }

    dst_snap_num_ = snap_num;
    cur_snap_num_ = 0;
    upload_url_ = url;
    message_ptr_ = message_ptr;
    snap_success = false;

    // interval 抓拍间隔 单位: 秒
    js_create_timer_r(sch_, 10, interval * 1000, StaticStartSnapshotCb, this, &hdl_snap_);

    return SUCCESS;
}

/*用于清除上一次抓拍的残留数据*/
int GbSnapshot::StopSnapshot()
{
    if (sch_ == nullptr) {
        GB_DBG("snapshot not init\n");
        return FAILURE;
    }

    if (hdl_snap_ != nullptr) {
        js_delete_timer_r(&hdl_snap_);
        hdl_snap_ = nullptr;
    }

    dst_snap_num_ = cur_snap_num_ = 0;
    message_ptr_ = nullptr;
    vector<string>().swap(file_id_);

    return SUCCESS;
}

void GbSnapshot::StaticStartSnapshotCb(void *data)
{
    GbSnapshot *this_ptr = static_cast<GbSnapshot *>(data);
    this_ptr->StartSnapshotCb();
}

void GbSnapshot::StartSnapshotCb()
{
    /*抓拍完成通知 sip server，但是调度还没被删除的情况*/
    if (snap_success == true) {
        return ;
    }

    if (cur_snap_num_ < dst_snap_num_) {
        do {
#ifdef CUST_SNAPSHOT
            int jpeg_len = MAX_PIC_LEN;
            char jpeg_buf[MAX_PIC_LEN] = {0};
            int ret = 0;

            ret = encode_snapshot_ex(jpeg_buf, &jpeg_len);
            if (ret != 0) {
                GB_ERR("snap jpeg fail : %d\n", ret);
                break;
            }

            // 生成图片名称，总共 45 位
            // 20 位设备 ID + 固定两位 "02" + 17 位时间 "YYYYMMDDhhmmssSSS" + 两位序列码 + 四位 ".jpg"
            char pic_name[46] = {0};
            memcpy(pic_name, profile_.get_sip_user_name(), 20);
            memcpy(pic_name + 20, "02", 2);

            /*组成时间部分*/
            // 获取当前时间
            struct timeval tv;
            gettimeofday(&tv, NULL);
            // 将时间转换为 UTC 时间
            struct tm *utcTime = gmtime(&tv.tv_sec);
            // 格式化日期和时间
            strftime(pic_name + 22, 15, "%Y%m%d%H%M%S", utcTime);
            // 添加毫秒部分
            snprintf(pic_name + 36, 4, "%03lld", tv.tv_usec / 1000);

            // 添加序列号
            snprintf(pic_name + 39, 7, "%02d.jpg", snap_no_%100);
            snap_no_++;

            std::ostringstream upload_url; // 组 url，上传 url 里面需要携带 name=&sessionID=
            upload_url << upload_url_ << "?name=" << pic_name << "&SessionID=" << message_ptr_->get_session_id();
            /* 上传图片*/
            ret = jhttp_update_jpg(upload_url.str(), pic_name, jpeg_buf, jpeg_len);
            if (ret < 0) {
                GB_ERR("update jpg fail\n");
                break;
            }

            GB_DBG("post pic[%s] success\n", pic_name);

            /*上传成功，添加列表*/
            file_id_.push_back(pic_name);
#else
            GB_ERR("not support snapshot\n");
            break;
#endif
        } while(0);

        cur_snap_num_++;
    } else {
        // 结束抓拍
        snap_success = true;
        GbClientManager::Enqueue(SNAPSHOT_END);
    }
}
