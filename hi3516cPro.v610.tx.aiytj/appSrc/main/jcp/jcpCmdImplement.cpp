/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpCmdImplement.cpp
 * @Created Time : 2013-12-25
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <openssl/md5.h>


#include "delay_exec.h"

#include "debug.h"
#include "alarmapi.h"
#include "logapi.h"
#include "recordapi.h"

#include "jcpCmd.h"
#include "jcpUniversalUtils.h"
#include "jcpCmdImplement.h"

#include "utils.h"
#include "jcpService.h"

#include "mxml.h"
#include "conf_list.h"
#include "conf_nand.h"
#include "jconfig.h"
#include "jconfstruct.h"
#include "pthread_manage.h"
#include "our_md5.h"
#include "passwdtrans.h"
#include "conf_nand.h"
#include "search_service.h"
#include "qrEncode.h"
#include "encodeapi.h"
#include "ptz_ctrl.h"
#include "ptz_follow.h"

#include "upnp.h"
#include "ddnsstrategy.h"
#include "jcpService.h"
#include "usr_super.h"
#include "update.h"
#include "valgrind.h"
#include "conftypedef.h"
#include "confutils.h"
#include "factry_tool.h"
#include "factory_db.h"
#include "http_main.h"
#include "system_sch.h"
#include "alarm_service.h"
#include "gpio.h"
#include "airlink.h"
#include "net_config.h"
#include "record_file_manage.h"
#include "sim4g.h"
#include "sim4g_common_api.h"
#include "encodeapi.h"
#include "net_qrcode.h"
#include "time_sync.h"
#include "net_check.h"
#include "record_disk.h"
#include "encode_video.h"

#include "g_sys.h"
#include "g_run.h"
#include "g_log.h"
#include "g_stat.h"
#include "id_protect.h"

#include "base64.h"
#include "libsecurity.h"
#include "encrypt.h"
#include "soft_check.h"

//OTA升级
#include "ota_updata.h"
#include "alarmapi.h"
#include "confapi.h"
#include "jevent.h"
#include "confapi.h"
#include "jconfstruct.h"
#include "aging_test.h"
#include "conf_spi.h"
#include "encode_audio_queue.h"
#include "lamp_sal.h"
#include "lamp_hal.h"
#include "time_config.h"
#include "encode_common.h"
#include "ble_services.h"
#include "sd_recovery.h"
#include "record_alarm_param.h"
#include "record_lib.h"
#include "record_scan_days.h"
#include "shm_buf_pool.h"
#include "watch_rstkey.h"
#include "aging8h.h"
#include "tencent_video_call.h"
#include "encode_audio_input.h"
#include "ss_mpi_audio.h"

#ifdef PLATFORM_TENCENT
#include "iv_sys.h"
#include "tencent_server.h"
#include "tencent_cloud_storage.h"
#include "tencent_record_play.h"
#include "tencent_http_service.h"
#endif

#include "basetype.h"

#include "jpwm.h"

#ifdef PLATFORM_GB
 #include "gb_api.h"
#endif

#include "util_log.h"

extern "C" {
void close_all_record_connection(void);
}

#define JCP_ACTION_LEN    12
#define GET_ALARM_INTERVAL 3
#define MD5_DIGEST_LENGTH 16
#define TOG_BUILD " Build @" __DATE__ "." __TIME__

#define CONF_XML      "/opt/conf/config.xml"
#define TIME_OUT      60*60

//PK校验
#ifndef __PK__
#define __PK__ "not_define_pk"
#endif
#define XSTR(s) STR(s)
#define STR(s) #s
#define PK XSTR(__PK__)

#if defined(BRANCH_STAR)
#   define __STAR 1
#else
#   define __STAR 0
#endif

#define CUSTOMCONF_APPEND_INT(inner, outer, format, args...) do {       \
    if(inner != outer) {                                                \
        char custom_buf[512] = {0};                                     \
        sprintf(custom_buf, format, ##args);                            \
        if (SUCCESS == AppendFile(CUSTOM_CONF, custom_buf)) {           \
            SYSLOG("%s: %s", CUSTOM_CONF, custom_buf);                  \
        }                                                               \
    }                                                                   \
} while(0)

#define CUSTOMCONF_APPEND_STR(inner, outer, format, args...) do {       \
    if(0 != strcmp(inner, outer)) {                                     \
        char custom_buf[512] = {0};                                     \
        sprintf(custom_buf, format, ##args);                            \
        if (SUCCESS == AppendFile(CUSTOM_CONF, custom_buf)) {           \
            SYSLOG("%s: %s", CUSTOM_CONF, custom_buf);                  \
        }                                                               \
    }                                                                   \
} while(0)

extern tzoneS tzone[];

/*
array:多通道定义的结构体数组
arr_size: 定义的结构体的长度。
*/
#define JCP_ARG_PARSER(array, arr_size) do {                                \
    if (SUCCESS != parser_jcp_arg(argc, argv, opts, buf, array, arr_size)) {\
        char *pb = buf;                                                     \
        pb += strlen(buf);                                                  \
        sprintf((char *)pb,                                                 \
        "\r\n    <<parser error, ask to [%s -?] for help!>>", argv[0]);     \
        return FAILURE;                                                     \
    }                                                                       \
                                                                            \
    if (SUCCESS == arg_opt_if_set("?", opts)) {                             \
        help_jcp_arg_msg(argc, argv, (char *)buf, buflen,opts, helps);             \
        return SUCCESS;                                                     \
    }                                                                       \
}while(0)

#define JCP_ARG_PARSER_EX(array, arr_size) do {                                \
        if (SUCCESS != parser_jcp_arg_ex(argc, argv, opts, buf, array, arr_size)) {\
            char *pb = buf;                                                     \
            pb += strlen(buf);                                                  \
            sprintf((char *)pb,                                                 \
            "\r\n    <<parser error, ask to [%s -?] for help!>>", argv[0]);     \
            return FAILURE;                                                     \
        }                                                                       \
                                                                                \
        if (SUCCESS == arg_opt_if_set_ex("?", opts)) {                          \
            help_jcp_arg_msg_ex(argc, argv, (char *)buf, buflen, opts, helps);          \
            return SUCCESS;                                                     \
        }                                                                       \
    }while(0)


#define JCP_RETURN_SUCC_IF_MEM_EQ(s1, s2, n) do {                   \
    if (0 == memcmp(s1, s2, n)) {                                   \
        sprintf(buf, "cmdline & xml are the same!");                \
        if (get_g_sys(factest))                     \
            SYSLOG("%s cmdline & xml are the same\n", __func__);    \
        return SUCCESS;                                             \
    }                                                               \
} while(0)

#define RETURN_FAIL_IF_API_ERR(ret) do {                            \
    if (ret != SUCCESS) {                                           \
        DBG("Fail @%s\n", __FUNCTION__);                            \
        sprintf(buf, "return @%s|%d|\n", __FUNCTION__, __LINE__);   \
        return FAILURE;                                             \
    }                                                               \
} while(0)


#define ASMJCP_LIST_STRING(buf, buflen, opts) do {                \
    if( -1 == assembleListString((char *)buf, buflen, opts)) {    \
        sprintf(buf, "asm jcp list , return @%s|%d|\n", __FUNCTION__, __LINE__); \
            return FAILURE;                                       \
    }                                                             \
}while(0)

#define ASMJCP_LIST_STRING_EX(buf, buflen, opts) do {                \
    if( -1 == assembleListString_ex((char *)buf, buflen, opts)) {     \
        sprintf(buf, "asm jcp list , return @%s|%d|\n", __FUNCTION__, __LINE__); \
            return FAILURE;                                       \
    }                                                             \
}while(0)


#define SET_PARAM_RULE_CHECK(opts) do {                                 \
    if(FAILURE == setmustListonlyRule(buf, opts)){                      \
        char *pb = buf;                                                  \
        pb += strlen(buf);                                               \
        sprintf((char *)pb,                                              \
        "\r\n    <<setmustListonlyRule error, ask to [%s -?] for help!>>", argv[0]); \
        return FAILURE;                                                 \
    }                                                                   \
}while(0)


#define LIST_PARAM_RULE_CHECK(opts) do {                                    \
        if(FAILURE == checkListRule(opts)){                                 \
            sprintf(buf,                                                    \
            "only list no param\r\n    "                                    \
            "<<checkListRule error, ask to [%s -?] for help!>>", argv[0]); \
            return FAILURE;                                                 \
        }                                                                   \
}while(0)

#define LIST_PARAM_RULE_CHECK_EX(opts) do {                                    \
        if(FAILURE == checkListRule_ex(opts)){                                 \
            sprintf(buf,                                                    \
            "only list no param\r\n    "                                    \
            "<<checkListRule error, ask to [%s -?] for help!>>", argv[0]); \
            return FAILURE;                                                 \
        }                                                                   \
}while(0)


// 多通道时调用这个组装
#define ASMJCP_LIST_COUNT(arry, str_size, arr_size) do{             \
        char listbuf[1024] = {0};                                   \
        char *p = listbuf;                                          \
        int i = 0;                                                  \
        for (i = 0; i < arr_size; i++) {                            \
            p += asmListCount(p, opts, arry, i, str_size);          \
        }                                                           \
        strcat(buf,listbuf);                                        \
}while(0)

//获取节点参数赋值操作
#define JCP_GET_NODE_VALUE_API(handleCfgFunction,inner_st,out_st) do{\
    ret = get_config(handleCfgFunction, inner_st);                    \
    RETURN_FAIL_IF_API_ERR(ret);                                      \
    memcpy(&out_st, &inner_st, sizeof(out_st));                       \
}while(0)


#define alert_write_so_manytimes() do{      \
    static time_t mod_file_time[10] = {0,}; \
    static size_t i = 0;                    \
    mod_file_time[(i++)%10] = mono_sec();   \
    if ((i > 10) && (mod_file_time[(i-1)%10] - mod_file_time[i%10] < TIME_OUT)) { \
        ERR("write file so manytimes, alarm\n");                                   \
    }                                       \
}while(0)

#define SET_TIME    0   //00HHMMSS
#define GET_TIME    1
#define SET_DATE    2   //YYMMWWDD
#define GET_DATE    3

static int sync_presetcfg(int cmd, int preset, char *preset_name)
{
    if (preset > MAX_PRESET_NUM || preset <= 0 || cmd == 2) {
        return -1;
    }

    follow_info_t follow_cfg = {0,};
    presetcfg precfg = {0};
    get_config(handleFollowCfg, follow_cfg);
    get_config(handlePreSetNewCfg, precfg);

    switch (cmd) {
        case 1:
            precfg.preset[preset].enable = 1;
            strcpy(precfg.preset[preset].name, preset_name);
            break;
        case 3:
            precfg.preset[preset].enable = 0;
            sprintf(precfg.preset[preset].name, "preset%d", preset);
            if (preset == follow_cfg.preset) {
                follow_cfg.preset = 0;
            }
            break;
        default:
            break;
    }

    set_config(handleFollowCfg, follow_cfg);
    set_config(handlePreSetNewCfg, precfg);
    return 0;
}

static void user_md5Encpypt(char *user, char *passwd, char *ha)
{
    AuthRealmS auth = {{0},};
    MD5_CTX_OUR md5CtxA1;
    get_config(handleAuthRealmCfg, auth);

    our_MD5Init(&md5CtxA1);
    ourMD5Update(&md5CtxA1, (unsigned char *)user, strlen(user));
    ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
    ourMD5Update(&md5CtxA1, (unsigned char *)auth.realm, strlen(auth.realm));
    ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
    ourMD5Update(&md5CtxA1, (unsigned char *)passwd, strlen(passwd));
    our_MD5Final((unsigned char*)ha, &md5CtxA1);
}

static void user_cvtohex(char *Bin, char *Hex)
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < 16; i++)
    {
      j = (Bin[i] >> 4) & 0xf;
      if (j <= 9)
        Hex[i * 2] = (j + '0');
      else
        Hex[i * 2] = (j + 'a' - 10);
      j = Bin[i] & 0xf;
      if (j <= 9)
        Hex[i * 2 + 1] = (j + '0');
      else
        Hex[i * 2 + 1] = (j + 'a' - 10);
    };

    Hex[32] = '\0';
}

static int add_usercfg(char *user, char *passwd, int group)
{
    if(NULL == user || NULL == passwd || 2 < group || 0 > group)
    {
        ERR("Parameter error!\n");
        return FAILURE;
    }

    int ii = 0;
    for(ii = 0; ii < (int)strlen(passwd); ii++)
    {
        if((passwd[ii] < '0') || (passwd[ii] > '9' && passwd[ii] < 'A') ||
            (passwd[ii] > 'Z' && passwd[ii] < 'a' && passwd[ii] != '_') || passwd[ii] > 'z' )
        {
            ERR("passwd paramter error!\n");
            return FAILURE;
        }
    }

    int ret = SUCCESS;
    int exist = 0;
    SysUserS suser = {0,};
    do
    {
        if(get_config(handleUserCfg, suser) < 0)
        {
            ERR("conf_get_usercfg failed!\n");
            ret = FAILURE;
            break;
        }

        if(suser.gnum >= USER_MAX_NUM)
        {
            ERR("Can't add more user!\n");
            ret = -4;  //用户达到最大数目
            break;
        }

        int i = 0;
        for(i = 0; i < USER_MAX_NUM; i++)
        {
            if(!strcmp(user, suser.user[i].username))
            {
                exist = 1;
                break;
            }
        }

        if(1 == exist)
        {
            ERR("%s is exist\n", user);
            ret = -2;     //用户已存在
            break;
        }

        int j = 0;

        for(j = 0; j < USER_MAX_NUM; j++)
        {
            if(0 == strlen(suser.user[j].username))
            {
                break;
            }
        }

        if(j >= USER_MAX_NUM)
        {
            ERR("Unknow error!\n");
            ret = FAILURE;
            break;
        }

        char *ptr = NULL;
        ptr = j_crypt(passwd, "jc");
        if(ptr == NULL)
        {
            ERR("Encrypt failed!\n");
            ret = FAILURE;
            break;
        }

        strncpy(suser.user[j].cryptpasswd, ptr, sizeof(suser.user[j].cryptpasswd));
        strncpy(suser.user[j].username, user, sizeof(suser.user[j].username));
        passwd_trans_encode(suser.user[j].onvifpasswd, passwd, strlen(passwd));
        suser.user[j].group = group;

        char ha1[20] = {0};

        user_md5Encpypt(user, passwd, ha1);
        user_cvtohex(ha1, suser.user[j].digestpasswd);

        suser.gnum++;

        if(set_config(handleUserCfg, suser) < 0)
        {
            ERR("conf_set_usercfg failed!\n");
            ret = FAILURE;
            break;
        }
    }
    while(0);

    return ret;
}

static int set_user_passwd(char *user, char *passwd, int group)
{
    if(NULL == user || NULL == passwd || group > 2)
    {
        ERR("Paramter error!\n");
        return FAILURE;
    }

    int ii = 0;
    for(ii = 0; ii < (int)strlen(passwd); ii++)
    {
        if((passwd[ii] < '0') || (passwd[ii] > '9' && passwd[ii] < 'A') ||
            (passwd[ii] > 'Z' && passwd[ii] < 'a' && passwd[ii] != '_') || passwd[ii] > 'z' )
        {
            ERR("passwd paramter error!\n");
            return FAILURE;
        }
    }

    int i = 0;
    SysUserS users = {0,};
    if(get_config(handleUserCfg, users) != SUCCESS)
    {
        ERR("conf_get_usercfg failed!\n");
        return FAILURE;
    }

    for(i = 0; i < USER_MAX_NUM; i++)
    {
        if(!strcmp(user, users.user[i].username))
        {
            break;
        }
    }

    if(i >= USER_MAX_NUM)
    {
        ERR("User [%s] not exist!\n", user);
        return -3;
    }

    if(group >= 0)
    {
        users.user[i].group = group;
    }

    char *ptr = NULL;
    ptr = j_crypt(passwd, "jc");
    if(NULL == ptr)
    {
        return FAILURE;
    }

    sprintf(users.user[i].cryptpasswd, "%s", ptr);

    char ha1[20] = {0};

    user_md5Encpypt(user, passwd, ha1);
    user_cvtohex(ha1, users.user[i].digestpasswd);

    passwd_trans_encode(users.user[i].onvifpasswd, passwd, strlen(passwd));

    if(set_config(handleUserCfg, users)< 0)
    {
        ERR("conf_set_usercfg failed!\n");
        return FAILURE;
    }

    return SUCCESS;
}


/*
    用户相关函数返回值:
    成功 : group id/SUCCESS; -1 : FAILURE; -2 : 用户已经存在
    -3 : 用户不存在 -4 : 用户数目已上限 -5 : 密码错误
*/
static int user_basic_auth(char *user, char *passwd)
{
    SysUserS users = {0};
    AuthtypeE auth = NONE_AUTH;
    if(get_config(handleUserCfg, users) < 0 || get_config(handleAuthModecfg, auth) < 0)
    {
        ERR("get parameters failed!\n");
        return FAILURE;
    }

    if(0 == auth)
    {
        return SUCCESS;
    }

    int i = 0;
    for(i = 0; i < USER_MAX_NUM; i++)
    {
        if(!strcmp(user, users.user[i].username))
        {
            break;
        }
    }

    if(i >= USER_MAX_NUM)
    {
        return -3;
    }

    char *ptr = NULL;

    ptr = j_crypt(passwd, users.user[i].cryptpasswd);
    if(ptr == NULL || strcmp(ptr, users.user[i].cryptpasswd))
    {
        ERR("Basic auth failed!\n");
        return -5;  //密码错误
    }

    return users.user[i].group;
}

static int del_usercfg(char *user)
{
    if(NULL == user)
    {
        ERR("Parameter error!\n");
        return FAILURE;
    }

    int ret = SUCCESS;
    int exist = 0;
    SysUserS suser = {0,};

    do
    {
        if(get_config(handleUserCfg, suser) < 0)
        {
            ERR("conf_get_usercfg failed!\n");
            ret = FAILURE;
            break;
        }

        int i = 0;
        for(i = 0; i< USER_MAX_NUM; i++)
        {
            if((!strcmp(user, suser.user[i].username)))
            {
                exist = 1;
                break;
            }
        }

        if(0 == exist)
        {
            ERR("%s in not exist!\n", user);
            ret = -3;    //用户不存在
            break;
        }

        memset(suser.user[i].username, 0x00, sizeof(suser.user[i].username));
        memset(suser.user[i].cryptpasswd, 0x00, sizeof(suser.user[i].cryptpasswd));
        memset(suser.user[i].digestpasswd, 0x00, sizeof(suser.user[i].digestpasswd));
        memset(suser.user[i].onvifpasswd, 0x00, sizeof(suser.user[i].onvifpasswd));
        suser.user[i].group = 0;
        suser.gnum--;

        for(i = 0; i< USER_MAX_NUM; i++) {     //用户名顺序重组
            if (strlen(suser.user[i].username) != 0 || i == USER_MAX_NUM-1) {
                continue;
            }
            memcpy(&suser.user[i], &suser.user[i+1], sizeof(SysUser0));
            suser.user[i].id = i;

            memset(&suser.user[i+1], 0x00, sizeof(SysUser0));
            suser.user[i+1].id = i+1;
        }

        if(set_config(handleUserCfg, suser) < 0)
        {
            ERR("conf_set_usercfg failed!\n");
            ret = FAILURE;
            break;
        }
    }
    while(0);

    return ret;
}


//设置时间
int JCOTimeChipSet(time_t tmSet)
{
    int fd;
    struct tm timedate;
    unsigned long tmp1=0;
    unsigned long tmp2=0;
    unsigned long temp;

    if (NULL == gmtime_r(&tmSet, &timedate))
    {
        return FAILURE;
    }
    // 生成年月日
    tmp1 += timedate.tm_mday;
    tmp1 += timedate.tm_wday * 100;
    tmp1 += (timedate.tm_mon + 1 ) * 10000;
    temp = timedate.tm_year + 1900;
    temp = temp - (temp / 100) * 100;
    tmp1 += temp * 1000000;

    // 生成时分秒
    tmp2 = 0;
    tmp2 = timedate.tm_sec;
    tmp2 += timedate.tm_min * 100;
    tmp2 += timedate.tm_hour * 10000;

    fd = open("/dev/sysclock", O_NONBLOCK);
    if (0 > fd)
    {
        return FAILURE;
    }
    //set_hard_date(tmp1);
    //set_haard_time(tmp2);
    //ioctl(fd, SET_DATE, &tmp1);
    //ioctl(fd, SET_TIME, &tmp2);

    close(fd);
    return SUCCESS;
}

//获取时间
int JCOTimeChipGet(time_t *tmGet)
{
    int fd;
    struct tm timedate;
    unsigned long tmp=0;
    int retry = 0;

    if (NULL == tmGet)
    {
        return FAILURE;
    }

    do
    {
        fd = open("/dev/sysclock", O_NONBLOCK);
        if (fd < 0)
        {
            DBG("open sysclock failed, retry=%d\n", retry++);
            sleep(1);
            continue;
        }
    }
    while ((0 > fd) && (10 > retry));

    if (0 > fd)
    {
        return FAILURE;
    }

    // 获取时分秒
    //get_hard_time(tmp);
    //if (ioctl(fd, GET_TIME, &tmp) != 0)
    //{
    //  close(fd);
    //  return FAILURE;
    //}

    timedate.tm_sec = tmp - (tmp / 100) * 100;
    tmp /= 100;
    timedate.tm_min = tmp - (tmp / 100) * 100;      // 分，0~59
    tmp /= 100;
    timedate.tm_hour = tmp - (tmp / 100) * 100;     // 小时，0~23

    // 获取年月日
    //get_hard_date(tmp);

    //if (ioctl(fd, GET_DATE, &tmp) != 0)
    //{
    //  close(fd);
    //  return FAILURE;
    //}
    timedate.tm_mday = tmp - (tmp / 100) * 100;     // 月份中的日期，1~31
    tmp /= 100;
    timedate.tm_wday = tmp - (tmp / 100) * 100;     // 星期几，0~6（周日为0）
    tmp /= 100;
    timedate.tm_mon = tmp - (tmp / 100) * 100 - 1;  // 月份，0~11（一月份为0）
    tmp /= 100;
    timedate.tm_year = 100 + tmp - (tmp / 100) * 100;   // 从1900年开始计算的年份
    timedate.tm_yday = 0;   // 年份中的日期，0~365
    timedate.tm_isdst = 0;  // 是否夏令时

    *tmGet = mktime(&timedate);

    close(fd);
    return SUCCESS;
}

int JCPCmdCA2MDCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    MotionDetectLinkS inner = {0};
    MotionDetectLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "移动侦测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"             },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"soundsel" , "音源选择 0: 默认 1：犬吠 2：自定义"   },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2mdcfg -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"soundsel" , ArgTypesInt , "0~2"     , (void*)&outer.soundsel   , sizeof(outer.soundsel)   },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };
    ret = get_config(handleMotionDetLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(MotionDetectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(MotionDetectLinkS));
        ret = set_config(handleMotionDetLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

int JCPCmdCA2HDCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    HumanDetectLinkS inner = {0};
    HumanDetectLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "人形侦测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"             },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2humandetectcfg -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };
    ret = get_config(handleHumanDetLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(HumanDetectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(HumanDetectLinkS));
        ret = set_config(handleHumanDetLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

int JCPCmdCA2CarCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    CarDetectLinkS inner = {0};
    CarDetectLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "车形侦测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"             },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2cardetectcfg -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };
    ret = get_config(handleCarDetLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(CarDetectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(CarDetectLinkS));
        ret = set_config(handleCarDetLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

int JCPCmdCA2PetCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    PetDetectLinkS inner = {0};
    PetDetectLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "宠形侦测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"              },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2petdetectcfg -? 获取帮助"             },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };

    ret = get_config(handlePetDetLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(PetDetectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(PetDetectLinkS));
        ret = set_config(handlePetDetLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

int JCPCmdCA2CryCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    CryDetectLinkS inner = {0};
    CryDetectLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "哭声检测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"              },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2crydetectcfg -? 获取帮助"             },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };

    ret = get_config(handleCryDetLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(CryDetectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(CryDetectLinkS));
        ret = set_config(handleCryDetLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

int JCPCmdCA2VglineCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    VglineLinkS inner = {0};
    VglineLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "越界侦测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"             },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"soundsel" , "报警音源选择  0: 默认 1：犬吠 2：自定义"  },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2vglinecfg -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"soundsel" , ArgTypesInt , "0~2"     , (void*)&outer.soundsel   , sizeof(outer.soundsel)   },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };
    ret = get_config(handleVglineLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VglineLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VglineLinkS));
        ret = set_config(handleVglineLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

int JCPCmdCA2VgrectCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    VgrectLinkS inner = {0};
    VgrectLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "区域侦测联动设置"                        },
        {"act"      , "list:获取所有参数 set:设置参数"          },
        {"interval" , "报警间隔 3s到36000s(10小时)"             },
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"         },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"             },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"             },
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"             },
        {"recorden" , "是否录像  1: 是  0 : 否"                 },
        {"ftpen"    , "是否ftp上传  1: 是  0 : 否"             },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"             },
        {"soundsel" , "报警音源选择 0：默认 1：犬吠  2：自定义"                 },
        {"captureen", "是否抓拍  1: 是  0 : 否"                 },
        {"preset"   , "是否调用预置位  0 : 否，大于1 表示预置位"},
        {"End"      , "ca2vgrectcfg -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"ftpen"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"soundsel" , ArgTypesInt , "0~2"     , (void*)&outer.soundsel   , sizeof(outer.soundsel)   },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.snapshot   , sizeof(outer.snapshot)   },
        {"preset"   , ArgTypesInt , "0~255"   , (void*)&outer.preset     , sizeof(outer.preset)     },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };
    ret = get_config(handleVgrectLinkCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VgrectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VgrectLinkS));
        ret = set_config(handleVgrectLinkCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;
}

int JCPAudioAlarm(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    AudioAlarmS inner = {0,};
    AudioAlarmS outer = {0,};

    HelpMsgS helps[] = {
        {"?"       , "声音报警参数设置"                    },
        {"act"     , "list:获取所有参数 set:设置参数"      },
        {"show"    , "声音报警显示"},
        {"enable"  , "声音报警使能"},
        {"type"    , "报警音源选择 0:默认 1:犬吠 2:人声 3:自定义 4:录制"},
        {"place"   , "时间布防，0:夜间 1:白天 2:全天 3:自定义"    },
        {"beginhour","自定义布防起始时间"},
        {"beginmin", "自定义布防起始时间"},
        {"endhour" , "自定义布防结束时间"},
        {"endmin"  , "自定义布防结束时间"},
        {"times"   , "音频播放次数"},
        {"aindex"   , "自定义语音 idx"     },
        {"atext"    , "自定义语音文本"       },
        {"End"     , "audioalarmcfg -? 获取帮助"             },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK, NULL      , NULL                     , 0             },
        {"act"       , ARG_TYPE_ACT, "list|set", action                   , sizeof(action)},
        {"show"      , ArgTypesInt , "0|1"     , &outer.show              , sizeof(int)   },
        {"enable"    , ArgTypesInt , "0|1"     , &outer.enable            , sizeof(int)   },
        {"type"      , ArgTypesInt , "0~4"     , &outer.type              , sizeof(int)   },
        {"place"     , ArgTypesInt , "0~3"     , &outer.place             , sizeof(int)   },
        {"beginhour" , ArgTypesInt , "0~23"    , &outer.timeseg.beginhour , sizeof(int)   },
        {"beginmin"  , ArgTypesInt , "0~59"    , &outer.timeseg.beginmin  , sizeof(int)   },
        {"endhour"   , ArgTypesInt , "0~23"    , &outer.timeseg.endhour   , sizeof(int)   },
        {"endmin"    , ArgTypesInt , "0~59"    , &outer.timeseg.endmin    , sizeof(int)   },
        {"times"     , ArgTypesInt , "0~10"    , &outer.times             , sizeof(int)   },
        {"aindex"    , ArgTypesInt   , NULL    , &outer.aindex            , sizeof(int)        },
        {"atext"     , ArgTypesString, NULL    , outer.atext              , sizeof(outer.atext)},
        {"End"       , ArgTypesEnd , NULL      , NULL                     , 0             },
    };

    ret = get_config(handleAudioAlarmCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(AudioAlarmS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(AudioAlarmS));
        ret = set_config(handleAudioAlarmCfg, outer);
        if (get_g_sys(factest)) {
             CUSTOMCONF_APPEND_INT(inner.show, outer.show, "1 /cfg/audioalarm/show %d\n", outer.show);
        }
    }

    return SUCCESS;
}

int JCPLightAlarm(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    LightAlarmS inner = {0,};
    LightAlarmS outer = {0,};

    HelpMsgS helps[] = {
        {"?"       , "灯光报警参数设置"                    },
        {"act"     , "list:获取所有参数 set:设置参数"      },
        {"enable"  , "灯光报警使能"},
        {"place"   , "时间布防，0:夜间 1:白天 2:全天 3:自定义"    },
        {"beginhour","自定义布防起始时间"},
        {"beginmin", "自定义布防起始时间"},
        {"endhour" , "自定义布防结束时间"},
        {"endmin"  , "自定义布防结束时间"},
        {"time"    , "白光灯亮灯时长"},
        {"End"     , "lightalarmcfg -? 获取帮助"             },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK, NULL      , NULL             , 0             },
        {"act"       , ARG_TYPE_ACT, "list|set", action           , sizeof(action)},
        {"enable"    , ArgTypesInt , "0|1"     , &outer.enable    , sizeof(int)   },
        {"place"     , ArgTypesInt , "0~3"     , &outer.place     , sizeof(int)   },
        {"beginhour" , ArgTypesInt , "0~23"    , &outer.timeseg.beginhour , sizeof(int)   },
        {"beginmin"  , ArgTypesInt , "0~59"    , &outer.timeseg.beginmin  , sizeof(int)   },
        {"endhour"   , ArgTypesInt , "0~23"    , &outer.timeseg.endhour   , sizeof(int)   },
        {"endmin"    , ArgTypesInt , "0~59"    , &outer.timeseg.endmin    , sizeof(int)   },
        {"time"      , ArgTypesInt , "10~60"   , &outer.time      , sizeof(int)   },
        {"End"       , ArgTypesEnd , NULL      , NULL             , 0             },
    };

    ret = get_config(handleLightAlarmCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(LightAlarmS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(LightAlarmS));

        LightExtCfg lightextcfg = {0};
        ret = conf_get_lightext_cfg(&lightextcfg);
        RETURN_FAIL_IF_API_ERR(ret);

        if (outer.enable == TRUE) {
            if (lightextcfg.lamptype == LAMP_IR || lightextcfg.lamptype == LAMP_STAR) {
                SYSLOG("Current lamp type not support light alarm!!!\r\n");
                snprintf(buf, strlen("not_support")+1, "not_support");
                return SUCCESS;
            }
        }

        ret = set_config(handleLightAlarmCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.time, outer.time, "1 /cfg/lightalarm/time %d\n", outer.time);
        }
    }

    return SUCCESS;
}

int JCPIOAlarm(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    IOAlarmS inner = {0,};
    IOAlarmS outer = {0,};

    HelpMsgS helps[] = {
        {"?"       , "IO 报警参数设置"                    },
        {"act"     , "list:获取所有参数 set:设置参数"      },
        {"show"    , "IO 报警显示"},
        {"enable"  , "IO 报警使能"},
        {"place"   , "时间布防，0:夜间 1:白天 2:全天 3:自定义"    },
        {"beginhour","自定义布防起始时间"},
        {"beginmin", "自定义布防起始时间"},
        {"endhour" , "自定义布防结束时间"},
        {"endmin"  , "自定义布防结束时间"},
        {"time"    , "红蓝灯亮灯时长"},
        {"End"     , "ioalarmcfg -? 获取帮助"             },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK, NULL      , NULL             , 0             },
        {"act"       , ARG_TYPE_ACT, "list|set", action           , sizeof(action)},
        {"show"      , ArgTypesInt , "0|1"     , &outer.show      , sizeof(int)   },
        {"enable"    , ArgTypesInt , "0|1"     , &outer.enable    , sizeof(int)   },
        {"place"     , ArgTypesInt , "0~3"     , &outer.place     , sizeof(int)   },
        {"beginhour" , ArgTypesInt , "0~23"    , &outer.timeseg.beginhour , sizeof(int)   },
        {"beginmin"  , ArgTypesInt , "0~59"    , &outer.timeseg.beginmin  , sizeof(int)   },
        {"endhour"   , ArgTypesInt , "0~23"    , &outer.timeseg.endhour   , sizeof(int)   },
        {"endmin"    , ArgTypesInt , "0~59"    , &outer.timeseg.endmin    , sizeof(int)   },
        {"time"      , ArgTypesInt , "10~60"   , &outer.time      , sizeof(int)   },
        {"End"       , ArgTypesEnd , NULL      , NULL             , 0             },
    };

    ret = get_config(handleIOAlarmCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(IOAlarmS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(IOAlarmS));
        ret = set_config(handleIOAlarmCfg, outer);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.show, outer.show, "1 /cfg/ioalarm/show %d\n", outer.show);
        }
    }

    return SUCCESS;
}

int JCPCmdVMaskAlarmCfg(char *buf, int buflen, int argc, char **argv)
{
    char timestrategy[128] = {0};
    char action[JCP_ACTION_LEN] = {0};
    VMaskAlarmS inner = {0,};
    VMaskAlarmS outer = {0,};
    int ret = SUCCESS;

    HelpMsgS helps[] = {
        {"?"           , "视频遮挡报警参数设置"                      },
        {"act"         , "list:获取所有参数 set:设置参数"            },
        {"enable"      , "视频遮挡告警开关，0 disable     , 1 enable"},
        {"thresh"      , "视频遮挡告警等级"                          },
        {"timestrategy", "布防时间描述\r\n"
                         "时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
                         "低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
                         "如全时段禁用：0,0,0,0,0,0,0\r\n"
                         "365中的格式是二元组的形式\r\n"
                         "timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"},
        {"End"         , "vmaskalarmcfg -? 获取帮助"},
    };

    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL         , 0                  },
        {"act"         , ARG_TYPE_ACT  , "list|set", action       , sizeof(action)     },
        {"enable"      , ArgTypesInt   , "0|1"     , &outer.enable, sizeof(int)        },
        {"thresh"      , ArgTypesInt   , "0~100"   , &outer.thresh, sizeof(int)        },
        {"timestrategy", ArgTypesString, NULL      , timestrategy  , sizeof(timestrategy)},
        {"End"         , ArgTypesEnd   , NULL      , NULL         , 0                  },
    };

    ret = conf_get_vmaskalarmcfg(&inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VMaskAlarmS));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.times);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VMaskAlarmS));
        ret = conf_set_vmaskalarmcfg(outer);
    }   
    
    return SUCCESS;
}


int JCPCmdCA2VMaskAlarmCfg(char *buf, int buflen, int argc, char **argv)
{
    VMaskAlarmLinkS inner = {0};
    VMaskAlarmLinkS outer = {0};
    char action[JCP_ACTION_LEN] = {0};
    int ret = SUCCESS;

    HelpMsgS helps[] = {
        {"?"        , "视频遮挡报警联动设置"           },
        {"act"      , "list:获取所有参数 set:设置参数" },  
        {"interval" , "报警间隔 3s到36000s(10小时)"    },  
        {"acen"     , "是否通知报警中心，1: 是  0 : 否"},  
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"    },  
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"    },  
        {"emailen"  , "是否邮件通知  1: 是  0 : 否"    },  
        {"recorden" , "是否录像  1: 是  0 : 否"        },  
        {"sounden"  , "是否发出声音  1: 是  0 : 否"    },
        {"captureen", "是否抓拍  1: 是  0 : 否"        },
        {"ftpup"    , "是否ftp上传  1: 是  0 : 否"     }, 
        {"End"      , "ca2vmcfg -? 获取帮助"           },  
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                     , 0                        },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action            , sizeof(action)           },
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval   , sizeof(outer.interval)   },
        {"acen"     , ArgTypesInt , "0|1"     , (void*)&outer.alarmcenter, sizeof(outer.alarmcenter)},
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout1  , sizeof(outer.alarmout1)  },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.alarmout2  , sizeof(outer.alarmout2)  },
        {"emailen"  , ArgTypesInt , "0|1"     , (void*)&outer.email      , sizeof(outer.email)      },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.record     , sizeof(outer.record)     },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sound      , sizeof(outer.sound)      },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.capture    , sizeof(outer.capture)    },
        {"ftpup"    , ArgTypesInt , "0|1"     , (void*)&outer.ftpup      , sizeof(outer.ftpup)      },
        {"End"      , ArgTypesEnd , NULL      , NULL                     , 0                        },
    };

    ret = conf_get_vmaskalarmlinkcfg(&inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VMaskAlarmLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VMaskAlarmLinkS));
        ret = conf_set_vmaskalarmlinkcfg(outer);
    }

    return SUCCESS;
}

int JCPCmdTestAlarm(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};

    int type = 0;
    int mail = 0;
    int dorules = 0;
    int chn = 0;
    HelpMsgS helps[] = {
        {"?"        , "模拟报警源"                  },
        {"act"      , "set:设置参数"                },
        {"alarmtype", "报警类型描述:\r\n"
            "         JALARM_TYPE_MD = 1  移动侦测\r\n"
            "         JALARM_TYPE_VGLINE = 2\r\n"
            "         JALARM_TYPE_VGRECT = 3\r\n"
            "         JALARM_TYPE_VL = 4  视频丢失\r\n"
            "         JALARM_TYPE_DISK_FULL = 5 磁盘满\r\n"
            "         JALARM_TYPE_DISK_ERR = 6  磁盘错误\r\n"
            "         JALARM_TYPE_CABLE_DISC = 7   网线被拔出\r\n"
            "         JALARM_TYPE_IP_CONFLICT = 8  IP冲突\r\n"
            "         JALARM_TYPE_ILLEGAL_ACCESS = 9  违法访问\r\n"
            "         JALARM_TYPE_AI = 10  报警输入\r\n"
            "         JALARM_TYPE_EXP = 11 拓展报警\r\n"
            "         JALARM_TYPE_MASK = 12 遮挡报警"
            "         JALARM_TYPE_HUMAN_DETECT = 13 人形侦测\r\n"
            "         JALARM_TYPE_FACE = 14 人脸侦测\r\n"
            "         JALARM_TYPE_PLATE = 15 车牌检测\r\n"
            "         JALARM_TYPE_PET = 16 混合宠物侦测\r\n"
            "         JALARM_TYPE_HUMAN_MIX = 17 混合人形侦测\r\n"
            "         JALARM_TYPE_EBIKE = 18 人形侦测\r\n"
            "         JALARM_TYPE_THROW = 19 高空抛物\r\n"
            "         JALARM_TYPE_CAR = 20 车型侦测\r\n"
            "         JALARM_TYPE_CIGARETTE = 21 吸烟侦测\r\n"
            "         JALARM_TYPE_PASSENGER_FS = 22 客流侦测\r\n"
            "         JALARM_TYPE_FALL = 23 跌倒侦测\r\n"
            "         JALARM_TYPE_BARCODE = 24 条形码\r\n"
            "         JALARM_TYPE_FACESNAP = 25 人脸抓拍\r\n"         },
        {"chn"   , "报警输入(0~3) 和 拓展报警时代表通道号（0~15）"},
        {"filter", "0，忽略规则过滤，1，启用规则过滤"},
        {"mail"     , "1 to send a mail, result: \r\n"
                      "\tMAIL_SUCC = 0,\r\n"
                      "\tMAIL_FAIL_UNKNOW = 1,\r\n"
                      "\tMAIL_FAIL_SVR = 2,\r\n"
                      "\tMAIL_FAIL_SSL = 3,\r\n"
                      "\tMAIL_FAIL_USER_PASSWD = 4,\r\n"
                      "\tMAIL_FAIL_ANTI_SPAM = 5,\r\n"},
        {"End"   , "alarmtest -? 获取帮助"                       },
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL         , 0                  },
        {"act"         , ARG_TYPE_ACT  , "set", action       , sizeof(action)     },
        {"alarmtype"   , ArgTypesInt   , "1~25"     , &type        , sizeof(int)        },
        {"chn"         , ArgTypesInt   , "0~15"    , &chn         , sizeof(int)        },
        {"filter"      , ArgTypesInt   , "0|1"     , &dorules     , sizeof(int)        },
        {"mail"        , ArgTypesInt   , "0|1"     , &mail        , sizeof(int)        },
        {"End"         , ArgTypesEnd   , NULL      , NULL         , 0                  },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        sprintf(buf, "only set alarm test");
    } else if(!strncasecmp("set", action,strlen("set"))) {
        if (mail && type) {
            int ret = record_email_text((JALARM_TYPE)type);
            sprintf(buf, "result=%d", ret);
            DBG("email test\n");
        }

        if (type) {
            SET_PARAM_RULE_CHECK(opts);
            int flag = (dorules == 1 ? NEED_TIME_CHECK : DONT_TIME_CHECK);
            alarm_report((JALARM_TYPE)type, chn, flag, "test_alarm");
        }
    }

    return SUCCESS;
}

int JCPDriveOutCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    DriveOut inner = {0,};
    DriveOut outer = {0,};

    HelpMsgS helps[] = {
        {"?"       , "驱赶参数设置"                    },
        {"act"     , "list:获取所有参数 set:设置参数"      },
        {"show"    , "驱赶显示"},
        {"enable"  , "驱赶使能"},
        {"whiteen" , "白光灯使能"},
        {"audioen" , "声音使能"    },
        {"time"    , "时间"},
        {"rest"    , "剩余时间"},
        {"End"     , "driveoutcfg -? 获取帮助"             },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK, NULL      , NULL             , 0             },
        {"act"       , ARG_TYPE_ACT, "list|set", action           , sizeof(action)},
        {"show"      , ArgTypesInt , "0|1"     , &outer.show      , sizeof(int)   },
        {"enable"    , ArgTypesInt , "0|1"     , &outer.enable    , sizeof(int)   },
        {"whiteen"   , ArgTypesInt , "0|1"     , &outer.whiteen   , sizeof(int)   },
        {"audioen"   , ArgTypesInt , "0|1"     , &outer.audioen   , sizeof(int)   },
        {"time"      , ArgTypesInt , "10~60"   , &outer.time      , sizeof(int)   },
        {"rest"      , ArgTypesInt , "0~60"    , &outer.rest_time , sizeof(int)   },
        {"End"       , ArgTypesEnd , NULL      , NULL             , 0             },
    };

    ret = get_config(handleDriveOutCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(DriveOut));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(DriveOut));
        ret = set_config(handleDriveOutCfg, outer);
        if (get_g_sys(factest)) {
             CUSTOMCONF_APPEND_INT(inner.show, outer.show, "1 /cfg/driveout/show %d\n", outer.show);
        }
    }

    return SUCCESS;
}

//2014-04-09 14:50:23
int str_to_tm(char *str, struct tm *ts)
{
    dbg_jcp("TIME : %s\n", str);
    int ret = sscanf(str,"%d-%d-%d %d:%d:%d", &ts->tm_year, &ts->tm_mon, &ts->tm_mday,
        &ts->tm_hour, &ts->tm_min, &ts->tm_sec);

    if (ret != 6) {
        DBG("sscanf ret = %d\n", ret);
        return -1;
    }

    //DBG("%d-%d-%d %d:%d:%d\n", ts->tm_year, ts->tm_mon, ts->tm_mday,
        //ts->tm_hour, ts->tm_min, ts->tm_sec);
    ts->tm_year -= 1900;
    ts->tm_mon -= 1;
    return 0;
}

int JCPCmdLightCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int ret = SUCCESS;
    int lt50isday = 0;
    int obsolete = 50;

    LightExtCfg inner = {0};
    LightExtCfg outer = {0};

    HelpMsgS helps[] = {
        {"?"            , "以太网设置"                                    },
        {"act"          , "list:获取所有参数 set:设置参数"                },
        {"mode"         , "0 红外灯；1 白光灯；2 双光源"                  },
        {"openlightlux" , "1: 强制白天算法(关灯); 100: 强制黑夜算法(开灯)"},
        {"closelightlux", "0~100; 关灯灵敏度"                             },
        {"End"          , "lightcfg -? 获取帮助"                          },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK, NULL      , NULL           , 0                     },
        {"act"          , ARG_TYPE_ACT, "list|set", action         , sizeof(action)        },
        {"mode"         , ArgTypesInt , "0~100"   , &outer.lamptype, sizeof(outer.lamptype)},
        {"openlightlux" , ArgTypesInt , "0~100"   , &lt50isday     , sizeof(int)           },
        {"closelightlux", ArgTypesInt , "0~100"   , &obsolete      , sizeof(int)           },
        {"End"          , ArgTypesEnd , NULL      , NULL           , 0                     },
    };

    ret = get_config(handleLightExtCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(inner));

    /*
    switch (inner.gt_1_forceday) {
    case 0:     lt50isday = 100; break;
    case 1:     lt50isday = 50; break;
    case 2:     lt50isday = 1; break;
    default:    lt50isday = 50; break;
    }
    */

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        /*
        switch (lt50isday) {
        case 1:     outer.gt_1_forceday = 2; break;
        case 50:    outer.gt_1_forceday = 1; break;
        case 100:   outer.gt_1_forceday = 0; break;
        default:    outer.gt_1_forceday = 1; break;
        }
        */

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(inner));
        ret = set_config(handleLightExtCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return ret;
}

int JCPCmdIRCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int ret = SUCCESS;
    int lt50isday = 0;
    int ledtest = 0;
    LightExtCfg inner = {0};
    LightExtCfg outer = {0};

    HelpMsgS helps[] = {
        {"?"           , "红外控制设置"                                    },
        {"act"         , "list:获取所有参数 set:设置参数"                  },
        {"webturnonlux", "1: 强制白天算法(关灯); 100: 强制黑夜算法(开灯)"  },
        {"ledtest"     , "0: 全关 1: 全开 2-3 开-关白光灯 4-5: 开-关红外灯"},
        {"ircutmode"   , "IRCUT模式选择"                                   },
        {"End"         , "ircfg -? 获取帮助"                               },
    };

    ArgOptS opts[] ={
        {"?"           , ARG_TYPE_ASK, NULL      , NULL                , 0                },
        {"act"         , ARG_TYPE_ACT, "list|set", action              , sizeof(action)   },
        {"webturnonlux", ArgTypesInt , "0~100"   , &lt50isday          , sizeof(lt50isday)},
        {"ircutmode"   , ArgTypesInt , "0~2"     , &outer.ircut_reverse, sizeof(int)      },
        {"ledtest"     , ArgTypesInt , "0~100"   , &ledtest            , sizeof(ledtest)  },
        {"End"         , ArgTypesEnd , NULL      , NULL                , 0                },
    };

    ret = get_config(handleLightExtCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(inner));

    switch (inner.gt_1_forceday) {
    case 0:     lt50isday = 1; break;
    case 1:     lt50isday = 100; break;
    case 2:     lt50isday = 50; break;
    default:    lt50isday = 50; break;
    }

    DBG("gt_1_forceday:%d ircutmode = %d\n", outer.gt_1_forceday, outer.ircut_reverse);

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        switch (lt50isday) {
        case 1:     outer.gt_1_forceday = 0; break;
        case 50:    outer.gt_1_forceday = 2; break;
        case 100:   outer.gt_1_forceday = 1; break;
        default:    outer.gt_1_forceday = 2; break;
        }
        DBG("gt_1_forceday:%d ircutmode = %d\n", outer.gt_1_forceday, outer.ircut_reverse);

        if (SUCCESS == arg_opt_if_set("ledtest", opts)) {
            send_event_chn(JEvent_LedTest, ledtest);
        }
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(inner));
        ret = set_config(handleLightExtCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}


int JCPCmdWhiteLedCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    WhiteLedS inner = {0,};
    WhiteLedS outer = {0,};
    int force_type = 0;

    HelpMsgS helps[] = {
        {"?"      , "白光灯控制"                                },
        {"act"    , "list:获取所有参数 set:设置参数"            },
        {"reverse", "高低电平反转，1:低.反转 0:高.不反转，默认0"},
        {"force_type", "1 强制打开白光灯，3分钟后关闭"},
        {"End"    , "whiteledcfg -? 获取帮助"                   },
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK, NULL        , NULL               , 0                     },
        {"act"         , ARG_TYPE_ACT, "list|set"  , action             , sizeof(action)        },
        {"reverse"     , ArgTypesInt , "0|1"       , &outer.reverse     , sizeof(outer.reverse) },
        {"force_type"  , ArgTypesInt , "0|1"       , &force_type        , sizeof(force_type)    },
        {"End"         , ArgTypesEnd , NULL        , NULL               , 0                     },
    };

    ret = get_config(handlewhiteledCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(WhiteLedS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        //if (force_type > 0) {
            //handle_force_white(&force_type);
            //return 0;
        //}
        LightExtCfg lightcfg = {0};
        conf_get_lightext_cfg(&lightcfg);
        lightcfg.is_wh_triglow = (outer.reverse > 0 ? 0:1);
        conf_set_lightext_cfg(lightcfg);

        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(WhiteLedS));
        ret = set_config(handlewhiteledCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.reverse, outer.reverse, "1 /cfg/WhiteLed/reverse %d\n", outer.reverse);
        }
    }

    return SUCCESS;

}

int JCPCmdGetIframe(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int chn = -1;

    HelpMsgS helps[] = {
        {"?"  , "码流控制"                                                         },
        {"act", "set:立即请求I帧"                                                  },
        {"chn", "0:立即请求主码流I帧，1:立即请求次码流I帧，100:同时请求主次码流I帧"},
        {"End", "getiframe -?获取帮助"                                             },
    };

    ArgOptS opts[] = {
        {"?"  , ARG_TYPE_ASK, NULL , NULL  , 0             },
        {"act", ARG_TYPE_ACT, "set", action, sizeof(action)},
        {"chn", ArgTypesInt , NULL , &chn  , sizeof(int)   },
        {"End", ArgTypesEnd , NULL , NULL  , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("set", action)) {
        if (chn == 0) {
            DBG("main\n");
            encode_immediate_iframe(CH_FS_MAIN0);
        } else if (chn == 1) {
            DBG("sub\n");
            encode_immediate_iframe(CH_FS_SUB0);
        } else if (chn >= 100) {
            DBG("main and sec\n");
            encode_immediate_iframe(CH_FS_MAIN0);
            encode_immediate_iframe(CH_FS_SUB0);
        }
    }

    return SUCCESS;

}

int JCPCmdLightExtCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int obsolete = 0;

    char action[JCP_ACTION_LEN] = {0};

    LightExtCfg inner = {0};
    LightExtCfg outer = {0};

    HelpMsgS helps[] = {
        {"?"              , "补光设置"                     },
        {"act"            , "list:获取所有参数 set:设置参数" },
        {"devtype"        , "0 红外灯；1 白光灯；2 双光源 3 星光夜视"},
        {"irswitchmode"   , "2 自动模式；3 定时模式；4 强制关灯；5 强制开灯"              },
        {"beginhour"      , "彩转黑时间开始 时"},
        {"beginmin"       , "彩转黑时间开始 分"        },
        {"endhour"        , "彩转黑时间结束 时"},
        {"endmin"         , "彩转黑时间结束 分"},
        {"fcolorbeginhour", "全彩模式彩转黑时间开始 时"},
        {"fcolorbeginmin" , "全彩模式彩转黑时间开始 分"},
        {"fcolorendhour"  , "全彩模式彩转黑时间结束 时"},
        {"fcolorendmin"   , "全彩模式彩转黑时间结束 分"},
        {"truestar"       , "0|1,是否为臻全彩设备"},
        {"lightcontrol"   , "0 强制开灯；1 自动；2 强制关灯"},
        {"openlightlux"   , "0~100,开灯灵敏度"},
        {"closelightlux"  , "0~100,关灯灵敏度"},
        {"lightgrade"     , "1~100,手动调节灯亮度"},
        {"lightev"        , "0|1,是否支持变光， 0 不支持 1 支持"},
        {"showautolight"  , "0|1,app自动开灯模式下是否显示自动调光选项, 0 不显示, 1显示"},
        {"autolighten"    , "可变灯开关，1 开启 自动调节光源 0 关闭 手动调节"},
        {"lightsuppress"  , "白光灯发蒙抑制,0 关闭 1 开启"},
        {"whitectrl"      , "白光灯高低电平控制，0 高电平 1 低电平"},
        {"irledctrl"      , "红外灯高低电平控制，0 高电平 1 低电平"},
        {"shinemode"      , "闪灯模式，0 全彩模式 1黑白模式 2 智能模式"},
        {"whitetime"      , "白光灯开启时间 0-60s"},
        {"ircut_reverse"  , "IRCUT模式选择 0-正向 1-反向"    },
        {"irledmode"      , "0: 读高， 1: 读低"},
        {"lampmode"       , "0: 硬光敏， 1: 软光敏"},
        {"lightboard"     , "灯板，0:红外灯板 1:白光灯板 2:双光灯板"},
        {"lighten"        , "小夜灯亮度"},
        {"fcopenevda"     , "全彩动态开灯曝光阈值"},
        {"fcopenevst"     , "全彩静态开灯曝光阈值"},
        {"fcopenearly"    , "全彩早开灯曝光阈值"},
        {"fcopenmiddle"   , "全彩中开灯曝光阈值"},
        {"fcopenlate"     , "全彩晚开灯曝光阈值"},
        {"fctargeevst"    , "全彩静态稳定曝光阈值"},
        {"fccloseevst"    , "全彩静态关灯曝光阈值"},
        {"fctargetratio"  , "全彩开灯情况下允许的波动比率"},
        {"fclightmaxev"   , "全彩灯光下sensor最大支持的曝光值"},
        {"fclightminev"   , "全彩灯光下sensor最小支持的曝光值"},
        {"End"            , "lightextcfg -? 获取帮助"           },
    };

    ArgOptS opts[] =
    {
        {"?"              , ARG_TYPE_ASK, NULL      , NULL                  , 0                            },
        {"act"            , ARG_TYPE_ACT, "list|set", action                , sizeof(action)               },
        {"devtype"        , ArgTypesInt , "0~3"     , &outer.lamptype       , sizeof(outer.lamptype)       },
        {"irswitchmode"   , ArgTypesInt , "0~5"     , &outer.alg            , sizeof(outer.alg)            },
        {"beginhour"      , ArgTypesInt , "0~23"    , &outer.beginhour      , sizeof(outer.beginhour)      },
        {"beginmin"       , ArgTypesInt , "0~59"    , &outer.beginmin       , sizeof(outer.beginmin)       },
        {"endhour"        , ArgTypesInt , "0~23"    , &outer.endhour        , sizeof(outer.endhour)        },
        {"endmin"         , ArgTypesInt , "0~59"    , &outer.endmin         , sizeof(outer.endmin)         },
        {"fcolorbeginhour", ArgTypesInt , "0~23"    , &outer.fcolorbeginhour, sizeof(outer.fcolorbeginhour)},
        {"fcolorbeginmin" , ArgTypesInt , "0~59"    , &outer.fcolorbeginmin , sizeof(outer.fcolorbeginmin) },
        {"fcolorendhour"  , ArgTypesInt , "0~23"    , &outer.fcolorendhour  , sizeof(outer.fcolorendhour)  },
        {"fcolorendmin"   , ArgTypesInt , "0~59"    , &outer.fcolorendmin   , sizeof(outer.fcolorendmin)   },
        {"truestar"       , ArgTypesInt , "0~1"     , &outer.truestar       , sizeof(outer.truestar)       },
        {"lightcontrol"   , ArgTypesInt , "0~2"     , &outer.gt_1_forceday  , sizeof(outer.gt_1_forceday)  },
        {"openlightlux"   , ArgTypesInt , "0~100"   , &outer.turn_on_pct    , sizeof(outer.turn_on_pct)    },
        {"closelightlux"  , ArgTypesInt , "0~100"   , &outer.turn_off_pct   , sizeof(outer.turn_off_pct)   },
        {"lightgrade"     , ArgTypesInt , "1~100"   , &outer.pwm_percent    , sizeof(outer.pwm_percent)    },
        {"lightev"        , ArgTypesInt , "0|1"     , &outer.pwm_ev         , sizeof(outer.pwm_ev)         },
        {"showautolight"  , ArgTypesInt , "0|1"     , &outer.showautolight  , sizeof(outer.showautolight)  },
        {"autolighten"    , ArgTypesInt , "0|1"     , &outer.adjustable     , sizeof(outer.adjustable)     },
        {"lightsuppress"  , ArgTypesInt , "0|1"     , &obsolete             , sizeof(obsolete)             },
        {"whitectrl"      , ArgTypesInt , "0|1"     , &outer.is_wh_triglow  , sizeof(outer.is_wh_triglow)  },
        {"irledctrl"      , ArgTypesInt , "0|1"     , &outer.is_ir_triglow  , sizeof(outer.is_ir_triglow)  },
        {"shinemode"      , ArgTypesInt , "0~2"     , &outer.shinemode      , sizeof(outer.shinemode)      },
        {"whitetime"      , ArgTypesInt , "0~60"    , &outer.shinetime      , sizeof(outer.shinetime)      },
        {"ircut_reverse"  , ArgTypesInt , "0~2"     , &outer.ircut_reverse  , sizeof(outer.ircut_reverse)  },
        {"irledmode"      , ArgTypesInt , "0|1"     , &outer.irledmode      , sizeof(outer.irledmode)      },
        {"lampmode"       , ArgTypesInt , "0|1"     , &outer.lampmode       , sizeof(outer.lampmode)       },
        {"lightboard"     , ArgTypesInt , "0~2"     , &outer.lightboard     , sizeof(outer.lightboard)     },
        {"lighten"        , ArgTypesInt , "0~100"   , &outer.nightled       , sizeof(outer.nightled)       },
        {"fcopenevda"     , ArgTypesFloat , "0.01~1000000.00" , &outer.fcopenevda   , sizeof(outer.fcopenevda)   },
        {"fcopenevst"     , ArgTypesFloat , "0.01~1000000.00" , &outer.fcopenevst   , sizeof(outer.fcopenevst)   },
        {"fcopenearly"    , ArgTypesFloat , "0.01~1000000.00" , &outer.fcopenearly  , sizeof(outer.fcopenearly)  },
        {"fcopenmiddle"   , ArgTypesFloat , "0.01~1000000.00" , &outer.fcopenmiddle , sizeof(outer.fcopenmiddle) },
        {"fcopenlate"     , ArgTypesFloat , "0.01~1000000.00" , &outer.fcopenlate   , sizeof(outer.fcopenlate)   },
        {"fctargeevst"    , ArgTypesFloat , "0.01~1000000.00" , &outer.fctargeevst  , sizeof(outer.fctargeevst)  },
        {"fccloseevst"    , ArgTypesFloat , "0.01~1000000.00" , &outer.fccloseevst  , sizeof(outer.fccloseevst)  },
        {"fctargetratio"  , ArgTypesFloat , "0.001~1000.00"   , &outer.fctargetratio, sizeof(outer.fctargetratio)},
        {"fclightmaxev"   , ArgTypesFloat , "0.01~1000000.00" , &outer.fclightmaxev , sizeof(outer.fclightmaxev) },
        {"fclightminev"   , ArgTypesFloat , "0.01~1000000.00" , &outer.fclightminev , sizeof(outer.fclightminev) },
        {"End"            , ArgTypesEnd , NULL      , NULL                  , 0                            },
    };

    ret = get_config(handleLightExtCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(LightExtCfg));

    JCP_ARG_PARSER(NULL, 0);

    if (outer.shinetime < 10) { // 不能设置10以下
        outer.shinetime = 10;
    }
    if (0 == outer.lampmode) {
        outer.lampmode = 1;
    }
    if (1 == outer.is_wh_triglow) {
        outer.is_wh_triglow = 0;
    }
    if (1 == outer.is_ir_triglow) {
        outer.is_ir_triglow = 0;
    }

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        char lampbuf[128] = {0};
        LightExtCfg lamp = {0};
        lamp_get_ev_lux(&lamp);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        sprintf(lampbuf, "fccurev=%f;", lamp.fccurev);
        strcat(buf, lampbuf);
    } else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);

        if(arg_opt_if_set("lighten",opts) != SUCCESS ) {
            JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(LightExtCfg));
        }

        LightAlarmS lightalarm = {0};
        ret = conf_get_lightalarm_cfg(&lightalarm);
        RETURN_FAIL_IF_API_ERR(ret);

        if (outer.lamptype == LAMP_IR || outer.lamptype == LAMP_STAR) {
            lightalarm.enable = FALSE;
        } else {
            lightalarm.enable = TRUE;
        }

        ret = set_config(handleLightAlarmCfg, lightalarm);
        RETURN_FAIL_IF_API_ERR(ret);


        ret = set_config(handleLightExtCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.shinetime    , outer.shinetime    , "1 /cfg/lightalarm/time %d\n"          , outer.shinetime);
            CUSTOMCONF_APPEND_INT(inner.irledmode    , outer.irledmode    , "1 /cfg/lightextcfg/irledmode %d\n"    , outer.irledmode);
            CUSTOMCONF_APPEND_INT(inner.lampmode     , outer.lampmode     , "1 /cfg/lightextcfg/lampmode %d\n"     , outer.lampmode);
            CUSTOMCONF_APPEND_INT(inner.is_wh_triglow, outer.is_wh_triglow, "1 /cfg/lightextcfg/is_wh_triglow %d\n", outer.is_wh_triglow);
            CUSTOMCONF_APPEND_INT(inner.is_ir_triglow, outer.is_ir_triglow, "1 /cfg/lightextcfg/is_ir_triglow %d\n", outer.is_ir_triglow);
        }
    }

    return ret;

}

int JCPCmdGetDayNightStatus(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int is_day = TRUE;
    char action[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"    , "获取日夜状态"                    },
        {"act"  , "list:获取所有参数"               },
        {"isday", "is_day: 0 夜晚；1 白天；2 白光灯"},
        {"End"  , "daylightscence -? 获取帮助"      },
    };

    ArgOptS opts[] = {
        {"?"    , ARG_TYPE_ASK, NULL  , NULL   , 0             },
        {"act"  , ARG_TYPE_ACT, "list", action , sizeof(action)},
        {"isday", ArgTypesInt , "0~2" , &is_day, sizeof(int)   },
        {"End"  , ArgTypesEnd , NULL  , NULL   , 0             },
    };

   // is_day = is_day_realtime();

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }

    return ret;
}

int JCPCmdEthCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int running = FALSE;

    char action[JCP_ACTION_LEN] = {0};

    NetEthS inner = {{0,},};
    NetEthS outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"       , "以太网设置"                            },
        {"act"     , "list:获取所有参数 set:设置参数"        },
        {"nic"     , "网卡名称"                              },
        {"ethip"   , "设备IP"                                },
        {"ethmask" , "设备掩码"                              },
        {"ethgw"   , "设备网关"                              },
        {"enable"  , "网口使能"                              },
        {"ethdhcp" , "dhcpc开关"                             },
        {"nreticle", "非标准长网线开关"                      },
        {"ipadaen" , "自适应ip，和dhcp保持互斥"              },
        {"dns"     , "DNS地址"                               },
        {"ethmac"  , "MAC地址"                               },
        {"ethmtu"  , "MTU"                                   },
        {"ipcheck" , "是否比较ip和网关，0:默认比较，1:不比较"},
        {"running" , "ifconfig running"                  },
        {"End"    , "ethcfg -? 获取帮助"                },
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK  , NULL       , NULL             , 0                 },
        {"act"     , ARG_TYPE_ACT  , "list|set" , (void*)action    , sizeof(action)    },
        {"nic"     , ArgTypesString, NULL       , (void*)outer.nic , sizeof(outer.nic) },
        {"ethip"   , ArgTypesString, NULL       , (void*)outer.ip  , sizeof(outer.ip)  },
        {"ethmask" , ArgTypesString, NULL       , (void*)outer.mask, sizeof(outer.mask)},
        {"ethgw"   , ArgTypesString, NULL       , (void*)outer.gw  , sizeof(outer.gw)  },
        {"enable"  , ARG_TYPE_LISTI, "0|1"      , &outer.enable    , sizeof(int)       },
        {"ethdhcp" , ArgTypesInt   , "0|1"      , &outer.dhcpen    , sizeof(int)       },
        {"nreticle", ArgTypesInt   , "0|1"      , &outer.reticle   , sizeof(int)       },
        {"ipadaen" , ArgTypesInt   , "0|1"      , &outer.ipadaen   , sizeof(int)       },
        {"dns"     , ArgTypesString, NULL       , (void*)outer.dns , sizeof(outer.dns) },
        {"ethmac"  , ArgTypesString, NULL       , (void*)outer.mac , sizeof(outer.mac) },
        {"ethmtu"  , ArgTypesInt   , "1300~1500", &outer.mtu       , sizeof(int)       },
        {"ipcheck" , ArgTypesInt   , "0|1"      , &outer.ipcheck   , sizeof(int)       },
        {"running" , ARG_TYPE_LISTI, "0|1"      , &running             , sizeof(int)           },
        {"End"     , ArgTypesEnd   , NULL       , NULL                 , 0                     },
    };

    //if (get_g_stat(AIRLINK_STATICIP)) {
    //    DBG("______________ modify ip is forbiden\n");
    //    return SUCCESS;
    //}
    ret = get_config(handleEthCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(NetEthS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        NetEthS info = {{0}};
        running = (net_link_status("eth0") == 1);
        get_ethinfo(&info);
        if (strlen(info.mac) != 0) {
            memcpy(outer.mac, info.mac, sizeof(info.mac));
        }
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(NetEthS));
        ret = set_config(handleEthCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPCmdUPNPCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    NetUpnpS inner = {0};
    NetUpnpS outer = {0};

    HelpMsgS helps[] = {
        {"?"       , "UPNP设置"                              },
        {"act"     , "list:获取所有参数 set:设置参数"        },
        {"rtspen"  , "流服务端口UPNP开关：1，启用；0，禁用"  },
        {"httpen"  , "网页访问端口UPNP开关：1，启用；0，禁用"},
        {"ftpen"   , "下载端口UPNP开关：1，启用；0，禁用"    },
        {"voiceen" , "音频对讲端口UPNP开关：1，启用；0，禁用"},
        {"updateen", "设备升级端口UPNP开关：1，启用；0，禁用"},
        {"End"     , "upnpcfg -? 获取帮助"                   },
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK, NULL      , NULL         , 0             },
        {"act"     , ARG_TYPE_ACT, "list|set", (void*)action, sizeof(action)},
        {"rtspen"  , ArgTypesInt , "0|1"     , &outer.rtsp  , sizeof(int)   },
        {"httpen"  , ArgTypesInt , "0|1"     , &outer.http  , sizeof(int)   },
        {"ftpen"   , ArgTypesInt , "0|1"     , &outer.ftp   , sizeof(int)   },
        {"voiceen" , ArgTypesInt , "0|1"     , &outer.voice , sizeof(int)   },
        {"updateen", ArgTypesInt , "0|1"     , &outer.update, sizeof(int)   },
        {"End"     , ArgTypesEnd , NULL      , NULL         , 0             },
    };

    ret = get_config(handleUpnpCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);
    memcpy(&outer, &inner, sizeof(NetUpnpS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(NetUpnpS));
        ret = set_config(handleUpnpCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return ret;

}

int JCPCmdSdcard(char *buf, int buflen, int argc, char **argv)
{
    SdcardS outer = {{0}};
    HelpMsgS helps[] = {
        {"?"   , "SD hotplug"                },
        {"act" , "add|remove|stop"           },
        {"path", "挂载路径"                   },
        {"End" , "sdcard -? 获取帮助"         },
    };

    ArgOptS opts[] = {
        {"?"   , ARG_TYPE_ASK                 , NULL                , NULL      , 0                 },
        {"act" , (ArgTypesMust|ArgTypesString), "add|remove|stop"   , outer.act , sizeof(outer.act) },
        {"path", (ArgTypesMust|ArgTypesString), NULL                , outer.path, sizeof(outer.path)},
        {"End" , ArgTypesEnd                  , NULL                , NULL      , 0                 },
    };

    JCP_ARG_PARSER(NULL, 0);

    SYSLOG("sdcard event[%s][%s]\n", outer.act, outer.path);

    if(!strncasecmp("add", outer.act,strlen("add"))) {
        sdcard_is_change();

        //修复一次最新的tmp文件
        repair_last_record(outer.path);

        firmware_answer(false);

        record_storage_dev_manage();

        record_storage_dev_add(outer.path);
        
        clr_g_stat(record, ~SD_REC_FORMAT);  // 插拔卡只保留格式化状态，其它全部清除
        chk_sdstat();

        // refresh_recalarm_flag();  //检测是否换卡,换卡需重新读取flag

        if (0 == access(FACTORY_SDFIRELOG, F_OK)) {
            UtilSystemCmd((char *)"telnetd -p24");
            toggle_redirect(1);
        }
    } else if(!strncasecmp("remove", outer.act,strlen("remove"))) {
        clr_g_stat(record, __FF__ & (~(SD_REC_TMP_REPAIR | SD_ERR_STOP)));

        record_storage_dev_manage();

        toggle_redirect(0);

#ifdef PLATFORM_GB
        gb_stop_playback();
#endif

        close_all_record_connection();

#ifdef PLATFORM_TENCENT
        replay_stop_allchn();
#endif

        uninit_record_watch();

        record_storage_dev_remove(outer.path);

        init_record_watch();

        if (get_g_stat(record, SD_ERR_WRITE_PROTECT)) {
            // 因为写保护的卡概率拔卡后 /mnt 下还有数据
            SYSLOG("______ sdcard removing with ro, REBOOT ______\n");
            DELAY_REBOOT_LINUX();
            return SUCCESS;
        }

        int need_rebulid = 0;
        send_event_data(JEvent_AlarmRMRcord, &need_rebulid);
        SYSLOG("______ sdcard removing ______\n");
        UtilSystemCmd((char *)"/ipc/bin/lzbox fat &");
    } else if(!strncasecmp("stop", outer.act,strlen("stop"))) {
        toggle_redirect(0);

#ifdef PLATFORM_GB
        gb_stop_playback();
#endif
        close_all_record_connection();

#ifdef PLATFORM_TENCENT
        replay_stop_allchn();
#endif
        uninit_record_watch();

        record_storage_dev_remove(outer.path);

        init_record_watch();
    }

    return SUCCESS;
}

int JCPCmdPortCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    NetPortS inner = {0};
    NetPortS outer = {0};

    HelpMsgS helps[] = {
        {"?"     , "常用服务端口设置"              },
        {"act"   , "list:获取所有参数 set:设置参数"},
        {"web"   , "WEB网页访问端口"               },
        {"rtsp"  , "RTSP流服务端口"                },
        {"ftp"   , "FTP下载服务端口"               },
        {"audio" , "音频对讲端口"                  },
        {"update", "设备升级端口"                  },
        {"End"   , "portcfg -? 获取帮助"           },
    };

    ArgOptS opts[] =
    {
        {"?"     , ARG_TYPE_ASK, NULL      , NULL                    , 0             },
        {"act"   , ARG_TYPE_ACT, "list|set", (void*)action           , sizeof(action)},
        {"web"   , ArgTypesInt , "1~65535" , (void*)&outer.httpport, sizeof(int)   },
        {"rtsp"  , ArgTypesInt , "1~65535" , (void*)&outer.rtspport, sizeof(int)   },
        {"ftp"   , ArgTypesInt , "1~65535" , (void*)&outer.ftpport, sizeof(int)   },
        {"audio" , ArgTypesInt , "1~65535" , (void*)&outer.audioport, sizeof(int)   },
        {"update", ArgTypesInt , "1~65535" , (void*)&outer.updateport, sizeof(int)   },
        {"End"   , ArgTypesEnd , NULL      , NULL                    , 0             },
    };

    ret = get_config(handleNetPortCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);
    memcpy(&outer, &inner, sizeof(NetPortS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        UPNP_MAP_S upnp = {0};
        char upnpbuf[128] = {0};
        get_upnp_map_info(&upnp);
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        sprintf(upnpbuf, "web_upnp=%d;ftp_upnp=%d;rtsp_upnp=%d;audio_upnp=%d;update_upnp=%d;",
            upnp.ports[SYSTEM_PORT_WEB].extPort,
            upnp.ports[SYSTEM_PORT_FTP].extPort,
            upnp.ports[SYSTEM_PORT_RTSP].extPort,
            upnp.ports[SYSTEM_PORT_VOICE].extPort,
            upnp.ports[SYSTEM_PORT_UPDATE].extPort);
        strcat(buf, upnpbuf);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(NetPortS));
        ret = set_config(handleNetPortCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPCmdOSDTxtCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    OsdExpandS inner = {0};
    OsdExpandS outer = {0};

    OsdExpand0 *z = NULL;

    HelpMsgS helps[] = {
        {"?"      , "多行osd扩展设置,支持8行设置"                  },
        {"act"    , "list:获取所有参数 set:设置参数"               },
        {"size"   , "字体大小 0:小 1:中 2:大"                      },
        {"font"   , "字体(暂保留用osdstylecfg设置)"                },
        {"gnum"   , "支持设置的osd行数"                            },
        {"index"  , "osd行序号id"                                  },
        {"enable" , "0，禁用osd；1，使能osd"                       },
        {"left"   , "最左上像素x坐标（以1080P x坐标为参考）"},
        {"top"    , "最左上像素y坐标（以1080P y坐标为参考）"},
        {"content", "要显示的内容，输入和输出都是UTF-8的编码格式\r\n"
                    "         否则会是乱码。\r\n"
                    "         如果内容要支持空格则需在内容两端加\" \r\n"},
        {"End"    , "osdstrcfg -?获取帮助"                         },
    };

    ArgOptS opts[] =
    {
        {"?"      , ARG_TYPE_ASK        , NULL      , NULL              , 0                 },
        {"act"    , ARG_TYPE_ACT        , "list|set", (void*)action     , sizeof(action)    },
        {"size"   , ArgTypesInt         , "0~2"   , (void*)&outer.size, sizeof(int)       },
        {"font"   , ArgTypesInt         , "0~3"     , (void*)&outer.font, sizeof(int)       },
        {"gnum"   , ArgTypesListOnly    , "10"       , NULL              , 0                 },
        {"index"  , ARG_TYPE_LIST_ID    , "0~9"     , (void*)&z->id     , sizeof(int)       },
        {"enable" , ARG_TYPE_LIST_MEMINT, "0|1"     , (void*)&z->enable , sizeof(int)       },
        {"left"   , ARG_TYPE_LIST_MEMINT, "0~1919"  , (void*)&z->x      , sizeof(int)       },
        {"top"    , ARG_TYPE_LIST_MEMINT, "0~1079"  , (void*)&z->y      , sizeof(int)       },
        {"content", ARG_TYPE_LIST_MEMSTR, NULL      , (void*)z->content , sizeof(z->content)},
        {"End"    , ArgTypesEnd         , NULL      , NULL              , 0                 },
    };
    ret = get_config(handleOsdExpandCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(OsdExpandS));

    JCP_ARG_PARSER(outer.cusosd, sizeof(OsdExpand0));

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        int len = sprintf(buf, "gnum=%d;", OSD_EXPAND_MAX_CHN);
        ASMJCP_LIST_STRING(buf+len, buflen, opts);
        ASMJCP_LIST_COUNT(inner.cusosd, sizeof(OsdExpand0), OSD_EXPAND_MAX_CHN);

    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(OsdExpandS));
        ret = set_config(handleOsdExpandCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPCmdDevRcrdCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};
    char timestrategy[128] = {0};
    char diskbuf[1024] = {0};

    RecordCtrlS inner = {0};
    RecordCtrlS outer = {0};
    int  has_exfat = FALSE;
    int  to_format = FALSE;
    //int  to_promote = FALSE;

    HelpMsgS helps[] = {
        {"?"           , "录像控制参数设置"                                        },
        {"act"         , "list:获取所有参数 set:设置参数"                          },
        {"recordtime"  , "报警时录像时间，单位秒"                                  },
        {"filetime"    , "单个文件最大录像时长  单位 分钟"                         },
        {"diskreserve" , "单个磁盘保留空间，单位MB，（当前不作配置，默认100M)"     },
        {"diskstrategy", "磁盘满策略. 0 is stop. 1 is delete（循环录)"             },
        {"filestrategy", "文件满策略. 0 is stop. 1 is switch（暂不使用，默认使用1)"},
        {"schedule"    , "布防时间描述\r\n"
         "        时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
         "        低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
         "        如全时段禁用：0,0,0,0,0,0,0\r\n"
         "        365中的格式是二元组的形式\r\n"
         "        timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"},
        {"rcrdchn", "录像的视频大小，必须是现有的视频通道中的一个。VENC_SIZE_E\r\n"
            "         VencSizeE_QCIF = 0,      // QVGA\r\n"
            "         VencSizeE_CIF = 1,       // CIF\r\n"
            "         VencSizeE_D1 = 2,        // D1\r\n"
            "         VencSizeE_720P = 3,      // 720P\r\n"
            "         VencSizeE_UVGA = 4,      // UVGA \r\n"
            "         VencSizeE_1080P = 5,     // 1080P\n"
            "         VencSizeE_QVGA = 6,      // QVGA\r\n"
            "         VencSizeE_VGA = 7,       // VGA\r\n"
            "         VencSizeE_960P = 8,      // 960P\r\n"
            "         VencSizeE_3M = 9,        // 3M\r\n"
            "         VencSizeE_180P = 10,     // 180P\r\n"
            "         VencSizeE_360P = 11,     // 360P\r\n"
            "         VencSizeE_4M = 12,     // 4M\r\n"
            "         VencSizeE_5M = 13,     // 5M"},
        {"prerecorden", "是否启用预录制，功能0:不启用 1:启用"},
        {"prerecordtime", "预录制时间"                      },
        {"recorden"     , "1，开启录像；0，录像停止"           },
        {"rec_type"     , "0:主码流， 1：子码流"           },
        {"sd_times"     , "0~1,1倍容量  2：2倍功能 4：4倍功能 8:8倍功能"           },
        {"has_exfat"    , "1:新版本SD策略，支持exfat 0:不支持"},
        {"to_promote"    ,"1:fat32升级为 exfat"},
        {"to_format"    , "1:格式化 0:不格式"},
        {"sd_stat"      , "0:正常 1:读错误 2:写错误 3:读写错误"},
        {"End"        , "devrecordcfg -?获取帮助"            },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK  , NULL       , NULL                , 0                   },
        {"act"          , ARG_TYPE_ACT  , "list|set" , action              , sizeof(action)      },
        {"recordtime"   , ArgTypesInt   , "10~150"   , &outer.alarmseconds , sizeof(int)         },
        {"filetime"     , ArgTypesInt   , "1~60"     , &outer.schedminutes , sizeof(int)         },
        {"diskreserve"  , ArgTypesInt   , "10~100000", &outer.diskreservemb, sizeof(int)         },
        {"diskstrategy" , ArgTypesInt   , "0|1"      , &outer.diskstrategy , sizeof(int)         },
        {"filestrategy" , ArgTypesInt   , "0|1"      , &outer.filestrategy , sizeof(int)         },
        {"schedule"     , ArgTypesString, NULL       , timestrategy        , sizeof(timestrategy)},
        {"prerecorden"  , ArgTypesInt   , "0|1"      , &outer.prerecord    , sizeof(int)         },
        {"prerecordtime", ArgTypesInt   , "5~10"     , &outer.prerecordtime, sizeof(int)         },
        {"recorden"     , ARG_TYPE_SETI , "0|1|2"    , &outer.isrecording  , sizeof(int)         },
        {"rec_type"     , ArgTypesInt   , "0|1"      , &outer.rec_type     , sizeof(int)         },
        {"sd_times"     , ArgTypesInt   , "0~10"     , &outer.sd_times     , sizeof(int)         },
        {"has_exfat"    , ARG_TYPE_LISTI, "0|1"      , &has_exfat          , sizeof(int)         },
        {"to_format"    , ARG_TYPE_LISTI, "0|1"      , &to_format          , sizeof(int)         },
        {"sd_stat"      , ArgTypesInt   , "0~99"     , &outer.sd_stat      , sizeof(int)         },
        {"End"          , ArgTypesEnd   , NULL       , NULL                , 0                   },
    };

    ret = get_config(handleRecordCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(RecordCtrlS));
    outer.isrecording = 2;

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        // 每次 list 检查 SD_CD_IN
        // char result[4] = {0};
        // LoadFile(F_MMC_PRESENT, result, sizeof(result));
        // int curr = (result[0] == '1') ? SD_CD_IN : 0;
        // if (curr ^ get_g_stat(record, SD_CD_IN)) {
        //    chk_sdstat();
        // }
        outer.sd_stat = get_sdstat();
        has_exfat = fsck_exfat();
        to_format = get_g_stat(record, SD_ERR_UNAUTH | SD_ERR_ACCESS3) ? 1 : 0;  // 未认证,读写或挂载错

        LIST_PARAM_RULE_CHECK(opts);

        intarray_to_timestr(timestrategy, inner.timestrategy);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        ret = record_get_storage_info(&inner, diskbuf, sizeof(diskbuf));
        if (SUCCESS == ret) {
            strcat(buf, diskbuf);
        }
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.timestrategy);
        if (1 == outer.isrecording) {
            record_request_rec(JREC_TYPE_MANUAL);
        } else if (0 == outer.isrecording){
            record_request_stop();
        }

        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.rec_type, outer.rec_type, "1 /cfg/recordCtrl/rec_type %d\n", outer.rec_type);
        }

        outer.isrecording = 0;
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(RecordCtrlS));
        ret = set_config(handleRecordCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPCmdEmailCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    EmailS inner = {{0,},};
    EmailS outer = {{0,},};
    HelpMsgS helps[] = {
        {"?"         , "email服务设置，用账户user发送邮件给sendto"},
        {"act"       , "list  get parameter; set  set parameter"  },
        {"smtpserver", "smtp提交商，如：smtp.163.com"             },
        {"toaddr"    , "邮件接收者，如：anonymous@163.com"        },
        {"smtpuser"  , "用户名"                                   },
        {"smtppasswd", "用户密码"                                 },
        {"End"       , "emailcfg -?获取帮助"                      },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK  , NULL      , NULL                   , 0                       },
        {"act"       , ARG_TYPE_ACT  , "list|set", (void*)action          , sizeof(action)          },
        {"smtpserver", ArgTypesString, NULL      , (void*)outer.smtpserver, sizeof(outer.smtpserver)},
        {"toaddr"    , ArgTypesString, NULL      , (void*)outer.sendto    , sizeof(outer.sendto)    },
        {"smtpuser"  , ArgTypesString, NULL      , (void*)outer.user      , sizeof(outer.user)      },
        {"smtppasswd", ArgTypesString, NULL      , (void*)outer.password  , sizeof(outer.password)  },
        {"End"       , ArgTypesEnd   , NULL      , NULL                   , 0                       },
    };

    ret = get_config(handleEmailCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(EmailS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(EmailS));
        ret = set_config(handleEmailCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPCmdSysCtrl(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    AutoRebootS inner = {0};
    AutoRebootS outer = {0};
    int cmd = -1;
    int len = 0;

    HelpMsgS helps[] = {
        {"?"      , "系统定时重启设置, 一些系统控制, 系统状态"               },
        {"act"    , "list:获取所有参数 set:设置参数"},
        {"cmd"    , "0 reboot 重启设备, 1 reset 重启服务, 2 default, 3 简单复位, 11 SEGV"},
        {"arben"  , "auto reboot enable 0|1"        },
        {"arbtm"  , "单位小时, 对应一天的24个小时, 比如12对应中午的12点整"   },
        {"arbweek", " 0~6对应星期天~星期六 7对应每天"                        },
        {"End"    , "sysctrl -?获取帮助"            },
    };

    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK , NULL      , NULL            , 0                      },
        {"act"    , ARG_TYPE_ACT , "list|set", action          , sizeof(action)         },
        {"cmd"    , ARG_TYPE_SETI, "0|1|2|3|11", &cmd          , sizeof(cmd)            },
        {"arben"  , ArgTypesInt  , "0|1"     , &outer.enable   , sizeof(outer.enable)   },
        {"arbtm"  , ArgTypesInt  , "0~23"    , &outer.alarmhour, sizeof(outer.alarmhour)},
        {"arbweek", ArgTypesInt  , "0~7"     , &outer.alarmday , sizeof(outer.alarmday) },
        {"End"    , ArgTypesEnd  , NULL      , NULL            , 0                      },
    };

    ret = get_config(handleAutoRebootCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(AutoRebootS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        strcat(buf, "status=");
        len = strlen(buf);
        system_status_info(buf+len);
        strcat(buf, ";");
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);

        if (cmd == 11/*SIGSEGV*/) {
            SEND_SEGV_SELF();
        }

        if (cmd >= 0 && cmd <= 3) {
            SYSLOG("a sysctrl cmd[%d] from jcpcmd\n", cmd);
            return handleSysCtrlCfg(&cmd, NULL, 0, NULL, NULL);
        }

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(AutoRebootS));
        ret = set_config(handleAutoRebootCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        chanage_auto_reboot(outer);
    }

    return SUCCESS;
}

int JCPCmdVersion(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    SysInfoS inner = {{0,},};
    SysInfoS outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"        , "系统信息，所有选项都为只读(设备名称可设置)"},
        {"act"      , "list:获取所有参数 set:设置参数"            },
        {"solution" , "解决方案{ti | nxp | hisi}"                 },
        {"platform" , "平台类型"                                  },
        {"jcpver"   , "jcpver目前版本为3.00"                      },
        {"kernelver", "内核版本 only read"                        },
        {"serverver", "服务版本 only read"                        },
        {"webver"   , "网页版本 only read"                        },
        {"devname"  , "设备名称  "                                },
        {"devtype"  , "设备型号 JCO-xx-xx only read"              },
        {"devid"    , "设备出厂ID only read"                      },
        {"devtype_select", "设备类型显示方式0:显示mcu上传的，1:定制的，2:不显示"},
        {"custom_ui", "代表哪个客户定制的网页"                    },
        {"custom_appid", "app定制 30000:中性版本"                },
        {"ver_lite" , "设备子版本号，用于定制版本"                },
        {"verifystr", "获取IPC官方认证，只在act=set时有效，返回键值对<key:val>" },
        {"air_burn" , "区分设备类型 APC" },
        {"End"      , get_fw_ver()      },
    };

    ArgOptS opts[] =
    {
        {"?"             , ARG_TYPE_ASK  , NULL      , NULL                 , 0                      },
        {"act"           , ARG_TYPE_ACT  , "list|set", action               , sizeof(action)         },
        {"solution"      , ARG_TYPE_LISTS, NULL      , outer.solution       , sizeof(outer.solution) },
        {"platform"      , ARG_TYPE_LISTS, NULL      , outer.platform       , sizeof(outer.platform) },
        {"jcpver"        , ARG_TYPE_LISTS, NULL      , outer.plugver        , sizeof(outer.plugver)  },
        {"kernelver"     , ARG_TYPE_LISTS, NULL      , outer.kernelver      , sizeof(outer.kernelver)},
        {"serverver"     , ARG_TYPE_LISTS, NULL      , outer.serverver      , sizeof(outer.serverver)},
        {"webver"        , ARG_TYPE_LISTS, NULL      , outer.webver         , sizeof(outer.webver)   },
        {"devname"       , ArgTypesString, NULL      , outer.devname        , sizeof(outer.devname)  },
        {"devtype"       , ArgTypesString, NULL      , outer.devtype        , sizeof(outer.devtype)  },
        {"devid"         , ARG_TYPE_LISTS, NULL      , outer.devid          , sizeof(outer.devid)    },
        {"devtype_select", ArgTypesInt   , "0~2"     , &outer.devtype_select, sizeof(int)            },
        {"custom_ui"     , ARG_TYPE_LISTS, NULL      , outer.custom_ui      , sizeof(outer.custom_ui)},
        {"custom_appid"  , ArgTypesString, NULL      , outer.custom_appid   ,sizeof(outer.custom_appid)},
        {"ver_lite"      , ArgTypesInt   , NULL      , &outer.ver_lite      , sizeof(outer.ver_lite) },
        {"verifystr"     , ArgTypesString, NULL      , outer.verifystr      , sizeof(outer.verifystr)},
        {"air_burn"      , ArgTypesString, NULL      , outer.air_burn      , sizeof(outer.air_burn)  },
        {"End"           , ArgTypesEnd   , NULL      , NULL                 , 0                      },
    };

    ret = get_config(handleSysInfoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(SysInfoS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        char config[128] = {0};
        snprintf(config, buflen - strlen(buf) - 1, "%s;", "chkpk=1");
        strcat(buf,config);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(SysInfoS));

        ret = set_config(handleSysInfoCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);

        char bak[64] = {0};
        char str[64] = {0};

        if (0 != strlen(outer.verifystr)) {
            strncpy(bak, outer.verifystr, sizeof(bak)-1);
            strncpy(str, outer.verifystr, sizeof(str)-1);
            do_verifystr(str, bak, sizeof(bak));
            size_t verifystr_size = sizeof(outer.verifystr);
            if (strlen(str) + strlen(bak) + 2 <= verifystr_size) {
                snprintf(buf, verifystr_size-1, "%s:%s", str, bak);
            } else {
                 ERR("Error: Buffer too small to hold formatted string\n");
            }
        }
    }
    return SUCCESS;

}

int JCPCmdotainfo(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    OtaInfoS inner = {{0,},};
    OtaInfoS outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"            , "获取OTA的版本信息及设置升级包的URL"          },
        {"act"          , "list:获取所有参数"                           },
        {"devclass"     , "设备分类(IPC/NVR/SHAKE/CARD/FISHEYE/APIPC)"  },
        {"cpu"          , "cpu型号"                                     },
        {"flash"        , "Flash大小，单位M"                            },
        {"version"      , "完整版本号"                                  },
        {"compiledate"  , "编译日期，比如20190724"                      },
        {"customer"     , "定制厂家，通用的为GEN"                       },
        {"sensor"       , "Sensor型号"                                  },
        {"devtype"      , "类型"                                        },
        {"upgrade_time" , "升级时间"                                    },
        {"End"          , "otainfocfg -act list"                        },
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL               , 0                         },
        {"act"         , ARG_TYPE_ACT  , "list|set", action             , sizeof(action)            },
        {"devclass"    , ArgTypesString, NULL      , outer.devclass     , sizeof(outer.devclass)    },
        {"cpu"         , ArgTypesString, NULL      , outer.cpu          , sizeof(outer.cpu)         },
        {"flash"       , ArgTypesString, NULL      , outer.flash        , sizeof(outer.flash)       },
        {"version"     , ArgTypesString, NULL      , outer.version      , sizeof(outer.version)     },
        {"compiledate" , ArgTypesString, NULL      , outer.compiledate  , sizeof(outer.compiledate) },
        {"customer"    , ArgTypesString, NULL      , outer.customer     , sizeof(outer.customer)    },
        {"sensor"      , ArgTypesString, NULL      , outer.Sensor       , sizeof(outer.Sensor)      },
        {"devtype"     , ArgTypesString, NULL      , outer.DevType      , sizeof(outer.DevType)     },
        {"upgrade_time", ArgTypesInt   , "0~65536" , &outer.upgrade_time, sizeof(outer.upgrade_time)},
        {"End"         , ArgTypesEnd   , NULL      , NULL               , 0                         },
    };

    ret = get_config(handleotainfoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(OtaInfoS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);

        ASMJCP_LIST_STRING(buf, buflen, opts);
    }

    return SUCCESS;

}

int JCPCmdotaUpgradeUrl(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};

    OtaUpgradeUrlS inner = {{0,},};
    OtaUpgradeUrlS outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"        , "设备发送升级路径"            },
        {"act"      , "set:设置参数"            },
        {"url"      , "设置升级包URL"            },
        {"End"      , "设备发送升级路径"            },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK  , NULL          , NULL            , 0                      },
        {"act"      , ARG_TYPE_ACT  , "set"         , action          , sizeof(action)         },
        {"url"      , ArgTypesString, NULL          , outer.url       , sizeof(outer.url)      },
        {"End"      , ArgTypesEnd   , NULL          , NULL            , 0                      },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(OtaUpgradeUrlS));

        //获取升级文件;开始升级
        DownFileThread(&outer);
    }

    return SUCCESS;
}
int JCPCmdNtpCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    SysNtpS outer = {0};
    SysNtpS inner = {0};

    HelpMsgS helps[] = {
        {"?"       , "ntp服务器设置"                 },
        {"act"     , "list:获取所有参数 set:设置参数"},
        {"ntpen"   , "是否启用NTP 1: 启用 0:禁止"    },
        {"ntphost" , "NTP服务器地址"                 },
        {"ntpport" , "NTP服务器端口，标准端口是123"  },
        {"interval", "同步时间间隔，分钟为单位"      },
        {"End"     , "ntpcfg -?获取帮助"             },
    };

    ArgOptS opts[] =
    {
        {"?"      , ARG_TYPE_ASK  , NULL      , NULL           , 0                      },
        {"act"    , ARG_TYPE_ACT  , "list|set", action         , sizeof(action)         },
        {"ntpen"  , ArgTypesInt   , "0|1"     , &outer.enable  , sizeof(int)            },
        {"ntphost", ArgTypesString, NULL      , outer.ntpserver, sizeof(outer.ntpserver)},
        {"ntpport", ArgTypesInt   , "1~65535" , &outer.ntpport , sizeof(outer.ntpport)},
        {"interval", ArgTypesInt  , "1~65535"  , &outer.interval, sizeof(outer.interval)},
        {"End"    , ArgTypesEnd   , NULL      , NULL           , 0                      },
    };

    ret = get_config(handleNtpcfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);
    memcpy(&outer, &inner, sizeof(SysNtpS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(SysNtpS));

        ret = set_config(handleNtpcfg, outer);

        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

//是否需要按照POSIX时间规范，返回时间
int JCPCmdTimeCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN+8] = {0};
    time_t sec_utc = 0;

    TzoneS inner = {0};
    TzoneS outer = {0};

    HelpMsgS helps[] = {
        {"?"       , "时间设置"                                                       },
        {"act"     , "list: 获取时间列表  set : 设置时间 poweron_sync: 低成本上电同步"},
        {"time"    , "从 1970-1-1 到现在 UTC时间 的秒数 "                             },
        {"timezone", "时区"                                                           },
        {"End"     , "timecfg -? 获取帮助"                                            },
    };

    ArgOptS opts[] = {
        {"?"       , ARG_TYPE_ASK  , NULL                   , NULL              , 0                        },
        {"act"     , ARG_TYPE_ACT  , "list|set|poweron_sync", (void*)action     , sizeof(action)           },
        {"time"    , ArgTypesInt   , "1~"                   , &sec_utc          , sizeof(sec_utc)          },
        {"timezone", ArgTypesInt   , "0~34"                 , &outer.idx        , sizeof(outer.idx)        },
        {"End"     , ArgTypesEnd   , NULL                   , NULL              , 0                        },
    };

    get_config(handleTimeZoneCfg, inner);
    memcpy(&outer, &inner, sizeof(TzoneS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        struct tm nowtm;
        char tmbuf[64] = {0};
        time(&sec_utc);
        localtime_r(&sec_utc, &nowtm);
        strftime(tmbuf, sizeof(tmbuf), "timestr=%F %H:%M:%S;", &nowtm);

        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        strcat(buf, tmbuf);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);

        //两个时区偏移不一样
        if (SUCCESS == arg_opt_if_set("timezone", opts)) {
            SYSLOG("set static timezone\n");
            if (inner.idx != outer.idx) {
                SYSLOG("set static timezone\n");
                clear_timesync();
                dump_tz_idx(outer.idx);
                outer.sec_east = get_tz_seceast();
                set_config(handleTimeZoneCfg, outer);
                return SUCCESS;
            }
        }

        if (SUCCESS == arg_opt_if_set("time", opts)) {
            SYSLOG("Set sec_utc: %lld\n", sec_utc);
            if (SUCCESS == check_timesync(sec_utc) && sec_utc != 0) {
                set_config(handleTimeCfg, sec_utc);
            }
        }
    } else if(!strncasecmp("poweron_sync", action,strlen("poweron_sync"))) { // && is_board_lowcost()
        static int synced = false;
        SET_PARAM_RULE_CHECK(opts);

        if (sec_utc > 0 && (!synced)) {
            SYSLOG("sync from poweron, Time: %lld\n", sec_utc);
            if (SUCCESS != check_timesync(sec_utc)) {
                synced = true;
                return SUCCESS;
            }

            set_config(handleTimeCfg, sec_utc);
            synced = true;
        }
    }

    return SUCCESS;

}


/*
    成功返回[Success], 返回result 的值代码用户权限
    0: admin    1:operator  2: user
    失败返回[Error], 返回result 的值代表错误代号
    -1 : 获取参数错误
    -2 : 用户已经存在
    -3 : 用户不存在
    -4 : 用户数目已上限
*/

int JCPCmdUserPasswdCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    SysUserS inner = {0};
    SysUserS outer = {0};
    SysUser0 user = {0};

    HelpMsgS helps[] = {
        {"?"    , "用户密码管理，add 或 del用户后要用 -list 命令选项刷新用户列表，以更新相应的ID\r\n"
                   "        只支持8个用户设备，默认3个用户，且不能删除"},
        {"act"  , "list 用户列表  set 修改; add 新增; del 删除\r\n"
         "     set和add后面必须带id和参数,在参数输入时就需过滤,底层不另外处理\r\n"
         "     del 必须带id，不带其他参数, 一次只能删除一个"},
        {"gnum" , "用户数"                                                   },
        {"group", "0 admin组  管理员，有设置、控制、浏览视频权限\r\n"
                    "        1 operator组  操作员，有控制、浏览视频权限\r\n"
                    "        2 user组  用户，有浏览视频权限"},
        {"user"    , "登录用户名"                   },
        {"password", "用户名密码"                   },
        {"End"     , "userpasswd -? 获取帮助;"
         "如需删除用户名为admin1的用户，则输入userpasswd -act del -user admin1\r\n"
         "           返回结果：result=-3，说明用户名不存在。\r\n"
         "           result=-4, 用户达到最大数。\r\n"
         "           result=-2， 说明用户名已存在，不能增加。"},
    };

    ArgOptS opts[] = {
        {"?"       , ARG_TYPE_ASK  , NULL              , NULL            , 0                       },
        {"act"     , ARG_TYPE_ACT  , "list|set|add|del", action          , sizeof(action)          },
        {"gnum"    , ARG_TYPE_LISTI, "0~8"              , &outer.gnum     , sizeof(outer.gnum)      },
        {"group"   , ArgTypesInt   , "0~2"             , &user.group     , sizeof(user.group)      },
        {"user"    , ArgTypesString, NULL              , user.username   , sizeof(user.username)   },
        {"password", ArgTypesString, NULL              , user.cryptpasswd, sizeof(user.cryptpasswd)},
        {"End"     , ArgTypesEnd   , NULL              , NULL            , 0                       },
    };
    ret = get_config(handleUserCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(SysUserS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        int i = 0;
        int len = 0;
        len += sprintf(buf+len, "gnum=%d;", inner.gnum);
        for (i = 0; i < USER_MAX_NUM; i++) {
            if (strlen(inner.user[i].username) != 0) {
                len += sprintf(buf+len, "id=%d;user=%s;group=%d#",
                    i, inner.user[i].username, inner.user[i].group);
            }
        }
        return SUCCESS;
    } else if(!strncasecmp("set", action,strlen("set"))) {   //只修改已存在的
        SET_PARAM_RULE_CHECK(opts);
        ret = set_user_passwd(user.username, user.cryptpasswd, user.group);
    } else if (!strncasecmp("add", action,strlen("add"))) {   //增加没有的
        SET_PARAM_RULE_CHECK(opts);
        ret = add_usercfg(user.username, user.cryptpasswd, user.group);

    } else if (!strncasecmp("del", action,strlen("del"))) {
        ret = del_usercfg(user.username);
    }
    sprintf(buf, "result=%d", ret);
    return ret;

}

int JCPCmdAeAwbBlcCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    Video3aS inner = {0};
    Video3aS outer = {0};

    int occupy = 0;
    HelpMsgS helps[] = {
        {"?"              , "快门，白平衡，背光补偿，增益等设置功能"       },
        {"act"            , "list:获取所有参数 set:设置参数"               },
        {"aeCtrlMode"     , "快门控制模式.0 自动曝光(Automatic Exposure)\r\n"
                          "        1:1/10000s  2:1/5000s 3:1/2000s\r\n"
                          "        4:1/1000s  5:1/500s  6:1/250s 7:1/200s\r\n"
                          "        8:1/125s  9:1/100s  10:1/50s 11:1/25s\r\n"},
        {"awbCtrlMode"    , " 自动白平衡(Automatic White Balance\r\n"
                          "        0:自动\r\n"
                          "        1:晴天\r\n"
                          "        2:阴天\r\n"
                          "        3:荧光灯\r\n"
                          "        4:钨丝灯\r\n"
                          "        5:室内\r\n"
                          "        6:室外\r\n"
                          "        7:自定义"},
        {"blcEnable"      , "背光补偿 Black Light Compensation 使能开关：0，禁用；1，使能"},
        {"redGain"        , "红增益"                                                      },
        {"blueGain"       , "蓝增益"                                                      },
        {"lowlightenhance", "低照亮度增强"                                                      },
        {"nightfacemode"  , "夜视人脸模式使能开关：0，禁用；1，使能"                      },
        {"defogenhance"   , "去雾强度：0，禁用；1~100逐渐增强"                            },
        {"End"            , "aeawbblccfg -? 获取帮助"                                     },
    };

    ArgOptS opts[] = {
        {"?"              , ARG_TYPE_ASK, NULL      , NULL                         , 0                            },
        {"act"            , ARG_TYPE_ACT, "list|set", (void*)action                , sizeof(action)               },
        {"aeCtrlMode"     , ArgTypesInt , "0~11"    , (void*)&outer.ae             , sizeof(outer.ae)             },
        {"awbCtrlMode"    , ArgTypesInt , "0~7"     , (void*)&outer.awb            , sizeof(outer.awb)            },
        {"blcEnable"      , ArgTypesInt , "0|1"     , (void*)&outer.blc            , sizeof(outer.blc)            },
        {"redGain"        , ArgTypesInt , "1~255"   , (void*)&outer.redgain        , sizeof(outer.redgain)        },
        {"blueGain"       , ArgTypesInt , "1~255"   , (void*)&outer.bluegain       , sizeof(outer.bluegain)       },
        {"lowlightenhance", ArgTypesInt , "0~100"   , (void*)&outer.lowlightEnhance, sizeof(outer.lowlightEnhance)},
        {"nightfacemode"  , ArgTypesInt , "0~7"     , (void*)&outer.nightfacemode  , sizeof(outer.nightfacemode)  },
        {"defogenhance"   , ArgTypesInt , "0~100"   , (void*)&occupy               , sizeof(occupy)               },
        {"End"            , ArgTypesEnd , NULL      , NULL                         , 0                            },
    };
    ret = get_config(handleVideo3aCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(Video3aS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(Video3aS));
        ret = set_config(handleVideo3aCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}


int JCPCmdViCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int occupy = 0;

    char action[JCP_ACTION_LEN] = {0};

    ViInfoS inner = {0};
    ViInfoS outer = {0};

    HelpMsgS helps[] = {
        {"?"            , "亮度. 对比度. 色度. 饱和度设置"                       },
        {"act"          , "list:获取所有参数 set:设置参数"                       },
        {"nightluma"    , "夜视亮度"                                             },
        {"bright"       , "亮度"                                                 },
        {"contrast"     , "对比度"                                               },
        {"hue"          , "色度"                                                 },
        {"saturation"   , "饱和度"                                               },
        {"sharpness"    , "锐度"                                                 },
        {"lampfrequency", "光源频率，0：60HZ，1：50HZ"                           },
        {"reverse"      , "视频镜像 0: 正常 1: 水平镜像 2: 垂直镜像  3: 对角镜像"},
        {"gain"         , "增益 0~255"                                           },
        {"brightlevel"  , "强度等级"                                             },
        {"suppress"     , "强光抑制"                                             },
        {"stren"        , "宽动态 0~255"                                        },
        {"End"          , "vicfg -? 获取帮助"                                    },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK, NULL      , NULL                       , 0                          },
        {"act"          , ARG_TYPE_ACT, "list|set", (void*)action              , sizeof(action)             },
        {"nightluma"    , ArgTypesInt , "0~255"   , (void*)&outer.nightluma    , sizeof(outer.nightluma)    },
        {"bright"       , ArgTypesInt , "0~255"   , (void*)&outer.bright       , sizeof(outer.bright)       },
        {"contrast"     , ArgTypesInt , "0~255"   , (void*)&outer.contrast     , sizeof(outer.contrast)     },
        {"hue"          , ArgTypesInt , "0~255"   , (void*)&outer.hue          , sizeof(outer.hue)          },
        {"saturation"   , ArgTypesInt , "0~255"   , (void*)&outer.saturation   , sizeof(outer.saturation)   },
        {"sharpness"    , ArgTypesInt , "0~255"   , (void*)&outer.sharpness    , sizeof(outer.sharpness)    },
        {"lampfrequency", ArgTypesInt , "0|1"     , (void*)&outer.lampfrequency, sizeof(outer.lampfrequency)},
        {"reverse"      , ArgTypesInt , "0~3"     , (void*)&outer.reverse      , sizeof(outer.reverse)      },
        {"gain"         , ArgTypesInt , "0~255"   , (void*)&outer.gain         , sizeof(outer.gain)         },
        {"brightlevel"  , ArgTypesInt , "0~6"     , (void*)&outer.brightlevel  , sizeof(outer.brightlevel)  },
        {"suppress"     , ArgTypesInt , "0~100"   , (void*)&outer.suppress     , sizeof(outer.suppress)     },
        {"stren"        , ArgTypesInt , "0~255"   , (void*)&occupy             , sizeof(occupy)             },
        {"End"          , ArgTypesEnd , NULL      , NULL                       , 0                          },
    };

    ret = get_config(handleViinfoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(ViInfoS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(ViInfoS));
        ret = set_config(handleViinfoCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

const char *fps_scope(void)
{
    static char buf[5] = {0};
    int max_fps = get_enc_max_fps();
    sprintf(buf, "0~%d", max_fps);
    return buf;
}

int JCPCmdDevveCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    VideoEncS inner = {0};
    VideoEncS outer = {0};
    VideoEnc0 *z = NULL;

    HelpMsgS helps[] = {
        {"?"     , "视频参数设置"},
        {"act"   , "list:获取所有参数 set:设置参数"   },
        {"gnum"  , "支持的视频通道数"                 },
        {"id"    , "通道ID, 0 主码流  1 次码流   2 mjpg"},
        {"enable", "该通道是否时能，1: 启用  0:不启用"},
        {"codec" , "压缩算法，2 h264; 5 mjpg; 7 h265" },
        {"vencsize", "录像的视频大小，必须是现有的视频通道中的一个。VENC_SIZE_E\r\n"
            "         VencSizeE_QCIF = 0,      // QVGA\r\n"
            "         VencSizeE_CIF = 1,       // CIF\r\n"
            "         VencSizeE_D1 = 2,        // D1\r\n"
            "         VencSizeE_720P = 3,      // 720P\r\n"
            "         VencSizeE_UVGA = 4,      // UVGA \r\n"
            "         VencSizeE_1080P = 5,     // 1080P\n"
            "         VencSizeE_QVGA = 6,      // QVGA\r\n"
            "         VencSizeE_VGA = 7,       // VGA\r\n"
            "         VencSizeE_960P = 8,      // 960P\r\n"
            "         VencSizeE_3M = 9,        // 3M\r\n"
            "         VencSizeE_180P = 10,     // 180P\r\n"
            "         VencSizeE_360P = 11,     // 360P\r\n"
            "         VencSizeE_4M = 12,     // 4M\r\n"
            "         VencSizeE_5M = 13,     // 5M\r\n"
            "         VencSizeE_4M_Dahua = 14// 4M_Dahua"
            "         VencSizeE_8M = 15,     // 8M"
            "         VencSizeE_6M = 16,     // 6M\r\n"},
        {"standard", "制式，0,P制; 1, N制。（现都高清，只支持0）"},
        {"fps"     , "帧率"                   },
        {"bps"     , "码率"                   },
        {"gop"     , "I帧间隔"                },
        {"fixfps"  , "1 质量优先，0 速度优先" },
        {"fixbps"  , "1 定码流，0 变码流"     },
        {"End"     , "devvecfg -?获取帮助"},
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK        , NULL            , NULL        , 0             },
        {"act"     , ARG_TYPE_ACT        , "list|set"      , action      , sizeof(action)},
        {"gnum"    , ARG_TYPE_LISTI      , "0~4"           , &outer.gnum , sizeof(int)   },
        {"id"      , ARG_TYPE_LIST_ID    , "0~2"           , &z->id      , sizeof(int)   },
        {"enable"  , ARG_TYPE_LIST_MEMINT, "0~1"           , &z->enable  , sizeof(int)   },
        {"codec"   , ARG_TYPE_LIST_MEMINT, "2|5|7"         , &z->codec   , sizeof(int)   },
        {"vencsize", ARG_TYPE_LIST_MEMINT, "0|1|2|3|5|6|7|8|9|11|12|13|14|15|16" , &z->vencsize, sizeof(int)   },
        {"standard", ARG_TYPE_LIST_MEMINT, "0~1"           , &z->standard, sizeof(int)   },
        {"fps"     , ARG_TYPE_LIST_MEMINT, "1~30"          , &z->fps     , sizeof(int)   },
        {"bps"     , ARG_TYPE_LIST_MEMINT, "32~4096"       , &z->bps     , sizeof(int)   },
        {"gop"     , ARG_TYPE_LIST_MEMINT, "1~300"          , &z->gop     , sizeof(int)   },
        {"fixfps"  , ARG_TYPE_LIST_MEMINT, "0|1"           , &z->fixfps, sizeof(int)   },
        {"fixbps"  , ARG_TYPE_LIST_MEMINT, "0~4"           , &z->fixbps  , sizeof(int)   },
        {"End"     , ArgTypesEnd         , NULL            , NULL        , 0             },
    };

    ret = get_config(handleVideoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VideoEncS));

    JCP_ARG_PARSER(outer.enc, sizeof(VideoEnc0));
    Appvecfg vec = {0};
    conf_get_appve_cfg(&vec);
    if (vec.webxvsz == VencSizeE_5M && inner.enc[0].vencsize == VencSizeE_3M) {
        inner.enc[0].vencsize = VencSizeE_5M;
    }
    else if(vec.webxvsz == VencSizeE_4M && inner.enc[0].vencsize == VencSizeE_3M) {
        inner.enc[0].vencsize = VencSizeE_4M;
    }

    outer.enc[0].fps = get_valid_fps(outer.enc[0].fps);
    outer.enc[1].fps = get_valid_fps(outer.enc[1].fps);

    outer.enc[0].bps = RANGE(outer.enc[0].bps, 256, 8192);
    outer.enc[1].bps = RANGE(outer.enc[1].bps, 32, 2048);

    outer.enc[0].gop = MIN(360, outer.enc[0].gop);
    outer.enc[1].gop = MIN(360, outer.enc[1].gop);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        ASMJCP_LIST_COUNT(inner.enc, sizeof(VideoEnc0), ENCODE_MAX_CHN);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VideoEncS));
        ret = set_config(handleVideoCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

int JCPCmdDevaudioOpt(char *buf, int buflen, int argc, char **argv)
{
    AudioCfgS inner = {0};
    int ret = get_config(handleAudioCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    const char *fmt_devaudioopt =
    "{"
        "'audioin':["
            "{'def':%d, 'min':1,  'max':100}"
        "],"
        "'audioout':["
            "{'def':%d, 'min':1,  'max':100}"
        "]"
    "}";

    char devaudioopt[512] = {0};
    sprintf(devaudioopt, fmt_devaudioopt, inner.definvolume, inner.defoutvolume);

    // 将' -> \"
    char *p = buf;
    size_t i;
    for (i = 0; i < strlen(devaudioopt); i++) {
        if (devaudioopt[i] == '\'') {
            *(p++) = '\\';
            *(p++) = '"';
        } else {
            *(p++) = devaudioopt[i];
        }
    }

    *p = '\0';

    return SUCCESS;
}

int JCPCmdDevveOpt(char *buf, int buflen, int argc, char **argv)
{
    // 1. [] {} 后不能有多余的 ,
    // 2. 添加新的opt后，在 chrome 中进行验证
    //    F12 -> Console -> JSON.parse("")
   const char *devveopt_5M_16_9 =
            "{'id':13, 'name':'5M',"
                            "'bps_def':3840, 'bps_min':256, 'bps_max':8192,"
                            "'fps_def':15, 'fps_min':5, 'fps_max':20,"
                            "'gop_def':45, 'gop_min':10, 'gop_max':300}"
                            ","
                            ;

   const char *devveopt_4M =
        "{'id':12, 'name':'4M',"
                            "'bps_def':1200, 'bps_min':256, 'bps_max':3072,"
                            "'fps_def':15, 'fps_min':5, 'fps_max':20,"
                            "'gop_def':45, 'gop_min':10, 'gop_max':300}"
                            ","
                            ;

    const char *devveopt_default =
            "{'id':9, 'name':'3M',"
                                "'bps_def':1024, 'bps_min':256, 'bps_max':2048,"
                                "'fps_def':%d, 'fps_min':5, 'fps_max':%d,"
                                "'gop_def':%d,   'gop_min':10,  'gop_max':300}"
                                ","
            "{'id':5, 'name':'1080P',"
                              "'bps_def':1024, 'bps_min':256, 'bps_max':2048,"
                                "'fps_def':%d, 'fps_min':5, 'fps_max':%d,"
                                "'gop_def':%d,   'gop_min':10,  'gop_max':300}"
                                ","
            "{'id':3, 'name':'720P',"
                              "'bps_def':1024, 'bps_min':256, 'bps_max':2048,"
                                "'fps_def':%d, 'fps_min':5, 'fps_max':%d,"
                                "'gop_def':%d, 'gop_min':10, 'gop_max':300}"
                                ;

    const char *devveopt_fmt =
    "{"
        "'x264':'1.25',"
        "'master':["
            "%s"                // devveopt_8M or EMPTY-string
            "%s"                // devveopt_6M or EMPTY-string
            "%s"                // devveopt_5M or EMPTY-string
            "%s"                // devveopt_4M
            "%s"                // less 3M
        "],"
        "'slave':["
            "{'id':11, 'name':'360P',"
                                "'bps_def':384, 'bps_min':64, 'bps_max':1024,"
                                "'fps_def':15,  'fps_min':5,  'fps_max':15,"
                                "'gop_def':45,  'gop_min':10,  'gop_max':300}"
        "]"
    "}";

    char default_arr[1024] = {0};
    int def_fps = 15;
    int max_fps = get_enc_max_fps();
    sprintf(default_arr, devveopt_default,
            def_fps, max_fps, (int)(max_fps * 2.25) ,
            def_fps, max_fps, (int)(max_fps * 2.25) ,
            def_fps, max_fps, (int)(max_fps * 2.25));

    char json[2048] = {0};
    Appvecfg vec = {0};
    conf_get_appve_cfg(&vec);

    switch (encode_max_web_idx(0)) {
    case VideoIdxE_5M:
        snprintf(json, sizeof(json)-1, devveopt_fmt,  "", "", devveopt_5M_16_9, devveopt_4M, default_arr);
        break;
    case VideoIdxE_4M:
        snprintf(json, sizeof(json)-1, devveopt_fmt, "", "", "", devveopt_4M, default_arr);
        break;
    case VideoIdxE_3M_16_9:
        if (vec.webxvsz== VencSizeE_4M) {
            snprintf(json, sizeof(json)-1, devveopt_fmt, "", "", devveopt_4M, default_arr);
        } else {
            snprintf(json, sizeof(json)-1, devveopt_fmt, "", "", "", "", default_arr);
        }
        break;
    default:
        snprintf(json, sizeof(json)-1, devveopt_fmt, "", "", "", "", default_arr);
        break;
    }

    // 将' -> \"
    char *p = buf;
    size_t i;
    for (i = 0; i < strlen(json); i++) {

            if (json[i] == '\'') {
            *(p++) = '\\';
            *(p++) = '"';
            } else {
                *(p++) = json[i];
            }

    }
    *p = '\0';

    printf("This is buf %s\n", buf);

    return SUCCESS;
}

static int JCPCmdResoCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int nr = 0;
    char action[JCP_ACTION_LEN] = {0};

    VideoEncS inner = {0};
    VideoEncS outer = {0};

    HelpMsgS helps[] = {
        {"?"  , "视频参数设置"                  },
        {"act", "list:获取所有参数 set:设置参数"},
        {"nr" , "级别 0|50|100"                 },
        {"End"     , "devvecfg -?获取帮助"},
    };

    ArgOptS opts[] =
    {
        {"?"  , ARG_TYPE_ASK, NULL      , NULL  , 0             },
        {"act", ARG_TYPE_ACT, "list|set", action, sizeof(action)},
        {"nr" , ArgTypesInt , "0~100"   , &nr   , sizeof(int)   },
        {"End", ArgTypesEnd , NULL      , NULL  , 0             },
    };

    ret = get_config(handleVideoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VideoEncS));

    JCP_ARG_PARSER(outer.enc, sizeof(VideoEnc0));

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        int res = 0;
        int fps = 0;
        int bps = 0;

        SET_PARAM_RULE_CHECK(opts);
        outer.enc[0].fps = fps;
        outer.enc[0].bps = bps;
        outer.enc[0].vencsize = (VencSizeE)res;
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VideoEncS));
        ret = set_config(handleVideoCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

int JCPCmdVideoMaskCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    
    char action[JCP_ACTION_LEN] = {0};

    VideoMaskS inner = {0};
    VideoMaskS outer = {0};
    VideoMask0 *z = NULL;
    
    HelpMsgS helps[] = {
        {"?"     , "视频遮挡块设置，左上角坐标为(left , top)，右下角坐标为(right, bottom)"},
        {"act"   , "list:获取所有参数 set:设置参数"                             },
        {"gnum"  , "遮挡块总数"                                                 },
        {"maskid", "在一组数据的最前面，指定组id"                               },
        {"masken", "是否启用: 1，启用；0，禁止"                                 },
        {"color" , "颜色索引，8种灰色，6种彩色,以第一块遮挡块颜色为准，其他块颜色设置无效\r\n"
                   "      0：纯黑  1：灰黑  ~ 7：灰白   8：红色  9：绿色  9：绿色 \r\n"
                   "      10：蓝色  11：淡蓝色   12：黄色   13：紫红色"},
        {"left"  , "遮挡块左上x坐标"                                            },
        {"top"   , "遮挡块左上y坐标"                                            },
        {"right" , "遮挡块右下x坐标"                                            },
        {"bottom", "遮挡块右下y坐 标"                                           },
        {"End"   , "videomaskcfg -?获取帮助"                                    },
    };

    ArgOptS opts[] =
    {
        {"?"     , ARG_TYPE_ASK        , NULL      , NULL       , 0             },
        {"act"   , ARG_TYPE_ACT        , "list|set", action     , sizeof(action)},
        {"gnum"  , ARG_TYPE_LISTI      , "8"       , &outer.gnum, sizeof(int)   },
        {"maskid", ARG_TYPE_LIST_ID    , "0~7"     , &z->id     , sizeof(int)   },
        {"masken", ARG_TYPE_LIST_MEMINT, "0|1"     , &z->enable , sizeof(int)   },
        {"color" , ArgTypesInt         , "0~13"    , &outer.mask[0].color, sizeof(int)   },
        {"left"  , ARG_TYPE_LIST_MEMINT, "0~1919"  , &z->x0     , sizeof(int)   },
        {"top"   , ARG_TYPE_LIST_MEMINT, "0~1079"  , &z->y0     , sizeof(int)   },
        {"right" , ARG_TYPE_LIST_MEMINT, "0~1919"  , &z->x1     , sizeof(int)   },
        {"bottom", ARG_TYPE_LIST_MEMINT, "0~1079"  , &z->y1     , sizeof(int)   },
        {"End"   , ArgTypesEnd         , NULL      , NULL       , 0             },
    };
    
    ret = conf_get_videomaskcfg(&inner);
    RETURN_FAIL_IF_API_ERR(ret);
    
    memcpy(&outer, &inner, sizeof(VideoMaskS));

    JCP_ARG_PARSER(outer.mask, sizeof(VideoMask0));

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        ASMJCP_LIST_COUNT(inner.mask, sizeof(VideoMask0), VIDEO_MASK_MAX_CHN);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VideoMaskS));
        ret = conf_set_videomaskcfg(outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }   

    return SUCCESS;
}

int JCPCmdVideoMaskPlan(char *buf, int buflen, int argc, char **argv)
{
    int ret = 0;
    char action[JCP_ACTION_LEN+8] = {0};

    videomask_plan_t inner = {0,};
    videomask_plan_t outer = {0,};

    HelpMsgS helps[] = {
        {"?"              , "时间设置"                          },
        {"act"            , "list: 获取时间列表  set : 设置时间"},
        {"week"           , "7位数字串 第1位代表周日"           },
        {"enable"         , "表示当前时间布防有没有开启"        },
        {"mask_enable"    , "隐私遮挡开关"                      },
        {"cover_direction", "隐私遮挡方向，0 上，1 下"          },
        {"beginhour"      , "自定义布防起始时间"                },
        {"beginmin"       , "自定义布防起始时间"                },
        {"endhour"        , "自定义布防结束时间"                },
        {"endmin"         , "自定义布防结束时间"                },
        {"End"            , "videomaskplan -? 获取帮助"         },
    };

    ArgOptS opts[] = {
        {"?"              , ARG_TYPE_ASK   , NULL      , NULL                  , 0                 },
        {"act"            , ARG_TYPE_ACT   , "list|set", (void*)action         , sizeof(action)    },
        {"week"           , ArgTypesString , NULL      , outer.week            , sizeof(outer.week)},
        {"enable"         , ArgTypesInt    , NULL      , &outer.enable         , sizeof(int)       },
        {"mask_enable"    , ArgTypesInt    , NULL      , &outer.mask_enable    , sizeof(int)       },
        {"cover_direction", ArgTypesInt    , NULL      , &outer.cover_direction, sizeof(int)       },
        {"beginhour"      , ArgTypesInt    , NULL      , &outer.beginhour      , sizeof(int)       },
        {"beginmin"       , ArgTypesInt    , NULL      , &outer.beginmin       , sizeof(int)       },
        {"endhour"        , ArgTypesInt    , NULL      , &outer.endhour        , sizeof(int)       },
        {"endmin"         , ArgTypesInt    , NULL      , &outer.endmin         , sizeof(int)       },
        {"End"            , ArgTypesEnd    , NULL      , NULL                  , 0                 },
    };

    ret = conf_get_videomaskplan_cfg(&inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(videomask_plan_t));
    JCP_ARG_PARSER(NULL,0);

    if(!strncasecmp("list", action, strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(videomask_plan_t));

        ret = conf_set_videomaskplan_cfg(outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

int JCPCmdAppOpt(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int location_4g = 1;
    char action[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"  , "视频参数设置"              },
        {"act", "list:获取所有参数"         },
        {"End", "devappopt -?获取帮助" },
    };

    ArgOptS opts[] =
    {
        {"?"  , ARG_TYPE_ASK, NULL      , NULL  , 0             },
        {"act", ARG_TYPE_ACT, "list"    , action, sizeof(action)},
        {"End", ArgTypesEnd , NULL      , NULL  , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);
    Sim4g sim4g_info = {0,};
    ret = sim4g_get_stat(&sim4g_info);
    location_4g = sim4g_info.location_enable;

    SysInfoS inner = {{0,},};
    ret = get_config(handleSysInfoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    SysCustomS sys_customs = {0};
    ret = get_config(handleCapability, sys_customs);
    RETURN_FAIL_IF_API_ERR(ret);

    AudioAlarmS audioalarm = {0};
    ret = get_config(handleAudioAlarmCfg, audioalarm);
    RETURN_FAIL_IF_API_ERR(ret);

    IOAlarmS ioalarm = {0};
    ret = get_config(handleIOAlarmCfg, ioalarm);
    RETURN_FAIL_IF_API_ERR(ret);

    DriveOut driveout = {0};
    ret = get_config(handleDriveOutCfg, driveout);
    RETURN_FAIL_IF_API_ERR(ret);

    LightExtCfg lightext = {0};
    ret = get_config(handleLightExtCfg, lightext);
    RETURN_FAIL_IF_API_ERR(ret);

    sVideoCallCfg videocall = {0};
    conf_get_videocall_cfg(&videocall);
    RETURN_FAIL_IF_API_ERR(ret);

    int width = 1920, height = 1080;
    Appvecfg vec = {0};
    conf_get_appve_cfg(&vec);
    switch (vec.appxvsz) {
        case VencSizeE_4M:
            encode_vencsize_to_resolution(VencSizeE_4M, &width, &height);
            break;
        case VencSizeE_5M:
            encode_vencsize_to_resolution(VencSizeE_5M, &width, &height);
            break;
        case VencSizeE_8M:
            encode_vencsize_to_resolution(VencSizeE_8M, &width, &height);
            break;
        default:
            encode_idx_to_resolution(encode_max_idx(0), &width, &height);
            break;
    }

    // ipc 模组, apc WiFi/4G球, lpc 低功耗球, qpc 枪球, lqpc 低功耗枪球, spc 摇头机
    char product_conf[8] = {"apc"};
    int wifi_info = (get_g_sys(usb_wifi)==1) ? 1 : 0;

    int sim4g_x2  = 2; //(get_g_sys(usb_4g)==1) ? 1 : 0;
    /* 0：不支持
     * 1：电信、移动
     * 2：电信、联通
     * 3：移动、联通
     */
    if ((sim4g_info.ecard_type == E_TELECOM && 
        sim4g_info.card_type == E_MOBILE) ||
        (sim4g_info.ecard_type == E_MOBILE && 
        sim4g_info.card_type == E_TELECOM)) {
        sim4g_x2 = 1;
    } else if ((sim4g_info.ecard_type == E_TELECOM && 
        sim4g_info.card_type == E_UNICOM) ||
        (sim4g_info.ecard_type == E_UNICOM && 
        sim4g_info.card_type == E_TELECOM)) {
        sim4g_x2 = 2;
    } else if ((sim4g_info.ecard_type == E_MOBILE && 
        sim4g_info.card_type == E_UNICOM) ||
        (sim4g_info.ecard_type == E_UNICOM && 
        sim4g_info.card_type == E_MOBILE)) {
        sim4g_x2 = 3;
    }

    int eth_disable = (get_g_sys(eth) == 0) ? 1 : 0;
    int zoom_optic = 0;
    int zoom_digit = 1;
    int ir = 1, white = 1, lightmode_opt = 1;
    // 灯板，0:红外灯板 1:白光灯板 2:双光灯板
    if (1 == lightext.lightboard) {
        ir = 0;
        white = 1;
        lightmode_opt = 0;
    } else if (2 == lightext.lightboard) {
        ir = 1;
        white = 1;
        lightmode_opt = 1;
    } else{
        ir = 1;
        white = 0;
        lightmode_opt = 0;
    }

    // app能力集: https://alidocs.dingtalk.com/i/nodes/ZX6GRezwJlgBYggyUNaaB47jVdqbropQ
    struct optval {
        const char *fmt;
        const void *p_val;
        const char type;    // d or not d
    } maps[] = {
        {"product=%s;"              , product_conf          , 's'}, // 设备类型
        {"version=%s;"              , inner.serverver       , 's'}, // 版本号
        {"ota=1;"                   , NULL                  , 'd'}, // OTA  是否支持模组类设备自研OTA
        {"lang=0;"                  , NULL                  , 'd'}, // 语言 0 中文, 1 英文
        {"sysinfo=TW35:0*0:%d"      , &width                , 'd'}, // app截图 w
        {"*%d;"                     , &height               , 'd'}, // app截图 h
        {"location_4g=%d;"          , &location_4g          , 'd'}, // 4g定位
        {"switchcard=%d;"           , &sim4g_x2             , 'd'}, // 4G网络切换
        {"mic=1;"                   , NULL                  , 'd'}, // 咪头
        {"speaker=1;"               , NULL                  , 'd'}, // 扬声器
        {"duplex=1;"                , NULL                  , 'd'}, // 对讲
        {"cust_audio=1;"            , NULL                  , 'd'}, // 自定义语音
        {"sd=1;"                    , NULL                  , 'd'}, // 是否支持 SD 卡
        {"sd_interrupt=1;"          , NULL                  , 'd'}, // 报警录像
        {"rec_type=1;"              , NULL                  , 'd'}, // 录像质量设置
        {"sdinfo=%d;"               , &sys_customs.sdinfo   , 'd'}, // 系统产测工具定制 sys_customs.sdinfo
        {"multiple=1;"              , NULL                  , 'd'}, // 是否支持录像倍速回放
        {"rec_datelist=1;"          , NULL                  , 'd'}, // 是否支持查询当月录像日期
        {"md=1;"                    , NULL                  , 'd'}, // 移动侦测
        {"hd=1;"                    , NULL                  , 'd'}, // 人形侦测
        {"car=0;"                   , NULL                  , 'd'}, // 人车: 0-不显示车辆侦测 1-显示车辆侦测 2-人车共检
        {"smoke_flame=0;"           , NULL                  , 'd'}, // 烟火识别
        {"vgline=0;"                , NULL                  , 'd'}, // 越界侦测
        {"vgrect=0;"                , NULL                  , 'd'}, // 区域侦测
        {"flame=0;"                 , NULL                  , 'd'}, // 火焰报警
        {"mutexmode=6;"             , NULL                  , 'd'}, // 报警互斥模式:4是移动和人形互斥，越界和区域互斥；5是移动和人形互斥，越界、人形、区域三选一；6或不存在是四选一
        {"follow=%d;"               , &sys_customs.follow   , 'd'}, // 人形追踪
        {"passenger=0;"             , NULL                  , 'd'}, // 客流统计
        {"throw=0;"                 , NULL                  , 'd'}, // 高空抛物
        {"ebike=0;"                 , NULL                  , 'd'}, // 电动车
        {"lightalarm=1;"            , NULL                  , 'd'}, // 灯光报警
        {"audioalarm=%d;"           , &audioalarm.show      , 'd'}, // 声音报警
        {"ioalarm=%d;"              , &ioalarm.show         , 'd'}, // IO报警/红蓝灯报警
        {"pir=0;"                   , NULL                  , 'd'}, // pir报警
        {"driveout=%d;"             , &driveout.show        , 'd'}, // 驱赶设置
        {"videocall=%d;"            , &videocall.show       , 'd'}, // 一键呼叫开关
        {"twecall=%d;"              , &videocall.show       , 'd'}, // 微信通话开关
        {"ir=%d;"                   , &ir                   , 'd'}, // 红外灯
        {"white=%d;"                , &white                , 'd'}, // 白光灯
        {"var=0;"                   , NULL                  , 'd'}, // PWM 变光
#ifdef __TW36__
        {"truestar=1;"              , NULL                  , 'd'}, // 黑光臻全彩APP新界面
#else
        {"truestar=0;"              , NULL                  , 'd'}, // 黑光臻全彩APP新界面
#endif
        {"lightgrade=0;"            , NULL                  , 'd'}, // 灯光调档
        {"newlightctrl=0;"          , NULL                  , 'd'}, // 新版补灯模式
        {"lampselect=1;"            , NULL                  , 'd'}, // 0 老版双光源, 1 新版双光源
        {"status_led=0;"            , NULL                  , 'd'}, // 指示灯状态
        {"zoom=1;"                  , NULL                  , 'd'}, // 数字变倍
        {"zoom_en=%d;"              , &zoom_optic           , 'd'}, // 数字变倍开关
        {"focus=%d;"                , &zoom_optic           , 'd'}, // 支持变焦
        {"demist=0;"                , NULL                  , 'd'}, // 除雾
        {"linkage=0;"               , NULL                  , 'd'}, // 枪球联动
        {"linkage_calibration=0;"   , NULL                  , 'd'}, // 枪球联动
        {"ptz_speed=55;"            , NULL                  , 'd'}, // 云台默认旋转速度
        {"ptz_timestamp=1;"         , NULL                  , 'd'}, // 云台运动命令带时间戳
        {"ptz=%d;"                  , &sys_customs.ptz      , 'd'}, // 云台功能
        {"seq=1;"                   , NULL                  , 'd'}, // 巡航功能
        {"preset=1;"                , NULL                  , 'd'}, // 预置位功能
        {"check=%d;"                , &sys_customs.follow   , 'd'}, // 云台校准
        {"panoramic=1;"             , NULL                  , 'd'}, // 全景扫描
        {"ap=%d;"                   , &wifi_info            , 'd'}, // 热点连接
        {"sta=%d;"                  , &wifi_info            , 'd'}, // WiFi 无线连接
        {"osd=%d;"                  , &sys_customs.osd      , 'd'}, // 0-不支持, 1-通用osd, 2-新版osd
        {"videocode=1;"             , NULL                  , 'd'}, // 视频编码
        {"iris=0;"                  , NULL                  , 'd'}, // 光圈调节
        {"sensetype=2;"             , NULL                  , 'd'}, // 软光敏
        {"ethctr=%d;"               , &eth_disable          , 'd'}, // 网口 1 禁用
        {"zoom_follow=%d;"          , &zoom_digit           , 'd'}, // 支持跟踪变倍开关
        {"person_center=1;"         , NULL                  , 'd'}, // 人形居中
        {"faceae=1;"                , NULL                  , 'd'}, // 人脸收光
        {"lightmode_opt=%d;"        , &lightmode_opt        , 'd'}, // 灯光模式-星光夜视
        {"petdet=1;"                , NULL                  , 'd'}, // 宠物识别
        {"crydet=1;"                , NULL                  , 'd'}, // 哭声识别
        {"privacy_masking=1;"       , NULL                  , 'd'}, // 隐私遮挡
        {"videomask_alarm=1;"       , NULL                  , 'd'}, // 视频遮挡
        {"lighting=1;"              , NULL                  , 'd'}, // 小夜灯
    };

    /*整理后未归档配置：
        enable=1;
        sub_product=%s; ((get_g_sys(usb_asix) || get_g_sys(usb_4g))?"4g":"apc")
        view_angle=0;  视频相关
        rec_h2301=1;   录像相关
        audiocustom=1;
        algo=0;  algo=1 软光敏
        index_led=0;
        face=1;
        rec_alarm=1;   rec_alarm 标识支持recflag字段标识当天告警信息
        mcu=0;  // 0-不显示电量 1-显示电量
#ifdef CUSTOMER_HBSD
            "osdcode=%d;"  sys_customs.osdcode// 0-不显示OSD和编码参数 1-显示OSD和编码参数
#endif
#if defined(CUST_BLE)
            "ble=1;"        //蓝牙
#endif
    chksim=%d;    ((get_g_sys(usb_asix) || get_g_sys(usb_4g))?1:0)
    */

    if(!strcmp(action, "list")){
        char *p = buf;
        for (int i = 0; i < ARRAY_SIZE(maps); i++) {
            if (NULL == maps[i].p_val) {
                p += sprintf(p, maps[i].fmt);
            } else {
                if ('d' == maps[i].type) {
                    p += sprintf(p, maps[i].fmt, *(int *)(maps[i].p_val) );
                } else {
                    p += sprintf(p, maps[i].fmt, (char *)maps[i].p_val);
                }
            }
        }
        DBG("%s dev_info = %s%s\n", "\033[1;32m", buf, "\033[0m");
    }

    return SUCCESS;
}

int JCPCmdCA2PtureCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    CaptureS inner = {0,};
    CaptureS outer = {0,};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"           , "定时和手动抓拍功能设置"                                     },
        {"act"         , "list:获取所有参数 set:设置参数 manual 触发手动抓拍"},
        {"vesize"      , "抓拍图片尺寸\r\n"
                         "     VencSizeE_QCIF = 0,      // QVGA\r\n"
                         "     VencSizeE_CIF = 1,       // CIF\r\n"
                         "     VencSizeE_D1 = 2,        // D1\r\n"
                         "     VencSizeE_720P = 3,      // 720P\r\n"
                         "     VencSizeE_UVGA = 4,      // UVGA \r\n"
                         "     VencSizeE_1080P = 5,     // 1080P\n"
                         "     VencSizeE_QVGA = 6,      // QVGA\r\n"
                         "     VencSizeE_VGA = 7,       // VGA\r\n"
                         "     VencSizeE_960P = 8,      // 960P\r\n"
                         "     VencSizeE_3M = 9,        // 3M\r\n"
                         "     VencSizeE_180P = 10,     // 180P\r\n"
                         "     VencSizeE_360P = 11,     // 360P"
                         "     VencSizeE_4M = 12,     // 4M\r\n"
                         "     VencSizeE_5M = 13,     // 5M"},
        {"interv"      , "抓拍时间间隔，1~600秒，功能已弃用"},
        {"alarmnum"    , "报警抓拍张数"                                     },
        {"alarminterv" , "报警抓拍间隔"                                     },
        {"timestrategy", "布防时间描述\r\n"
         "        时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
         "        低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
         "        如全时段禁用：0,0,0,0,0,0,0\r\n"
         "        365中的格式是二元组的形式\r\n"
         "        timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"},
        {"End" , "capturecfg -? 获取帮助"},
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK  , NULL            , NULL              , 0                         },
        {"act"         , ARG_TYPE_ACT  , "list|set|manual", action            , sizeof(action)            },
        {"vesize"      , ArgTypesInt   , "0~16"           , &outer.vesize     , sizeof(int)               },
        {"interv"      , ArgTypesInt   , NULL            , &outer.interv     , sizeof(int)               },
        {"alarmnum"    , ArgTypesInt   , NULL            , &outer.alarmnum, sizeof(int)               },
        {"alarminterv" , ArgTypesInt   , NULL            , &outer.alarminterv, sizeof(int)               },
        {"timestrategy", ArgTypesString, NULL            , timestrategy      , sizeof(timestrategy)},
        {"End"         , ArgTypesEnd   , NULL            , NULL              , 0                         },
    };

    ret = get_config(handleCaptureCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(CaptureS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.timestrategy);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.timestrategy);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(CaptureS));
        ret = set_config(handleCaptureCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    } else if (!strncasecmp("manual", action,strlen("manual"))) {
        //抓拍
#if defined(__CAPTURE__)
        record_request_manual_capture(0, JALARM_TYPE_BEGIN);
#endif
    }

    return SUCCESS;
}

int JCPCmdCA2Ipconflict(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    IpLinkS inner = {0};
    IpLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "IP冲突联动设置"              },
        {"act"      , "list:获取所有参数 set:设置参数"},
        {"interval" , "报警间隔 3s到36000s(10小时)"   },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"   },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"   },
        {"recorden" , "是否录像  1: 是  0 : 否"       },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"   },
        {"captureen", "是否抓拍图片  1: 是  0 : 否"   },
        {"End"      , "ca2ipconflict -? 获取帮助"     },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                   , 0             },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action          , sizeof(action)},
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval , sizeof(int)   },
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.ao0en    , sizeof(int)   },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.ao1en    , sizeof(int)   },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.recorden , sizeof(int)   },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sounden  , sizeof(int)   },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.captureen, sizeof(int)   },
        {"End"      , ArgTypesEnd , NULL      , NULL                   , 0             },
    };
    ret = get_config(handleIpConflictCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(IpLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(IpLinkS));
        ret = set_config(handleIpConflictCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

int JCPCmdCA2Linkbroken(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    IpLinkS inner = {0};
    IpLinkS outer = {0};
    HelpMsgS helps[] = {
        {"?"        , "网口断开联动设置"              },
        {"act"      , "list:获取所有参数 set:设置参数"},
        {"interval" , "报警间隔 3s到36000s(10小时)"   },
        {"ao0en"    , "是否报警输出1 1: 是  0 : 否"   },
        {"ao1en"    , "是否报警输出2 1: 是  0 : 否"   },
        {"recorden" , "是否录像  1: 是  0 : 否"       },
        {"sounden"  , "是否发出声音  1: 是  0 : 否"   },
        {"captureen", "是否发抓拍图片  1: 是  0 : 否"   },
        {"End"      , "ca2linkbroken -? 获取帮助"     },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                   , 0             },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action          , sizeof(action)},
        {"interval" , ArgTypesInt , "3~36000" , (void*)&outer.interval , sizeof(int)   },
        {"ao0en"    , ArgTypesInt , "0|1"     , (void*)&outer.ao0en    , sizeof(int)   },
        {"ao1en"    , ArgTypesInt , "0|1"     , (void*)&outer.ao1en    , sizeof(int)   },
        {"recorden" , ArgTypesInt , "0|1"     , (void*)&outer.recorden , sizeof(int)   },
        {"sounden"  , ArgTypesInt , "0|1"     , (void*)&outer.sounden  , sizeof(int)   },
        {"captureen", ArgTypesInt , "0|1"     , (void*)&outer.captureen, sizeof(int)   },
        {"End"      , ArgTypesEnd , NULL      , NULL                   , 0             },
    };

    ret = get_config(handleIpBrokenCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(IpLinkS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(IpLinkS));
        ret = set_config(handleIpBrokenCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}

#define TIME_START {struct timeval start, end; gettimeofday(&start, NULL);
#define TIME_END gettimeofday(&end, NULL);\
        float timeuse = 1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec; \
        timeuse /= 1000; \
        printf("[%s:%d]%f ms\n", __FUNCTION__, __LINE__, \
        timeuse);}

int JCPCmdGetAlarmEvent(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    char alarmbuf[4*1024] = {0};

    QueryInfoS info = {0};

    HelpMsgS helps[] = {
        {"?"    , "获取alarm信息，点击WEB视频页面工具框中黄灯可以得到报警列表"},
        {"act"  , "list:获取所有参数"  },
        {"type" , "报警类型 (0~9) \r\n "
            "        JALARM_TYPE_BEGIN = 0  返回所有类型\r\n"
            "        JALARM_TYPE_MD = 1  移动侦测\r\n"
            "        JALARM_TYPE_VGLINE = 2 VGLINE\r\n"
            "        JALARM_TYPE_VGRECT = 3 VGRECT\r\n"
            "        JALARM_TYPE_VL = 4  视频丢失\r\n"
            "        JALARM_TYPE_DISK_FULL = 5  磁盘满\r\n"
            "        JALARM_TYPE_DISK_ERR = 6  磁盘错误\r\n"
            "        JALARM_TYPE_CABLE_DISC = 7   网线被拔出\r\n"
            "        JALARM_TYPE_IP_CONFLICT = 8  IP冲突\r\n"
            "        JALARM_TYPE_ILLEGAL_ACCESS = 9  违法访问\r\n"
            "        JALARM_TYPE_AI = 10  报警输入\r\n"
            "        JALARM_TYPE_EXP = 11 拓展报警\r\n"
            "        JALARM_TYPE_MASK = 12 遮挡报警\r\n"
            "        JALARM_TYPE_HUMAN_DETECT = 13 人形侦测\r\n"
            "        JALARM_TYPE_CAR = 14 车型侦测\r\n"
            "        JALARM_TYPE_PLATE = 15 车牌侦测\r\n"
            "        JALARM_TYPE_PET = 16 人形侦测\r\n"
            "        JALARM_TYPE_HUMAN_MIX = 17 人形混合侦测\r\n"
            "        JALARM_TYPE_EBIKE = 18 电动车侦测\r\n"
            "        JALARM_TYPE_THROW = 19 高空抛物侦测\r\n"
            "        JALARM_TYPE_CIGARETTE = 21 吸烟侦测\r\n"
            "        JALARM_TYPE_PASSENGER_FS = 22 客流统计变化\r\n"
            "        JALARM_TYPE_FALL = 23 跌倒侦测\r\n"
            "        JALARM_TYPE_BARCODE = 24 条形码侦测\r\n"
            "        JALARM_TYPE_FACESNAP = 25 人脸抓拍\r\n"
        },
        {"starttime"   , "开始搜索时间，格式: 2014-03-14 11:15:39 "  },
        {"endtime"     , "结束搜索时间，格式: 2014-03-14 11:15:39 "  },
        {"itemindex"    , "整个日志记录中的偏移，取值范围[1, itemtotal]"},
        {"itemnum"     , "输入时为请求数目，[index ~ index+num-1]\r\n"
                        "        输出时为实际返回数目。"},
        {"alarmchn", "报警通道号 (报警输入，和拓展报警时候用来代表不同的通道)\r\n"
            "        报警通道号 AI(0~3). EXP(0~15). "},
        {"End"     , "getalarmevent -? 获取帮助\r\n"
            "     返回结果中itemtotal:表示符合条件后的所有记录数"},
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK    , NULL    , NULL           , 0                 },
        {"act"      , ARG_TYPE_ACT    , "list"  , (void*)action  , sizeof(action)    },
        {"type"     , ARG_TYPE_MUSTINT, "0~14"   , &info.type     , sizeof(int)       },
        {"starttime", ARG_TYPE_ACT    , NULL    , info.stime     , sizeof(info.stime)},
        {"endtime"  , ARG_TYPE_ACT    , NULL    , info.etime     , sizeof(info.etime)},
        {"itemindex", ARG_TYPE_MUSTINT, "1~65535", &info.itemindex, sizeof(int)       },
        {"itemnum"  , ARG_TYPE_MUSTINT, "0~50"  , &info.itemnum  , sizeof(int)       },
        {"alarmchn" , ArgTypesInt     , "0~15"  , &info.channel  , sizeof(int)       },
        {"End"      , ArgTypesEnd     , NULL    , NULL           , 0                 },
    };

    JCP_ARG_PARSER(NULL, 0);

    if (strcmp(info.stime, info.etime) > 0) {
        sprintf(buf, "%s", "starttime > endtime");
        return -1;
    }

    int len = 0;

    TIME_START
    alarm_query(&info, alarmbuf, sizeof(alarmbuf));
    len = strlen(alarmbuf);
    if (len < buflen) {
        memcpy(buf, alarmbuf, len);
    } else {
        memcpy(buf, alarmbuf, buflen);
    }

    //DBG("alarmbuf = %s\n", alarmbuf);
    TIME_END
    return SUCCESS;
}

int JCPCmdOsdstyleCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    OsdStyleS inner = {0};
    OsdStyleS outer = {0};

    HelpMsgS helps[] = {
        {"?"        , "设置osd字体大小及颜色，不支持动态设置"               },
        {"act"      , "list:获取所有参数 set:设置参数"},
        {"colormode", "0: 自动 1:白色 2:黑色"         },
        {"font"     , "0:宋体"                        },
        {"width"    , "字体宽度像素值，8像素对齐"     },
        {"height"   , "字体高度像素值，8像素对齐"     },
        {"End"      , "osdstyle -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL                   , 0             },
        {"act"      , ARG_TYPE_ACT, "list|set", (void*)action          , sizeof(action)},
        {"colormode", ArgTypesInt , "0~2"   , (void*)&outer.colormode, sizeof(int)   },
        {"font"     , ArgTypesInt , "0"       , (void*)&outer.font     , sizeof(int)   },
        {"width"    , ArgTypesInt , "8~96"    , (void*)&outer.width    , sizeof(int)   },
        {"height"   , ArgTypesInt , "8~96"    , (void*)&outer.height   , sizeof(int)   },
        {"End"      , ArgTypesEnd , NULL      , NULL                   , 0             },
    };

    ret = get_config(handleOsdStyleCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(OsdStyleS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("list")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(OsdStyleS));
        ret = set_config(handleOsdStyleCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPCmdOSDCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    OsdInfoS inner = {0};
    OsdInfoS outer = {0};

    HelpMsgS helps[] = {
        {"?"           , "osd基础信息设置"                                },
        {"act"         , "list:获取所有参数 set:设置参数"                 },
        {"timeen"      , "时间显示开关 1:显示  0:不显示"                  },
        {"timeleft"    , "最左上像素x坐标  0~1919"                        },
        {"timetop"     , "最左上像素y坐标  0~1079"                        },
        {"bpsen"       , "码率显示开关 1:显示  0:不显示"                  },
        {"bpsleft"     , "最左上像素x坐标  0~1919"                        },
        {"bpstop"      , "最左上像素y坐标  0~1079"                        },
        {"nameen"      , "osd名字显示开关 1:显示  0:不显示"               },
        {"nameleft"    , "最左上像素x坐标  0~1919"                        },
        {"nametop"     , "最左上像素y坐标  0~1079"                        },
        {"name"        , "OSD名字内容"                                    },
        {"gpsen"       , "GPS显示开关 1:显示  0:不显示(当前不支持GPS功能)"},
        {"gpsleft"     , "最左上像素x坐标  0~1919"                        },
        {"gpstop"      , "最左上像素y坐标 0~1079"                         },
        {"osdcolormode", "OSD颜色(些功能移动到osdstylecfg)"               },
        {"osdlanguage" , "OSD语言 0:中文 1:英文"                          },
        {"osdweek"     , "OSD显示星期: 0:不显示 1:显示"                   },
        {"hdtop"       , "hd_pic最左上像素x坐标  0~1919"                 },
        {"hdleft"      , "hd_pic最左上像素y坐标 0~1079"                  },
        {"dateformat"  , "日期显示格式：              \
                            0: 2020年1月1日           \
                            1: 1日1月2020年           \
                            2: 2020-1-1               \
                            3: 1-1-2020               \
                            4: 2020/1/1               \
                            5: 1/1/2020               \
                            6: 12:12 2020年1月1日     \
                            7: 12:12 2020-1-1         \
                            8: 12:12 2020/1/1"                        },
        {"End"         , "osdcfg -? 获取帮助"                             },
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL                  , 0                 },
        {"act"         , ARG_TYPE_ACT  , "list|set", (void*)action         , sizeof(action)    },
        {"timeen"      , ArgTypesInt   , "0|1"     , (void*)&outer.timeen  , sizeof(int)       },
        {"timeleft"    , ArgTypesInt   , "0~1920"  , (void*)&outer.timeleft, sizeof(int)       },
        {"timetop"     , ArgTypesInt   , "0~1080"  , (void*)&outer.timetop , sizeof(int)       },
        {"bpsen"       , ArgTypesInt   , "0|1"     , (void*)&outer.bpsen   , sizeof(int)       },
        {"bpsleft"     , ArgTypesInt   , "0~1920"  , (void*)&outer.bpsleft , sizeof(int)       },
        {"bpstop"      , ArgTypesInt   , "0~1080"  , (void*)&outer.bpstop  , sizeof(int)       },
        {"nameen"      , ArgTypesInt   , "0|1"     , (void*)&outer.nameen  , sizeof(int)       },
        {"nameleft"    , ArgTypesInt   , "0~1920"  , (void*)&outer.nameleft, sizeof(int)       },
        {"nametop"     , ArgTypesInt   , "0~1080"  , (void*)&outer.nametop , sizeof(int)       },
        {"name"        , ArgTypesString, NULL      , (void*)outer.name     , sizeof(outer.name)},
        {"gpsen"       , ArgTypesInt   , "0|1"     , (void*)&outer.gpsen   , sizeof(int)       },
        {"gpsleft"     , ArgTypesInt   , "0~1920"  , (void*)&outer.gpsleft , sizeof(int)       },
        {"gpstop"      , ArgTypesInt   , "0~1080"  , (void*)&outer.gpstop  , sizeof(int)       },
        {"osdcolormode", ArgTypesInt   , "0~3"     , (void*)&outer.osdcolor, sizeof(int)       },
        {"osdlanguage" , ArgTypesInt   , "0~3"     , (void*)&outer.osdlanguage  , sizeof(int)  },
        {"osdweek"     , ArgTypesInt   , "0|1"     , (void*)&outer.osdweek , sizeof(int)       },
        {"hdtop"       , ArgTypesInt   , "0~1080"  , (void*)&outer.hdtop   , sizeof(int)       },
        {"hdleft"      , ArgTypesInt   , "0~1920"  , (void*)&outer.hdleft  , sizeof(int)       },
        {"dateformat"  , ArgTypesInt   , "0~8"     , (void*)&outer.dateformat  , sizeof(int)       },

        {"End"         , ArgTypesEnd   , NULL      , NULL                  , 0                 },
    };

    ret = get_config(handleOsdinfoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(OsdInfoS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(OsdInfoS));
        ret = set_config(handleOsdinfoCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}

int JCPTimeOSDCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    TimeOSD inner = {0};
    TimeOSD outer = {0};

    HelpMsgS helps[] = {
        {"?"        , "OSD 时间显示格式"                    },
        {"act"      , "list:获取所有参数 set:设置参数"},
        {"seq"      , "年月日显示顺序(0~5)\r\n"
         "            0:年月日 1:年日月 2:月年日\r\n"
         "            3:日年月 4:月日年 5:日月年\r\n"
        },
        {"connector", "年月日连接符 0是/ 1是- 2是:"   },
        {"End"      , "timeosdcfg -? 获取帮助"            },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK    , NULL      , NULL            , 0                 },
        {"act"      , ARG_TYPE_ACT    , "list|set", action          , sizeof(action)    },
        {"seq"      , ArgTypesInt     , "0~5"     , &outer.seq      , sizeof(int)       },
        {"connector", ArgTypesInt     , "0~2"     , &outer.connector, sizeof(int)       },
        {"End"      , ArgTypesEnd     , NULL      , NULL            , 0                 },
    };

    ret = get_config(handleTimeOSDCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(TimeOSD));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(TimeOSD));

        ret = set_config(handleTimeOSDCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;
}

int JCPCmdRoiCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    RoiAreaS inner = {0};
    RoiAreaS outer = {0};
    RoiArea0 *z = NULL;

    HelpMsgS helps[] = {
        {"?"     , "ROI区域设置  左上角坐标为(x0 , y0)，右下角坐标为(x1, y1)"   },
        {"act"   , "list:获取所有参数 set:设置参数"                             },
        {"gnum"  , "遮挡块总数"                                                 },
        {"id"    , "在一组数据的最前面，指定组id"                               },
        {"enable", "是否启用: 1，启用；0，禁止"                                 },
        {"qp"    , "指定窗口的量化步长，不需要调整"                              },
        {"interval", "指定窗口或背景的帧编码间隔，不需要调整"                    },
        {"left"  , "initial x pos"                                              },
        {"top"   , "initial y pos"                                              },
        {"right" , "end x pos"                                                  },
        {"bottom", "end y pos"                                                  },
        {"End"   , "roicfg -?获取帮助"                                          },
    };

    ArgOptS opts[] =
    {
        {"?"     , ARG_TYPE_ASK        , NULL      , NULL       , 0             },
        {"act"   , ARG_TYPE_ACT        , "list|set", action     , sizeof(action)},
        {"gnum"  , ARG_TYPE_LISTI      , "8"     , &outer.gnum, sizeof(int)   },
        {"id"    , ARG_TYPE_LIST_ID    , "0~7"     , &z->id     , sizeof(int)   },
        {"enable", ARG_TYPE_LIST_MEMINT, "0|1"     , &z->enable , sizeof(int)   },
        {"qp"    , ARG_TYPE_LIST_MEMINT, "74~125"  , &z->qp, sizeof(int)   },
        {"interval", ARG_TYPE_LIST_MEMINT, "0~30"  , &z->interval, sizeof(int)   },
        {"left"  , ARG_TYPE_LIST_MEMINT, "0~1919"  , &z->left   , sizeof(int)   },
        {"top"   , ARG_TYPE_LIST_MEMINT, "0~1079"  , &z->top    , sizeof(int)   },
        {"right" , ARG_TYPE_LIST_MEMINT, "0~1919"  , &z->right  , sizeof(int)   },
        {"bottom", ARG_TYPE_LIST_MEMINT, "0~1079"  , &z->bottom , sizeof(int)   },
        {"End"   , ArgTypesEnd         , NULL      , NULL       , 0             },
    };

    ret = get_config(handleRoiCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(RoiAreaS));

    JCP_ARG_PARSER(outer.area, sizeof(RoiArea0));

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        ASMJCP_LIST_COUNT(inner.area, sizeof(RoiArea0), MAX_ROI_AREA);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(RoiAreaS));

        ret = set_config(handleRoiCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;

}


int JCPCmdGetLog(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    char alarmbuf[4*1024] = {0};

    LogQueryInfoS info = {0};

    HelpMsgS helps[] = {
        {"?"   , "查询系统日志信息"               },
        {"act" , "list:查询命令"},
        {"type", "日志类型 保留 暂不支持分类型查询,只查询全部的"},
        {"starttime", "查询开始时间 格式 : 2014-03-14 11:15:39 "  },
        {"endtime"  , "查询结束时间 格式 : 2014-03-14 11:15:39 "  },
        {"itemindex", "整个日志记录中的偏移，取值范围[1, itemtotal]"},
        {"itemnum"  , "输入时为请求数目，[index ~ index+num-1]\r\n"
                      "       输出时为实际返回数目。"},
        {"level"    , "日志级别，保留，只支持所有，不分级别"},
        {"End"      , "getlog -act list -starttime “2014-04-24 17:02:00” "
                       "-endtime “2014-04-25 11:15:39”-itemindex  1 "
                       "-itemnum 20\r\n"
                   "        指查询这时间段内从第一条开始的20条日志信息\r\n"
                   "        查询后返回的结果中， itemtotal 表示符合条件后的所有记录数"},
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK    , NULL    , NULL           , 0                     },
        {"act"      , ARG_TYPE_ACT    , "list"  , (void*)action  , sizeof(action)        },
        {"type"     , ArgTypesInt     , "0~9"   , &info.type     , sizeof(int)           },
        {"starttime", ARG_TYPE_ACT    , NULL    , info.starttime , sizeof(info.starttime)},
        {"endtime"  , ARG_TYPE_ACT    , NULL    , info.endtime   , sizeof(info.endtime)  },
        {"itemindex", ARG_TYPE_MUSTINT, "1~65535", &info.itemindex, sizeof(int)           },
        {"itemnum"  , ARG_TYPE_MUSTINT, "0~30"  , &info.itemnum  , sizeof(int)           },
        {"level"    , ArgTypesInt     , "0~2"  , &info.level    , sizeof(int)           },
        {"End"      , ArgTypesEnd     , NULL    , NULL           , 0                     },
    };

    JCP_ARG_PARSER(NULL, 0);

    if (strcmp(info.starttime, info.endtime) > 0) {
        sprintf(buf, "%s", "starttime > endtime");
        return -1;
    }
    int len = 0;
    TIME_START
    log_query(&info, alarmbuf, sizeof(alarmbuf));
    len = strlen(alarmbuf);
    DBG("len = %d, buflen = %d\n", len, buflen);
    if (len < buflen) {
        memcpy(buf, alarmbuf, len);
    } else {
        memcpy(buf, alarmbuf, buflen);
    }
    TIME_END

    return SUCCESS;

}

int JCPCmdUpdate(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int  ret;
    int  cmd = 0;
    int  ota = 0;

    UpdateS inner = {0,};
    UpdateS outer = {0,};

    HelpMsgS helps[] = {
        {"?"          , "升级状态获取，失败重启需要3s，每2秒读一次状态，保证成功"       },
        {"act"        , "list:获取进度， set: 设置升级类型， clr 初始升级状态"          },
        {"cmd"        , "兼容升级工具"                                                  },
        {"type"       , "0:初始状态，1:升级包，2:升级单片机，3:升级定制项"              },
        {"progressbar", "小于100时进度值；100成功；101~110包格式错误；111:脚本执行错误" },
        {"ota"        , "远程Wget后手动触发升级 /tmp/upgrade.tgz"                       },
        {"End"        , "update -? 获取帮助"                                            },
    };

    ArgOptS opts[] = {
        {"?"          , ARG_TYPE_ASK  , NULL          , NULL              , 0                        },
        {"act"        , ArgTypesString, "list|set|clr", (void*)action     , sizeof(action)           },
        {"cmd"        , ArgTypesInt   , NULL          , &cmd              , sizeof(cmd)              },
        {"type"       , ArgTypesInt   , "0~3"         , &outer.type       , sizeof(outer.type      ) },
        {"progressbar", ArgTypesInt   , "0~111"       , &outer.progressbar, sizeof(outer.progressbar)},
        {"ota"        , ArgTypesInt   , "0|1"         , &ota              , sizeof(ota)              },
        {"End"        , ArgTypesEnd   , NULL          , NULL              , 0                        },
    };

    ret = get_config(handleUpdateCfg, inner);
    memcpy(&outer, &inner, sizeof(UpdateS));        // parser must after memcpy()
    JCP_ARG_PARSER(NULL, 0);

    if (SUCCESS == arg_opt_if_set("cmd", opts)) {
        return SUCCESS;
    }
    if (SUCCESS != arg_opt_if_set("act", opts)) {
        sprintf(buf, "-act is needed of update\n");
        return FAILURE;
    }

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);

#ifdef __migrate_ball_over__
        ShowWebS web = {0};
        get_capability(&web);
        if (web.dome == 0) {
            return SUCCESS;
        }

        char mcubuf[128] ={0};
        char version[128] = {0};
        ptz_get_version(version);
        sprintf(mcubuf, "process=%d;%s;", getProcess(), version);
        strcat(buf, mcubuf);
#endif
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        if (ota) {
            return SUCCESS;
        }
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(UpdateS));
        ret = set_config(handleUpdateCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    } else if(!strncasecmp("clr", action,strlen("clr"))) {
        // conf_clr_updatecfg();
        memset(&outer, 0, sizeof(UpdateS));
        ret = set_config(handleUpdateCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;

}


int JCPCmdVeprofileCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    VeProfileS inner;
    VeProfileS outer;
    memset(&inner, 0, sizeof(VeProfileS));
    memset(&outer, 0, sizeof(VeProfileS));
    ProfileS *z = NULL;

    HelpMsgS helps[] = {
        {"?"   , "视频尺寸编码参数配置"          },
        {"act" , "list:获取所有参数 set:设置参数"},
        {"gnum", "支持的视频尺寸数目"            },
        {"vesize", "视频尺寸"
                    "         VencSizeE_QCIF = 0,      // QVGA\r\n"
                    "         VencSizeE_CIF = 1,       // CIF\r\n"
                    "         VencSizeE_D1 = 2,        // D1\r\n"
                    "         VencSizeE_720P = 3,      // 720P\r\n"
                    "         VencSizeE_UVGA = 4,      // UVGA \r\n"
                    "         VencSizeE_1080P = 5,     // 1080P\n"
                    "         VencSizeE_QVGA = 6,      // QVGA\r\n"
                    "         VencSizeE_VGA = 7,       // VGA\r\n"
                    "         VencSizeE_960P = 8,      // 960P\r\n"
                    "         VencSizeE_3M = 9,        // 3M\r\n"
                    "         VencSizeE_4M = 12,       // 4M"
                    "         VencSizeE_5M = 13,       // 5M\r\n"
                    "         VencSizeE_4M_Dahua = 14  // 4M_Dahua"
                    "         VencSizeE_8M = 15,       // 8M"},
        {"profile"   , "编码profile 0 high; 1 main; 2 base"                           },
        {"level"     , "编码level"                                                    },
        {"bIDREnable", "IDR使能"                                                      },
        {"End"       , "如设置CIF的编码参数，veprofile -vesize 1 -profile 1 -level 10"},
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK        , NULL      , NULL          , 0             },
        {"act"       , ARG_TYPE_ACT        , "list|set", action        , sizeof(action)},
        {"gnum"      , ArgTypesListOnly    , "10"      , NULL          , 0             },
        {"vesize"    , ARG_TYPE_LIST_ID    , "0~15"     , &z->vesize    , sizeof(int)   },
        {"profile"   , ARG_TYPE_LIST_MEMINT, "0~2"     , &z->profile   , sizeof(int)   },
        {"level"     , ARG_TYPE_LIST_MEMINT, "0~50"    , &z->level     , sizeof(int)   },
        {"bIDREnable", ARG_TYPE_LIST_MEMINT, "0~1"     , &z->bIDREnable, sizeof(int)   },
        {"End"       , ArgTypesEnd         , NULL      , NULL          , 0             },
    };

    ret = get_config(handleProfileCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VeProfileS));

    JCP_ARG_PARSER(outer.ps, sizeof(ProfileS));

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        int len = sprintf(buf, "gnum=%d;", MAX_PROFILE_NUM);
        ASMJCP_LIST_STRING(buf+len, buflen, opts);
        ASMJCP_LIST_COUNT(inner.ps, sizeof(ProfileS), MAX_PROFILE_NUM);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VeProfileS));
        ret = set_config(handleProfileCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);

    }
    return SUCCESS;

}

/*
| 通过预置位实现      | 开       | 关      |
| :------------------ | :------  | :------ |
| 机械变倍(x1~x4)开关 |          |         |
| 数字变倍(x5~x8)开关 | call 255 | del 255 |
| OSD动态显示开关     | set 254  | del 254 |
*/

static int JCPCmdX4x8ctrl(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int osd = 0;
    int x8 = 0;
    int State;
    HelpMsgS helps[] = {
        {"?"  , "网页识别项"                        },
        {"act", "list:获取所有参数 "                },
        {"osd", "OSD 动态显示开关 set  254 &del 254"},
        {"x8" , "数字变倍开关     call 255 &del 255"},
        {"End", "恢复出厂: 开osd，关数字变倍"       },
    };

    ArgOptS opts[] =
    {
        {"?"  , ARG_TYPE_ASK, NULL      , NULL  , 0             },
        {"act", ARG_TYPE_ACT, "set|list", action, sizeof(action)},
        {"osd", ArgTypesInt , "0|1"     , &osd  , sizeof(int)   },
        {"x8" , ArgTypesInt , "0|1"     , &x8   , sizeof(int)   },
        {"End", ArgTypesEnd , NULL      , 0     , sizeof(int)   },
    };

    JCP_ARG_PARSER(NULL, 0);
    if(!strcasecmp("list", action)) {
        State = is_preset_vaild(PRESET_ZOOM_OSD);
        if(State) osd = 1;
        DBG("p254: State=%d osd=%d\n",State,osd);
        State = is_preset_vaild(PRESET_ENABLE_DZOOM);
        if(State)  x8 = 1;
        DBG("p255: State=%d x8=%d\n",State,x8);
#if defined(DZOOM_12X)
        osd = 1;
        x8 = 1;
#endif

        ASMJCP_LIST_STRING(buf, buflen, opts);
        return SUCCESS;
    } else {
        sprintf(buf, "list only\n");
        return FAILURE;
    }
}

int JCPCmdXkcd(char *buf, int buflen, int argc, char **argv)
{
    static int is_superman = FALSE;

    if (!is_superman) {
        char superpwd[36] = {0};
        char devid[16] = {0};

        system_get_dev_id(devid);
        get_usr_super(devid, superpwd, sizeof(superpwd));

        if (argc == 2 && 0 == strcmp(argv[1], superpwd)) {
            SYSLOG("superman key coming, enable xkcd\n");
            sprintf(buf, "[telnet IP 24] success");
            UtilSystemCmd((char *)"telnetd -p24");
            set_jcp_authorization();
            is_superman = TRUE;
        } else {
            SYSLOG("superman %s key needed, try again\n", devid);
            sprintf(buf, "%s KEY is wrong, try again", devid);
        }
    } else {
        char cmdline[256] = {0};
        char *p_cmd = cmdline;
        int i = 0;

        for (i = 1; i < argc; i++) {
            sprintf(p_cmd, "%s ", argv[i]);
            p_cmd += strlen(argv[i]) +1;
        }
        int is_ccli = FALSE;
        if (strstr(cmdline, "ccli")) {
            is_ccli = TRUE;
            strcat(cmdline, " > /tmp/ccout &");
        }

        drop_tail_space(cmdline);
        size_t sz = strlen(cmdline);
        SYSLOG("xkcd@box # %s\n", cmdline);
        if (cmdline[sz-1] == '&') {
            UtilSystemCmd(cmdline);
            if (TRUE == is_ccli) {
                const char *hint = "the result is located in /tmp/ccout";
                snprintf(buf, buflen-1, "[%s] exec ok, %s", cmdline, hint);
            } else {
                snprintf(buf, buflen-1, "[%s] exec ok", cmdline);
            }
        } else {
            ReadCmdResult(cmdline, buf, buflen-1);
            replace_str(buf, "\r\n", "<CR>");
            replace_str(buf, "\n", "<CR>");
        }
    }

    return SUCCESS;
}

static int JCPCmdGpio(char *buf, int buflen, int argc, char **argv)
{
    int  ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    gpio_t inner = {0};
    gpio_t outer = {0};

    HelpMsgS helps[] = {
        {"?"        , "cmd describle"      },
        {"act"      , "list:read set:write"},
        {"led_index", "led"                },
        {"ao_prompt", "audio_prompt"       },
        {"End"      , ""                   },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL            , 0                      },
        {"act"      , ARG_TYPE_ACT, "list|set", action          , sizeof(action)         },
        {"led_index", ArgTypesInt , "0|1"     , &outer.led_index, sizeof(outer.led_index)},
        {"ao_prompt", ArgTypesInt , "0|1"     , &outer.ao_prompt, sizeof(outer.ao_prompt)},
        {"End"      , ArgTypesEnd , NULL      , NULL            , 0                      },
    };


    ret = get_config(handleGpioCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(gpio_t));

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(gpio_t));
        ret = set_config(handleGpioCfg, outer);
    }
    return SUCCESS;
}

int JCPCmdGsys(char *buf, int buflen, int argc, char **argv)
{
    sSys tmp_sys = {0};
    char act[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"        , "system info"     },
        {"act"      , "list|set"        },
        {"usb_4g"   , "4G"              },
        {"usb_asix" , "usb转eth"        },
        {"usb_wifi" , "Wifi"            },
        {"eth"      , "以太网卡"        },
        {"jz"       , "君正"            },
        {"df"       , "多方"            },
        {"fh"       , "富瀚"            },
        {"ax"       , "爱芯"            },
        {"hs"       , "海思"            },
        {"maxheight", "拉升后最大高度"  },
        {"testing"  , "测试"            },
        {"factest"  , "产测"            },
        {"agingtest", "老化"            },
        {"upgrading", "升级中"          },
        {"End"      , "gsys -? 获取帮助"},
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL              , 0          },
        {"act"      , ARG_TYPE_ACT, "list|set", act               , sizeof(act)},
        {"usb_4g"   , ArgTypesInt , "0|1"     , &tmp_sys.usb_4g   , sizeof(int)},
        {"usb_asix" , ArgTypesInt , "0|1"     , &tmp_sys.usb_asix , sizeof(int)},
        {"usb_wifi" , ArgTypesInt , "0|1"     , &tmp_sys.usb_wifi , sizeof(int)},
        {"eth"      , ArgTypesInt , "0|1"     , &tmp_sys.eth      , sizeof(int)},
        {"jz"       , ArgTypesInt , "0|1"     , &tmp_sys.jz       , sizeof(int)},
        {"df"       , ArgTypesInt , "0|1"     , &tmp_sys.df       , sizeof(int)},
        {"fh"       , ArgTypesInt , "0|1"     , &tmp_sys.fh       , sizeof(int)},
        {"ax"       , ArgTypesInt , "0|1"     , &tmp_sys.ax       , sizeof(int)},
        {"hs"       , ArgTypesInt , "0|1"     , &tmp_sys.hs       , sizeof(int)},
        {"maxheight", ArgTypesInt , NULL      , &tmp_sys.maxheight, sizeof(int)},
        {"testing"  , ArgTypesInt , "0|1"     , &tmp_sys.testing  , sizeof(int)},
        {"factest"  , ArgTypesInt , "0|1"     , &tmp_sys.factest  , sizeof(int)},
        {"agingtest", ArgTypesInt , "0|1"     , &tmp_sys.agingtest, sizeof(int)},
        {"upgrading", ArgTypesInt , "0|1"     , &tmp_sys.upgrading, sizeof(int)},
        {"End"      , ArgTypesEnd , NULL      , NULL              , 0          },
    };

    load_g_sys(&tmp_sys);
    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", act, strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", act, strlen("set"))) {
        dump_g_sys(&tmp_sys);
        for (int i = 2; i < argc; i+=2) {
            SYSLOG("gsys: %s %s\n", argv[i], argv[i+1]);
        }
    }

    return SUCCESS;
}

int JCPCmdGlog(char *buf, int buflen, int argc, char **argv)
{
    char act[JCP_ACTION_LEN] = {0};
    sMod tmp_log = {0};

    HelpMsgS helps[] = {
        {"?"      , "各模块 log 开关"      },
        {"act"    , "list|set"             },
        {"venc"   , "venc日志    1:开 0:关"},
        {"audio"  , "audio日志   1:开 0:关"},
        {"record" , "record日志  1:开 0:关"},
        {"jcp"    , "jcp日志     1:开 0:关"},
        {"sim4g"  , "4g日志      1:开 0:关"},
        {"wifi"   , "wifi日志    1:开 0:关"},
        {"rtsp"   , "rtsp日志    1:开 0:关"},
        {"search" , "search日志  1:开 0:关"},
        {"upgrade", "upgrade日志 1:开 0:关"},
        {"http"   , "http日志    1:开 0:关"},
        {"onvif"  , "onvif日志   1:开 0:关"},
        {"tencent", "tencent日志 1:开 0:关"},
        {"osd"    , "osd日志     1:开 0:关"},
        {"lamp"   , "lamp日志    1:开 0:关"},
        {"alarm"  , "alarm日志   1:开 0:关"},
        {"md"     , "md日志      1:开 0:关"},
        {"hd"     , "hd日志      1:开 0:关"},
        {"ptz"    , "ptz日志     1:开 0:关"},
        {"gb28181", "gb28181日志 1:开 0:关"},
        {"isp"    , "isp日志     1:开 0:关"},
        {"pwm"    , "pwm日志     1:开 0:关"},
        {"cry"    , "cry日志     1:开 0:关"},
        {"vidcall", "vidcall日志 1:开 0:关"},
        {"od"     , "od日志      1:开 0:关"},
        {"vidmask", "vidmask日志 1:开 0:关"},
        {"ivx"    , "ivx日志     1:开 0:关"},
        {"mmi"    , "ali mmi日志 1:开 0:关"},
        {"asr"    , "asr 日志    1:开 0:关"},
        {"dbg"    , "日志重定向  1:开 0:关"},
        {"face"   , "face日志    1:开 0:关"},
        {"End"    , "glog -? 获取帮助"     },
    };

    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK, NULL      , NULL            , 0          },
        {"act"    , ARG_TYPE_ACT, "list|set", act             , sizeof(act)},
        {"venc"   , ArgTypesInt , NULL      , &tmp_log.venc   , sizeof(int)},
        {"audio"  , ArgTypesInt , NULL      , &tmp_log.audio  , sizeof(int)},
        {"record" , ArgTypesInt , NULL      , &tmp_log.record , sizeof(int)},
        {"jcp"    , ArgTypesInt , NULL      , &tmp_log.jcp    , sizeof(int)},
        {"sim4g"  , ArgTypesInt , NULL      , &tmp_log.sim4g  , sizeof(int)},
        {"wifi"   , ArgTypesInt , NULL      , &tmp_log.wifi   , sizeof(int)},
        {"rtsp"   , ArgTypesInt , NULL      , &tmp_log.rtsp   , sizeof(int)},
        {"search" , ArgTypesInt , NULL      , &tmp_log.search , sizeof(int)},
        {"upgrade", ArgTypesInt , NULL      , &tmp_log.upgrade, sizeof(int)},
        {"http"   , ArgTypesInt , NULL      , &tmp_log.http   , sizeof(int)},
        {"onvif"  , ArgTypesInt , NULL      , &tmp_log.onvif  , sizeof(int)},
        {"tencent", ArgTypesInt , NULL      , &tmp_log.tencent, sizeof(int)},
        {"osd"    , ArgTypesInt , NULL      , &tmp_log.osd    , sizeof(int)},
        {"lamp"   , ArgTypesInt , NULL      , &tmp_log.lamp   , sizeof(int)},
        {"alarm"  , ArgTypesInt , NULL      , &tmp_log.alarm  , sizeof(int)},
        {"md"     , ArgTypesInt , NULL      , &tmp_log.md     , sizeof(int)},
        {"hd"     , ArgTypesInt , NULL      , &tmp_log.hd     , sizeof(int)},
        {"ptz"    , ArgTypesInt , NULL      , &tmp_log.ptz    , sizeof(int)},
        {"gb28181", ArgTypesInt , NULL      , &tmp_log.gb28181, sizeof(int)},
        {"isp"    , ArgTypesInt , NULL      , &tmp_log.isp    , sizeof(int)},
        {"pwm"    , ArgTypesInt , NULL      , &tmp_log.pwm    , sizeof(int)},
		{"cry"    , ArgTypesInt , NULL      , &tmp_log.cry    , sizeof(int)},
		{"vidcall", ArgTypesInt , NULL      , &tmp_log.vidcall, sizeof(int)},
		{"od"     , ArgTypesInt , NULL      , &tmp_log.od     , sizeof(int)},
		{"vidmask", ArgTypesInt , NULL      , &tmp_log.vidmask, sizeof(int)},
        {"ivx"    , ArgTypesInt , NULL      , &tmp_log.ivx    , sizeof(int)},
		{"mmi"    , ArgTypesInt , NULL      , &tmp_log.mmi    , sizeof(int)},
		{"asr"    , ArgTypesInt , NULL      , &tmp_log.asr    , sizeof(int)},
        {"dbg"    , ArgTypesInt , NULL      , &tmp_log.dbg    , sizeof(int)},
        {"face"   , ArgTypesInt , NULL      , &tmp_log.face   , sizeof(int)},
        {"End"    , ArgTypesEnd , NULL      , NULL            , 0          },
    };

    load_g_log(&tmp_log);
    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", act,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", act, strlen("set"))) {
        dump_g_log(&tmp_log);
        for (int i = 2; i < argc; i+=2) {
            SYSLOG("glog: %s %s\n", argv[i], argv[i+1]);
        }
        if (SUCCESS == arg_opt_if_set("dbg", opts)) {
            toggle_redirect(get_g_log(dbg));
        }
    }

    return SUCCESS;
}

int JCPCmdGrun(char *buf, int buflen, int argc, char **argv)
{
    char act[JCP_ACTION_LEN] = {0};
    sMod tmp_run = {0};
    
    HelpMsgS helps[] = {
        {"?"      , "动态调试接口"    },
        {"act"    , "list|set"        },
        {"venc"   , "venc模块"        },
        {"audio"  , "audio模块"       },
        {"record" , "record模块"      },
        {"jcp"    , "jcp模块"         },
        {"sim4g"  , "4g模块"          },
        {"wifi"   , "wifi模块"        },
        {"rtsp"   , "rtsp模块"        },
        {"search" , "search模块"      },
        {"upgrade", "upgrade模块"     },
        {"http"   , "http模块"        },
        {"onvif"  , "onvif模块"       },
        {"tencent", "tencent模块"     },
        {"osd"    , "osd模块"         },
        {"lamp"   , "lamp模块"        },
        {"alarm"  , "alarm模块"       },
        {"md"     , "md模块"          },
        {"hd"     , "hd模块"          },
        {"ptz"    , "ptz模块"         },
        {"gb28181", "gb28181模块"     },
		{"isp"    , "isp模块"         },
        {"pwm"    , "pwm模块"         },
        {"cry"    , "cry模块"         },
        {"vidcall", "vidcall模块"     },
        {"od"     , "od模块"          },
        {"vidmask", "vidmask模块"     },
        {"ivx"    , "ivx模块"         },
        {"mmi"    , "ali mmi模块"     },
        {"dbg"    , "调试模块打印开关" },
        {"code"   , "模块内部调试参数" },
        {"End"    , "grun -? 获取帮助"},
    };

    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK, NULL      , NULL            , 0          },
        {"act"    , ARG_TYPE_ACT, "list|set", act             , sizeof(act)},
        {"venc"   , ArgTypesInt , NULL      , &tmp_run.venc   , sizeof(int)},
        {"audio"  , ArgTypesInt , NULL      , &tmp_run.audio  , sizeof(int)},
        {"record" , ArgTypesInt , NULL      , &tmp_run.record , sizeof(int)},
        {"jcp"    , ArgTypesInt , NULL      , &tmp_run.jcp    , sizeof(int)},
        {"sim4g"  , ArgTypesInt , NULL      , &tmp_run.sim4g  , sizeof(int)},
        {"wifi"   , ArgTypesInt , NULL      , &tmp_run.wifi   , sizeof(int)},
        {"rtsp"   , ArgTypesInt , NULL      , &tmp_run.rtsp   , sizeof(int)},
        {"search" , ArgTypesInt , NULL      , &tmp_run.search , sizeof(int)},
        {"upgrade", ArgTypesInt , NULL      , &tmp_run.upgrade, sizeof(int)},
        {"http"   , ArgTypesInt , NULL      , &tmp_run.http   , sizeof(int)},
        {"onvif"  , ArgTypesInt , NULL      , &tmp_run.onvif  , sizeof(int)},
        {"tencent", ArgTypesInt , NULL      , &tmp_run.tencent , sizeof(int)},
        {"osd"    , ArgTypesInt , NULL      , &tmp_run.osd    , sizeof(int)},
        {"lamp"   , ArgTypesInt , NULL      , &tmp_run.lamp   , sizeof(int)},
        {"alarm"  , ArgTypesInt , NULL      , &tmp_run.alarm  , sizeof(int)},
        {"md"     , ArgTypesInt , NULL      , &tmp_run.md     , sizeof(int)},
        {"hd"     , ArgTypesInt , NULL      , &tmp_run.hd     , sizeof(int)},
        {"ptz"    , ArgTypesInt , NULL      , &tmp_run.ptz    , sizeof(int)},
        {"gb28181", ArgTypesInt , NULL      , &tmp_run.gb28181, sizeof(int)},
        {"pwm"    , ArgTypesInt , NULL      , &tmp_run.pwm    , sizeof(int)},
        {"cry"    , ArgTypesInt , NULL      , &tmp_run.cry    , sizeof(int)},
        {"vidcall", ArgTypesInt , NULL      , &tmp_run.vidcall, sizeof(int)},
        {"od"     , ArgTypesInt , NULL      , &tmp_run.od     , sizeof(int)},
        {"vidmask", ArgTypesInt , NULL      , &tmp_run.vidmask, sizeof(int)},
        {"ivx"    , ArgTypesInt , NULL      , &tmp_run.ivx    , sizeof(int)},
		{"mmi"    , ArgTypesInt , NULL      , &tmp_run.mmi    , sizeof(int)},
		{"asr"    , ArgTypesInt , NULL      , &tmp_run.asr    , sizeof(int)},
        {"dbg"    , ArgTypesInt , NULL      , &tmp_run.dbg    , sizeof(int)},
        {"code"   , ArgTypesInt , NULL      , &tmp_run.code   , sizeof(int)},
        {"End"    , ArgTypesEnd , NULL      , NULL            , 0          },
    };

    load_g_run(&tmp_run);
    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", act,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", act, strlen("set"))) {
        dump_g_run(&tmp_run);
        if (SUCCESS == arg_opt_if_set("dbg", opts)) {
            if (get_g_run(dbg, RUN_DBG_SIGSEGV)) {
                int *p = NULL;
                *p = 0XBADF00D;
            }
        }

#if defined(PLATFORM_TENCENT)
        if (SUCCESS == arg_opt_if_set("tencent", opts)) {
            if (pop_g_run(tencent, RUN_SDK_LOG_LEVEL)) {
                iv_sys_set_log_level((iv_sys_log_level_type_e)tmp_run.code); // SDK log print/upload level
            }
            if (pop_g_run(tencent, RUN_XP2P_LOG_LEVEL)) {
                iv_sys_set_log_level((iv_sys_log_level_type_e)tmp_run.code);
                iv_avt_set_p2p_log_level((iv_sys_log_level_type_e)tmp_run.code); // XP2P log print/upload level
            }

            if (pop_g_run(tencent, RUN_CS_QURAY)) {
                check_cs_status();
            }
            return SUCCESS;
        }
#endif

        if (SUCCESS == arg_opt_if_set("mmi", opts)) {
            if (pop_g_run(mmi, RUN_MMI_LOG_LEVEL)) {
                DBG("set mmi log level %d\n", tmp_run.code);
                util_set_log_level(tmp_run.code); // SDK log print/upload level
            }
        }

        if (SUCCESS == arg_opt_if_set("audio", opts)) {
            if (pop_g_run(audio, RUN_AUDIO_PLAY_FORCE)) {
                encode_audio_queue_push_amr((AUDIO_PROMPT)tmp_run.code, true);
            } else if (pop_g_run(audio, RUN_AUDIO_PLAY_NUM)) {
                encode_audio_queue_push_amr((AUDIO_PROMPT)tmp_run.code, false);
            } else if (pop_g_run(audio, RUN_AUDIO_CAPTURE)) {
                chdir("/mnt");
            }
        }

        if (SUCCESS == arg_opt_if_set("lamp", opts)) {
            if (pop_g_run(lamp, E_RUN_IRCUT_DAY)) {
                set_ircut_status(1);
            } else if (pop_g_run(lamp, E_RUN_IRCUT_NIGHT)) {
                set_ircut_status(0);
            } else {
                WAR("tmp_run.code: %d\n", tmp_run.code);
                switch (tmp_run.code) {
                case 1:
                    lamp_set_day(TRUE);
                    break;
                case 2:
                    lamp_set_color(TRUE);
                    break;
                case 3:
                    lamp_set_night();
                    break;
                default:
                    break;
                }
            }
        }

        if (SUCCESS == arg_opt_if_set("ivx", opts)) {
            if (pop_g_run(ivx, RUN_IVX_PRINT_FOLLOW_STAT)) {
                DBG("follow_status: %d, person center status: %d\n",
                    get_follow_status(), get_person_center_status());
            }
        }

#ifndef STEPLESS_PWM
        if (SUCCESS == arg_opt_if_set("pwm", opts)) {
            if (pop_g_run(pwm, RUN_PWM_CHN0_ENABLE)) {
                if (tmp_run.code == TRUE) {
                    //pwm_open_export(PWM_WHITE, TRUE);
                    pwm_enable_chn(PWM_WHITE, TRUE);
                    pwm_set_period(PWM_WHITE, PWM_PERIOD);
                    pwm_set_polarity(PWM_WHITE, POLARITY_NORMAL);
                } else if (tmp_run.code == FALSE) {
                    pwm_enable_chn(PWM_WHITE, FALSE);
                    //pwm_open_export(PWM_WHITE, FALSE);
                }
            } else if (pop_g_run(pwm, RUN_PWM_CHN_ENABLE_GET)) {
                if (tmp_run.code >= 0 && tmp_run.code <= 1) {
                    int enable = 0;
                    pwm_get_chn_enable(tmp_run.code, &enable);
                    DBG("pwm chn: %d enable: %d\n", tmp_run.code, enable);
                }
            } else if (pop_g_run(pwm, RUN_PWM_DUTY_CYCLE_GET)) {
                int duty_cycle = 0;
                pwm_get_duty_cycle(PWM_WHITE, &duty_cycle);
                DBG("read duty: %d --> %d\n", duty_cycle, duty_cycle/24);
            } else if (pop_g_run(pwm, RUN_PWM_DUTY_CYCLE_SET)) {
                if (tmp_run.code >= 0 && tmp_run.code <= 1000) {
                    pwm_set_duty_cycle(PWM_WHITE, tmp_run.code*24);
                    DBG("set duty: %d --> %d\n", tmp_run.code, tmp_run.code*24);
                } else {
                    DBG("pwm duty %d is out of range\n", tmp_run.code);
                }
            }
        }
#endif

    }

    return SUCCESS;
}

int JCPCmdGstat(char *buf, int buflen, int argc, char **argv)
{
    char act[JCP_ACTION_LEN] = {0};
    sMod tmp_stat = {0};

    HelpMsgS helps[] = {
        {"?"      , "系统内部状态设置" },
        {"act"    , "list|set"         },
        {"venc"   , "venc模块"         },
        {"audio"  , "audio模块"        },
        {"record" , "record模块"       },
        {"jcp"    , "jcp模块"          },
        {"sim4g"  , "4g模块"           },
        {"wifi"   , "wifi模块"         },
        {"rtsp"   , "rtsp模块"         },
        {"search" , "search模块"       },
        {"upgrade", "upgrade模块"      },
        {"http"   , "http模块"         },
        {"onvif"  , "onvif模块"        },
        {"aliyun" , "aliyun模块"       },
        {"osd"    , "osd模块"          },
        {"lamp"   , "lamp模块"         },
        {"alarm"  , "alarm模块"        },
        {"md"     , "md模块"           },
        {"hd"     , "hd模块"           },
        {"ptz"    , "ptz模块"          },
        {"gb28181", "gb28181模块"      },
        {"cry"    , "cry模块"          },
        {"vidcall", "vidcall模块"      },
        {"od"     , "od模块"           },
        {"vidmask", "vidmask模块"      },
        {"ivx"    , "ivx 模块"         },
        {"mmi"    , "ali mmi模块"      },
        {"asr"    , "asr模块"          },
        {"dbg"    , "调试模块"         },
        {"code"   , "模块内部调试参数" },
        {"End"    , "gstat -? 获取帮助"},
    };

    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK, NULL      , NULL           , 0          },
        {"act"    , ARG_TYPE_ACT, "list|set", act            , sizeof(act)},
        {"venc"   , ArgTypesInt , NULL      , &tmp_stat.venc   , sizeof(int)},
        {"audio"  , ArgTypesInt , NULL      , &tmp_stat.audio  , sizeof(int)},
        {"record" , ArgTypesInt , NULL      , &tmp_stat.record , sizeof(int)},
        {"jcp"    , ArgTypesInt , NULL      , &tmp_stat.jcp    , sizeof(int)},
        {"sim4g"  , ArgTypesInt , NULL      , &tmp_stat.sim4g  , sizeof(int)},
        {"wifi"   , ArgTypesInt , NULL      , &tmp_stat.wifi   , sizeof(int)},
        {"rtsp"   , ArgTypesInt , NULL      , &tmp_stat.rtsp   , sizeof(int)},
        {"search" , ArgTypesInt , NULL      , &tmp_stat.search , sizeof(int)},
        {"upgrade", ArgTypesInt , NULL      , &tmp_stat.upgrade, sizeof(int)},
        {"http"   , ArgTypesInt , NULL      , &tmp_stat.http   , sizeof(int)},
        {"onvif"  , ArgTypesInt , NULL      , &tmp_stat.onvif  , sizeof(int)},
        {"tencent", ArgTypesInt , NULL      , &tmp_stat.tencent , sizeof(int)},
        {"osd"    , ArgTypesInt , NULL      , &tmp_stat.osd    , sizeof(int)},
        {"lamp"   , ArgTypesInt , NULL      , &tmp_stat.lamp   , sizeof(int)},
        {"alarm"  , ArgTypesInt , NULL      , &tmp_stat.alarm  , sizeof(int)},
        {"md"     , ArgTypesInt , NULL      , &tmp_stat.md     , sizeof(int)},
        {"hd"     , ArgTypesInt , NULL      , &tmp_stat.hd     , sizeof(int)},
        {"ptz"    , ArgTypesInt , NULL      , &tmp_stat.ptz    , sizeof(int)},
        {"gb28181", ArgTypesInt , NULL      , &tmp_stat.gb28181, sizeof(int)},
        {"cry"    , ArgTypesInt , NULL      , &tmp_stat.cry    , sizeof(int)},
        {"vidcall", ArgTypesInt , NULL      , &tmp_stat.vidcall, sizeof(int)},
        {"od"     , ArgTypesInt , NULL      , &tmp_stat.od     , sizeof(int)},
        {"vidmask", ArgTypesInt , NULL      , &tmp_stat.vidmask, sizeof(int)},
		{"mmi"    , ArgTypesInt , NULL      , &tmp_stat.mmi    , sizeof(int)},
		{"asr"    , ArgTypesInt , NULL      , &tmp_stat.asr    , sizeof(int)},
        {"ivx"    , ArgTypesInt , NULL      , &tmp_stat.ivx    , sizeof(int)},
        {"dbg"    , ArgTypesInt , NULL      , &tmp_stat.dbg    , sizeof(int)},
        {"code"   , ArgTypesInt , NULL      , &tmp_stat.code   , sizeof(int)},
        {"End"    , ArgTypesEnd , NULL      , NULL             , 0          },
    };

    load_g_stat(&tmp_stat);
    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", act,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", act, strlen("set"))) {
        dump_g_stat(&tmp_stat);
    }

    return SUCCESS;
}

int JCPCmdMDMBCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    MotionDetectS inner = {0,};
    MotionDetectS outer = {0,};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"     , "移动侦测告警"                                                              },
        {"act"   , "list:获取所有参数 set:设置参数"                                            },
        {"enable", "是否启用: 1，启用；0，禁止"                                                },
        {"thresh", "移动侦测阀值，阀值为所有区域公用一个阀值参数，共用一个移动量动态信息显示条"},
        {"mbdesc", "移动侦测宏块描述:\r\n"
         "      (1)界面方格数量固定（目前使用22×18个方格)\r\n"
         "      方格的长度=分辨率/22，向下取整，比如720/22=32，1280/22=58等\r\n"
         "      (2)界面与IPC之间的通信协议\r\n"
         "       使用行列表示法，0000000000000000000000，1111111111111111111111,.....\r\n"
         "       每行的一个宏块被选择，用1表示，没有选择用0表示，行与行用逗号间隔\r\n"
         "       上面的例子表示第一行没有选中，第二行选中\r\n"},
        {"timestrategy", "布防时间描述\r\n"
         "        时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
         "        低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
         "        如全时段禁用：0,0,0,0,0,0,0\r\n"
         "        365中的格式是二元组的形式\r\n"
         "        timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"},
        {"level" , "移动侦测级别，热点模式下APP适配，level=0不能代替enable=0来禁用MD功能"      },
        {"End" , "mdmbcfg -act set -enable 1 -thresh 20 -timestrategy 0:0,1:0,2:0,3:0,4:0,5:0,6:0"
                " -mbdesc 0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                "0000000000000000000000,0000000000000000000000,0000000000000000000000,"},
    };


    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL  , NULL         , 0                   },
        {"act"         , ARG_TYPE_ACT  , NULL  , action       , sizeof(action)      },
        {"enable"      , ArgTypesInt   , "0|1" , &outer.enable, sizeof(outer.enable)},
        {"thresh"      , ArgTypesInt   , "0~100", &outer.thresh, sizeof(outer.thresh)},
        {"mbdesc"      , ArgTypesString, NULL  , outer.mbdesc , sizeof(outer.mbdesc)},
        {"timestrategy", ArgTypesString, NULL  , timestrategy  , sizeof(timestrategy) },
        {"level"       , ArgTypesInt   , "0~3"  , &outer.level , sizeof(outer.level) },
        {"End"         , ArgTypesEnd   , NULL  , NULL         , 0                   },
    };

    ret = get_config(handleMotionDetectCfg, inner);

    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(MotionDetectS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.times);

        JCP_RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(MotionDetectS));
        ret = set_config(handleMotionDetectCfg, outer);
    }

    //分析字符串
    return SUCCESS;
}

int JCPCmdHDCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    HumanDetectionS inner = {0,};
    HumanDetectionS outer = {0,};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"            , "人形侦测告警"                                                         },
        {"act"          , "list:获取所有参数 set:设置参数"                                        },
        {"enable"       , "是否启用: 1.启用; 0.禁止"                                              },
        {"screenenable" , "屏幕是否显示人形框: 1.显示 2.不显示"                                    },
        {"thresh"       , "人形侦测阀值"                                                         },
        {"humandistance", "人体目标距离设置参数"                                                  },
        {"drag"         , "1.thresh已动态调整 0.未调整"                                           },
        {"mbdesc"       , "人形侦测宏块描述:22x18个方格"                                           },
        {"timestrategy" , "布防时间: 逗7分字符串"                                                  },
        {"outdoor"      , "0: scene indoor; 1 outdoor-man: 2 outdoor-car\r\n"                    },
       {"person_center", "人形居中功能: 1.启用; 0.禁止"                                           },
       {"faceae"       , "人脸收光功能: 1.启用; 0.禁止"                                           },
       {"level"        , "人形侦测级别, APP适配, level=0不能代替enable=0来禁用HD功能"              },
       {"End" , "humandetectcfg -act set -enable 1 -screenenable 1 -thresh 50 -humandistance 50  "
                 "-drag 1 -timestrategy 0:0,1:0,2:0,3:0,4:0,5:0,6:0 -outdoor 0 -mbdesc "
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"},
    };

    ArgOptS opts[] = {
        {"?"               , ARG_TYPE_ASK  , NULL   , NULL                 , 0                         },
        {"act"             , ARG_TYPE_ACT  , NULL   , action               , sizeof(action)            },
        {"enable"          , ArgTypesInt   , "0|1"  , &outer.enable        , sizeof(outer.enable)      },
        {"screenenable"    , ArgTypesInt   , "0|1"  , &outer.screenenable  , sizeof(outer.screenenable)},
        {"thresh"          , ArgTypesInt   , "0~100", &outer.thresh        , sizeof(outer.thresh)      },
        {"humandistance"   , ArgTypesInt   , "0~100", &outer.humandistance , sizeof(outer.humandistance)},
        {"drag"            , ArgTypesInt   , "0|1"  , &outer.drag          , sizeof(outer.drag)        },
        {"mbdesc"          , ArgTypesString, NULL   , outer.mbdesc         , sizeof(outer.mbdesc)      },
        {"timestrategy"    , ArgTypesString, NULL   , timestrategy         , sizeof(timestrategy)      },
        {"outdoor"         , ArgTypesInt   , "0~2"  , &outer.mode          , sizeof(outer.mode)        },
       {"person_center"   , ArgTypesInt   , "0|1"  , &outer.person_center  , sizeof(outer.person_center)},
       {"faceae"          , ArgTypesInt   , "0|1"  , &outer.faceae         , sizeof(outer.faceae)      },
       {"level"           , ArgTypesInt   , "0~3"  , &outer.level          , sizeof(outer.level)       },
       {"End"             , ArgTypesEnd   , NULL   , NULL                  , 0                         },
    };

    ret = get_config(handleHumanDetectCfg, inner);

    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(HumanDetectionS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);

        if (SUCCESS == arg_opt_if_set("drag", opts) && outer.drag) {
            static time_t prev_drag = 0;
            time_t curr = time(NULL);
            if ((outer.humandistance/10) == (inner.humandistance/10) && curr-prev_drag <= 1) {
                DBG("inner.drag : %d , outer.drag : %d....\n", inner.drag, outer.drag);
                DBG("same drag lvl:%d vs old:%d\n", outer.humandistance, inner.humandistance);
                prev_drag = curr;
                return SUCCESS;
            }
            prev_drag = curr;
        } else {
            outer.drag = 0;
        }
        timestr_to_intarray(timestrategy, outer.times);
        JCP_RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(HumanDetectionS));
        ret = set_config(handleHumanDetectCfg, outer);
    }

    //分析字符串
    return SUCCESS;
}

int JCPCmdCarCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    CarDetectionS inner = {0,};
    CarDetectionS outer = {0,};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"            , "车形侦测告警"                                                                },
        {"act"          , "list:获取所有参数 set:设置参数"                                                  },
        {"enable"       , "是否启用：2.人车共检；1.启用；0.禁止"                                                        },
        {"show"         , "车辆侦测是否显示：2.支持人车共检 1.显示；0.不显示"                                                   },
        {"screenenable" , "屏幕是否显示车形框：1.显示 2.不显示"                                                  },
        {"thresh"       , "车形侦测阀值"                                                                },
        {"level" , "车形侦测级别，热点模式下APP适配，level=0不能代替enable=0来禁用HD功能"      },
        {"cardistance", "车体目标距离设置参数"                                                            },
        {"drag"         , "1.thresh已动态调整 0.未调整"                                                   },
        {"mbdesc"       , "车形侦测宏块描述:22x18个方格"                                                     },
        {"timestrategy" , "布防时间：逗7分字符串"                                                           },
        {"outdoor"      , "0: scene indoor；1 outdoor-man: 2 outdoor-car\r\n"                      },
        {"End" , "humandetectcfg -act set -enable 1 -screenenable 1 -thresh 50 -cardistance 50  "
                 "-drag 1 -timestrategy 0:0,1:0,2:0,3:0,4:0,5:0,6:0 -outdoor 0 -mbdesc "
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"},
    };


    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL  , NULL               , 0                         },
        {"act"         , ARG_TYPE_ACT  , NULL  , action             , sizeof(action)            },
        {"enable"      , ArgTypesInt   , "0|1" , &outer.enable      , sizeof(outer.enable)      },
        {"show"        , ArgTypesInt   , "0|1|2", &outer.show       , sizeof(outer.show)        },
        {"screenenable", ArgTypesInt   , "0|1" , &outer.screenenable, sizeof(outer.screenenable)},
        {"thresh"      , ArgTypesInt   , "0~100", &outer.thresh     , sizeof(outer.thresh)      },
        {"level"       , ArgTypesInt   , "0~3"  , &outer.level      , sizeof(outer.level)       },
        {"cardistance" , ArgTypesInt   , "0~100", &outer.cardistance, sizeof(outer.cardistance) },
        {"drag"        , ArgTypesInt   , "0|1"  , &outer.drag       , sizeof(outer.drag)        },
        {"mbdesc"      , ArgTypesString, NULL  , outer.mbdesc       , sizeof(outer.mbdesc)      },
        {"timestrategy", ArgTypesString, NULL  , timestrategy       , sizeof(timestrategy)      },
        {"outdoor"     , ArgTypesInt   , "0~2" , &outer.mode        , sizeof(outer.mode)        },
        {"End"         , ArgTypesEnd   , NULL  , NULL               , 0                         },
    };

    ret = get_config(handleCarDetectCfg, inner);

    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(CarDetectionS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.times);
        JCP_RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(CarDetectionS));
        ret = set_config(handleCarDetectCfg, outer);
    }

    //分析字符串
    return SUCCESS;
}

int JCPCmdPetCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    PetDetectionS inner = {0,};
    PetDetectionS outer = {0,};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"            , "宠形侦测告警"                                                        },
        {"act"          , "list:获取所有参数 set:设置参数"                                      },
        {"enable"       , "是否启用：1.启用；0.禁止"                                            },
        {"screenenable" , "屏幕是否显示宠形框：1.显示 2.不显示"                                 },
        {"thresh"       , "宠形侦测阀值"                                                        },
        {"level"        , "宠形侦测级别，热点模式下APP适配，level=0不能代替enable=0来禁用HD功能"},
        {"petdistance"  , "宠形目标距离设置参数"                                                },  
        {"drag"         , "1.thresh已动态调整 0.未调整"                                         }, 
        {"mbdesc"       , "宠形侦测宏块描述:22x18个方格"                                        },
        {"timestrategy" , "布防时间：逗7分字符串"                                               },
        {"outdoor"      , "0: scene indoor；1 outdoor-man: 2 outdoor-pet\r\n"                   },
        {"End" , "petdetectcfg -act set -enable 1 -screenenable 1 -thresh 50 -petdistance 50  "
                 "-drag 1 -timestrategy 0:0,1:0,2:0,3:0,4:0,5:0,6:0 -outdoor 0 -mbdesc "
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"
                 "0000000000000000000000,0000000000000000000000,0000000000000000000000,"},
    };


    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL   , NULL               , 0                         },
        {"act"         , ARG_TYPE_ACT  , NULL   , action             , sizeof(action)            },
        {"enable"      , ArgTypesInt   , "0|1"  , &outer.enable      , sizeof(outer.enable)      },
        {"screenenable", ArgTypesInt   , "0|1"  , &outer.screenenable, sizeof(outer.screenenable)},
        {"thresh"      , ArgTypesInt   , "0~100", &outer.thresh      , sizeof(outer.thresh)      },
        {"level"       , ArgTypesInt   , "0~3"  , &outer.level       , sizeof(outer.level)       },
        {"petdistance" , ArgTypesInt   , "0~100", &outer.petdistance , sizeof(outer.petdistance) },
        {"drag"        , ArgTypesInt   , "0|1"  , &outer.drag        , sizeof(outer.drag)        },
        {"mbdesc"      , ArgTypesString, NULL   , outer.mbdesc       , sizeof(outer.mbdesc)      },
        {"timestrategy", ArgTypesString, NULL   , timestrategy       , sizeof(timestrategy)      },
        {"outdoor"     , ArgTypesInt   , "0~2"  , &outer.mode        , sizeof(outer.mode)        },
        {"End"         , ArgTypesEnd   , NULL   , NULL               , 0                         },
    };

    ret = get_config(handlePetDetectCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(PetDetectionS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.times);

        JCP_RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(PetDetectionS));
        ret = set_config(handlePetDetectCfg, outer);
    }

    //分析字符串
    return SUCCESS;
}

int JCPCmdCryCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    CryDetectionS inner = {0,};
    CryDetectionS outer = {0,};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"            , "哭声检测告警"                                                        },
        {"act"          , "list:获取所有参数 set:设置参数"                                      },
        {"enable"       , "是否启用：1.启用；0.禁止"                                            },
        {"screenenable" , "屏幕是否显示哭声框：1.显示 2.不显示"                                 },
        {"thresh"       , "哭声检测阀值"                                                        },
        {"level"        , "哭声检测级别，热点模式下APP适配，level=0不能代替enable=0来禁用HD功能"},
        {"timestrategy" , "布防时间：逗7分字符串"                                               },
        {"End" , "crydetectcfg -act set -enable 1 -thresh 50 -timestrategy"
                 " 0:0,1:0,2:0,3:0,4:0,5:0,6:0"},
    };


    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL   , NULL               , 0                         },
        {"act"         , ARG_TYPE_ACT  , NULL   , action             , sizeof(action)            },
        {"enable"      , ArgTypesInt   , "0|1"  , &outer.enable      , sizeof(outer.enable)      },
        {"thresh"      , ArgTypesInt   , "0~100", &outer.thresh      , sizeof(outer.thresh)      },
        {"level"       , ArgTypesInt   , "0~3"  , &outer.level       , sizeof(outer.level)       },
        {"timestrategy", ArgTypesString, NULL   , timestrategy       , sizeof(timestrategy)      },
        {"End"         , ArgTypesEnd   , NULL   , NULL               , 0                         },
    };

    ret = get_config(handleCryDetectCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(CryDetectionS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.times);

        JCP_RETURN_SUCC_IF_MEM_EQ(&inner, &outer, sizeof(CryDetectionS));
        ret = set_config(handleCryDetectCfg, outer);
    }

    //分析字符串
    return SUCCESS;
}

static int JCPCmdAlarmAudioTypeCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    alarm_audio_t inner;
    alarm_audio_t outer;
    memset(&inner, 0, sizeof(alarm_audio_t));
    memset(&outer, 0, sizeof(alarm_audio_t));

    MotionDetectLinkS inner_motionDetect = {0};
    MotionDetectLinkS outer_motionDetect = {0};
    VglineLinkS inner_vgline = {0};
    VglineLinkS outer_vgline = {0};
    VgrectLinkS inner_vgrect = {0};
    VgrectLinkS outer_vgrect = {0};
    HumanDetectLinkS inner_humanDetect = {0};
    HumanDetectLinkS outer_humanDetect = {0};

    HelpMsgS helps[] = {
        {"?"     , "移动报警音频输出"                   },
        {"act"   , "list:获取所有参数 set:设置参数"     },
        {"alarm_type" , "报警声音类型 1 报警声音  2 狗叫声  3 自定义声音"               },
        {"time"  , "报警声音持续时间"},
        {"End"   , "alarmaudio -?获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK, NULL      , NULL                , 0             },
        {"act"          , ARG_TYPE_ACT, "list|set", action              , sizeof(action)},
        {"alarm_type"   , ArgTypesInt , "0~10"    , &outer.audio_type   , sizeof(int)   },
        {"time"         , ArgTypesInt , "0~60"    , &outer.time         , sizeof(int)   },
        {"End"          , ArgTypesEnd , NULL      , NULL                , 0             },
    };

    ret = get_config(handleAlarmAudioTypeCfg, inner);

    RETURN_FAIL_IF_API_ERR(ret);

    ret = get_config(handleMotionDetLinkCfg, inner_motionDetect);
    RETURN_FAIL_IF_API_ERR(ret);

    ret = get_config(handleVglineLinkCfg, inner_vgline);
    RETURN_FAIL_IF_API_ERR(ret);

    ret = get_config(handleVgrectLinkCfg, inner_vgrect);
    RETURN_FAIL_IF_API_ERR(ret);

    ret = get_config(handleHumanDetLinkCfg, inner_humanDetect);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(int));
    memcpy(&outer_motionDetect, &inner_motionDetect, sizeof(MotionDetectLinkS));
    memcpy(&outer_vgline, &inner_vgline, sizeof(VglineLinkS));
    memcpy(&outer_vgrect, &inner_vgrect, sizeof(VgrectLinkS));
    memcpy(&outer_humanDetect, &inner_humanDetect, sizeof(HumanDetectLinkS));

    JCP_ARG_PARSER(NULL, 0);

    int soundsel = outer.audio_type - 1;
        outer_motionDetect.soundsel = soundsel;
        outer_vgline.soundsel = soundsel;
        outer_vgrect.soundsel = soundsel;
        outer_humanDetect.soundsel = soundsel;

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(int));
        ret = set_config(handleAlarmAudioTypeCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        ret = set_config(handleMotionDetLinkCfg, outer_motionDetect);
        RETURN_FAIL_IF_API_ERR(ret);
        ret = set_config(handleVglineLinkCfg, outer_vgline);
        RETURN_FAIL_IF_API_ERR(ret);
        ret = set_config(handleVgrectLinkCfg, outer_vgrect);
        RETURN_FAIL_IF_API_ERR(ret);
        ret = set_config(handleHumanDetLinkCfg, outer_humanDetect);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return ret;
}

int JCPCmdAlarmManageCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    //mdmbcfg
    MotionDetectS inner_mdmbcfg = {0,};
    MotionDetectS outer_mdmbcfg = {0,};
    char timestrategy_mdmbcfg[128] = {0};
    //vgrect
    VgrectS inner_vgrect = {0,};
    VgrectS outer_vgrect = {0,};
    char timestrategy_vgrect[128] = {0};
    //vgline
    VglineS inner_vgline = {0,};
    VglineS outer_vgline = {0,};
    char timestrategy_vgline[128] = {0};
    //humandetectcfg
    HumanDetectionS inner_hd = {0,};
    HumanDetectionS outer_hd = {0,};
    char timestrategy_hd[128] = {0};

    //humandetectcfg
    CarDetectionS inner_car = {0,};
    CarDetectionS outer_car = {0,};
    char timestrategy_car[128] = {0};

    //alarmaudio
    alarm_audio_t inner_aaudio = {0,};
    alarm_audio_t outer_aaudio = {0,};

    HelpMsgS helps[] = {
        {"?"     , "报警管理"                 },
        {"act"   , "list:获取所有参数"          },
        {"End"   , "获取移动侦测，区域侦测，越界侦测，人性侦测，报警声音参数"          },
    };

    ArgOptS_Expand opts[] = {
        {"?"           , "?"          , ARG_TYPE_ASK  , NULL  , NULL         , 0                   },
        {"act"         , "act"        , ARG_TYPE_ACT  , NULL  , action       , sizeof(action)      },

        //mdmbcfg
        {"mdmbcfg"     , "enable"      , ArgTypesInt   , "0|1" , &outer_mdmbcfg.enable, sizeof(outer_mdmbcfg.enable)},
        {"mdmbcfg"     , "thresh"      , ArgTypesInt   , "0~100", &outer_mdmbcfg.thresh, sizeof(outer_mdmbcfg.thresh)},
        {"mdmbcfg"     , "level"       , ArgTypesInt   , "0~3"  , &outer_mdmbcfg.level , sizeof(outer_mdmbcfg.level) },
        {"mdmbcfg"     , "mbdesc"      , ArgTypesString, NULL   , outer_mdmbcfg.mbdesc , sizeof(outer_mdmbcfg.mbdesc)},
        {"mdmbcfg"     , "timestrategy", ArgTypesString, NULL   , timestrategy_mdmbcfg , sizeof(timestrategy_mdmbcfg)},

        //vgrect
        {"vgrect"      , "x0"          , ArgTypesInt   , "0~1920"  , &outer_vgrect.x0    , sizeof(outer_vgrect.x0)    },
        {"vgrect"      , "y0"          , ArgTypesInt   , "0~1080"  , &outer_vgrect.y0    , sizeof(outer_vgrect.y0)    },
        {"vgrect"      , "x1"          , ArgTypesInt   , "0~1920"  , &outer_vgrect.x1    , sizeof(outer_vgrect.x1)    },
        {"vgrect"      , "y1"          , ArgTypesInt   , "0~1080"  , &outer_vgrect.y1    , sizeof(outer_vgrect.y1)    },
        {"vgrect"      , "x2"          , ArgTypesInt   , "0~1920"  , &outer_vgrect.x2    , sizeof(outer_vgrect.x2)    },
        {"vgrect"      , "y2"          , ArgTypesInt   , "0~1080"  , &outer_vgrect.y2    , sizeof(outer_vgrect.y2)    },
        {"vgrect"      , "x3"          , ArgTypesInt   , "0~1920"  , &outer_vgrect.x3    , sizeof(outer_vgrect.x3)    },
        {"vgrect"      , "y3"          , ArgTypesInt   , "0~1080"  , &outer_vgrect.y3    , sizeof(outer_vgrect.y3)    },
        {"vgrect"      , "enable"      , ArgTypesInt   , "0|1"     , &outer_vgrect.enable, sizeof(outer_vgrect.enable)},
        {"vgrect"      , "blink"       , ArgTypesInt   , "0|1"     , &outer_vgrect.blink , sizeof(outer_vgrect.blink) },
        {"vgrect"      , "indoor"      , ArgTypesInt   , "0|1"     , &outer_vgrect.indoor, sizeof(outer_vgrect.indoor)},
        {"vgrect"      , "dir"         , ArgTypesInt   , "0~2"     , &outer_vgrect.dir   , sizeof(outer_vgrect.dir   )},
        {"vgrect"      , "thresh"      , ArgTypesInt   , "0~100"   , &outer_vgrect.thresh, sizeof(outer_vgrect.thresh)},
        {"vgrect"      , "level"       , ArgTypesInt   , "0~3"     , &outer_vgrect.level , sizeof(outer_vgrect.level) },
        {"vgrect"      , "timestrategy", ArgTypesString, NULL      , timestrategy_vgrect , sizeof(timestrategy_vgrect)},

        //vgline
        {"vgline"      , "x0"          , ArgTypesInt   , "0~1920"  , &outer_vgline.x0    , sizeof(outer_vgline.x0)    },
        {"vgline"      , "y0"          , ArgTypesInt   , "0~1080"  , &outer_vgline.y0    , sizeof(outer_vgline.y0)    },
        {"vgline"      , "x1"          , ArgTypesInt   , "0~1920"  , &outer_vgline.x1    , sizeof(outer_vgline.x1)    },
        {"vgline"      , "y1"          , ArgTypesInt   , "0~1080"  , &outer_vgline.y1    , sizeof(outer_vgline.y1)    },
        {"vgline"      , "dx0"         , ArgTypesInt   , "0~1920"  , &outer_vgline.dx0   , sizeof(outer_vgline.dx0)   },
        {"vgline"      , "dy0"         , ArgTypesInt   , "0~1080"  , &outer_vgline.dy0   , sizeof(outer_vgline.dy0)   },
        {"vgline"      , "dx1"         , ArgTypesInt   , "0~1920"  , &outer_vgline.dx1   , sizeof(outer_vgline.dx1)   },
        {"vgline"      , "dy1"         , ArgTypesInt   , "0~1080"  , &outer_vgline.dy1   , sizeof(outer_vgline.dy1)   },
        {"vgline"      , "enable"      , ArgTypesInt   , "0|1"     , &outer_vgline.enable, sizeof(outer_vgline.enable)},
        {"vgline"      , "blink"       , ArgTypesInt   , "0|1"     , &outer_vgline.blink , sizeof(outer_vgline.blink) },
        {"vgline"      , "indoor"      , ArgTypesInt   , "0|1"     , &outer_vgline.indoor, sizeof(outer_vgline.indoor)},
        {"vgline"      , "thresh"      , ArgTypesInt   , "0~100"   , &outer_vgline.thresh, sizeof(outer_vgline.thresh)},
        {"vgline"      , "level"       , ArgTypesInt   , "0~3"     , &outer_vgline.level , sizeof(outer_vgline.level) },
        {"vgline"      , "timestrategy", ArgTypesString, NULL      , timestrategy_vgline , sizeof(timestrategy_vgline)},

        //humandetectcfg
        {"humandetectcfg" , "enable"       , ArgTypesInt   , "0|1"  , &outer_hd.enable       , sizeof(outer_hd.enable)      },
        {"humandetectcfg" , "screenenable" , ArgTypesInt   , "0|1"  , &outer_hd.screenenable , sizeof(outer_hd.screenenable)},
        {"humandetectcfg" , "thresh"       , ArgTypesInt   , "0~100", &outer_hd.thresh       , sizeof(outer_hd.thresh)      },
        {"humandetectcfg" , "level"        , ArgTypesInt   , "0~3"  , &outer_hd.level         , sizeof(outer_hd.level)         },
        {"humandetectcfg" , "humandistance", ArgTypesInt   , "0~100", &outer_hd.humandistance, sizeof(outer_hd.humandistance) },
        {"humandetectcfg" , "drag"         , ArgTypesInt   , "0|1"  , &outer_hd.drag         , sizeof(outer_hd.drag)        },
        {"humandetectcfg" , "mbdesc"       , ArgTypesString, NULL   , outer_hd.mbdesc        , sizeof(outer_hd.mbdesc)      },
        {"humandetectcfg" , "timestrategy" , ArgTypesString, NULL   , timestrategy_hd        , sizeof(timestrategy_hd)      },
        {"humandetectcfg" , "outdoor"      , ArgTypesInt   , "0~2"  , &outer_hd.mode         , sizeof(outer_hd.mode)        },
        {"humandetectcfg" , "person_center", ArgTypesInt   , "0|1"  , &outer_hd.person_center, sizeof(outer_hd.person_center)},

        //cardetectcfg
        {"cardetectcfg" , "enable"       , ArgTypesInt   , "0|1"  , &outer_car.enable       , sizeof(outer_car.enable)      },
        {"cardetectcfg" , "show"         , ArgTypesInt   , "0|1|2", &outer_car.show         , sizeof(outer_car.show)        },
        {"cardetectcfg" , "screenenable" , ArgTypesInt   , "0|1"  , &outer_car.screenenable , sizeof(outer_car.screenenable)},
        {"cardetectcfg" , "thresh"       , ArgTypesInt   , "0~100", &outer_car.thresh       , sizeof(outer_car.thresh)      },
        {"cardetectcfg" , "level"        , ArgTypesInt   , "0~3"  , &outer_car.level        , sizeof(outer_car.level)       },
        {"cardetectcfg" , "cardistance"  , ArgTypesInt   , "0~100", &outer_car.cardistance  , sizeof(outer_car.cardistance) },
        {"cardetectcfg" , "drag"         , ArgTypesInt   , "0|1"  , &outer_car.drag         , sizeof(outer_car.drag)        },
        {"cardetectcfg" , "mbdesc"       , ArgTypesString, NULL   , outer_car.mbdesc        , sizeof(outer_car.mbdesc)      },
        {"cardetectcfg" , "timestrategy" , ArgTypesString, NULL   , timestrategy_car        , sizeof(timestrategy_car)      },
        {"cardetectcfg" , "outdoor"      , ArgTypesInt   , "0~2"  , &outer_car.mode         , sizeof(outer_car.mode)        },

        //alarmaudio
        {"alarmaudio"   , "alarm_type"      , ArgTypesInt , "0~10"    , &outer_aaudio.audio_type   , sizeof(int)   },
        {"alarmaudio"   , "time"            , ArgTypesInt , "0~60"    , &outer_aaudio.time          , sizeof(int)   },

        {"End"         , "End"            , ArgTypesEnd   , NULL   , NULL         , 0                   },
    };

    JCP_GET_NODE_VALUE_API(handleMotionDetectCfg,inner_mdmbcfg,outer_mdmbcfg);
    JCP_GET_NODE_VALUE_API(handleVgrectCfg,inner_vgrect,outer_vgrect);
    JCP_GET_NODE_VALUE_API(handleVglineCfg,inner_vgline,outer_vgline);
    JCP_GET_NODE_VALUE_API(handleHumanDetectCfg,inner_hd,outer_hd);
    JCP_GET_NODE_VALUE_API(handleCarDetectCfg,inner_car,outer_car);
    JCP_GET_NODE_VALUE_API(handleAlarmAudioTypeCfg,inner_aaudio,outer_aaudio);

    JCP_ARG_PARSER_EX(NULL, 0);
    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK_EX(opts);

        intarray_to_timestr(timestrategy_mdmbcfg, inner_mdmbcfg.times);
        intarray_to_timestr(timestrategy_vgrect, inner_vgrect.times);
        intarray_to_timestr(timestrategy_vgline, inner_vgline.times);
        intarray_to_timestr(timestrategy_hd, inner_hd.times);
        intarray_to_timestr(timestrategy_car, inner_car.times);

        ASMJCP_LIST_STRING_EX(buf, buflen, opts);
    }

    //分析字符串
    return SUCCESS;
}

static int JCPCmdCapability(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    SysCustomS inner = {0};
    SysCustomS outer = {0};


    HelpMsgS helps[] = {
        {"?"        , "拓展配置项"                    },
        {"act"      , "list:获取所有参数 set:设置参数" },
        {"wiper"    , "是否有雨刷"                     },
        {"ircut"    , "是否有红外切换器"               },
        {"irlight"  , "是否有红外灯"                 },
        {"ircolor"  , "彩转黑"                         },
        {"alarmhost", "电子报警主机"                   },
        {"gps"      , "是否有GPS模块"                  },
        {"ptz"      , "是否有ptz模块"                  },
        {"alarmin"  , "是否带报警输入,0:没有，其他值代表多少路"},
        {"alarmout" , "是否带报警输出,0:没有，其他值代表多少路"},
        {"graintype", "表示grain设备类型，0:标准配置，1:全功能型"},
        {"webdeflang", "0: 中英文默认中文，1: 纯英文，2: 俄英文默认俄文" },
        {"pixels"   , "240: 2M TO 2.4M,300: 2M TO 3M" },
        {"follow"   , "是否有移动追踪模块"                  },
        {"sdinfo"   , "0: 默认显示，1: 隐藏，2: 显示写入状态"              },
        {"osd"      , "0: 不支持，1: 通用 osd，2: 新版 osd"         },
        {"End"      , "capability -?获取帮助"       },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK, NULL      , NULL            , 0             },
        {"act"          , ARG_TYPE_ACT, "list|set", action          , sizeof(action)},
        {"wiper"        , ArgTypesInt , "0|1"     , &outer.wiper    , sizeof(int)   },
        {"ircut"        , ArgTypesInt , "0|1"     , &outer.ircut    , sizeof(int)   },
        {"irlight"      , ArgTypesInt , "0|1"     , &outer.irlight  , sizeof(int)   },
        {"ircolor"      , ArgTypesInt , "0|1"     , &outer.ircolor  , sizeof(int)   },
        {"alarmhost"    , ArgTypesInt , "0|1"     , &outer.alarmhost, sizeof(int)   },
        {"gps"          , ArgTypesInt , "0|1"     , &outer.gps      , sizeof(int)   },
        {"ptz"          , ArgTypesInt , "0|1"     , &outer.ptz      , sizeof(int)   },
        {"alarmin"      , ArgTypesInt , "0~4"     , &outer.alarmin  , sizeof(int)   },
        {"alarmout"     , ArgTypesInt , "0~2"     , &outer.alarmout , sizeof(int)   },
        {"graintype"    , ArgTypesInt , "0~1"     , &outer.graintype  , sizeof(int) },
        {"webdeflang"   , ArgTypesInt , "0~2"     , &outer.webdeflang , sizeof(int) },
        {"pixels"       , ArgTypesInt , "240~300" , &outer.pixels   , sizeof(int)   },
        {"follow"       , ArgTypesInt , "0|1"     , &outer.follow   , sizeof(int)   },
        {"sdinfo"       , ArgTypesInt , "0~10"    , &outer.sdinfo   , sizeof(int)   },
        {"osd"          , ArgTypesInt , "0~2"     , &outer.osd      , sizeof(int)   },
        {"End"          , ArgTypesEnd , NULL      , NULL            , 0             },
    };

    ret = get_config(handleCapability, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(SysCustomS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(SysCustomS));
        ret = set_config(handleCapability, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.follow, outer.follow, "1 /cfg/sysCustomize/follow %d\n", outer.follow);
            CUSTOMCONF_APPEND_INT(inner.sdinfo, outer.sdinfo, "1 /cfg/sysCustomize/sdinfo %d\n", outer.sdinfo);
            CUSTOMCONF_APPEND_INT(inner.osd, outer.osd, "1 /cfg/sysCustomize/osd %d\n", outer.osd);
        }
    }

    return SUCCESS;

}

static int asm_board_string(char *szMsgBuf, ArgOptS opts[], int sz_opts, HelpMsgS maps[], int sz_maps)
{
    int i = 0;
    char buf[16] = {0};
    if(NULL == szMsgBuf) {
        sprintf(szMsgBuf, "MSGBUF = NULL\n");
        return -1;
    }

    if(sz_opts != sz_maps) {
        sprintf(szMsgBuf, "data size error!\n");
        return -1;
    }

    for(i = 0; i < sz_opts; i++) {
        if(strncasecmp(opts[i].pOpt, maps[i].pOpt,128) != 0) {
            sprintf(szMsgBuf, "opt[%s] not same!\n", opts[i].pOpt);
            return -1;
        }
    }
    for(i = 2; opts[i].argType< ArgTypesEnd; i++) {
        sprintf(buf, "%s=%d;", opts[i].pOpt, *(int *)(opts[i].pSetValue));
        strcat(szMsgBuf, buf);
    }

    return 0;
}


static int JCPCmdShowWeb(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    ShowWebS weblist = {0};

    HelpMsgS helps[] = {
        {"?"          , "网页识别项"                    },
        {"act"        , "list:获取所有参数 set:设置参数"},
        {"alarmevent" , "定时获取alarm event"           },
        {"blackmargin", "黑边"                          },
        {"colorbase"  , "基础色彩调节页面"              },
        {"hue"        , "灰度调节"                      },
        {"gain"       , "增益"                          },
        {"dynamic"    , "宽动态界面"                    },
        {"sharpness"  , "锐度"                          },
        {"ae_aw_blc"  , "白平衡"                        },
        {"position_3D", "3D功能"                        },
        {"ptz_ctrl"   , "云台控制"                      },
        {"follow"     , "移动跟踪"                      },
        {"customtype" , "客户类型"                      },
        {"new_pelco"  , "新老单片机"                    },
        {"rain"       , "雨刷"                          },
        {"irlight"    , "红外光敏"                      },
        {"irmode"     , "红外设置"                      },
        {"shelter"    , "隐私遮挡"                      },
        {"MCUupgrade" , "单片机升级"                    },
        {"G3"         , "3G模块"                        },
        {"dome"       , "球机"                          },
        {"alarmin"    , "设备是否支持输入报警 大于0表示支持多少路"},
        {"alarmout"   , "设备是否支持输出报警 大于0表示支持多少路"},
        {"audio"      , "0: 无音频，大于0: 有音频"      },
        {"graintype"  , "0:标配， 1: 全功能型"       },
        {"webdeflang" , "0: 中英文默认中文，1: 纯英文，2: 俄英文默认俄文" },
        {"hdetect"    , "humandetect: 1，使能；0，禁用"           },
        {"__4g"       , "4g"                    },
        {"wifi"       , "wifi"                  },
        {"End"        , "返回值为1或者大于1，都表示该设置有这功能"},
    };

    ArgOptS opts[] =
    {
        {"?"          , ARG_TYPE_ASK, NULL      , NULL                , 0             },
        {"act"        , ARG_TYPE_ACT, "set|list", action              , sizeof(action)},
        {"alarmevent" , ArgTypesInt , "0|1"     , &weblist.alarmevent , sizeof(int)   },
        {"blackmargin", ArgTypesInt , "0|1"     , &weblist.blackmargin, sizeof(int)   },
        {"colorbase"  , ArgTypesInt , "0|1"     , &weblist.colorbase  , sizeof(int)   },
        {"hue"        , ArgTypesInt , "0|1"     , &weblist.hue        , sizeof(int)   },
        {"gain"       , ArgTypesInt , "0|1"     , &weblist.gain       , sizeof(int)   },
        {"dynamic"    , ArgTypesInt , "0|1"     , &weblist.dynamic    , sizeof(int)   },
        {"sharpness"  , ArgTypesInt , "0|1"     , &weblist.sharpness  , sizeof(int)   },
        {"ae_aw_blc"  , ArgTypesInt , "0|1"     , &weblist.ae_aw_blc  , sizeof(int)   },
        {"position_3D", ArgTypesInt , "0|1"     , &weblist.position_3D, sizeof(int)   },
        {"ptz_ctrl"   , ArgTypesInt , "0|1"     , &weblist.ptz_ctrl   , sizeof(int)   },
        {"follow"     , ArgTypesInt , "0|1"     , &weblist.follow     , sizeof(int)   },
        {"customtype" , ArgTypesInt , "1~9"     , &weblist.customtype , sizeof(int)   },
        {"new_pelco"  , ArgTypesInt , "0|1"     , &weblist.new_pelco  , sizeof(int)   },
        {"rain"       , ArgTypesInt , "0|1"     , &weblist.rain       , sizeof(int)   },
        {"irlight"    , ArgTypesInt , "0|1"     , &weblist.irlight    , sizeof(int)   },
        {"irmode"     , ArgTypesInt , "0|1"     , &weblist.irmode     , sizeof(int)   },
        {"shelter"    , ArgTypesInt , "0~24"    , &weblist.shelter    , sizeof(int)   },
        {"MCUupgrade" , ArgTypesInt , "0|1"     , &weblist.MCUupgrade , sizeof(int)   },
        {"G3"         , ArgTypesInt , "0|1"     , &weblist.G3         , sizeof(int)   },
        {"dome"       , ArgTypesInt , "0|1"     , &weblist.dome       , sizeof(int)   },
        {"alarmin"    , ArgTypesInt , "0~4"     , &weblist.alarmin    , sizeof(int)   },
        {"alarmout"   , ArgTypesInt , "0~2"     , &weblist.alarmout   , sizeof(int)   },
        {"audio"      , ArgTypesInt , "0~2"     , &weblist.audio      , sizeof(int)   },
        {"graintype"  , ArgTypesInt , "0~1"     , &weblist.graintype  , sizeof(int)   },
        {"webdeflang" , ArgTypesInt , "0~2"     , &weblist.webdeflang , sizeof(int)   },
        {"hdetect"    , ArgTypesInt , "0|1"     , &weblist.hdetect    , sizeof(int)   },
        {"__4g"       , ArgTypesInt , "0|1"     , &weblist.__4g       , sizeof(int)   },
        {"wifi"       , ArgTypesInt , "0|1"     , &weblist.wifi       , sizeof(int)   },
        {"End"        , ArgTypesEnd , NULL      , 0                   , sizeof(int)   },
    };

    get_capability(&weblist);

    JCP_ARG_PARSER(NULL, 0);
    if(!strncasecmp("list", action,strlen("list")))
    {
        return asm_board_string((char *)buf, opts, ARRAY_SIZE(opts), helps, ARRAY_SIZE(helps));
    }

    return SUCCESS;
}


int JCPCmdPtzSerialCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    PtzSerialS inner = {0};
    PtzSerialS outer = {0};
    HelpMsgS helps[] = {
        {"?"       , "ptz串口（RS232 RS485）参数设置"},
        {"act"     , "list:获取所有参数 set:设置参数"},
        {"baud"    , "波特率"},
        {"data"    , "数据位位数"},
        {"stop"    , "停止位位数"},
        {"parity"  , "N|n is no parity, O|o is odd, E|e is even, S|s is stop"},
        {"addr"    , "地址"},
        {"protocol", "球机当前协议，范围见protocollist"},
        {"protocollist", "球机当前协议，"},
        {"hreverse", "水平方向PTZ控制命令反转"},
        {"vreverse", "垂直方向PTZ控制命令反转"},
        {"End"     , "ptz_serial -?获取帮助"},
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK    , NULL                   , NULL                  , 0                     },
        {"act"         , ARG_TYPE_ACT    , "list|set"             , action         , sizeof(action)        },
        {"baud"        , ArgTypesInt     , "300~115200"          , &outer.baud    , sizeof(int)           },
        {"data"        , ArgTypesInt     , "5|6|7|8"              , &outer.databits, sizeof(int)           },
        {"stop"        , ArgTypesInt     , "1|2"                  , &outer.stop    , sizeof(int)           },
        {"parity"      , ArgTypesString  , "N|O|E|S"              , outer.parity   , sizeof(outer.parity)  },
        {"addr"        , ArgTypesInt     , "1~255"                , &outer.addr    , sizeof(int)           },
        {"protocol"    , ArgTypesString  , NULL                   , outer.protocol , sizeof(outer.protocol)},
        {"protocollist", ARG_TYPE_LISTS  , NULL                   , outer.protocollist, sizeof(outer.protocollist)},
        {"hreverse"    , ArgTypesInt     , "0|1"                  , &outer.hreverse, sizeof(int)           },
        {"vreverse"    , ArgTypesInt     , "0|1"                  , &outer.vreverse, sizeof(int)           },
        {"End"         , ArgTypesEnd     , NULL                   , NULL                  , 0                     },
    };

    ret = get_config(handlePtzSerialCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(PtzSerialS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(PtzSerialS));
        ret = set_config(handlePtzSerialCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.baud    , outer.baud    , "1 /cfg/ptzSerial/baud %d\n"     , inner.baud    );
            CUSTOMCONF_APPEND_INT(inner.databits, outer.databits, "1 /cfg/ptzSerial/databits %d\n" , inner.databits);
            CUSTOMCONF_APPEND_INT(inner.stop    , outer.stop    , "1 /cfg/ptzSerial/stop %d\n"     , inner.stop    );
            CUSTOMCONF_APPEND_INT(inner.addr    , outer.addr    , "1 /cfg/ptzSerial/addr %d\n"     , inner.addr    );
            CUSTOMCONF_APPEND_STR(inner.protocol, outer.protocol, "1 /cfg/ptzSerial/protocol %s\n" , inner.protocol);
            CUSTOMCONF_APPEND_STR(inner.parity  , outer.parity  , "1 /cfg/ptzSerial/parity %s\n"   , inner.parity  );
        }
    }
    return SUCCESS;

}

//#ifdef __migrate_ball_over__
static int JCPCmdPelcod20ctrl(char *buf, int buflen, int argc, char **argv)
{
    char time_str[64] = {0};
    static unsigned long long last_timestamp = 0;
    unsigned long long cur_timestamp = 0;

    char scope_type[8] = {0};
    char scope_cmd[8] = "1~255";
    const char *str_help = "Usage@%d: pelcod20ctrl -? \n"
                     "       pelcod20ctrl -type %s -? \n"
                     "       pelcod20ctrl -type xx -cmd yy -? \n"
                     "       pelcod20ctrl -type xx -cmd yy ... \n";

    PelcoCmd outer = {0,};
#if 0
    HelpMsgS helps[] = {
        {"?"       , "pelcod 参数获取"},
        {"type"    , "命令类型"},
        {"cmd"     , "命令号"},
        {"data1"   , "数据位1"},
        {"data2"   , "数据位2"},
        {"packet"  , "包数据"},
        {"timestamp"  , "时间戳，精确到 ms"},
        {"End"     , "pelcod20ctrl -?获取帮助"},
    };
#endif
    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK    , NULL          , NULL       ,  0                     },
        {"type"   , ARG_TYPE_MUSTINT, scope_type    , &outer.type,  sizeof(int)           },
        {"cmd"    , ArgTypesInt     , scope_cmd     , &outer.cmd,   sizeof(int)           },
        {"data1"  , ArgTypesInt     , NULL          , &outer.data1, sizeof(int)           },
        {"data2"  , ArgTypesInt     , NULL          , &outer.data2, sizeof(int)           },
        {"packet" , ArgTypesString  , NULL          , outer.packet, sizeof(outer.packet)  },
        {"timestamp", ArgTypesString, NULL          , time_str    , sizeof(time_str)       },
        {"END"    , ArgTypesEnd     , NULL          , &outer.cmd,   sizeof(int)           },
    };

    snprintf(scope_type, sizeof(scope_type), "1~%d", 3);
    if (SUCCESS != parser_jcp_arg(argc, argv, opts, buf, NULL, 0)) {
        sprintf((char *)buf, str_help, __LINE__, scope_type);
        DBG("parser_jcp_arg fail\n");
        return FAILURE;
    }

    // 隐私遮挡开启时
    videomask_plan_t videomask = {0,};
    conf_get_videomaskplan_cfg(&videomask);
    if (videomask.mask_enable) {
        DBG("skip pelcod20ctrl\n");
        return SUCCESS;
    }

    cur_timestamp = atoll(time_str);
    if (cur_timestamp > 0) {
        if (cur_timestamp < last_timestamp && last_timestamp - cur_timestamp < 30 * 1000) {
            SYSLOG(" cur:%llu last:%llu the command sequence is incorrect, ignore.\n", cur_timestamp, last_timestamp);
            return SUCCESS;
        }
        last_timestamp = cur_timestamp;
    }

   outer.timestamp = mono_msec();

   if (outer.type == 3) { /*执行一些不能下 pelco 指令的查询，例如巡航*/
       if (outer.cmd == 1) {
           if (outer.data2 == 1) {
               // 巡航状态查询
               sprintf(buf, "seq=%d;seq_time_left=%lld;pan_scan=%d;tilt_scan=%d;", sequence_is_enable(), sequence_time_left(), pan_scan_is_enable(), tilt_scan_is_enable());
               return SUCCESS; // 任何Pelcod指令都会打断巡航, 所以查询状态后直接返回
           }
       }
   }

    peclo_cmd_enqueue(&outer);
   /*等待指令执行结束，然后返回执行结果*/
   int ret = SUCCESS;
   if (outer.type == 2) {
       sync_presetcfg(outer.cmd, outer.data2, outer.packet); /*同步预置位到 xml*/

       if (outer.cmd == 2) {
           if (outer.data2 == PRESET_START_SEQUENCE) { // 开始巡航
               /*云端超时是 3 秒，这里最多等待两秒*/
               ret = get_pelco_response(outer.timestamp, buf, buflen, 2 * 1000);
           }
       }
   } else if (outer.type == 3) {
       if (outer.cmd == 1) {
           if (outer.data2 == 10) {
               // 水平扫描，需要返回运行时间，目前计算不出准确时间，给个大概的时间
               sprintf(buf, "horizontal_scan=%d;vertical_scan=%d;", 20 * 1000, 25 * 1000);
           } else if (outer.data2 == 12) {
               // 垂直扫描，需要返回运行时间，目前计算不出准确时间，给个大概的时间
               sprintf(buf, "horizontal_scan=%d;vertical_scan=%d;", 20 * 1000, 25 * 1000);
           } else if (outer.data2 == 0) {
               /*开始巡航*/
               ret = get_pelco_response(outer.timestamp, buf, buflen, 2 * 1000);
           }
       }
   }

   return ret;
}

static int JCPCmdPelcod20Cfg(char *buf, int buflen, int argc, char **argv)
{
#if 0
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    PelcodCfg outer = {0,};
    PelcodCfg inner = {0,};

    HelpMsgS helps[] = {
        {"?"        , "pelcod 位置获取"                                 },
        {"act"      , "list:获取所有参数 set:设置参数"                  },
        {"x"        , "左右位置 右加 左减"                              },
        {"y"        , "上下位置 上加 下减"                              },
        {"revert"   , "摇头镜像 0:正常 1:垂直镜像 2:左右镜像 3:对角镜像"},
        {"End"      , ""                                                },
    };

    ArgOptS opts[] = {
        {"?"          , ARG_TYPE_ASK    , NULL          , NULL          , 0             },
        {"act"        , ARG_TYPE_ACT    , "list|set"    , action        , sizeof(action)},
        {"x"          , ArgTypesInt     , x_scope()     , &outer.x      , sizeof(int)   },
        {"y"          , ArgTypesInt     , y_scope()     , &outer.y      , sizeof(int)   },
        {"revert"     , ArgTypesInt     , "0~4"         , &outer.revert , sizeof(int)   },
        {"END"        , ArgTypesEnd     , NULL          , NULL          , 0          },
    };

    ret = get_config(handlePelcodCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(PelcodCfg));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(PelcodCfg));
        ret = set_config(handlePelcodCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
#endif
    return SUCCESS;

}
//#endif

//成功 : group id/SUCCESS; -1 : FAILURE; -2 : 用户已经存在
//-3 : 用户不存在 -4 : 用户数目已上限 -5 : 密码错误
//
int JCPCmdCheckUser(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    char username[64] = {0};
    char password[64] = {0};

    HelpMsgS helps[] = {
        {"?"       , "用户核查"},
        {"act"     , "set:设置参数"},
        {"user"    , "验证的用户名"},
        {"password", "验证的密码"},
        {"End"     , "使用命令之前先要调用authmode -act list 获取认证模式，参考命令authmod\r\n"
        "      　　1.如果用户验证模式是basic 则直接输入明文用户名和密码, checkuser -act set -user admin -password admin\r\n"
        "          2.如果用户验证模式时digest，数字验证模式，先根据authmode 命令的返回值realm,"
        "通过函数md5(username:realm:password);计算出密码，然后再发送checkuser -act set -user admin -password +密码 ， 进行验证。\r\n"
        "          3.设置成功，result的返回值大于或等于0, 0：admin权限， 1：operator权限  2：user权限\r\n"
        "          4.设置失败，result返回值小于0, 　　-3：用户名不存在，　　-5：用户命名错误"},
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK  , NULL , NULL    , 0               },
        {"act"     , ARG_TYPE_ACT  , "set", action  , sizeof(action)  },
        {"user"    , ArgTypesString, NULL , username, sizeof(username)},
        {"password", ArgTypesString, NULL , password, sizeof(password)},
        {"End"     , ArgTypesEnd   , NULL , NULL    , 0               },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        AuthtypeE mode = NONE_AUTH;
        ret = get_config(handleAuthModecfg, mode);

        if (BASIC_AUTH == mode) {
            ret = user_basic_auth(username, password);
        } else if (DIGEST_AUTH == mode) {
            SysUserS suser = {0};
            ret = get_config(handleUserCfg, suser);
            if (ret < 0) {
                sprintf(buf, "%s", "get usercfg fail\n");
                return -1;
            }
            int i = 0;
            for (i = 0; i < USER_MAX_NUM; i++) {
                if (strcmp(username, suser.user[i].username) == 0) {
                    break;
                }
            }

            if(i >= USER_MAX_NUM)
            {
                sprintf(buf, "result=%d", -3);
                return -3;
            }


            if (strcmp(password, suser.user[i].digestpasswd) != 0) {
                sprintf(buf, "result=%d", -5);
                return -5;
            }

            ret = suser.user[i].group;
        }

        sprintf(buf, "result=%d", ret);
        DBG("%s\n", buf);
    }

    return ret >=0 ? SUCCESS : FAILURE;

}

int JCPCmddenoiseCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};
    DnrCfgS outer = {0};
    DnrCfgS inner = {0};

    HelpMsgS helps[] = {
        {"?"     , "降噪设置"                      },
        {"act"   , "list:获取所有参数 set:设置参数"},
        {"enable", "是否开启 1 -> enable 0 -> 0%"  },
        {"mode"  , "strength强度"                  },
        {"End"   , "默认：enable=1 mode=50"       },
    };

    ArgOptS opts[] =
    {
        {"?"     , ARG_TYPE_ASK, NULL      , NULL         , 0             },
        {"act"   , ARG_TYPE_ACT, "list|set", action       , sizeof(action)},
        {"enable", ArgTypesInt , "0|1"     , &outer.enable, sizeof(int)   },
        {"mode"  , ArgTypesInt , "0~100"   , &outer.mode  , sizeof(int)   },
        {"End"   , ArgTypesEnd , NULL      , NULL         , 0             },
    };
    ret = get_config(handleDenoisecfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(DnrCfgS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(DnrCfgS));
        ret = set_config(handleDenoisecfg, outer);
    }
    return SUCCESS;

}


int JCPCmdAuthenModeCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};
    AuthtypeE outer = NONE_AUTH;
    AuthtypeE inner = NONE_AUTH;

    HelpMsgS helps[] = {
        {"?"       , "验证方式"},
        {"act"     , "list:获取所有参数 set:设置参数"},
        {"mode"    , "0:不验证  1:basic  2: digest"},
        {"End"     , "authmode -?获取帮助"},
    };

    ArgOptS opts[] =
    {
        {"?"   , ARG_TYPE_ASK, NULL      , NULL  , 0             },
        {"act" , ARG_TYPE_ACT, "list|set", action, sizeof(action)},
        {"mode", ArgTypesInt , "0~2"     , &outer, sizeof(int)   },
        {"End" , ArgTypesEnd , NULL      , NULL  , 0             },
    };

    ret = get_config(handleAuthModecfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(int));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        if (DIGEST_AUTH == outer) {
            char realm[64] = {0};
            AuthRealmS ars = {{0,},};
            ret = get_config(handleAuthRealmCfg, ars);
            if (ret == SUCCESS) {
                snprintf(realm, sizeof(realm), "realm=%s;", ars.realm);
            }
            strcat(buf+strlen(buf), realm);
        }
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(int));
        ret = set_config(handleAuthModecfg, outer);
    }
    return SUCCESS;

}

int JCPCmdPrienvCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    UbootEnvS inner = {{0,},};
    UbootEnvS outer = {{0,},};
    static int readflag = 0;
    static char bootbuf[1024] = {0};

    HelpMsgS helps[] = {
        {"?"        , "设备ID设置及环境变量获取"           },
        {"act"      , "list:获取所有参数 set:设置参数" },
        {"device_id", "设备ID，固定必须为11位数字"    },
        {"End"      , "prienv -? 获取帮助"           },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK  , NULL      , NULL           , 0                      },
        {"act"      , ARG_TYPE_ACT  , "list|set|flush|erase", action         , sizeof(action)         },
        {"device_id", ArgTypesString, NULL      , outer.device_id, sizeof(outer.device_id)},
        {"End"      , ArgTypesEnd   , NULL      , NULL           , 0                      },
    };

    system_get_dev_id(outer.device_id);
    memcpy(&inner, &outer, sizeof(UbootEnvS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        if (readflag == 1) {
            strcpy(buf, bootbuf);
            return SUCCESS;
        }

        config_uboot_env(action, opts, bootbuf);
        SYSLOG("flush buf:%s\n", bootbuf);
        strcpy(buf, bootbuf);
        readflag = 1;
    } else if(!strncasecmp("flush", action,strlen("flush"))) {
        LIST_PARAM_RULE_CHECK(opts);
        strcpy(action, "list");
        config_uboot_env(action, opts, bootbuf);
        strcpy(buf, bootbuf);
        readflag = 1;
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        if (strlen(outer.device_id) != MAX_ID_LEN) {      //固定11位
            ERR("strlen device_id != 11");
            sprintf(buf, "%s", "strlen device_id != 11");
            return FAILURE;
        }
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(UbootEnvS));

        if (Security_HardWare == system_get_security_type()) {
            SYSLOG("not support Security_HardWare\n");
            return FAILURE;
        } else {
            DumpFile(PATH_BKUP_DEVID, outer.device_id, strlen(outer.device_id));
            if (uboot_devid_set(outer.device_id) != SUCCESS) {
                SYSLOG("uboot_devid_set device id failed\n");
                DBG("uboot_devid_set device id failed\n");
                return FAILURE;
            }
            SYSLOG("settings devid to %s\n", outer.device_id);
            if (uboot_devinfo_set() != SUCCESS) {
                SYSLOG("uboot_devid_set device info failed\n");
                DBG("uboot_devid_set device info failed\n");
                return FAILURE;
            }

#ifdef __FRESH_SECU__
            SYSLOG("secu: %d\n", system_get_security());
            system_clr_security();
            SYSLOG("secu: %d\n", system_get_security());
            set_aging8h();
#endif
        }

        system_set_dev_id(outer.device_id);
        log_sync();
        //DELAY_REBOOT_LINUX();
        delay_one_minute_reboot();
    }else if(!strncasecmp("erase", action,strlen("erase"))) {
        UtilSystemCmd((char *)"/bin/busybox flash_eraseall /dev/mtd1; rm -f /opt/conf/*.key /opt/conf/tencent.conf*; sync");
        DELAY_REBOOT_LINUX();
    }

    return SUCCESS;
}


static int JCPCmdBootArgs(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    BOOTARGS_CFG_S inner = {{0,},};
    BOOTARGS_CFG_S outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"        , "bootargs 参数设置"                                   },
        {"act"      , "list:获取所有参数 set:设置参数"                      },
        {"sensor"   , "sensor类型: OV9712|AR0130|AR0330可自动探测"          },
        {"flash"    , "flash类型: SF时为spi-flash"                          },
        {"maxheight", "最大帧高度，reset2factory依赖之，必须排在输出第2位。"},
        {"cpu"      , "cpu 类型: 8852/8856                                 "},
        {"hdetect"  , "humandetect: 1，使能；0，禁用"                       },
        {"feature"  , "0:ms_200w"                                           },
        {"lang"     , "0:中文jco 1:中文neu 2:英文 3:德语 4:西班牙语 5:法语 6:意大利语 7:荷兰 8:波兰"},
        {"End"      , "bootargs -? 获取帮助"                                },
    };

    const char *sensor_scope = "NONE|IMX122|CAM1080P|CAM720P|AR0130_1|AR0130|PS1211";

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK  , NULL                    , NULL            , 0                      },
        {"act"      , ARG_TYPE_ACT  , "list|set"              , action          , sizeof(action)         },
        {"sensor"   , ArgTypesString, sensor_scope            , outer.sensor    , sizeof(outer.sensor)   },
        {"flash"    , ArgTypesString, NULL                    , outer.flash     , sizeof(outer.flash)    },
        {"maxheight", ArgTypesInt   , "720|960|1080|1296|1536", &outer.maxheight, sizeof(outer.maxheight)},
        {"cpu"      , ArgTypesString, NULL                    , outer.cpu       , sizeof(outer.cpu)      },
        {"hdetect"  , ArgTypesInt   , "0|1"                   , &outer.hdetect  , sizeof(outer.hdetect)  },
        {"feature"  , ArgTypesInt   , "0~100"                 , &outer.feature  , sizeof(outer.feature)  },
        {"lang"     , ArgTypesInt   , "0~10"                  , &outer.lang     , sizeof(outer.lang)     },
        {"End"      , ArgTypesEnd   , NULL                    , NULL            , 0                      },
    };

    ret = config_bootargs("get", opts, &outer);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&inner, &outer, sizeof(BOOTARGS_CFG_S));
    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        outer.maxheight = get_maxheight();
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(BOOTARGS_CFG_S));
        ret = config_bootargs("set", opts, &outer);
        RETURN_FAIL_IF_API_ERR(ret);
#if defined(PLATFORM_HANBANG)
        // 为方便工厂生产，设置音频开关不重启
        int i = 0;
        for (i = 0; i < argc; i++) {
            if (!strcmp("-audioen", argv[i])) {
                return SUCCESS;
            }
        }
#endif
        // 2019-03-20，方便产测，设置feature后需要恢复出厂，因为不进行重启
        int k = 0;
        for (k = 0; k < argc; k++) {
            if (!strcmp("-feature", argv[k]) || !strcmp("-lang", argv[k]) || !strcmp("-maxheight", argv[k])) {
                return SUCCESS;
            }
        }

        int j = 0;
        for (j = 0; j < argc; j++) {
            if (!strcmp("-hdetect", argv[j])) {
                return SUCCESS;
            }
        }
        DELAY_REBOOT_LINUX();
    }

    return SUCCESS;
}

static int JCPCmdGuobiaoCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    GuoBiaoS inner = {{0,},};
    GuoBiaoS outer = {{0,},};
    char gb_version[64] = {0};

    HelpMsgS helps[] = {
        {"?"           , "国标配置参数"                  },
        {"act"         , "list:获取所有参数 set:设置参数"},
        {"manufacturer", "设备厂商"                      },
        {"owner"       , "设备归属"                      },
        {"civilcode"   , "行政区域"                      },
        {"srvip"       , "SIP服务器ip"                   },
        {"port"        , "端口号"                        },
        {"srvid"       , "服务器ID"                      },
        {"devsysname"  , "设备系统名称"                  },
        {"devtype"     , "设备型号"                      },
        {"devid"       , "设备ID"                        },
        {"alarmid"     , "报警ID"                        },
        {"reginterval" , "注册时间间隔"                  },
        {"hbinterval"  , "心跳时间间隔"                  },
        {"authname"    , "认证用户名"                    },
        {"username"    , "用户名"                        },
        {"password"    , "用户名密码"                    },
        {"videochannel", "0:主码流，1:子码流"            },
        {"localport"     , "设备端端口"                          },
        {"protocoltype"  , "设备端协议类型UDP/TCP"       },
        {"streamtype"    , "主子码流0/1"                 },
        {"connect_status", "设备连接状态(离线/在线)"     },
        {"gb_version"    , "设备国标版本，字符串"     },
        {"End"         , "guobiaocfg -? 获取帮助"        },
    };

    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL               , 0                         },
        {"act"         , ARG_TYPE_ACT  , "list|set", action             , sizeof(action)            },
        {"manufacturer", ArgTypesString, NULL      , outer.manufacturer , sizeof(outer.manufacturer)},
        {"owner"       , ArgTypesString, NULL      , outer.owner        , sizeof(outer.owner)       },
        {"civilcode"   , ArgTypesString, NULL      , outer.civilcode    , sizeof(outer.civilcode)   },
        {"srvip"       , ArgTypesString, NULL      , outer.sip_srv_ip   , sizeof(outer.sip_srv_ip)  },
        {"port"        , ArgTypesInt   , "1~65535" , &outer.port        , sizeof(outer.port)        },
        {"srvid"       , ArgTypesString, NULL      , outer.srv_id       , sizeof(outer.srv_id)      },
        {"devsysname"  , ArgTypesString, NULL      , outer.dev_sysname  , sizeof(outer.dev_sysname) },
        {"devtype"     , ArgTypesString, NULL      , outer.dev_type     , sizeof(outer.dev_type)    },
        {"devid"       , ArgTypesString, NULL      , outer.video_channal_id, sizeof(outer.video_channal_id)      },
        {"alarmid"     , ArgTypesString, NULL      , outer.alarm_id     , sizeof(outer.alarm_id)    },
        {"reginterval" , ArgTypesInt   , "1~65535" , &outer.reg_interval, sizeof(outer.reg_interval)},
        {"hbinterval"  , ArgTypesInt   , "0~600"   , &outer.hb_interval , sizeof(int)               },
        {"authname"    , ArgTypesString, NULL      , outer.authname     , sizeof(outer.authname)    },
        {"username"    , ArgTypesString, NULL      , outer.username     , sizeof(outer.username)    },
        {"password"    , ArgTypesString, NULL      , outer.password     , sizeof(outer.password)    },
        {"enable"      , ArgTypesInt   , "0~1"     , &outer.enable      , sizeof(int)               },
        {"videochannel", ArgTypesInt   , "0~1"     , &outer.videochannel, sizeof(int)               },
        {"localport"     , ArgTypesInt   , "1025~65535", &outer.localport     , sizeof(int)               },
        {"protocoltype"  , ArgTypesInt   , "0|1"       , &outer.protocoltype  , sizeof(int)               },
        {"streamtype"    , ArgTypesInt   , "0|1"       , &outer.streamtype    , sizeof(int)               },
        {"connect_status", ArgTypesInt   , "0|1"       , &outer.connect_status, sizeof(int)               },
        {"gb_version"    , ArgTypesString, NULL        , gb_version           , sizeof(gb_version)        },
        {"End"         , ArgTypesEnd   , NULL      , NULL               , 0                         },
    };

    ret = get_config(handleGuoBiaoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(GuoBiaoS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
#ifdef PLATFORM_GB
        outer.connect_status = gb_get_online();
        snprintf(gb_version, sizeof(gb_version), "%s", gb_get_version_str());
#endif
        ASMJCP_LIST_STRING(buf, buflen, opts);

    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        if (SUCCESS == arg_opt_if_set("password", opts)) {
            replace_str(outer.password, "%26", "&");
        }

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(GuoBiaoS));
        ret = set_config(handleGuoBiaoCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return ret;
}

static int JCPCmdGuobiaoAddrCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    GBAddrS inner = {{0,},};
    GBAddrS outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"       , "国标位置信息"},
        {"act"     , "list:获取所有参数 set:设置参数"},
        {"address" , "位置信息"},
        {"longitude", "经度"},
        {"latitude", "纬度"},
        {"End"     , "guobiaoaddr -?获取帮助"},
    };

    ArgOptS opts[] =
    {
        {"?"   , ARG_TYPE_ASK, NULL      , NULL  , 0             },
        {"act" , ARG_TYPE_ACT, "list|set", action, sizeof(action)},
        {"address", ArgTypesString , NULL , outer.address, sizeof(outer.address)     },
        {"longitude", ArgTypesFloat , "0~180" , &outer.longitude, sizeof(float)  },
        {"latitude", ArgTypesFloat , "0~90", &outer.latitude, sizeof(float)  },
        {"End" , ArgTypesEnd , NULL      , NULL  , 0             },
    };

    ret = get_config(handleGuoBiaoAddrCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(GBAddrS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(GBAddrS));
        ret = set_config(handleGuoBiaoAddrCfg, outer);
    }

    return ret;
}

static int JCPCmdAudioCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    AudioCfgS inner = {0};
    AudioCfgS outer = {0};

    HelpMsgS helps[] = {
        {"?"            , "音频配置参数获取和设置"        },
        {"act"          , "list:获取所有参数 set:设置参数"},
        {"inenable"     , "音频开关 ， 0:关闭，1:打开"    },
        {"inputtype"    , "音频输入方式 ， 0:Mic输入，1:Line-In输入"    },
        {"involume"     , "音频输入音量"                  },
        {"outvolume"    , "音频输出音量"                  },
        {"definvolume"  , "默认音频输入音量"              },
        {"defoutvolume" , "默认音频输出音量"              },
        {"codetype"     , "音频编码格式，1:G711A, 2:G711u"},
        {"amrbps"       , "音频码率"                      },
        {"talkvolume"   , "对讲音量大小"                  },
        {"talkamp"      , "对讲音量数字放大倍数"          },
        {"outamp"       , "本地音频音量数字放大倍数"      },
        {"inamp"        , "音频输入音量数字放大倍数"      },
        {"End"          , "audiocfg -?获取帮助"           },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK  , NULL      , NULL                , 0             },
        {"act"          , ARG_TYPE_ACT  , "list|set", action              , sizeof(action)},
        {"inenable"     , ArgTypesInt   , "0|1"     , &outer.inenable     , sizeof(int)   },
        {"inputtype"    , ArgTypesInt   , "0|1"     , &outer.inputtype    , sizeof(int)   },
        {"involume"     , ArgTypesInt   , "0~100"   , &outer.involume     , sizeof(int)   },
        {"outvolume"    , ArgTypesInt   , "0~100"   , &outer.outvolume    , sizeof(int)   },
        {"definvolume"  , ArgTypesInt   , "0~100"   , &outer.definvolume  , sizeof(int)   },
        {"defoutvolume" , ArgTypesInt   , "0~100"   , &outer.defoutvolume , sizeof(int)   },
        {"codetype"     , ArgTypesInt   , "0~7"     , &outer.codetype     , sizeof(int)   },
        {"amrbps"       , ArgTypesInt   , "0~7"     , &outer.amrbps       , sizeof(int)   },
        {"talkvolume"   , ArgTypesInt   , "0~100"   , &outer.talkvolume   , sizeof(int)   },
        {"talkamp"      , ArgTypesFloat , NULL      , &outer.talkamp      , sizeof(float) },
        {"outamp"       , ArgTypesFloat , NULL      , &outer.outamp       , sizeof(float) },
        {"inamp"        , ArgTypesFloat , NULL      , &outer.inamp        , sizeof(float) },
        {"End"          , ArgTypesEnd   , NULL      , NULL                , 0             },
    };

    ret = get_config(handleAudioCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(AudioCfgS));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(AudioCfgS));
        ret = set_config(handleAudioCfg, outer);
    }

    return ret;

}

static int JCPCmdAudioTestCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};

    AudioTestCfgS inner = {0};
    AudioTestCfgS outer = {0};

    HelpMsgS helps[] = {
        {"?"        , "音频配置参数获取和设置"        },
        {"act"      , "list:获取所有参数 set:设置参数"},
        {"start"    , "音频回环开关 ， 0:关闭，1:打开"    },
        {"inputtype", "音频输入方式 ， 0:Mic输入，1:Line-In输入"    },
        {"involume" , "音频输入音量"                  },
        {"outvolume", "音频输出音量"                  },
        {"End"      , "audiotestcfg -?获取帮助"           },
    };

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL            , 0             },
        {"act"      , ARG_TYPE_ACT, "list|set", action          , sizeof(action)},
        {"start"    , ArgTypesInt , "0|1"     , &outer.start    , sizeof(int)   },
        {"inputtype", ArgTypesInt , "0|1"     , &outer.inputtype , sizeof(int)   },
        {"involume" , ArgTypesInt , "0~100"   , &outer.involume , sizeof(int)   },
        {"outvolume", ArgTypesInt , "0~100"   , &outer.outvolume, sizeof(int)   },
        {"End"      , ArgTypesEnd , NULL      , NULL            , 0             },
    };

    ret = get_config(handleAudioTestCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(outer));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(outer));
        ret = set_config(handleAudioTestCfg, outer);
    }

    return ret;
}

static int JCPCmdFormat(char *buf, int buflen, int argc, char **argv)
{
    char name[JCP_ACTION_LEN] = {0};
    int enable = 0;

    HelpMsgS helps[] = {
        {"?"      , "格式化命令"               },
        {"name"   , "磁盘名称"               },
        {"enable" , "是否格式化，0:否  1:是" },
        {"End"    , "format -?获取帮助"     },
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK  , NULL      , NULL          , 0                     },
        {"name"    , ARG_TYPE_ACT  , NULL      , name        , sizeof(name)        },
        {"enable"  , ArgTypesInt   , "0|1"     , &enable , sizeof(int)           },
        {"End"     , ArgTypesEnd   , NULL      , NULL          , 0                     },
    };

    JCP_ARG_PARSER(NULL, 0);

    if (SUCCESS == arg_opt_if_set("name", opts) &&
        (SUCCESS == arg_opt_if_set("enable", opts))) {
        if (1 == enable) {
            SYSLOG("will be format --- %s\n", name);
#if defined(PLATFORM_TENCENT)
            tencent_format_sd_card();
#endif
        }
    }

    return SUCCESS;
}

static int JCPCmdFormatProgressBar(char *buf, int buflen, int argc, char **argv)
{

    char action[JCP_ACTION_LEN] = {0};
    char progbar[32] = {0};
    int status = 0;

    HelpMsgS helps[] = {
        {"?"      , "格式化进度"               },
        {"act"    , "list:获取所有参数"},
        {"progbar" , "磁盘名称"        },
        {"End"    , "mkdosfsprogbar -?获取帮助"     },
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK  , NULL      , NULL   , 0             },
        {"act"     , ARG_TYPE_ACT  , "list"    , action , sizeof(action)},
        {"progbar" , ArgTypesString   , NULL   , &progbar, sizeof(progbar)},
        {"End"     , ArgTypesEnd   , NULL      , NULL   , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))){
        record_get_format_status(&status);
        sprintf((char *)buf, "diskid=%s;progbar=%d;",progbar, status);
    }

    return SUCCESS;
}

static int JCPCmdLensDPCS(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int ret = SUCCESS;

    LensDPCS outer = {0};

    HelpMsgS helps[] = {
        {"?"      , "坏点和镜头阴影矫正"            },
        {"act"    , "list:获取所有参数 set:设置参数"},
        {"type"   , "1:镜头阴影矫正，2:坏点矫正"    },
        {"strengh", "镜头阴影矫正强度,仅镜头阴影矫正使用"},
        {"End"    , "tutkcfg -?获取帮助"            },
    };

    ArgOptS opts[] =
    {
        {"?"       , ARG_TYPE_ASK  , NULL      , NULL          , 0                     },
        {"act"     , ARG_TYPE_ACT  , "list|set", action        , sizeof(action)        },
        {"type"    , ARG_TYPE_MUSTINT, "1~2"     , &outer.type, sizeof(int)            },
        {"strengh" , ArgTypesInt   , "0~200"   , &outer.ratio, sizeof(int)             },
        {"End"     , ArgTypesEnd   , NULL      , NULL          , 0                     },
    };
    outer.type = 1;
    outer.ratio = 85;
    JCP_ARG_PARSER(NULL, 0);
    if(!strncasecmp("list", action,strlen("list")))
    {
        outer.status = 2;         //  2:获取矫正
#ifdef __migrate_conf_over__
        ret = encode_lensdpc(&outer);
#endif
        sprintf(buf, "type=%d;status=%d;", outer.type, outer.status);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        outer.status = 1;        // 1:开始矫正
#ifdef __migrate_conf_over__
        ret = encode_lensdpc(&outer);
#endif
    }

    return ret;
}

static int JCPCmdDhcpNotify(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    DhcpNotifyS outer = {{0,}};

    HelpMsgS helps[] = {
        {"?"         , "DHCP配置结果通知"              },
        {"act"       , "list:通知 set:通知"            },
        {"result"    , "dhcp结果: success|fail"        },
        {"interface" , "eth0|wlan0|ra0"                },
        {"ip"        , "dhcp成功的IP"                  },
        {"End"       , ""                              },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK  , NULL      , NULL           , 0                        },
        {"act"      , ARG_TYPE_ACT  , "set|list", action         , sizeof(action)           },
        {"result"   , ArgTypesString, NULL      , outer.result   , sizeof(outer.result)     },
        {"interface", ArgTypesString, NULL      , outer.interface, sizeof(outer.interface)  },
        {"ip"       , ArgTypesString, NULL      , outer.ip       , sizeof(outer.ip)         },
        {"End"      , ArgTypesEnd   , NULL      , NULL           , 0                        },
    };

    JCP_ARG_PARSER(NULL, 0);
    //if (_io_jcp) DBG("result:%s interface:%s ip:%s\n", outer.result, outer.interface, outer.ip);
    DBG("result:%s interface:%s ip:%s\n", outer.result, outer.interface, outer.ip);
    if (strcmp(outer.result, "fail") == 0) {
        return 0;
    }

   if(!strcasecmp("set", action) || !strcasecmp("list", action)) {
        //dhcp成功后通知onvif，否则onvif会不出图
        set_config(handleDhcpNotify, outer);

       if (strcmp(outer.result, "success") == 0) {
        if (0 == strcmp(outer.interface, "eth0")) {
            return 0;
        }

        static time_t prev = 0;
        if (labs(time(NULL)-prev) > 60  || !get_g_stat(wifi, WIFI_DHCPSUCC)) {
            prev = time(NULL);

            if (!get_g_sys(usb_4g)) {
                encode_audio_queue_push_amr(AUDIO_IP_SUCESS, FALSE);
            }
        }

        // wifi 4g 共用 DHCPONCE, 第二次才 JEvent_TencentReset
        if (get_g_sys(usb_wifi)) {
            if (get_g_stat(wifi, WIFI_DHCPONCE)) {
                send_event(JEvent_TencentReset);
            }
            set_g_stat(wifi, WIFI_DHCPONCE);
        } else {
            if (get_g_stat(sim4g, SIM4G_DHCPONCE)) {
                send_event(JEvent_TencentReset);
            }
            set_g_stat(sim4g, SIM4G_DHCPONCE);
        }

        set_g_stat(wifi, WIFI_DHCPSUCC);

       } else {
        clr_g_stat(wifi, WIFI_DHCPSUCC);
       }
   }


    return 0;
}

static int JCPCmdWifiCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    int tmp = 0;

    char action[JCP_ACTION_LEN] = {0};

    char blename[32] = {0};
    char camname[32] = {0};
    NetWifiS inner = {0};
    NetWifiS outer = {0};

    HelpMsgS helps[] = {
        {"?"          , "wifi配置"                                 },
        {"act"        , "list:获取所有参数 set:设置参数"                   },
        {"nic"        , "wifi网卡名称"                               },
        {"ssid"       , "wifi ssid name"                         },
        {"weppasswd"  , "wifi wep passwd 密钥"                     },
        {"token"      , "设备绑定token"                              },
        {"mode"       , "0:AP 1:AP+STA"                          },
        {"dhcp"       , "IP配置类型，0:手动配置IP，1:自动获取IP" },
        {"ip"         , "ip地址"                                   },
        {"mask"       , "掩码"                                     },
        {"gw"         , "网关"                                     },
        {"mac"        , "wifi  MAC地址"                            },
        {"wepauthtype", "已经废弃，只做兼容"                              },
        {"encrypttype", "已经废弃，只做兼容"                              },
        {"status"     , "只供App使用，1 Wifi已连接，0未连接, 查询信号使用wifilist"},
        {"blename"    , "蓝牙名称:AIBLE%d-******"                    },
        {"camname"    , "热点名称:AICAM%d-******"                    },
        {"End"        , "wificfg -?获取帮助"                         },
    };

    ArgOptS opts[] = {
        {"?"          , ARG_TYPE_ASK  , NULL      , NULL           , 0                      },
        {"act"        , ARG_TYPE_ACT  , "list|set", action         , sizeof(action)         },
        {"nic"        , ArgTypesString, NULL      , outer.nic      , sizeof(outer.nic)      },
        {"ssid"       , ArgTypesString, NULL      , outer.ssid     , sizeof(outer.ssid)     },
        {"weppasswd"  , ArgTypesString, NULL      , outer.weppasswd, sizeof(outer.weppasswd)},
        {"token"      , ArgTypesString, NULL      , outer.token    , sizeof(outer.token)    },
        {"mode"       , ArgTypesInt   , "0~1"     , &outer.mode    , sizeof(int)            },
        {"dhcp"       , ArgTypesInt   , "0|1"     , &outer.dhcp    , sizeof(int)            },
        {"ip"         , ArgTypesString, NULL      , outer.ip       , sizeof(outer.ip)       },
        {"mask"       , ArgTypesString, NULL      , outer.mask     , sizeof(outer.mask)     },
        {"gw"         , ArgTypesString, NULL      , outer.gw       , sizeof(outer.gw)       },
        {"mac"        , ArgTypesString, NULL      , outer.mac      , sizeof(outer.mac)      },
        {"wepauthtype", ArgTypesInt   , NULL      , &tmp           , sizeof(int)            },
        {"encrypttype", ArgTypesInt   , NULL      , &tmp           , sizeof(int)            },
        {"status"     , ArgTypesInt   , NULL      , &outer.status  , sizeof(int)            },
        {"blename"    , ArgTypesString, NULL      , blename        , sizeof(blename)        },
        {"camname"    , ArgTypesString, NULL      , camname        , sizeof(camname)        },
        {"End"        , ArgTypesEnd   , NULL      , NULL           , 0                      },
    };

    ret = get_config(handleWifiCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(NetWifiS));

    JCP_ARG_PARSER(NULL, 0);
    if(!strcasecmp("list", action)) {
        int j = 0;
        int appid = 0;
        char mac_data[8] = {0,};
        SysInfoS sysinfo = {0};
        conf_get_sysinfocfg(&sysinfo);
        for (int i=0; i<17;i++) {
            if (i ==9 || i ==10 || i == 12 || i==13 || i == 15 || i == 16) {
                mac_data[j++] = outer.mac[i];
            }
        }
        appid = atoi10(sysinfo.custom_appid)%100;
        if (appid == 0) {   // 中性版本兼容当前已提交上线的大卫看家app版本
            snprintf(blename, sizeof(blename), "AIBLE-%s", mac_data);
            snprintf(camname, sizeof(camname), "AICAM-%s", mac_data);
        } else {
            snprintf(blename, sizeof(blename), "AIBLE%d-%s", appid, mac_data);
            snprintf(camname, sizeof(camname), "AICAM%d-%s", appid, mac_data);
        }

        outer.status = (COMPLETED == get_wifi_status());

        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strcasecmp("set", action)) {
        if (strncmp(inner.ssid, outer.ssid, sizeof(outer.ssid)) == 0 &&
            strncmp(inner.weppasswd, outer.weppasswd, sizeof(outer.weppasswd)) == 0 &&
            access(SUPPLICANT_OK_CONF, F_OK) != 0) {
            memset(inner.ssid, 0, sizeof(inner.ssid));
        }

        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(NetWifiS));
        ret = set_config(handleWifiCfg, outer);

        if (inner.mode != outer.mode) {
            DBG("__________ changed mode \n");
        }
    }

    return ret;
}

static int JCPCmdWifiList(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};

    int status = 0;
    char ip[32] = {0};
    char wifiscan[4096] = {0};

    struct timeval start, end;
    gettimeofday(&start, NULL);

    HelpMsgS helps[] = {
        {"?"     , "获取局域网内的wifi"            },
        {"act"   , "list:获取wifi列表"             },
        {"End"   , ""                              },
    };

    ArgOptS opts[] = {
        {"?"     , ARG_TYPE_ASK  , NULL   , NULL         , 0             },
        {"act"   , ARG_TYPE_ACT  , "set|list|status"  , action       , sizeof(action)},
        {"End"   , ArgTypesEnd   , NULL   , NULL         , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("list", action)) {
        get_wifi_hotspot_list(wifiscan);
        snprintf(buf, buflen, "%s", wifiscan);
    } else if (!strcasecmp("status", action)) {
        status = get_wifi_status();
        net_get_ipaddr("wlan0", ip,sizeof(ip));
        sprintf(buf, "status=%d;ip_address=%s;", status, ip);
    }

    gettimeofday(&end, NULL);
    float timeuse = 1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec;
    timeuse /= 1000;
    DBG("[%s:%d]%f ms\n", __FUNCTION__, __LINE__, timeuse);

    return 0;
}

static void _read_frame(void *p, int sn, char *tFrame, int sz, eShmFrameType type, double t)
{
    printf("frame_timestamp: %lf, sz: %dK\n", t, sz>>10);
    printf("diff_usec: %lf, chn:%d\n", get_usec_of_day()/1000000.0 - t, (int)p);
}

/*
 * join 已经分离的 tid 可能会有 SEGV
 **/
static const char *tid_stat(pthread_t tid)
{
    void *retval;
    int rc = pthread_tryjoin_np(tid, &retval);

    switch (rc) {
    case 0:     // 成功连接：线程已退出且未分离
        return "join-succ";
    case EBUSY: // 线程仍在运行
        return "running";
    case EINVAL: // 线程已分离或 ID 无效
        return "detached";
    case ESRCH: // 线程不存在
        return "not-exist";
    default:    // 其他未定义错误
        printf("unexpected error: %d\n", rc);
        return "unknown-error";
    }
}

static int print_jsdbg_usage()
{
    printf(
        "Usage: jsdbg cmd list:\n"
        "\t u        # 测试模式, unbind ali, when reset key broken\n"
        "\t x        # buf 中 oldest newest I帧时间戳\n"
        "\t w        # uninit wdt\n"
        "\t c  FD    # 关 fd\n"
        "\t e  EVENT # 发裸事件\n"
        "\t E  alarm # 发裸事件\n"
        "\t d        # dump 所有 sch 开执行的 cb\n"
        "\t b        # 阻塞 test sch\n"
        "\t t  [leg] # lt eq gt 定时器测试\n"
        "\t s        # 更新 secu\n"
        "\t 8        # 更新 aging8h pass=1\n"
        "\t L        # 1:begin 0:end 6,7,8 day,night,daynight 6->7->8->6->8->7->6\n"
        "\t k        # pthread_kill tid\n"
        "\t 4        # m94g 9M 1m\n"
        "\t 5        # tail messages\n"
        "\t r        # sd report\n"
        );
    return 0;
}

void do_lamp_isp(int i)
{
    switch (i) {
    case 0:
    case 1:
    case 6 ... 8:
        if (i == 6) {   // 白天效果
            send_event_chn(JEvent_RunIspColor, ISP_DAY);
        } else if (i == 8) {    // 晚上效果
            send_event_chn(JEvent_RunIspColor, ISP_COLOR_NIGHT);
        } else {
            send_event_chn(JEvent_LedTest, i);
        }
        break;
    default:
        print_jsdbg_usage();
        break;
    }
}

static int print_vm_usage()
{
    printf(
        "Usage: vm cmd, cmd list:\n"
        "\t p        # pthread_create\n"
        "\t m        # malloc\n"
        "\t r        # reset malloc() trace\n"
        "\t c        # clock_gettime(CLOCK_MONOTONIC)\n"
        );
    return 0;
}

/*
 * cpu + mem + time
 **/
static int JCPCmdVm(char *buf, int buflen, int argc, char **argv)
{
    if (argc < 2) return print_vm_usage();

    switch (argv[1][0]) {
#if __VALTHREAD
    case 'p':	vm_pri_thread_running(); break;
#endif
#if __VALGRIND
    case 'm':	vm_statistic(); break;
    case 'r':   {
                DBG("total vm_bigfreekb(buddy>=256KB): %d\n\n", vm_bigfreekb(7));
                vm_wrap_restart();
                vm_statistic();
                break;
                }
#endif
    case 'c':   {
                static struct timespec prev;
                if (prev.tv_sec == 0) { ms_clock_reset(&prev); }
                printf("ms : %lld\n", ms_since_previous(&prev));
                printf("sec: %lld\n", sec_since_previous(&prev));
                break;
                }
    default: print_vm_usage();	break;
    }

    return 0;
}

static int JCPCmdJsDbg(char *buf, int buflen, int argc, char **argv)
{
    if (argc < 2) return print_jsdbg_usage();

    switch (argv[1][0]) {
    case 'L':   { do_lamp_isp(argc == 3 ? atoi(argv[2]) : -1); break; }
    // case 'i':   { exec_cmd_manualawb(atoi(argv[2])); break; }
    case 'x':   {
                int ret;
                ret = shm_buf_read_frame(get_shm_buf_pool(0), -2/*oldest*/, _read_frame, (void *)0);
                printf("xgop[0] %d\n", ret);
                ret = shm_buf_read_frame(get_shm_buf_pool(1), -2/*oldest*/, _read_frame, (void *)1);
                printf("xgop[1] %d\n", ret);
                break;
                }
    case 'w':   DBG("stop dog now\n"); uninit_client_watchdog_feed(); break;
    case 'e':   send_conf_nake(atoi(argv[2])); break;
    case 'E':   send_event(atoi(argv[2])); break;
    case 'b':   { static int on = FALSE; if (!on) { on = TRUE; start_test_sch(); }; break; }
    // case 'B':   { set_storage_watch_blocked_detect(); break; }
    // case 't':   { start_test_timer(argc == 3 ? argv[2][0] : '0');  break; }
    case 'c':   close(atoi(argv[2])); break;
    case 'd':   js_dump_schedulers_callback(); break;
    case 'k':   printf("tid stat: %s\n", tid_stat((pthread_t)atoi(argv[2]))); break;
    case '8':   set_aging8h(); break;
    case 's':   {
                SYSLOG("secu: %d\n", system_get_security());
                system_clr_security();
                SYSLOG("secu: %d\n", system_get_security());
                break;
                }
    case 'u':   {
                SYSLOG("unbind from jcp @test_ver only\n");
                if (!is_test_ver()) break;
                break;
                }
    case 'r':   {
#ifdef PLATFORM_TENCENT
                report_sd_stat();
#endif
                break;
                }
    case '4':   UtilSystemCmd("/ipc/bin/m94g"); strcpy(buf, "do NEXT 5"); break;
    case '5':   LoadFile("/tmp/ff4g.log", buf, MIN(1800, buflen));
                replace_str(buf, "\r\n", "<CR>");
                replace_str(buf, "\n", "<CR>");
                break;
    default :   print_jsdbg_usage(); break;
    }
    return 0;
}

static int JCPCmdKeepAlive(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"  , "keep-alive发送一条命令测试JCP是否阻塞" },
        {"act", "live:测试命令"                         },
        {"End", "keepalive -?获取帮助"                  },
    };

    ArgOptS opts[] = {
        {"?"  , ARG_TYPE_ASK, NULL  , NULL  , 0             },
        {"act", ARG_TYPE_ACT, "live", action, sizeof(action)},
        {"End", ArgTypesEnd , NULL  , NULL  , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    //进来后直接返回
    return SUCCESS;
}

static int JCPCmdlist(char *buf, int buflen, int argc, char **argv)
{
    int i;
    char *p = buf;

    for (i = 0; JcpCmdAll[i].cmd != NULL; i++) {
        p += sprintf(p, "%s;", JcpCmdAll[i].cmd);
    }

    return SUCCESS;
}

int JCPCmdDevTestCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    DevTestS outer = {0};
    HelpMsgS helps[] = {
        {"?"            , "测试设备基本功能\r\n"                   },
        {"act"          , "list:获取所有参数\r\n"                  },
        {"factest"      , "使能产测模式 true 只支持开启，不支持关闭\r\n"},
        {"code"         , "自动化产测\r\n"
                          "1 ?像5s后自动播放: /tmp/mic.pcm \r\n"
                          "2 LED-BLINK\r\n"
                          "3 LED-BLINK-OFF\r\n"
                          "4 LED-WHITE-ON\r\n"
                          "5 LED-WHITE-OFF\r\n"
                          "6 LED-DOUBLEFLASH-ON\r\n"
                          "7 LED-DOUBLEFLASH-OFF\r\n"
                          "8 LED-RED-ON\r\n"
                          "9 LED-RED-OFF\r\n"
        },
        {"rtcstat"      , "rtc是否正常， 1:正常、0:不正常\r\n"     },
        {"ecodestat"    , "编码是否正常，1:正常、0:不正常\r\n"     },
        {"sdexist"      , "sd卡是否存在，1:存在、0:不存在\r\n"     },
        {"rstkey"       , "复位键状态，1:按下、0:弹起\r\n"         },
        {"callstat"     , "呼叫状态，0:未通话、1:发起通话操作、2:呼叫中、3:通话中、10:拒接\r\n"},
        {"wifiexist"    , "wlan0是否存在，1:存在、0:不存在\r\n"    },
        {"bleexist"     , "ble是否存在，1:存在、0:不存在\r\n"      },
        {"wificonnected", "wifi是否连接，1:连接、0:未连接\r\n"     },
        {"wifiquality"  , "wifi信号强度 0-100\r\n"                 },
        {"wifipass"     , "wifi是否通过测试， 1:通过、0:不通过\r\n"},
        {"doubleboard"  , "单双板，1:双板、0:单板，-1:未配置\r\n"  },
        {"eth0mac"      , "eth0物理地址\r\n"                       },
        {"wlan0mac"     , "wlan0物理地址\r\n"                      },
        {"sim4g"        , "4g模块是否存在\r\n"                      },
        {"stepdebug"    , "步长调试: 0-关闭 1-开启 2-清除 P T\r\n" },
        {"End"          , "devtest -? 获取帮助"                    },
    };

    ArgOptS opts[] = {
        {"?"            , ARG_TYPE_ASK  , NULL      , NULL                       , 0                          },
        {"act"          , ARG_TYPE_ACT  , "set|list", (void*)action              , sizeof(action)             },
        {"factest"      , ArgTypesInt   , "0|1"     , (void*)&outer.factest      , sizeof(outer.factest)      },
        {"code"         , ArgTypesInt   , NULL      , (void*)&outer.code         , sizeof(outer.code)         },
        {"rtcstat"      , ArgTypesInt   , "0|1"     , (void*)&outer.rtcstat      , sizeof(outer.rtcstat)      },
        {"ecodestat"    , ArgTypesInt   , "0|1"     , (void*)&outer.ecodestat    , sizeof(outer.ecodestat)    },
        {"sdexist"      , ArgTypesInt   , "0|1"     , (void*)&outer.sdexist      , sizeof(outer.sdexist)      },
        {"rstkey"       , ArgTypesInt   , "0|1"     , (void*)&outer.rstkey       , sizeof(outer.rstkey)       },
        {"callstat"     , ArgTypesInt   , "0~10"    , (void*)&outer.callstat     , sizeof(outer.callstat)     },
        {"wifiexist"    , ArgTypesInt   , "0|1"     , (void*)&outer.wifiexist    , sizeof(outer.wifiexist)    },
        {"bleexist"     , ArgTypesInt   , "0|1"     , (void*)&outer.bleexist     , sizeof(outer.bleexist)     },
        {"wificonnected", ArgTypesInt   , "0|1"     , (void*)&outer.wificonnected, sizeof(outer.wificonnected)},
        {"wifiquality"  , ArgTypesInt   , "0~100"   , (void*)&outer.wifiquality  , sizeof(outer.wifiquality)  },
        {"wifipass"     , ArgTypesInt   , "0|1"     , (void*)&outer.wifipass     , sizeof(outer.wifipass)     },
        {"doubleboard"  , ArgTypesInt   , "-1|0|1"  , (void*)&outer.doubleboard  , sizeof(outer.doubleboard)  },
        {"eth0mac"      , ArgTypesString, NULL      , (void*)&outer.eth0mac      , sizeof(outer.eth0mac)      },
        {"wlan0mac"     , ArgTypesString, NULL      , (void*)&outer.wlan0mac     , sizeof(outer.wlan0mac)     },
        {"sim4g"        , ArgTypesInt   , "0|1"     , (void*)&outer.sim4g        , sizeof(outer.sim4g)        },
        {"stepdebug"    , ArgTypesInt   , "0|1|2"   , (void*)&outer.stepdebug    , sizeof(outer.stepdebug)    },
        {"End"          , ArgTypesEnd   , NULL      , NULL                       , 0                          },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        outer.ecodestat = 1;
        outer.factest = get_g_sys(factest) ? 1 : 0;
        get_rtcstat(&outer.rtcstat);
        system_get_sdexist(&outer.sdexist);
        rst_get_stat(&outer.rstkey);
        outer.wifiexist = get_g_sys(usb_wifi) ? get_wifiexist() : FALSE;
        outer.bleexist = is_okey(F_BLE) ? TRUE : FALSE;
        outer.wificonnected = (COMPLETED == get_wifi_status())? TRUE : FALSE;
        outer.wifiquality = get_wifi_quality();
        outer.callstat = get_video_call_status();
        processEthMac(ConfGet, "eth0", outer.eth0mac);
        if(0 != outer.wifiexist)
            processEthMac(ConfGet, "wlan0", outer.wlan0mac);

        if (get_g_sys(usb_4g)) {
            outer.sim4g = TRUE;
        }

        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strcasecmp("set", action)) {
        // 不插卡产测
        if (outer.factest && !get_g_sys(factest)) {
            encode_audio_queue_push_amr(AUDIO_FACTORY_TEST, FALSE);
            set_g_sys(factest);
            uninit_record_watch();   //产测关闭录像
            SYSLOG("entry factest mod by jcp\n");
        }

        if (SUCCESS == arg_opt_if_set("code", opts)) {
            factry_tool_main(outer.code);
        }

        if (SUCCESS == arg_opt_if_set("stepdebug", opts)) {
            factry_tool_handle_step(outer.stepdebug);
        }

        if (SUCCESS == arg_opt_if_set("callstat", opts) &&
            10 == outer.callstat) {
            call_evt_enqueue(E_EVT_CALL_REJECT);
        }
    }

    return SUCCESS;

}

static int JCPCmdRecordAlarmStop(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    char stop_time[32] = {0};
    //time_t    stoptime;
    int status = 0;

    HelpMsgS helps[] = {
        {"?"       , "cmd describle"                                                         },
        {"act"     , "list:read set:write"                                                   },
        {"stoptime", "报警触发`UTC`时间，?像在这端时间内触发的话，会停止时间"               },
        {"End"     , "status  1:打包成功，需等待2s  2: 已停止状态  3: 在?像范围外不需要打包"},
    };

    ArgOptS opts[] = {
        {"?"       , ARG_TYPE_ASK  , NULL , NULL     , 0                },
        {"act"     , ARG_TYPE_ACT  , "set", action   , sizeof(action)   },
        {"stoptime", ArgTypesString, NULL , stop_time, sizeof(stop_time)},
        {"End"     , ArgTypesEnd   , NULL , NULL     , 0                },
    };

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("set", action)) {
        //int curr = time(NULL);
        //if (strlen(stop_time) > 10) { COLOR_R("WARNING: ms coming"); stop_time[10] = '\0'; }
        //stoptime = atoi(stop_time) + aliyun_get_time_offset();

        //todo

        sprintf(buf, "status=%d;\n", status);
    }
    return SUCCESS;
}

static int JCPCmdSensorFps(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    char action[JCP_ACTION_LEN] = {0};
    int outer;
    memset(&outer, 0, sizeof(int));

    HelpMsgS helps[] = {
        {"?"     , "设置sensor帧率，调试使用"                },
        {"act"   , "set:设置参数"                       },
        {"fps"   , "sensor 帧率"                      },
        {"End"   , "sensorfps -?获取帮助"               },
    };

    ArgOptS opts[] =
    {
        {"?"     , ARG_TYPE_ASK, NULL      , NULL         , 0             },
        {"act"   , ARG_TYPE_ACT, "set"     , action       , sizeof(action)},
        {"fps"   , ArgTypesInt , "1~30"    , &outer       , sizeof(int)   },
        {"End"   , ArgTypesEnd , NULL      , NULL         , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        ret = set_config(handlesensorfps, outer);
    }

    return ret;
}

static int JCPCmdAgingTestCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    ag_params_t outer = {0};
    ag_params_t inner = {0};

    HelpMsgS helps[] = {
        {"?"        , "老化测试控制命令"                             },
        {"act"      , "list:获取所有参数 set:设置参数"               },
        {"enable"   , "0:关;1:工具开;2:工具停;4:脚本次数跑完停"      },
        {"port"     , "1:激活 8007"                                  },
        {"keep"     , "0:断电不继续老化，1:断电继续老化"             },
        {"ircut"    , "0:不开启;1：切红外ircut;2:切白光ircut"        },
        {"audio"    , "0:不开启，1:卡里的音频文件，2：“测试模式”音频"},
        {"led_red"  , "红外灯老化控制"                               },
        {"led_white", "白光灯老化控制"                               },
        {"ptz"      , "云台老化项"                                   },
        {"speed"    , "云台老化测试速度"                             },
        {"zoom"     , "变倍老化测试"                                 },
        {"End"      , "agtest -? 获取帮助"                           },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL       , NULL            , 0             },
        {"act"      , ARG_TYPE_ACT, "list|set" , action          , sizeof(action)},
        {"enable"   , ArgTypesInt , "0|1|2|3|4", &outer.enable   , sizeof(int)   },
        {"port"     , ArgTypesInt , "0|1"      , &outer.port     , sizeof(int)   },
        {"keep"     , ArgTypesInt , "0|1|2"    , &outer.keep     , sizeof(int)   },
        {"ircut"    , ArgTypesInt , "0|1|2"    , &outer.ircut    , sizeof(int)   },
        {"audio"    , ArgTypesInt , "0|1|2"    , &outer.audio    , sizeof(int)   },
        {"led_red"  , ArgTypesInt , "0|1|2"    , &outer.led_red  , sizeof(int)   },
        {"led_white", ArgTypesInt , "0|1|2"    , &outer.led_white, sizeof(int)   },
        {"ptz"      , ArgTypesInt , "0|1|2|3|4", &outer.ptz      , sizeof(int)   },
        {"speed"    , ArgTypesInt , "32~63"    , &outer.speed    , sizeof(int)   },
        {"zoom"     , ArgTypesInt , "0|1|2"    , &outer.zoom     , sizeof(int)   },
        {"End"      , ArgTypesEnd , NULL       , NULL            , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)){
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }else if(!strcasecmp("set", action)){
        SET_PARAM_RULE_CHECK(opts);

        if(inner.enable != outer.enable){
            aging_test_process(AGTEST_ENABLE, outer.enable);
        }

        if(inner.ircut != outer.ircut){
            aging_test_process(AGTEST_IRCUT, outer.ircut);
        }

        if(inner.audio != outer.audio && outer.audio == 1) {
            aging_test_process(AGTEST_AUDIO, outer.audio);
        }

        if(inner.led_red != outer.led_red){
            aging_test_process(AGTEST_LED_RED, outer.led_red);
        }

        if(inner.led_white != outer.led_white){
            aging_test_process(AGTEST_LED_WHITE, outer.led_white);
        }

        if(outer.ptz){
            aging_test_process(AGTEST_PTZ, outer.ptz);
        }

        if(outer.speed){
            aging_test_process(AGTEST_PTZ_SPEED, outer.speed);
        }

        if(outer.zoom){
            aging_test_process(AGTEST_ZOOM, outer.zoom);
        }

        if(inner.keep != outer.keep){
            aging_test_process(AGTEST_KEEP, outer.keep);
        }
    }

    return ret;
}

static int JCPCmdAudioOutCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    AudioOutCfg inner = {0};
    AudioOutCfg outer = {0};
    char    action[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"             , "cmd describle"                                             },
        {"act"           , "list:获取所有参数 set:设置参数"                                      },
        {"enable"        , "音频输出开关:0-关,1-开"                                            },
        {"volumn"        , "音频输出音量:0~100"                                              },
        {"End"           , ""                                                          },
    };

    ArgOptS opts[] = {
        {"?"                , ARG_TYPE_ASK  , NULL      , NULL                 , 0             },
        {"act"              , ARG_TYPE_ACT  , "list|set", action               , sizeof(action)},
        {"enable"           , ArgTypesInt   , "0|1"     , &outer.enable        , sizeof(int)  },
        {"volumn"           , ArgTypesInt   , "0~100"   , &outer.volumn        , sizeof(int)  },
        {"End"              , ArgTypesEnd   , NULL      , NULL                 , 0             },
    };

    ret = get_config(handleAudioOutCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(AudioOutCfg));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)){
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strcasecmp("set", action)){
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(AudioOutCfg));
        DBG("===============1=============\n");
        ret = set_config(handleAudioOutCfg, outer);
    }

    return SUCCESS;
}

static int JCPCmdVgline(char *buf, int buflen, int argc, char **argv)
{
    int  ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    VglineS inner = {0};
    VglineS outer = {0};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"     , "cmd describle"                        },
        {"act"   , "list:read set:write"                  },
        {"x0"    , "x0"                                   },
        {"y0"    , "y0"                                   },
        {"x1"    , "x1"                                   },
        {"y1"    , "y1"                                   },
        {"dx0"   , "方向线段起点 x 坐标"                  },
        {"dy0"   , "方向线段起点 y 坐标"                  },
        {"dx1"   , "方向线段终点 x 坐标"                  },
        {"dy1"   , "方向线段终点 y 坐标"                  },
        {"k"     , "k，分别代表八种画线方式1~8"                      },
        {"enable", "开:1 关:0"                            },
        {"blink" , "开:1 关:0"                            },
        {"indoor", "0:室外 1:室内及线段与cam距离<6m:"     },
        {"thresh", "物体大小与画面占比，＜则忽略，默认5%" },
        {"dir"   , "0:B到A 1:A到B 2:双向都触发"                  },
        {"timestrategy", "布防时间描述\r\n"
         "\t        时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
         "\t        低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
         "\t        如全时段禁用：0,0,0,0,0,0,0\r\n"
         "\t        365中的格式是二元组的形式\r\n"
         "\t        timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"},
        {"level" , "VG级别，level=0不能代替enable=0来禁用VG功能"},
        {"End"   , "dir两点异侧时单身侦测，否则双向侦测"  },
    };

    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL         , 0                   },
        {"act"         , ARG_TYPE_ACT  , "list|set", action       , sizeof(action)      },
        {"x0"          , ArgTypesInt   , "0~1920"  , &outer.x0    , sizeof(outer.x0)    },
        {"y0"          , ArgTypesInt   , "0~1080"  , &outer.y0    , sizeof(outer.y0)    },
        {"x1"          , ArgTypesInt   , "0~1920"  , &outer.x1    , sizeof(outer.x1)    },
        {"y1"          , ArgTypesInt   , "0~1080"  , &outer.y1    , sizeof(outer.y1)    },
        {"dx0"         , ArgTypesInt   , "0~1920"  , &outer.dx0   , sizeof(outer.dx0)   },
        {"dy0"         , ArgTypesInt   , "0~1080"  , &outer.dy0   , sizeof(outer.dy0)   },
        {"dx1"         , ArgTypesInt   , "0~1920"  , &outer.dx1   , sizeof(outer.dx1)   },
        {"dy1"         , ArgTypesInt   , "0~1080"  , &outer.dy1   , sizeof(outer.dy1)   },
        {"k"           , ArgTypesInt   , "1~12"    , &outer.k     , sizeof(outer.k)     },
        {"enable"      , ArgTypesInt   , "0|1"     , &outer.enable, sizeof(outer.enable)},
        {"blink"       , ArgTypesInt   , "0|1"     , &outer.blink , sizeof(outer.blink) },
        {"indoor"      , ArgTypesInt   , "0|1"     , &outer.indoor, sizeof(outer.indoor)},
        {"dir"         , ArgTypesInt   , "0~2"       , &outer.dir   , sizeof(outer.dir   )},
        {"thresh"      , ArgTypesInt   , "0~100"   , &outer.thresh, sizeof(outer.thresh)},
        {"timestrategy", ArgTypesString, NULL      , timestrategy , sizeof(timestrategy)},
        {"level"       , ArgTypesInt   , "0~3"     , &outer.level , sizeof(outer.level) },
        {"End"         , ArgTypesEnd   , NULL      , NULL         , 0                   },
    };


    ret = get_config(handleVglineCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VglineS));

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        if (SUCCESS == arg_opt_if_set("timestrategy", opts)) {
            timestr_to_intarray(timestrategy, outer.times);
            printf("%u %u %u\n", outer.times[0], outer.times[1], outer.times[2]);
        }
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VglineS));
        ret = set_config(handleVglineCfg, outer);
    }
    return SUCCESS;
}

static int JCPCmdVgrect(char *buf, int buflen, int argc, char **argv)
{
    int  ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    VgrectS inner = {0};
    VgrectS outer = {0};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"     , "cmd describle"                            },
        {"act"   , "list:read set:write"                      },
        {"x0"    , "x0"                                       },
        {"y0"    , "y0"                                       },
        {"x1"    , "x1"                                       },
        {"y1"    , "y1"                                       },
        {"x2"    , "x2"                                       },
        {"y2"    , "y2"                                       },
        {"x3"    , "x3"                                       },
        {"y3"    , "y3"                                       },
        {"enable", "开:1 关:0"                                },
        {"blink" , "开:1 关:0"                            },
        {"indoor", "场景，室内:1 室外及线段与cam距离<6m:0"    },
        {"dir"   , "0:进入触发 1:离开触发 2:进入离开都触发"   },
        {"thresh", "物体大小阈值，小于被忽略，默认5%"         },
        {"timestrategy", "布防时间描述\r\n"
         "\t        时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
         "\t        低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
         "\t        如全时段禁用：0,0,0,0,0,0,0\r\n"
         "\t        365中的格式是二元组的形式\r\n"
         "\t        timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"},
        {"level" , "VG级别，level=0不能代替enable=0来禁用VG功能"},
        {"End"   , "p0~p3需要以顺时针方向排布，且p0在左上位置"},
    };

    ArgOptS opts[] = {
        {"?"           , ARG_TYPE_ASK  , NULL      , NULL         , 0                   },
        {"act"         , ARG_TYPE_ACT  , "list|set", action       , sizeof(action)      },
        {"x0"          , ArgTypesInt   , "0~1920"  , &outer.x0    , sizeof(outer.x0)    },
        {"y0"          , ArgTypesInt   , "0~1080"  , &outer.y0    , sizeof(outer.y0)    },
        {"x1"          , ArgTypesInt   , "0~1920"  , &outer.x1    , sizeof(outer.x1)    },
        {"y1"          , ArgTypesInt   , "0~1080"  , &outer.y1    , sizeof(outer.y1)    },
        {"x2"          , ArgTypesInt   , "0~1920"  , &outer.x2    , sizeof(outer.x2)    },
        {"y2"          , ArgTypesInt   , "0~1080"  , &outer.y2    , sizeof(outer.y2)    },
        {"x3"          , ArgTypesInt   , "0~1920"  , &outer.x3    , sizeof(outer.x3)    },
        {"y3"          , ArgTypesInt   , "0~1080"  , &outer.y3    , sizeof(outer.y3)    },
        {"enable"      , ArgTypesInt   , "0|1"     , &outer.enable, sizeof(outer.enable)},
        {"blink"       , ArgTypesInt   , "0|1"     , &outer.blink , sizeof(outer.blink) },
        {"indoor"      , ArgTypesInt   , "0|1"     , &outer.indoor, sizeof(outer.indoor)},
        {"dir"         , ArgTypesInt   , "0~2"     , &outer.dir   , sizeof(outer.dir   )},
        {"thresh"      , ArgTypesInt   , "0~100"   , &outer.thresh, sizeof(outer.thresh)},
        {"timestrategy", ArgTypesString, NULL      , timestrategy , sizeof(timestrategy)},
        {"level"       , ArgTypesInt   , "0~3"     , &outer.level , sizeof(outer.level) },
        {"End"         , ArgTypesEnd   , NULL      , NULL         , 0                   },
    };


    ret = get_config(handleVgrectCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(VgrectS));

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        intarray_to_timestr(timestrategy, inner.times);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        if ((outer.x0<outer.x1 && outer.y1<outer.y2 && outer.x2>outer.x3 && outer.y3>outer.y0) ||
            (outer.x0+outer.y0 +  outer.x1+outer.y1 +  outer.y2+outer.x2 +  outer.x3+outer.y3) == 0) {
            DBG("point set success\n");
        } else {
            sprintf(buf, "x0<x1 && y1<y2 && x2>x3 && y3>y0");
            return FAILURE;
        }
        if (SUCCESS == arg_opt_if_set("timestrategy", opts)) {
            timestr_to_intarray(timestrategy, outer.times);
        }

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(VgrectS));
        ret = set_config(handleVgrectCfg, outer);
    }
    return SUCCESS;
}

int JCPCmdPreSetCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    presetcfg inner = {0};
    presetcfg outer = {0};

    presetlist *z = NULL;
    HelpMsgS helps[] = {
        {"?"            , "预置点设置"                                         },
        {"act"          , "list:获取所有参数 set:设置参数"                        },
        {"gnum"         , "最多支持8个预置点"                                   },
        {"id"           , "预置点编号1-8"                                      },
        {"x"            , "预置点对应的横坐标 右加 左减"                        },
        {"y"            , "预置点对应的纵坐标 上加 下减"                        },
        {"isdefault"    , "是否是看守位(所有预置点只能有一个看守位)0，是；1,否" },
        {"enable"       , "预置点是否有效"                                       },
        {"name"         , "预置点名称"                                         },
        {"End"          , "presetcfg -?获取帮助"                                },
    };
    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK        , NULL                , NULL                , 0                 },
        {"act"      , ARG_TYPE_ACT        , "list|set"          , (void*)action       , sizeof(action)    },
        {"gnum"     , ARG_TYPE_LISTI      , "8"                 , (void*)&outer.gnum  , sizeof(int)       },
        {"id"       , ARG_TYPE_LIST_ID    , "0~2147483647"      , (void*)&z->id       , sizeof(int)       },
        {"x"        , ARG_TYPE_LIST_MEMINT, x_scope()           , (void*)&z->x        , sizeof(int)       },
        {"y"        , ARG_TYPE_LIST_MEMINT, y_scope()           , (void*)&z->y        , sizeof(int)       },
        {"isdefault", ARG_TYPE_LIST_MEMINT, "0|1"               , (void*)&z->isdefault, sizeof(int)       },
        {"enable"   , ARG_TYPE_LIST_MEMINT, "0|1"               , (void*)&z->enable   , sizeof(int)       },
        {"name"     , ARG_TYPE_LIST_MEMSTR, NULL                , (void*)&z->name     , sizeof(z->name)   },
        {"End"      , ArgTypesEnd         , NULL                , NULL                , 0                 },
    };
    ret = get_config(handlePreSetNewCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(presetcfg));

    JCP_ARG_PARSER(outer.preset, sizeof(presetlist));

    if (!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
        ASMJCP_LIST_COUNT(inner.preset, sizeof(presetlist), MAX_PRESET_NUM);
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);

        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(presetcfg));
        ret = set_config(handlePreSetNewCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return SUCCESS;
}

int JCPCmdLampCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    DaynightCfgS inner = {0};
    DaynightCfgS outer = {0};

    //presetlist *z = NULL;
    HelpMsgS helps[] = {
        {"?"         , "预置点设置"                    },
        {"act"       , "list:获取所有参数 set:设置参数"},
        {"mode"      , "固定阈值功能:1开，0关(jco灯板)"},
        {"type"      , "光敏类型:1硬光敏，0软光敏"},
        {"reverse"   , "红外灯高低电平反转，1:低.反转 0:高.不反转，默认0"},
        {"nighttoday", "夜晚模式，黑转彩阈值"          },
        {"daytonight", "白天模式，彩转黑阈值"          },
        {"nighttoday_mode1", "mode=1时：夜晚模式，黑转彩阈值"          },
        {"daytonight_mode1", "mode=1时：白天模式，彩转黑阈值"          },
        {"End"       , "lampcfg -?获取帮助"          },
    };
    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK, NULL      , NULL                    , 0             },
        {"act"       , ARG_TYPE_ACT, "list|set", (void*)action           , sizeof(action)},
        {"mode"      , ArgTypesInt , "0|1"     , (void*)&outer.mode      , sizeof(int)   },
        {"type"      , ArgTypesInt , "0|1"     , (void*)&outer.type      , sizeof(int)   },
        {"reverse"   , ArgTypesInt , "0|1"     , (void*)&outer.reverse   , sizeof(int)   },
        {"nighttoday", ArgTypesInt , "0~65535" , (void*)&outer.nighttoday, sizeof(int)   },
        {"daytonight", ArgTypesInt , "0~65535" , (void*)&outer.daytonight, sizeof(int)   },
        {"nighttoday_mode1", ArgTypesInt , "0~65535" , (void*)&outer.nighttoday_mode1, sizeof(int)   },
        {"daytonight_mode1", ArgTypesInt , "0~65535" , (void*)&outer.daytonight_mode1, sizeof(int)   },
        {"End"       , ArgTypesEnd , NULL      , NULL                    , 0             },
    };

    ret = get_config(handleDaynightCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(DaynightCfgS));
    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(DaynightCfgS));
        ret = set_config(handleDaynightCfg, outer);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.mode   , outer.mode    , "1 /cfg/daynight/mode %d\n"   , outer.mode   );
            CUSTOMCONF_APPEND_INT(inner.type   , outer.type    , "1 /cfg/daynight/type %d\n"   , outer.type   );
            CUSTOMCONF_APPEND_INT(inner.reverse, outer.reverse , "1 /cfg/daynight/reverse %d\n", outer.reverse);
        }
    }
    return ret;
}

static int JCPCmdMotor(char *buf, int buflen, int argc, char **argv)
{
    int  ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    motor_t inner = {0};
    motor_t outer = {0};

    HelpMsgS helps[] = {
        {"?"        , "Pelcod20cfg to chk org pos"  },
        {"act"      , "list:read set:write"         },
        {"type"     , "0:BW.yinuo 1:BW.zde 2:SL.zde"},
        {"reverse"  , "1：正常， 2:上下反 3：左右反 4：全反"},
        {"o_speed"  , "circle speed"                },
        {"o_seconds", "circle seconds"              },
        {"h_maxstep", "horizon max step 水平"            },
        {"v_maxstep", "vertical max step 垂直"           },
        {"h_speed"  , "horizon speed"               },
        {"v_speed"  , "vertical speed"              },
        {"h_startstep"  , "云台水平起始偏移步数:  默认= 50"              },
        {"v_startstep"  , "云台垂直起始偏移步数:  默认=120"              },
        {"waittime"  , "预置位巡航 驻留时间；单位为秒"              },
        {"seqtimes"  , "预置位巡航 扫描总周期次数"              },
        {"max_h_Angle"    , "云台水平最大旋转角度"           },
        {"max_v_Angle"    , "云台垂直最大旋转角度"           },
        {"max_h_ViewAngle", "镜头水平最大视角"               },
        {"max_v_ViewAngle", "镜头垂直最大视角"               },
        {"h_init_speed", "镜头自检水平速度"              },
        {"v_init_speed", "镜头自检垂直速度"              },
        {"End"      , ""                            },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK, NULL      , NULL            , 0                      },
        {"act"      , ARG_TYPE_ACT, "list|set", action          , sizeof(action)         },
        {"type"     , ArgTypesInt , "0~9"     , &outer.type     , sizeof(outer.type     )},
        {"reverse"  , ArgTypesInt , "1~4"     , &outer.reverse  , sizeof(outer.reverse  )},
        {"o_speed"  , ArgTypesInt , NULL      , &outer.o_speed  , sizeof(outer.o_speed)  },
        {"o_seconds", ArgTypesInt , NULL      , &outer.o_seconds, sizeof(outer.o_seconds)},
        {"h_maxstep", ArgTypesInt , NULL      , &outer.h_maxstep, sizeof(outer.h_maxstep)},
        {"v_maxstep", ArgTypesInt , NULL      , &outer.v_maxstep, sizeof(outer.v_maxstep)},
        {"h_speed"  , ArgTypesInt , NULL      , &outer.h_speed  , sizeof(outer.h_speed  )},
        {"v_speed"  , ArgTypesInt , NULL      , &outer.v_speed  , sizeof(outer.v_speed  )},
        {"h_startstep"  , ArgTypesInt , NULL  , &outer.h_Startstep  , sizeof(outer.h_Startstep  )},
        {"v_startstep"  , ArgTypesInt , NULL  , &outer.v_Startstep  , sizeof(outer.v_Startstep  )},
        {"waittime" , ArgTypesInt , "0~255"   , &outer.WaitTime , sizeof(outer.WaitTime )},
        {"seqtimes" , ArgTypesInt , NULL      , &outer.SeqTimes , sizeof(outer.SeqTimes )},
        {"max_h_Angle"      , ArgTypesInt , NULL      , &outer.max_h_Angle      , sizeof(outer.max_h_Angle) },
        {"max_v_Angle"      , ArgTypesInt , NULL      , &outer.max_v_Angle      , sizeof(outer.max_v_Angle) },
        {"max_h_ViewAngle"  , ArgTypesInt , NULL      , &outer.max_h_ViewAngle  , sizeof(outer.max_h_ViewAngle)},
        {"max_v_ViewAngle"  , ArgTypesInt , NULL      , &outer.max_v_ViewAngle  , sizeof(outer.max_v_ViewAngle)},
        {"End"      , ArgTypesEnd , NULL      , NULL            , 0                      },
    };


    ret = get_config(handleMotorCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(motor_t));

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.type           , outer.type           , "1 /cfg/motorcfg/type %d\n"           , outer.type           );
            CUSTOMCONF_APPEND_INT(inner.reverse        , outer.reverse        , "1 /cfg/motorcfg/reverse %d\n"        , outer.reverse        );
            CUSTOMCONF_APPEND_INT(inner.o_speed        , outer.o_speed        , "1 /cfg/motorcfg/o_speed %d\n"        , outer.o_speed        );
            CUSTOMCONF_APPEND_INT(inner.o_seconds      , outer.o_seconds      , "1 /cfg/motorcfg/o_seconds %d\n"      , outer.o_seconds      );
            CUSTOMCONF_APPEND_INT(inner.h_Startstep    , outer.h_Startstep    , "1 /cfg/motorcfg/h_startstep %d\n"    , outer.h_Startstep    );
            CUSTOMCONF_APPEND_INT(inner.v_Startstep    , outer.v_Startstep    , "1 /cfg/motorcfg/v_startstep %d\n"    , outer.v_Startstep    );
            CUSTOMCONF_APPEND_INT(inner.max_h_ViewAngle, outer.max_h_ViewAngle, "1 /cfg/motorcfg/max_h_ViewAngle %d\n", outer.max_h_ViewAngle);
            CUSTOMCONF_APPEND_INT(inner.max_v_ViewAngle, outer.max_v_ViewAngle, "1 /cfg/motorcfg/max_v_ViewAngle %d\n", outer.max_v_ViewAngle);
            CUSTOMCONF_APPEND_INT(inner.WaitTime       , outer.WaitTime       , "1 /cfg/motorcfg/waittime %d\n"       , outer.WaitTime       );
            CUSTOMCONF_APPEND_INT(inner.SeqTimes       , outer.SeqTimes       , "1 /cfg/motorcfg/seqtimes %d\n"       , outer.SeqTimes       );
            CUSTOMCONF_APPEND_INT(inner.h_maxstep      , outer.h_maxstep      , "1 /cfg/motorcfg/h_maxstep %d\n"      , outer.h_maxstep      );
            CUSTOMCONF_APPEND_INT(inner.v_maxstep      , outer.v_maxstep      , "1 /cfg/motorcfg/v_maxstep %d\n"      , outer.v_maxstep      );
            CUSTOMCONF_APPEND_INT(inner.h_speed        , outer.h_speed        , "1 /cfg/motorcfg/h_speed %d\n"        , outer.h_speed        );
            CUSTOMCONF_APPEND_INT(inner.v_speed        , outer.v_speed        , "1 /cfg/motorcfg/v_speed %d\n"        , outer.v_speed        );
        }
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(motor_t));
        ret = set_config(handleMotorCfg, outer);
    }
    return SUCCESS;
}

int JCPCmdFollowCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    int p = 0;
    int t = 0;
    int x = 0, y = 0, w = 0, h = 0;

    follow_info_t inner = {0};
    follow_info_t outer = {0};
    char timestrategy[128] = {0};

    HelpMsgS helps[] = {
        {"?"            , "预置点设置"                                                      },
        {"act"          , "list:获取所有参数 set:设置参数 auto: 自动校正，校正后回到看守位" },
        {"enable"       , "移动跟踪总使能"                                                  },
        {"humanenable"  , "人形跟踪分使能"                                                  },
        {"carenable"    , "车形跟踪分使能"                                                  },
        {"petenable"    , "宠物跟踪分使能"                                                  },
        {"screenenable" , "目标标记使能"                                                    },
        {"zoom"         , "跟踪变倍开关 0-关闭 1-开启"                                      },
        {"idle"         , "空闲多久时间调用"                                                },
        {"thresh"       , "灵敏度，暂不用"                                                  },
        {"preset"       , "看守位"                                                          },
        {"timestrategy" , "布防时间描述\r\n"
         "        时间字符串是一个“逗7分字符串”，一周七天，每个字符串段代表一天(从周日到周六)\r\n"
         "        低24位，每个位代表1个小时，从第0位到第23位分别代表0点到23点\r\n"
         "        如全时段禁用：0,0,0,0,0,0,0\r\n"
         "        365中的格式是二元组的形式\r\n"
         "        timestrategy= 0:0,1:0,2:0,3:0,4:0,5:0,6:0,;"                              },
        {"p"            , "垂直移动位置"                                                    },
        {"t"            , "水平移动位置"                                                    },
        {"reverse"      , "翻转状态 0 默认, 1 对角, 2 水平, 3 垂直"                         },
        {"x"            , "跟踪测试，目标 x 坐标"                                           },
        {"y"            , "跟踪测试，目标 y 坐标"                                           },
        {"w"            , "跟踪测试，目标宽"                                                },
        {"h"            , "跟踪测试，目标高"                                                },
        {"End"          , "followcfg -?获取帮助"                                            },
    };
    ArgOptS opts[] =
    {
        {"?"           , ARG_TYPE_ASK  , NULL                , NULL                      , 0                   },
        {"act"         , ARG_TYPE_ACT  , "list|set|auto|test", (void*)action             , sizeof(action)      },
        {"enable"      , ArgTypesInt   , "0|1"               , (void*)&outer.enable      , sizeof(int)         },
        {"humanenable" , ArgTypesInt   , "0|1"               , (void*)&outer.humanenable , sizeof(int)         },
        {"carenable"   , ArgTypesInt   , "0|1"               , (void*)&outer.carenable   , sizeof(int)         },
        {"petenable"   , ArgTypesInt   , "0|1"               , (void*)&outer.petenable   , sizeof(int)         },
        {"screenenable", ArgTypesInt   , "0|1"               , (void*)&outer.screenenable, sizeof(int)         },
        {"zoom"        , ArgTypesInt   , "0|1"               , (void*)&outer.zoom        , sizeof(int)         },
        {"thresh"      , ArgTypesInt   , "0~100"             , (void*)&outer.thresh      , sizeof(int)         },
        {"idle"        , ArgTypesInt   , "0~3600"            , (void*)&outer.idle        , sizeof(int)         },
        {"preset"      , ArgTypesInt   , "0~255"             , (void*)&outer.preset      , sizeof(int)         },
        {"timestrategy", ArgTypesString, NULL                , timestrategy              , sizeof(timestrategy)},
        {"p"           , ArgTypesInt   , "0~65535"           , (void*)&p                 , sizeof(int)         },
        {"t"           , ArgTypesInt   , "0~65535"           , (void*)&t                 , sizeof(int)         },
        {"reverse"     , ArgTypesInt   , "0~3"               , (void*)&outer.reverse     , sizeof(int)         },
        {"x"           , ArgTypesInt   , "0~65535"           , (void*)&x                 , sizeof(int)         },
        {"y"           , ArgTypesInt   , "0~65535"           , (void*)&y                 , sizeof(int)         },
        {"w"           , ArgTypesInt   , "0~65535"           , (void*)&w                 , sizeof(int)         },
        {"h"           , ArgTypesInt   , "0~65535"           , (void*)&h                 , sizeof(int)         },
        {"End"         , ArgTypesEnd   , NULL                , NULL                      , 0                   },
    };

    ret = get_config(handleFollowCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(follow_info_t));

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        unsigned short FollowReverse_Table[16] = {
            0, 3, 2, 1,
            2, 0, 1, 3,
            3, 1, 0, 2,
            0, 3, 2, 1
        };

        motor_t motor = {0};
        conf_get_motorcfg(&motor);
        ViInfoS videoInfos = {0};
        get_config(handleViinfoCfg, videoInfos);
        outer.reverse = FollowReverse_Table[(videoInfos.reverse*4 + motor.reverse -1)%16];

        MotorStatus status;
        get_motor_status(&status);
        p = status.steps[PAN_MOTOR];
        t = status.steps[TILT_MOTOR];

        intarray_to_timestr(timestrategy, inner.times);
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        timestr_to_intarray(timestrategy, outer.times);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(follow_info_t));

        if (SUCCESS == arg_opt_if_set("enable", opts)) {
            if (outer.enable) {
                outer.humanenable = 1;
                outer.petenable = 1;
            } else {
                outer.humanenable = 0;
                outer.petenable = 0;
            }
        }

        ret = set_config(handleFollowCfg, outer);
        if (get_g_sys(factest)) {
            CUSTOMCONF_APPEND_INT(inner.reverse, outer.reverse, "1 /cfg/followcfg/reverse %d\n", outer.reverse);
        }
    } else if(!strcasecmp("auto", action)) {
        // 隐私遮挡开启时，不响应云台校准
        videomask_plan_t videomask = {0,};
        conf_get_videomaskplan_cfg(&videomask);
        if (videomask.mask_enable) {
            return SUCCESS;
        }
        motor_reinit();
    } else if (!strcasecmp("test", action)) {
        if (x != 0 || y != 0 || w != 0 || h != 0) {
            ptz_follow_handing(0, 0, x, y, w, h);
        }

        if (p != 0 || t != 0) {
            debug_to_preset(p, t);
        }
    }

    return SUCCESS;
}

int JCPCmdFactoryCustomCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};

    int id = 0;
    char cmd[128] = {0};
    char path[128] = {0};
    char value[512] = {0};
    char custom_buf[1024] = {0};

    HelpMsgS helps[] = {
        {"?"            , "预置点设置"                                         },
        {"act"          , "list:获取所有参数 set:设置参数 rm 删除参数"                  },
        {"id"           , "序号"                                            },
        {"path"         , "配置路径"                                          },
        {"value"        , "数值"                                            },
        {"End"          , "customcfg -?获取帮助"                              },
    };
    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK          , NULL              , NULL              , 0                 },
        {"act"      , ARG_TYPE_ACT          , "list|set|rm"     , (void*)action     , sizeof(action)    },
        {"id"       , ArgTypesInt           , "0~10"            , &id               , sizeof(int)       },
        {"path"     , ArgTypesString        , NULL              , path              , sizeof(path)      },
        {"value"    , ArgTypesString        , NULL              , value             , sizeof(value)     },
        {"End"      , ArgTypesEnd           , NULL              , NULL              , 0                 },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        ;
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);

        char *str = strstr(path, "/cfg/");
        if (NULL == str) {
            ERR("cfg fail\n");
            SYSLOG("cfg fail path = %s\n", path);
            return FAILURE;
        }

#if     defined(LGTBOARD_WHT) || defined(LGTBOARD_INF)
        if (!strcmp(path, "/cfg/lightextcfg/lightboard")) {
            ERR("device don't support setting up lightboard\n");
            return FAILURE;
        }
#endif

#ifdef STEPLESS_PWM
        if (!strcmp(path, "/cfg/lightextcfg/lightboard")) {
            if (value[0] != '1') {
                ERR("device don't support setting up ir\n");
                return FAILURE;
            }
        }
#endif

        sprintf(custom_buf, "%d  %s  %s\n", id, path, value);
        AppendFile(CUSTOM_CONF, custom_buf);
        SYSLOG("custom: %s\n", custom_buf);
        sync();
    } else if(!strcasecmp("rm", action)) {
        // 一次上电，多次产测可继承
        static int removed = FALSE;
        if (!removed) {
            removed = TRUE;
            snprintf(cmd , sizeof(cmd), "sed -i '\\|/cfg/devinfo/custom_appid|!d' %s", CUSTOM_CONF);
            UtilSystemCmd(cmd);
            SYSLOG("remove [ %s ]\n", CUSTOM_CONF);
            sync();
        }
    }

    return SUCCESS;
}

static int JCPCmdOnvifInfoCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    OnvifInfoCfg inner = {0};
    OnvifInfoCfg outer = {0};

    HelpMsgS helps[] = {
        {"?"            , "Onvif Info配置"                  },
        {"act"          , "list:获取所有参数 set:设置参数"  },
        {"name"         , "制造商"                          },
        {"hardware"     , "型号"                            },
        {"searchable"   , "搜索使能"                        },
        {"End"          , "onvifinfocfg -?获取帮助"         },
    };

    ArgOptS opts[] =
    {
        {"?"            , ARG_TYPE_ASK  , NULL      , NULL              , 0                         },
        {"act"          , ARG_TYPE_ACT  , "list|set", action            , sizeof(action)            },
        {"name"         , ArgTypesString, NULL      , outer.name        , sizeof(outer.name)        },
        {"hardware"     , ArgTypesString, NULL      , outer.hardware    , sizeof(outer.hardware)    },
        {"searchable"   , ArgTypesInt   , "0~1"     , &outer.searchable , sizeof(outer.searchable)  },
        {"End"          , ArgTypesEnd   , NULL      , NULL              , 0                         },
    };

    ret = get_config(handleOnvifInfoCfg, inner);

    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(OnvifInfoCfg));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list")))
    {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    else if(!strncasecmp("set", action,strlen("set")))
    {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(OnvifInfoCfg));
        ret = set_config(handleOnvifInfoCfg, outer);
    }

    return SUCCESS;
}

int JCPCmdSim4g(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    char cmd[128] = {0};
    Sim4g inner = {0};
    Sim4g outer = {0};

    HelpMsgS helps[] = {
        {"?"            , "4G状态设置"                         },
        {"act"          , "list:获取所有参数 set:设置参数"     },
        {"sim"          , "1 sim是存在; 0 不存在 "             },
        {"e_sim"        , "1 e_sim是存在; 0 不存在"            },
        {"online"       , "1 正常模式; 0 飞行模式"             },
        {"is4G"         , "1 4G模式; 0 3G; -1 异常"            },
        {"esim_is4G"    , "1 4G模式; 0 3G; -1 异常"            },
        {"dbm"          , "信号强度，负数"                     },
        {"esim_dbm"     , "信号强度，负数"                     },
        {"signal"       , "信号强度，百分比"                   },
        {"connected"    , "1 已拨号连接，0 未连接"             },
        {"txBpsec"      , "上行速度"                           },
        {"rxBpsec"      , "下行速度"                           },
        {"ip"           , "ip地址"                             },
        {"iccid"        , "sim卡标识"                          },
        {"esim_iccid"   , "sim卡标识"                          },
        {"imsi"         , "运营商识别sim卡号"                  },
        {"fw_version"   , "4g软件版本"                         },
        {"spnname"      , "运营商商标"                         },
        {"imei"         , "设备标识id"                         },
        {"cmd"          , "要查询的AT指令"                     },
        {"fdd"          , "动态打开锁定FDD 0.自动 1.fdd 2.tdd" },
        {"is_fdd"       , "当前 0.未驻网, 1.fdd, 2.tdd"        },
        {"card"         , "用户运营商配置选择，默认 1-电信"    },
        {"operators"    , "当前使用运营商，0-无 1-电信 2-移动" },
        {"sim_status"   , "双机贴卡状态，0-异常 1-正常"        },
        {"sim_card"     , "双机贴卡，移动卡号"                 },
        {"esim_card"    , "双机贴卡，电信卡号"                 },
        {"report_status", "双机贴卡上报状态"                },
        {"token"        , "4G 基站定位访问服务器的密钥"        },
        {"longitude"    , "经度             "            },
        {"latitude"     , "纬度             "            },
        {"End"          , "sim4g -?获取帮助"              },
    };

    ArgOptS opts[] = {
        {"?"            , ARG_TYPE_ASK  , NULL   , NULL                , 0                       },
        {"act"          , ARG_TYPE_ACT  , "list|set|reflesh", action   , sizeof(action)          },
        {"sim"          , ARG_TYPE_LISTI, "0|1"  , &outer.sim          , sizeof(int)             },
        {"e_sim"        , ARG_TYPE_LISTI, "0|1"  , &outer.e_sim        , sizeof(int)             },
        {"online"       , ArgTypesInt   , "0|1"  , &outer.online       , sizeof(int)             },
        {"is4G"         , ARG_TYPE_LISTI, "0|1"  , &outer.is4G         , sizeof(int)             },
        {"esim_is4G"    , ARG_TYPE_LISTI, "0|1"  , &outer.esim_is4G    , sizeof(int)             },
        {"dbm"          , ARG_TYPE_LISTI, "0|1"  , &outer.dbm          , sizeof(int)             },
        {"esim_dbm"     , ARG_TYPE_LISTI, "0|1"  , &outer.esim_dbm     , sizeof(int)             },
        {"signal"       , ARG_TYPE_LISTI, "0~100", &outer.signal       , sizeof(int)             },
        {"connected"    , ARG_TYPE_LISTI, "0|1"  , &outer.connected    , sizeof(int)             },
        {"txBpsec"      , ARG_TYPE_LISTI, "0|1"  , &outer.txBpsec      , sizeof(int)             },
        {"rxBpsec"      , ARG_TYPE_LISTI, "0|1"  , &outer.rxBpsec      , sizeof(int)             },
        {"ip"           , ARG_TYPE_LISTS, NULL   , &outer.ip           , sizeof(outer.ip)        },
        {"iccid"        , ARG_TYPE_LISTS, NULL   , &outer.iccid        , sizeof(outer.iccid)     },
        {"esim_iccid"   , ARG_TYPE_LISTS, NULL   , &outer.esim_iccid   , sizeof(outer.esim_iccid)},
        {"imsi"         , ARG_TYPE_LISTS, NULL   , &outer.imsi         , sizeof(outer.imsi)      },
        {"fw_version"   , ARG_TYPE_LISTS, NULL   , &outer.fw_version   , sizeof(outer.fw_version)},
        {"spnname"      , ARG_TYPE_LISTS, NULL   , &outer.spnname      , sizeof(outer.spnname)   },
        {"imei"         , ARG_TYPE_LISTS, NULL   , &outer.imei         , sizeof(outer.imei)      },
        {"cmd"          , ARG_TYPE_SETS , NULL   , &cmd                , sizeof(cmd)             },
        {"fdd"          , ArgTypesInt   , "0~2"  , &outer.fdd          , sizeof(int)             },
        {"is_fdd"       , ArgTypesInt   , "0~2"  , &outer.is_fdd       , sizeof(int)             },
        {"card"         , ArgTypesInt   , "1|2|3", &outer.card         , sizeof(int)             },
        {"operators"    , ArgTypesInt   , "0~2"  , &outer.operators    , sizeof(int)             },
        {"sim_status"   , ArgTypesInt   , "-1~1" , &outer.sim_status   , sizeof(int)             },
        {"sim_card"     , ARG_TYPE_LISTS, NULL   , &outer.sim_card     , sizeof(outer.sim_card)  },
        {"esim_card"    , ARG_TYPE_LISTS, NULL   , &outer.esim_card    , sizeof(outer.esim_card) },
        {"report_status", ArgTypesInt   , "-1~7" , &outer.report_status, sizeof(int)             },
        {"token"        , ARG_TYPE_LISTS, NULL   , &outer.token        , sizeof(outer.token)     },
        {"longitude"    , ARG_TYPE_LISTS, NULL   , &outer.longitude    , sizeof(outer.longitude) },
        {"latitude"     , ARG_TYPE_LISTS, NULL   , &outer.latitude     , sizeof(outer.latitude)  },
        {"End"          , ArgTypesEnd   , NULL   , NULL                , 0                       },
    };

    static struct timespec ts = {0,};

    ret = sim4g_get_stat(&inner);
    Sim4g sim4gcfg = {0};
    ret = get_config(handleSim4gCfg, sim4gcfg);
    inner.card = sim4gcfg.card;
    if (strlen(inner.token) <= 0) {
        strncpy(inner.token, sim4gcfg.token, sizeof(inner.token));
    }
    RETURN_FAIL_IF_API_ERR(ret);
    memcpy(&outer, &inner, sizeof(inner));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action, strlen("list"))) {
        if (get_g_sys(factest) && get_g_stat(wifi, WIFI_DHCPONCE)) {
            outer.connected = outer.online = (platform_on_line() || outer.online);
            outer.is4G = outer.esim_is4G = TRUE;
            outer.dbm = outer.esim_dbm = MAX(MAX(outer.dbm, 3), outer.esim_dbm);
        }
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", action, strlen("set"))) {
        if (SUCCESS == arg_opt_if_set("fdd", opts)) {
            set_config(handleSim4gCfg, outer);
        }

        if (SUCCESS == arg_opt_if_set("card", opts)) {
            SET_PARAM_RULE_CHECK(opts);
            ret = set_config(handleSim4gCfg, outer);
        }
    } else if (!strncasecmp("reflesh", action, strlen("reflesh"))) {
        if (ms_clock_is_timeup(&ts, 60*1000) < 0) {
            DBG("Refresh interval is too short\n");
            snprintf(buf, buflen, "try_again_later");
            return ret;
        }
        send_event(JEvent_Sim4gLocation);
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    return ret;
}

static int JCPCmdPrivCtrl(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    priv_ctrl_t inner = {0};
    priv_ctrl_t outer = {0};

    HelpMsgS helps[] = {
        {"?"     , "私有控制"                            },
        {"act"   , "list:获取所有参数 set:设置参数"      },
        {"video" , "阿里出流允许 1/0"                    },
        {"End"   , "privctrl -?获取帮助"                 },
    };

    ArgOptS opts[] = {
        {"?"     , ARG_TYPE_ASK, NULL      , NULL         , 0             },
        {"act"   , ARG_TYPE_ACT, "list|set", (void*)action, sizeof(action)},
        {"video" , ArgTypesInt , "0|1"     , &outer.video , sizeof(int)   },
        {"End"   , ArgTypesEnd , NULL      , NULL         , 0             },
    };

    ret = get_config(handlePrivCtrlCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);
    memcpy(&outer, &inner, sizeof(priv_ctrl_t));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(priv_ctrl_t));
        ret = set_config(handlePrivCtrlCfg, outer);
    }

    return ret;
}

static int JCPCmdQueryRecord(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    char month[8];
    char path[128] = {0};

    HelpMsgS helps[] = {
        {"?"         , "按月查询存储设备录像"           },
        {"act"       , "list:获取所有参数"              },
        {"month"     , "查询某月的录像,格式：202101"    },
        {"End"       , "queryrecord -?获取帮助"         },//命令：ccli queryrecord -act list -month 202101
    };

    ArgOptS opts[] = {
        {"?"         , ARG_TYPE_ASK                      , NULL       , NULL              , 0               },
        {"act"       , ARG_TYPE_ACT                      , "list"     , (void*)action     , sizeof(action)  },
        {"month"     , ArgTypesString | ArgTypesSetMust  , NULL       , month             , sizeof(month)   },
        {"End"       , ArgTypesEnd                       , NULL       , NULL              , 0               },
    };

    JCP_ARG_PARSER(NULL, 0);
    SET_PARAM_RULE_CHECK(opts);

    if (!strcasecmp("list", action)) {
        if (storage_get_mmcpath(path) < 0) {
            return FAILURE;
        }

        if (record_scan_days_fast("/mnt/IPCamera", month, buf, buflen) > 0) {
            DBG("The date which have record is %s\r\n",buf);
        } else {
            DBG("The date which have record is empty\r\n");
        }
    }

    return ret;
}

int JCPCmdAppmsg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    int ota = 0;

    HelpMsgS helps[] = {
        {"?"        , "app 主动下发的消息"                                     },
        {"act"      , "list:获取进度， set: 设置升级类型"                          },
        {"ota"      , "app 是否主动触发 ota 升级, 0:否，1:是"                      },
        {"End"      , "appmsg -? 获取帮助"                                  },
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK   , NULL       , NULL          , 0                 },
        {"act"      , ArgTypesString , "list|set" , (void*)action , sizeof(action)    },
        {"ota"      , ArgTypesInt    , "0|1"      , &ota          , sizeof(ota)       },
        {"End"      , ArgTypesEnd    , NULL       , NULL          , 0                 },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action, strlen("list"))) {
        ota = is_okey(F_MANUAL_OTA) ? 1 : 0;
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action, strlen("set"))) {
        if (ota) {
            TouchFile(F_MANUAL_OTA);
        } else {
            remove(F_MANUAL_OTA);
        }
        SET_PARAM_RULE_CHECK(opts);
    }

    return ret;
}

static int JCPCmdAging8h(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char    action[JCP_ACTION_LEN] = {0};
    int pass = 0;

    HelpMsgS helps[] = {
        {"?"   , "老化测试控制命令"              },
        {"act" , "list:获取所有参数 set:设置参数"},
        {"pass", "老化时长，单位: h"             },
        {"End" , "aging8h -? 获取帮助"           },
    };

    ArgOptS opts[] = {
        {"?"   , ARG_TYPE_ASK, NULL  , NULL  , 0             },
        {"act" , ARG_TYPE_ACT, "list", action, sizeof(action)},
        {"pass", ArgTypesInt , "0~8" , &pass , sizeof(int)   },
        {"End" , ArgTypesEnd , NULL  , NULL  , 0             },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)){
        pass = get_aging8h() % BURNED_SIGN;
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }

    return ret;
}

static int JCPCmdDevCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;

    DevConfS inner = {0};
    DevConfS outer = {0};
    char    action[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"         , "物模型属性上报设置"              },
        {"act"       , "list:获取所有参数 set:设置参数"  },
        {"devicebind", "设备绑定状态 0:未绑定 1:已绑定"  },
        {"definition", "清晰度 0:流畅 1:高清 2:超清"     },
        {"End"       , "devcfg -? 获取帮助"             },
    };

    ArgOptS opts[] = {
        {"?"         , ARG_TYPE_ASK, NULL      , NULL             , 0             },
        {"act"       , ARG_TYPE_ACT, "list|set", action           , sizeof(action)},
        {"devicebind", ArgTypesInt , "0|1"     , &outer.devicebind, sizeof(int)   },
        {"definition", ArgTypesInt , "0|1|2"   , &outer.definition, sizeof(int)   },
        {"End"       , ArgTypesEnd , NULL      , NULL             , 0             },
    };

    ret = get_config(handleDevConf, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(DevConfS));

    JCP_ARG_PARSER(NULL, 0);

    if (!strcasecmp("list", action)) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strcasecmp("set", action)) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(DevConfS));
        ret = set_config(handleDevConf, outer);
        if (SUCCESS == arg_opt_if_set("devicebind", opts)) {
            if (outer.devicebind && get_g_sys(usb_4g)) {
                encode_audio_queue_push_amr(AUDIO_SIM4G_BIND_SUCCESS, FALSE);
            }
        }
    }

    return ret;
}

int JCPCmdRecplan(char *buf, int buflen, int argc, char **argv)
{
    struct recplan {
        int     enable;
        char    week[32];
        char    start_time[12];
        char    end_time[12];
    };

    char action[JCP_ACTION_LEN+8] = {0};

    struct recplan inner = {0,};
    struct recplan outer = {0,};
    int event_en = 0;

    HelpMsgS helps[] = {
        {"?"         , "时间设置"                                                       },
        {"act"       , "list: 获取时间列表  set : 设置时间 poweron_sync: 低成本上电同步"},
        {"enable"    , "使能开关"                                                       },
        {"event_en"  , "写时1有效，清布防时间；读时{1|0}代表开启或关闭"                 },
        {"week"      , "0,1,0,1,0,1,0 格式的逗分7位数字串 第1位代表周日"                },
        {"start_time", "start 06:08:08"                                                 },
        {"end_time"  , "end 20:08:08"                                                   },
        {"End"       , "recplan -? 获取帮助"                                            },
    };

    ArgOptS opts[] = {
        {"?"         , ARG_TYPE_ASK  , NULL      , NULL            , 0                       },
        {"act"       , ARG_TYPE_ACT  , "list|set", (void*)action   , sizeof(action)          },
        {"enable"    , ArgTypesInt   , "0|1"     , &outer.enable   , sizeof(outer.enable    )},
        {"event_en"  , ArgTypesInt   , "0|1"     , &event_en       , sizeof(int)             },
        {"week"      , ArgTypesString, NULL      , outer.week      , sizeof(outer.week      )},
        {"start_time", ArgTypesString, NULL      , outer.start_time, sizeof(outer.start_time)},
        {"end_time"  , ArgTypesString, NULL      , outer.end_time  , sizeof(outer.end_time  )},
        {"End"       , ArgTypesEnd   , NULL      , NULL            , 0                       },
    };

    {   /* read */
        int plan_num = 0;

        sRecPlan recplan[2] = {{0}};
        record_get_plan(recplan, &plan_num);

        inner.enable = plan_num ? 1 : 0;
        if (plan_num <= 1) {
            memcpy(inner.start_time, recplan[0].start_time, sizeof(inner.start_time));
            memcpy(inner.end_time, recplan[0].end_time, sizeof(inner.end_time));
        } else {
            memcpy(inner.start_time, recplan[0].start_time, sizeof(inner.start_time));
            memcpy(inner.end_time, recplan[1].end_time, sizeof(inner.end_time));
        }

        int i;
        int w[7] = {0};
        for (i = 0; plan_num && i < 7; i++) {
            if (recplan[0].week[i]) {
                w[recplan[0].week[i]%7] = 1;
            }
        }
        sprintf(inner.week,
                "%d,%d,%d,%d,%d,%d,%d", w[0], w[1], w[2], w[3], w[4], w[5], w[6]);

        memcpy(&outer, &inner, sizeof(outer));
    }

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        event_en = !outer.enable;       // 二才必开其一
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        if (SUCCESS == arg_opt_if_set("event_en", opts) && event_en == 1) {
            outer.enable = 0;
        }

        int i, iw = 0;
        int w[7] = {0};

        sRecPlan recplan = {0};
        sscanf((const char *)outer.week, "%d,%d,%d,%d,%d,%d,%d",
            &w[0], &w[1], &w[2], &w[3], &w[4], &w[5], &w[6]);

        for(i = 0; outer.enable && i < 7; i++){
            if(w[i]){
                recplan.week[iw] = i ? i :7;
                iw++;
            }
        }

        if(atoi(outer.end_time) >= atoi(outer.start_time)){
            DBG("only 1\n");
            /* clean global data */
            recplan.status = 0; recplan.record_no = 2;
            memcpy(recplan.start_time, outer.start_time, sizeof(outer.start_time));
            memcpy(recplan.end_time  , outer.end_time  , sizeof(outer.end_time));
            recplan.week_count = iw;
            record_set_plan(&recplan);

            /* set */
            recplan.status = 1; recplan.record_no = 1;
            record_set_plan(&recplan);
        }else{
            DBG("have 2\n");
            /* 跨天 1st */
            /* modify global data */
            recplan.status = 1; recplan.record_no = 2;
            memcpy(recplan.start_time, "00:00:00",sizeof(outer.start_time));
            memcpy(recplan.end_time  , outer.end_time  , sizeof(outer.end_time));
            recplan.week_count = iw;
            record_set_plan(&recplan);

            /* 跨天 2nd */
            recplan.status = 1; recplan.record_no = 1;

            memcpy(recplan.start_time, outer.start_time, sizeof(outer.start_time));
            memcpy(recplan.end_time  , "23:59:00", sizeof(outer.end_time));
            recplan.week_count = iw;
            record_set_plan(&recplan);
        }
    }
    return SUCCESS;
}

int JCPCmdSearchFileCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    int len = 0;
    int pos = 0;

    int type;
    int suffix;
    int research;
    int itemnum;
    int complete = 0;
    char starttime[32] = {0};
    char endtime[32] = {0};

    struct tm tm_start = {0};
    struct tm tm_end = {0};

    HelpMsgS helps[] = {
        {"?"   , "按条件搜索文件，并分段输出"},
        {"act" , "list:设置搜索条件"},
        {"type", "文件类型\r\n"
                 "\t0 所有文件     /\r\n"
                 "\t1 定时触发文件 S\r\n"
                 "\t2 手动触发文件 H\r\n"
                 "\t3 手动录像     M\r\n"
                 "\t4 报警录像     A"},
        {"suffix"   , "文件类型 2:mp4文件  1: jpg文件  0 任何文件"},
        {"pos",  "录像记录查询位置"},
        {"research" , "从头搜索标志：1，从头搜索，0，继承搜索"},
        {"itemnum"  , "输入时为请求数目，[index ~ index+num-1]\r\n"
                 "\t输出时为实际返回数目"},
        {"starttime", "搜索开始时间，格式如：2014-04-09 14:50:23"},
        {"endtime"  , "搜索结束时间，格式如：2014-04-09 14:50:23 "},
        {"End"      , "命令:searchfilecfg -act list -research 1 -type 0 -suffix 0 -starttime “2014-04-24 17:02:00” -endtime “2014-04-25 11:15:39” -itemnum 20\r\n"
                 "\t是指要从头搜索，时间段在上述时间之内的所有文件\r\n"
                 "\t返回字符串 mp4dir   包含日期的路径文件\r\n"
                 "\t   complete 后面值为搜索完成标志：1，未完成，0，完成\r\n"},
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK    , NULL  , NULL     , 0                },
        {"act"      , ARG_TYPE_ACT    , "list" , action   , sizeof(action)   },
        {"type"     , ArgTypesInt     , "0~5" , &type    , sizeof(int)      },
        {"pos"      , ArgTypesInt     , "0~9999" , &pos   , sizeof(int)      },
        {"suffix"   , ARG_TYPE_MUSTINT, "0~2" , &suffix  , sizeof(int)      },
        {"research" , ARG_TYPE_MUSTINT, "0|1" , &research, sizeof(int)      },
        {"itemnum"  , ARG_TYPE_MUSTINT, "1~50", &itemnum , sizeof(int)      },
        {"starttime", ARG_TYPE_ACT    , NULL  , starttime, sizeof(starttime)},
        {"endtime"  , ARG_TYPE_ACT    , NULL  , endtime  , sizeof(endtime)  },
        {"End"      , ArgTypesEnd     , NULL  , NULL     , 0                },
    };

    JCP_ARG_PARSER(NULL, 0);
    SET_PARAM_RULE_CHECK(opts);

    int lineno = 0;
    int64_t start_utc = 0 ;
    //int64_t end_utc = 0 ;
    char mp4dir[128] = {0};
    int i = 0;
    int j = 0;
    int got_num = 0;
    char recalarm_flag[1440 + 1] = {0};
    char temp[4*1024] = {0};

    int temp_len = 0;
    static int i_last = 0;

    sRec1File *olist = NULL;
    olist = (sRec1File *)calloc(MAX_RECS_OF_DAY, sizeof(sRec1File));

    if (NULL == olist) {
        lineno = __LINE__; goto __errfmt;
    }

    if (0 != strlen(starttime)) {
        if (str_to_tm(starttime, &tm_start) < 0) {
            lineno = __LINE__; goto __errfmt;
        }
    } else {
        lineno = __LINE__; goto __errfmt;
    }

    if (0 != strlen(endtime)) {
        if (str_to_tm(endtime, &tm_end) < 0) {
            lineno = __LINE__; goto __errfmt;
        }
    } else {
        lineno = __LINE__; goto __errfmt;
    }

    start_utc = mktime(&tm_start);
    record_get_path_of_ymd(record_get_ymd_of_epoch(start_utc), mp4dir, sizeof(mp4dir), 0);
    got_num = record_query_list(start_utc, MAX_RECS_OF_DAY, olist, 0);
    if (SUCCESS == arg_opt_if_set("pos", opts)) {
        i = (research) ? 0 : pos;
    } else {
        i = (research) ? 0 : i_last;
    }

    for (; j < MAX_RECS_OF_DAY/*100 every qurey*/ && i < got_num; i++) {//MAX_NR_REC_PER_DAY
        if (type == 0/*All*/) {
        } else if (type == 1/*S*/) {
        } else if (type == 4/*A*/) {
        } else if (type == 5/*录像下载不返回9999文件*/) {
            if (olist[i].is_tmp_file == TRUE) {
                continue;
            }
        }

        if (olist[i].file_name[0] == '\0') {
            continue;
        }

        if (olist[i].is_tmp_file == TRUE) {
            record_set_len_of_tmpfile(&olist[i]);
        }

        j++;
        len += sprintf(buf+len, "%s#", olist[i].file_name);
        //此处修改针对热点连接时查询录像文件少数据的问题。由于jcp命令处理最大量只能是2048个字符，
        //120 是下面头的长度。
        //60*24=1440
        if (len > buflen-120-1440) {
            DBG("________________________________ buflen is limitted to %d, please resize for [%d/%d]\n", buflen, i, got_num);
            i++;
            break;
        }
    }

    DBG("j:%d, got_num:%d, i:%d, len:%d buflen:%d \n", j, got_num, i, len, buflen);

    if (i >= got_num) {
        complete = 0;
    } else {
        complete = 1;
    }
    pos = i_last = i;
    get_recalarm_flag(record_get_ymd_of_epoch(start_utc), recalarm_flag, sizeof(recalarm_flag) - 1);
    recalarm_flag[sizeof(recalarm_flag) - 1] = '\0';
    temp_len = sprintf(temp, "pos=%d;complete=%d;itemnum=%d;mp4dir=%s;recflag=%s;resultlist=", pos, complete, j, mp4dir, recalarm_flag);

    memmove(buf+temp_len, buf, len);
    memcpy(buf, temp, temp_len);
    strcat(buf, ";"); // buf[temp_len+len] = ';';

    if (olist) {
        free(olist);
        olist = NULL;
    }

    return SUCCESS;

__errfmt:
    if (olist) {
        free(olist);
        olist = NULL;
    }
    sprintf(buf, "format error @%d\n", lineno);
    return FAILURE;
}

#if defined(PLATFORM_TENCENT)
static int JCPCmdTencentConfCfg(char *buf, int buflen, int argc, char **argv)
{
    int write_len = 0;
    int ret       = SUCCESS;
    FILE *fp      = NULL;
    char id_buf[256]        = {0};
    TripleInfoS triple_cfg = {0};
    char action[JCP_ACTION_LEN] = {0};

    HelpMsgS helps[] = {
        {"?"             , "腾讯配置文件设置"                                  },
        {"act"           , "list:获取三元组信息和连接状态 set:设置参数 lspk:获取设备PK"},
        {"product_key"   , "三元组key"                                    },
        {"device_name"   , "设备名称"                                      },
        {"device_secret" , "设备密钥"                                      },
        {"End"           , "txconfcfg -? 获取帮助"                         },
    };

    ArgOptS opts[] =
    {
        {"?"               , ARG_TYPE_ASK    , NULL            , NULL            , 0                     },
        {"act"             , ARG_TYPE_ACT    , "lspk|list|set|clean", action          , sizeof(action)        },
        {"product_key"     , ArgTypesString  , NULL            , triple_cfg.product_key     , sizeof(triple_cfg.product_key)   },
        {"device_name"     , ArgTypesString  , NULL            , triple_cfg.device_name     , sizeof(triple_cfg.device_name)   },
        {"device_secret"   , ArgTypesString  , NULL            , triple_cfg.device_secret   , sizeof(triple_cfg.device_secret) },
        {"End"             , ArgTypesEnd     , NULL            , NULL            , 0                     },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strcasecmp("list", action)) {
        tencent_get_conf_info(buf, buflen);
        return SUCCESS;
    } else if(!strcasecmp("lspk", action)) {
        DBG("PK = %s\n",PK);
        return SUCCESS;
    } else if(!strcasecmp("set", action)) {

        if(0 != strcmp(PK, triple_cfg.product_key)) {
            sprintf(buf, "pk=%s\n", PK);
            SYSLOG("product key %s mismatching inner: %s\n", triple_cfg.product_key, PK);
            return FAILURE;
        }

        if (access(F_P2P_TRIPLE, F_OK) == 0) {
            sprintf(buf, "tx.conf is exist, will return failure");
            SYSLOG("tx.conf is exist, will return failure\n");
            return FAILURE;
        }

        fp = fopen(F_P2P_TRIPLE, "wb+");
        if (fp == NULL) {
            sprintf(buf, "open [%s] fail\n", F_P2P_TRIPLE);
            SYSLOG("open fail,product_key = %s\n", triple_cfg.product_key);
            return FAILURE;
        }

        int len = sprintf(id_buf, "%s;%s;%s;", triple_cfg.product_key, triple_cfg.device_name, triple_cfg.device_secret);
        write_len = Writefully(fileno(fp), id_buf, len);
        if (write_len != len) {
            sprintf(buf,"write fail,write len = %d, ali_len = %d, product_key = %s\n", write_len, len, triple_cfg.product_key);
            SYSLOG("write fail,write len = %d, ali_len = %d, product_key = %s\n", write_len, len, triple_cfg.product_key);
            fclose(fp);
            return FAILURE;
        }
        if (Security_HardWare == system_get_security_type()) {
            sprintf(buf,"not support Security_HardWare!\n");
            SYSLOG("not support Security_HardWare\n");
            return FAILURE;
        } else {
            if (strlen(triple_cfg.product_key) <= 0 || strlen(triple_cfg.device_name) <= 0 || strlen(triple_cfg.device_secret) <= 0) {
                sprintf(buf,"aliconf strlen small than zero!\n");
                SYSLOG("aliconf strlen small than zero!\n");
                return FAILURE;
            }

            if (strlen(triple_cfg.product_key) > 20 || strlen(triple_cfg.device_name) > 32 || strlen(triple_cfg.device_secret) > 64) {
                sprintf(buf,"aliconf strlen bigger than max!\n");
                SYSLOG("aliconf strlen bigger than max!\n");
                return FAILURE;
            }
            SYSLOG("settings txconf to %s\n", id_buf);
            if (uboot_txconf_set((char *)"txconf", id_buf) != SUCCESS) {
                sprintf(buf,"uboot_txconf_set txconf failed\n");
                LOG("uboot_txconf_set txconf failed\n");
                DBG("uboot_txconf_set txconf failed\n");
                return FAILURE;
            }
            fflush(fp);
            fsync(fileno(fp));
            fclose(fp);
        }
    }
    log_sync();
    return ret;
}
#endif

#ifndef __BURN_DEV__
#define __BURN_DEV__ "not_define_burndev"
#endif
#define BURN_DEV XSTR(__BURN_DEV__)

/*
 * 此JCP专门用于获取CPUID
*/
int JCPCmdBurninfoCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    char cpuid[32] = {0};
    char devtype[32] = {0};

    HelpMsgS helps[] = {
        {"?"            , "获取烧录ID需要的相关信息"                          },
        {"act"          , "list:获取所有参数 set:设置参数"            },
        {"cpuid"        , "芯片cpuid"                          },
        {"devtype"      , "申请烧录ID的devtype"                          },
        {"End"          , "JCPCmdBurninfoCfg -?获取帮助"            },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK  , NULL      , NULL           , 0               },
        {"act"       , ARG_TYPE_ACT  , "list|set", (void*)action  , sizeof(action)  },
        {"cpuid"     , ArgTypesString, NULL      , cpuid          , sizeof(cpuid)   },
        {"devtype"   , ArgTypesString, NULL      , devtype        , sizeof(devtype) },
        {"End"       , ArgTypesEnd   , NULL      , NULL           , 0               },
    };

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action, strlen("list"))) {
        strncpy(cpuid, get_cpuid(), MAX_CPUID_LEN);
        snprintf(devtype, sizeof(devtype), "%s", BURN_DEV);
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    }
    return ret;
}

static int JCPCmdDownLoadCfg(char *buf, int buflen, int argc, char **argv)
{
    char action[JCP_ACTION_LEN] = {0};
    char filename[256] = {0};
    char filetotal[12] = {0};

    HelpMsgS helps[] = {
        {"?"        , "获取文件大小"          },
        {"act"      , "list:获取所有参数"     },
        {"filename" , "文件的绝对路径"        }, // /mnt/IPCamera/20241221/S-000000-0898.mp4
        {"filetotal", "文件的大小，单位为字节"},
        {"End"      , "downloadcfg -?获取帮助"},
    };

    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK  , NULL  , NULL         , 0                },
        {"act"      , ARG_TYPE_ACT  , "list", (void*)action, sizeof(action)   },
        {"filename" , ArgTypesString, NULL  , filename     , sizeof(filename) },
        {"filetotal", ArgTypesString, NULL  , filetotal    , sizeof(filetotal)},
        {"End"      , ArgTypesEnd   , NULL  , NULL         , 0                },
    };

    JCP_ARG_PARSER(NULL, 0);
    SET_PARAM_RULE_CHECK(opts);

    if (!strcasecmp("list", action)) {
        if (!is_okey(filename)) {
            ERR("filename %s is not exist, return FAILURE\n", filename);
            return FAILURE;
        }

        struct stat st;
        bzero(&st, sizeof(st));
        if (FAILURE == stat(filename, &st)) {
            ERR("stat filename %s failed, return FAILURE\n", filename);
            return FAILURE;
        }

        snprintf(buf, buflen-1, "filetotal=%lld", st.st_size);
    }

    return SUCCESS;
}

static int JCPCmdDevBatch(char *buf, int buflen, int argc, char **argv)
{
    return jcpcmd_devbatch(buf, buflen, argc, argv);
}

int JCPCmdAppveCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    Appvecfg inner = {0};
    Appvecfg outer = {0};

    HelpMsgS helps[] = {
        {"?"       , "APP视频参数设置"                        },
        {"act"     , "list:获取所有参数 set:设置参数"             },
        {"w_min"   , "流畅"                               },
        {"w_mid"   , "高清"                               },
        {"w_max"   , "超清"                               },
        {"imaxqp_0", "主码流imaxqp"                        },
        {"iminqp_0", "主码流iminqp"                        },
        {"imaxqp_1", "子码流imaxqp"                        },
        {"iminqp_1", "子码流iminqp"                        },
        {"appxvsz" , "app 定制截图分辨率"                      },
        {"webxvsz" , "网页定制分辨率"                          },
        {"End"     , "appvecfg -?获取帮助"                  },
    };

    ArgOptS opts[] = {
        {"?"       , ARG_TYPE_ASK  , NULL      , NULL                          , 0                            },
        {"act"     , ARG_TYPE_ACT  , "list|set", action                        , sizeof(action)               },
        {"w_min"   , ArgTypesString, NULL      , (void *)outer.appve[0].appves , sizeof(outer.appve[0].appves)},
        {"w_mid"   , ArgTypesString, NULL      , (void *)outer.appve[1].appves , sizeof(outer.appve[1].appves)},
        {"w_max"   , ArgTypesString, NULL      , (void *)outer.appve[2].appves , sizeof(outer.appve[2].appves)},
        {"imaxqp_0", ArgTypesInt   , "20~51"   , (void *)&outer.imaxqp[0]      , sizeof(int)                  },
        {"iminqp_0", ArgTypesInt   , "10~40"   , (void *)&outer.iminqp[0]      , sizeof(int)                  },
        {"imaxqp_1", ArgTypesInt   , "20~51"   , (void *)&outer.imaxqp[1]      , sizeof(int)                  },
        {"iminqp_1", ArgTypesInt   , "10~40"   , (void *)&outer.iminqp[1]      , sizeof(int)                  },
        {"appxvsz" , ARG_TYPE_LISTI, "0~16"    , (void *)&outer.appxvsz        , sizeof(int)                  },
        {"webxvsz" , ARG_TYPE_LISTI, "0~16"    , (void *)&outer.webxvsz        , sizeof(int)                  },
        {"End"     , ArgTypesEnd   , NULL      , NULL                          , 0                            },
    };

    ret = get_config(handleAppveCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);
    memcpy(&outer, &inner, sizeof(Appvecfg));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(Appvecfg));
        ret = set_config(handleAppveCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

int JCPCmdAlarmInfoCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    AlarmInfocfg inner = {0};
    AlarmInfocfg outer = {0};

    HelpMsgS helps[] = {
        {"?"       , "报警间隔参数设置"              },
        {"act"     , "list:获取所有参数 set:设置参数"},
        {"interval", "报警间隔 1min到3600s(1小时)"   },
        {"End"     , "alarminfoCfg -?获取帮助"       },
    };

    ArgOptS opts[] = {
        {"?"       , ARG_TYPE_ASK, NULL      , NULL                  , 0                     },
        {"act"     , ARG_TYPE_ACT, "list|set", action                , sizeof(action)        },
        {"interval", ArgTypesInt , "60~3600" , (void*)&outer.interval, sizeof(outer.interval)},
        {"End"     , ArgTypesEnd , NULL      , NULL                  , 0                     },
    };

    ret = get_config(handleAlarmInfoCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(AlarmInfocfg));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action, strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", action, strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(AlarmInfocfg));
        ret = set_config(handleAlarmInfoCfg, outer);
    }

    return ret;
}

int JCPCmdConvergenceCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};

    sFacialConvergence inner = {{0,},};
    sFacialConvergence outer = {{0,},};

    HelpMsgS helps[] = {
        {"?"               , "以太网设置"                         },
        {"act"             , "list:获取所有参数 set:设置参数"},
        {"wh_iso1"         , "全彩1档iso阈值"                       },
        {"wh_iso2"         , "全彩2档iso阈值"                       },
        {"wh_iso3"         , "全彩3档iso阈值"                       },
        {"ir_iso1"         , "红外1档iso阈值"                       },
        {"ir_iso2"         , "红外2档iso阈值"                       },
        {"ir_iso3"         , "红外3档iso阈值"                       },
        {"wh_area1"        , "全彩1档人形面积阈值"                      },
        {"wh_area2"        , "全彩2档人形面积阈值"                      },
        {"wh_area3"        , "全彩3档人形面积阈值"                      },
        {"ir_area1"        , "红外1档人形面积阈值"                      },
        {"ir_area2"        , "红外2档人形面积阈值"                      },
        {"ir_area3"        , "红外3档人形面积阈值"                      },
        {"wh_compensation1", "全彩1档收光强度"                        },
        {"wh_compensation2", "全彩2档收光强度"                        },
        {"wh_compensation3", "全彩3档收光强度"                        },
        {"ir_compensation1", "红外1档收光强度"                        },
        {"ir_compensation2", "红外2档收光强度"                        },
        {"ir_compensation3", "红外3档收光强度"                        },
        {"End"             , "convergencecfg -? 获取帮助"          },
    };

    ArgOptS opts[] =
    {
        {"?"               , ARG_TYPE_ASK , NULL          , NULL                   , 0                              },
        {"act"             , ARG_TYPE_ACT , "list|set"    , (void*)action          , sizeof(action)                 },
        {"wh_iso1"         , ArgTypesInt  , "0~10000"     , &outer.wh.iso1         , sizeof(outer.wh.iso1)          },
        {"wh_iso2"         , ArgTypesInt  , "0~10000"     , &outer.wh.iso2         , sizeof(outer.wh.iso2)          },
        {"wh_iso3"         , ArgTypesInt  , "0~10000"     , &outer.wh.iso3         , sizeof(outer.wh.iso3)          },
        {"ir_iso1"         , ArgTypesInt  , "0~10000"     , &outer.ir.iso1         , sizeof(outer.ir.iso1)          },
        {"ir_iso2"         , ArgTypesInt  , "0~10000"     , &outer.ir.iso2         , sizeof(outer.ir.iso2)          },
        {"ir_iso3"         , ArgTypesInt  , "0~10000"     , &outer.ir.iso3         , sizeof(outer.ir.iso3)          },
        {"wh_area1"        , ArgTypesFloat, "0~1000000.00", &outer.wh.area1        , sizeof(outer.wh.area1)         },
        {"wh_area2"        , ArgTypesFloat, "0~1000000.00", &outer.wh.area2        , sizeof(outer.wh.area2)         },
        {"wh_area3"        , ArgTypesFloat, "0~1000000.00", &outer.wh.area3        , sizeof(outer.wh.area3)         },
        {"ir_area1"        , ArgTypesFloat, "0~1000000.00", &outer.ir.area1        , sizeof(outer.ir.area1)         },
        {"ir_area2"        , ArgTypesFloat, "0~1000000.00", &outer.ir.area2        , sizeof(outer.ir.area2)         },
        {"ir_area3"        , ArgTypesFloat, "0~1000000.00", &outer.ir.area3        , sizeof(outer.ir.area3)         },
        {"wh_compensation1", ArgTypesInt  , "0~10000"     , &outer.wh.compensation1, sizeof(outer.wh.compensation1) },
        {"wh_compensation2", ArgTypesInt  , "0~10000"     , &outer.wh.compensation2, sizeof(outer.wh.compensation2) },
        {"wh_compensation3", ArgTypesInt  , "0~10000"     , &outer.wh.compensation3, sizeof(outer.wh.compensation3) },
        {"ir_compensation1", ArgTypesInt  , "0~10000"     , &outer.ir.compensation1, sizeof(outer.ir.compensation1) },
        {"ir_compensation2", ArgTypesInt  , "0~10000"     , &outer.ir.compensation2, sizeof(outer.ir.compensation2) },
        {"ir_compensation3", ArgTypesInt  , "0~10000"     , &outer.ir.compensation3, sizeof(outer.ir.compensation3) },
        {"End"             , ArgTypesEnd  , NULL          , NULL                   , 0                              },
    };

    ret = get_config(handleConvergenceCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(sFacialConvergence));
    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        JCP_RETURN_SUCC_IF_MEM_EQ(&outer, &inner, sizeof(sFacialConvergence));
        ret = set_config(handleConvergenceCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }

    return SUCCESS;
}

/*
 *此JCP专门用于一键呼叫功能中，APP接听和挂断时通知设备端，方便设备端响应
*/
int JCPCmdVideoCallCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    int status = 0;
    sVideoCallCfg inner = {0};
    sVideoCallCfg outer = {0};

    HelpMsgS helps[] = {
        {"?"            , "一键呼叫状态"                          },
        {"act"          , "list:获取所有参数 set:设置参数"            },
        {"status"       , "0:挂断，1：接听"                       },
        {"show"         , "APP 界面是否显示一键呼叫开关"                },
        {"enable"       , "一键呼叫功能开关，0-关闭 1-开启"             },
        {"modelId"      , "voip model_id "               },
        {"appId"        , "0-未开通，1-已开通"                  },
        {"openId"       , "voip openID"                  },
        {"openStatus"   , "1 开通状态 0-未开通，1-已开通"   },
        {"End"          , "videocall -?获取帮助"            },
    };

    ArgOptS opts[] =
    {
        {"?"         , ARG_TYPE_ASK     , NULL      , NULL              , 0               },
        {"act"       , ARG_TYPE_ACT     , "list|set", (void*)action     , sizeof(action)  },
        {"status"    , ArgTypesInt      , "0|1"     , &status           , sizeof(int)     },
        {"show"      , ArgTypesInt      , "0|1"     , &outer.show       , sizeof(outer.show)  },
        {"enable"    , ArgTypesInt      , "0|1"     , &outer.enable     , sizeof(outer.enable)},
        {"modelId"   , ArgTypesString   , NULL      , &outer.modelId    , sizeof(outer.modelId)  },
        {"appId"     , ArgTypesString   , NULL      , &outer.appId      , sizeof(outer.appId)},
        {"openId"    , ArgTypesString   , NULL      , &outer.openId     , sizeof(outer.openId)},
        {"openStatus", ArgTypesInt      , "0|1"     , &outer.openStatus , sizeof(outer.openStatus)},
        {"End"       , ArgTypesEnd      , NULL      , NULL              , 0               },
    };

    ret = conf_get_videocall_cfg(&inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(sVideoCallCfg));

    JCP_ARG_PARSER(NULL, 0);

    if(!strncasecmp("list", action,strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if(!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        call_evt_enqueue((eCallEvt)status);
        ret = conf_set_videocall_cfg(outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return ret;
}

int JCPCmdAiVqeV2Cfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    sAiVqeV2Cfg outer = {0};
    sAiVqeV2Cfg inner = {0};
    sVqeV2PnrCfg *p_pnr = &outer.pnr_cfg;
    sVqeV2NrCfg  *p_nr  = &outer.nr_cfg;
    sVqeV2AgcCfg *p_agc = &outer.agc_cfg;
    sVqeV2FmpCfg *p_fmp = &outer.fmp_cfg;
    sVqeV2AecCfg *p_aec = &outer.aec_cfg;
    sVqeV2WnrCfg *p_wnr = &outer.wnr_cfg;
    sVqeV2HsCfg  *p_hs  = &outer.hs_cfg;

    HelpMsgS helps[] = {
        {"?"                           , "海思音频输入 VQE V2 参数"                  },
        {"act"                         , "list: 获取 cfg 所有参数，set: 设置参数，args 列出所有参数"},

        {"open_mask"                   , "VQE V2 开启的总功能"},

        {"pnr_enable"                  , "是否开启二次降噪功能，0，否，1 是。"},
        {"pnr_usr_mode"                , "是否采用用户模式：0：自动模式，1：用户模式，默认为0关闭。"},
        {"pnr_min_gain_limit"          , "所允许的最大降噪力度，取值范围：[0, 32767]，默认值为5827。值越小，噪声抑制能力越强。"},
        {"pnr_snr_prior_limit"         , "先验信噪比最小值，取值范围：[0, 32767]，默认值为1036。值越小，噪声抑制能力越强。"},
        {"pnr_ht_threshold"            , "语音判定阈值，取值范围：[0, 80]，默认值为10。值越小，越容易判定为语音。"},
        {"pnr_hs_threshold"            , "谐波检测阈值，取值范围：[100, 1100]，默认值为100。值越小，越容易判定为语音。"},
        {"pnr_alpha_ph"                , "语音概率平滑系数，取值范围：[0, 100]，默认值为90。调节无效，保留参数。"},
        {"pnr_alpha_psd"               , "噪声估计平滑系数，取值范围：[0, 100]，默认值为65。调节无效，保留参数。"},
        {"pnr_prior_snr_fixed"         , "固定先验信噪比，取值范围：[1, 99]，默认值为30。值越小，越保护语音。"},
        {"pnr_cep_threshold"           , "倒谱平滑语音阈值，取值范围：[0, 100]，默认值为16。值越小，越保护语音，tcs_enable为1时调节有效。"},
        {"pnr_cep_amp"                 , "语音保护参数，取值范围：[100, 1000]，默认值为120。值越大，越保护语音，tcs_enable为1时调节有效。"},
        {"pnr_low_freq_protect"        , "低频信号保护，取值范围：[0, 1]，默认值为1。0代表关闭，1代表开启。"},
        {"pnr_speech_protect_threshold", "语音保护参数，取值范围：[0, 100]，默认值为75。值越大，越保护语音。"},
        {"pnr_hem_enable"              , "谐波增强开关，取值范围：[0, 1]，默认值为0。0代表关闭，1代表开启。"},
        {"pnr_tcs_enable"              , "倒谱平滑开关，取值范围：[0, 1]，默认值为1。0代表关闭，1代表开启。"},

        {"nr_enable"                   , "是否开启噪声抑制功能，0 否，1 是。"},
        {"nr_usr_mode"                 , "是否采用用户模式，0：自动模式，1：用户模式，默认为 0 关闭。"},
        {"nr_min_gain_limit"           , "所允许的最大降噪力度，取值范围：[1, 32767]，默认值为 1036。值越小，噪声抑制能力越强。"},
        {"nr_snr_prior_limit"          , "先验信噪比最小值，取值范围：[1, 32767]，默认值为 33。值越小，噪声抑制能力越强。"},
        {"nr_ht_threshold"             , "语音判定阈值，取值范围：[0, 1000]，默认值为 130。值越小，降噪力度越大。"},
        {"nr_hs_threshold"             , "谐波检测阈值，取值范围：[100, 1100]，默认值为 100。调节无效，保留参数。"},
        {"nr_cep_threshold"            , "倒谱平滑语音阈值，取值范围：[0, 100]，默认值10，值越小，越保护语音，但可能会泄漏底噪"
                                         "，gain_sm_mode为0时调节有效。"},
        {"nr_cep_amp"                  , "语音保护参数，取值范围：[0,1000]，默认值200，值越大越保护语音，但可能会抬升底噪"
                                         "，gain_sm_mode为0时调节有效。"},
        {"nr_prior_snr"                , "先验信噪比，取值范围：[0, 20]，默认值为10。值越大，越偏向降噪。"                       },
        {"nr_snr_smooth_factor"        , "信噪比平滑系数，取值范围：[5000, 10000]，默认值为 6666。值越大，跟踪越平稳，降噪力度越大。"},
        {"nr_speech_prob_smooth_factor", "语音概率平滑系数，取值范围：[5000, 10000]，默认值为 7900。值越大，跟踪越平稳，语音保留越多。"},
        {"nr_noise_pwr_smooth_factor"  , "噪声能量平滑系数，取值范围：[5000, 10000]，默认值为 7594。值越大，跟踪越平稳。底噪越平稳。"},
        {"nr_low_freq_suppress_enable" , "低频抑制开关，取值范围：[0, 1]，默认值为1。0代表关闭，1代表开启，在低频噪声较大时开启有助于"
                                         "语音判定，更好得保护语音。"},
        {"nr_low_freq_gain_suppress"   , "低频增益抑制模式，对0到300Hz的低频噪声进行抑制，取值范围：[0, 2]，默认值为2。0代表关闭，1代"
                                         "表输出抑制，2代表平滑抑制，为2时的抑制强度在为0和1之间，当low_freq_suppress_enable为1且开启"
                                         "FMP功能模块时调节有效。"},
        {"nr_env_mode"                 , "噪声环境模式选择，取值范围：[0, 1]，默认值为1。0代表indoor，1代表outdoor，outdoor模式下降噪"
                                         "力度更大，当low_freq_suppress_enable低频抑制开关为1，开启FMP功能模块，且打开"
                                         "low_freq_gain_suppress低频增益抑制模式时调节有效。"},
        {"nr_cep_alpha"                , "噪声平滑速率，取值范围：[0, 100]，默认值100，值越大噪声越平滑，但可能会损伤语音"
                                         "，gain_sm_mode为0时调节有效。"},
        {"nr_gain_sm_mode"             , "降噪平滑模式，取值范围：[0, 1]，默认值为1。0代表倒谱平滑模式，1代表增益平滑模式，倒谱平滑模"
                                         "式相比增益平滑模式降噪效果更好，但语音损伤更厉害。"},
        {"nr_gain_sm_alpha1"           , "语音降噪平滑系数，取值范围：[0, 100]，默认值30，越大底噪越平稳，但同时混响感可能会变重"
                                         "，gain_sm_mode为1时调节有效。"},
        {"nr_gain_sm_alpha2"           , "非语音降噪平滑系数，取值范围：[0, 100]，默认值70，越大底噪越平稳，但同时混响感可能会变重"
                                         "，gain_sm_mode为1时调节有效。"},
        {"nr_gain_sm_alpha3"           , "300Hz以下低频降噪平滑系数，取值范围：[0, 100]，默认值30，值越小，低频降噪抑制力度越大"
                                         "，gain_sm_mode为1时调节有效。"},

        {"agc_enable"                  , "是否开启增益控制，0 否，1 是。"},
        {"agc_usr_mode"                , "是否采用用户模式：0：自动模式，1：用户模式，默认为0关闭。"},
        {"agc_target_level"            , "目标电平，取值范围：[-120, 0]，默认值为-16。值越大，增益越大。"},
        {"agc_max_gain"                , "所允许的最大增益，取值范围：[-120, 240]，默认值为96。值越大，增益越大。同时必须满足"
                                         "max_gain >= min_gain。"},
        {"agc_min_gain"                , "所允许的最小增益，取值范围：[-120, 0]，默认值为-60。值越大，增益越大。"},
        {"agc_up_gradient_ratio"       , "抬升速度，取值范围：[1, 30]，默认值为9。值越大，输入信号抬升的越快。"},
        {"agc_down_gradient_ratio"     , "压制速度，取值范围：[1, 30]，调节无效，保留参数。"},
        {"agc_decay"                   , "慢包络下降速率控制，取值范围：[-650, 0]，默认值为-260。值越小，包络下降越快，音量抬升越快。"},
        {"agc_vad_threshold"           , "语音判断阈值，取值范围：[0, 1024]，默认值为100。值越大，小语音越可能判断为噪声，开启NR功能模"
                                         "块时调节有效。"},
        {"agc_vad_ctrl"                , "vad控制开关，取值范围：[0, 1]，默认值为1。0代表关闭vad检测，非人声时的噪声会随着语音增益变大"
                                         "同步变大，1代表开启vad检测，非人声时的噪声不会随着语音增益变大同步变大，开启NR功能模块时调节"
                                         "有效。"},

        {"fmp_enable"                  , "是否开启噪声补偿功能，0 否，1 是。"},
        {"fmp_usr_mode"                , "是否采用用户模式：0：自动模式，1：用户模式，默认为0关闭。"},
        {"fmp_comfort_flag"            , "开启噪声补偿标记，取值为[0,1]，默认值为1。0代表关闭，1代表开启。"},
        {"fmp_comfort_intensity"       , "噪声补偿力度因子，取值范围：[1, 10]，默认值为1，即补偿的噪声幅值放大1倍。"},

        {"wnr_enable"                  , "是否开启降风噪功能，0，否，1 是。"},
        {"wnr_usr_mode"                , "是否采用用户模式：0：自动模式，1：用户模式，默认为0关闭。"},
        {"wnr_min_gain_limit"          , "最小允许的噪声抑制，取值范围：[1, 8], 默认为 8"},

        {"aec_enable"                  , "是否开启回声消除功能，0：否，1 是。"},
        {"aec_usr_mode"                , "是否采用用户模式：0：自动模式，1：用户模式，默认为0关闭。"},
        {"aec_pure_delay"              , "参考信号和回声之间延时，取值范围为[0, 300]，默认值为0，单位：ms。"},
        {"aec_switch_nlp"              , "非线性滤波开关，取值范围为[0,1]，默认值为1。 0代表关闭，1代表开启。"},
        {"aec_band1"                   , "增益控制子带1，取值范围为[0, 6000]，默认值为100，单位：Hz。"},
        {"aec_band2"                   , "增益控制子带2，取值范围为[band1, 6000]，默认值为1500，单位: Hz。"},
        {"aec_band3"                   , "增益控制子带3，取值范围为[band2, 6000]，默认值为3000，单位: Hz。"},
        {"aec_band4"                   , "增益控制子带4，取值范围为[band3, 6000]，默认值为4500，单位: Hz。"},
        {"aec_gain_lower_limit1"       , "0-band1子带控制增益下限，越大双讲效果越好，但是回声残差也越大，取值范围为[0, 100]，默认值为0。"},
        {"aec_gain_lower_limit2"       , "band1-band2子带控制增益下限，效果同上，取值范围为[0, 100]，默认值为0。"},
        {"aec_gain_lower_limit3"       , "band2-band3子带控制增益下限，效果同上，取值范围为[0, 100]，默认值为0。"},
        {"aec_gain_lower_limit4"       , "band3-band4子带控制增益下限，效果同上，取值范围为[0, 100]，默认值为0。"},
        {"aec_gain_lower_limit5"       , "band4-8kHz 子带控制增益下限，效果同上，取值范围为[0, 100]，默认值为0。"},
        {"aec_ols_on"                  , "削波场景回声抑制开关，取值范围：[0, 1]，默认值 1。0代表关闭，1代表开启。"},
        {"aec_speaker_nl_on"           , "喇叭非线性失真回声抑制开关，取值范围：[0, 1]，默认值1。0代表关闭，1代表开启。"},
        {"aec_block_num"               , "参考信号处理时间窗口，数值越大能有效应对更大延迟的回声信号，取值范围为[3, 30]，默认值为6"},
        {"aec_echo_boost1"             , "0-band1子带非线性回声抑制系数，值越大，子带的非线性回声抑制效果越大，但可能产生一定的双讲剪切现"
                                         "象，取值范围为[1, 100]，默认值为1。"},
        {"aec_echo_boost2"             , "0-band1子带非线性回声抑制系数，值越大，子带的非线性回声抑制效果越大，但可能产生一定的双讲剪切现"
                                         "象，取值范围为[1, 100]，默认值为1。"},
        {"aec_echo_boost3"             , "band2-band3子带非线性回声抑制系数，效果同上，取值范围为[1, 100]，默认值为12。"},
        {"aec_echo_boost4"             , "band3-band4子带非线性回声抑制系数，效果同上，取值范围为[1, 100]，默认值为4。"},
        {"aec_echo_boost5"             , "band4-8kHz 子带子带非线性回声抑制系数，效果同上，取值范围为[1, 100]，默认值为1。"},

        {"hs_enable"                   , "是否开启抗啸叫功能，0：否，1 是。"},
        {"hs_usr_mode"                 , "是否采用用户模式：0：自动模式，1：用户模式，默认为0关闭。"},
        {"hs_hold_time"                , "调整抑制啸叫增益的等待时间（ms），取值范围：[0, 1000]，默认值为100。越大则调整增益之前等待的时间越久。"},
        {"hs_min_gain"                 , "啸叫抑制力度，取值范围：[0, 100]，默认值为1。值越小抑制力度越大。"},
        {"hs_threshold"                , "啸叫判定阈值，取值范围：[0, 50]，默认值为20。越大越容易判断成啸叫。"},
        {"hs_smooth_time"              , "信号能量平滑时间（ms），取值范围：[0, 1000]，默认值为200。越大越判断啸叫越鲁棒，但及时性也越差。"},
        {"hs_freq_move"                , "移频力度（Hz），取值范围：[0, 40]，默认值为5。数字越大可能会对抗啸叫越有帮助，但是对语音损伤也越大。"},

        {"End"                         , "aivqev2cfg -?获取帮助"  },
    };

    ArgOptS opts[] =
    {
        {"?"                           , ARG_TYPE_ASK, NULL           , NULL                            , 0             },
        {"act"                         , ARG_TYPE_ACT, "list|set|args", (void*)action                   , sizeof(action)},

        {"open_mask"                   , ArgTypesInt , NULL           , &outer.open_mask                , sizeof(int)   },

        {"pnr_enable"                  , ArgTypesInt , "0|1"          , &outer.pnr_cfg.enable           , sizeof(int)   },
        {"pnr_usr_mode"                , ArgTypesInt , "0|1"          , &p_pnr->usr_mode                , sizeof(int)   },
        {"pnr_min_gain_limit"          , ArgTypesInt , "0~32767"      , &p_pnr->min_gain_limit          , sizeof(int)   },
        {"pnr_snr_prior_limit"         , ArgTypesInt , "0~32767"      , &p_pnr->snr_prior_limit         , sizeof(int)   },
        {"pnr_ht_threshold"            , ArgTypesInt , "0~80"         , &p_pnr->ht_threshold            , sizeof(int)   },
        {"pnr_hs_threshold"            , ArgTypesInt , "100~1100"     , &p_pnr->hs_threshold            , sizeof(int)   },
        {"pnr_alpha_ph"                , ArgTypesInt , "0~100"        , &p_pnr->alpha_ph                , sizeof(int)   },
        {"pnr_alpha_psd"               , ArgTypesInt , "0~100"        , &p_pnr->alpha_psd               , sizeof(int)   },
        {"pnr_prior_snr_fixed"         , ArgTypesInt , "1~99"         , &p_pnr->prior_snr_fixed         , sizeof(int)   },
        {"pnr_cep_threshold"           , ArgTypesInt , "0~100"        , &p_pnr->cep_threshold           , sizeof(int)   },
        {"pnr_cep_amp"                 , ArgTypesInt , "100~1000"     , &p_pnr->cep_amp                 , sizeof(int)   },
        {"pnr_low_freq_protect"        , ArgTypesInt , "0|1"          , &p_pnr->low_freq_protect        , sizeof(int)   },
        {"pnr_speech_protect_threshold", ArgTypesInt , "0~100"        , &p_pnr->speech_protect_threshold, sizeof(int)   },
        {"pnr_hem_enable"              , ArgTypesInt , "0|1"          , &p_pnr->hem_enable              , sizeof(int)   },
        {"pnr_tcs_enable"              , ArgTypesInt , "0|1"          , &p_pnr->tcs_enable              , sizeof(int)   },

        {"nr_enable"                   , ArgTypesInt , "0|1"          , &outer.nr_cfg.enable            , sizeof(int)   },
        {"nr_usr_mode"                 , ArgTypesInt , "0|1"          , &p_nr->usr_mode                 , sizeof(int)   },
        {"nr_min_gain_limit"           , ArgTypesInt , "1~32767"      , &p_nr->min_gain_limit           , sizeof(int)   },
        {"nr_snr_prior_limit"          , ArgTypesInt , "1~32767"      , &p_nr->snr_prior_limit          , sizeof(int)   },
        {"nr_ht_threshold"             , ArgTypesInt , "0~1000"       , &p_nr->ht_threshold             , sizeof(int)   },
        {"nr_hs_threshold"             , ArgTypesInt , "100~1100"     , &p_nr->hs_threshold             , sizeof(int)   },
        {"nr_cep_threshold"            , ArgTypesInt , "0~100"        , &p_nr->cep_threshold            , sizeof(int)   },
        {"nr_cep_amp"                  , ArgTypesInt , "0~1000"       , &p_nr->cep_amp                  , sizeof(int)   },
        {"nr_prior_snr"                , ArgTypesInt , "0~20"         , &p_nr->prior_snr                , sizeof(int)   },
        {"nr_snr_smooth_factor"        , ArgTypesInt , "5000~10000"   , &p_nr->snr_smooth_factor        , sizeof(int)   },
        {"nr_speech_prob_smooth_factor", ArgTypesInt , "5000~10000"   , &p_nr->speech_prob_smooth_factor, sizeof(int)   },
        {"nr_noise_pwr_smooth_factor"  , ArgTypesInt , "5000~10000"   , &p_nr->noise_pwr_smooth_factor  , sizeof(int)   },
        {"nr_low_freq_suppress_enable" , ArgTypesInt , "0|1"          , &p_nr->low_freq_suppress_enable , sizeof(int)   },
        {"nr_low_freq_gain_suppress"   , ArgTypesInt , "0~2"          , &p_nr->low_freq_gain_suppress   , sizeof(int)   },
        {"nr_env_mode"                 , ArgTypesInt , "0|1"          , &p_nr->env_mode                 , sizeof(int)   },
        {"nr_cep_alpha"                , ArgTypesInt , "0~100"        , &p_nr->cep_alpha                , sizeof(int)   },
        {"nr_gain_sm_mode"             , ArgTypesInt , "0|1"          , &p_nr->gain_sm_mode             , sizeof(int)   },
        {"nr_gain_sm_alpha1"           , ArgTypesInt , "0~100"        , &p_nr->gain_sm_alpha1           , sizeof(int)   },
        {"nr_gain_sm_alpha2"           , ArgTypesInt , "0~100"        , &p_nr->gain_sm_alpha2           , sizeof(int)   },
        {"nr_gain_sm_alpha3"           , ArgTypesInt , "0~100"        , &p_nr->gain_sm_alpha3           , sizeof(int)   },

        {"agc_enable"                  , ArgTypesInt , "0|1"          , &outer.agc_cfg.enable           , sizeof(int)   },
        {"agc_usr_mode"                , ArgTypesInt , "0|1"          , &p_agc->usr_mode                , sizeof(int)   },
        {"agc_target_level"            , ArgTypesInt , "0~120"        , &p_agc->target_level            , sizeof(int)   },
        {"agc_max_gain"                , ArgTypesInt , "0~360"        , &p_agc->max_gain                , sizeof(int)   },
        {"agc_min_gain"                , ArgTypesInt , "0~120"        , &p_agc->min_gain                , sizeof(int)   },
        {"agc_up_gradient_ratio"       , ArgTypesInt , "1~30"         , &p_agc->up_gradient_ratio       , sizeof(int)   },
        {"agc_down_gradient_ratio"     , ArgTypesInt , "1~30"         , &p_agc->down_gradient_ratio     , sizeof(int)   },
        {"agc_decay"                   , ArgTypesInt , "0~650"        , &p_agc->decay                   , sizeof(int)   },
        {"agc_vad_threshold"           , ArgTypesInt , "0~1024"       , &p_agc->vad_threshold           , sizeof(int)   },
        {"agc_vad_ctrl"                , ArgTypesInt , "0|1"          , &p_agc->vad_ctrl                , sizeof(int)   },

        {"fmp_enable"                  , ArgTypesInt , "0|1"          , &outer.fmp_cfg.enable           , sizeof(int)   },
        {"fmp_usr_mode"                , ArgTypesInt , "0|1"          , &p_fmp->usr_mode                , sizeof(int)   },
        {"fmp_comfort_flag"            , ArgTypesInt , "0|1"          , &p_fmp->comfort_flag            , sizeof(int)   },
        {"fmp_comfort_intensity"       , ArgTypesInt , "1~10"         , &p_fmp->comfort_intensity       , sizeof(int)   },

        {"wnr_enable"                  , ArgTypesInt , "0|1"          , &outer.wnr_cfg.enable           , sizeof(int)   },
        {"wnr_usr_mode"                , ArgTypesInt , "0|1"          , &p_wnr->usr_mode                , sizeof(int)   },
        {"wnr_min_gain_limit"          , ArgTypesInt , "1~8"          , &p_wnr->min_gain_limit          , sizeof(int)   },

        {"aec_enable"                  , ArgTypesInt , "0|1"          , &outer.aec_cfg.enable           , sizeof(int)   },
        {"aec_usr_mode"                , ArgTypesInt , "0|1"          , &p_aec->usr_mode                , sizeof(int)   },
        {"aec_pure_delay"              , ArgTypesInt , "0~300"        , &p_aec->pure_delay              , sizeof(int)   },
        {"aec_switch_nlp"              , ArgTypesInt , "0|1"          , &p_aec->switch_nlp              , sizeof(int)   },
        {"aec_band1"                   , ArgTypesInt , "0~6000"       , &p_aec->band1                   , sizeof(int)   },
        {"aec_band2"                   , ArgTypesInt , "0~6000"       , &p_aec->band2                   , sizeof(int)   },
        {"aec_band3"                   , ArgTypesInt , "0~6000"       , &p_aec->band3                   , sizeof(int)   },
        {"aec_band4"                   , ArgTypesInt , "0~6000"       , &p_aec->band4                   , sizeof(int)   },
        {"aec_gain_lower_limit1"       , ArgTypesInt , "0~100"        , &p_aec->gain_lower_limit1       , sizeof(int)   },
        {"aec_gain_lower_limit2"       , ArgTypesInt , "0~100"        , &p_aec->gain_lower_limit2       , sizeof(int)   },
        {"aec_gain_lower_limit3"       , ArgTypesInt , "0~100"        , &p_aec->gain_lower_limit3       , sizeof(int)   },
        {"aec_gain_lower_limit4"       , ArgTypesInt , "0~100"        , &p_aec->gain_lower_limit4       , sizeof(int)   },
        {"aec_gain_lower_limit5"       , ArgTypesInt , "0~100"        , &p_aec->gain_lower_limit5       , sizeof(int)   },
        {"aec_ols_on"                  , ArgTypesInt , "0|1"          , &p_aec->ols_on                  , sizeof(int)   },
        {"aec_speaker_nl_on"           , ArgTypesInt , "0|1"          , &p_aec->speaker_nl_on           , sizeof(int)   },
        {"aec_block_num"               , ArgTypesInt , "3~30"         , &p_aec->block_num               , sizeof(int)   },
        {"aec_echo_boost1"             , ArgTypesInt , "1~100"        , &p_aec->echo_boost1             , sizeof(int)   },
        {"aec_echo_boost2"             , ArgTypesInt , "1~100"        , &p_aec->echo_boost2             , sizeof(int)   },
        {"aec_echo_boost3"             , ArgTypesInt , "1~100"        , &p_aec->echo_boost3             , sizeof(int)   },
        {"aec_echo_boost4"             , ArgTypesInt , "1~100"        , &p_aec->echo_boost4             , sizeof(int)   },
        {"aec_echo_boost5"             , ArgTypesInt , "1~100"        , &p_aec->echo_boost5             , sizeof(int)   },

        {"hs_enable"                   , ArgTypesInt , "0|1"          , &outer.hs_cfg.enable            , sizeof(int)   },
        {"hs_usr_mode"                 , ArgTypesInt , "0|1"          , &p_hs->usr_mode                 , sizeof(int)   },
        {"hs_hold_time"                , ArgTypesInt , "0~1000"       , &p_hs->hold_time                , sizeof(int)   },
        {"hs_min_gain"                 , ArgTypesInt , "0~100"        , &p_hs->min_gain                 , sizeof(int)   },
        {"hs_threshold"                , ArgTypesInt , "0~50"         , &p_hs->threshold                , sizeof(int)   },
        {"hs_smooth_time"              , ArgTypesInt , "0~1000"       , &p_hs->smooth_time              , sizeof(int)   },
        {"hs_freq_move"                , ArgTypesInt , "0~40"         , &p_hs->freq_move                , sizeof(int)   },

        {"End"                         , ArgTypesEnd , NULL           , NULL                            , 0             },
    };

    ret = get_config(handleAiVqeV2Cfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(sAiVqeV2Cfg));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action, strlen("list"))) {
        ot_ai_talk_vqe_v2_cfg vqe_v2 = {0};
        ss_mpi_ai_get_talk_vqe_v2_attr(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, &vqe_v2);
        outer.open_mask = vqe_v2.open_mask;

        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("args", action, strlen("args"))) {
        int bytes_args = 20 * 1024;
        char *buf_args = (char *)calloc(1, bytes_args);
        if (NULL != buf_args) {
            help_jcp_arg_msg(argc, argv, buf_args, bytes_args, opts, helps);
            printf("%s\n", buf_args);
            free(buf_args);
        }
    } else if (!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        ret = set_config(handleAiVqeV2Cfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return ret;
}

int JCPCmdAiSpeexCfg(char *buf, int buflen, int argc, char **argv)
{
    int ret = SUCCESS;
    char action[JCP_ACTION_LEN] = {0};
    sAiSpeexCfg outer = {0};
    sAiSpeexCfg inner = {0};

    HelpMsgS helps[] = {
        {"?"                   , "speex 音频输入预处理参数"                                 },
        {"act"                 , "list: 获取所有参数，set: 设置参数"                        },

        {"agc_enable"          , "增益使能，范围 [0, 1]，0 关，1 开。"                      },
        {"agc_level"           , "目标增益电平，范围 [8000.0, 24000.0]，默认 8000.0。"      },
        {"agc_max_gain"        , "最大增益上限，范围 [10, 40]，默认 30。"                   },
        {"agc_increment"       , "增益增长速度，范围 [1, 100]，默认 12。"                   },
        {"agc_decrement"       , "增益降低速度，范围 [-100, -1]，默认 -40。"                },

        {"nr_enable"           , "降噪使能，范围 [0, 1]，0 关，1 开。"                      },
        {"nr_decrement"        , "噪声衰减量，范围 [-90, -5]，默认 -25。"                   },

        {"aec_enable"          , "回声消除使能，范围 [0, 1]，0 关，1 开。"                  },
        {"aec_filter_len"      , "回声消除的长度，必须得是 frame_size 的整数倍，默认 1280。"},
        {"aec_suppress"        , "非活跃时残余回声抑制强度，范围 [-60, -1]，默认 -40。"     },
        {"aec_suppress_active" , "活跃时残余回声抑制强度，范围 [-60, -1]，默认 -15。"       },

        {"End"                 , "aispeexcfg -?获取帮助"  },
    };

    ArgOptS opts[] = {
        {"?"                   , ARG_TYPE_ASK , NULL            , NULL                       , 0             },
        {"act"                 , ARG_TYPE_ACT , "list|set"      , (void*)action              , sizeof(action)},

        {"agc_enable"          , ArgTypesInt  , "0|1"           , &outer.agc_enable          , sizeof(int)   },
        {"agc_level"           , ArgTypesFloat, "8000.0~24000.0", &outer.agc_level           , sizeof(float) },
        {"agc_max_gain"        , ArgTypesInt  , "10~40"         , &outer.agc_max_gain        , sizeof(int)   },
        {"agc_increment"       , ArgTypesInt  , "1~100"         , &outer.agc_increment       , sizeof(int)   },
        {"agc_decrement"       , ArgTypesInt  , "1~100"         , &outer.agc_decrement       , sizeof(int)   },

        {"nr_enable"           , ArgTypesInt  , "0|1"           , &outer.nr_enable           , sizeof(int)   },
        {"nr_decrement"        , ArgTypesInt  , "5~90"          , &outer.nr_decrement        , sizeof(int)   },

        {"aec_enable"          , ArgTypesInt  , "0|1"           , &outer.aec_enable          , sizeof(int)   },
        {"aec_filter_len"      , ArgTypesInt  , "320~2560"      , &outer.aec_filter_len      , sizeof(int)   },
        {"aec_suppress"        , ArgTypesInt  , "1~60"          , &outer.aec_suppress        , sizeof(int)   },
        {"aec_suppress_active" , ArgTypesInt  , "1~60"          , &outer.aec_suppress_active , sizeof(int)   },

        {"End"                 , ArgTypesEnd  , NULL            , NULL                       , 0             },
    };

    ret = get_config(handleAiSpeexCfg, inner);
    RETURN_FAIL_IF_API_ERR(ret);

    memcpy(&outer, &inner, sizeof(sAiSpeexCfg));

    JCP_ARG_PARSER(NULL, 0);

    if (!strncasecmp("list", action, strlen("list"))) {
        LIST_PARAM_RULE_CHECK(opts);
        ASMJCP_LIST_STRING(buf, buflen, opts);
    } else if (!strncasecmp("set", action,strlen("set"))) {
        SET_PARAM_RULE_CHECK(opts);
        ret = set_config(handleAiSpeexCfg, outer);
        RETURN_FAIL_IF_API_ERR(ret);
    }
    return ret;
}

struct JcpCmdMap JcpCmdAll[] =
{
    {"list"             , JCPCmdlist            },
    {"aeawbblccfg"      , JCPCmdAeAwbBlcCfg     },
    {"alarmtest"        , JCPCmdTestAlarm       },
    {"audiocfg"         , JCPCmdAudioCfg        },
    {"audiotestcfg"     , JCPCmdAudioTestCfg    },
    {"ca2mdcfg"         , JCPCmdCA2MDCfg        },
    {"ca2cardetectcfg"  , JCPCmdCA2CarCfg       },
    {"ca2petdetectcfg"  , JCPCmdCA2PetCfg       },
	{"ca2crydetectcfg"  , JCPCmdCA2CryCfg       },
    {"ca2vglinecfg"     , JCPCmdCA2VglineCfg    },
    {"ca2vgrectcfg"     , JCPCmdCA2VgrectCfg    },
    {"ca2humandetectcfg", JCPCmdCA2HDCfg        },
    {"ca2vmcfg"         , JCPCmdCA2VMaskAlarmCfg},

    {"capturecfg"    , JCPCmdCA2PtureCfg    },
    {"ca2ipconflict" , JCPCmdCA2Ipconflict  },
    {"ca2linkbroken" , JCPCmdCA2Linkbroken  },
    {"devrecordcfg"  , JCPCmdDevRcrdCfg     },
    {"devvecfg"      , JCPCmdDevveCfg       },
    {"devaudioopt"   , JCPCmdDevaudioOpt    },
    {"devveopt"      , JCPCmdDevveOpt       },
    {"devappopt"     , JCPCmdAppOpt         },
    {"recplan"       , JCPCmdRecplan        },
    {"gpio"          , JCPCmdGpio           },
    {"recordstop"    , JCPCmdRecordAlarmStop},
    {"presetcfg"     , JCPCmdPreSetCfg      },
    {"resocfg"       , JCPCmdResoCfg        },
    {"denoisecfg"    , JCPCmddenoiseCfg     },
    {"emailcfg"      , JCPCmdEmailCfg       },
    {"ethcfg"        , JCPCmdEthCfg         },

    {"getalarmevent" , JCPCmdGetAlarmEvent  },
    {"ircfg"         , JCPCmdIRCfg          },
    {"lightcfg"      , JCPCmdLightCfg       },
    {"mdmbcfg"       , JCPCmdMDMBCfg        },
    {"humandetectcfg", JCPCmdHDCfg          },
    {"osdcfg"        , JCPCmdOSDCfg         },
    {"osdstrcfg"     , JCPCmdOSDTxtCfg      },
    {"osdstylecfg"   , JCPCmdOsdstyleCfg    },
    {"portcfg"       , JCPCmdPortCfg        },

    {"roicfg"        , JCPCmdRoiCfg         },
    {"searchfilecfg" , JCPCmdSearchFileCfg  },
    {"getlog"        , JCPCmdGetLog         },
    {"sysctrl"       , JCPCmdSysCtrl        },
    {"timecfg"       , JCPCmdTimeCfg        },
    {"update"        , JCPCmdUpdate         },
    {"ntpcfg"        , JCPCmdNtpCfg         },
    {"upnpcfg"       , JCPCmdUPNPCfg        },
    {"sdcard"        , JCPCmdSdcard         },

    {"userpasswd"    , JCPCmdUserPasswdCfg  },
    {"veprofile"     , JCPCmdVeprofileCfg   },
    {"version"       , JCPCmdVersion        },
    {"vicfg"         , JCPCmdViCfg          },
    {"xkcd"          , JCPCmdXkcd           },
    {"gsys"          , JCPCmdGsys           },
    {"glog"          , JCPCmdGlog           },
    {"grun"          , JCPCmdGrun           },
    {"gstat"         , JCPCmdGstat          },

    {"videomaskcfg"  , JCPCmdVideoMaskCfg   },
    {"videomaskplan" , JCPCmdVideoMaskPlan  },
    {"capability"    , JCPCmdCapability     },
    {"showweb"       , JCPCmdShowWeb        },
    {"ptzcfg"        , JCPCmdPtzSerialCfg   },

    {"checkuser"     , JCPCmdCheckUser      },
    {"authmode"      , JCPCmdAuthenModeCfg  },
    {"prienv"        , JCPCmdPrienvCfg      },
    {"bootargs"      , JCPCmdBootArgs       },
    {"lampcfg"       , JCPCmdLampCfg        },
    {"motorcfg"      , JCPCmdMotor          },
    {"sim4g"         , JCPCmdSim4g          },
    {"followcfg"     , JCPCmdFollowCfg      },
//#ifdef __migrate_ball_over__
    {"pelcod20ctrl"  , JCPCmdPelcod20ctrl   },
    {"pelcod20cfg"   , JCPCmdPelcod20Cfg    },
//#endif
    {"guobiaocfg"    , JCPCmdGuobiaoCfg     },
    {"guobiaoaddr"   , JCPCmdGuobiaoAddrCfg },

    {"format"        , JCPCmdFormat         },
    {"mkdosfsprogbar", JCPCmdFormatProgressBar},
    {"lensdpc"       , JCPCmdLensDPCS       },
    {"dhcpnotify"    , JCPCmdDhcpNotify     },
    {"wificfg"       , JCPCmdWifiCfg        },
    {"wifilist"      , JCPCmdWifiList       },
    {"appvecfg"      , JCPCmdAppveCfg       },
    {"jsdbg"         , JCPCmdJsDbg          },
    {"vm"            , JCPCmdVm             },
    {"keepalive"     , JCPCmdKeepAlive      },
    {"devtest"       , JCPCmdDevTestCfg     },
    {"audioOutcfg"   , JCPCmdAudioOutCfg    },
    {"cardetectcfg"  , JCPCmdCarCfg         },
    {"petdetectcfg"  , JCPCmdPetCfg         },
    {"crydetectcfg"  , JCPCmdCryCfg         },
    {"alarmaudio"    , JCPCmdAlarmAudioTypeCfg},
    {"alarmmanage"   , JCPCmdAlarmManageCfg },
    {"sensorfps"     , JCPCmdSensorFps      },
    {"getiframe"     , JCPCmdGetIframe      },
    {"daylightscence", JCPCmdGetDayNightStatus},
    {"vgline"        , JCPCmdVgline         },
    {"vgrect"        , JCPCmdVgrect         },
    {"otainfocfg"    , JCPCmdotainfo        },//获取OTA的版本信息
    {"otaUpgradeUrl" , JCPCmdotaUpgradeUrl  },//获取OTA升级文件
    {"lightextcfg"   , JCPCmdLightExtCfg    },//新版补光设置2020.5.6
    {"timeosdcfg"    , JCPTimeOSDCfg        },
    {"whiteledcfg"   , JCPCmdWhiteLedCfg    },
    {"customcfg"     , JCPCmdFactoryCustomCfg},
    {"onvifinfocfg"  , JCPCmdOnvifInfoCfg   },
    {"audioalarmcfg" , JCPAudioAlarm        },
    {"lightalarmcfg" , JCPLightAlarm        },
    {"ioalarmcfg"    , JCPIOAlarm           },
    {"vmaskalarmcfg" , JCPCmdVMaskAlarmCfg  },
    {"driveoutcfg"   , JCPDriveOutCfg       },
    {"agtest"        , JCPCmdAgingTestCfg   },
    {"x4x8ctrl"      , JCPCmdX4x8ctrl       },
    {"privctrl"      , JCPCmdPrivCtrl       },
    {"queryrecord"   , JCPCmdQueryRecord    },
    {"appmsg"        , JCPCmdAppmsg         },
    {"aging8h"       , JCPCmdAging8h        },
    {"devcfg"        , JCPCmdDevCfg         },
    {"recplan"       , JCPCmdRecplan        },
    {"searchfilecfg" , JCPCmdSearchFileCfg  },
#if defined(PLATFORM_TENCENT)
    {"txconfcfg"     , JCPCmdTencentConfCfg },
#endif
    {"burninfo"      , JCPCmdBurninfoCfg    },
    {"downloadcfg"   , JCPCmdDownLoadCfg    },
    {"devbatch"      , JCPCmdDevBatch       },
    {"alarminfocfg"  , JCPCmdAlarmInfoCfg   },
    {"convergencecfg", JCPCmdConvergenceCfg },
    {"videocall"     , JCPCmdVideoCallCfg   },
    {"aivqev2cfg"    , JCPCmdAiVqeV2Cfg     },
    {"aispeexcfg"    , JCPCmdAiSpeexCfg     },
    {NULL            , NULL                 }
};

