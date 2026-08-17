/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : confapi.c
 * @Created Time : 2013-10-28
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
 
#include "mxml.h"
#include "debug.h"
#include "utils.h"
#include "our_md5.h"
#include "passwdtrans.h"
#include "conf_list.h"
#include "confapi.h"
#include "jcpService.h"

#define XML_HEAD    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"


typedef int (*cbFunc)(int, void **);

typedef enum
{
    ArgTypeString     = 1<<3, /*字符串*/
    ArgTypeChar       = 1<<4, /*字符型*/
    ArgTypeInt        = 1<<5, /*整型*/
    ArgTypeFloat      = 1<<6, /*浮点型*/
    ArgTypeTree       = 1<<7,
    ArgTypeEnd        = 1<<13,
}ArgTypeE;

typedef struct
{
    const char* pOpt;
    ArgTypeE    u32Type;
    void*       pArg;
    int         szMbr;
    cbFunc      pCb;
    void**      pCbArg;
}ConfApiMap;

static VencMaxParamS MaxParam[] = {
    {WIDTH_QCIF , HEIGHT_QCIF, MAXBPS_QCIF, VIDEO_MAX_FPS, VencSizeE_QCIF },
    {WIDTH_180P , HEIGHT_180P, MAXBPS_180P, VIDEO_MAX_FPS, VencSizeE_180P },
    {WIDTH_QVGA , HEIGHT_QVGA, MAXBPS_QVGA, VIDEO_MAX_FPS, VencSizeE_QVGA },
    {WIDTH_CIF  , HEIGHT_CIF , MAXBPS_CIF , VIDEO_MAX_FPS, VencSizeE_CIF  },
    {WIDTH_360P , HEIGHT_360P, MAXBPS_360P, VIDEO_MAX_FPS, VencSizeE_360P  },
    {WIDTH_VGA  , HEIGHT_VGA , MAXBPS_VGA , VIDEO_MAX_FPS, VencSizeE_VGA  },
    {WIDTH_D1   , HEIGHT_D1  , MAXBPS_D1  , VIDEO_MAX_FPS, VencSizeE_D1   },
    {WIDTH_720P , HEIGHT_720P, MAXBPS_720P, VIDEO_MAX_FPS, VencSizeE_720P },
    {WIDTH_960P , HEIGHT_960P, MAXBPS_960P, VIDEO_MAX_FPS, VencSizeE_960P },
    {WIDTH_UVGA , HEIGHT_UVGA, MAXBPS_UVGA, VIDEO_MAX_FPS, VencSizeE_UVGA },
    {WIDTH_1080P, HEIGHT_1080P, MAXBPS_1080P, VIDEO_MAX_FPS, VencSizeE_1080P},
    {WIDTH_2M_3M, HEIGHT_2M_3M, MAXBPS_2M_3M, VIDEO_MAX_FPS, VencSizeE_2M_3M},
    {WIDTH_2M_3M, HEIGHT_2M_3M, MAXBPS_2M_3M, VIDEO_MAX_FPS, VencSizeE_3M},
    {WIDTH_4M   , HEIGHT_4M  , MAXBPS_4M  , VIDEO_MAX_FPS, VencSizeE_4M   },
    {WIDTH_5M   , HEIGHT_5M  , MAXBPS_5M  , VIDEO_MAX_FPS, VencSizeE_5M   },
    {WIDTH_8M   , HEIGHT_8M  , MAXBPS_8M  , VIDEO_MAX_FPS, VencSizeE_8M   },
};

static void md5Encpypt(char *user, char *passwd, char *ha);
static void cvtohex(char *Bin, char *Hex);

/* ATTENTION: 把 static放到SERVER中，如此才能在多个进程中共享。
 **/
int conf_clr_updatecfg()
{
    UpdateS data = {0,};
    return set_config(handleUpdateCfg, data);
}

int conf_get_updatecfg(UpdateS *data)
{
    return get_config(handleUpdateCfg, (*data));
}

int conf_set_updatecfg(UpdateS data)
{
    return set_config(handleUpdateCfg, data);
}

int conf_get_vglinecfg(VglineS *data)
{
    return get_config(handleVglineCfg, (*data));
}

int conf_set_vglinecfg(VglineS data)
{
    return set_config(handleVglineCfg, data);
}

int conf_get_vgrectcfg(VgrectS *data)
{
    return get_config(handleVgrectCfg, (*data));
}

int conf_set_vgrectcfg(VgrectS data)
{
    return set_config(handleVgrectCfg, data);
}

int conf_get_OnvifInfocfg(OnvifInfoCfg *data)
{
    return get_config(handleOnvifInfoCfg, (*data));
}

int conf_set_OnvifInfocfg(OnvifInfoCfg data)
{
    return set_config(handleOnvifInfoCfg, data);
}

int conf_get_update_type()
{
    UpdateS update;
    conf_get_updatecfg(&update);

    return update.type;
}

int conf_set_update_type(int type)
{
    UpdateS update;
    conf_get_updatecfg(&update);

    // update start, set update type > UPDATE_UNINIT
    // update stop,  set update type UPDATE_UNINIT
    if (update.type != UPDATE_UNINIT) {
        return FAILURE;
    }
    update.type = type;
    return conf_set_updatecfg(update);
}

int conf_get_update_progressbar()
{
    UpdateS update;
    conf_get_updatecfg(&update);

    return update.progressbar;
}

int conf_set_update_progressbar(int progressbar)
{
    UpdateS update;
    conf_get_updatecfg(&update);
    update.progressbar = progressbar;
    return conf_set_updatecfg(update);
}


int conf_get_ethcfg(NetEthS *data)
{
    return get_config(handleEthCfg, (*data));
}

int conf_set_ethcfg(NetEthS data)
{
    return set_config(handleEthCfg, data);
}

int conf_get_bootargs(BOOTARGS_CFG_S *data)
{
    return get_config(handleBootargs, (*data));
}

int conf_get_sysinfocfg(SysInfoS *data)
{
    return get_config(handleSysInfoCfg, (*data));
}

int conf_set_sysinfocfg(SysInfoS data)
{
    return set_config(handleSysInfoCfg, data);
}

int conf_get_ptzserialcfg(PtzSerialS* data)
{
    return get_config(handlePtzSerialCfg, (*data));
}

int conf_set_ptzserialcfg(PtzSerialS data)
{
    return set_config(handlePtzSerialCfg, data);
}

int conf_get_ntpcfg(SysNtpS *data)
{
    return get_config(handleNtpcfg, (*data));
}

int conf_set_ntpcfg(SysNtpS data)
{   
    return set_config(handleNtpcfg, data);
}

int conf_get_timezonecfg(TzoneS *data)
{
    return get_config(handleTimeZoneCfg, (*data));
}

int conf_set_timezonecfg(TzoneS data)
{
    return set_config(handleTimeZoneCfg, data);
}

int conf_get_videocfg(VideoEncS *data)
{
    return get_config(handleVideoCfg, (*data));
}

int conf_set_videocfg(VideoEncS data)
{
    return set_config(handleVideoCfg, data);
}
int conf_get_realvideocfg(VideoEncS *data)
{
    return get_config(handleRealVideoCfg, (*data));
}

int conf_get_osdexpandcfg(OsdExpandS *data)
{
    return get_config(handleOsdExpandCfg, (*data));
}

int conf_set_osdexpandcfg(OsdExpandS data)
{
    return set_config(handleOsdExpandCfg, data);
}

int conf_get_audiocfg(AudioCfgS* data)
{
    return get_config(handleAudioCfg, (*data));
}

int conf_set_audiocfg(AudioCfgS data)
{
    return set_config(handleAudioCfg, data);
}

int conf_get_audiotestcfg(AudioTestCfgS* data)
{
    return get_config(handleAudioTestCfg, (*data));
}

int conf_set_audiotestcfg(AudioTestCfgS data)
{
    return set_config(handleAudioTestCfg, data);
}

int conf_get_devconf_cfg(DevConfS *data)
{
    return get_config(handleDevConf, (*data));
}

int conf_set_devconf_cfg(DevConfS data)
{
    return set_config(handleDevConf, data);
}

int conf_get_appve_cfg(Appvecfg *data)
{
    return get_config(handleAppveCfg, (*data));
}

int conf_set_appve_cfg(Appvecfg data)
{
    return set_config(handleAppveCfg, data);
}

int conf_get_httpportcfg(int* port)
{
    NetPortS data = {0};
    int ret = get_config(handleNetPortCfg, (data));
    *port = data.httpport;
    return ret;
}

int conf_set_httpportcfg(int port)
{
    NetPortS data = {0};
    get_config(handleNetPortCfg, (data));
    data.httpport = port;
    return set_config(handleNetPortCfg, data);
}

int conf_get_ftpportcfg(int* port)
{   
    NetPortS data = {0};
    int ret = get_config(handleNetPortCfg, (data));
    *port = data.ftpport;
    return ret;
}

int conf_set_ftpportcfg(int port)
{
    NetPortS data = {0};
    get_config(handleNetPortCfg, (data));
    data.ftpport = port;
    return set_config(handleNetPortCfg, data);
}

int conf_get_rtspportcfg(int* port)
{
    NetPortS data = {0};
    int ret = get_config(handleNetPortCfg, (data));
    *port = data.rtspport;
    return ret;
}

int conf_set_rtspportcfg(int port)
{
    NetPortS data = {0};
    get_config(handleNetPortCfg, (data));
    data.rtspport = port;
    return set_config(handleNetPortCfg, data);
}

int conf_get_speekportcfg(int *port)
{
    NetPortS data = {0};
    int ret = get_config(handleNetPortCfg, (data));
    *port = data.audioport;
    return ret;
}

int conf_set_speekportcfg(int port)
{
    NetPortS data = {0};
    get_config(handleNetPortCfg, (data));
    data.audioport = port;
    return set_config(handleNetPortCfg, data);
}

int conf_get_updateportcfg(int* port)
{
    NetPortS data = {0};
    int ret = get_config(handleNetPortCfg, (data));
    *port = data.updateport;
    return ret;
}

int conf_set_updateportcfg(int port)
{
    NetPortS data = {0};
    get_config(handleNetPortCfg, (data));
    data.updateport = port;
    return set_config(handleNetPortCfg, data);
}

int conf_set_netportcfg(NetPortS data)
{
    return set_config(handleNetPortCfg, data);
}

int conf_get_netportcfg(NetPortS *data)
{
    return get_config(handleNetPortCfg, (*data));
}

int conf_get_emailcfg(EmailS* data)
{
    return get_config(handleEmailCfg, (*data));
}

int conf_set_emailcfg(EmailS data)
{
    return set_config(handleEmailCfg, data);
}

int conf_get_wificfg(NetWifiS* data)
{
    return get_config(handleWifiCfg, (*data));
}

int conf_set_wificfg(NetWifiS data)
{
    return set_config(handleWifiCfg, data);
}

int conf_get_motiondetectlinkcfg(MotionDetectLinkS* data)
{
    return get_config(handleMotionDetLinkCfg, (*data));
}

int conf_set_motiondetectlinkcfg(MotionDetectLinkS data)
{
    return set_config(handleMotionDetLinkCfg, data);
}

int conf_get_humandetectlinkcfg(HumanDetectLinkS* data)
{
    return get_config(handleHumanDetLinkCfg, (*data));
}

int conf_set_humandetectlinkcfg(HumanDetectLinkS data)
{
    return set_config(handleHumanDetLinkCfg, data);
}

int conf_get_cardetectlinkcfg(CarDetectLinkS* data)
{
    return get_config(handleCarDetLinkCfg, (*data));
}

int conf_set_cardetectlinkcfg(CarDetectLinkS data)
{
    return set_config(handleCarDetLinkCfg, data);
}

int conf_get_petdetectlinkcfg(PetDetectLinkS* data)
{
    return get_config(handlePetDetLinkCfg, (*data));
}

int conf_set_petdetectlinkcfg(PetDetectLinkS data)
{
    return set_config(handlePetDetLinkCfg, data);
}

int conf_get_crydetectlinkcfg(CryDetectLinkS* data)
{
    return get_config(handleCryDetLinkCfg, (*data));
}

int conf_set_crydetectlinkcfg(CryDetectLinkS data)
{
    return set_config(handleCryDetLinkCfg, data);
}

int conf_get_vglinelinkcfg(VglineLinkS* data)
{
    return get_config(handleVglineLinkCfg, (*data));
}

int conf_set_vglinelinkcfg(VglineLinkS data)
{
    return set_config(handleVglineLinkCfg, data);
}

int conf_get_vgrectlinkcfg(VgrectLinkS* data)
{
    return get_config(handleVgrectLinkCfg, (*data));
}

int conf_set_vgrectlinkcfg(VgrectLinkS data)
{
    return set_config(handleVgrectLinkCfg, data);
}

int conf_get_irctrlcfg(IrCtrlS *data)
{
    return get_config(handleIrCtrlCfg, (*data));
}

int conf_set_irctrlcfg(IrCtrlS data)
{
    return set_config(handleIrCtrlCfg, data);
}

int conf_get_lightcfg(LightCfg *data)
{
    return get_config(handleLightCfg, (*data));
}

int conf_set_lightcfg(LightCfg data)
{
    return set_config(handleLightCfg, data);
}

int conf_get_upnpcfg(NetUpnpS *data)
{
    return get_config(handleUpnpCfg, (*data));
}

int conf_set_upnpcfg(NetUpnpS data)
{
    return set_config(handleUpnpCfg, data);
}

int conf_get_osdstylecfg(OsdStyleS *data)
{
    return get_config(handleOsdStyleCfg, (*data));
}

int conf_set_osdstylecfg(OsdStyleS data)
{
    return set_config(handleOsdStyleCfg, data);
}

int conf_get_recordcfg(RecordCtrlS* data)
{
    return get_config(handleRecordCfg, (*data));
}

int conf_set_recordcfg(RecordCtrlS data)
{
    return set_config(handleRecordCfg, data);
}

int conf_get_autorebootcfg(AutoRebootS *data)
{
    return get_config(handleAutoRebootCfg, (*data));
}

int conf_set_autorebootcfg(AutoRebootS data)
{
    return set_config(handleAutoRebootCfg, data);
}

int conf_get_bootargcfg(SysBootArgS* data)
{
    return get_config(handleBootargCfg, (*data));
}

int conf_set_bootargcfg(SysBootArgS data)
{
    return set_config(handleBootargCfg, data);
}

int conf_get_equipcfg(SysCustomS *data)
{
    return get_config(handleCapability, (*data));
}

int conf_set_equipcfg(SysCustomS data)
{
    return set_config(handleCapability, data);
}


int conf_get_usercfg(SysUserS *data)
{
    return get_config(handleUserCfg, (*data));
}

int conf_set_usercfg(SysUserS data)
{
    return set_config(handleUserCfg, data);
}

int conf_add_usercfg(char *user, char *passwd, int group)
{
    if(NULL == user || NULL == passwd || 2 < group || 0 > group)
    {
        ERR("Parameter error!\n");
        return FAILURE;
    }

    int ii = 0;
    for(ii = 0; ii < strlen(passwd); ii++)
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
        if(conf_get_usercfg(&suser) < 0)
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

        md5Encpypt(user, passwd, ha1);
        cvtohex(ha1, suser.user[j].digestpasswd);       

        suser.gnum++;

        if(conf_set_usercfg(suser) < 0)
        {
            ERR("conf_set_usercfg failed!\n");
            ret = FAILURE;
            break;
        }
    }
    while(0);

    return ret;
}

int conf_set_user_passwd(char *user, char *passwd, int group)
{
    if(NULL == user || NULL == passwd || group > 2)
    {
        ERR("Paramter error!\n");
        return FAILURE;
    }

    int ii = 0;
    for(ii = 0; ii < strlen(passwd); ii++)
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

    if(conf_get_usercfg(&users) != SUCCESS)
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

    md5Encpypt(user, passwd, ha1);
    cvtohex(ha1, users.user[i].digestpasswd);
    
    passwd_trans_encode(users.user[i].onvifpasswd, passwd, strlen(passwd));

    if(conf_set_usercfg(users)< 0)
    {
        ERR("conf_set_usercfg failed!\n");
        return FAILURE;
    }

    return SUCCESS;
}

int conf_del_usercfg(char *user)
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
        if(conf_get_usercfg(&suser) < 0)
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

        if(conf_set_usercfg(suser) < 0)
        {
            ERR("conf_set_usercfg failed!\n");
            ret = FAILURE;
            break;
        }
    }
    while(0);
    
    return ret;
}

int conf_get_auth_realm(AuthRealmS *data)
{
    return get_config(handleAuthRealmCfg, (*data));
}

int conf_get_webshowcfg(ShowWebS *data)
{
    return get_config(handleWebShowCfg, (*data));
}

int conf_get_video3acfg(Video3aS *data)
{
    return get_config(handleVideo3aCfg, (*data));
}

int conf_set_video3acfg(Video3aS data)
{
    return set_config(handleVideo3aCfg, data);
}

int conf_stop_synccfg()
{
    int data;
    return set_config(handleStopSyncCfg, data);
}

int conf_restore_default()
{
    int data;
    return set_config(handleDefaultCfg, data);
}


void md5Encpypt(char *user, char *passwd, char *ha)
{
    AuthRealmS auth = {{0},};
    MD5_CTX_OUR md5CtxA1;       

    conf_get_auth_realm(&auth);

    our_MD5Init(&md5CtxA1);
    ourMD5Update(&md5CtxA1, (unsigned char *)user, strlen(user));
    ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
    ourMD5Update(&md5CtxA1, (unsigned char *)auth.realm, strlen(auth.realm));
    ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
    ourMD5Update(&md5CtxA1, (unsigned char *)passwd, strlen(passwd));
    our_MD5Final((unsigned char*)ha, &md5CtxA1);
}

void cvtohex(char *Bin, char *Hex)
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

int conf_get_ipconflictcfg(IpLinkS* data)
{
    return get_config(handleIpConflictCfg, (*data));
}

int conf_set_ipconflictcfg(IpLinkS data)
{
    return set_config(handleIpConflictCfg, data);
}

int conf_get_ipbrokencfg(IpLinkS* data)
{
    return get_config(handleIpBrokenCfg, (*data));
}

int conf_set_ipbrokencfg(IpLinkS data)
{
    return set_config(handleIpBrokenCfg, data);
}


int conf_get_osdinfocfg(OsdInfoS* data)
{
    return get_config(handleOsdinfoCfg, (*data));
}

int conf_set_osdinfocfg(OsdInfoS data)
{
    return set_config(handleOsdinfoCfg, data);
}

int conf_get_video_maxparam(VencMaxParamS *maxparam, VencSizeE size)
{
    if (maxparam == NULL)
    {
        return FAILURE;
    }

    int ret = FAILURE;
    int i = 0;
    ViInfoS data = {0};
    //DBG("number of VencMaxParamS : %d\n", sizeof(MaxParam)/sizeof(VencMaxParamS));
    for(i=0; i < sizeof(MaxParam)/sizeof(VencMaxParamS); i++)
    {
        if(MaxParam[i].size != size)
            continue;
        
        memcpy(maxparam, &MaxParam[i], sizeof(VencMaxParamS));
        ret = SUCCESS;
        break;
    }

    if(FAILURE == conf_get_viinfocfg(&data))
        return ret;

    if(data.lampfrequency == 1)
        maxparam->maxfps = 25;

    if (VencSizeE_3M == size) {
        SysCustomS custom = {0};
        conf_get_capability(&custom);
        if (290 > custom.pixels) {
            maxparam->width = 2048;
            maxparam->height = 1152;
        } else {
            maxparam->width = WIDTH_2M_3M;
            maxparam->height = HEIGHT_2M_3M;
        }
    }

    return ret;
}

int conf_get_viinfocfg(ViInfoS* data)
{
    return get_config(handleViinfoCfg, (*data));
}

int conf_set_viinfocfg(ViInfoS data)
{
    return set_config(handleViinfoCfg, data);
}

/*
    用户相关函数返回值: 
    成功 : group id/SUCCESS; -1 : FAILURE; -2 : 用户已经存在
    -3 : 用户不存在 -4 : 用户数目已上限 -5 : 密码错误
*/
int conf_user_basic_auth(char *user, char *passwd)
{
    SysUserS users = {0};
    AuthtypeE auth = NONE_AUTH;

    if(conf_get_usercfg(&users) < 0 || conf_get_authmodecfg(&auth) < 0) 
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

int conf_user_digest_auth(char *user, char *ha2Hex, char *nonce, char *nc, 
                    char *cNonce, char *qop, char *passwd)
{
    if(NULL == user || NULL == ha2Hex || NULL == nonce)
    {
        ERR("Parameter error!\n");
        return FAILURE;
    }

    int ret = SUCCESS;
    int i = 0;
    AuthtypeE auth = NONE_AUTH;
    char HaResult[20] = {0};
    char ResultHex[36] = {0};
    SysUserS suser = {0,};
    MD5_CTX_OUR md5CtxA1;
    
    if(conf_get_usercfg(&suser) < 0 || conf_get_authmodecfg(&auth) < 0)
    {
        ERR("conf_get_usercfg failed!\n");
        return FAILURE;
    }

    if(0 == auth)
    {
        return SUCCESS;
    }

    for(i = 0; i < USER_MAX_NUM; i++)
    {
        if(!strcmp(user, suser.user[i].username))
        {
            break;
        }
    }

    if(i >= USER_MAX_NUM)
    {
        ERR("%s is not exist\n", user);
        ret = -3;
        return ret;
    }

    our_MD5Init(&md5CtxA1);
    ourMD5Update(&md5CtxA1, (unsigned char *)suser.user[i].digestpasswd, 32);
    ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
    ourMD5Update(&md5CtxA1, (unsigned char *)nonce, strlen(nonce));

    do
    {
        if(NULL == qop)
        {
            break;
        }

        if(NULL == nc || NULL == cNonce || NULL == qop)
        {
            ERR("Parameter error!\n");
            ret = FAILURE;
            return ret;
        }

        if((!strncasecmp(qop, "auth",strlen("auth"))))
        {
            ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
            ourMD5Update(&md5CtxA1, (unsigned char*)nc, strlen(nc));
            ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
            ourMD5Update(&md5CtxA1, (unsigned char*)cNonce, strlen(cNonce));
            ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
            ourMD5Update(&md5CtxA1, (unsigned char*)qop, strlen(qop));
            
            DBG("nc : %s, nonce : %s, qop : %s\n", nc, cNonce, qop);
        }
        else if(!strncasecmp(qop, "auth-int",strlen("auth-int")))
        {
            //
        }
    }
    while(0);

    ourMD5Update(&md5CtxA1, (unsigned char *)":", 1);
    ourMD5Update(&md5CtxA1, (unsigned char *)ha2Hex, 32);
    our_MD5Final((unsigned char*)HaResult, &md5CtxA1);  

    cvtohex(HaResult, ResultHex);   

    if(strcmp(ResultHex, passwd))
    {
        ret = -5;
        return ret;
    }
    
    return suser.user[i].group;
}

int conf_set_roicfg(RoiAreaS data)
{
    return set_config(handleRoiCfg, data);
}

int conf_get_roicfg(RoiAreaS *data)
{
    return get_config(handleRoiCfg, (*data));
}

int conf_get_profilecfg(VeProfileS *data)
{
    return get_config(handleProfileCfg, (*data));
}

int conf_set_profilecfg(VeProfileS data)
{
    return set_config(handleProfileCfg, data);
}

int conf_get_denoisecfg(DnrCfgS *data)
{
    return get_config(handleDenoisecfg, (*data));
}

int conf_set_denoisecfg(DnrCfgS data)
{
    return set_config(handleDenoisecfg, data);
}

int conf_get_authmodecfg(AuthtypeE *data)
{
    return get_config(handleAuthModecfg, (*data));
}

int conf_set_authmodecfg(AuthtypeE data)
{
    return set_config(handleAuthModecfg, data);
}

int conf_get_motiondetectcfg(MotionDetectS* data)
{
    return get_config(handleMotionDetectCfg, (*data));
}

int conf_set_motiondetectcfg(MotionDetectS data)
{
    return set_config(handleMotionDetectCfg, data);
}

int conf_get_humandetectioncfg(HumanDetectionS* data)
{
    return get_config(handleHumanDetectCfg, (*data));
}

int conf_set_humandetectioncfg(HumanDetectionS data)
{
    return set_config(handleHumanDetectCfg, data);
}

int conf_get_cardetectioncfg(CarDetectionS* data)
{
    return get_config(handleCarDetectCfg, (*data));
}

int conf_set_cardetectioncfg(CarDetectionS data)
{
    return set_config(handleCarDetectCfg, data);
}

int conf_get_petdetectioncfg(PetDetectionS* data)
{
    return get_config(handlePetDetectCfg, (*data));
}

int conf_set_petdetectioncfg(PetDetectionS data)
{
    return set_config(handlePetDetectCfg, data);
}

int conf_get_crydetectioncfg(CryDetectionS* data)
{
    return get_config(handleCryDetectCfg, (*data));
}

int conf_set_crydetectioncfg(CryDetectionS data)
{
    return set_config(handleCryDetectCfg, data);
}

//0 reboot 重启设备，1 reset 重启服务，2 default 恢复出厂
int conf_sysctrl_dev(int cmd)
{
    return set_config(handleSysCtrlCfg, cmd);
}

int conf_get_guobiaocfg(GuoBiaoS *data)
{
    return get_config(handleGuoBiaoCfg, (*data));
}

int conf_get_guobiaoaddr(GBAddrS *data)
{
    return get_config(handleGuoBiaoAddrCfg, (*data));
}

int conf_get_gpiocfg(gpio_t *data)
{
    return get_config(handleGpioCfg, (*data));
}

int conf_set_gpiocfg(gpio_t data)
{
    return set_config(handleGpioCfg, data);
}

int conf_get_capability(SysCustomS *data)
{
    return get_config(handleCapability, (*data));
}

int conf_get_whiteledcfg(WhiteLedS *data)
{
    return get_config(handlewhiteledCfg, (*data));
}

int conf_set_whiteledcfg(WhiteLedS data)
{
    return set_config(handlewhiteledCfg, data);
}

int conf_get_alarmaudiocfg(alarm_audio_t *data)
{
    return get_config(handleAlarmAudioTypeCfg, (*data));
}

int conf_set_alarmaudiocfg(alarm_audio_t data)
{
    return set_config(handleAlarmAudioTypeCfg, data);
}

int conf_set_pelcodcfg(PelcodCfg *data)
{
    return set_config(handlePelcodCfg, (*data));
}

int conf_get_pelcodcfg(PelcodCfg *data)
{
    return get_config(handlePelcodCfg, (*data));
}

int conf_get_presetnew_cfg(presetcfg *data)
{
    return get_config(handlePreSetNewCfg, (*data));
}

int conf_set_presetnew_cfg(presetcfg data)
{
    return set_config(handlePreSetNewCfg, data);
}

int conf_get_daynightcfg(DaynightCfgS *data)
{
    return get_config(handleDaynightCfg, (*data));
}

int conf_set_daynightcfg(DaynightCfgS data)
{
    return set_config(handleDaynightCfg, data);
}

int conf_get_motorcfg(motor_t *data)
{
    return get_config(handleMotorCfg, (*data));
}

int conf_set_motorcfg(motor_t data)
{
    return set_config(handleMotorCfg, data);
}

int conf_get_sim4g_cfg(Sim4gCfgS *data)
{
    return get_config(handleSim4gCfg, (*data));
}

int conf_set_sim4g_cfg(Sim4gCfgS data)
{
    return set_config(handleSim4gCfg, data);
}

int conf_get_follow_cfg(follow_info_t *data)
{
    return get_config(handleFollowCfg, (*data));
}

int conf_set_follow_cfg(follow_info_t data)
{
    return set_config(handleFollowCfg, data);
}

int conf_get_lightext_cfg(LightExtCfg *data)
{
    return get_config(handleLightExtCfg, (*data));
}

int conf_set_lightext_cfg(LightExtCfg data)
{
    return set_config(handleLightExtCfg, data);
}

int conf_get_audioalarm_cfg(AudioAlarmS *data)
{
    return get_config(handleAudioAlarmCfg, (*data));
}

int conf_set_audioalarm_cfg(AudioAlarmS data)
{
    return set_config(handleAudioAlarmCfg, data);
}

int conf_get_lightalarm_cfg(LightAlarmS *data)
{
    return get_config(handleLightAlarmCfg, (*data));
}

int conf_set_lightalarm_cfg(LightAlarmS data)
{
    return set_config(handleLightAlarmCfg, data);
}

int conf_get_IOalarm_cfg(IOAlarmS *data)
{
    return get_config(handleIOAlarmCfg, (*data));
}

int conf_set_IOalarm_cfg(IOAlarmS data)
{
    return set_config(handleIOAlarmCfg, data);
}

int conf_get_vmaskalarmcfg(VMaskAlarmS *data)
{
    return get_config(handleVMaskAlarmCfg, (*data));
}

int conf_set_vmaskalarmcfg(VMaskAlarmS data)
{
    return set_config(handleVMaskAlarmCfg, data);
}

int conf_get_vmaskalarmlinkcfg(VMaskAlarmLinkS *data)
{
    return get_config(handleVMaskAlarmLinkCfg, (*data));
}

int conf_set_vmaskalarmlinkcfg(VMaskAlarmLinkS data)
{
    return set_config(handleVMaskAlarmLinkCfg, data);
}

int conf_get_driveout_cfg(DriveOut *data)
{
    return get_config(handleDriveOutCfg, (*data));
}

int conf_set_driveout_cfg(DriveOut data)
{
    return set_config(handleDriveOutCfg, data);
}

int conf_get_videomaskcfg(VideoMaskS *data)
{
    return get_config(handleVideoMaskCfg, (*data));
}

int conf_set_videomaskcfg(VideoMaskS data)
{
    return set_config(handleVideoMaskCfg, data);
}

int conf_get_videomaskplan_cfg(videomask_plan_t *data)
{
    return get_config(handleVideoMaskPlanCfg, (*data));
}

int conf_set_videomaskplan_cfg(videomask_plan_t data)
{
    return set_config(handleVideoMaskPlanCfg, data);
}

int conf_get_alarminfo_cfg(AlarmInfocfg *data)
{
    return get_config(handleAlarmInfoCfg, (*data));
}

int conf_set_alarminfo_cfg(AlarmInfocfg data)
{
    return set_config(handleAlarmInfoCfg, data);
}

int conf_get_videocall_cfg(sVideoCallCfg *data)
{
    return get_config(handleVideoCallCfg, (*data));
}

int conf_set_videocall_cfg(sVideoCallCfg data)
{
    return set_config(handleVideoCallCfg, data);
}

int conf_get_aivqev2_cfg(sAiVqeV2Cfg *data)
{
    return get_config(handleAiVqeV2Cfg, (*data));
}

int conf_set_aivqev2_cfg(sAiVqeV2Cfg data)
{
    return set_config(handleAiVqeV2Cfg, data);
}

int conf_get_aispeex_cfg(sAiSpeexCfg *data)
{
    return get_config(handleAiSpeexCfg, (*data));
}

int conf_set_aispeex_cfg(sAiSpeexCfg data)
{
    return set_config(handleAiSpeexCfg, data);
}
