#ifdef PLATFORM_TENCENT
#include <errno.h>
#include <assert.h>

#include "utils.h"
#include "debug.h"
#include "js_scheduler.h"
#include "jcpService.h"
#include "jconfig.h"
#include "confapi.h"
#include "net_config.h"
#include "g_sys.h"
#include "net_check.h"
#include "cJSON.h"
#include "factory_db.h"
#include "utils.h"
#include "base64.h"
#include "encode_audio_queue.h"
#include "sim4g.h"
#include "airlink.h"

#include "iv_usrex.h"
#include "iv_cm.h"
#include "iv_dm.h"
#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "tencent_model.h"
#include "tencent_server.h"

#define AMR_AUDIO_TMP_PATH  "/tmp/custom.amr"

#define JCPCMD_MATCHED(cmd) (0 == strncmp(jcp_cmd, cmd, strlen(cmd)))

void tencent_report_event(int eventType, int channel, char *payload)
{
    ivm_lock();
    g_ivm_objs.Event.m_uploadDeviceEvent.m_eventType = eventType;
    g_ivm_objs.Event.m_uploadDeviceEvent.m_channel = channel;
    strcpy(g_ivm_objs.Event.m_uploadDeviceEvent.m_payload, payload);
    iv_dm_event_report("uploadDeviceEvent"); //这里填截图中的event_name
    DBG("iv_dm_event_report\n");
    ivm_unlock();
}

void tencent_report_attr(int status)
{
    DBG("status:%d\n", status);
    ivm_ProReadonly_setInt(CallStatus, status);
}

static int tencent_write_amr_file(char *jcp_cmd, char *resp_buf, int resp_len)
{
    int ret = FAILURE;
    char *p = NULL;

    static FILE *s_amr_fp = NULL;
    static int s_file_size = 0;
    static int s_recv_size = -1;

    do {
        if (JCPCMD_MATCHED("AMR1.0=start")) {
            p = strstr(jcp_cmd, "file_size=");
            if (NULL == p) {
                ERR("not find amr file size\n");
                break;
            }

            p += strlen("file_size=");
            int file_size = atoi(p);
            if (file_size <= 0) {
                ERR("src amr file size error:%d\n", file_size);
                break;
            }
            s_file_size = file_size;
            s_recv_size = 0;

            if (NULL != s_amr_fp) {
                fclose(s_amr_fp);
                s_amr_fp = NULL;
                remove(AMR_AUDIO_TMP_PATH);
            }

            s_amr_fp = fopen(AMR_AUDIO_TMP_PATH, "wb");
            if (s_amr_fp == NULL) {
                ERR("open amr file:%s fail\n", AMR_AUDIO_TMP_PATH);
                break;
            }

            ret = SUCCESS;
            DBG("AMR1.0 start receive file size:%d\n", s_file_size);
        } else if (JCPCMD_MATCHED("AMR1.0=seq")) {
            if (NULL == s_amr_fp) {
                break;
            }

            char amr_buf[2*1024] = {0};
            int real_size = 0;

            int head_len = strlen("AMR1.0=seq file_seq=000");
            base64decode(amr_buf, &real_size, (char *)(jcp_cmd+head_len), strlen(jcp_cmd)-head_len);
            int write_size = system_fwrite(amr_buf, 1, real_size, s_amr_fp);
            if (write_size != real_size) {
                ERR("write arm file fail write size:%d real size:%d\n", write_size, real_size);
                break;
            }

            s_recv_size += real_size;
            ret = SUCCESS;
            DBG("AMR1.0 receiving, decode size:%d\n", real_size);
        } else {
            DBG("AMR1.0 end receive recv_size:%d, real_size:%d\n", s_recv_size, s_file_size);
            if (NULL == s_amr_fp) {
                break;
            }

            if (s_recv_size == s_file_size) {
                fsync(fileno(s_amr_fp));
                fclose(s_amr_fp);
                s_amr_fp = NULL;
                sync();

                char *path = NULL;
                ret = get_amr_path_from_alarm_type(AUDIO_ALARM_CUSTOM, &path);
                if (SUCCESS == ret) {
                    CopyFile(path, AMR_AUDIO_TMP_PATH);
                    remove(AMR_AUDIO_TMP_PATH);
                } else {
                    ERR("get alarm type error\n");
                }
            } else {
                fclose(s_amr_fp);
                s_amr_fp = NULL;
                remove(AMR_AUDIO_TMP_PATH);
            }

            s_recv_size = 0;
            s_file_size = 0;
        }
    } while(0);

    snprintf(resp_buf, resp_len, "[%s]", ret == SUCCESS ? "Success" : "Error");
    return ret;
}

static int tencent_recv_amr(char *jcp_cmd, char *resp_buf, int resp_len)
{
    int ret = -1;

    if (JCPCMD_MATCHED("AMR1.0=start") || JCPCMD_MATCHED("AMR1.0=seq") || JCPCMD_MATCHED("AMR1.0=end")) {
        ret = tencent_write_amr_file(jcp_cmd, resp_buf, resp_len);
    } else if (JCPCMD_MATCHED("AMR1.0=play")) {
        ret = encode_audio_queue_push_amr(AUDIO_ALARM_OTHER, TRUE);
    } else if (JCPCMD_MATCHED("AMR1.0=default")) {
        ret = UtilSystemCmd("mv /opt/custom/def_other.amr /opt/custom/other.amr");
    }

    return ret;
}

/* app 获取网络配置，优先返回有线数据，未连接有线则返回无线数据 */
static void tencent_request_get_usbnet(char *resp_buf, int resp_len)
{
    int i= 0 ;
    int port = 0;
    int count = 0;
    char gate_net[32] = {0};
    InterfaceInfoS interfaceInfo[4];

    NetWifiS wifi = {0};
    NetEthS ethcfg = {{0,}};

    conf_get_httpportcfg(&port);
    conf_get_wificfg(&wifi);
    conf_get_ethcfg(&ethcfg);
    memset(interfaceInfo, 0, sizeof(interfaceInfo));
    net_get_interface_running_info(interfaceInfo, &count);
    net_set_interface_info(&interfaceInfo[0]);

    if (get_g_sys(usb_4g)) {
        get_dev_gateway("usb0", gate_net);
        net_get_ipaddr("usb0", wifi.ip,sizeof(wifi.ip));
    } else {
        get_dev_gateway("wlan0", gate_net);
    }

    for (i = 0; i < count; i++) {
        if (strcmp(interfaceInfo[i].nic, wifi.nic) == 0) {
            strcpy(wifi.ip, interfaceInfo[i].ip);
            strcpy(wifi.mask, interfaceInfo[i].mask);
            break;
        }
    }

    snprintf(resp_buf, resp_len, "[Success]nic=eth0;ethip=%s;ethmask=%s;ethgw=%s;ethdhcp=%d;ipadaen=0;dns=%s;ethmac=%s;ipcheck=0;web=%d",
    wifi.ip, wifi.mask, gate_net, wifi.dhcp, ethcfg.dns, wifi.mac, port);

    return;
}

static void tencent_exec_jcp_cmd(char *jcp_cmd, char *resp_buf, int resp_len)
{
    if (strlen(jcp_cmd) <= 0) {
        return;
    }

    if (JCPCMD_MATCHED("AMR1.0")) {
        tencent_recv_amr(jcp_cmd, resp_buf, resp_len);
    } else if (!is_tencent_eth0_linked() && JCPCMD_MATCHED("ethcfg -act list")) {
        tencent_request_get_usbnet(resp_buf, resp_len);
    } else {
        jcpcmd_sendrecv(jcp_cmd, resp_buf, resp_len);
    }
}

//JCP物模型对应的回调
int iv_usrcb_Action_DataServicesByJCP(ivm_DataServicesByJCP_t *DataServicesByJCP)
{
    //注意: 回调函数中,不能做阻塞式操作,不得做耗时的操作。会导致核心通讯线程阻塞!!!
    DBG("recv data = [%s]\n", DataServicesByJCP->action_in.m_jcpParams);

    char jcp_param[JCP_MAX_LEN] = {0};
    char resp_buf[MAX_MODEL_BUFSIZE] = {0};

    memcpy(jcp_param, DataServicesByJCP->action_in.m_jcpParams, sizeof(jcp_param));

    tencent_exec_jcp_cmd(jcp_param, resp_buf, sizeof(resp_buf));

    //返回结果里有\r\n，会影响腾讯SDK组装JSON格式
    char *ptr = strstr(resp_buf+(strlen(resp_buf) - strlen("\r\n")),"\r\n");
    if(NULL != ptr){
        resp_buf[strlen(resp_buf) - strlen("\r\n")] = '\0';
    }

    DBG("strlen(resp_buf)=%d\n", strlen(resp_buf));
    memcpy(DataServicesByJCP->action_out.m_result, resp_buf, sizeof(DataServicesByJCP->action_out.m_result));
    DBG("result = [%s]\n", DataServicesByJCP->action_out.m_result);

    return 0;
}

int iv_usrcb_ProWritable_record_enable(const TYPE_DEF_TEMPLATE_BOOL *record_enable)
{
    //User implementation code
    //注意: 回调函数中,不能做阻塞式操作,不得做耗时的操作。会导致核心通讯线程阻塞!!!
    return 0;
}

int iv_usrcb_ProWritable_SensorNum(const TYPE_DEF_TEMPLATE_INT *sensor_num)
{
    //User implementation code
    //注意: 回调函数中,不能做阻塞式操作,不得做耗时的操作。会导致核心通讯线程阻塞!!!

    return 0;
}

/*
 *物模型增加步骤：
 * 1、在云平台上建立好物模型后，点击左上角的“查看物模型JSON”，
 *    复制好JSON数据后在SDK的tools目录下新建一个json文件，将数据保存到json文件中
 * 2、在Linux下执行./codegen.py -c xxx.json，会生成3个文件，文件里就是物模型初始化的示例代码
 * 注意事项:
 * iv_usrex.c:用户物模型实现源文件,实现物模型初始化相关的代码,开发者不能修改这个文件
 * iv_usrex.h:用户物模型定义头文件,定义相关数据结构,开发者不能修改这个文件
 */
int tencent_model_init(void)
{
    int ret = 0;
    int location_4g = 0;
    Sim4g sim4g_info = {0,};
    ret = sim4g_get_stat(&sim4g_info);
    location_4g = sim4g_info.location_enable;
    DBG("tencent_model_init \n");

    //物模型初始化参数回调
    iv_dm_init_parm_s stInitParm;
    stInitParm.iv_dm_env_init_cb = ivm_env_init;
    //物模型初始化
    ret = iv_dm_init(&stInitParm);
    if (ret < 0) {
        ERR("model init failed!");
        return ret;
    }
    // 属性初值上报
    if (get_g_sys(usb_4g)) {
        ivm_ProReadonly_setInt(location_4g, location_4g);
    }
    ivm_ProWritable_setInt(record_enable, 0);

    return ret;
}

void tencent_model_uninit(void)
{
    iv_dm_exit();
}

#endif //PLATFORM_TENCENT

