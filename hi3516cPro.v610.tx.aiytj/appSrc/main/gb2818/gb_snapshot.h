/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_snapshot.h
* @Created Time : 2024-07-29
* @Version      : 1.0
* @Author       :
* @Description  : 国标图像抓拍 支持定时连拍 一次最多运行一个任务
*/

#ifndef GB_SNAPSHOT_H_
#define GB_SNAPSHOT_H_

#include "js_scheduler.h"

#include "gb_profile.h"
#include "gb_dialog_mng.h"

class GbSnapshot {
public:
    GbSnapshot(GbProfile &profile);
    ~GbSnapshot();

    std::vector<std::string> &get_file_id() {
        return file_id_;
    }

    std::shared_ptr<MessageDialog> &get_message_ptr() {
        return message_ptr_;
    }

    int StartSnapshot(uint32_t snap_num, uint32_t interval, const char *url, std::shared_ptr<MessageDialog> &message_ptr);
    int StopSnapshot();

private:
    static void StaticStartSnapshotCb(void *data);
    void StartSnapshotCb();

private:
    const GbProfile &profile_;
    JSScheduler sch_ = nullptr;             // 抓拍服务调度，不能和 sip 会话用一个调度
    JSTCHandle  hdl_snap_ = nullptr;  // 服务器可能会下发定时抓拍

    uint32_t dst_snap_num_ = 0;  // 目标连拍张数
    uint32_t cur_snap_num_ = 0;  // 目前抓拍张数
    std::string upload_url_; // 抓拍上传路径
    std::vector<std::string> file_id_; // 抓拍图片文件 ID ，最后上传结果时需要附带 ID 列表
    std::shared_ptr<MessageDialog> message_ptr_; // 抓拍会话的信息，最后上传结果需要用到

    bool snap_success = false;  // 抓拍完成标志
    uint32_t snap_no_ = 1; // 抓拍图像序号
};


#endif