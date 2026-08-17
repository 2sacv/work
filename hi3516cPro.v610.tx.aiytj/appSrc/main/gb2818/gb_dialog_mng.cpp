/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_sip_session.cpp
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 sip 会话管理
*/

#include "osipparser2/osip_parser.h"

#include "gb_common.h"
#include "gb_dialog_mng.h"
#include "gb_sdp_parse.h"
Dialog::Dialog(osip_message_t* message, time_t timeout_s) : Dialog(timeout_s)
{
    if (message == nullptr) {
        GB_ERR("messag is null\n");
        return;
    }

    /*普通的会话只需要 call id 就够了，不进行多余的 clone，代码保留，有需要再放开*/
    set_call_id(message->call_id);
    //set_from(message->from);
    //set_to(message->to);

    return ;
}

Dialog::Dialog(time_t timeout_time)
{
    initial_time_ = mono_sec();
    timeout_ = timeout_time;
}

/*基类资源释放函数*/
void Dialog::Clear()
{
    if (call_id_ != nullptr) {
        osip_call_id_free(call_id_);
        call_id_ = nullptr;
    }
    if (from_ != nullptr) {
        osip_from_free(from_);
        from_ = nullptr;
    }
    if (to_ != nullptr) {
        osip_to_free(to_);
        to_ = nullptr;
    }
}

int Dialog::set_from(osip_from_t *from)
{
    if (from == nullptr) {
        GB_ERR("from is nullptr\n");
        return FAILURE;
    }

    if (from_ == NULL && osip_from_clone(from, &from_) != OSIP_SUCCESS) {
        GB_ERR("osip from clone fail\n");
        return FAILURE;
    }

    return SUCCESS;
}

int Dialog::set_to(osip_to_t *to)
{
    if (to == nullptr) {
        GB_ERR("to is nullptr\n");
        return FAILURE;
    }

    if (to_ == NULL && osip_to_clone(to, &to_) != OSIP_SUCCESS) {
        GB_ERR("osip to clone fail\n");
        return FAILURE;
    }

    return SUCCESS;
}

int Dialog::set_call_id(osip_call_id_t *call_id)
{
    if (call_id == nullptr) {
        GB_ERR("call id is nullptr\n");
    }

    if (call_id_ == NULL && osip_call_id_clone(call_id, &call_id_) != OSIP_SUCCESS) {
        GB_ERR("osip call id clone fail\n");
        return FAILURE;
    }

    return SUCCESS;
}

bool Dialog::IsMatch(osip_message_t* message)
{
    /*正常情况下 对话由 to from call_id 唯一确定，从海康的测试来看他们只要求 call_id 匹配，这里只判断 call_id*/
    if (message == nullptr || message->call_id == nullptr) {
        GB_ERR("message error\n");
        return false;
    }

    if (osip_call_id_match(message->call_id, call_id_) != OSIP_SUCCESS) {
        return false;
    }

    return true;
}

InviteDialog::InviteDialog(osip_message_t* message) : Dialog(message, 20)
{
    set_from(message->from);
    set_to(message->to);
    /*body 消息类型判断，invite 只接受 application/sdp*/
    osip_content_type_t *content_type = message->content_type;
    if (content_type == nullptr ||
            content_type->type == nullptr || content_type->subtype == nullptr) {

        GB_ERR("content type is nullptr\n");
        return ;
    }
    if (strcasecmp(content_type->type, "application") != 0 ||
            strcasecmp(content_type->subtype, "sdp") != 0) {

        GB_ERR("content type[%s/%s] error\n", content_type->type, content_type->subtype);
        return ;
    }

    /*sdp 解析*/
    osip_body_t *body = nullptr;
    osip_message_get_body(message, 0, &body);
    if(body == nullptr) {
        GB_ERR("request body error\n");
        return;
    }

    int ret = sdp_parse_.Decode(body->body);
    if (ret != SUCCESS) {
        GB_ERR("sdp parse fail\n");
        return ;
    }

    /*Invite 类型判断*/
    if (sdp_parse_.get_name() == "Play") {
        type_ = Type::PLAY; /*语音广播和直播都是 play ，要具体细分的话要外部设置*/
    } else if (sdp_parse_.get_name() == "Playback") {
        type_ = Type::PLAYBACK;
        /*回放如果播放结束需要发送 sip 通知服务器，需要保留 from 和 to*/
        set_from(message->from);
        set_to(message->to);
    } else if (sdp_parse_.get_name() == "Download") {
        type_ = Type::DOWNLOAD;
    } else {
        GB_DBG("not find name:%s\n", sdp_parse_.get_name().c_str());
    }
}

int MessageDialog::SetDialogInfo(osip_message_t* message)
{
    int ret = SUCCESS;
    if (message == nullptr) {
        GB_ERR("messag is null\n");
        return FAILURE;
    }

    ret = set_call_id(message->call_id);
    //ret = set_from(message->from);
    //ret = set_to(message->to);

    return ret;
}

/**
 * 添加对话，内部会持有共享指针，需要释放需要显示调用 pop 函数
 *
 * @param[ptr] dialog 共享指针引用
 *
 * @return 成功返回会话的下标，失败返回 FAILURE
 */
int GbDialogMng::AddDialog(std::shared_ptr<InviteDialog> &ptr)
{
    if (ptr == nullptr) {
        GB_ERR("ptr is nullptr\n");
        return FAILURE;
    }

    /*找空位插入对话并且清除超时对话*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        if (arr_ptr == nullptr || arr_ptr->IsTimeout()) {
            arr_ptr = ptr;
            return SUCCESS;
        }
    }

    GB_INFO("not find position, add dialog fail\n");
    return FAILURE;
}

int GbDialogMng::AddDialog(std::shared_ptr<MessageDialog> &ptr)
{
    if (ptr == nullptr) {
        GB_ERR("ptr is nullptr\n");
        return FAILURE;
    }

    /*找空位插入对话并且清除超时对话*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        if (arr_ptr == nullptr || arr_ptr->IsTimeout()) {
            arr_ptr = ptr;
            return SUCCESS;
        }
    }

    GB_INFO("not find position, add dialog fail\n");
    return FAILURE;
}

int GbDialogMng::AddDialog(std::shared_ptr<RegisterDialog> &ptr)
{
    if (ptr == nullptr) {
        GB_ERR("ptr is nullptr\n");
        return FAILURE;
    }

    /*找空位插入对话并且清除超时对话*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        if (arr_ptr == nullptr || arr_ptr->IsTimeout()) {
            arr_ptr = ptr;
            return SUCCESS;
        }
    }

    GB_INFO("not find position, add dialog fail\n");
    return FAILURE;
}

/*弹出对话指针，如果未结束就再通过 add 添加*/
int GbDialogMng::PopDialog(osip_message_t* message, std::shared_ptr<InviteDialog> &ptr)
{
    if (message == nullptr || ptr != nullptr) {
        GB_ERR("message is nullptr\n");
        return FAILURE;
    }

    /*匹配对话并且弹出*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        if (arr_ptr != nullptr && arr_ptr->IsMatch(message)) {
            ptr = std::dynamic_pointer_cast<InviteDialog>(arr_ptr);
            if (ptr == nullptr) {
                GB_ERR("point type error");
                return FAILURE;
            }
            arr_ptr = nullptr;
            return SUCCESS;
        }
    }

    return FAILURE;
}

int GbDialogMng::PopDialog(osip_message_t* message, std::shared_ptr<MessageDialog> &ptr)
{
    if (message == nullptr || ptr != nullptr) {
        GB_ERR("message is nullptr\n");
        return FAILURE;
    }

    /*匹配对话并且弹出*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        if (arr_ptr != nullptr && arr_ptr->IsMatch(message)) {
            ptr = std::dynamic_pointer_cast<MessageDialog>(arr_ptr);
            if (ptr == nullptr) {
                GB_ERR("point type error");
                return FAILURE;
            }
            arr_ptr = nullptr;
            return SUCCESS;
        }
    }

    return FAILURE;
}

int GbDialogMng::PopDialog(osip_message_t* message, std::shared_ptr<RegisterDialog> &ptr)
{
    if (message == nullptr || ptr != nullptr) {
        GB_ERR("message is nullptr\n");
        return FAILURE;
    }

    /*匹配对话并且弹出*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        if (arr_ptr != nullptr && arr_ptr->IsMatch(message)) {
            ptr = std::dynamic_pointer_cast<RegisterDialog>(arr_ptr);
            if (ptr == nullptr) {
                GB_ERR("point type error");
                return FAILURE;
            }
            arr_ptr = nullptr;
            return SUCCESS;
        }
    }

    return FAILURE;
}

void GbDialogMng::PopAllDialog()
{
    /*清除会话管理这边的引用计数*/
    for (std::shared_ptr<Dialog> &arr_ptr : dialog_arr_) {
        arr_ptr = nullptr;
    }
}