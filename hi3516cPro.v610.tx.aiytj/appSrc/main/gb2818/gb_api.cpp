/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_api.h
* @Created Time : 2024-04-29
* @Version      : 1.0
* @Author       :
* @Description  : 国标模块对外提供的所有接口都要在这里声明，外部调用国标函数的地方统一包含此文件
*/
#include "gb_common.h"
#include "gb_client_manager.h"
#include "gb_api.h"

static std::shared_ptr<GbClientManager> gb_mng;

int gb_init_server()
{
    if (gb_mng != NULL) {
        GB_ERR("gb already init\n");
        return FAILURE;
    }

    gb_mng = std::make_shared<GbClientManager>();

    GB_DBG("gb_init success\n");
    return SUCCESS;
}

int gb_uninit_server()
{
    if (gb_mng == NULL) {
        GB_ERR("gb is uninitialized\n");
        return FAILURE;
    }

    if (gb_mng->is_upgrading()) {
        /*通过国标 ota 需要向服务器上报状态，不进行反初始化*/
        GB_DBG("gb is upgrading, keep it running\n");
        return SUCCESS;
    }

    if (gb_mng.use_count() != 1) {
        // 可能有其它地方未释放指针，这里打印提示一下
        GB_ERR("current gb use count:%ld\n", gb_mng.use_count());
    }

    gb_mng.reset();

    GB_DBG("gb_uinit success\n");
    return SUCCESS;
}

int gb_get_online()
{
    if (gb_mng == NULL)
        return false;

    return gb_mng->IsRegister();
}

int gb_stop_playback()
{
    if (gb_mng == NULL)
        return FAILURE;

    gb_mng->StopClientPlayback();

    return SUCCESS;
}

const char *gb_get_version_str()
{
    if (gb_mng == NULL)
        return "unkown";

    return gb_mng->GetGbVersionStr();
}
