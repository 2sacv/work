/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jconfig.cpp
 * @Created Time : 2013-10-15
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <syslog.h>

#include "BasicUsageEnvironment.hh"
#include "mxml.h"

#include "conf_list.h"
#include "jconfig.h"
#include "jconfstruct.h"
#include "confutils.h"
#include "confextcb.h"
#include "jevent.h"

#include "debug.h"
#include "time_call.h"
#include "utils.h"
#include "conftypedef.h"
#include "confxmlargparser.h"
#include "jcpService.h"
#include "net_config.h"
#include "delay_exec.h"
#include "search_service.h"
#include "logapi.h"

#include "conf_nand.h"
#include "conf_list.h"
#include "upnp.h"
#include "ddnsstrategy.h"
#include "encodeapi.h"
#include "system_main.h"
#include "net_check.h"
#include "js_scheduler.h"
#include "ptz_ctrl.h"
#include "ptz_follow.h"
#include "factory_db.h"
#include "soft_name_def.h"
#include "confapi.h"
#include "cJSON.h"
#include "airlink.h"
#include "sim4g.h"
#include "sim4g_common_api.h"

#include "g_sys.h"
#include "g_log.h"
#include "g_run.h"
#include "system_ctrl.h"
#include "encode_common.h"
#include "time_config.h"
#include "shm_buf_pool.h"
#include "encode_audio_input.h"

/*
命名规则
conf_get_ethcfg
    --  提交的api接口名（全小写）
        conf  + get(set) + 配置名cfg，以‘_’连接，
        配置名cfg部分没有下划线
"getEthCfg"
    --  jconfig内部命令
        get(set) + 配置名cfg，以配置头字母及Cfg组成

handleEthCfg
    --  jconf入口函数
        把jconfig命令中get(set)换成handle

SysConfEthCfg
    --  配置文件访问函数
        把jconfig命令中get(set)换成SysConf

JEvent_EthCfgChg
    -- JEventType枚举成员
        把jconfig命令中get(set)换成 JEvent_，末尾再加Chg
*/

static JSTCHandle g_config_sync_handle = NULL;
static mxml_node_t *g_root = NULL;
static pthread_mutex_t g_mutex;
static int g_setflag = 0;
static int g_current_language = 0;
static JSEventManager *g_p_conf = NULL;

int array_sorted_vesize[] = {
    VencSizeE_QCIF,         // QCIF
    VencSizeE_180P,         // QQ720P
    VencSizeE_QVGA,         // QVGA
    VencSizeE_CIF,          // CIF
    VencSizeE_360P,         // Q720P
    VencSizeE_VGA,          // VGA
    VencSizeE_D1,           // D1
    VencSizeE_720P,         // 720P
    VencSizeE_960P,         // 960P
    VencSizeE_UVGA,         // UVGA
    VencSizeE_1080P,        // 1080P
    VencSizeE_3M,           // 3M
    VencSizeE_4M,           // 4M
    VencSizeE_5M,
    vencSizeE_4M_Dahua,
    VencSizeE_2M_3M,
    VencSizeE_8M,
};

JSEventManager *get_ev_mng_conf(void)
{
    return g_p_conf;
}

static int get_index_of_vesize(VencSizeE vencsize)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(array_sorted_vesize); i++) {

        if (vencsize == array_sorted_vesize[i]) {
            return i;
        }
    }
    return -1;
}

static void conf_lock()
{
    pthread_mutex_lock(&g_mutex);
}
static void conf_unlock()
{
    pthread_mutex_unlock(&g_mutex);
}

static void syncXmlToFile();

static void confSyncHanding(void *data);

#define CONF_XML      "/opt/conf/config.xml"
#define CONF_XML_TMP  "/opt/conf/config.xml.tmp"

#define XML_HEAD    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"

#define RETURN_FAIL_IF_XML_ERR(ret)                             \
                do {                                            \
                    if (ret != SUCCESS)                         \
                    {                                           \
                        ERR("SysConf err : %s\n", __FUNCTION__);\
                        return FAILURE;                         \
                    }                                           \
                } while(0)

#define RETURN_SUCC_IF_MEM_EQ(s1, s2, n)                            \
                    do {                                            \
                        if (0 == memcmp(s1, s2, n))                 \
                        {                                           \
                            DBG("cmdline & inner are the same!\n"); \
                            return SUCCESS;                         \
                        }                                           \
                    } while(0)


#define RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer) \
    do {                                                \
        if(NULL != data)                                \
        {                                               \
            bzero(&outer, sizeof(outer));               \
            memcpy(&outer, data, sizeof(outer));        \
            if(confParserStructValue_t(opts) != SUCCESS)\
            {                                           \
                ERR("Parser Arg_T error!\n");           \
                return FAILURE;                         \
            }                                           \
        }                                               \
    }while(0)

#define JCONF_GET_STRUCT_T()                            \
        do {                                            \
            if((int)sizeof(inner) > bufSize)            \
            {                                           \
                return FAILURE;                         \
            }                                           \
                                                        \
            memcpy(buf, &inner, sizeof(inner));         \
            if (bufLen != NULL) *bufLen = sizeof(inner);\
    }while(0)

#define JCONF_STRUCT_PARSER_CALLOC()                   \
    do {                                               \
        if(NULL != data)                               \
        {                                              \
            memcpy(&inner, data, sizeof(inner));       \
            if(confParserStructValue(opts) != SUCCESS) \
            {                                          \
                ERR("Parser Arg error!\n");            \
                FREE_EXT_STRUCT();                     \
                return FAILURE;                        \
            }                                          \
        }                                              \
    }while(0)

#define JCONF_GET_STRUCT_CALLOC()                      \
        do {                                           \
            if((int)sizeof(geter) > bufSize)           \
            {                                          \
                FREE_EXT_STRUCT();                     \
                return FAILURE;                        \
            }                                          \
                                                       \
            memcpy(buf, &geter, sizeof(geter));        \
            if(bufLen != NULL) *bufLen = sizeof(geter);\
    }while(0)

#define RETURN_FAIL_IF_XML_ERR_CALLOC(ret)              \
        do {                                            \
            if (ret != SUCCESS)                         \
            {                                           \
                ERR("SysConf err : %s\n", __FUNCTION__);\
                FREE_EXT_STRUCT();                      \
                return FAILURE;                         \
            }                                           \
        } while(0)

#define RETURN_SUCC_IF_MEM_EQ_CALLOC(s1, s2, n)         \
        do {                                            \
            if (0 == memcmp(s1, s2, n))                 \
            {                                           \
                DBG("cmdline & inner are the same!\n"); \
                FREE_EXT_STRUCT();                      \
                return SUCCESS;                         \
            }                                           \
        } while(0)

#define CALLOC_EXT_STRUCT(type, ext, size, soure)                 \
        do                                                        \
        {                                                         \
            ext = (type*)calloc(size + 1, sizeof(type));          \
            for(int i=0; i < size; i++)                           \
            {                                                     \
                memcpy(&ext[i], &soure[0], sizeof(soure[0]));     \
            }                                                     \
            memcpy(&ext[size], &soure[1], sizeof(soure[1]));      \
        }                                                         \
        while(0)

#define FREE_EXT_STRUCT()             \
        do                            \
        {                             \
            free(extOpts);            \
        }                             \
        while(0)

#define FREE_EXT_MAPKEY_STRUCT()      \
        do                            \
        {                             \
            free(extMap);             \
        }                             \
        while(0)

#define JCONF_CPY_DATA2OUTER(opts, data, outer) \
    do {                                                \
        if(NULL != data)                                \
        {                                               \
            bzero(&outer, sizeof(outer));               \
            memcpy(&outer, data, sizeof(outer));        \
            if(confParserStructValue_t(opts) != SUCCESS)\
            {                                           \
                ERR("Parser Arg_T error!\n");           \
                return FAILURE;                         \
            }                                           \
        }                                               \
    }while(0)

#define CPY_INNER2BUF(inner, buf)     do {              \
            if((int)sizeof(inner) > bufSize) {          \
                return FAILURE;                         \
            }                                           \
            memcpy(buf, &inner, sizeof(inner));         \
            if (bufLen != NULL) *bufLen = sizeof(inner);\
    }while(0)

#define CPY_DATA2OUTER(data, outer) do {               \
        if(NULL != data) {                             \
            memcpy(&outer, data, sizeof(outer));       \
            if(confParserStructValue(opts) != SUCCESS){\
                ERR("Parser Arg error!\n");            \
                FREE_EXT_STRUCT();                     \
                return FAILURE;                        \
            }                                          \
        }                                              \
    }while(0)

#define CPY_GETER2BUF(geter, buf) do {                 \
            if((int)sizeof(geter) > bufSize) {         \
                FREE_EXT_STRUCT();                     \
                return FAILURE;                        \
            }                                          \
            memcpy(buf, &geter, sizeof(geter));        \
            if(bufLen != NULL) *bufLen = sizeof(geter);\
    }while(0)


#define SERVER_VER_MAJOR    1
#define SERVER_VER_MINOR    0
#define SERVER_VER_LITTLE   10

#define L_VERSION    01

#define STR1(R) #R
#define STR2(R) STR1(R)
#define SERVER_BUILD_DATE   COMPILE_DATE

#if 0
static int get_complier_timer(void)
{
    char buf[128] = {0};
    int time = 0;
    // 10:31:29
    sprintf(buf,"%s",__TIME__);
    time = atoi(buf)*100 + atoi(buf+3);
    return time;
}
#endif

const char *get_web_ver(const char *pltfm, const char *devtype, const char *ui, char *version)
{
    const char *fmt_ver = "%s%d.%d.%s.%s-%s%s-%s";
    char rollback[256] = {0};
    size_t nLen = 0;

    if (is_okey(F_UPGRADE_ROLLBACK)) {
        LoadFile(F_UPGRADE_ROLLBACK, rollback, sizeof(rollback));
        nLen = sprintf(version, "%s", rollback);
    } else {
        nLen = sprintf(
                version,
                fmt_ver,
                is_test_ver() ? "YTJT" : "YTJ",
                SERVER_VER_MAJOR,
                SERVER_VER_MINOR,
                system_get_product_name(devtype),
                COMPILE_DTTM,
                get_g_sys(usb_4g)? "DUAL4G-" : "",
                pltfm,
                ui
                );
        printf("verdttm " COMPILE_PKTM " len: %u\n", nLen);
    }
    return version;
}

const char *get_fw_ver()
{
    static char ver[64] = {0};
    if (ver[0]) {
        return ver;
    }

    SysInfoS sysinfo = {{0}};
    conf_get_sysinfocfg(&sysinfo);

    snprintf(ver, sizeof(ver)-1, "YTJ%s", sysinfo.serverver + 3 + is_test_ver());

    SYSLOG("OTA version: %s\n", ver);

    return ver;
}

/*
*  函数名称： get_value_from_cjson    作用：解析JSCON格式字符串获取匹配项数值
*  参数说明： char *cjsondate         JSCON格式源字符串
*             char*keystr             待匹配项字符串常量
*             void *keyvalue          匹配项的数值，出参
*             unsigned int strvalue_len 如果匹配项的数值是字符串类型，则需要本参数描述最大长度，用于合法性检查
*   返回值：   -1 函数执行错误，具体错误类型见串口日志
*              0  函数执行成功
*/
int get_value_from_cjson(char *cjsondate, char*keystr, void *keyvalue,unsigned int strvalue_len)
{
    int ret = 0;
    char *pckeyvalue = NULL;
    int  *pikeyvalue = NULL;
    cJSON *cjson_root = NULL;
    cJSON *cjson_obj  = NULL;

    if ((NULL == cjsondate) || (NULL == keystr) || (NULL == keyvalue)){
        ERR("param error!\n");
        return -1;
    }

    if (strstr(cjsondate, "{") == NULL) {
        ERR("is not json\n");
        return -1;
    }

    cjson_root = cJSON_Parse(cjsondate);
    if (cjson_root == NULL) {
        ERR("code json root is NULL\n");
        return -1;
    }

    cjson_obj  = cJSON_GetObjectItem(cjson_root, keystr);
    if (cjson_obj == NULL) {
        ERR("code json obj is NULL\n");
        cJSON_Delete(cjson_root);
        return -1;
    }

    if ((cjson_obj->type == cJSON_String) || (cjson_obj->type == cJSON_Raw)){
        if (NULL ==  cjson_obj->valuestring){
            ERR("code obj is invalid\n");
            cJSON_Delete(cjson_root);
            return -1;
        }

        if (strvalue_len <= strlen(cjson_obj->valuestring)){
            ERR("code json is too long\n");
            cJSON_Delete(cjson_root);
            return -1;
        }
        pckeyvalue = (char *)keyvalue;
        strcpy(pckeyvalue, cjson_obj->valuestring);
        pckeyvalue[strvalue_len-1] = '\0';
        DBG("json get obj %s: %s\n", keystr,pckeyvalue);
    }else if(cjson_obj->type == cJSON_Number){
        pikeyvalue = (int *)keyvalue;
        *pikeyvalue = cjson_obj->valueint;
        DBG("json get obj %s: %d\n", keystr,*pikeyvalue);
    }else{
        DBG("is not jscon formate or unknown formate\n");
    }

    cJSON_Delete(cjson_root);
    return ret;
}


// 事件发生，不再提供相应消息，而是：
// 1. eventname     枚举名
// 2. eventtype     枚举值
// 3. 使用同名的宏以简化使用
void send_conf_data(JSEventType jevent, void *ptr, int size)
{
    if (jevent <= JEvent_Begin || jevent >= JEvent_End) {
        ERR("invalid jevent %d\n", jevent);
        return;
    }

    return js_event_send(g_p_conf, jevent, ptr, size);
}


void send_conf_nake(JSEventType jevent)
{
    if (jevent <= JEvent_Begin || jevent >= JEvent_End) {
        SYSLOG("invalid jevent %d\n", jevent);
        return;
    }
    return js_event_send(g_p_conf, jevent, NULL, 0);
}

int get_capability(ShowWebS *web)
{
    static int capability_flag = FALSE;
    static ShowWebS weblist = {0};

    if (capability_flag) {
        memcpy(web, &weblist, sizeof(ShowWebS));
        return SUCCESS;
    }

    weblist.__4g = get_g_sys(usb_4g)?TRUE:FALSE;
    weblist.wifi = get_g_sys(usb_wifi)?TRUE:FALSE;

    int ret = 0;
    SysCustomS capability = {0,};

    ret += get_config(handleCapability, capability);

    BOOTARGS_CFG_S bootargs;
    memset(&bootargs, 0, sizeof(bootargs));
    ret += config_bootargs("get", NULL, &bootargs);

    if (ret < 0) {
        return ret;
    }

    weblist.hdetect = 1;
    weblist.ptz_ctrl = TRUE;
    weblist.alarmin = capability.alarmin;
    weblist.alarmout = capability.alarmout;
    weblist.graintype = capability.graintype;
    weblist.irlight = capability.irlight;
    weblist.follow = capability.follow;

    if (capability.webdeflang == 1 || capability.webdeflang == 2) {
        weblist.webdeflang = capability.webdeflang;
    } else {
        weblist.webdeflang = (bootargs.lang == 2) ? 1 : 0;
    }

    weblist.audio = 1;
    weblist.graintype = TRUE;

    weblist.alarmin = MAX_ALARM_IN;
    weblist.alarmout = MAX_ALARM_OUT;

    memcpy(web, &weblist, sizeof(ShowWebS));
    capability_flag = TRUE;

    return SUCCESS;
}

static int vencsize_is_live(VencSizeE vensize)
{
    VideoEncS ves = {0};
    if (handleVideoCfg((void *)NULL, &ves, sizeof(VideoEncS), NULL, (void*)"get") != SUCCESS) {
        return FAILURE;
    }

    if (((ves.enc[0].enable == 1) && (ves.enc[0].vencsize == vensize))
        || ((ves.enc[1].enable == 1) && (ves.enc[1].vencsize == vensize))) {
        return SUCCESS;
    }

    return FAILURE;
}

int SysConfVideoCfg(ConfAct act, ArgOpt opts[], VideoEncS* pdata)
{
    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    VideoEnc0 ais = {0,};

    void *arg[] = {pdata->enc, &ais};

    (act == ConfGet) ? cb = videoEncOptsCbFunc : cb = videoEncMapsCbFunc;

    MapOptKey mapspara[] = {
        {"id"      , ARG_INT_RD_ONLY, (void*)&ais.id      , sizeof(ais.id      ), 0},
        {"enable"  , ArgTypeInt     , (void*)&ais.enable  , sizeof(ais.enable  ), 0},
        {"codec"   , ArgTypeInt     , (void*)&ais.codec   , sizeof(ais.codec   ), 0},
        {"vencsize", ArgTypeInt     , (void*)&ais.vencsize, sizeof(ais.vencsize), 0},
        {"standard", ArgTypeInt     , (void*)&ais.standard, sizeof(ais.standard), 0},
        {"fps"     , ArgTypeInt     , (void*)&ais.fps     , sizeof(ais.fps     ), 0},
        {"bps"     , ArgTypeInt     , (void*)&ais.bps     , sizeof(ais.bps     ), 0},
        {"gop"     , ArgTypeInt     , (void*)&ais.gop     , sizeof(ais.gop     ), 0},
        {"fixfps"  , ArgTypeInt     , (void*)&ais.fixfps  , sizeof(ais.fixfps  ), 0},
        {"fixbps"  , ArgTypeInt     , (void*)&ais.fixbps  , sizeof(ais.fixbps  ), 0},
        {"End"     , ArgTypeEnd     , NULL                , 0                   , 0},
    };

    MapOptKey mapslist[] = {
        {"encode", ArgTypeTree, (void*)mapspara, sizeof(mapspara), 0, cb, arg},
        {"End"   , ArgTypeEnd , NULL          , 0                , 0         },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, ENCODE_MAX_CHN, mapslist);

    MapOptKey maps[] = {
        {"gnum"   , ArgTypeInt , (void*)&pdata->gnum, sizeof(pdata->gnum), 0},
        {"enclist", ArgTypeTree, (void*)extMap      , sizeof(extMap     ), 0},
        {"End"    , ArgTypeEnd , NULL               , 0                  , 0},
    };

    conf_lock();
    int ret = confAccessRoot(act, g_root, "videoEncode", opts, maps);
    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;

        //send_conf_data(JEvent_VideoCfgChg, pdata, sizeof(VideoEncS));

    }
    conf_unlock();

    FREE_EXT_MAPKEY_STRUCT();

    return ret;
}

int SysConfOsdExpandCfg(ConfAct act, ArgOpt opts[], OsdExpandS* pdata)
{

    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    OsdExpand0 ais = {0,};

    void *arg[] = {(void*)&(pdata->cusosd), &ais};

    (act == ConfGet) ? cb = osdExpandOptsCbFunc : cb = osdExpandMapsCbFunc;

    MapOptKey mapspara[] = {
        {"id"     , ArgTypeInt   , (void*)&ais.id    , sizeof(ais.id     ), 0 },
        {"enable" , ArgTypeInt   , (void*)&ais.enable, sizeof(ais.enable ), 0 },
        {"x"      , ArgTypeInt   , (void*)&ais.x     , sizeof(ais.x      ), 0 },
        {"y"      , ArgTypeInt   , (void*)&ais.y     , sizeof(ais.y      ), 0 },
        {"content", ArgTypeString, (void*)ais.content, sizeof(ais.content), ""},
        {"End"    , ArgTypeEnd   , NULL              , 0                  , 0 },
    };

    MapOptKey mapslist[] = {
        {"cusosd", ArgTypeTree, (void*)mapspara, sizeof(mapspara), 0, cb, arg},
        {"End"   , ArgTypeEnd , NULL           , 0               , 0         },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, OSD_EXPAND_MAX_CHN, mapslist);

    MapOptKey maps[] = {
        {"size"   , ArgTypeInt , (void*)&pdata->size, sizeof(pdata->size), 0},
        {"font"   , ArgTypeInt , (void*)&pdata->font, sizeof(pdata->font), 0},
        {"osdlist", ArgTypeTree, (void*)extMap      , sizeof(extMap     ), 0},
        {"End"    , ArgTypeEnd , NULL               , 0                  , 0},
    };

    conf_lock();
    int ret = confAccessRoot(act, g_root, "osdExpand", opts, maps);
    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;

        send_conf_data(JEvent_OsdExpandCfgChg, pdata, sizeof(OsdExpandS));

    }
    conf_unlock();

    FREE_EXT_MAPKEY_STRUCT();

    return ret;
}

int SysConfVideoMaskCfg(ConfAct act, ArgOpt opts[], VideoMaskS* pdata)
{
    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    VideoMask0 ais = {0,};

    void *arg[] = {(void*)&(pdata->mask), &ais};

    (act == ConfGet) ? cb = videoMaskOptsCbFunc : cb = videoMaskMapsCbFunc;

    MapOptKey mapspara[] = {
        {"id"    , ARG_INT_RD_ONLY, (void*)&ais.id    , sizeof(ais.id    ), 0},
        {"enable", ArgTypeInt     , (void*)&ais.enable, sizeof(ais.enable), 0},
        {"color" , ArgTypeInt     , (void*)&ais.color , sizeof(ais.color ), 0},
        {"x0"    , ArgTypeInt     , (void*)&ais.x0    , sizeof(ais.x0    ), 0},
        {"y0"    , ArgTypeInt     , (void*)&ais.y0    , sizeof(ais.y0    ), 0},
        {"x1"    , ArgTypeInt     , (void*)&ais.x1    , sizeof(ais.x1    ), 0},
        {"y1"    , ArgTypeInt     , (void*)&ais.y1    , sizeof(ais.y1    ), 0},
        {"End"   , ArgTypeEnd     , NULL              , 0                  , 0},
    };

    MapOptKey mapslist[] = {
        {"mask", ArgTypeTree, (void*)mapspara, sizeof(mapspara), 0, cb, arg},
        {"End" , ArgTypeEnd , NULL           , 0               , 0,        },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, VIDEO_MASK_MAX_CHN, mapslist);

    MapOptKey maps[] = {
        {"gnum"    , ArgTypeInt , (void*)&pdata->gnum, sizeof(pdata->gnum), 0},
        {"masklist", ArgTypeTree, (void*)extMap      , sizeof(extMap     ), 0},
        {"End"     , ArgTypeEnd , NULL               , 0                  , 0},
    };

    conf_lock();
    int ret = confAccessRoot(act, g_root, "videoMask", opts, maps);
    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;

        send_conf_data(JEvent_VideoMaskCfgChg, pdata, sizeof(VideoMaskS));
    }
    conf_unlock();

    FREE_EXT_MAPKEY_STRUCT();

    return ret;
}

static int XmlConfSet(ArgOptS_T opts[], const char *node_dsr, JSEventType type, void *ptr, size_t size)
{
    conf_lock();
    int ret = confAccessRoot_t(ConfSet, g_root, node_dsr, opts);
    if(ret == 0) {
        g_setflag = 1;
        if (type > JEvent_Begin && type < JEvent_End) {
            send_conf_data(type, ptr, size);
        } else {
            DBG("ignore event %d @str %s\n", type, node_dsr);
        }
    }
    conf_unlock();
    return ret;
}

int SysConfCfg(ConfAct act, ArgOptS_T opts[], const char *node_dsr, JSEventType type)
{
    conf_lock();

    int ret = confAccessRoot_t(act, g_root, node_dsr, opts);
    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;
        send_conf_nake(type);
    }
    conf_unlock();

    return ret;
}

int SysConfUserCfg(ConfAct act, ArgOpt opts[], SysUserS *pdata)
{
    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    SysUser0  aexp = {0,};

    void *arg[] = {(void*)pdata->user, (void*)&aexp};

    (act == ConfGet) ? cb =  sysUserOptsCbFunc : cb = sysUserMapsCbFunc;

    MapOptKey mapsPara[] = {
        {"id"          , ArgTypeInt   , (void*)&aexp.id          , sizeof(aexp.id          ), 0 },
        {"group"       , ArgTypeInt   , (void*)&aexp.group       , sizeof(aexp.group       ), 0 },
        {"username"    , ArgTypeString, (void*)&aexp.username    , sizeof(aexp.username    ), ""},
        {"cryptpasswd" , ArgTypeString, (void*)&aexp.cryptpasswd , sizeof(aexp.cryptpasswd ), ""},
        {"digestpasswd", ArgTypeString, (void*)&aexp.digestpasswd, sizeof(aexp.digestpasswd), ""},
        {"onvifpasswd" , ArgTypeString, (void*)&aexp.onvifpasswd , sizeof(aexp.onvifpasswd ), ""},
        {"End"         , ArgTypeEnd   , NULL                     , 0                        , 0 },
    };

    MapOptKey mapsList[] = {
        {"user", ArgTypeTree, (void*)mapsPara, sizeof(mapsPara), 0, cb, arg},
        {"End" , ArgTypeEnd , NULL           , 0               , 0         },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, USER_MAX_NUM, mapsList);

    MapOptKey maps[] = {
        {"gnum"    , ArgTypeInt , (void*)&pdata->gnum, sizeof(pdata->gnum), 0},
        {"userlist", ArgTypeTree, extMap             , sizeof(extMap     ), 0},
        {"End"     , ArgTypeEnd , NULL               , 0                  , 0},
    };

    conf_lock();
    int ret = confAccessRoot(act, g_root, "sysUser", opts, maps);
    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;

        send_conf_data(JEvent_UserCfgChg, pdata, sizeof(SysUserS));

    }
    conf_unlock();

    FREE_EXT_MAPKEY_STRUCT();

    return ret;
}

int SysConfRoiCfg(ConfAct act, ArgOpt opts[], RoiAreaS* pdata)
{
    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    RoiArea0 roi = {0,};
    void *arg[] = {&(pdata->area), &roi};

    (act == ConfGet) ? cb = roiListOptsCbFunc : cb = roiListMapsCbFunc;

    MapOptKey mapsPara[]    = {
        {"id"    , ARG_INT_RD_ONLY, (void*)&roi.id    , sizeof(int   ), 0},
        {"enable", ArgTypeInt, (void*)&roi.enable, sizeof(int), 0},
        {"qp"    , ArgTypeInt, (void*)&roi.qp, sizeof(int), 0},
        {"interval", ArgTypeInt, (void*)&roi.interval, sizeof(int), 0},
        {"left", ArgTypeInt, (void*)&roi.left, sizeof(int), 0},
        {"top"  , ArgTypeInt, (void*)&roi.top, sizeof(int), 0},
        {"right", ArgTypeInt, (void*)&roi.right, sizeof(int), 0},
        {"bottom", ArgTypeInt, (void*)&roi.bottom, sizeof(int), 0},
        {"End"   , ArgTypeEnd, NULL              , 0                 , 0},
    };

    MapOptKey ainList[] = {
        {"area", ArgTypeTree, (void*)mapsPara, sizeof(mapsPara), 0, cb, arg},
        {"End", ArgTypeEnd , NULL      , 0          , 0         },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, MAX_ROI_AREA, ainList);

    MapOptKey maps[] = {
        {"gnum"    , ARG_INT_RD_ONLY, (void*)&pdata->gnum, sizeof(pdata->gnum)  , 0      },
        {"arealist", ArgTypeTree    , (void*)extMap        , sizeof(extMap)      , 0      },
        {"END"    , ArgTypeEnd     , NULL                 , 0                   , 0      },
    };

    conf_lock();

    int ret = confAccessRoot(act, g_root, "roiarea", opts, maps);

    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;

        send_conf_data(JEvent_RoiCfgChg, pdata, sizeof(RoiAreaS));

    }
    conf_unlock();

    FREE_EXT_MAPKEY_STRUCT();

    return ret;
}

int SysConfProfileCfg(ConfAct act, ArgOpt opts[], VeProfileS* pdata)
{
    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    ProfileS pro = {0,};
    void *arg[] = {&(pdata->ps), &pro};

    (act == ConfGet) ? cb = profileListOptsCbFunc : cb = profileListMapsCbFunc;

    MapOptKey mapsPara[]    = {
        {"vesize"    , ARG_INT_RD_ONLY, (void*)&pro.vesize, sizeof(int   ), 0},
        {"profile", ArgTypeInt, (void*)&pro.profile, sizeof(int), 0},
        {"level", ArgTypeInt, (void*)&pro.level, sizeof(int), 0},
        {"bIDREnable"  , ArgTypeInt, (void*)&pro.bIDREnable, sizeof(int), 0},
        {"End"   , ArgTypeEnd, NULL              , 0                 , 0},
    };

    MapOptKey ainList[] = {
        {"proinfo", ArgTypeTree, (void*)mapsPara, sizeof(mapsPara), 0, cb, arg},
        {"End", ArgTypeEnd , NULL      , 0          , 0         },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, MAX_PROFILE_NUM, ainList);

    MapOptKey maps[] = {
        {"profilelist", ArgTypeTree    , (void*)extMap        , sizeof(extMap)      , 0      },
        {"END"    , ArgTypeEnd     , NULL                 , 0                   , 0      },
    };

    conf_lock();

    int ret = confAccessRoot(act, g_root, "profilecfg", opts, maps);

    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;

        send_conf_data(JEvent_ProfileCfgChg, pdata, sizeof(VeProfileS));

    }
    conf_unlock();

    FREE_EXT_MAPKEY_STRUCT();

    return ret;
}
#if 0
static int backup_eth2bootargs(NetEthS *outer)
{
    /* backup ethcfg */
    char buf[1024] = {0};
    snprintf(buf, sizeof(buf)-1,
        "1 /cfg/eth/nic     %s\n"
        "1 /cfg/eth/ip      %s\n"
        "1 /cfg/eth/mask    %s\n"
        "1 /cfg/eth/gw      %s\n"
        "1 /cfg/eth/dhcpen  %d\n"
        "1 /cfg/eth/reticle %d\n"
        "1 /cfg/eth/ipadaen %d\n"
        "1 /cfg/eth/dns     %s\n",
        outer->nic    ,
        outer->ip     ,
        outer->mask   ,
        outer->gw     ,
        outer->dhcpen ,
        outer->reticle,
        outer->ipadaen,
        outer->dns);
    return DumpFile("/opt/conf/eth.pair", buf, strlen(buf));
}
#endif

int SysConfPresetCfg(ConfAct act, ArgOpt opts[], presetcfg* pdata)
{

    MapOptKey *extMap = NULL;
    cbReLabel cb = NULL;
    presetlist ais = {0,};

    void *arg[] = {(void*)&(pdata->preset), &ais};

    (act == ConfGet) ? cb = PresetlistOptsCbFunc : cb = PresetlistMapsCbFunc;

    MapOptKey mapspara[] = {
        {"id"           , ArgTypeInt   , (void*)&ais.id         , sizeof(ais.id)            , 0 },
        {"x"            , ArgTypeInt   , (void*)&ais.x          , sizeof(ais.x)             , 0 },
        {"y"            , ArgTypeInt   , (void*)&ais.y          , sizeof(ais.y)             , 0 },
        {"isdefault"    , ArgTypeInt   , (void*)&ais.isdefault  , sizeof(ais.isdefault)     , 0 },
        {"enable"       , ArgTypeInt   , (void*)&ais.enable     , sizeof(ais.enable)        , 0 },
        {"name"         , ArgTypeString, (void*)ais.name        , sizeof(ais.name)          , ""},
        {"End"          , ArgTypeEnd   , NULL                   , 0                         , 0 },
    };

    MapOptKey mapslist[] = {
        {"preset", ArgTypeTree, (void*)mapspara, sizeof(mapspara), 0, cb, arg},
        {"End"   , ArgTypeEnd , NULL           , 0               , 0         },
    };

    CALLOC_EXT_STRUCT(MapOptKey, extMap, MAX_PRESET_NUM, mapslist);

    MapOptKey maps[] = {
        {"gnum"         , ArgTypeInt , (void*)&pdata->gnum  , sizeof(pdata->gnum)   ,   0},
        {"presetlist"   , ArgTypeTree, (void*)extMap        , sizeof(extMap)        ,   0},
        {"End"          , ArgTypeEnd , NULL                 , 0                     ,   0},
    };

    conf_lock();
    int ret = confAccessRoot(act, g_root, "presetcfg", opts, maps);
    if((ret == 0) && (act == ConfSet)) {
        g_setflag = 1;
        send_conf_data(JEvent_PresetCfg, pdata, sizeof(presetcfg));
    }
    conf_unlock();
    FREE_EXT_MAPKEY_STRUCT();
    return ret;
}

int handleEthCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    NetEthS   outer = {{0},};
    NetEthS   inner = {{0},};

    ArgOptS_T opts[] = {
        {"nic"    , ArgTypeString     , NULL       , outer.nic     , sizeof(outer.nic) , "eth0"             },
        {"ip"     , ArgTypeString     , NULL       , outer.ip      , sizeof(outer.ip)  , "192.168.1.217"    },
        {"mask"   , ArgTypeString     , NULL       , outer.mask    , sizeof(outer.mask), "255.255.255.0"    },
        {"gw"     , ArgTypeString     , NULL       , outer.gw      , sizeof(outer.gw)  , "192.168.1.1"      },
        {"enable" , ArgTypeInt        , "0|1"      , &outer.enable , sizeof(int)       , 0                  },
        {"mtu"    , ArgTypeInt        , "1200~1500", &outer.mtu    , sizeof(outer.mtu) , 0                  },
        {"dhcpen" , ArgTypeInt        , "0|1"      , &outer.dhcpen , sizeof(int)       , 0                  },
        {"ipadaen", ArgTypeInt        , "0|1"      , &outer.ipadaen, sizeof(int)       , 0                  },
        {"reticle", ArgTypeInt        , "0|1"      , &outer.reticle, sizeof(int)       , 0                  },
        {"dns"    , ArgTypeString     , NULL       , outer.dns     , sizeof(outer.dns) , "202.96.128.86"    },
        {"mac"    , ARG_STRING_RD_ONLY, NULL       , outer.mac     , sizeof(outer.mac) , "00:00:01:02:03:04"},
        {"ipcheck", ARG_INT_RD_ONLY   , "0|1"      , &outer.ipcheck, sizeof(int)       , 0                  },
        {"End"    , ArgTypeEnd        , NULL       , NULL          , 0                 , 0                  }
    };

    ret = SysConfCfg(ConfGet, opts, "eth", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    if (get_g_sys(usb_asix)) {
        strcpy(outer.nic, "usb0");
    } else {
        strcpy(outer.nic, "eth0"); // 如果没有恢复出厂，会被写入到 config.xml，一定要明确指出
    }

    static int mac_changed = TRUE;
    static char emac[32] = {0};

    if (mac_changed) {
        uboot_mac_get(outer.mac, sizeof(outer.mac));
        memcpy(emac, outer.mac, strlen(outer.mac));
        mac_changed = FALSE;
    } else {
        memcpy(outer.mac, emac, strlen(emac));
    }

    memcpy(&inner, &outer, sizeof(NetEthS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {

        if (inner.dhcpen) {
            memset(inner.ip,0, sizeof(inner.ip));
            memset(inner.gw, 0, sizeof(inner.gw));
            net_get_ipaddr("eth0", inner.ip, sizeof(inner.ip));
            unsigned int gateway = inet_addr(inner.ip);
            unsigned char *gw = (unsigned char*)&gateway;
            gw[3] = 1;
            snprintf(inner.gw, sizeof(inner.gw), "%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);
        }

        if(is_okey(F_ETH_ENABLE)) {
            inner.enable = 1;
        }

        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(NetEthS));

        if(0 != strcmp(inner.mac, outer.mac)) {
            int  head_8_bits = 0;
            LOG("settings mac from %s to %s\n", inner.mac, outer.mac);
            sscanf(outer.mac, "%x", &head_8_bits);
            if (head_8_bits & 0x01) {
                LOG("%s: lowest bit err of 1st Byte\n", outer.mac);
                return FAILURE;
            }
            processEthMac(ConfSet, "eth0", outer.mac);
            delay_ctrl_exec(DELAY_CMD_SETETHMAC, outer.mac, sizeof(outer.mac));

            send_conf_data(JEvent_EthMacCfgChg, &outer, sizeof(NetEthS));

            set_ethinfo(&outer);
            mac_changed = TRUE;
            return SUCCESS;
        }

        if(outer.enable != inner.enable) {
            delay_ctrl_exec(DELAY_CMD_SETETHIP, NULL, 0);
            ret = SysConfCfg(ConfSet, opts, "eth", JEvent_EthcfgChg);
        }

        if(0 != memcmp(&inner, &outer, (long)((NetEthS *)0)->nicEnd)) {
            if (ipMaskGatewayRuleCheck(outer.ip, outer.mask, outer.gw) < 0) {
                DBG("gw check error\n");
                return FAILURE;
            }
            if(outer.ipcheck == 0 && ipMaskGatewayCheck(outer.ip, outer.mask, outer.gw) < 0) {
                return FAILURE;
            }

            LOG("will change IP form %s to %s\n", inner.ip, outer.ip);
            ret = XmlConfSet(opts, "eth", JEvent_EthcfgChg, &outer, sizeof(outer));
            if(SUCCESS == ret) {
                delay_upnp_update_descfile();
                ddns_cfg_changed_process();
            }
            if (get_g_sys(usb_asix)) {
                net_set_ipaddr("usb0", outer.ip);  //设置IP
                net_set_gateway(outer.gw);       //设置网关
            }

            set_arp_network(&outer);
            set_arp_conflict_flag(FALSE);
            if (access(FACTORY_FACTEST, F_OK) != 0) {
                delay_ctrl_exec(DELAY_CMD_SETETHIP, NULL, 0);
            }
        } else if (0 != strncmp(inner.dhcpname, outer.dhcpname,32)) {
            ret = XmlConfSet(opts, "eth", JEvent_EthcfgChg, &outer, sizeof(outer));
        }

        if(0 != strcmp(inner.dns, outer.dns)) {
            DBG("set dns begin\n");
            ret = XmlConfSet(opts, "eth", JEvent_DnsCfgChg, &outer, sizeof(outer));
            DBG("set dns over\n");
        }

        if (outer.dhcpen == 0) {
            delay_ctrl_exec(DELAY_CMD_SETDNS, NULL, 0);
        }

        if(inner.mtu != outer.mtu) {
            DBG("set mtu begin\n");
            ret = XmlConfSet(opts, "eth", JEvent_Begin, &outer, sizeof(outer));
            net_set_mtu("eth0",outer.mtu);
            net_set_mtu("usb0",outer.mtu);
            DBG("set mtu over\n");
        }

        if (inner.reticle != outer.reticle)
            system_set_eth_rate(outer.reticle);

        set_ethinfo(&outer);
    }

    return ret;
}

int handleSysInfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    SysInfoS   outer = {{0},};
    SysInfoS   inner = {{0},};

    ArgOptS_T opts[] = {
        {"platform"      , ARG_STRING_RD_ONLY, NULL , (outer.platform)       , sizeof(outer.platform) , 0},
        {"solution"      , ARG_STRING_RD_ONLY, NULL , (outer.solution)       , sizeof(outer.solution) , 0},
        {"devname"       , ArgTypeString     , NULL , (outer.devname  )      , sizeof(outer.devname)  , 0},
        {"kernelver"     , ARG_STRING_RD_ONLY, NULL , (outer.kernelver)      , sizeof(outer.kernelver), 0},
        {"serverver"     , ARG_STRING_RD_ONLY, NULL , (outer.serverver)      , sizeof(outer.serverver), 0},
        {"webver"        , ARG_STRING_RD_ONLY, NULL , (outer.webver   )      , sizeof(outer.webver)   , 0},
        {"devtype"       , ArgTypeString     , NULL , (outer.devtype  )      , sizeof(outer.devtype)  , 0},
        {"devid"         , ARG_STRING_RD_ONLY, NULL , (outer.devid    )      , sizeof(outer.devid)    , 0},
        {"devtype_select", ArgTypeInt        , "0~2", (&outer.devtype_select), sizeof(int)            , 0},
        {"plugver"       , ARG_STRING_RD_ONLY, NULL , (outer.plugver  )      , sizeof(outer.plugver)  , 0},
        {"custom_ui"     , ARG_STRING_RD_ONLY, NULL , (outer.custom_ui)      , sizeof(outer.custom_ui), 0},
        {"custom_appid"  , ArgTypeString     , NULL , outer.custom_appid     ,sizeof(outer.custom_appid), NULL},
        {"ver_lite"      , ArgTypeInt        , NULL , (&outer.ver_lite)      , sizeof(outer.ver_lite) , 0},
        {"verifystr"     , ARG_STRING_RD_ONLY, NULL , NULL                   , 0                      , 0},
        {"air_burn"      , ArgTypeString     , NULL , (outer.air_burn  )     , sizeof(outer.air_burn) , 0},
        {"END"           , ArgTypeEnd        , NULL , NULL                   , 0                      , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "devinfo", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(SysInfoS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        char idbuf[16] = {0};
        system_get_dev_id(idbuf);
        sprintf(inner.devid, "%s", idbuf);

        static char ver[64] = {0};
        if (ver[0] == '\0') {
            static char _platform[128] = {0};
            if (_platform[0] == 0) {
                char *p = NULL;
                strncpy(_platform, inner.platform, sizeof(_platform));
                p = strchr(_platform, ',');
                if (p != NULL) {
                    p[0] = '.';
                }
            }

            SYSLOG("WEB version: %s\n",
                get_web_ver(_platform, inner.devtype, inner.custom_ui, ver));
        } else {
            //DBG("WEB version: %s\n", ver);
        }

        snprintf(inner.serverver, sizeof(inner.serverver), "%s", ver);
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(SysInfoS));
        if (strlen(outer.devname) == 0) {
            ERR("devname strlen is 0, return fail\n");
            return -1;
        }
        if (strlen(outer.devtype) == 0) {
            ERR("devtype strlen is 0, return fail\n");
            return -1;
        }

        ret = XmlConfSet(opts, "devinfo", JEvent_SysInfoCfgChg, &outer, sizeof(outer));
        set_sysinfo(&outer);
    }

    return ret;
}

int handleotainfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    OtaInfoS   outer = {{0},};
    SysInfoS   szSysInfo = {0};

    if (!strncasecmp("get", (const char *)action,3)) {
        if((int)sizeof(OtaInfoS) > bufSize) {
            return FAILURE;
        }

        BOOTARGS_CFG_S bootargs = {{0,},};

        conf_get_bootargs(&bootargs);
        conf_get_sysinfocfg(&szSysInfo);

        strncpy(outer.devclass, "IPC", (size_t)sizeof("IPC"));
        strncpy(outer.cpu, bootargs.cpu, (size_t)sizeof(bootargs.cpu));
        strncpy(outer.Sensor, bootargs.sensor, (size_t)sizeof(bootargs.sensor));

        const char *flash = "16";
        strcpy(outer.flash, flash);

        //获取DevType
        if (LoadFile2("/ipc/etc/dev_type", "%s", outer.DevType) <= 0) {
            ERR("read /ipc/etc/dev_type fail!\n");
        }

        strncpy(outer.version, szSysInfo.serverver, (size_t)sizeof(szSysInfo.serverver));
        strcpy(outer.compiledate, SERVER_BUILD_DATE);//格式：20190923
        strncpy(outer.customer, szSysInfo.custom_ui, (size_t)sizeof(szSysInfo.custom_ui));
        outer.upgrade_time = 300;
        memcpy(buf, &outer, sizeof(OtaInfoS));
    }

    return ret;
}

int handleStreamNotify(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    DBG("JEvent_StreamNotify\n");
    send_conf_nake(JEvent_StreamNotify);

    return 0;
}

int handlePtzSerialCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    PtzSerialS outer = {0,};
    PtzSerialS inner = {0,};

    ArgOptS_T opts[] = {
        {"baud"        , ArgTypeInt     , "300~115200", &outer.baud         , sizeof(outer.baud        ), (const void*)4800},
        {"databits"    , ArgTypeInt     , "5|6|7|8"              , &outer.databits     , sizeof(outer.databits    ), (const void*)8   },
        {"stop"        , ArgTypeInt     , "1|2"                  , &outer.stop         , sizeof(outer.stop        ), (const void*)1   },
        {"addr"        , ArgTypeInt     , "1~255"                , &outer.addr         , sizeof(outer.addr        ), 0                },
        {"hreverse"    , ArgTypeInt     , "0|1"                  , &outer.hreverse     , sizeof(outer.hreverse    ), 0                },
        {"vreverse"    , ArgTypeInt     , "0|1"                  , &outer.vreverse     , sizeof(outer.vreverse    ), 0                },
        {"protocol"    , ArgTypeString  ,   NULL                 , outer.protocol     , sizeof(outer.protocol    ), NULL               },
        {"parity"      , ArgTypeString  , "N|n|O|o|E|e|S|s"      , (outer.parity      ), sizeof(outer.parity      ), "N"              },
        {"protocollist", ArgTypeString  , NULL                   , (outer.protocollist), sizeof(outer.protocollist), ""               },
        {"END"         , ArgTypeEnd    , NULL                    , NULL                , 0                          , 0                },
    };


    ret = SysConfCfg(ConfGet, opts, "ptzSerial", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(PtzSerialS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(PtzSerialS));
        ret = XmlConfSet(opts, "ptzSerial", JEvent_PtzSerialCfgChg, &outer, sizeof(outer));

        //serial_init();
    }

    return ret;
}

int handleNtpcfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    SysNtpS outer = {0,};
    SysNtpS inner = {0,};
    int port = 234;
    int interval = 6;
    ArgOptS_T opts[] = {
        {"enable"   , ArgTypeInt   , "0|1"     , &outer.enable, sizeof(int)              , 0              },
        {"ntpserver", ArgTypeString, NULL      , outer.ntpserver, sizeof(outer.ntpserver), "clock.isc.org"},
        {"ntpport"  , ArgTypeInt   , "1~65535" , &outer.ntpport, sizeof(outer.ntpport)   , (void *)&port} ,
        {"interval" , ArgTypeInt   , "1~65535" , &outer.interval, sizeof(outer.interval) , (void *)&interval},
        {"End"      , ArgTypeEnd   , NULL      , NULL     , 0                            , 0              },
    };

    ret = SysConfCfg(ConfGet, opts, "ntpcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(SysNtpS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(SysNtpS));
        ret = XmlConfSet(opts, "ntpcfg", JEvent_NtpcfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleTimeZoneCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    TzoneS outer = {0,};
    TzoneS inner = {0,};

    ArgOptS_T opts[] = {
        {"timezone"  , ArgTypeInt   , "0~34"         , &outer.idx       , sizeof(outer.idx)          },
        {"timeoffset", ArgTypeInt   , "-65535~65535" , &outer.sec_east  , sizeof(outer.sec_east)     },
        {"End"       , ArgTypeEnd   , NULL           , NULL             , 0                          },
    };

    ret = SysConfCfg(ConfGet, opts, "systime", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(TzoneS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(TzoneS));
        ret = XmlConfSet(opts, "systime", JEvent_TimeZoneCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleUpdateCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    static int firsttime = TRUE;
    static UpdateS outer = {0,};

    ArgOptS_T opts[] = {
        {"type"       , ArgTypeInt, "0~3"  , &outer.type       , sizeof(outer.type      ) },
        {"progressbar", ArgTypeInt, "0~111", &outer.progressbar, sizeof(outer.progressbar)},
        {"End"        , ArgTypeEnd, NULL   , NULL              , 0                        },
    };

    if (firsttime) {
        memset(&outer, 0, sizeof(UpdateS));
        firsttime = FALSE;
    }

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        UpdateS inner = {0,};
        memcpy(&inner, &outer, sizeof(UpdateS));

        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        SYSLOG("type=%d progressbar is %d\n", outer.type, outer.progressbar);
        // set is done at CONF_ARG_PARSER_T()
    } else if(!strncasecmp("clr", (const char *)action,3)) {
        memset(&outer, 0, sizeof(UpdateS));
    }

    return SUCCESS;
}



int handleBootargs(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    if(!strncasecmp("get", (const char *)action,3)) {
        BOOTARGS_CFG_S outer = {{0,},};
        BOOTARGS_CFG_S inner = {{0,},};
        config_bootargs("get", NULL, &outer);
        memcpy(&inner, &outer, sizeof(BOOTARGS_CFG_S));
        JCONF_GET_STRUCT_T();
    }

    return SUCCESS;
}

int handleRealVideoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt     *extOpts = NULL;

    VideoEncS geter = {0,};
    VideoEncS inner = {0,};

    VideoEnc0 tmpIn = {0,};

    void *argOpt[] = {(void*)inner.enc, (void*)&tmpIn };

    ArgOpt optspara[] = {
        {"id"      , ARG_INT_RD_ONLY, "0~2"           , (void*)&tmpIn.id      },
        {"enable"  , ArgTypeInt     , "0|1"           , (void*)&tmpIn.enable  },
        {"codec"   , ArgTypeInt     , "2|5|7"         , (void*)&tmpIn.codec   },
        {"vencsize", ArgTypeInt     , "0|1|2|3|5|6|7|8|9|11|12|13|14|15"  , (void*)&tmpIn.vencsize},
        {"standard", ArgTypeInt     , "0|1"           , (void*)&tmpIn.standard},
        {"fps"     , ArgTypeInt     , "1~30"          , (void*)&tmpIn.fps     },
        {"bps"     , ArgTypeInt     , "32~8192"       , (void*)&tmpIn.bps     },
        {"gop"     , ArgTypeInt     , "1~360"         , (void*)&tmpIn.gop     },
        {"fixfps"  , ArgTypeInt     , "0|1"           , (void*)&tmpIn.fixfps  },
        {"fixbps"  , ArgTypeInt     , "0~4"           , (void*)&tmpIn.fixbps  },
        {"End"     , ArgTypeEnd     , NULL           , NULL                  },
    };

    ArgOpt optslist[] = {
        {"encode", ArgTypeTree, NULL, (void*)optspara, videoEncMapsCbFunc, argOpt},
        {"End"   , ArgTypeEnd , NULL, NULL                                       },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, ENCODE_MAX_CHN, optslist);

    ArgOpt opts[] = {
        {"gnum"   , ArgTypeInt , "1~3", (void*)&inner.gnum},
        {"enclist", ArgTypeTree, NULL , (void*)extOpts    },
        {"End"    , ArgTypeEnd , NULL , NULL              },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfVideoCfg(ConfGet, opts, &geter);

    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action,3)) {
        if (encode_vencsize_to_idx(geter.enc[0].vencsize) > encode_max_idx(0)) {
            geter.enc[0].vencsize = encode_idx_to_vencsize(encode_max_idx(0));
        }
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(VideoEncS));
        if (encode_vencsize_to_idx(geter.enc[0].vencsize) > encode_max_idx(0)) {
            geter.enc[0].vencsize = encode_idx_to_vencsize(encode_max_idx(0));
        }
        ret = SysConfVideoCfg(ConfSet, opts, &inner);
        set_videoinfo(&inner);
    }

    FREE_EXT_STRUCT();

    return ret;
}

static void do_busy_protect(VideoEnc0 enc_old[], VideoEnc0 enc_new[])
{
    static struct timespec clock[2] = {{0},};

    for (size_t i = 0; i < 2; i++) {
        if (enc_new[i].vencsize != enc_old[i].vencsize || enc_new[i].codec != enc_old[i].codec) {
            if (sec_since_previous(&clock[i]) > 1) {
                ms_clock_reset(&clock[i]);
            } else {
                DBG("busy protect[%d] to vsize:%d codec:%d\n", i, enc_old[i].vencsize, enc_old[i].codec);
                memcpy(&enc_new[i], &enc_old[i], sizeof(enc_new[i]));
            }
        }
    }

    return;
}

int handleVideoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt     *extOpts = NULL;

    VideoEncS geter = {0,};
    VideoEncS inner = {0,};

    VideoEnc0 tmpIn = {0,};

    void *argOpt[] = {(void*)inner.enc, (void*)&tmpIn };

    ArgOpt optspara[] = {
        {"id"      , ARG_INT_RD_ONLY, "0~2"           , (void*)&tmpIn.id      },
        {"enable"  , ArgTypeInt     , "0|1"           , (void*)&tmpIn.enable  },
        {"codec"   , ArgTypeInt     , "2|5|7"         , (void*)&tmpIn.codec   },
        {"vencsize", ArgTypeInt     , "0|1|2|3|5|6|7|8|9|11|12|13|14|15|16"  , (void*)&tmpIn.vencsize},
        {"standard", ArgTypeInt     , "0|1"           , (void*)&tmpIn.standard},
        {"fps"     , ArgTypeInt     , "1~30"          , (void*)&tmpIn.fps     },
        {"bps"     , ArgTypeInt     , "32~4096"       , (void*)&tmpIn.bps     },
        {"gop"     , ArgTypeInt     , "1~300"         , (void*)&tmpIn.gop     },
        {"fixfps"  , ArgTypeInt     , "0|1"           , (void*)&tmpIn.fixfps  },
        {"fixbps"  , ArgTypeInt     , "0~4"           , (void*)&tmpIn.fixbps  },
        {"End"     , ArgTypeEnd     , NULL            , NULL                  },
    };

    ArgOpt optslist[] = {
        {"encode", ArgTypeTree, NULL, (void*)optspara, videoEncMapsCbFunc, argOpt},
        {"End"   , ArgTypeEnd , NULL, NULL                                       },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, ENCODE_MAX_CHN, optslist);

    ArgOpt opts[] = {
        {"gnum"   , ArgTypeInt , "1~3", (void*)&inner.gnum},
        {"enclist", ArgTypeTree, NULL , (void*)extOpts    },
        {"End"    , ArgTypeEnd , NULL , NULL              },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfVideoCfg(ConfGet, opts, &geter);

    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action,3)) {
        // 自动限制
        VideoIdxE idx;
        idx = encode_vencsize_to_idx(geter.enc[0].vencsize);
        geter.enc[0].vencsize = encode_idx_to_vencsize(get_valid_vidx(idx, encode_min_idx(0), encode_max_idx(0)));
        idx = encode_vencsize_to_idx(geter.enc[1].vencsize);
        geter.enc[1].vencsize = encode_idx_to_vencsize(get_valid_vidx(idx, encode_min_idx(1), encode_max_idx(1)));

        geter.enc[0].fps = get_valid_fps(inner.enc[0].fps);
        geter.enc[1].fps = get_valid_fps(inner.enc[1].fps);

        CPY_GETER2BUF(geter, buf);

    } else if(!strncasecmp("set", (const char *)action,3)) {
        VideoEnc0 chn1 = {0,};
        VideoEnc0 chn2 = {0,};

#ifdef __TWO_CHN_COUND_NOT_BE_SAME__
        if(inner.enc[0].vencsize == inner.enc[1].vencsize) {
            ERR("Two Encode chn is the same size!\n");
            FREE_EXT_STRUCT();
            return FAILURE;
        }
#endif
        if(inner.enc[0].vencsize == VencSizeE_960P) {
            ERR("this main vencsize is not support!\n");
            FREE_EXT_STRUCT();
            return FAILURE;
        }

        if(inner.enc[1].vencsize != VencSizeE_360P) {
            ERR("this sub vencsize is not support!\n");
            FREE_EXT_STRUCT();
            return FAILURE;
        }

        if(inner.enc[0].id == 0) {
            memcpy(&chn1, &inner.enc[0], sizeof(VideoEnc0));
            memcpy(&chn2, &inner.enc[1], sizeof(VideoEnc0));
        } else {
            memcpy(&chn1, &inner.enc[1], sizeof(VideoEnc0));
            memcpy(&chn2, &inner.enc[0], sizeof(VideoEnc0));
        }

        int index_d1 = get_index_of_vesize(VencSizeE_720P);
        int index_chn1 = get_index_of_vesize(chn1.vencsize);
        int index_chn2 = get_index_of_vesize(chn2.vencsize);
        int index_max = get_index_of_vesize(encode_idx_to_vencsize(encode_max_web_idx(0)));

        inner.enc[0].enable = inner.enc[1].enable = 1;
        // 自动限制
        VideoIdxE idx;
        idx = encode_vencsize_to_idx(inner.enc[0].vencsize);
        inner.enc[0].vencsize = encode_idx_to_vencsize(get_valid_vidx(idx, encode_min_idx(0), encode_max_idx(0)));
        idx = encode_vencsize_to_idx(inner.enc[1].vencsize);
        inner.enc[1].vencsize = encode_idx_to_vencsize(get_valid_vidx(idx, encode_min_idx(1), encode_max_idx(1)));
        inner.enc[1].codec = inner.enc[0].codec;
        inner.enc[0].fps = get_valid_fps(inner.enc[0].fps);
        inner.enc[1].fps = get_valid_fps(inner.enc[1].fps);
        SYSLOG("ve:%d idx0: %d ~ %d\n", inner.enc[0].vencsize, encode_min_idx(0), encode_max_idx(0));
        SYSLOG("ve:%d idx1: %d ~ %d\n", inner.enc[1].vencsize, encode_min_idx(1), encode_max_idx(1));
        do_busy_protect(geter.enc, inner.enc);

        if((index_chn1 < index_d1) || (index_chn2 > index_d1)) {
            ERR("chn1 size small than D1 or chn2 size big than D1\n");
            FREE_EXT_STRUCT();
            return FAILURE;
        }

        if(index_chn1 > index_max) {
            ERR("chn1 size big than max_size %d %d \n",index_chn1,index_max);
            FREE_EXT_STRUCT();
            return FAILURE;
        }

        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(VideoEncS));
        ret = SysConfVideoCfg(ConfSet, opts, &inner);

        int video_change = 0;
        if (0 != memcmp(&inner.enc[0], &geter.enc[0], sizeof(VideoEnc0)) ||
            0 != memcmp(&inner.enc[1], &geter.enc[1], sizeof(VideoEnc0))) {
            video_change = 1;
        }

        if (inner.enc[0].vencsize == VencSizeE_5M) {
            inner.enc[0].vencsize = VencSizeE_4M;
        }

        if(1 == video_change) {
            DBG("videoinfo is change\n");
            set_videoinfo(&inner);
            //check_record_vensize(inner);
            send_conf_data(JEvent_VideoCfgChg, &inner, sizeof(VideoEncS));
        }
    }

    FREE_EXT_STRUCT();

    return ret;
}

int handleVideoCallCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    sVideoCallCfg outer = {0};
    sVideoCallCfg inner = {0};

    ArgOptS_T opts[] = {
        {"show"      ,  ArgTypeInt,     "0|1",  &outer.show      ,  sizeof(outer.show)  },
        {"enable"    ,  ArgTypeInt,     "0|1",  &outer.enable    ,  sizeof(outer.enable)},
        {"modelId"   ,  ArgTypeString,  NULL ,  outer.modelId    ,  sizeof(outer.modelId)  },
        {"appId"     ,  ArgTypeString,  NULL ,  outer.appId      ,  sizeof(outer.appId)},
        {"openId"    ,  ArgTypeString,  NULL ,  outer.openId     ,  sizeof(outer.openId)},
        {"openStatus",  ArgTypeInt,     "0|1",  &outer.openStatus,  sizeof(outer.openStatus)},
        {"End"       ,  ArgTypeEnd,     NULL ,  NULL             ,  0                   },
    };

    ret = SysConfCfg(ConfGet, opts, "videocall", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    memcpy(&inner, &outer, sizeof(sVideoCallCfg));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(sVideoCallCfg));
        ret = XmlConfSet(opts, "videocall", JEvent_VideoCallCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleOsdExpandCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt     *extOpts = NULL;

    OsdExpandS geter = {0,};
    OsdExpandS inner = {0,};

    OsdExpand0 tmpIn = {0,};

    void *argOpt[] = {(void*)inner.cusosd, (void*)&tmpIn };

    ArgOpt optspara[] = {
        {"id"     , ArgTypeInt   , "0~9"   , (void*)&tmpIn.id    },
        {"enable" , ArgTypeInt   , "0|1"   , (void*)&tmpIn.enable},
        {"x"      , ArgTypeInt   , "0~1919", (void*)&tmpIn.x     },
        {"y"      , ArgTypeInt   , "0~1079", (void*)&tmpIn.y     },
        {"content", ArgTypeString, NULL    , (void*)tmpIn.content},
        {"End"    , ArgTypeEnd   , NULL    , NULL                },
    };

    ArgOpt optslist[] = {
        {"cusosd", ArgTypeTree, NULL, (void*)optspara, osdExpandMapsCbFunc, argOpt},
        {"End"   , ArgTypeEnd , NULL, NULL                                        },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, OSD_EXPAND_MAX_CHN, optslist);

    ArgOpt opts[] = {
        {"size"   , ArgTypeInt , "0~128", (void*)&inner.size},
        {"font"   , ArgTypeInt , "0~3"  , (void*)&inner.font},
        {"osdlist", ArgTypeTree, NULL   , (void*)extOpts    },
        {"End"    , ArgTypeEnd , NULL   , NULL              },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfOsdExpandCfg(ConfGet, opts, &geter);
    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(OsdExpandS));
        #if defined(CUSTOMER_HBSD_PRO) || defined(CUSTOMER_CC_PRO)
        for(int i = 0; i < 4; i++) {
            inner.cusosd[i].enable = 0;
        }
        #endif
        #if defined(PLATFORM_HANBANG)
            if (get_g_sys(factest)) {
                ret = SysConfOsdExpandCfg(ConfSet, opts, &inner);
            }
        #else
            ret = SysConfOsdExpandCfg(ConfSet, opts, &inner);
        #endif
    }

    FREE_EXT_STRUCT();

    return ret;
}

int handleVideoMaskCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt *extOpts = NULL;

    VideoMaskS inner = {0,};
    VideoMaskS geter = {0,};

    VideoMask0 tmpIn = {0,};

    void *argOpt[] = {(void*)inner.mask, (void*)&tmpIn };

    ArgOpt optspara[] = {
        {"id"    , ARG_INT_RD_ONLY, "0~7"   , (void*)&tmpIn.id    },
        {"enable", ArgTypeInt     , "0|1"   , (void*)&tmpIn.enable},
        {"color" , ArgTypeInt     , "0~13"  , (void*)&tmpIn.color },
        {"x0"    , ArgTypeInt     , "0~1919", (void*)&tmpIn.x0    },
        {"y0"    , ArgTypeInt     , "0~1079", (void*)&tmpIn.y0    },
        {"x1"    , ArgTypeInt     , "0~1919", (void*)&tmpIn.x1    },
        {"y1"    , ArgTypeInt     , "0~1079", (void*)&tmpIn.y1    },
        {"End"   , ArgTypeEnd     , NULL    , NULL                },
    };

    ArgOpt optslist[] = {
        {"mask", ArgTypeTree, NULL, (void*)optspara, videoMaskMapsCbFunc, argOpt},
        {"End" , ArgTypeEnd , NULL, NULL                                        },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, VIDEO_MASK_MAX_CHN, optslist);

    ArgOpt opts[] = {
        {"gnum"    , ArgTypeInt , "0~8", (void*)&inner.gnum}, 
        {"masklist", ArgTypeTree, NULL , (void*)extOpts    },
        {"End"     , ArgTypeEnd , NULL , NULL              },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfVideoMaskCfg(ConfGet, opts, &geter);
    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        int i = 0;
        for(i = 1; i < VIDEO_MASK_MAX_CHN; i++) {
            inner.mask[i].color = inner.mask[0].color;
        }

        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(VideoMaskS));
        ret = SysConfVideoMaskCfg(ConfSet, opts, &inner);
    }

    FREE_EXT_STRUCT();

    return ret;
}

int handleVideoMaskPlanCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    videomask_plan_t outer = {0,};
    videomask_plan_t inner = {0,};

    ArgOptS_T opts[] = {
        {"week"           , ArgTypeString, NULL, outer.week            , sizeof(outer.week), "0000000"},
        {"enable"         , ArgTypeInt   , NULL, &outer.enable         , sizeof(int)       , 0},
        {"mask_enable"    , ArgTypeInt   , NULL, &outer.mask_enable    , sizeof(int)       , 0},
        {"cover_direction", ArgTypeInt   , NULL, &outer.cover_direction, sizeof(int)       , 0},
        {"beginhour"      , ArgTypeInt   , NULL, &outer.beginhour      , sizeof(int)       , 0},
        {"beginmin"       , ArgTypeInt   , NULL, &outer.beginmin       , sizeof(int)       , 0},
        {"endhour"        , ArgTypeInt   , NULL, &outer.endhour        , sizeof(int)       , 0},
        {"endmin"         , ArgTypeInt   , NULL, &outer.endmin         , sizeof(int)       , 0},
        {"End"            , ArgTypeEnd   , NULL, NULL                  , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "videomaskplan", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(videomask_plan_t));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(videomask_plan_t));
        ret = XmlConfSet(opts, "videomaskplan", JEvent_VideoMaskPlanCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAudioCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    AudioCfgS outer = {0,};
    AudioCfgS inner = {0,};

    ArgOptS_T opts[] = {
        {"enable"      , ArgTypeInt  , "0|1"  , &outer.inenable    , sizeof(int)  , 0},
        {"volumn"      , ArgTypeInt  , "0~100", &outer.involume    , sizeof(int)  , 0},
        {"ingain"      , ArgTypeInt  , "0~31" , &outer.ingain      , sizeof(int)  , 0},
        {"format"      , ArgTypeInt  , "0~7"  , &outer.codetype    , sizeof(int)  , 0},
        {"amrbps"      , ArgTypeInt  , "0~7"  , &outer.amrbps      , sizeof(int)  , 0},
        {"outvolume"   , ArgTypeInt  , "0~100", &outer.outvolume   , sizeof(int)  , 0},
        {"outgain"     , ArgTypeInt  , "0~31" , &outer.outgain     , sizeof(int)  , 0},
        {"inputtype"   , ArgTypeInt  , "0~1"  , &outer.inputtype   , sizeof(int)  , 0},
        {"definvolume" , ArgTypeInt  , "0~100", &outer.definvolume , sizeof(int)  , 0},
        {"defoutvolume", ArgTypeInt  , "0~100", &outer.defoutvolume, sizeof(int)  , 0},
        {"talkvolume"  , ArgTypeInt  , "0~100", &outer.talkvolume  , sizeof(int)  , 0},
        {"talkamp"     , ArgTypeFloat, NULL   , &outer.talkamp     , sizeof(float), 0},
        {"outamp"      , ArgTypeFloat, NULL   , &outer.outamp      , sizeof(float), 0},
        {"inamp"       , ArgTypeFloat, NULL   , &outer.inamp       , sizeof(float), 0},
        {"End"         , ArgTypeEnd  , NULL   , NULL               , 0            , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "audioIn", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(AudioCfgS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(AudioCfgS));
        ret = XmlConfSet(opts, "audioIn", JEvent_AudioInCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAudioTestCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    static AudioTestCfgS outer = {0, 1, 90, 90};
    AudioTestCfgS inner = {0,};

    memcpy(&inner, &outer, sizeof(inner));
    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &data, sizeof(inner));
        memcpy(&outer, data, sizeof(outer));
        send_conf_nake(JEvent_AudioTestCfgChg);
    }

    return ret;
}

int handleNetPortCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    NetPortS outer = {0,};
    NetPortS inner = {0,};

    ArgOptS_T opts[] = {
        {"httpPort"  , ArgTypeInt, "0~65535", &outer.httpport  , sizeof(outer.httpport)  , (char *)80  },
        {"ftpPort"   , ArgTypeInt, "0~65535", &outer.ftpport   , sizeof(outer.ftpport)   , (char *)21  },
        {"rtspPort"  , ArgTypeInt, "0~65535", &outer.rtspport  , sizeof(outer.rtspport)  , (char *)554 },
        {"speekPort" , ArgTypeInt, "0~65535", &outer.audioport , sizeof(outer.audioport) , (char *)8004},
        {"updatePort", ArgTypeInt, "0~65535", &outer.updateport, sizeof(outer.updateport), (char *)8006},
        {"End"       , ArgTypeEnd, NULL     , NULL             , 0                       , 0           },
    };

    ret = SysConfCfg(ConfGet, opts, "netport", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(NetPortS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(NetPortS));
        ret = XmlConfSet(opts, "netport", JEvent_Begin, &outer, sizeof(outer));
        set_portinfo(&outer);

        if (inner.httpport != outer.httpport) {
            send_conf_data(JEvent_HttpPortCfgChg, &outer, sizeof(outer));
            delay_upnp_update_descfile();
        }

        if (inner.rtspport != outer.rtspport) {
            send_conf_data(JEvent_RtspPortCfgChg, &outer, sizeof(outer));
        }

        if (inner.ftpport != outer.ftpport) {
            send_conf_data(JEvent_FtpPortCfgChg, &outer, sizeof(outer));
        }

        if (inner.audioport != outer.audioport) {
            send_conf_data(JEvent_SpeekPortCfgChg, &outer, sizeof(outer));
        }

        if (inner.updateport != outer.updateport) {
            send_conf_data(JEvent_UpdatePortCfgChg, &outer, sizeof(outer));
        }
    }

    return ret;

}

int handleEmailCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    EmailS inner = {{0},};
    EmailS outer = {{0},};

    ArgOptS_T opts[] = {
        {"smtpserver", ArgTypeString, NULL      , (void*)outer.smtpserver, sizeof(outer.smtpserver), "163.com"          },
        {"sendto"    , ArgTypeString, NULL      , (void*)outer.sendto    , sizeof(outer.sendto)    , "anonymous@163.com"},
        {"user"      , ArgTypeString, NULL      , (void*)outer.user      , sizeof(outer.user)      , "anonymous"        },
        {"password"  , ArgTypeString, NULL      , (void*)outer.password  , sizeof(outer.password)  , "anonymous"        },
        {"End"       , ArgTypeEnd   , NULL      , NULL                   , 0                       , 0                  },
    };

    ret = SysConfCfg(ConfGet, opts, "email", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(EmailS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(EmailS));
        ret = XmlConfSet(opts, "email", JEvent_EmailCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleWifiCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    NetWifiS outer = {0,};
    NetWifiS inner = {0,};

    ArgOptS_T opts[] = {
        {"nic"      , ARG_STRING_RD_ONLY, NULL , outer.nic      , sizeof(outer.nic)      , "wlan0"},
        {"ssid"     , ArgTypeString     , NULL , outer.ssid     , sizeof(outer.ssid)     , ""     },
        {"weppasswd", ArgTypeString     , NULL , outer.weppasswd, sizeof(outer.weppasswd), ""     },
        {"token"    , ArgTypeString     , NULL , &outer.token   , sizeof(outer.token)    , 0      },
        {"mode"     , ArgTypeInt        , "0|1", &outer.mode    , sizeof(int)            , 0      },
        {"dhcp"     , ArgTypeInt        , "0|1", &outer.dhcp    , sizeof(int)            , 0      },
        {"ip"       , ArgTypeString     , NULL , outer.ip       , sizeof(outer.ip)       , ""     },
        {"mask"     , ArgTypeString     , NULL , outer.mask     , sizeof(outer.mask)     , ""     },
        {"gw"       , ArgTypeString     , NULL , outer.gw       , sizeof(outer.gw)       , ""     },
        {"mac"      , ARG_STRING_RD_ONLY, NULL , outer.mac      , sizeof(outer.mac)      , ""     },
        {"status"   , ARG_INT_RD_ONLY   , NULL , &outer.status  , sizeof(int)            , 0      },
        {"End"      , ArgTypeEnd        , NULL , NULL           , 0                      , 0      },
    };

    ret = SysConfCfg(ConfGet, opts, "wifi", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(NetWifiS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        if (get_g_sys(usb_wifi)) {
            processEthMac(ConfGet, outer.nic, inner.mac);
            char ipaddr[32] = {0};
            net_get_ipaddr(outer.nic, ipaddr, sizeof(ipaddr));
            if (strlen(ipaddr) > 0) {
                memcpy(inner.ip, ipaddr, sizeof(ipaddr));
                unsigned int gateway = inet_addr(inner.ip);
                memset(inner.gw, 0, sizeof(inner.gw));
                unsigned char *gw = (unsigned char *)&gateway;
                gw[3] = 1;
                snprintf(inner.gw, sizeof(inner.gw)-1, "%d.%d.%d.%d", gw[0], gw[1], gw[2], gw[3]);
            }
        }

        JCONF_GET_STRUCT_T();

    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(NetWifiS));

        if (strncmp(inner.ssid, outer.ssid, sizeof(outer.ssid)) == 0 &&
            strncmp(inner.weppasswd, outer.weppasswd, sizeof(outer.weppasswd)) == 0 &&
            access(SUPPLICANT_OK_CONF, F_OK) != 0) {
            memset(inner.ssid, 0, sizeof(inner.ssid));
        }

        if(0 != memcmp(&inner, &outer, (long)((NetWifiS *)0)->mac)) {
            if(ipMaskGatewayCheck(outer.ip, outer.mask, outer.gw) < 0) {
                DBG("ipMaskGatewayCheck fail\n");
                return FAILURE;
            }

            ret = XmlConfSet(opts, "wifi", JEvent_WifiCfgChg, &outer, sizeof(outer));
        }

        if(inner.dhcp != outer.dhcp) {
            //to do...
        }


    }

    return ret;

}

int handleMotionDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    MotionDetectLinkS outer = {0,};
    MotionDetectLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"soundsel"   , ArgTypeInt, "0~2"     , &outer.soundsel   , sizeof(outer.soundsel)   , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "motionDetLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(MotionDetectLinkS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(MotionDetectLinkS));
        ret = XmlConfSet(opts, "motionDetLink", JEvent_MotionDetLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleHumanDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    HumanDetectLinkS outer = {0,};
    HumanDetectLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"soundsel"   , ArgTypeInt, "0~2"     , &outer.soundsel   , sizeof(outer.soundsel)   , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "humanDetLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(HumanDetectLinkS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(HumanDetectLinkS));
        ret = XmlConfSet(opts, "humanDetLink", JEvent_HumanDetLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleVglineLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    VglineLinkS outer = {0,};
    VglineLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"soundsel"   , ArgTypeInt, "0~2"     , &outer.soundsel   , sizeof(outer.soundsel)   , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "vglineLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(VglineLinkS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(VglineLinkS));
        ret = XmlConfSet(opts, "vglineLink", JEvent_VglineLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleVgrectLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    VgrectLinkS outer = {0,};
    VgrectLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"soundsel"   , ArgTypeInt, "0~2"     , &outer.soundsel   , sizeof(outer.soundsel)   , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "vgrectLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(VgrectLinkS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(VgrectLinkS));
        ret = XmlConfSet(opts, "vgrectLink", JEvent_VgrectLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleCarDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    CarDetectLinkS outer = {0,};
    CarDetectLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "carDetLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(CarDetectLinkS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(CarDetectLinkS));
        ret = XmlConfSet(opts, "carDetLink", JEvent_CarDetLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handlePetDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    PetDetectLinkS outer = {0,};
    PetDetectLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "petDetLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(PetDetectLinkS));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(PetDetectLinkS));
        ret = XmlConfSet(opts, "petDetLink", JEvent_PetDetLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleCryDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    CryDetectLinkS outer = {0,};
    CryDetectLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"snapshot"   , ArgTypeInt, "0|1"     , &outer.snapshot   , sizeof(outer.snapshot)   , 0},
        {"preset"     , ArgTypeInt, "0~255"   , &outer.preset     , sizeof(outer.preset)     , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "cryDetLink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(CryDetectLinkS));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(CryDetectLinkS));
        ret = XmlConfSet(opts, "cryDetLink", JEvent_CryDetLinkCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleAudioAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    AudioAlarmS inner = {0,};
    AudioAlarmS outer = {0,};

    ArgOptS_T opts[] = {
        {"show"      , ArgTypeInt , "0|1"  , &outer.show             , sizeof(outer.show)               , 0 },
        {"enable"    , ArgTypeInt , "0|1"  , &outer.enable            , sizeof(outer.enable)            , 0 },
        {"type"      , ArgTypeInt , "0~4"  , &outer.type              , sizeof(outer.type)              , 0 },
        {"place"     , ArgTypeInt , "0~3"  , &outer.place             , sizeof(outer.place)             , 0 },
        {"beginhour" , ArgTypeInt , "0~23" , &outer.timeseg.beginhour , sizeof(outer.timeseg.beginhour) , 0 },
        {"beginmin"  , ArgTypeInt , "0~59" , &outer.timeseg.beginmin  , sizeof(outer.timeseg.beginmin)  , 0 },
        {"endhour"   , ArgTypeInt , "0~23" , &outer.timeseg.endhour   , sizeof(outer.timeseg.endhour)   , 0 },
        {"endmin"    , ArgTypeInt , "0~59" , &outer.timeseg.endmin    , sizeof(outer.timeseg.endmin)    , 0 },
        {"times"     , ArgTypeInt , "0~10" , &outer.times             , sizeof(outer.times)             , 0 },
        {"aindex"    , ArgTypeInt   , NULL , &outer.aindex            , sizeof(outer.aindex)            , 0 },
        {"atext"     , ArgTypeString, NULL , outer.atext              , sizeof(outer.atext)             , 0 },
        {"End"       , ArgTypeEnd , NULL   , NULL                     , 0                               , 0 },
    };

    ret = SysConfCfg(ConfGet, opts, "audioalarm", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(AudioAlarmS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(AudioAlarmS));
        ret = XmlConfSet(opts, "audioalarm", JEvent_AudioAlarmCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleLightAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    LightAlarmS inner = {0,};
    LightAlarmS outer = {0,};

    ArgOptS_T opts[] = {
        {"enable"    , ArgTypeInt , "0|1"  , &outer.enable    , sizeof(outer.enable)    , 0 },
        {"place"     , ArgTypeInt , "0~3"  , &outer.place     , sizeof(outer.place)     , 0 },
        {"beginhour" , ArgTypeInt , "0~23" , &outer.timeseg.beginhour , sizeof(outer.timeseg.beginhour) , 0 },
        {"beginmin"  , ArgTypeInt , "0~59" , &outer.timeseg.beginmin  , sizeof(outer.timeseg.beginmin)  , 0 },
        {"endhour"   , ArgTypeInt , "0~23" , &outer.timeseg.endhour   , sizeof(outer.timeseg.endhour)   , 0 },
        {"endmin"    , ArgTypeInt , "0~59" , &outer.timeseg.endmin    , sizeof(outer.timeseg.endmin)    , 0 },
        {"time"      , ArgTypeInt , "10~60", &outer.time      , sizeof(outer.time)      , 0 },
        {"End"       , ArgTypeEnd , NULL   , NULL             , 0                       , 0 },
    };

    ret = SysConfCfg(ConfGet, opts, "lightalarm", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(LightAlarmS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(LightAlarmS));
        ret = XmlConfSet(opts, "lightalarm", JEvent_LightAlarmCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleIOAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    IOAlarmS inner = {0,};
    IOAlarmS outer = {0,};

    ArgOptS_T opts[] = {
        {"show"      , ArgTypeInt , "0|1"  , &outer.show      , sizeof(outer.show)      , 0 },
        {"enable"    , ArgTypeInt , "0|1"  , &outer.enable    , sizeof(outer.enable)    , 0 },
        {"place"     , ArgTypeInt , "0~3"  , &outer.place     , sizeof(outer.place)     , 0 },
        {"beginhour" , ArgTypeInt , "0~23" , &outer.timeseg.beginhour , sizeof(outer.timeseg.beginhour) , 0 },
        {"beginmin"  , ArgTypeInt , "0~59" , &outer.timeseg.beginmin  , sizeof(outer.timeseg.beginmin)  , 0 },
        {"endhour"   , ArgTypeInt , "0~23" , &outer.timeseg.endhour   , sizeof(outer.timeseg.endhour)   , 0 },
        {"endmin"    , ArgTypeInt , "0~59" , &outer.timeseg.endmin    , sizeof(outer.timeseg.endmin)    , 0 },
        {"time"      , ArgTypeInt , "10~60", &outer.time      , sizeof(outer.time)      , 0 },
        {"End"       , ArgTypeEnd , NULL   , NULL             , 0                       , 0 },
    };

    ret = SysConfCfg(ConfGet, opts, "ioalarm", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(IOAlarmS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(IOAlarmS));
        ret = XmlConfSet(opts, "ioalarm", JEvent_IOAlarmCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleVMaskAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    char times[128] = {0};
    VMaskAlarmS outer = {0,};
    VMaskAlarmS inner = {0,};
    int ret = SUCCESS;

    ArgOptS_T opts[] = {
        {"enable"  , ArgTypeInt   , "0|1"     , &outer.enable, sizeof(int)  , 0      },
        {"thresh"  , ArgTypeInt   , "0~100"   , &outer.thresh, sizeof(int)  , 0      },
        {"times"   , ArgTypeString, NULL      , times        , sizeof(times), "0:0,1:0,2:0,3:0,4:0,5:0,6:0,"},
        {"End"     , ArgTypeEnd   , NULL      , NULL         , 0            , 0      },
    };

    ret = SysConfCfg(ConfGet, opts, "vmaskalarm", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(VMaskAlarmS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        intarray_to_timestr(times, outer.times);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(VMaskAlarmS));
        ret = XmlConfSet(opts, "vmaskalarm", JEvent_VMaskAlarmCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleVMaskAlarmLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    VMaskAlarmLinkS outer = {0,};
    VMaskAlarmLinkS inner = {0,};
    int ret = SUCCESS;

    ArgOptS_T opts[] = {
        {"interval"   , ArgTypeInt, "3~36000" , &outer.interval   , sizeof(outer.interval)   , 0},
        {"alarmcenter", ArgTypeInt, "0|1"     , &outer.alarmcenter, sizeof(outer.alarmcenter), 0},
        {"email"      , ArgTypeInt, "0|1"     , &outer.email      , sizeof(outer.email)      , 0},
        {"alarmout1"  , ArgTypeInt, "0|1"     , &outer.alarmout1  , sizeof(outer.alarmout1)  , 0},
        {"alarmout2"  , ArgTypeInt, "0|1"     , &outer.alarmout2  , sizeof(outer.alarmout2)  , 0},
        {"sound"      , ArgTypeInt, "0|1"     , &outer.sound      , sizeof(outer.sound)      , 0},
        {"record"     , ArgTypeInt, "0|1"     , &outer.record     , sizeof(outer.record)     , 0},
        {"captureen"  , ArgTypeInt, "0|1"     , &outer.capture    , sizeof(outer.capture)     , 0},
        {"ftpup"      , ArgTypeInt, "0|1"     , &outer.ftpup      , sizeof(outer.ftpup)      , 0},
        {"End"        , ArgTypeEnd, NULL      , NULL                     , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "vmalarmlink", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    memcpy(&inner, &outer, sizeof(VMaskAlarmLinkS));
    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_T();
    } else if (!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(VMaskAlarmLinkS));
        ret = XmlConfSet(opts, "vmalarmlink", JEvent_VMaskAlarmLinkCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleDriveOutCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    DriveOut inner = {0,};
    DriveOut outer = {0,};

    ArgOptS_T opts[] = {
        {"show"      , ArgTypeInt , "0|1"  , &outer.show       , sizeof(outer.show)    , 0 },
        {"enable"    , ArgTypeInt , "0|1"  , &outer.enable     , sizeof(outer.enable)  , 0 },
        {"whiteen"   , ArgTypeInt , "0|1"  , &outer.whiteen    , sizeof(outer.whiteen) , 0 },
        {"audioen"   , ArgTypeInt , "0|1"  , &outer.audioen    , sizeof(outer.audioen) , 0 },
        {"time"      , ArgTypeInt , "10~60", &outer.time       , sizeof(outer.time)    , 0 },
        {"rest"      , ArgTypeInt , "0~60" , &outer.rest_time  , sizeof(outer.rest_time), 0 },
        {"End"       , ArgTypeEnd , NULL   , NULL              , 0                     , 0 },
    };

    ret = SysConfCfg(ConfGet, opts, "driveout", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(DriveOut));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(DriveOut));
        ret = XmlConfSet(opts, "driveout", JEvent_DriveOutCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleIrCtrlCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    IrCtrlS outer = {0,};
    IrCtrlS inner = {0,};

    ArgOptS_T opts[] = {
        {"type"         , ARG_INT_RD_ONLY, "0|1|2"   , &outer.type          , sizeof(outer.type)        , 0},
        {"turnonlux"    , ArgTypeInt     , "0~100"   , &outer.turnonlux     , sizeof(outer.turnonlux)   , 0},
        {"webturnonlux" , ArgTypeInt     , "0~100"   , &outer.webturnonlux  , sizeof(outer.webturnonlux), 0},
        {"lightmode"    , ArgTypeInt     , "0~3"     , &outer.lightmode     , sizeof(outer.lightmode)   , 0},
        {"switchmode"   , ArgTypeInt     , "1|2|3"   , &outer.switchmode    , sizeof(outer.switchmode)  , 0},
        {"beginhour"    , ArgTypeInt     , "0~23"    , &outer.beginhour     , sizeof(outer.beginhour)   , 0},
        {"beginmin"     , ArgTypeInt     , "0~59"    , &outer.beginmin      , sizeof(outer.beginmin)    , 0},
        {"endhour"      , ArgTypeInt     , "0~23"    , &outer.endhour       , sizeof(outer.endhour)     , 0},
        {"endmin"       , ArgTypeInt     , "0~59"    , &outer.endmin        , sizeof(outer.endmin)      , 0},
        {"irtesten"     , ArgTypeInt     , "0|1"     , &outer.irtesten      , sizeof(outer.irtesten)    , 0},
        {"irtestresult" , ArgTypeInt     , "0|1"     , &outer.irtestresult  , sizeof(outer.irtestresult), 0},
        {"ircutmode"    , ArgTypeInt     , "0~2"     , &outer.eIrcutMode    , sizeof(outer.eIrcutMode)  , 0},
        {"shinemode"    , ArgTypeInt     , "0|1|2"   , &outer.shinemode     , sizeof(outer.shinemode)   , 0},
        {"shinetime"    , ArgTypeInt     , "10~60"   , &outer.shinetime     , sizeof(outer.shinetime)   , 0},
        {"autolighten"  , ArgTypeInt     , "0|1"     , &outer.autolighten   , sizeof(outer.autolighten) , 0},
        {"lightgrade"   , ArgTypeInt     , "1~100"   , &outer.lightgrade    , sizeof(outer.lightgrade)  , 0},
        {"whitectrl"    , ArgTypeInt     , "0|1"     , &outer.whitectrl     , sizeof(outer.whitectrl)   , 0},
        {"fcolorbeginhour"   , ArgTypeInt      , "0~23"    , &outer.fcolorbeginhour   , sizeof(outer.fcolorbeginhour)  , 0},
        {"fcolorbeginmin"    , ArgTypeInt      , "0~59"    , &outer.fcolorbeginmin    , sizeof(outer.fcolorbeginmin)   , 0},
        {"fcolorendhour"     , ArgTypeInt      , "0~23"    , &outer.fcolorendhour     , sizeof(outer.fcolorendhour)    , 0},
        {"fcolorendmin"      , ArgTypeInt      , "0~59"    , &outer.fcolorendmin      , sizeof(outer.fcolorendmin)     , 0},
        {"End"          , ArgTypeEnd     , NULL      , NULL                 , 0                         , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "irCtrl", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(IrCtrlS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
#ifdef MOTION_TRIGGER_LIGHT
        DBG("outer.shinemode:%d, turnonlux:%d, webturnonlux:%d\n", outer.shinemode, outer.turnonlux, outer.webturnonlux);
        if(outer.shinemode == 1){
            ;//light_ctrl(LED_WHITE_OFF);
        }
#endif
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(IrCtrlS));
        ret = XmlConfSet(opts, "irCtrl", JEvent_IrCtrlCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleLightCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    LightCfg outer = {0,};
    LightCfg inner = {0,};

    ArgOptS_T opts[] = {
        {"mode"          , ArgTypeInt   , "0~100"   , &outer.mode           , sizeof(outer.mode)          , 0},
        {"openlightlux"  , ArgTypeInt   , "0~100"   , &outer.openlightlux   , sizeof(outer.openlightlux)  , 0},
        {"closelightlux" , ArgTypeInt   , "0~100"   , &outer.closelightlux  , sizeof(outer.closelightlux) , 0},
        {"End"           , ArgTypeEnd   , NULL      , NULL                  , 0                           , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "lightcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(LightCfg));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
#if defined(BRANCH_FPS)
        inner.mode = 1;
#elif defined(BRANCH_DBL)
        inner.mode = 2;
#else
        inner.mode = 0;
#endif

#if defined(BRANCH_STAR)
        inner.mode = (get_feature() == 1 ? 1 : 0);
#elif defined(DIGITAL_ZOOM) || defined(OPTICS_ZOOM)
        inner.mode = 0;
#endif

        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(LightCfg));
        ret = XmlConfSet(opts, "lightcfg", JEvent_LightCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleLightExtCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    static int nightled = 0;
    int ret = SUCCESS;

    LightExtCfg outer = {0,};
    LightExtCfg outer_evt = {0,};
    LightExtCfg inner = {0,};
    int lightboardchg = 0;

    ArgOptS_T opts[] = {
        {"lamptype"       , ArgTypeInt, "0~3"  , &outer.lamptype       , sizeof(outer.lamptype)       , 0},
        {"alg"            , ArgTypeInt, "0~5"  , &outer.alg            , sizeof(outer.alg)            , 0},
        {"beginhour"      , ArgTypeInt, "0~23" , &outer.beginhour      , sizeof(outer.beginhour)      , 0},
        {"beginmin"       , ArgTypeInt, "0~59" , &outer.beginmin       , sizeof(outer.beginmin)       , 0},
        {"endhour"        , ArgTypeInt, "0~23" , &outer.endhour        , sizeof(outer.endhour)        , 0},
        {"endmin"         , ArgTypeInt, "0~59" , &outer.endmin         , sizeof(outer.endmin)         , 0},
        {"fcolorbeginhour", ArgTypeInt, "0~23" , &outer.fcolorbeginhour, sizeof(outer.fcolorbeginhour), 0},
        {"fcolorbeginmin" , ArgTypeInt, "0~59" , &outer.fcolorbeginmin , sizeof(outer.fcolorbeginmin) , 0},
        {"fcolorendhour"  , ArgTypeInt, "0~23" , &outer.fcolorendhour  , sizeof(outer.fcolorendhour)  , 0},
        {"fcolorendmin"   , ArgTypeInt, "0~59" , &outer.fcolorendmin   , sizeof(outer.fcolorendmin)   , 0},
        {"truestar"       , ArgTypeInt, "0~1"  , &outer.truestar       , sizeof(outer.truestar)       , 0},
        {"gt_1_forceday"  , ArgTypeInt, "0~2"  , &outer.gt_1_forceday  , sizeof(outer.gt_1_forceday)  , 0},
        {"turn_on_pct"    , ArgTypeInt, "0~100", &outer.turn_on_pct    , sizeof(outer.turn_on_pct)    , 0},
        {"turn_off_pct"   , ArgTypeInt, "0~100", &outer.turn_off_pct   , sizeof(outer.turn_off_pct)   , 0},
        {"pwm_percent"    , ArgTypeInt, "0~100", &outer.pwm_percent    , sizeof(outer.pwm_percent)    , 0},
        {"pwm_ev"         , ArgTypeInt, "0|1"  , &outer.pwm_ev         , sizeof(outer.pwm_ev)         , 0},
        {"showautolight"  , ArgTypeInt, "0|1"  , &outer.showautolight  , sizeof(outer.showautolight)  , 0},
        {"adjustable"     , ArgTypeInt, "0|1"  , &outer.adjustable     , sizeof(outer.adjustable)     , 0},
        {"is_wh_triglow"  , ArgTypeInt, "0|1"  , &outer.is_wh_triglow  , sizeof(outer.is_wh_triglow)  , 0},
        {"is_ir_triglow"  , ArgTypeInt, "0|1"  , &outer.is_ir_triglow  , sizeof(outer.is_ir_triglow)  , 0},
        {"shinemode"      , ArgTypeInt, "0~2"  , &outer.shinemode      , sizeof(outer.shinemode)      , 0},
        {"shinetime"      , ArgTypeInt, "0~60" , &outer.shinetime      , sizeof(outer.shinetime)      , 0},
        {"ircut_reverse"  , ArgTypeInt, "0~2"  , &outer.ircut_reverse  , sizeof(outer.ircut_reverse)  , 0},
        {"irledmode"      , ArgTypeInt, "0|1"  , &outer.irledmode      , sizeof(outer.irledmode)      , 0},
        {"lampmode"       , ArgTypeInt, "0|1"  , &outer.lampmode       , sizeof(outer.lampmode)       , 0},
        {"lightboard"     , ArgTypeInt, "0~2"  , &outer.lightboard     , sizeof(outer.lightboard)     , 0},
        {"lighten"        , ArgTypeInt, "0~100", &outer.nightled       , sizeof(outer.nightled)       , 0},
        {"fcopenevda"     , ArgTypeFloat , "0.01~1000000.00" , &outer.fcopenevda   , sizeof(outer.fcopenevda)  , 0},
        {"fcopenevst"     , ArgTypeFloat , "0.01~1000000.00" , &outer.fcopenevst   , sizeof(outer.fcopenevst)  , 0},
        {"fcopenearly"    , ArgTypeFloat , "0.01~1000000.00" , &outer.fcopenearly  , sizeof(outer.fcopenearly) , 0},
        {"fcopenmiddle"   , ArgTypeFloat , "0.01~1000000.00" , &outer.fcopenmiddle , sizeof(outer.fcopenmiddle), 0},
        {"fcopenlate"     , ArgTypeFloat , "0.01~1000000.00" , &outer.fcopenlate   , sizeof(outer.fcopenlate)  , 0},
        {"fccloseevst"    , ArgTypeFloat , "0.01~1000000.00" , &outer.fccloseevst  , sizeof(outer.fccloseevst) , 0},
        {"fctargeevst"    , ArgTypeFloat , "0.01~1000000.00" , &outer.fctargeevst  , sizeof(outer.fctargeevst) , 0},
        {"fctargetratio"  , ArgTypeFloat , "0.001~1000.00"   , &outer.fctargetratio, sizeof(outer.fctargetratio),0},
        {"fclightmaxev"   , ArgTypeFloat , "0.01~1000000.00" , &outer.fclightmaxev , sizeof(outer.fclightmaxev) ,0},
        {"fclightminev"   , ArgTypeFloat , "0.01~1000000.00" , &outer.fclightminev , sizeof(outer.fclightminev) ,0},
        {"End"            , ArgTypeEnd, NULL   , NULL                  , 0                            , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "lightextcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    outer.nightled = nightled;
    memcpy(&inner, &outer, sizeof(LightExtCfg));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(outer.lightboard != inner.lightboard) {
        lightboardchg = 1;
    }

#if     defined(LGTBOARD_WHT)
    outer.lightboard = inner.lightboard = 1;
#elif   defined(LGTBOARD_INF)
    outer.lightboard = inner.lightboard = 0;
#endif

    if(!strncasecmp("get", (const char *)action,3)) {
        outer.nightled = nightled;
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {

#if    defined(LGTBOARD_WHT) || defined(LGTBOARD_INF)
        if(get_g_sys(factest) && lightboardchg) {
            ERR("device don't support setting up lightboard, please reset2factory!\n");
            return FAILURE;
        }
#endif

        if (outer.lightboard == 0) {       // 红外灯只显示红外模式
            outer.lamptype = LAMP_IR;
        } else if (outer.lightboard == 1) {  // 白光灯只显示全彩模式
            outer.lamptype = LAMP_WHITE;
        }

        if(outer.turn_on_pct <= 33){
            outer.fcopenevst = outer.fcopenearly ;      // 早
        } else if(outer.turn_on_pct <= 66) {
            outer.fcopenevst = outer.fcopenmiddle;      // 中
        } else {
            outer.fcopenevst = outer.fcopenlate  ;      // 晚
        }

        if (nightled == outer.nightled) {
            RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(LightExtCfg));
        } 

        memcpy(&outer_evt, &outer, sizeof(LightExtCfg));
        nightled = outer.nightled;
        outer.nightled = 0;

        ret = XmlConfSet(opts, "lightextcfg", JEvent_Begin, &outer, sizeof(outer));
        
        send_conf_data(JEvent_LightExtCfgChg, &outer_evt,sizeof(outer_evt));

    }

    return ret;
}

int handleUpnpCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    NetUpnpS outer = {0,};
    NetUpnpS inner = {0,};

    ArgOptS_T opts[] = {
        {"rtsp"  , ArgTypeInt, "0|1"     , &outer.rtsp  , sizeof(int), 0},
        {"http"  , ArgTypeInt, "0|1"     , &outer.http  , sizeof(int), 0},
        {"ftp"   , ArgTypeInt, "0|1"     , &outer.ftp   , sizeof(int), 0},
        {"voice" , ArgTypeInt, "0|1"     , &outer.voice , sizeof(int), 0},
        {"update", ArgTypeInt, "0|1"     , &outer.update, sizeof(int), 0},
        {"End"   , ArgTypeEnd, NULL      , NULL         , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "netUpnp", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(NetUpnpS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(NetUpnpS));
        ret = XmlConfSet(opts, "netUpnp", JEvent_UpnpCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleOsdStyleCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    OsdStyleS outer = {0,};
    OsdStyleS inner = {0,};

    ArgOptS_T opts[] = {
        {"colormode", ArgTypeInt, "0~2" , &outer.colormode, sizeof(int), 0},
        {"font"     , ArgTypeInt, "0"   , &outer.font     , sizeof(int), 0},
        {"width"    , ArgTypeInt, "8~96", &outer.width    , sizeof(int), 0},
        {"height"   , ArgTypeInt, "8~96", &outer.height   , sizeof(int), 0},
        {"End"      , ArgTypeEnd, NULL  , NULL            , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "osdStyle", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(OsdStyleS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(OsdStyleS));
        ret = XmlConfSet(opts, "osdStyle", JEvent_OsdStyleCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleRecordCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    RecordCtrlS outer = {0,};
    RecordCtrlS inner = {0,};
    char timestrategy[128] = {0};
    ArgOptS_T opts[] = {
        {"alarmseconds" , ArgTypeInt     , "10~150"   , &outer.alarmseconds , sizeof(int)         , 0      },
        {"schedminutes" , ArgTypeInt     , "1~3600"   , &outer.schedminutes , sizeof(int)         , 0      },
        {"diskreservemb", ArgTypeInt     , "10~100000", &outer.diskreservemb, sizeof(int)         , 0      },
        {"diskstrategy" , ArgTypeInt     , "0|1"      , &outer.diskstrategy , sizeof(int)         , 0      },
        {"filestrategy" , ArgTypeInt     , "0|1"      , &outer.filestrategy , sizeof(int)         , 0      },
        {"rec_type"     , ArgTypeInt     , "0|1"      , &outer.rec_type     , sizeof(int)         , 0      },
        {"prerecord"    , ArgTypeInt     , "0|1"      , &outer.prerecord    , sizeof(int)         , 0      },
        {"prerecordtime", ArgTypeInt     , "5~10"     , &outer.prerecordtime, sizeof(int)         , 0      },
        {"isrecording"  , ARG_INT_RD_ONLY, "0|1"      , &outer.isrecording  , sizeof(int)         , 0      },
        {"timestrategy" , ArgTypeString  , NULL       , timestrategy        , sizeof(timestrategy), "0:10,"},
        {"sd_times"     , ArgTypeInt     , "0~10"     , &outer.sd_times     , sizeof(int)         , 0      },
        {"sd_stat"      , ArgTypeInt     , "0~99"     , &outer.sd_stat      , sizeof(int)         , 0      },
        {"End"          , ArgTypeEnd     , NULL       , NULL                , 0                   , 0      },
    };

    ret = SysConfCfg(ConfGet, opts, "recordCtrl", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(timestrategy, outer.timestrategy);
    memcpy(&inner, &outer, sizeof(RecordCtrlS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        //check vencSize to do..
        intarray_to_timestr(timestrategy, outer.timestrategy);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(RecordCtrlS));
        ret = XmlConfSet(opts, "recordCtrl", JEvent_RecordCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAutoRebootCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    AutoRebootS outer = {0,};
    AutoRebootS inner = {0,};

    ArgOptS_T opts[] = {
        {"enable"   , ArgTypeInt, "0|1"     , &outer.enable   , sizeof(outer.enable)   , 0},
        {"alarmday" , ArgTypeInt, "0~7"     , &outer.alarmday , sizeof(outer.alarmday) , 0},
        {"alarmhour", ArgTypeInt, "0~23"    , &outer.alarmhour, sizeof(outer.alarmhour), 0},
        {"End"      , ArgTypeEnd, NULL      , NULL            , 0                      , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "autoReboot", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(AutoRebootS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(AutoRebootS));
        ret = XmlConfSet(opts, "autoReboot", JEvent_AutoRebootCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleBootargCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    SysBootArgS outer = {0,};
    SysBootArgS inner = {0,};

    ArgOptS_T opts[] = {
        {"lcdtype", ArgTypeInt, "0~2"     , &outer.lcdtype, sizeof(outer.lcdtype), 0       },
        {"lcdlogo", ArgTypeInt, "0~13"    , &outer.lcdlogo, sizeof(outer.lcdlogo), 0       },
        {"sensor" , ArgTypeInt, "6~10"    , &outer.sensor , sizeof(outer.sensor) , (void*)6},
        {"End"    , ArgTypeEnd, NULL      , NULL          , 0                    , 0       },
    };

    ret = SysConfCfg(ConfGet, opts, "sysBootArg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(SysBootArgS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(SysBootArgS));
        ret = XmlConfSet(opts, "sysBootArg", JEvent_BootargCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleCapability(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    SysCustomS outer = {0,};
    SysCustomS inner = {0,};

    ArgOptS_T opts[] = {
        {"wiper"        , ArgTypeInt, "0|1"     , &outer.wiper      , sizeof(int)   , 0},
        {"ircut"        , ArgTypeInt, "0|1"     , &outer.ircut      , sizeof(int)   , 0},
        {"irlight"      , ArgTypeInt, "0|1"     , &outer.irlight    , sizeof(int)   , 0},
        {"ircolor"      , ArgTypeInt, "0|1"     , &outer.ircolor    , sizeof(int)   , 0},
        {"alarmhost"    , ArgTypeInt, "0|1"     , &outer.alarmhost  , sizeof(int)   , 0},
        {"gps"          , ArgTypeInt, "0|1"     , &outer.gps        , sizeof(int)   , 0},
        {"ptz"          , ArgTypeInt, "0|1"     , &outer.ptz        , sizeof(int)   , 0},
        {"follow"       , ArgTypeInt, "0|1"     , &outer.follow     , sizeof(int)   , 0},
        {"alarmin"      , ArgTypeInt, "0~4"     , &outer.alarmin    , sizeof(int)   , 0},
        {"alarmout"     , ArgTypeInt, "0~2"     , &outer.alarmout   , sizeof(int)   , 0},
        {"graintype"    , ArgTypeInt, "0~1"     , &outer.graintype  , sizeof(int)   , 0},
        {"webdeflang"   , ArgTypeInt, "0~2"     , &outer.webdeflang , sizeof(int)   , 0},
        {"pixels"       , ArgTypeInt, "240~300" , &outer.pixels     , sizeof(int)   , 0},
        {"sdinfo"       , ArgTypeInt, "0~10"    , &outer.sdinfo     , sizeof(int)   , 0},
        {"osd"          , ArgTypeInt, "0~2"     , &outer.osd        , sizeof(int)   , 0},
        {"End"          , ArgTypeEnd, NULL      , NULL              , 0             , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "sysCustomize", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(SysCustomS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
#if defined(DEV_TYPE_BASE)
        inner.graintype = GRAIN_ALL_FUN;
#elif defined(DEV_TYPE_ENHANCED)
        inner.graintype = GRAIN_ALL_FUN;
#endif
#ifdef CUST_2MTOFIX3M
        inner.pixels = CUST_2MTOFIX3M;
#endif
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(SysCustomS));
        ret = XmlConfSet(opts, "sysCustomize", JEvent_EquipCfgChg, &outer, sizeof(outer));
        if(inner.pixels != outer.pixels) {
            send_conf_nake(JEvent_ProfileCfgChg);// notify rtsp reinit
        }
    }

    return ret;
}

int handleUserCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt     *extOpts = NULL;

    SysUserS geter = {0,};
    SysUserS inner = {0,};

    SysUser0 userIn = {0,};

    void *argOpt[] = {(void*)inner.user, (void*)&userIn };

    ArgOpt optsPara[] = {
        {"id"          , ArgTypeInt    , "0~8", (void*)&userIn.id          },
        {"group"       , ArgTypeInt    , "0~2", (void*)&userIn.group       },
        {"username"    , ArgTypeString , NULL , (void*)userIn.username     },
        {"cryptpasswd" , ArgTypeString , NULL , (void*)userIn.cryptpasswd  },
        {"digestpasswd", ArgTypeString , NULL , (void*)userIn.digestpasswd },
        {"onvifpasswd" , ArgTypeString , NULL , (void*)userIn.onvifpasswd  },
        {"End"         , ArgTypeEnd    , NULL , NULL                       },
    };

    ArgOpt optsList[] = {
        {"user", ArgTypeTree, NULL, (void*)optsPara, sysUserMapsCbFunc, argOpt},
        {"End" , ArgTypeEnd , NULL, NULL                                      },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, USER_MAX_NUM, optsList);

    ArgOpt opts[] = {
        {"gnum"    , ArgTypeInt , "0~8", (void*)&inner.gnum},
        {"userlist", ArgTypeTree, NULL , (void*)extOpts    },
        {"End"     , ArgTypeEnd , NULL , NULL              },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfUserCfg(ConfGet, opts, &geter);
    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(SysUserS));
        ret = SysConfUserCfg(ConfSet, opts, &inner);
    }

    FREE_EXT_STRUCT();

    return ret;
}

int handleAuthRealmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    AuthRealmS outer = {{0},};
    AuthRealmS inner = {{0},};

    ArgOptS_T opts[] = {
        {"realm"  , ArgTypeString, NULL      , &outer.realm  , sizeof(outer.realm)  , 0},
        {"End"    , ArgTypeEnd   , NULL      , NULL          , 0                    , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "sysUser", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(AuthRealmS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else {
        ERR("[%s] cmd unknow\n", (char *)action);
        ret = FAILURE;
    }

    return ret;
}

int handleWebShowCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    ShowWebS outer = {0};
    ret = get_capability(&outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        if((int)sizeof(ShowWebS) > bufSize) {
            return FAILURE;
        }

        memcpy(buf, &outer, sizeof(ShowWebS));
    }

    return ret;
}

int handleVideo3aCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    Video3aS outer = {0,};
    Video3aS inner = {0,};

    ArgOptS_T opts[] = {
        {"ae"             , ArgTypeInt, "0~11"    , &outer.ae             , sizeof(outer.ae)             , 0},
        {"awb"            , ArgTypeInt, "0~7"     , &outer.awb            , sizeof(outer.awb)            , 0},
        {"blc"            , ArgTypeInt, "0|1"     , &outer.blc            , sizeof(outer.blc)            , 0},
        {"redgain"        , ArgTypeInt, "1~255"   , &outer.redgain        , sizeof(outer.redgain)        , 0},
        {"bluegain"       , ArgTypeInt, "1~255"   , &outer.bluegain       , sizeof(outer.bluegain)       , 0},
        {"lowlightenhance", ArgTypeInt, "0~100"   , &outer.lowlightEnhance, sizeof(outer.lowlightEnhance), 0},
        {"nightfacemode"  , ArgTypeInt, "0~7"     , &outer.nightfacemode  , sizeof(outer.nightfacemode)  , 0},
        {"End"            , ArgTypeEnd, NULL      , NULL                  , 0                            , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "video3a", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(Video3aS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
#ifdef CUST_KESHIAN
    DBG("keshian\n");
    inner.nightfacemode = 0;
#endif
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(Video3aS));
        ret = XmlConfSet(opts, "video3a", JEvent_Video3aCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleStopSyncCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int set_flag = 0;

    conf_lock();
    set_flag = g_setflag;
    conf_unlock();

    js_delete_timer_r(&g_config_sync_handle);

    if(set_flag) {
        DBG("handleStopSyncCfg\n");
        syncXmlToFile();
    }

    return SUCCESS;
}

int handleDefaultCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    SYSLOG("handleDefaultCfg\n");
    remove("/opt/aging_test.sh");
    remove("/opt/keep_aging_test");
    DBG("action:%s\n",(char *)action);
    if(!strncmp((char *)action, "badxml", strlen("badxml")))
        UtilSystemCmd((char *)"/ipc/bin/reset2factory badxml");
    else
        UtilSystemCmd((char *)"/ipc/bin/reset2factory");

    return SUCCESS;
}

int handleCaptureCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    CaptureS outer = {0,};
    CaptureS inner = {0,};
    char timestrategy[128] = {0};

    ArgOptS_T opts[] = {
        {"vesize"      , ArgTypeInt   , "0~16", &outer.vesize     , sizeof(outer.vesize)      , (void *)0},
        {"interval"    , ArgTypeInt   , NULL  , &outer.interv    , sizeof(outer.interv)      , (void *)5},
        {"alarminterv" , ArgTypeInt   , NULL  , &outer.alarminterv, sizeof(outer.alarminterv) , 0        },
        {"alarmnum"    , ArgTypeInt   , NULL  , &outer.alarmnum   , sizeof(outer.alarmnum)    , 0        },
        {"timestrategy", ArgTypeString, NULL  , timestrategy      , sizeof(timestrategy)      , "0:0,1:0,2:0,3:0,4:0,5:0,6:0,"},
        {"End"         , ArgTypeEnd   , NULL  , NULL              , 0                         , 0        },
    };

    ret = SysConfCfg(ConfGet, opts, "capture", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(timestrategy, outer.timestrategy);
    memcpy(&inner, &outer, sizeof(CaptureS));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action,3)) {
        if (vencsize_is_live(outer.vesize) != SUCCESS) {
            ERR("vencsize is wrong\n");
            return FAILURE;
        }

        intarray_to_timestr(timestrategy, outer.timestrategy);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(CaptureS));
        ret = XmlConfSet(opts, "capture", JEvent_CaptureCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

static int handleIpLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action, JEventType type, const char *recType)
{
    int ret = SUCCESS;

    IpLinkS outer = {0,};
    IpLinkS inner = {0,};

    ArgOptS_T opts[] = {
        {"interval" , ArgTypeInt, "3~36000", &outer.interval , sizeof(int), (void *)5},
        {"ao0en"    , ArgTypeInt, "0|1"    , &outer.ao0en    , sizeof(int), 0        },
        {"ao1en"    , ArgTypeInt, "0|1"    , &outer.ao1en    , sizeof(int), 0        },
        {"recorden" , ArgTypeInt, "0|1"    , &outer.recorden , sizeof(int), 0        },
        {"sounden"  , ArgTypeInt, "0|1"    , &outer.sounden  , sizeof(int), 0        },
        {"captureen", ArgTypeInt, "0|1"    , &outer.captureen, sizeof(int), 0        },
        {"End"      , ArgTypeEnd, NULL     , NULL            , 0          , 0        },
    };

    ret = SysConfCfg(ConfGet, opts, recType, JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(IpLinkS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(IpLinkS));
        ret = XmlConfSet(opts, recType, type, &outer, sizeof(outer));
    }

    return ret;
}

int handleIpConflictCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    return handleIpLinkCfg(data, buf, bufSize, bufLen, action, JEvent_IpConflictCfgChg, "ipconflictlink");
}

int handleIpBrokenCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    return handleIpLinkCfg(data, buf, bufSize, bufLen, action, JEvent_IpBrokenCfgChg, "ipbrokenlink");
}

static void set_osd_name_error(OsdInfoS *osd)
{
    osd->nameen   = 1;
    osd->nameleft = 840;
    osd->nametop  = 440;
    osd->osdcolor = 9;
    strcpy(osd->name, "ERROR");
}

int handleOsdinfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    int burn_ok = 0;
    OsdInfoS outer = {0};
    OsdInfoS inner = {0};

    ArgOptS_T opts[] = {
        {"timeen"       , ArgTypeInt   , "0|1"   , &outer.timeen  , sizeof(int)       , 0},
        {"timeleft"     , ArgTypeInt   , "0~1920", &outer.timeleft, sizeof(int)       , 0},
        {"timetop"      , ArgTypeInt   , "0~1080", &outer.timetop , sizeof(int)       , 0},
        {"bpsen"        , ArgTypeInt   , "0|1"   , &outer.bpsen   , sizeof(int)       , 0},
        {"bpsleft"      , ArgTypeInt   , "0~1920", &outer.bpsleft , sizeof(int)       , 0},
        {"bpstop"       , ArgTypeInt   , "0~1080", &outer.bpstop  , sizeof(int)       , 0},
        {"nameen"       , ArgTypeInt   , "0|1"   , &outer.nameen  , sizeof(int)       , 0},
        {"nameleft"     , ArgTypeInt   , "0~1920", &outer.nameleft, sizeof(int)       , 0},
        {"nametop"      , ArgTypeInt   , "0~1080", &outer.nametop , sizeof(int)       , 0},
        {"name"         , ArgTypeString, NULL    , outer.name     , sizeof(outer.name), 0},
        {"gpsen"        , ArgTypeInt   , "0|1"   , &outer.gpsen   , sizeof(int)       , 0},
        {"gpsleft"      , ArgTypeInt   , "0~1920", &outer.gpsleft , sizeof(int)       , 0},
        {"gpstop"       , ArgTypeInt   , "0~1080", &outer.gpstop  , sizeof(int)       , 0},
        {"osdcolor"     , ArgTypeInt   , "0~3"   , &outer.osdcolor, sizeof(int)       , 0},
        {"osdlanguage"  , ArgTypeInt   , "0~3"   , &outer.osdlanguage   , sizeof(int) , 0},
        {"hdtop"        , ArgTypeInt   , "0~1080", &outer.hdtop   , sizeof(int)       , 0},
        {"hdleft"       , ArgTypeInt   , "0~1920", &outer.hdleft  , sizeof(int)       , 0},
        {"osdweek"      , ArgTypeInt   , "0|1"   , &outer.osdweek , sizeof(int)       , 0},
        {"dateformat"  , ArgTypeInt   , "0~8"   , &outer.dateformat  , sizeof(int)       , 0},
        {"END"     , ArgTypeEnd   , NULL    , NULL           , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "osdinfo", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    get_burn_result(&burn_ok);
    if (!system_get_security() && !burn_ok) {
        set_osd_name_error(&outer);
    }

    memcpy(&inner, &outer, sizeof(OsdInfoS));
    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(OsdInfoS));
        if (strlen(outer.name) == 0) {
            ERR("osd name is NULL\n");
            return -1;
        }

        ret = XmlConfSet(opts, "osdinfo", JEvent_OsdCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleTimeOSDCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    TimeOSD inner = {0};
    TimeOSD outer = {0};

    ArgOptS_T opts[] = {
        {"seq"       , ArgTypeInt     , "0~5"     , &outer.seq       , sizeof(int) , 0},
        {"connector" , ArgTypeInt     , "0~2"     , &outer.connector , sizeof(int) , 0},
        {"END"       , ArgTypeEnd     , NULL      , NULL             , 0           , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "timeosd", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    memcpy(&inner, &outer, sizeof(TimeOSD));
    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if (!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if (!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(TimeOSD));
        ret = XmlConfSet(opts, "timeosd", JEvent_TimeOsdCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleViinfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ViInfoS outer = {0};
    ViInfoS inner = {0};

    ArgOptS_T opts[] = {
        {"nightluma"    , ArgTypeInt, "0~255", &outer.nightluma    , sizeof(int), (void*)35 },
        {"bright"       , ArgTypeInt, "0~255", &outer.bright       , sizeof(int), (void*)128},
        {"contrast"     , ArgTypeInt, "0~255", &outer.contrast     , sizeof(int), (void*)128},
        {"hue"          , ArgTypeInt, "0~255", &outer.hue          , sizeof(int), (void*)128},
        {"saturation"   , ArgTypeInt, "0~255", &outer.saturation   , sizeof(int), (void*)128},
        {"sharpness"    , ArgTypeInt, "0~255", &outer.sharpness    , sizeof(int), (void*)128},
        {"lampfrequency", ArgTypeInt, "0|1"  , &outer.lampfrequency, sizeof(int), (void*)1  },
        {"reverse"      , ArgTypeInt, "0~3"  , &outer.reverse      , sizeof(int), 0         },
        {"gain"         , ArgTypeInt, "0~255", &outer.gain         , sizeof(int), 0         },
        {"brightlevel"  , ArgTypeInt, "0~6"  , &outer.brightlevel  , sizeof(int), 0         },
        {"suppress"     , ArgTypeInt, "0~100", &outer.suppress     , sizeof(int), 0         },
        {"END"          , ArgTypeEnd, NULL   , NULL                , 0          , 0         },
    };

    ret = SysConfCfg(ConfGet, opts, "viinfo", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(ViInfoS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(ViInfoS));
        ret = XmlConfSet(opts, "viinfo", JEvent_ViinfoCfgChg, &outer, sizeof(outer));

        if (outer.lampfrequency == 1) {    //50HZ   fps (1~25)
            VideoEncS ves = {0};
            int cflag = 0;
            get_config(handleRealVideoCfg, ves);

            if (ves.enc[0].fps > 25) {
                ves.enc[0].fps = 25;
                cflag = 1;
            }

            if (ves.enc[1].fps > 25) {
                ves.enc[1].fps = 25;
                cflag = 1;
            }

            if (cflag == 1) {
                set_config(handleVideoCfg, ves);
            }
        }

    }

    return ret;
}

int handleRoiCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt     *extOpts = NULL;

    RoiAreaS geter = {0,};
    RoiAreaS inner = {0,};

    RoiArea0 vmin = {0,};

    void *argOpt[] = {(void*)inner.area, (void*)&vmin};

    ArgOpt optsAin[] = {
        {"id"    , ARG_INT_RD_ONLY, "0~7"   , &vmin.id    },
        {"enable", ArgTypeInt, "0|1"   , &vmin.enable},
        {"qp"    , ArgTypeInt, "74~125"   , &vmin.qp},
        {"interval", ArgTypeInt, "0~30"   , &vmin.interval},
        {"left"  , ArgTypeInt, "0~1920", &vmin.left},
        {"top"   , ArgTypeInt, "0~1080", &vmin.top},
        {"right" , ArgTypeInt, "0~1920", &vmin.right},
        {"bottom", ArgTypeInt, "0~1080", &vmin.bottom},
        {"End"   , ArgTypeEnd, NULL    , NULL        },
    };

    ArgOpt optsAinList[] = {
        {"area", ArgTypeTree, NULL, (void*)optsAin, roiListMapsCbFunc, argOpt},
        {"End", ArgTypeEnd , NULL, NULL                                         },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, MAX_ROI_AREA, optsAinList);

    ArgOpt opts[] = {
        {"gnum"     , ARG_INT_RD_ONLY, "0~8", (void*)&inner.gnum  },
        {"arealist" , ArgTypeTree    , NULL , (void*)(extOpts    )},
        {"END"      , ArgTypeEnd     , NULL , NULL                },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfRoiCfg(ConfGet, opts, &geter);
    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(RoiAreaS));
        ret = SysConfRoiCfg(ConfSet, opts, &inner);
    }

    FREE_EXT_STRUCT();

    return ret;
}

int handleProfileCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    ArgOpt     *extOpts = NULL;

    VeProfileS geter;
    VeProfileS inner;
    memset(&geter, 0, sizeof(VeProfileS));
    memset(&inner, 0, sizeof(VeProfileS));
    ProfileS vmin = {0};

    void *argOpt[] = {(void*)inner.ps, (void*)&vmin};

    ArgOpt optsAin[] = {
        {"vesize"    , ARG_INT_RD_ONLY, "0~15"  , &vmin.vesize    },
        {"profile"   , ArgTypeInt     , "0~2"  , &vmin.profile   },
        {"level"     , ArgTypeInt     , "0~50" , &vmin.level     },
        {"bIDREnable", ArgTypeInt     , "0~1"  , &vmin.bIDREnable},
        {"End"       , ArgTypeEnd     , NULL   , NULL            },
    };

    ArgOpt optsAinList[] = {
        {"proinfo", ArgTypeTree, NULL, (void*)optsAin , profileListMapsCbFunc, argOpt},
        {"End"    , ArgTypeEnd , NULL, NULL                                          },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, MAX_PROFILE_NUM, optsAinList);

    ArgOpt opts[] = {
        {"profilelist", ArgTypeTree, NULL, (void*)(extOpts    )},
        {"END"        , ArgTypeEnd , NULL, NULL                },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfProfileCfg(ConfGet, opts, &geter);
    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(VeProfileS));
        ret = SysConfProfileCfg(ConfSet, opts, &inner);
    }

    FREE_EXT_STRUCT();

    return ret;
}

int handleDenoisecfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    DnrCfgS outer = {0};
    DnrCfgS inner = {0};

    ArgOptS_T opts[] = {
        {"enable", ArgTypeInt, "0~1"  , &outer.enable, sizeof(int), 0},
        {"mode"  , ArgTypeInt, "0~100", &outer.mode  , sizeof(int), 0},
        {"END"   , ArgTypeEnd, NULL   , NULL         , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "dnrcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(DnrCfgS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(DnrCfgS));
        ret = XmlConfSet(opts, "dnrcfg", JEvent_DnrCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAuthModecfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    int outer = 0;
    int inner = 0;

    ArgOptS_T opts[] = {
        {"mode"     , ArgTypeInt, "0~2", &outer, sizeof(int)       , 0},
        {"END"     , ArgTypeEnd   , NULL    , NULL           , 0                 , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "authmode", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(int));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(int ));
        ret = XmlConfSet(opts, "authmode", JEvent_AuthModecfgChg, &outer, sizeof(outer));
    }

    return ret;
}

/*越界侦测与区域侦测互斥，人形侦测与移动侦测、越界侦测、区域侦测互斥*/
typedef enum{
    E_DET_MOTION,
    E_DET_VGLINE,
    E_DET_VGRECT,
    E_DET_FOLLOW,
    E_DET_HUMAN,
    E_DET_CAR,
    E_DET_PET
}eDetectType;

static int DoDetectMutexRelationConfig(eDetectType CurDetType)
{
    MotionDetectS tMotion = {0};
    VglineS tVglineS = {0};
    VgrectS tVgrectS = {0};
    follow_info_t tfllow = {0};
    HumanDetectionS tHumanConfigParam = {0};
    CarDetectionS tCarDetCfg = {0};
    PetDetectionS tPetDetCfg = {0};

    conf_get_motiondetectcfg(&tMotion);
    conf_get_vglinecfg(&tVglineS);
    conf_get_vgrectcfg(&tVgrectS);
    conf_get_follow_cfg(&tfllow);
    conf_get_humandetectioncfg(&tHumanConfigParam);
    conf_get_cardetectioncfg(&tCarDetCfg);
    conf_get_petdetectioncfg(&tPetDetCfg);

    if (E_DET_MOTION != CurDetType && 1 == tMotion.enable) {
        tMotion.enable = 0;
        conf_set_motiondetectcfg(tMotion);
    }

    if (E_DET_VGLINE != CurDetType) {
        if (1 == tVglineS.enable || 1 == tVglineS.blink) {
            tVglineS.enable = 0;
            conf_set_vglinecfg(tVglineS);
        }
    }

    if (E_DET_VGRECT != CurDetType) {
        if (1 == tVgrectS.enable || 1 == tVgrectS.blink) {
            tVgrectS.enable = 0;
            conf_set_vgrectcfg(tVgrectS);
        }
    }

    if (E_DET_FOLLOW != CurDetType && E_DET_HUMAN != CurDetType &&
        E_DET_CAR != CurDetType && E_DET_PET != CurDetType) {
        if(1 == tfllow.enable) {
            tfllow.enable = 0;
            conf_set_follow_cfg(tfllow);
        }

        if (1 == tHumanConfigParam.enable) {
           tHumanConfigParam.enable = 0;
           tHumanConfigParam.drag = 0;
           conf_set_humandetectioncfg(tHumanConfigParam);
        }

        if (1 == tCarDetCfg.enable || 1 == tCarDetCfg.screenenable) {
            tCarDetCfg.enable = 0;
            tCarDetCfg.drag = 0;
            conf_set_cardetectioncfg(tCarDetCfg);
        }

        if (1 == tPetDetCfg.enable || 1 == tPetDetCfg.screenenable) {
            tPetDetCfg.enable = 0;
            tPetDetCfg.drag = 0;
            conf_set_petdetectioncfg(tPetDetCfg);
        }
    }

    return 0;
}

int handleMotionDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    MotionDetectS outer = {0,};
    MotionDetectS inner = {0,};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"enable"  , ArgTypeInt   , "0|1"  , &outer.enable, sizeof(outer.enable), 0      },
        {"thresh"  , ArgTypeInt   , "0~100", &outer.thresh, sizeof(outer.thresh), 0      },
        {"mbdesc"  , ArgTypeString, NULL   , outer.mbdesc , sizeof(outer.mbdesc), "00000"},
        {"times"   , ArgTypeString, NULL   , times  , sizeof(times) , "0:10,1:10,2:10,3:10,4:10,5:10,6:10;"},
        {"End"     , ArgTypeEnd   , NULL   , NULL         , 0                   , 0      },
    };

    ret = SysConfCfg(ConfGet, opts, "motionDetect", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(MotionDetectS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(MotionDetectS));

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_MOTION);
        }

        ret = XmlConfSet(opts, "motionDetect", JEvent_MotionDetectCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleHumanDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    static int drag = 0;
    int ret = SUCCESS;

    HumanDetectionS outer = {0,};
    HumanDetectionS inner = {0,};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"enable"       , ArgTypeInt   , "0|1"  , &outer.enable       , sizeof(outer.enable)       , 0      },
        {"screenenable" , ArgTypeInt   , "0|1"  , &outer.screenenable , sizeof(outer.screenenable) , 0      },
        {"thresh"       , ArgTypeInt   , "0~100", &outer.thresh       , sizeof(outer.thresh)       , 0      },
        {"humandistance", ArgTypeInt   , "0~100", &outer.humandistance, sizeof(outer.humandistance), 0      },
        {"drag"         , ArgTypeInt   , "0|1"  , &outer.drag         , sizeof(outer.drag)         , 0      },
        {"mbdesc"       , ArgTypeString, NULL   , outer.mbdesc        , sizeof(outer.mbdesc)       , "00000"},
        {"times"        , ArgTypeString, NULL   , times               , sizeof(times)              , "0:10,1:10,2:10,3:10,4:10,5:10,6:10;"},
        {"outdoor"      , ArgTypeInt   , "0~2"  , &outer.mode         , sizeof(outer.mode  )       , 0      },
        {"person_center", ArgTypeInt   , "0|1"  , &outer.person_center, sizeof(outer.person_center), 0      },
        {"faceae"       , ArgTypeInt   , "0|1"  , &outer.faceae       , sizeof(outer.faceae)       , 0      },
        {"End"          , ArgTypeEnd   , NULL   , NULL                , 0                          , 0      },
    };

    ret = SysConfCfg(ConfGet, opts, "humanDetect", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(HumanDetectionS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        inner.drag = drag;
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        if (outer.drag) {
            DBG("thresh dragging\n");
            drag = 1;
        } else {
            drag = 0;
            RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(HumanDetectionS));
        }

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_HUMAN);
        }

        ret = XmlConfSet(opts, "humanDetect", JEvent_HumanDetectCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleCarDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    static int drag = 0;
    int ret = SUCCESS;

    CarDetectionS outer = {0,};
    CarDetectionS inner = {0,};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"enable"        , ArgTypeInt   , "0|1"  , &outer.enable       , sizeof(outer.enable)       , 0      },
        {"show"          , ArgTypeInt   , "0|1|2", &outer.show         , sizeof(outer.show)         , 0      },
        {"screenenable"  , ArgTypeInt   , "0|1"  , &outer.screenenable , sizeof(outer.screenenable) , 0      },
        {"thresh"        , ArgTypeInt   , "0~100", &outer.thresh       , sizeof(outer.thresh)       , 0      },
        {"cardistance"   , ArgTypeInt   , "0~100", &outer.cardistance  , sizeof(outer.cardistance)  , 0      },
        {"drag"          , ArgTypeInt   , "0|1"  , &outer.drag         , sizeof(outer.drag)         , 0      },
        {"mbdesc"        , ArgTypeString, NULL   , outer.mbdesc        , sizeof(outer.mbdesc)       , "00000"},
        {"times"         , ArgTypeString, NULL   , times               , sizeof(times)              , "0:10,1:10,2:10,3:10,4:10,5:10,6:10;"},
        {"outdoor"       , ArgTypeInt   , "0~2"  , &outer.mode         , sizeof(outer.mode  )       , 0      },
        {"End"           , ArgTypeEnd   , NULL   , NULL                , 0                          , 0      },
    };

    ret = SysConfCfg(ConfGet, opts, "carDetect", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(CarDetectionS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        inner.drag = drag;

        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        if (outer.drag) {
            DBG("thresh dragging\n");
            drag = 1;
        } else {
            drag = 0;
            RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(CarDetectionS));
        }

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_CAR);
        }

        ret = XmlConfSet(opts, "carDetect", JEvent_CarDetectCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handlePetDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    static int drag = 0;
    int ret = SUCCESS;

    PetDetectionS outer = {0,};
    PetDetectionS inner = {0,};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"enable"      , ArgTypeInt   , "0|1"  , &outer.enable      , sizeof(outer.enable)      , 0                                    },
        {"screenenable", ArgTypeInt   , "0|1"  , &outer.screenenable, sizeof(outer.screenenable), 0                                    },
        {"thresh"      , ArgTypeInt   , "0~100", &outer.thresh      , sizeof(outer.thresh)      , 0                                    },
        {"petdistance" , ArgTypeInt   , "0~100", &outer.petdistance , sizeof(outer.petdistance) , 0                                    },
        {"drag"        , ArgTypeInt   , "0|1"  , &outer.drag        , sizeof(outer.drag)        , 0                                    },
        {"mbdesc"      , ArgTypeString, NULL   , outer.mbdesc       , sizeof(outer.mbdesc)      , "00000"                              },
        {"times"       , ArgTypeString, NULL   , times              , sizeof(times)             , "0:10,1:10,2:10,3:10,4:10,5:10,6:10;"},
        {"outdoor"     , ArgTypeInt   , "0~2"  , &outer.mode        , sizeof(outer.mode  )      , 0                                    },
        {"End"         , ArgTypeEnd   , NULL   , NULL               , 0                         , 0                                    },
    };

    ret = SysConfCfg(ConfGet, opts, "petDetect", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(PetDetectionS));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        inner.drag = drag;
	    
		HumanDetectionS tHumanConfigParam = {0};
		conf_get_humandetectioncfg(&tHumanConfigParam);
		inner.petdistance = tHumanConfigParam.humandistance;
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        if (outer.drag) {
            DBG("thresh dragging\n");
            drag = 1;
        } else {
            drag = 0;
            RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(PetDetectionS));
        }

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_PET);
        }

        ret = XmlConfSet(opts, "petDetect", JEvent_PetDetectCfgChg, &outer, sizeof(outer));
    }

    return ret;

}

int handleCryDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    CryDetectionS outer = {0,};
    CryDetectionS inner = {0,};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"enable"      , ArgTypeInt   , "0|1"  , &outer.enable      , sizeof(outer.enable)      , 0                                    },
        {"thresh"      , ArgTypeInt   , "0~100", &outer.thresh      , sizeof(outer.thresh)      , 0                                    },
        {"times"       , ArgTypeString, NULL   , times              , sizeof(times)             , "0:10,1:10,2:10,3:10,4:10,5:10,6:10;"},
        {"End"         , ArgTypeEnd   , NULL   , NULL               , 0                         , 0                                    },
    };

    ret = SysConfCfg(ConfGet, opts, "cryDetect", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(CryDetectionS));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        ret = XmlConfSet(opts, "cryDetect", JEvent_CryDetectCfgChg, &outer, sizeof(outer));
    }

    return ret;

}


int handleSysCtrlCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int cmd = -1;

    if (NULL != data) {
        cmd = *(int *)data;
        printf("cmd = %d\n", cmd);
    }

    switch (cmd) {
        case 0:
            DELAY_REBOOT_LINUX();
            break;
        case 1:
            DELAY_RESET_APPS();
            break;
        case 2:{
            remove(SUPPLICANT_OK_CONF);
            sync();
            ptz_config_default();
            delay_ctrl_exec(DELAY_CMD_DEFAULT, NULL, 0);
            break;
        }
        case 3:{
            ptz_config_default();
            delay_ctrl_exec(DELAY_CMD_DEFAULT_KEEP_NET, NULL, 0);
            break;
        }
        default:
            LOG("sysctrl cmd = %d\n", cmd);
            break;
    }
    return SUCCESS;
}


int handleTimeCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    time_t nowtime = 0;
    int ret = SUCCESS;

    if(!strncasecmp("set", (const char *)action,3)) {
        if (NULL != data) {
            nowtime = *(time_t *)data;
            DBG("TIME : %llu\n", nowtime);
            ret = dump_system_time(nowtime);
        }
    }

    return ret;
}

int handleGuoBiaoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    GuoBiaoS outer = {{0,},};
    GuoBiaoS inner = {{0,},};

    ArgOptS_T opts[] = {
        {"enable"      , ArgTypeInt   , "0~1"    , &outer.enable      , sizeof(int)               , 0},
        {"videochannel", ArgTypeInt   , "0~1"    , &outer.videochannel, sizeof(int)               , 0},
        {"manufacturer", ArgTypeString, NULL     , outer.manufacturer , sizeof(outer.manufacturer), 0},
        {"owner"       , ArgTypeString, NULL     , outer.owner        , sizeof(outer.owner)       , 0},
        {"civilcode"   , ArgTypeString, NULL     , outer.civilcode    , sizeof(outer.civilcode)   , 0},
        {"srvip"       , ArgTypeString, NULL     , outer.sip_srv_ip   , sizeof(outer.sip_srv_ip)  , 0},
        {"port"        , ArgTypeInt   , "1~65535", &outer.port        , sizeof(int)               , 0},
        {"srvid"       , ArgTypeString, NULL     , outer.srv_id       , sizeof(outer.srv_id)      , 0},
        {"devsysname"  , ArgTypeString, NULL     , outer.dev_sysname  , sizeof(outer.dev_sysname) , 0},
        {"devtype"     , ArgTypeString, NULL     , outer.dev_type     , sizeof(outer.dev_type)    , 0},
        {"devid"       , ArgTypeString, NULL     , outer.video_channal_id, sizeof(outer.video_channal_id)      , 0},
        {"alarmid"     , ArgTypeString, NULL     , outer.alarm_id     , sizeof(outer.alarm_id)    , 0},
        {"reginterval" , ArgTypeInt   , "1~65535", &outer.reg_interval, sizeof(int)               , 0},
        {"hbinterval"  , ArgTypeInt   , "0~600"  , &outer.hb_interval , sizeof(int)               , 0},
        {"authname"    , ArgTypeString, NULL     , outer.authname     , sizeof(outer.authname)    , 0},
        {"username"    , ArgTypeString, NULL     , outer.username     , sizeof(outer.username)    , 0},
        {"password"    , ArgTypeString, NULL     , outer.password     , sizeof(outer.password)    , 0},
        {"localport"     , ArgTypeInt   , "1025~65535", &outer.localport     , sizeof(int)        , 0},
        {"protocoltype"  , ArgTypeInt   , "0|1"       , &outer.protocoltype  , sizeof(int)        , 0},
        {"streamtype"    , ArgTypeInt   , "0|1"       , &outer.streamtype    , sizeof(int)        , 0},
        {"END"         , ArgTypeEnd   , NULL     , NULL               , 0                         , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "guobiaocfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(GuoBiaoS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(GuoBiaoS));
        ret = XmlConfSet(opts, "guobiaocfg", JEvent_GuoBiaoCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleGuoBiaoAddrCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    GBAddrS outer = {{0,},};
    GBAddrS inner = {{0,},};

    ArgOptS_T opts[] = {
        {"address"  , ArgTypeString, NULL  , outer.address   , sizeof(outer.address), 0},
        {"longitude", ArgTypeFloat, "0~180", &outer.longitude, sizeof(float)        , 0},
        {"latitude" , ArgTypeFloat, "0~90" , &outer.latitude , sizeof(float)        , 0},
        {"END"      , ArgTypeEnd  , NULL   , NULL            , 0                    , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "guobiaocfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(GBAddrS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(GBAddrS));
        ret = XmlConfSet(opts, "guobiaocfg", Jevent_GuoBiaoAddrCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleSim4gCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    Sim4g inner = {0};
    Sim4g outer = {0};

    ArgOptS_T opts[] = {
        {"fdd" , ArgTypeInt, "0~2"  , &outer.fdd , sizeof(int), 0},
        {"card", ArgTypeInt, "1|2|3", &outer.card, sizeof(int), 0},
        {"token",ArgTypeString, NULL, &outer.token, sizeof(outer.token), 0},
        {"End" , ArgTypeEnd, NULL , NULL       , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "sim4g", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(inner));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        CPY_INNER2BUF(inner, buf);
    } else if (!strncasecmp("set", (const char *)action, 3)) {
        memcpy(&outer, data, sizeof(int));
        DBG("send event 4g, fdd: %d\n", outer.fdd);
        ret = XmlConfSet(opts, "sim4g", JEvent_Sim4g, &outer, sizeof(outer));
    }

    return ret;
}

int handleDhcpNotify(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    if(!strncasecmp("set", (const char *)action,3)) {
        DhcpNotifyS dhcp = {0};
        if (data != NULL) {
            memcpy(&dhcp, data, sizeof(DhcpNotifyS));
        }
        DBG("interface: %s\n", dhcp.interface);
        if (strlen(dhcp.interface) <= 0 || strcmp(dhcp.interface, "usb0") == 0) {
            return 0;
        }
        send_conf_nake(JEvent_DhcpNotify);
    }

    return 0;
}

int handlesensorfps(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    int outer;

     ArgOptS_T opts[] = {
        {"fps"   , ArgTypeInt, "1~30"  , &outer  , sizeof(int), 0},
        {"End"   , ArgTypeEnd, NULL    , NULL    , 0          , 0},
    };
    memset(&outer, 0, sizeof(int));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("set", (const char *)action,3)) {
        //ret = IMP_ISP_Tuning_SetSensorFPS(outer, 1);
        DBG("Set sensor fps to %d\n", outer);
    }
    return ret;
}

int handleGpioCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    gpio_t outer = {0};
    gpio_t inner = {0};

    ArgOptS_T opts[] = {
        {"led_index", ArgTypeInt, "0|1", &outer.led_index, sizeof(outer.led_index)},
        {"ao_prompt", ArgTypeInt, "0|1", &outer.ao_prompt, sizeof(outer.ao_prompt)},
        {"END"      , ArgTypeEnd, NULL , NULL            , 0                      },
    };

    ret = SysConfCfg(ConfGet, opts, "gpiocfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(gpio_t));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(gpio_t));
        ret = XmlConfSet(opts, "gpiocfg", JEvent_Begin, &outer, sizeof(outer));
    }

    return ret;
}
int handlewhiteledCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    WhiteLedS outer = {0,};
    WhiteLedS inner = {0,};

    ArgOptS_T opts[] = {
        {"reverse", ArgTypeInt, "0|1", &outer.reverse, sizeof(outer.reverse), 0},
        {"End"    , ArgTypeEnd, NULL , NULL          , 0                    , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "WhiteLed", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(WhiteLedS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3))
    {
        JCONF_GET_STRUCT_T();
    }
    else if(!strncasecmp("set", (const char *)action,3))
    {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(WhiteLedS));
        ret = XmlConfSet(opts, "WhiteLed", JEvent_WhiteLedCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAudioOutCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    AudioOutCfg outer = {0};
    AudioOutCfg inner = {0};

    ArgOptS_T opts[] = {
        {"enable", ArgTypeInt  , "0|1"   , &outer.enable, sizeof(int), 0},
        {"volumn", ArgTypeInt  , "0~100" , &outer.volumn, sizeof(int), 0},
        {"End"   , ArgTypeEnd  , NULL    , NULL         , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "audioOut", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(AudioOutCfg));

    JCONF_CPY_DATA2OUTER(opts, data, outer);
    if(!strncasecmp("get", (const char *)action, 3)) {
        CPY_INNER2BUF(inner, buf);
    } else if (!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(AudioOutCfg));
        ret = XmlConfSet(opts, "audioOut", JEvent_AudioOutCfgChg, &outer, sizeof(outer));
    }

    return SUCCESS;
}

int handleVglineCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    VglineS outer = {0};
    VglineS inner = {0};
    char times[128] = {0};

    ArgOptS_T opts[] ={
          {"x0"          , ArgTypeInt   , "0~1920", &outer.x0    , sizeof(int)  , 0  },
          {"y0"          , ArgTypeInt   , "0~1080", &outer.y0    , sizeof(int)  , 0  },
          {"x1"          , ArgTypeInt   , "0~1920", &outer.x1    , sizeof(int)  , 0  },
          {"y1"          , ArgTypeInt   , "0~1080", &outer.y1    , sizeof(int)  , 0  },
          {"dx0"         , ArgTypeInt   , "0~1920", &outer.dx0   , sizeof(int)  , 0  },
          {"dy0"         , ArgTypeInt   , "0~1080", &outer.dy0   , sizeof(int)  , 0  },
          {"dx1"         , ArgTypeInt   , "0~1920", &outer.dx1   , sizeof(int)  , 0  },
          {"dy1"         , ArgTypeInt   , "0~1080", &outer.dy1   , sizeof(int)  , 0  },
          {"k"           , ArgTypeInt   , "1~12"  , &outer.k     , sizeof(int)  , 0  },
          {"enable"      , ArgTypeInt   , "0|1"   , &outer.enable, sizeof(int)  , 0  },
          {"blink"       , ArgTypeInt   , "0|1"   , &outer.blink , sizeof(int)  , 0  },
          {"indoor"      , ArgTypeInt   , "0|1"   , &outer.indoor, sizeof(int)  , 0  },
          {"thresh"      , ArgTypeInt   , "0~100" , &outer.thresh, sizeof(int)  , 0  },
          {"dir"         , ArgTypeInt   , "0~2"     , &outer.dir   , sizeof(int)  , 0  },
          {"timestrategy", ArgTypeString, NULL    , times        , sizeof(times), "0"},
          {"END"         , ArgTypeEnd   , NULL    , NULL         , 0            , 0  },
    };

    ret = SysConfCfg(ConfGet, opts, "vgline", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(VglineS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(VglineS));

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_VGLINE);
        }

        ret = XmlConfSet(opts, "vgline", JEvent_VglineCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleVgrectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    VgrectS outer = {0};
    VgrectS inner = {0};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"x0"          , ArgTypeInt   , "0~1920", &outer.x0    , sizeof(int)  , 0  },
        {"y0"          , ArgTypeInt   , "0~1080", &outer.y0    , sizeof(int)  , 0  },
        {"x1"          , ArgTypeInt   , "0~1920", &outer.x1    , sizeof(int)  , 0  },
        {"y1"          , ArgTypeInt   , "0~1080", &outer.y1    , sizeof(int)  , 0  },
        {"x2"          , ArgTypeInt   , "0~1920", &outer.x2    , sizeof(int)  , 0  },
        {"y2"          , ArgTypeInt   , "0~1080", &outer.y2    , sizeof(int)  , 0  },
        {"x3"          , ArgTypeInt   , "0~1920", &outer.x3    , sizeof(int)  , 0  },
        {"y3"          , ArgTypeInt   , "0~1080", &outer.y3    , sizeof(int)  , 0  },
        {"enable"      , ArgTypeInt   , "0|1"   , &outer.enable, sizeof(int)  , 0  },
        {"blink"       , ArgTypeInt   , "0|1"   , &outer.blink , sizeof(int)  , 0  },
        {"indoor"      , ArgTypeInt   , "0|1"   , &outer.indoor, sizeof(int)  , 0  },
        {"dir"         , ArgTypeInt   , "0~2"   , &outer.dir   , sizeof(int)  , 0  },
        {"thresh"      , ArgTypeInt   , "0~100" , &outer.thresh, sizeof(int)  , 0  },
        {"timestrategy", ArgTypeString, NULL    , times        , sizeof(times), "0"},
        {"END"         , ArgTypeEnd   , NULL    , NULL         , 0            , 0  },
    };

    ret = SysConfCfg(ConfGet, opts, "vgrect", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(VgrectS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(VgrectS));

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_VGRECT);
        }

        ret = XmlConfSet(opts, "vgrect", JEvent_VgrectCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAlarmAudioTypeCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    alarm_audio_t outer = {0};
    alarm_audio_t inner = {0};

    ArgOptS_T opts[] = {
        {"alarm_type"   , ArgTypeInt, "0~10", &outer.audio_type , sizeof(int), 0},
        {"time"         , ArgTypeInt, "0~60", &outer.time     , sizeof(int), 0},
        {"END"          , ArgTypeEnd, NULL    , NULL    , 0             },
    };

    ret = SysConfCfg(ConfGet, opts, "alarm_auido", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(alarm_audio_t));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(alarm_audio_t));
        ret = XmlConfSet(opts, "alarm_auido", JEvent_AlarmAudioCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleMotorCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    motor_t outer = {0};
    motor_t inner = {0};

    ArgOptS_T opts[] = {
        {"type"             , ArgTypeInt, "0~9" , &outer.type           , sizeof(outer.type)        },
        {"reverse"          , ArgTypeInt, "1~4" , &outer.reverse        , sizeof(outer.reverse)     },
        {"o_speed"          , ArgTypeInt, NULL  , &outer.o_speed        , sizeof(outer.o_speed)     },
        {"o_seconds"        , ArgTypeInt, NULL  , &outer.o_seconds      , sizeof(outer.o_seconds)   },
        {"h_maxstep"        , ArgTypeInt, NULL  , &outer.h_maxstep      , sizeof(outer.h_maxstep)   },
        {"v_maxstep"        , ArgTypeInt, NULL  , &outer.v_maxstep      , sizeof(outer.v_maxstep)   },
        {"h_speed"          , ArgTypeInt, NULL  , &outer.h_speed        , sizeof(outer.h_speed)     },
        {"v_speed"          , ArgTypeInt, NULL  , &outer.v_speed        , sizeof(outer.v_speed)     },
        {"h_startstep"      , ArgTypeInt, NULL  , &outer.h_Startstep    , sizeof(outer.h_Startstep) },
        {"v_startstep"      , ArgTypeInt, NULL  , &outer.v_Startstep    , sizeof(outer.v_Startstep) },
        {"waittime"         , ArgTypeInt, NULL  , &outer.WaitTime       , sizeof(outer.WaitTime)    },
        {"seqtimes"         , ArgTypeInt, NULL  , &outer.SeqTimes       , sizeof(outer.SeqTimes)    },
        {"max_h_Angle"      , ArgTypeInt, NULL  , &outer.max_h_Angle    , sizeof(outer.max_h_Angle) },
        {"max_v_Angle"      , ArgTypeInt, NULL  , &outer.max_v_Angle    , sizeof(outer.max_v_Angle) },
        {"max_h_ViewAngle"  , ArgTypeInt, NULL  , &outer.max_h_ViewAngle, sizeof(outer.max_h_ViewAngle )},
        {"max_v_ViewAngle"  , ArgTypeInt, NULL  , &outer.max_v_ViewAngle, sizeof(outer.max_v_ViewAngle )},
        {"END"              , ArgTypeEnd, NULL  , NULL                  , 0                     },
    };

    ret = SysConfCfg(ConfGet, opts, "motorcfg", JEvent_MotorCfg);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(motor_t));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(motor_t));
        ret = XmlConfSet(opts, "motorcfg", JEvent_MotorCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handlePelcodCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    PelcodCfg outer;
    PelcodCfg inner;
    memset(&outer, 0, sizeof(PelcodCfg));
    memset(&inner, 0, sizeof(PelcodCfg));

    ArgOptS_T opts[] = {
        {"x"     , ArgTypeInt, x_scope(), &outer.x     , sizeof(int), 0},
        {"y"     , ArgTypeInt, y_scope(), &outer.y     , sizeof(int), 0},
        {"revert", ArgTypeInt, "0~3"    , &outer.revert, sizeof(int), 0},
        {"END"   , ArgTypeEnd, NULL     , NULL         , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "pelcodcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(PelcodCfg));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(PelcodCfg));
        ret = XmlConfSet(opts, "pelcodcfg", JEvent_PelcodCfg, &outer, sizeof(outer));
    }
    return ret;
}

int handlePreSetNewCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    ArgOpt     *extOpts = NULL;
    presetcfg geter = {0,};
    presetcfg inner = {0,};
    presetlist tmpIn = {0,};
    void *argOpt[] = {(void*)inner.preset, (void*)&tmpIn };
    ArgOpt optspara[] = {
        {"id"       , ArgTypeInt   , "0~2147483647"     , (void*)&tmpIn.id          },
        {"x"        , ArgTypeInt   , x_scope()          , (void*)&tmpIn.x           },
        {"y"        , ArgTypeInt   , y_scope()          , (void*)&tmpIn.y           },
        {"isdefault", ArgTypeInt   , "0|1"              , (void*)&tmpIn.isdefault   },
        {"enable"   , ArgTypeInt   , "0|1"              , (void*)&tmpIn.enable      },
        {"name"     , ArgTypeString, NULL               , (void*)tmpIn.name     },
        {"End"      , ArgTypeEnd   , NULL               , NULL                      },
    };

    ArgOpt optslist[] = {
        {"preset", ArgTypeTree, NULL, (void*)optspara, PresetlistMapsCbFunc, argOpt},
        {"End"   , ArgTypeEnd , NULL, NULL                                        },
    };

    CALLOC_EXT_STRUCT(ArgOpt, extOpts, MAX_PRESET_NUM, optslist);

    ArgOpt opts[] = {
        {"gnum"         , ArgTypeInt , "1~8", (void*)&inner.gnum },
        {"presetlist"   , ArgTypeTree, NULL , (void*)extOpts    },
        {"End"          , ArgTypeEnd , NULL , NULL              },
    };

    JCONF_STRUCT_PARSER_CALLOC();

    ret = SysConfPresetCfg(ConfGet, opts, &geter);
    RETURN_FAIL_IF_XML_ERR_CALLOC(ret);
    DBG("inner.gnum:%d\n", inner.gnum);
    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_CALLOC();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ_CALLOC(&inner, &geter, sizeof(presetcfg));
        ret = SysConfPresetCfg(ConfSet, opts, &inner);
    }
    FREE_EXT_STRUCT();
    return ret;
}

int handleDaynightCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    DaynightCfgS outer = {0,};
    DaynightCfgS inner = {0,};

    ArgOptS_T opts[] = {
        {"mode", ArgTypeInt , "0|1"     , &outer.mode, sizeof(int), 0},
        {"type"      , ArgTypeInt , "0|1"     , &outer.type       , sizeof(int), 0},
        {"reverse"   , ArgTypeInt , "0|1"     , &outer.reverse    , sizeof(int), 0},
        {"nighttoday", ArgTypeInt, "0~65535"   , &outer.nighttoday, sizeof(int), 0},
        {"daytonight", ArgTypeInt, "0~65535"   , &outer.daytonight, sizeof(int), 0},
        {"nighttoday_mode1", ArgTypeInt, "0~65535"   , &outer.nighttoday_mode1, sizeof(int), 0},
        {"daytonight_mode1", ArgTypeInt, "0~65535"   , &outer.daytonight_mode1, sizeof(int), 0},
        {"End"   , ArgTypeEnd , NULL      , NULL         , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "daynight", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(DaynightCfgS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(DaynightCfgS));
        ret = XmlConfSet(opts, "daynight", JEvent_Daynightcfg, &outer, sizeof(outer));
    }

    return ret;
}

int handleFollowCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    follow_info_t outer = {0,};
    follow_info_t inner = {0,};
    char times[128] = {0};

    ArgOptS_T opts[] = {
        {"enable",       ArgTypeInt,    "0|1",    &outer.enable,        sizeof(int),   0},
        {"humanenable",  ArgTypeInt,    "0|1",    &outer.humanenable,   sizeof(int),   0},
        {"carenable",    ArgTypeInt,    "0|1",    &outer.carenable,     sizeof(int),   0},
        {"petenable",    ArgTypeInt,    "0|1",    &outer.petenable,     sizeof(int),   0},
        {"screenenable", ArgTypeInt,    "0|1",    &outer.screenenable,  sizeof(int),   0},
        {"zoom",         ArgTypeInt,    "0|1",    &outer.zoom,          sizeof(int),   0},
        {"thresh",       ArgTypeInt,    "0~100",  &outer.thresh,        sizeof(int),   0},
        {"idle"  ,       ArgTypeInt,    "0~3600", &outer.idle,          sizeof(int),   0},
        {"preset",       ArgTypeInt,    "0~255",  &outer.preset,        sizeof(int),   0},
        {"reverse",      ArgTypeInt,    "0~3",    &outer.reverse,       sizeof(int),   0},
        {"times" ,       ArgTypeString, NULL,     times,                sizeof(times), "0:10,1:10,2:10,3:10,4:10,5:10,6:10;"},
        {"End"   ,       ArgTypeEnd ,   NULL,     NULL,                 0,             0},
    };

    ret = SysConfCfg(ConfGet, opts, "followcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    timestr_to_intarray(times, outer.times);
    memcpy(&inner, &outer, sizeof(follow_info_t));
    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        if (outer.enable) {
            outer.humanenable = 1;
            outer.petenable = 1;
        }
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        intarray_to_timestr(times, outer.times);
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(follow_info_t));

        if (outer.enable == 1) {
            DoDetectMutexRelationConfig(E_DET_FOLLOW);
        }

        ret = XmlConfSet(opts, "followcfg", JEvent_Followcfg, &outer, sizeof(outer));
    }
    return ret;
}

int handleOnvifInfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    OnvifInfoCfg outer = {0};
    OnvifInfoCfg inner = {0};

    ArgOptS_T opts[] = {
        {"name"         , ArgTypeString, NULL   , outer.name            , sizeof(outer.name)        , "0"},
        {"hardware"     , ArgTypeString, NULL   , outer.hardware        , sizeof(outer.hardware)    , "0"},
        {"location"     , ArgTypeString, NULL   , outer.location        , sizeof(outer.location)    , "0"},
        {"discoverable" , ArgTypeInt, "0~1"     , &outer.discoverable   , sizeof(int)               , 0  },
        {"searchable"   , ArgTypeInt, "0~1"     , &outer.searchable     , sizeof(int)               , 0  },
        {"lastreboot"   , ArgTypeString, NULL   , outer.lastreboot      , sizeof(outer.lastreboot)  , "0"},
        {"END"          , ArgTypeEnd   , NULL   , NULL                  , 0                         , 0  },
    };

    ret = SysConfCfg(ConfGet, opts, "onvifinfo", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(OnvifInfoCfg));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(OnvifInfoCfg));
        ret = XmlConfSet(opts, "onvifinfo", JEvent_OnvifInfoCfg, &outer, sizeof(outer));
    }

    return ret;
}

int handlePrivCtrlCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    static priv_ctrl_t info = {0};

    priv_ctrl_t outer = {0};
    priv_ctrl_t inner = {0};

    ArgOptS_T opts[] = {
        {"video", ArgTypeInt, "0|1", (void*)&outer.video, sizeof(int)},
        {"End"  , ArgTypeEnd, NULL , NULL               , 0          },
    };

    memcpy(&outer, &info, sizeof(priv_ctrl_t));
    memcpy(&inner, &outer, sizeof(priv_ctrl_t));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_T();
    } else if (!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&info, &outer, sizeof(priv_ctrl_t));
        memcpy(&info, &outer, sizeof(priv_ctrl_t));
        send_conf_nake(JEvent_PrivCtrl);
    }

    return ret;
}

int handleDevConf(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    DevConfS outer = {0};
    DevConfS inner = {0};

    ArgOptS_T opts[] = {
        {"devicebind"    , ArgTypeInt   , "0|1"     , &outer.devicebind    , sizeof(int), 0},
        {"definition"    , ArgTypeInt   , "0|1|2"   , &outer.definition    , sizeof(int), 0},
        {"END"           , ArgTypeEnd   , NULL      , NULL                 , 0          , 0},
    };

    ret = SysConfCfg(ConfGet, opts, "devconf", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(DevConfS));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);
    if(!strncasecmp("get", (const char *)action, 3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(DevConfS));
        ret = XmlConfSet(opts, "devconf", JEvent_DevCfg, &outer, sizeof(outer));
    }

    return SUCCESS;
}

int handleAppveCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    Appvecfg outer = {0};
    Appvecfg inner = {0};

    ArgOptS_T opts[] = {
        {"w_min"   , ArgTypeString, NULL   , (void *)outer.appve[0].appves , sizeof(outer.appve[0].appves)},
        {"w_mid"   , ArgTypeString, NULL   , (void *)outer.appve[1].appves , sizeof(outer.appve[1].appves)},
        {"w_max"   , ArgTypeString, NULL   , (void *)outer.appve[2].appves , sizeof(outer.appve[2].appves)},
        {"imaxqp_0", ArgTypeInt   , "20~51", (void *)&outer.imaxqp[0]      , sizeof(int)                  },
        {"iminqp_0", ArgTypeInt   , "10~40", (void *)&outer.iminqp[0]      , sizeof(int)                  },
        {"imaxqp_1", ArgTypeInt   , "20~51", (void *)&outer.imaxqp[1]      , sizeof(int)                  },
        {"iminqp_1", ArgTypeInt   , "10~40", (void *)&outer.iminqp[1]      , sizeof(int)                  },
        {"webxvsz" , ARG_INT_RD_ONLY   , "0~16" , (void *)&outer.webxvsz   , sizeof(int)                  },
        {"appxvsz" , ARG_INT_RD_ONLY   , "0~16" , (void *)&outer.appxvsz   , sizeof(int)                  },
        {"End"     , ArgTypeEnd   , NULL   , NULL                          , 0                            },
    };

    ret = SysConfCfg(ConfGet, opts, "appvecfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(Appvecfg));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(Appvecfg));
        ret = XmlConfSet(opts, "appvecfg", JEvent_AppveCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAlarmInfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    AlarmInfocfg outer = {0};
    AlarmInfocfg inner = {0};

    ArgOptS_T opts[] = {
        {"interval", ArgTypeInt, "60~3600", &outer.interval, sizeof(outer.interval)},
        {"End"     , ArgTypeEnd, NULL     , NULL           , 0                     },
    };

    ret = SysConfCfg(ConfGet, opts, "alarminfocfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(AlarmInfocfg));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if (!strncasecmp("get", (const char *)action,3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(AlarmInfocfg));
        ret = XmlConfSet(opts, "alarminfocfg", JEvent_AlarmInfoCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleConvergenceCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;

    sFacialConvergence outer = {0};
    sFacialConvergence inner = {0};

    ArgOptS_T opts[] = {
        {"wh_iso1"          , ArgTypeInt  , "0~10000"   , &outer.wh.iso1         , sizeof(outer.wh.iso1)         },
        {"wh_iso2"          , ArgTypeInt  , "0~10000"   , &outer.wh.iso2         , sizeof(outer.wh.iso2)         },
        {"wh_iso3"          , ArgTypeInt  , "0~10000"   , &outer.wh.iso3         , sizeof(outer.wh.iso3)         },
        {"ir_iso1"          , ArgTypeInt  , "0~10000"   , &outer.ir.iso1         , sizeof(outer.ir.iso1)         },
        {"ir_iso2"          , ArgTypeInt  , "0~10000"   , &outer.ir.iso2         , sizeof(outer.ir.iso2)         },
        {"ir_iso3"          , ArgTypeInt  , "0~10000"   , &outer.ir.iso3         , sizeof(outer.ir.iso3)         },
        {"wh_area1"         , ArgTypeFloat, "0~1000000.00", &outer.wh.area1        , sizeof(outer.wh.area1)        },
        {"wh_area2"         , ArgTypeFloat, "0~1000000.00", &outer.wh.area2        , sizeof(outer.wh.area2)        },
        {"wh_area3"         , ArgTypeFloat, "0~1000000.00", &outer.wh.area3        , sizeof(outer.wh.area3)        },
        {"ir_area1"         , ArgTypeFloat, "0~1000000.00", &outer.ir.area1        , sizeof(outer.ir.area1)        },
        {"ir_area2"         , ArgTypeFloat, "0~1000000.00", &outer.ir.area2        , sizeof(outer.ir.area2)        },
        {"ir_area3"         , ArgTypeFloat, "0~1000000.00", &outer.ir.area3        , sizeof(outer.ir.area3)        },
        {"wh_compensation1" , ArgTypeInt  , "0~10000"   , &outer.wh.compensation1, sizeof(outer.wh.compensation1)},
        {"wh_compensation2" , ArgTypeInt  , "0~10000"   , &outer.wh.compensation2, sizeof(outer.wh.compensation2)},
        {"wh_compensation3" , ArgTypeInt  , "0~10000"   , &outer.wh.compensation3, sizeof(outer.wh.compensation3)},
        {"ir_compensation1" , ArgTypeInt  , "0~10000"   , &outer.ir.compensation1, sizeof(outer.ir.compensation1)},
        {"ir_compensation2" , ArgTypeInt  , "0~10000"   , &outer.ir.compensation2, sizeof(outer.ir.compensation2)},
        {"ir_compensation3" , ArgTypeInt  , "0~10000"   , &outer.ir.compensation3, sizeof(outer.ir.compensation3)},
        {"END"              , ArgTypeEnd  , NULL        , NULL                   , 0                             },
    };

    ret = SysConfCfg(ConfGet, opts, "convergence", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);
    memcpy(&inner, &outer, sizeof(sFacialConvergence));

    RETURN_FAIL_IF_OUTER_INVALID(opts, data, outer);

    if(!strncasecmp("get", (const char *)action,3)) {
        JCONF_GET_STRUCT_T();
    } else if(!strncasecmp("set", (const char *)action,3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(sFacialConvergence));
        ret = XmlConfSet(opts, "convergence", JEvent_ConvergenceChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAiVqeV2Cfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    sAiVqeV2Cfg outer = {0};
    sAiVqeV2Cfg inner = {0};
    sVqeV2PnrCfg *p_pnr = &outer.pnr_cfg;
    sVqeV2NrCfg  *p_nr  = &outer.nr_cfg;
    sVqeV2AgcCfg *p_agc = &outer.agc_cfg;
    sVqeV2FmpCfg *p_fmp = &outer.fmp_cfg;
    sVqeV2AecCfg *p_aec = &outer.aec_cfg;
    sVqeV2WnrCfg *p_wnr = &outer.wnr_cfg;
    sVqeV2HsCfg  *p_hs  = &outer.hs_cfg;

    ArgOptS_T opts[] = {
        {"pnr_enable"                  , ArgTypeInt , "0|1"       , &outer.pnr_cfg.enable           , sizeof(int)   },
        {"pnr_usr_mode"                , ArgTypeInt , "0|1"       , &p_pnr->usr_mode                , sizeof(int)   },
        {"pnr_min_gain_limit"          , ArgTypeInt , "0~32767"   , &p_pnr->min_gain_limit          , sizeof(int)   },
        {"pnr_snr_prior_limit"         , ArgTypeInt , "0~32767"   , &p_pnr->snr_prior_limit         , sizeof(int)   },
        {"pnr_ht_threshold"            , ArgTypeInt , "0~80"      , &p_pnr->ht_threshold            , sizeof(int)   },
        {"pnr_hs_threshold"            , ArgTypeInt , "100~1100"  , &p_pnr->hs_threshold            , sizeof(int)   },
        {"pnr_alpha_ph"                , ArgTypeInt , "0~100"     , &p_pnr->alpha_ph                , sizeof(int)   },
        {"pnr_alpha_psd"               , ArgTypeInt , "0~100"     , &p_pnr->alpha_psd               , sizeof(int)   },
        {"pnr_prior_snr_fixed"         , ArgTypeInt , "1~99"      , &p_pnr->prior_snr_fixed         , sizeof(int)   },
        {"pnr_cep_threshold"           , ArgTypeInt , "0~100"     , &p_pnr->cep_threshold           , sizeof(int)   },
        {"pnr_cep_amp"                 , ArgTypeInt , "100~1000"  , &p_pnr->cep_amp                 , sizeof(int)   },
        {"pnr_low_freq_protect"        , ArgTypeInt , "0|1"       , &p_pnr->low_freq_protect        , sizeof(int)   },
        {"pnr_speech_protect_threshold", ArgTypeInt , "0~100"     , &p_pnr->speech_protect_threshold, sizeof(int)   },
        {"pnr_hem_enable"              , ArgTypeInt , "0|1"       , &p_pnr->hem_enable              , sizeof(int)   },
        {"pnr_tcs_enable"              , ArgTypeInt , "0|1"       , &p_pnr->tcs_enable              , sizeof(int)   },

        {"nr_enable"                   , ArgTypeInt , "0|1"       , &outer.nr_cfg.enable            , sizeof(int)   },
        {"nr_usr_mode"                 , ArgTypeInt , "0|1"       , &p_nr->usr_mode                 , sizeof(int)   },
        {"nr_min_gain_limit"           , ArgTypeInt , "1~32767"   , &p_nr->min_gain_limit           , sizeof(int)   },
        {"nr_snr_prior_limit"          , ArgTypeInt , "1~32767"   , &p_nr->snr_prior_limit          , sizeof(int)   },
        {"nr_ht_threshold"             , ArgTypeInt , "0~1000"    , &p_nr->ht_threshold             , sizeof(int)   },
        {"nr_hs_threshold"             , ArgTypeInt , "100~1100"  , &p_nr->hs_threshold             , sizeof(int)   },
        {"nr_cep_threshold"            , ArgTypeInt , "0~100"     , &p_nr->cep_threshold            , sizeof(int)   },
        {"nr_cep_amp"                  , ArgTypeInt , "0~1000"    , &p_nr->cep_amp                  , sizeof(int)   },
        {"nr_prior_snr"                , ArgTypeInt , "0~20"      , &p_nr->prior_snr                , sizeof(int)   },
        {"nr_snr_smooth_factor"        , ArgTypeInt , "5000~10000", &p_nr->snr_smooth_factor        , sizeof(int)   },
        {"nr_speech_prob_smooth_factor", ArgTypeInt , "5000~10000", &p_nr->speech_prob_smooth_factor, sizeof(int)   },
        {"nr_noise_pwr_smooth_factor"  , ArgTypeInt , "5000~10000", &p_nr->noise_pwr_smooth_factor  , sizeof(int)   },
        {"nr_low_freq_suppress_enable" , ArgTypeInt , "0|1"       , &p_nr->low_freq_suppress_enable , sizeof(int)   },
        {"nr_low_freq_gain_suppress"   , ArgTypeInt , "0~2"       , &p_nr->low_freq_gain_suppress   , sizeof(int)   },
        {"nr_env_mode"                 , ArgTypeInt , "0|1"       , &p_nr->env_mode                 , sizeof(int)   },
        {"nr_cep_alpha"                , ArgTypeInt , "0~100"     , &p_nr->cep_alpha                , sizeof(int)   },
        {"nr_gain_sm_mode"             , ArgTypeInt , "0|1"       , &p_nr->gain_sm_mode             , sizeof(int)   },
        {"nr_gain_sm_alpha1"           , ArgTypeInt , "0~100"     , &p_nr->gain_sm_alpha1           , sizeof(int)   },
        {"nr_gain_sm_alpha2"           , ArgTypeInt , "0~100"     , &p_nr->gain_sm_alpha2           , sizeof(int)   },
        {"nr_gain_sm_alpha3"           , ArgTypeInt , "0~100"     , &p_nr->gain_sm_alpha3           , sizeof(int)   },

        {"agc_enable"                  , ArgTypeInt , "0|1"       , &outer.agc_cfg.enable           , sizeof(int)   },
        {"agc_usr_mode"                , ArgTypeInt , "0|1"       , &p_agc->usr_mode                , sizeof(int)   },
        {"agc_target_level"            , ArgTypeInt , "0~120"     , &p_agc->target_level            , sizeof(int)   },
        {"agc_max_gain"                , ArgTypeInt , "0~360"     , &p_agc->max_gain                , sizeof(int)   },
        {"agc_min_gain"                , ArgTypeInt , "0~120"     , &p_agc->min_gain                , sizeof(int)   },
        {"agc_up_gradient_ratio"       , ArgTypeInt , "1~30"      , &p_agc->up_gradient_ratio       , sizeof(int)   },
        {"agc_down_gradient_ratio"     , ArgTypeInt , "1~30"      , &p_agc->down_gradient_ratio     , sizeof(int)   },
        {"agc_decay"                   , ArgTypeInt , "0~650"     , &p_agc->decay                   , sizeof(int)   },
        {"agc_vad_threshold"           , ArgTypeInt , "0~1024"    , &p_agc->vad_threshold           , sizeof(int)   },
        {"agc_vad_ctrl"                , ArgTypeInt , "0|1"       , &p_agc->vad_ctrl                , sizeof(int)   },

        {"fmp_enable"                  , ArgTypeInt , "0|1"       , &outer.fmp_cfg.enable           , sizeof(int)   },
        {"fmp_usr_mode"                , ArgTypeInt , "0|1"       , &p_fmp->usr_mode                , sizeof(int)   },
        {"fmp_comfort_flag"            , ArgTypeInt , "0|1"       , &p_fmp->comfort_flag            , sizeof(int)   },
        {"fmp_comfort_intensity"       , ArgTypeInt , "1~10"      , &p_fmp->comfort_intensity       , sizeof(int)   },

        {"wnr_enable"                  , ArgTypeInt , "0|1"       , &outer.wnr_cfg.enable           , sizeof(int)   },
        {"wnr_usr_mode"                , ArgTypeInt , "0|1"       , &p_wnr->usr_mode                , sizeof(int)   },
        {"wnr_min_gain_limit"          , ArgTypeInt , "1~8"       , &p_wnr->min_gain_limit          , sizeof(int)   },

        {"aec_enable"                  , ArgTypeInt , "0|1"       , &outer.aec_cfg.enable           , sizeof(int)   },
        {"aec_usr_mode"                , ArgTypeInt , "0|1"       , &p_aec->usr_mode                , sizeof(int)   },
        {"aec_pure_delay"              , ArgTypeInt , "0~300"     , &p_aec->pure_delay              , sizeof(int)   },
        {"aec_switch_nlp"              , ArgTypeInt , "0|1"       , &p_aec->switch_nlp              , sizeof(int)   },
        {"aec_band1"                   , ArgTypeInt , "0~6000"    , &p_aec->band1                   , sizeof(int)   },
        {"aec_band2"                   , ArgTypeInt , "0~6000"    , &p_aec->band2                   , sizeof(int)   },
        {"aec_band3"                   , ArgTypeInt , "0~6000"    , &p_aec->band3                   , sizeof(int)   },
        {"aec_band4"                   , ArgTypeInt , "0~6000"    , &p_aec->band4                   , sizeof(int)   },
        {"aec_gain_lower_limit1"       , ArgTypeInt , "0~100"     , &p_aec->gain_lower_limit1       , sizeof(int)   },
        {"aec_gain_lower_limit2"       , ArgTypeInt , "0~100"     , &p_aec->gain_lower_limit2       , sizeof(int)   },
        {"aec_gain_lower_limit3"       , ArgTypeInt , "0~100"     , &p_aec->gain_lower_limit3       , sizeof(int)   },
        {"aec_gain_lower_limit4"       , ArgTypeInt , "0~100"     , &p_aec->gain_lower_limit4       , sizeof(int)   },
        {"aec_gain_lower_limit5"       , ArgTypeInt , "0~100"     , &p_aec->gain_lower_limit5       , sizeof(int)   },
        {"aec_ols_on"                  , ArgTypeInt , "0|1"       , &p_aec->ols_on                  , sizeof(int)   },
        {"aec_speaker_nl_on"           , ArgTypeInt , "0|1"       , &p_aec->speaker_nl_on           , sizeof(int)   },
        {"aec_block_num"               , ArgTypeInt , "3~30"      , &p_aec->block_num               , sizeof(int)   },
        {"aec_echo_boost1"             , ArgTypeInt , "1~100"     , &p_aec->echo_boost1             , sizeof(int)   },
        {"aec_echo_boost2"             , ArgTypeInt , "1~100"     , &p_aec->echo_boost2             , sizeof(int)   },
        {"aec_echo_boost3"             , ArgTypeInt , "1~100"     , &p_aec->echo_boost3             , sizeof(int)   },
        {"aec_echo_boost4"             , ArgTypeInt , "1~100"     , &p_aec->echo_boost4             , sizeof(int)   },
        {"aec_echo_boost5"             , ArgTypeInt , "1~100"     , &p_aec->echo_boost5             , sizeof(int)   },

        {"hs_enable"                   , ArgTypeInt , "0|1"       , &outer.hs_cfg.enable            , sizeof(int)   },
        {"hs_usr_mode"                 , ArgTypeInt , "0|1"       , &p_hs->usr_mode                 , sizeof(int)   },
        {"hs_hold_time"                , ArgTypeInt , "0~1000"    , &p_hs->hold_time                , sizeof(int)   },
        {"hs_min_gain"                 , ArgTypeInt , "0~100"     , &p_hs->min_gain                 , sizeof(int)   },
        {"hs_threshold"                , ArgTypeInt , "0~50"      , &p_hs->threshold                , sizeof(int)   },
        {"hs_smooth_time"              , ArgTypeInt , "0~1000"    , &p_hs->smooth_time              , sizeof(int)   },
        {"hs_freq_move"                , ArgTypeInt , "0~40"      , &p_hs->freq_move                , sizeof(int)   },

        {"End"                         , ArgTypeEnd , NULL        , NULL                            , 0             },
    };

    ret = SysConfCfg(ConfGet, opts, "aivqev2cfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    memcpy(&inner, &outer, sizeof(sAiVqeV2Cfg));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(sAiVqeV2Cfg));
        ret = XmlConfSet(opts, "aivqev2cfg", JEvent_AiVqeV2CfgChg, &outer, sizeof(outer));
    }

    return ret;
}

int handleAiSpeexCfg(void *data, void *buf, int bufSize, int *bufLen, void *action)
{
    int ret = SUCCESS;
    sAiSpeexCfg outer = {0};
    sAiSpeexCfg inner = {0};

    ArgOptS_T opts[] = {
        {"agc_enable"          , ArgTypeInt  , "0|1"           , &outer.agc_enable          , sizeof(int)   },
        {"agc_level"           , ArgTypeFloat, "8000.0~24000.0", &outer.agc_level           , sizeof(float) },
        {"agc_max_gain"        , ArgTypeInt  , "10~40"         , &outer.agc_max_gain        , sizeof(int)   },
        {"agc_increment"       , ArgTypeInt  , "1~100"         , &outer.agc_increment       , sizeof(int)   },
        {"agc_decrement"       , ArgTypeInt  , "1~100"         , &outer.agc_decrement       , sizeof(int)   },

        {"nr_enable"           , ArgTypeInt  , "0|1"           , &outer.nr_enable           , sizeof(int)   },
        {"nr_decrement"        , ArgTypeInt  , "5~90"          , &outer.nr_decrement        , sizeof(int)   },

        {"aec_enable"          , ArgTypeInt  , "0|1"           , &outer.aec_enable          , sizeof(int)   },
        {"aec_filter_len"      , ArgTypeInt  , "320~2560"      , &outer.aec_filter_len      , sizeof(int)   },
        {"aec_suppress"        , ArgTypeInt  , "1~60"          , &outer.aec_suppress        , sizeof(int)   },
        {"aec_suppress_active" , ArgTypeInt  , "1~60"          , &outer.aec_suppress_active , sizeof(int)   },

        {"End"                 , ArgTypeEnd  , NULL            , NULL                       , 0             },
    };

    ret = SysConfCfg(ConfGet, opts, "aispeexcfg", JEvent_Begin);
    RETURN_FAIL_IF_XML_ERR(ret);

    memcpy(&inner, &outer, sizeof(sAiSpeexCfg));

    JCONF_CPY_DATA2OUTER(opts, data, outer);

    if (!strncasecmp("get", (const char *)action, 3)) {
        CPY_INNER2BUF(inner, buf);
    } else if(!strncasecmp("set", (const char *)action, 3)) {
        RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(sAiSpeexCfg));
        ret = XmlConfSet(opts, "aispeexcfg", JEvent_AiSpeexCfgChg, &outer, sizeof(outer));
    }

    return ret;
}

/* 后面的函数指针，只是作为一个参照 */
JconfCmdCbS confCmd[] ={
    {JEvent_EthcfgChg          , (cmdCbFunc)handleEthCfg           },
    {JEvent_SysInfoCfgChg      , (cmdCbFunc)handleSysInfoCfg       },
    {JEvent_PtzSerialCfgChg    , (cmdCbFunc)handlePtzSerialCfg     },
    {JEvent_NtpcfgChg          , (cmdCbFunc)handleNtpcfg           },
    {JEvent_TimeZoneCfgChg     , (cmdCbFunc)handleTimeZoneCfg      },
    {JEvent_TimeChange         , (cmdCbFunc)NULL                   },
    {JEvent_DayNightTypeChg    , (cmdCbFunc)NULL                   },
    {JEvent_VideoCfgChg        , (cmdCbFunc)handleVideoCfg         },
    {JEvent_VideoCallCfg       , (cmdCbFunc)handleVideoCallCfg     },
    {JEvent_OsdExpandCfgChg    , (cmdCbFunc)handleOsdExpandCfg     },
    {JEvent_VideoMaskCfgChg    , (cmdCbFunc)handleVideoMaskCfg     },
    {JEvent_VideoMaskPlanCfg   , (cmdCbFunc)handleVideoMaskPlanCfg },
    {JEvent_EmailCfgChg        , (cmdCbFunc)handleEmailCfg         },

    {JEvent_WifiCfgChg         , (cmdCbFunc)handleWifiCfg          },
    {JEvent_MotionDetLinkCfgChg, (cmdCbFunc)handleMotionDetLinkCfg },
    {JEvent_HumanDetLinkCfgChg , (cmdCbFunc)handleHumanDetLinkCfg  },
    {JEvent_VglineLinkCfgChg   , (cmdCbFunc)handleVglineLinkCfg    },
    {JEvent_VgrectLinkCfgChg   , (cmdCbFunc)handleVgrectLinkCfg    },
    {JEvent_CarDetLinkCfgChg   , (cmdCbFunc)handleCarDetLinkCfg    },
    {JEvent_PetDetLinkCfgChg   , (cmdCbFunc)handlePetDetLinkCfg    },
    {JEvent_CryDetLinkCfgChg   , (cmdCbFunc)handleCryDetLinkCfg    },
    {JEvent_AudioOutCfgChg     , (cmdCbFunc)handleAudioOutCfg      },
    {JEvent_IrCtrlCfgChg       , (cmdCbFunc)handleIrCtrlCfg        },
    {JEvent_LightCfgChg        , (cmdCbFunc)handleLightCfg         },
    {JEvent_LightExtCfgChg     , (cmdCbFunc)handleLightExtCfg      },
    {JEvent_UpnpCfgChg         , (cmdCbFunc)handleUpnpCfg          },
    {JEvent_OsdStyleCfgChg     , (cmdCbFunc)handleOsdStyleCfg      },
    {JEvent_RecordCfgChg       , (cmdCbFunc)handleRecordCfg        },
    {JEvent_AutoRebootCfgChg   , (cmdCbFunc)handleAutoRebootCfg    },
    {JEvent_BootargCfgChg      , (cmdCbFunc)handleBootargCfg       },

    {JEvent_EquipCfgChg        , (cmdCbFunc)handleCapability       },
    {JEvent_UserCfgChg         , (cmdCbFunc)handleUserCfg          },
    {JEvent_WebShowCfgChg      , (cmdCbFunc)handleWebShowCfg       },
    {JEvent_Video3aCfgChg      , (cmdCbFunc)handleVideo3aCfg       },

    {JEvent_CaptureCfgChg      , (cmdCbFunc)handleCaptureCfg       },
    {JEvent_IpConflictCfgChg   , (cmdCbFunc)handleIpConflictCfg    },
    {JEvent_IpBrokenCfgChg     , (cmdCbFunc)handleIpBrokenCfg      },

    {JEvent_ViinfoCfgChg       , (cmdCbFunc)handleViinfoCfg        },
    {JEvent_RoiCfgChg          , (cmdCbFunc)handleRoiCfg           },
    {JEvent_ProfileCfgChg      , (cmdCbFunc)handleProfileCfg       },
    {JEvent_DnrCfgChg          , (cmdCbFunc)handleDenoisecfg       },
    {JEvent_AuthModecfgChg     , (cmdCbFunc)handleAuthModecfg      },

    {JEvent_MotionDetectCfgChg , (cmdCbFunc)handleMotionDetectCfg  },
    {JEvent_HumanDetectCfgChg  , (cmdCbFunc)handleHumanDetectCfg   },
    {JEvent_VglineCfgChg       , (cmdCbFunc)handleVglineCfg        },
    {JEvent_VgrectCfgChg       , (cmdCbFunc)handleVgrectCfg        },
    {JEvent_CarDetectCfgChg    , (cmdCbFunc)handleCarDetectCfg     },
	{JEvent_PetDetectCfgChg    , (cmdCbFunc)handlePetDetectCfg     },
    {JEvent_CryDetectCfgChg    , (cmdCbFunc)handleCryDetectCfg     },

    {JEvent_GuoBiaoCfg         , (cmdCbFunc)handleGuoBiaoCfg       },
    {Jevent_GuoBiaoAddrCfg     , (cmdCbFunc)handleGuoBiaoAddrCfg   },
    {JEvent_AudioInCfgChg      , (cmdCbFunc)handleAudioCfg         },
    {JEvent_AudioTestCfgChg    , (cmdCbFunc)handleAudioTestCfg     },

    {JEvent_HttpPortCfgChg     , (cmdCbFunc)handleNetPortCfg       },
    {JEvent_FtpPortCfgChg      , (cmdCbFunc)handleNetPortCfg       },
    {JEvent_RtspPortCfgChg     , (cmdCbFunc)handleNetPortCfg       },
    {JEvent_SpeekPortCfgChg    , (cmdCbFunc)handleNetPortCfg       },
    {JEvent_UpdatePortCfgChg   , (cmdCbFunc)handleNetPortCfg       },
    {JEvent_OsdCfgChg          , (cmdCbFunc)handleOsdinfoCfg       },
    {JEvent_UpdateBegin        , NULL                              },
    {JEvent_TimeOsdCfgChg      , (cmdCbFunc)handleTimeOSDCfg       },
    {JEvent_Daynightcfg        , (cmdCbFunc)handleDaynightCfg      },
    {JEvent_DhcpNotify         , (cmdCbFunc)handleDhcpNotify       },
    {JEvent_PelcodCfg          , (cmdCbFunc)handlePelcodCfg        },
    {JEvent_Presetctrl         , (cmdCbFunc)handlePreSetNewCfg     },
    {JEvent_Sim4g              , (cmdCbFunc)handleSim4gCfg         },
    {JEvent_Followcfg          , (cmdCbFunc)handleFollowCfg        },
    {JEvent_MotorCfg           , (cmdCbFunc)handleMotorCfg         },
    {JEvent_OnvifInfoCfg       , (cmdCbFunc)handleOnvifInfoCfg     },
    {JEvent_AudioAlarmCfg      , (cmdCbFunc)handleAudioAlarmCfg    },
    {JEvent_LightAlarmCfg      , (cmdCbFunc)handleLightAlarmCfg    },
    {JEvent_IOAlarmCfg         , (cmdCbFunc)handleIOAlarmCfg       },
    {JEvent_VMaskAlarmCfg      , (cmdCbFunc)handleVMaskAlarmCfg    },
    {JEvent_VMaskAlarmLinkCfg  , (cmdCbFunc)handleVMaskAlarmLinkCfg},
    {JEvent_DriveOutCfg        , (cmdCbFunc)handleDriveOutCfg      },
    {JEvent_StreamNotify       , (cmdCbFunc)handleStreamNotify     },
    {JEvent_PrivCtrl           , (cmdCbFunc)handlePrivCtrlCfg      },
    {JEvent_DevCfg             , (cmdCbFunc)handleDevConf          },
    {JEvent_AppveCfgChg        , (cmdCbFunc)handleAppveCfg         },
    {JEvent_AlarmInfoCfgChg    , (cmdCbFunc)handleAlarmInfoCfg     },
    {JEvent_Quality_Change     , (cmdCbFunc)NULL                   },
    {JEvent_Sim4gLocation      , (cmdCbFunc)NULL                   },
    {JEvent_ConvergenceChg     , (cmdCbFunc)handleConvergenceCfg   },
    {JEvent_AiVqeV2CfgChg      , (cmdCbFunc)handleAiVqeV2Cfg       },
    {JEvent_AiSpeexCfgChg      , (cmdCbFunc)handleAiSpeexCfg       },
    {JEvent_Test               , NULL                              },
    {JEvent_End                , NULL                              },
};

int loadingFileToXml()
{
    FILE *file = fopen(CONF_XML, "r");
    if(NULL == file) {
        ERR("Open %s failed!\n", CONF_XML);
        return -1;
    }

    g_root = mxmlLoadFile(NULL, file, MXML_NO_CALLBACK);
    if(NULL == g_root) {
        ERR("mxmlLoadString failed!\n");
        fclose(file);
        file = NULL;
        return -1;
    }

    fclose(file);
    DBG("%s succ g_root\n", __func__);

    return 0;
}

int checkAndLoadingXml()
{
    int cpFlag = 0, ret = 0;

    if(access(CONF_XML, F_OK)) {
        LOG("xml may be crashed, copy form org!\n");
        if(handleDefaultCfg(NULL, NULL, 0, NULL, (void *)"badxml") < 0) {
            ERR("Execute handleDefaultCfg error\n");
            return -1;
        }

        cpFlag = 1;
    }

    do {
        if(loadingFileToXml() == 0) //success
            break;

        if(cpFlag) {
            ret = -1;
            break;
        }

        LOG("xml may be crashed, do reset2factory!\n");
        handleDefaultCfg(NULL, NULL, 0, NULL, (void *)"badxml");
        DELAY_REBOOT_LINUX_CONF();
        ret = 0;
    } while(0);

    return ret;
}

static const char* xml_format_write(mxml_node_t *node, int val)
{
    static char strBuf[128];
    const char *str = (const char *)strBuf;
    mxml_node_t *pNode = node;
    int depth = 0;
    int has_elem_child = 0;

    memset(strBuf, 0, sizeof(strBuf));
    while(pNode != NULL) {
        pNode = pNode->parent;
        depth ++;
    }
    if(depth >= 2) depth = depth - 2;
    else depth = 0;

    if(val == MXML_WS_BEFORE_OPEN) {
        memset(strBuf, ' ', depth * 2);
    } else if(val == MXML_WS_AFTER_OPEN) {
        pNode = node->child;
        while(pNode != NULL) {
            if(pNode->type == MXML_ELEMENT) {
                has_elem_child = 1;
                break;
            }
            pNode = pNode->next;
        }

        if(has_elem_child)
            strBuf[0] = '\n';
    } else if(val == MXML_WS_BEFORE_CLOSE) {
        pNode = node->child;
        while(pNode != NULL) {
            if(pNode->type == MXML_ELEMENT) {
                has_elem_child = 1;
                break;
            }
            pNode = pNode->next;
        }

        if(has_elem_child)
            memset(strBuf, ' ', depth * 2);
    } else if(val == MXML_WS_AFTER_CLOSE) {
        strBuf[0] = '\n';
    }

    return str;
}


void syncXmlToFile()
{
    int ret = 0;

    conf_lock();

    FILE *file = fopen(CONF_XML_TMP, "w+");
    if(NULL == file) {
        ERR("fopen %s failed!\n", CONF_XML_TMP);
        conf_unlock();
        return;
    }

    ret = mxmlSaveFile(g_root, file, xml_format_write);
    if(ret != 0) {
        ERR("mxmlSaveFile error!\n");
        fclose(file);
        conf_unlock();
        return ;
    }

    fflush(file);

    ret = fsync(fileno(file));

    fclose(file);

    if(0 == ret) {
        ret = rename(CONF_XML_TMP, CONF_XML);
        g_setflag = 0;
    }

    conf_unlock();
}

static void confSyncHanding(void *data)
{
    //DBG("conf sync start\n");
    int set_flag = 0;
    conf_lock();
    set_flag = g_setflag;
    conf_unlock();

    if (set_flag) {
        if (get_g_sys(factest)) {
            DBG("Parameter changed, but no sync for __factest__...\n");
            g_setflag = 0;
            return;
        }

        DBG("Parameter changed, sync...!\n");
        syncXmlToFile();
    }
    //DBG("conf sync end\n");
}

void test_attach_config_ethcfg(int id, void *p_src, int size, void *ctx)
{
    DBG("[%s] event id[%d]\n", __func__, id);
    return;
}


void test_attach_config_ethcfg_2(int id, void *p_src, int size, void *ctx)
{
    DBG("[%s] async id[%d] chn[%d]\n", __func__, id, *(int *)p_src);
    return;
}

void cmd_test_attach_config_ethcfg(int id, void *ptr, int size, void *data)
{
    DBG("[%s] event id[%d] chn[%d]\n", __func__, id, PTR2INT(ptr));
    return;
}

int cbCentreInit()
{

    g_p_conf = js_event_manager_new();
    return_val_if_fail(g_p_conf != NULL, FAILURE);

    int i = 0;
    for (i = 0; confCmd[i].id_param != JEvent_End; i++) {
        js_event_register_type(g_p_conf, confCmd[i].id_param);
    }

    /* just for test */
    attach_config(JEvent_EthcfgChg, cmd_test_attach_config_ethcfg, NULL);
    //attach_config_async(JEvent_EthcfgChg, test_attach_config_ethcfg_2, NULL);

    js_event_register_type(g_p_conf, JEvent_MetaOverRtsp);
    js_event_register_type(g_p_conf, JEvent_RtpOverMulticast);
    return 0;
}

int current_language_init(void)
{
    BOOTARGS_CFG_S bootargs = {{0,},};
    if(0 != config_bootargs("get", NULL, &bootargs)){
        DBG("get current language failed!\n");
        return -1;
    }
    g_current_language = bootargs.lang;
    DBG("get current language success!\n");
    return 0;
}

int get_current_lang(void)
{
    return g_current_language;
}

int init_server_config()
{
    DBG("config init\n");
    pthread_mutex_init(&g_mutex, NULL);

    if(checkAndLoadingXml() < 0) {
        return -1;
    }

    if(cbCentreInit() < 0) {
        ERR("Config's cbcetre init failed!\n");
        mxmlDelete(g_root);
        g_root = NULL;
        return -1;
    }

    if(current_language_init() < 0){
        DBG("current_language_init failed!\n");
        return -1;
    }

    /* eth config */
    NetEthS eth = {{0},};
    conf_get_ethcfg(&eth);

    if (eth.enable || is_okey(F_ETH_ENABLE)) {
        SYSLOG("_Exist_ eth0 enable: %d\n", eth.enable);
        if (is_okey("/sys/class/net/eth0/statistics/tx_bytes")) {
            set_g_sys(eth);
        }
    } else {
        SYSLOG("Disable eth0: %d\n", eth.enable);
        clr_g_sys(eth);
        UtilSystemCmd((char *)"ifconfig eth0 down");
    }

    return 0;
}

int init_client_config_sync(void *data)
{
    js_create_timer_r((JSScheduler)data, 3000, 3000, confSyncHanding, NULL, &g_config_sync_handle);

    return 0;
}

int uninit_client_config_sync()
{
    static int suicided = false;

    if (suicided) {
        DBG("a former stop sync done, return directly\n");
        return 0;
    }

    js_delete_timer_r(&g_config_sync_handle);

    confSyncHanding(NULL);
    suicided = true;

    conf_lock();
    // mxmlDelete(g_root);
    // g_root = NULL;
    conf_unlock();

    return 0;
}

int uninit_server_config()
{
    uninit_client_config_sync();

    return SUCCESS;
}

