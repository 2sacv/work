#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>

#include "debug.h"
#include "utils.h"
#include "confapi.h"
#include "airlink.h"
#include "g_sys.h"
#include "cJSON.h"
#include "factory_db.h"
#include "ble_uuid_server.h"
#include "ble_services.h"
#include "net_check.h"
#include "system_ctrl.h"

#ifdef PLATFORM_TENCENT
#include "tencent_param_conf.h"
#endif

int send_pk_dn_to_app(uint16_t att_hdl)
{
    cJSON *root = cJSON_CreateObject();
    if (NULL == root) {
        ERR("cJSON_CreateObject failed\n");
        return -1;
    }

    TripleInfoS ptriple = {0};
#if defined(PLATFORM_TENCENT)
    tencent_load_triple_info(F_P2P_TRIPLE , &ptriple);
#endif
    cJSON_AddStringToObject(root, "pk", ptriple.product_key);
    cJSON_AddStringToObject(root, "dn", ptriple.device_name);

    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    DBG("json: %s, strlen(json) = %d\n", json, strlen(json));
    ble_uuid_server_send_report_by_handle(att_hdl, (uint8_t*)json, strlen(json));

    cJSON_Delete(root);
    root = NULL;
    return 0;
}

static int get_wifi_json_data(char *recv_buf, NetWifiS *wifi_info)
{
    cJSON *cjson_root = NULL;

    char *str = strstr(recv_buf, "{");
    if(str == NULL){
        goto __err_exit;
    }

    /* json 解析 */
    cjson_root = cJSON_Parse(str);
    if (NULL == cjson_root) {
        ERR("root json is NULL\n");
        goto __err_exit;
    }

    cJSON *json_ssid = cJSON_GetObjectItem(cjson_root, "ssid");
    if (json_ssid == NULL || json_ssid->type != cJSON_String) {
        ERR("code json is NULL or no String\n");
        goto __err_exit;
    } else {
        strncpy(wifi_info->ssid, json_ssid->valuestring, sizeof(wifi_info->ssid));
    }

    cJSON *json_passwd = cJSON_GetObjectItem(cjson_root, "password");
    if (json_passwd == NULL || json_passwd->type != cJSON_String) {
        ERR("code json is NULL or no String\n");
        goto __err_exit;
    } else {
        strncpy(wifi_info->weppasswd, json_passwd->valuestring, sizeof(wifi_info->weppasswd));
    }

    cJSON *json_token = cJSON_GetObjectItem(cjson_root, "token");
    if (json_token == NULL || json_token->type != cJSON_String) {
        ERR("code json is NULL or no String\n");
        goto __err_exit;
    } else {
        strncpy(wifi_info->token, json_token->valuestring, sizeof(wifi_info->token));
    }

    wifi_info->mode = WifiModeE_AP_STATION;
    set_g_stat(tencent, TENCENT_BAND);
    if (cjson_root != NULL) {
        cJSON_Delete(cjson_root);
        cjson_root = NULL;
    }

    return SUCCESS;
__err_exit:
    if (cjson_root != NULL) {
        cJSON_Delete(cjson_root);
        cjson_root = NULL;
    }

    return FAILURE;
}

int ble_parse_rx_data(char *recv_buf, uint16_t att_hdl)
{
    int ret = 0;
    NetWifiS wifi_info = {0,};

    if (recv_buf == NULL || strlen(recv_buf) <= 0) {
        ERR("recv_buf err %s\n", recv_buf);
        return -1;
    }

    if (!(strstr(recv_buf, "ssid\":")  && strstr(recv_buf, "password\":"))) {
        DBG("ssid or password not exist recv_buf: %s", recv_buf);
        return -1;
    }

    conf_get_wificfg(&wifi_info);
    ret = get_wifi_json_data(recv_buf, &wifi_info);
    if (ERRCODE_BT_SUCCESS == ret) {
        send_pk_dn_to_app(att_hdl);
        conf_set_wificfg(wifi_info);
    } else {
        ERR("parse json error\n");
    }

    return 0;
}

int ble_services_start(void)
{
#if __WIFI__
    if (!get_g_sys(usb_wifi)) {
        return 0;
    }

    DevConfS devconf = {0};
    conf_get_devconf_cfg(&devconf);

    ble_uuid_server_deinit();

    if (!get_g_run(wifi, RUN_BLE_INIT)) {    //调试模式下不检查，直接起蓝牙
        if (!is_okey(F_BLE)) {
            SYSLOG("Module does not support Bluetooth\n");
            return 0;
        }

        if (get_g_sys(factest)) {   // 工厂模式不开蓝牙
            return 0;
        }

        if (system_get_security() != TRUE) {    // 无dev_id不开蓝牙
            return 0;
        }

        if (is_okey(SUPPLICANT_OK_CONF) || is_okey(FACTORY_SSID_FILE)) {
            DBG("%s is exist\n", SUPPLICANT_OK_CONF);
            return 0;
        }

        if ((net_link_status("eth0") == 1)) {
            DBG("eth0 is run\n");
            return 0;
        }
    }

    ble_uuid_server_init();
    DBG("ble_uuid_server_init\n");
#endif
    return 0;
}

void ble_services_stop(void)
{
#if __WIFI__
    if (!get_g_sys(usb_wifi)) {
        return;
    }

    ble_uuid_server_deinit();
#endif
}

