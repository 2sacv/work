#ifdef PLATFORM_TENCENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "debug.h"
#include "confapi.h"
#include "utils.h"
#include "base64.h"
#include "cJSON.h"
#include "jconfig.h"
#include "system_ctrl.h"
#include "tencent_http_service.h"
#include "tencent_server.h"
#include "js_http_client.h"
#include "jdns.h"
#include "conf_nand.h"
#include "factory_db.h"
#include "logapi.h"
#include "url.h"
#include "record_disk.h"
#include "jcpCmdImplement.h"
#include "delay_exec.h"

#define APPKEY            "25316781"
#define APPSECRET         "00175b033adf64ef0844ab78344a2eb6"
#define AIRBURN_APPKEY    "9678543276"
#define AIRBURN_APPSECRET "KDj84fj1dEiB6sWAyX26fAEhNJpbchr"

static void do_sha256(char *base64_buf, int len, char *hash)
{
    SHA256_CTX sha256;
    unsigned char md[SHA256_DIGEST_LENGTH] = {0};

    SHA256_Init(&sha256);  
    SHA256_Update(&sha256, base64_buf, len);  
    SHA256_Final((unsigned char *)md, &sha256);  

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash+i*2, "%02x", md[i]);
    }
}

static int assemble_header_msg(char *head_buf, const char *appkey, const char *appsecret)
{
    int random = 0;
    int  base64_buf_len  = 0;
    time_t curtime;
    char *str = NULL;
    char sign_str[512] = {0};
    char encrypte[512] = {0};
    char base64_buf[512] = {0};
    cJSON *cjson_head = NULL;
    TripleInfoS triple = {0};

    time(&curtime);
    tencent_load_triple_info(F_P2P_TRIPLE, &triple);
    if (strlen(triple.device_name) == 0) {  // 空中烧录无唯一字符串使用
        strncpy(triple.device_name, "12345678", sizeof(triple.device_name)-1);
    }

    srand((unsigned)time(NULL));
    random = (rand() % 90000000) + 10000000;
    base64_buf_len = sizeof(base64_buf);

    sprintf(sign_str, "appKey=%s&appSecret=%s&clientId=%s&nonce=%d&timestamp=%lld",
        appkey, appsecret, triple.device_name, random, curtime);

    base64encode(base64_buf, &base64_buf_len, sign_str, strlen(sign_str));
    do_sha256(base64_buf, base64_buf_len, encrypte);

    cjson_head = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_head, "appKey"   , appkey);
    cJSON_AddNumberToObject(cjson_head, "nonce"    , random);
    cJSON_AddNumberToObject(cjson_head, "timestamp", curtime);
    cJSON_AddStringToObject(cjson_head, "clientId" , triple.device_name);
    cJSON_AddStringToObject(cjson_head, "sign"     , encrypte);
    str = cJSON_PrintUnformatted(cjson_head);
    if (NULL != str) {
        snprintf(head_buf, HEAD_MAX_SIZE, "%s", str);
        free(str);
    }

    // 清理内存  
    if (cjson_head != NULL) {
        cJSON_Delete(cjson_head);
        cjson_head = NULL;
    }
    return 0;
}

static void add_maps_to_object(cJSON *obj, sFieldMap maps[])
{
    for (int i = 0; maps[i].key != NULL; i++) {
        switch (maps[i].type) {
        case E_FIELD_STRING:
            cJSON_AddStringToObject(obj, maps[i].key, (const char *)maps[i].val);
            break;
        case E_FIELD_INT:
            cJSON_AddNumberToObject(obj, maps[i].key, *(int*)(maps[i].val));
            break;
        default:
            break;  /* 可扩展其他类型 */
        }
    }

    return;
}

static int assemble_airburn_body_msg(char *body_buf, sFieldMap params_fields[])
{
    if (!body_buf) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return -1;
    }

    add_maps_to_object(root, params_fields);

    // 序列化并输出
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        snprintf(body_buf, BODY_MAX_SIZE, "%s", json_str);
        free(json_str);
    }
    DBG("body_buf:%s\n", body_buf);
    cJSON_Delete(root);

    return json_str ? 0 : -1;
}

static int assemble_body_msg(char *body_buf, sFieldMap params_fields[])
{
    if (!body_buf) {
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return -1;
    }

    // 定义 Request 的字段数组
    sFieldMap request_fields[] = {
        {"appVersion"    , E_FIELD_STRING, (void *)P_TYPE_STR},
        {"country"       , E_FIELD_STRING, (void *)"CN"},
        {"appLanguage"   , E_FIELD_STRING, (void *)"zh"},
        {"appPackageName", E_FIELD_STRING, (void *)"com.jco.tenx.viewai"},
        {"phoneModel"    , E_FIELD_STRING, (void *)"iPhone9,1"},
        {"requestId"     , E_FIELD_STRING, (void *)get_fw_ver()},
        {"phoneVersion"  , E_FIELD_STRING, (void *)"15.8.2"},
        {"network"       , E_FIELD_STRING, (void *)"WiFi"},
        {NULL            , E_FIELD_STRING, NULL}  // 结束标记
    };

    // 构建 Params 和 Request
    cJSON *params  = cJSON_AddObjectToObject(root, "Params");
    cJSON *request = cJSON_AddObjectToObject(root, "Request");
    add_maps_to_object(params, params_fields);
    add_maps_to_object(request, request_fields);

    // 序列化并输出
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        snprintf(body_buf, BODY_MAX_SIZE, "%s", json_str);
        free(json_str);
    }
    DBG("body_buf:%s\n", body_buf);
    cJSON_Delete(root);

    return json_str ? 0 : -1;
}

static void hdl_report_reply(void *userdata, const char *body, int bodysize, int isfinal)
{
    sim_4g_t *info = (sim_4g_t *)userdata;
    cJSON *root = NULL;
    DBG("response:[%s]\n", body);

    do {
        char *str = strstr(body,"{");
        if(str == NULL) {
            break;
        }

        root = cJSON_Parse(str);
        cJSON *json_status = cJSON_GetObjectItem(root, "code");
        if (json_status == NULL) {
            ERR("code json is NULL\n");
            break;
        }

        if (json_status->valueint != 200) {
            SYSLOG("code:%d\n", json_status->valueint);
        }

        switch (json_status->valueint) {
        case 200:
            info->SimInfo.report_status = E_REPORT_SUCCESS;
            break;
        case 202:
            info->SimInfo.report_status = E_REPORT_CHANGE_CARD;
            break;
        case 203:
            info->SimInfo.report_status = E_REPORT_IMEI_EXCEPTION;
            break;
        case 204:
            info->SimInfo.report_status = E_REPORT_SIGN_EXCEPTION;
            break;
        case 205:
            info->SimInfo.report_status = E_REPORT_OPERATOR_NOT_MATCH;
            break;
        case 206:
            info->SimInfo.report_status = E_REPORT_CARD_NOT_EXIST;
            break;
        case 207:
            info->SimInfo.report_status = E_REPORT_CARD_USED;
            break;
        default:
            info->SimInfo.report_status = E_REPORT_FAIL;
            break;
        }

        cJSON *json_data = cJSON_GetObjectItem(root, "data");
        if (!json_data || !cJSON_IsObject(json_data)) {
            ERR("data json is NULL\n");
            break;
        }

        // 获取 locationToken
        cJSON *location_token = cJSON_GetObjectItem(json_data, "locationToken");
        if (!location_token || !cJSON_IsString(location_token)) {
            DBG("locationToken error\n");
            break;
        }

        if (location_token->valuestring != NULL &&
            strlen(location_token->valuestring) > 0) {
            strncpy(info->SimInfo.token, location_token->valuestring,
                    sizeof(info->SimInfo.token));
        }
    } while (0);

    if (root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }

    return ;
}

static void hdl_privctrl_reply(void *userdata, const char *body, int bodysize, int isfinal)
{
    if (!body || bodysize <= 0) {
        ERR("Invalid input parameters\n");
        return;
    }

    SysInfoS sysinfo = {{0,},};
    priv_ctrl_t *info = (priv_ctrl_t *)userdata;
    int  type = 0;
    char cpu[64] = {0};
    char custom_appid[8] = {0};
    char modelId[64] = {0};
    char appId[64] = {0};
    char product_key[PRODUCT_KEY_MAXLEN] = {0};
    char device_name[DEVICE_NAME_MAXLEN] = {0};
    char device_secret[DEVICE_SECRET_MAXLEN] = {0};
    cJSON *root = NULL;
    sVideoCallCfg videocall = {0};

    conf_get_sysinfocfg(&sysinfo);
    conf_get_videocall_cfg(&videocall);
    DBG("response:[%.*s]\n", bodysize, body);
    // Find JSON start in response
    const char *json_start = strchr(body, '{');
    if (!json_start) {
      ERR("No JSON data found in response");
      return;
    }

    // Parse JSON
    root = cJSON_Parse(json_start);
    if (!root) {
        ERR("Failed to parse JSON");
        return;
    }

    do {
        cJSON *json_code = cJSON_GetObjectItem(root, "code");
        if (json_code == NULL) {
            ERR("code json is NULL\n");
            break;
        }

        DBG("code:%d\n", json_code->valueint);
        cJSON *json_msg = cJSON_GetObjectItem(root, "msg");
        if (json_msg == NULL) {
            ERR("code json is NULL\n");
            break;
        }

        DBG("msg:%s\n", json_msg->valuestring);
        cJSON *json_data = cJSON_GetObjectItem(root, "data");
        if (json_data == NULL || json_data->type != cJSON_Object) {
            ERR("data json is NULL or not an object\n");
        }

        cJSON *json_video = cJSON_GetObjectItem(json_data, "status");
        if (json_video == NULL) {
            ERR("code json is NULL\n");
            break;
        }
        info->video = json_video->valueint;

        // 通过cpu向平台查询三元组,判断平台绑定的三元组和设备flash三元组是否一致,不一致重新烧录
        struct {
            const char *field_name;
            char *dest_str;
            size_t dest_size;
        } field_mappings[] = {
            {"cpu"         , cpu                , sizeof(cpu)          },
            {"productKey"  , product_key        , sizeof(product_key)  },
            {"deviceName"  , device_name        , sizeof(device_name)  },
            {"devicesecret", device_secret      , sizeof(device_secret)},
            {"customAppId" , custom_appid       , sizeof(custom_appid) },
            {"callModelId" , modelId            , sizeof(modelId)      },
            {"appletAppId" , appId              , sizeof(appId )       },
        };

        for (size_t i = 0; i < sizeof(field_mappings) / sizeof(field_mappings[0]); i++) {
            cJSON *json_item = cJSON_GetObjectItem(json_data, field_mappings[i].field_name);
            if (json_item == NULL) {
                ERR("%s json is NULL\n", field_mappings[i].field_name);
                break;
            }
            strncpy(field_mappings[i].dest_str, json_item->valuestring,
                    field_mappings[i].dest_size - 1);
            // 确保字符串以 null 结尾
            field_mappings[i].dest_str[field_mappings[i].dest_size - 1] = '\0';
        }

        if (!is_okey(F_P2P_TRIPLE)) {
            break;
        }

        TripleInfoS triple = {0,};
        tencent_load_triple_info(F_P2P_TRIPLE, &triple);
        /*
        * 1. cpu未从平台查询到三元组，说明cpu未绑定三元组,发送p2p非法事件
        * 2. cpu从平台查询到的三元组和flash中三元组不一致,覆盖F_P2P_TRIPLE并发送p2p非法事件
        */
        if ((strlen(product_key) == 0 || strlen(device_name) == 0 || strlen(device_secret) == 0)
            || (strcmp(product_key, triple.product_key) != 0
            || strcmp(device_name, triple.device_name) != 0
            || strcmp(device_secret, triple.device_secret) != 0)) {
            SYSLOG("cloud_triple PK:%s DN:%s DS:%s\n", product_key, device_name, device_secret);
            DumpFile2(F_P2P_TRIPLE, "%s;%s;%s;", product_key, device_name, device_secret);
            char id_buf[256] = {0};
            sprintf(id_buf, "%s;%s;%s;", product_key, device_name, device_secret);
            if (uboot_p2pconf_set((char *)"txconf", id_buf) != SUCCESS) {
                LOG("uboot_txconf_set txconf failed\n");
                DBG("uboot_txconf_set txconf failed\n");
            }
            // 上报异常
            type = 1;
            char old_value[64] = {0};
            char new_value[64] = {0};
            snprintf(old_value, sizeof(old_value),"%s@%s", triple.product_key, triple.device_name);
            snprintf(new_value, sizeof(new_value), "%s@%s", product_key, device_name);
            report_exception_record(type, old_value, new_value);
            set_g_stat(tencent, TENCENT_INVALID);
        }

        if (strcmp(modelId, videocall.modelId) != 0 || strcmp(appId, videocall.appId) != 0) {
            char modle_buf[128] = {0};
            char appid_buf[128] = {0};
            sprintf(modle_buf, "1 /cfg/videocall/modelId %s\n", modelId);
            AppendFile(CUSTOM_CONF, modle_buf);
            sprintf(appid_buf, "1 /cfg/videocall/appId %s\n", appId);
            AppendFile(CUSTOM_CONF, appid_buf);
            strncpy(videocall.modelId, modelId, sizeof(videocall.modelId));
            strncpy(videocall.appId, appId, sizeof(videocall.appId));
            conf_set_videocall_cfg(videocall);
        }

        if (strcmp(custom_appid, sysinfo.custom_appid) != 0) {
            WAR("custom_appid: %s, sysinfo.custom_appid: %s\n", custom_appid, sysinfo.custom_appid);
            // 上报异常
            type = 2;
            report_exception_record(type, sysinfo.custom_appid, custom_appid);
            char custom_buf[128] = {0};
            sprintf(custom_buf, "1 /cfg/devinfo/custom_appid %s\n", custom_appid);
            AppendFile(CUSTOM_CONF, custom_buf);
            strncpy(sysinfo.custom_appid, custom_appid, sizeof(custom_appid));
            conf_set_sysinfocfg(sysinfo);

            // appid不一致，更新平台appid，重启设备
            DELAY_REBOOT_LINUX();
        }
    } while(0);

    if (root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }

    return ;
}

static void hdl_airburn_reply(void *userdata, const char *body, int bodysize, int isfinal)
{
    sBurnArg *dev = (sBurnArg *)userdata;
    cJSON *cjson_root = cJSON_Parse(body);

    AppendFile("/tmp/messages", body);
    DBG("response:[%s]\n", body);
    if (cjson_root == NULL) {
        ERR("Failed to parse JSON\n");
        dev->code = 400;
        return;
    }

    cJSON *json_status = cJSON_GetObjectItem(cjson_root, "code");
    if (json_status == NULL || json_status->type != cJSON_Number) {
        ERR("code json is NULL or not a number\n");
        dev->code = 400;
        cJSON_Delete(cjson_root);
        return;
    }

    dev->code = json_status->valueint;
    if (dev->code != 200) {
        SYSLOG("[code=%d]\n", dev->code);
        cJSON_Delete(cjson_root);
        return;
    }

    cJSON *json_data = cJSON_GetObjectItem(cjson_root, "data");
    if (json_data == NULL || json_data->type != cJSON_Object) {
        ERR("data json is NULL or not an object\n");
        dev->code = 400;
        cJSON_Delete(cjson_root);
        return;
    }

    struct {
        const char *field_name;
        char *dest_str;
        size_t dest_size;
    } field_mappings[] = {
        {"deviceId"    , dev->device_id           , sizeof(dev->device_id)           },
        {"mac"         , dev->mac                 , sizeof(dev->mac)                 },
        {"productKey"  , dev->triple.product_key  , sizeof(dev->triple.product_key)  },
        {"deviceName"  , dev->triple.device_name  , sizeof(dev->triple.device_name)  },
        {"devicesecret", dev->triple.device_secret, sizeof(dev->triple.device_secret)},
        {"cpu"         , dev->cpu                 , sizeof(dev->cpu)                 },
    };

    for (size_t i = 0; i < sizeof(field_mappings) / sizeof(field_mappings[0]); i++) {
        cJSON *json_item = cJSON_GetObjectItem(json_data, field_mappings[i].field_name);
        if (json_item == NULL || json_item->type != cJSON_String) {
            ERR("%s json is NULL or not a string\n", field_mappings[i].field_name);
            dev->code = 400;
            cJSON_Delete(cjson_root);
            return;
        }
        strncpy(field_mappings[i].dest_str, json_item->valuestring,
                field_mappings[i].dest_size - 1);
        // 确保字符串以 null 结尾
        field_mappings[i].dest_str[field_mappings[i].dest_size - 1] = '\0';
    }

    cJSON_Delete(cjson_root);
}

static void hdl_twecall_reply(void *userdata, const char *body, int bodysize, int isfinal)
{
#if 0
    cJSON *root = NULL;
    DBG("response:[%s]\n", body);
    char *str = strstr(body,"{");
    do {
        if(str == NULL) {
            return;
        }
        sVideoCallCfg videocall= {0,};
        conf_get_videocall_cfg(&videocall);

        root = cJSON_Parse(str);
        cJSON *json_status = cJSON_GetObjectItem(root, "code");
        if (json_status == NULL) {
            ERR("code json is NULL\n");
        }
        DBG("code:%d\n", json_status->valueint);

        if (json_status->valueint == 200) {
            cJSON *json_data = cJSON_GetObjectItem(root, "data");
            if (json_data == NULL) {
                ERR("data json is NULL\n");
                break;
            } else {
                cJSON *json_openid = cJSON_GetObjectItem(json_data, "openId");
                if (json_openid == NULL) {
                    ERR("code json is NULL\n");
                    break;
                }
                DBG("json_openid:%s\n", json_openid->valuestring);
                if (strlen(json_openid->valuestring) > 0) {
                    strncpy(videocall.openId, json_openid->valuestring, sizeof(videocall.openId));
                }

                cJSON *json_openstatus = cJSON_GetObjectItem(json_data, "openStatus");
                if (json_openstatus == NULL) {
                    ERR("code json is NULL\n");
                    break;
                }
                DBG("openStatus:%d\n", json_openstatus->valueint);
                videocall.openStatus = json_openstatus->valueint;
            }
        }
        conf_set_videocall_cfg(videocall);

        cJSON *json_msg = cJSON_GetObjectItem(root, "msg");
        if (json_msg == NULL) {
            ERR("code json is NULL\n");
        }
        DBG("json_msg:%s\n", json_msg->string);
    } while (0);

    if(root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }

    return ;
#endif
}

static void hdl_reply(void *userdata, const char *body, int bodysize, int isfinal)
{
    cJSON *root = NULL;
    DBG("response:[%s]\n", body);
    char *str = strstr(body,"{");
    do {
        if(str == NULL) {
            return;
        }
        sVideoCallCfg videocall= {0,};
        conf_get_videocall_cfg(&videocall);

        root = cJSON_Parse(str);
        cJSON *json_status = cJSON_GetObjectItem(root, "code");

        if (json_status == NULL) {
            ERR("code json is NULL\n");
            break;
        }
        DBG("code:%d\n", json_status->valueint);

        cJSON *json_msg = cJSON_GetObjectItem(root, "msg");
        if (json_msg == NULL) {
            ERR("code json is NULL\n");
            break;
        }
        DBG("json_msg:%s\n", json_msg->string);
    } while (0);

    if(root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }

    return ;
}

static int post_devinfo(sPostMsg *msg, void *info)
{
    int ret = 0;
    char ip[32] = {0};
    if (get_ip_by_domain(msg->host, ip, sizeof(ip)) == NULL) {
        DBG("get_ip_by_domain fail\n");
        return -1;
    }
    /* 添加头信息 */
    jhttp_client_t *http = jhttp_client_create("https", ip, msg->port);

    jhttp_client_set_header(http, "Content-Type", "application/json");
    jhttp_client_set_header(http, "Signature", msg->head_buf);
    jhttp_client_set_header(http, "Host", msg->host);

    switch (msg->action) {
    case E_ACTION_4G_AIRBURN:
        ret = jhttp_client_post(http, msg->method, msg->body_buf, strlen(msg->body_buf),
                                NULL, hdl_airburn_reply, info, msg->timeoutms);
        break;
    case E_ACTION_DEV_BIND:
    case E_ACTION_DEV_UNBIND:
    case E_ACTION_4G_LOCATION:
    case E_ACTION_ABNORMAL:
    case E_ACTION_SDSTAT:
        ret = jhttp_client_post(http, msg->method, msg->body_buf, strlen(msg->body_buf),
                                NULL, hdl_reply, NULL, msg->timeoutms);
        break;
    case E_ACTION_SIM4G:
        ret = jhttp_client_post(http, msg->method, msg->body_buf, strlen(msg->body_buf),
                                NULL, hdl_report_reply, info, msg->timeoutms);
        break;
    case E_ACTION_PRIVCTRL:
        ret = jhttp_client_post(http, msg->method, msg->body_buf, strlen(msg->body_buf),
                                NULL, hdl_privctrl_reply, info, msg->timeoutms);
        break;
    case E_ACTION_TWECALL:
        ret = jhttp_client_post(http, msg->method, msg->body_buf, strlen(msg->body_buf),
                                NULL, hdl_twecall_reply, NULL, msg->timeoutms);
        break;
    default:
        ret = jhttp_client_post(http, msg->method, msg->body_buf, strlen(msg->body_buf),
                                NULL, hdl_reply, NULL, msg->timeoutms);
        break;
    }

    jhttp_client_destroy(http);

    return ret;
}

/**
 * try_cnt: 重试次数, 每次重试超时时间会增加 3s
 */
int post_devinfo_ex(int try_cnt, sPostMsg *msg, void *info)
{
    int ret = SUCCESS;
    do {
        msg->timeoutms += 3 * 1000;

        ret = post_devinfo(msg, info);
        if (SUCCESS == ret) {
            break;
        } else {
            WAR("post_devinfo fail, method: %s, try_cnt: %d\n", msg->method, try_cnt);
        }
    } while(--try_cnt >= 0);

    return ret;
}

int report_4g_airburn(sim_4g_t *sim4g_info, sBurnArg *dev)
{
    int ret = -1;
    sPostMsg msg = {0};

    assemble_header_msg(msg.head_buf, AIRBURN_APPKEY, AIRBURN_APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"action"   , E_FIELD_STRING, (void *)"GET_ID_BY_DEVICE"},
        {"cpu"      , E_FIELD_STRING, (void *)get_cpuid()},
        {"devType"  , E_FIELD_STRING, (void *)"SPC_4G"},
        {"iccid"    , E_FIELD_STRING, (void *)sim4g_info->SimInfo.iccid},
        {"imei"     , E_FIELD_STRING, (void *)sim4g_info->SimInfo.imei},
        {NULL       , E_FIELD_STRING, NULL}  // 结束标记
    };
    assemble_airburn_body_msg(msg.body_buf, params_fields);
    msg.host = URL_4G_AIRBURN;
    msg.port = 443;
    msg.method = METHOD_API_ACTION;
    msg.timeoutms = 12*1000;
    msg.action = E_ACTION_4G_AIRBURN;
    ret = post_devinfo(&msg, dev);

    return ret;
}

int report_dev_bind(char *token)
{
    int ret = -1;
    sPostMsg msg = {0};
    TripleInfoS triple = {0};

    tencent_load_triple_info(F_P2P_TRIPLE, &triple);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action"    , E_FIELD_STRING, (void *)"BIND_DEVICE_BY_DEVICE"},
        {"productKey", E_FIELD_STRING, (void *)triple.product_key},
        {"deviceName", E_FIELD_STRING, (void *)triple.device_name},
        {"token"     , E_FIELD_STRING, (void *)token},
        {NULL        , E_FIELD_STRING, NULL}  // 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_DEV_BIND;
    // post
    ret = post_devinfo_ex(2, &msg, NULL);

    return ret;

}

int report_dev_unbind(void)
{
    int ret = -1;
    sPostMsg msg = {0};
    TripleInfoS triple = {0};

    tencent_load_triple_info(F_P2P_TRIPLE, &triple);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action",     E_FIELD_STRING, (void *)"UNBIND_DEVICE_BY_DEVICE"},
        {"productKey", E_FIELD_STRING, (void *)triple.product_key},
        {"deviceName", E_FIELD_STRING, (void *)triple.device_name},
        {NULL        , E_FIELD_STRING, NULL}  // 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_DEV_UNBIND;
    // post
    ret = post_devinfo_ex(2, &msg, NULL);

    return ret;
}

int report_twecall(void)
{
    int ret = 0;
    sPostMsg msg = {0};
    TripleInfoS triple = {0};

    tencent_load_triple_info(F_P2P_TRIPLE, &triple);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action",     E_FIELD_STRING, (void *)"QUERY_DEVICE_TWECALL_AUTH_INFO"},
        {"productKey", E_FIELD_STRING, (void *)triple.product_key},
        {"deviceName", E_FIELD_STRING, (void *)triple.device_name},
        {NULL        , E_FIELD_STRING, NULL}  // 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);

    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_TWECALL;
    // post
    ret = post_devinfo(&msg, NULL);

    return ret;
}

int report_4g_info(sim_4g_t * info)
{
    int ret = -1;
    int vendor = 0;
    sPostMsg msg = {0};
    TripleInfoS triple = {0,};

    // 平台根据模块类型下发指定的访问 4G 基站定位服务器密钥。为了兼容之前版本，默认下发移远密钥
    if (info->model_type == E_YGX09) {
        vendor = 2; // 域格
    } else {
        vendor = 1; // 移远
    }
    tencent_load_triple_info(F_P2P_TRIPLE, &triple);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action"    , E_FIELD_STRING, (void *)"UPLOAD_VIRTUAL_SIM_INFO"},
        {"productKey", E_FIELD_STRING, (void *)triple.product_key},
        {"deviceName", E_FIELD_STRING, (void *)triple.device_name},
        {"iccid"     , E_FIELD_STRING, (void *)info->SimInfo.iccid},
        {"iccid1"    , E_FIELD_STRING, (void *)info->SimInfo.esim_card},
        {"iccid2"    , E_FIELD_STRING, (void *)info->SimInfo.sim_card},
        {"imei"      , E_FIELD_STRING, (void *)info->SimInfo.imei},
        {"vendor"    , E_FIELD_INT   , &vendor},
        {NULL        , E_FIELD_STRING, NULL}  // 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_4G_REPORT;
    msg.port = 9521;
    msg.method = METHOD_API_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_SIM4G;
    ret = post_devinfo(&msg, info);

    return ret;
}

int report_4g_location(Sim4g *sim4g_info)
{
    int ret = 0;
    sPostMsg msg = {0};
    TripleInfoS triple = {0};

    tencent_load_triple_info(F_P2P_TRIPLE, &triple);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    sFieldMap params_fields[] = {
        {"Action"    , E_FIELD_STRING, (void *)"REPORT_DEVICE_LOCATION"},
        {"productKey", E_FIELD_STRING, (void *)triple.product_key},
        {"deviceName", E_FIELD_STRING, (void *)triple.device_name},
        {"iccid"     , E_FIELD_STRING, (void *)sim4g_info->iccid},
        {"imei"      , E_FIELD_STRING, (void *)sim4g_info->imei},
        {"longitude" , E_FIELD_STRING, (void *)sim4g_info->longitude},
        {"latitude"  , E_FIELD_STRING, (void *)sim4g_info->latitude},
        {NULL        , E_FIELD_STRING, NULL}  // 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_4G_LOCATION;
    ret = post_devinfo(&msg, sim4g_info);

    return ret;
}

int report_privctrl(priv_ctrl_t info)
{
    int ret = 0;
    sPostMsg msg = {0};
    SysInfoS version = {0};

    conf_get_sysinfocfg(&version);

    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action"       , E_FIELD_STRING, (void *)"QUERY_DEVICE_INFO_BY_DEVICE" },
        {"cpu"          , E_FIELD_STRING, (void *)get_cpuid()                   },
        {"deviceVersion", E_FIELD_STRING, (void *)version.serverver             },
        {"productModel" , E_FIELD_STRING, (void *)version.devtype               },
        {NULL           , E_FIELD_STRING, NULL                                  }  // 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_PRIVCTRL;
    ret = post_devinfo(&msg, &info);

    return ret;
}

int report_exception_record(int type, char *old_value, char *new_value)
{
    int ret = -1;
    sPostMsg msg = {0};
    TripleInfoS tx_info = {0,};

    tencent_load_triple_info(F_P2P_TRIPLE , &tx_info);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action"   , E_FIELD_STRING, (void *)"UPLOAD_DEVICE_EXCEPTION_RECORD"},
        {"cpu"      , E_FIELD_STRING, (void *)get_cpuid()},
        {"type"     , E_FIELD_INT   , &type},
        {"oldValue" , E_FIELD_STRING, (void *)old_value},
        {"newValue" , E_FIELD_STRING, (void *)new_value},
        {NULL       , E_FIELD_STRING, NULL}  // 结束标记
    };

    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_ABNORMAL;
    ret = post_devinfo(&msg, NULL);

    return ret;
}

/* 重点参数，要参考与平台的对接文档
 *
 * appKey
 * appSecret
 * host
 * port
 * method 
 *
 * action 从 https://alidocs.dingtalk.com/i/nodes/14lgGw3P8vaywaaLI7da2ag4J5daZ90D
 *
 **/
int report_sd_stat()
{
    int ret = -1;
    sPostMsg msg = {0};
    sSdinfoPkg sdinfo = {0};

    get_sdinfo_pkg(&sdinfo);
    assemble_header_msg(msg.head_buf, APPKEY, APPSECRET);
    // 定义 Params 的字段数组
    sFieldMap params_fields[] = {
        {"Action"   , E_FIELD_STRING, (void *)"UPLOAD_SD_EXCEPTION_RECORD"},
        {"cpuid"    , E_FIELD_STRING, (void *)sdinfo.cpuid                },
        {"cid"      , E_FIELD_STRING, (void *)sdinfo.cid                  },
        {"uptime"   , E_FIELD_INT   , (void *)&sdinfo.uptime              },
        {"version"  , E_FIELD_STRING, (void *)sdinfo.version              },
        {"sdStat"   , E_FIELD_INT   , (void *)&sdinfo.sd_stat             },
        {"temprt"   , E_FIELD_INT   , (void *)&sdinfo.temprt              },
        {"thisBoot" , E_FIELD_INT   , (void *)&sdinfo.this_boot           },
        {"dfUse"    , E_FIELD_INT   , (void *)&sdinfo.df_used             },
        {"cntRescan", E_FIELD_INT   , (void *)&sdinfo.cnt_rescan          },
        {"isTest"   , E_FIELD_INT   , (void *)&sdinfo.is_test             },
        {"allCount" , E_FIELD_INT   , (void *)&sdinfo.all_count           },
        {NULL       , E_FIELD_STRING, NULL                                }// 结束标记
    };
    assemble_body_msg(msg.body_buf, params_fields);
    msg.host = URL_PROD;
    msg.port = 443;
    msg.method = METHOD_TOOL_DEV;
    msg.timeoutms = 10*1000;
    msg.action = E_ACTION_SDSTAT;
    ret = post_devinfo(&msg, NULL);

    return ret;
}
#endif
