/*****************************************************************************
 * Tencent is pleased to support the open source community by making IoT Video available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 *is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 *or implied. See the License for the specific language governing permissions and limitations under
 *the License.
 *
 *
 * @file    iv_err.h
 * @brief   Description err infomation
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_ERR_H__

#define __IV_ERR_H__

#ifdef __cplusplus
extern "C" {
#endif /* "__cplusplus" */

/*Defines iv err code*/
typedef enum
{
    /*success*/
    IV_ERR_NONE = 0,

    /*system module err*/
    IV_ERR_SYS_INIT_PRM_NULL       = -100, /*sys initPrm is NULL*/
    IV_ERR_SYS_INIT_CB_NULL        = -101, /*sys init callback is NULL*/
    IV_ERR_SYS_INIT_PRM_RANGE      = -102, /*sys initPrm is out of range*/
    IV_ERR_SYS_DEVICE_INFORMATION  = -103, /*fail to get device information*/
    IV_ERR_SYS_DYNAMIC_REG_DEVIECE = -104, /*fail to dynamic register device*/
    IV_ERR_SYS_NOT_SUPPORT         = -105, /*don't support feature*/
    IV_ERR_SYS_NTP_NOT_AVAILABLE   = -106, /*ntp time not available*/
    IV_ERR_SYS_FILE_OPT_FAIL       = -107, /*sys opt file wrong*/
    IV_ERR_SYS_MALLOC_FAIL         = -108, /*fail to malloc*/
    IV_ERR_SYS_COMMUNICATE_FAIL    = -109, /*fail to communicate with server*/
    IV_ERR_SYS_COMMUNICATE_TIMEOUT = -110, /*communicate with server timeout*/
    IV_ERR_SYS_MAX_RETRY_COUNT     = -111, /*reach the maximum retry count*/

    /*data model module err*/
    IV_ERR_DM_INIT_PRM_NULL     = -200, /*data modle initPrm is NULL*/
    IV_ERR_DM_INIT_CB_NULL      = -201, /*data modle init callback is NULL*/
    IV_ERR_DM_INIT_PRM_RANGE    = -202, /*data modle initPrm is out of range*/
    IV_ERR_DM_INIT_ENV          = -203, /*fail to init data modle env*/
    IV_ERR_DM_TYPE_NOT_SUPPORT  = -204, /*data model type is not support*/
    IV_ERR_DM_NULL_PTR          = -205, /*input param has null point*/
    IV_ERR_DM_REPORT_EVENT_FAIL = -206, /*report event fail */
    IV_ERR_DM_REPORT_BUSY       = -207, /*data modle report busy */
    IV_ERR_DM_REQUEST_BUSY      = -208, /*data modle request busy */

    /*av module err*/
    IV_ERR_AVT_INIT_PRM_NULL        = -300, /*av talk initPrm is NULL*/
    IV_ERR_AVT_INIT_CB_NULL         = -301, /*av talk init callback is NULL*/
    IV_ERR_AVT_INIT_PRM_RANGE       = -302, /*av talk initPrm is out of range*/
    IV_ERR_AVT_REQ_CHN_BUSY         = -303, /*av talk request channel busy*/
    IV_ERR_AVT_SEND_STREAM_TOO_BIG  = -304, /*the avenc data is bigger than maxFrameSize*/
    IV_ERR_AVT_CHN_NOT_EXIT         = -305, /* av channel not exit */
    IV_ERR_AVT_NEED_IDR_FRAME       = -306, /* need idr frame */
    IV_ERR_AVT_MALLOC_BUFFER_FAILED = -307, /* malloc send buffer failure */
    IV_ERR_AVT_FAILED               = -308, /* error */
    IV_ERR_AVT_INPUT_PARAM_NULL     = -309, /*input parameter is NULL*/
    IV_ERR_AVT_INPUT_PARAM_INVAILD  = -310, /*input parameter is invalid*/
    IV_ERR_AVT_SEND_DATA_TIMEOUT    = -311, /*send data timeout*/
    IV_ERR_AVT_STATE_ERROR          = -312, /*state error*/
    IV_ERR_AVT_VIDEO_DATA_ERROR     = -313, /*audio,video data error*/

    /*av voip module err*/
    IV_ERR_AVT_VOIP_NOT_ACTIVATED       = -350, /*voip not activated*/
    IV_ERR_AVT_VOIP_EXPIRED             = -351, /*voip service expired*/
    IV_ERR_AVT_VOIP_SERVER_RESP_STAUS   = -352, /*voip server response errcode*/
    IV_ERR_AVT_VOIP_SERVER_RESP_MSG     = -353, /*voip server response msg wrong*/
    IV_ERR_AVT_VOIP_SN_INVAILD          = -354, /*voip sn is invalid*/
    IV_ERR_AVT_VOIP_MODELID_INVAILD     = -355, /*voip modeid is invalid*/
    IV_ERR_AVT_VOIP_CANCELLED           = -361, /*voip operation cancelled*/
    IV_ERR_AVT_VOIP_UNKNOWN             = -362, /*voip unknown error*/
    IV_ERR_AVT_VOIP_INVALID_ARGUMENT    = -363, /*voip invalid argument*/
    IV_ERR_AVT_VOIP_TIMEOUT             = -364, /*voip operation timeout*/
    IV_ERR_AVT_VOIP_NOT_FOUND           = -365, /*voip entity not found, e:file*/
    IV_ERR_AVT_VOIP_ALREADY_EXISTS      = -366, /*voip entity already exists*/
    IV_ERR_AVT_VOIP_PERMISSION_DENIED   = -367, /*voip permission denied*/
    IV_ERR_AVT_VOIP_RESOURCE_EXHAUSTED  = -368, /*voip resource exhausted*/
    IV_ERR_AVT_VOIP_FAILED_PRECONDITION = -369, /*voip failed precondition*/
    IV_ERR_AVT_VOIP_ABORTED             = -370, /*voip aborted*/
    IV_ERR_AVT_VOIP_OUT_OF_RANGE        = -371, /*voip operation out of range*/
    IV_ERR_AVT_VOIP_UNIMPLEMENTED       = -372, /*voip unimplemented*/
    IV_ERR_AVT_VOIP_INTERNAL            = -373, /*voip internal error*/
    IV_ERR_AVT_VOIP_UNAVAILABLE         = -374, /*voip unavailable*/
    IV_ERR_AVT_VOIP_DATA_LOSS           = -375, /*voip data loss*/
    IV_ERR_AVT_VOIP_UNAUTHENTICATED     = -376, /*voip unauthenticated*/
    IV_ERR_AVT_VOIP_IO                  = -377, /*voip io error*/
    IV_ERR_AVT_VOIP_RECV_MSG            = -378, /*voip recv msg error*/
    IV_ERR_AVT_VOIP_INVALID_DEVICE_INFO = -379, /*voip invalid device info*/

    /*add device err*/
    IV_ERR_AD_QR_NO_PARSE_RESULT = -400, /*no qr parse result*/
    IV_ERR_AD_QR_PARSE_PRM_RANGE = -401, /*qr parse result out of range*/
    IV_ERR_AD_INIT_PRM_RANGE     = -402, /*input param out of range*/
    IV_ERR_AD_INIT_PRM_NULL      = -403, /*input NULL pointer*/
    IV_ERR_AD_INIT_CB_NULL       = -404, /*callback function is NULL*/
    IV_ERR_AD_SUBSCRIBE_FAIL     = -405, /*subscribe fail*/
    IV_ERR_AD_PUBLISH_FAIL       = -406, /*publish fail*/
    IV_ERR_AD_TIME_OUT           = -407, /*add device time out*/

    /*record module err*/
    IV_ERR_RD_INIT_PRM_NULL        = -500, /*rd initPrm is NULL*/
    IV_ERR_RD_INIT_CB_NULL         = -501, /*rd init callback is NULL*/
    IV_ERR_RD_INIT_PRM_RANGE       = -502, /*rd initPrm is out of range*/
    IV_ERR_RD_INIT_SD_PATH_INVALID = -503, /*rd initPrm sd path is invalid*/
    IV_ERR_RD_CREATE_RECORD_DIR    = -504, /*create record dir fail*/

    /*cloud storage module err*/
    IV_ERR_CS_INIT_PRM_NULL             = -600, /*cloud storage initPrm is NULL*/
    IV_ERR_CS_INIT_CB_NULL              = -601, /*cloud storage callback function is NULL*/
    IV_ERR_CS_INIT_PRM_RANGE            = -602, /*cloud storage initPrm is out of range*/
    IV_ERR_CS_UPLOAD_AUTH_NOT_AVAILABLE = -603, /*cos upload auth info not inited*/
    IV_ERR_CS_UPLOAD_OPEN_FILE_FAIL     = -604, /*cos upload open ts/jpg file fail*/
    IV_ERR_CS_PRM_NOT_AVAILABLE         = -605, /*parameter not inited*/
    IV_ERR_CS_PRM_MALLOC_FAIL           = -606, /*parameter malloc fail*/
    IV_ERR_CS_APPLY_NO_SERVICE          = -607, /*cloud storage service stoped*/
    IV_ERR_CS_QUERY_SERVICE_TIMEOUT     = -608, /*query cloud service timeout*/
    IV_ERR_CS_EVENT_IS_VALID            = -609, /*cloud storge is invalid*/
    IV_ERR_CS_INIT_REPEAT               = -610, /*cloud storge is repetitive initial*/
    IV_ERR_CS_NEED_IDR_FRAME            = -611, /* need idr frame */
    IV_ERR_CS_NO_BALANCE                = -612, /* no available balance */
    IV_ERR_CS_HTTP_DATA_FAIL            = -613, /* http send data fail */

    /*user cmd module err*/
    IV_ERR_UC_INIT_PRM_NULL = -700, /*user cmd initPrm is NULL*/
    IV_ERR_UC_PRM_RANGE     = -701, /*user cmd pararm is out of range*/
    IV_ERR_UC_INIT_CB_NULL  = -702, /*user cmd callback function is NULL*/
    IV_ERR_UC_INIT_PRO_FAIL = -703, /*user cmd init pro fail*/
    IV_ERR_UC_MSG_SEND_FAIL = -704, /*user cmd send msg fail*/
    IV_ERR_UC_MSG_LEN_RANGE = -705, /*user cmd msg len is out of range*/

    /*cloud ai err*/
    IV_ERR_CLOUDAI_WRONG_PARA    = -750, /*cloud ai param wrong*/
    IV_ERR_CLOUDAI_HTTP_REQ      = -751, /*cloud ai http err*/
    IV_ERR_CLOUDAI_STOP          = -752, /*cloud ai stop*/
    IV_ERR_CLOUDAI_INIT_FAIL     = -753, /*cloud ai init fail*/
    IV_ERR_CLOUDAI_FILE_FAIL     = -754, /*cloud ai file operation fail*/
    IV_ERR_CLOUDAI_PROPERTY_FAIL = -755, /*cloud ai property send or set fail*/
    IV_ERR_CLOUDAI_MQTT_PUBILSH  = -756, /*cloud ai mqtt publish fail*/
    IV_ERR_CLOUDAI_MQTT_TIMEOUT  = -757, /*cloud ai mqtt msg timeout*/
    IV_ERR_CLOUDAI_MEMORY_FAIL   = -758, /*cloud ai memory fail*/
    IV_ERR_CLOUDAI_MODEL_FAIL    = -759, /*cloud ai modle fail*/
    IV_ERR_CLOUDAI_COMM_FAIL     = -760, /*cloud ai communicate fail*/

    /*ota err*/
    IV_ERR_OTA_INIT_PRM_NULL       = -800, /*ota initPrm is NULL*/
    IV_ERR_OTA_INIT_CB_NULL        = -801, /*ota callback function is NULL*/
    IV_ERR_OTA_START_FAIL          = -802, /*ota start service fail*/
    IV_ERR_OTA_PROGRESS_TYPE_ERROR = -803, /*ota update progress type not support*/

    /*keep alive err*/
    IV_ERR_KP_INPUT_PRM_NULL  = -900, /*keep alive initPrm is NULL*/
    IV_ERR_KP_REGISTER_FAILED = -901, /*keep alive register failed*/
    IV_ERR_KP_SEND_FAILED     = -902, /*keep alive send message failed*/
    IV_ERR_KP_GET_TIMEOUT     = -903, /*keep alive get message timeout*/

    /*general err*/
    IV_ERR_DEVICE_OFFLINE = -1001, /*device is offline*/

    IV_ERR_BUTT
} iv_err_code_e;

#ifdef __cplusplus
}
#endif /* "__cplusplus" */

#endif /* __IV_ERR_H__ */
