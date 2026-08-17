#ifndef _CONF_LIST_H
#define _CONF_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"

/* macro */

#define pop_config(handle, reference)                                           \
        handle((void *)NULL, &reference, sizeof(reference), &nwrite, (void *)"get")

#define get_config(handle, reference)                                           \
        handle((void *)NULL, &reference, sizeof(reference), NULL, (void *)"get")

#define set_config(handle, reference)                                           \
    handle((void *)&reference, (void *)NULL, 0, (int *)NULL, (void *)"set")

/* typedef */

    /*
     * declaration for jcpcmd access
     */
    int handleEthCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleSysInfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleotainfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlePtzSerialCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleNtpcfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleTimeZoneCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleUpdateCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleBootargs(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVideoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleRealVideoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVideoCallCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleOsdExpandCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVideoMaskCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVideoMaskPlanCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAudioCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAudioTestCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleNetPortCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleEmailCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleWifiCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleMotionDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleHumanDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlePetDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleCryDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVglineLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVgrectLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleLightExtCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleUpnpCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleOsdStyleCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleRecordCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAutoRebootCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleBootargCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleCapability(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleUserCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAuthRealmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleWebShowCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVideo3aCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleStopSyncCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleDefaultCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleCaptureCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleIpConflictCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleIpBrokenCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleOsdinfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleTimeOSDCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleViinfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleRoiCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleProfileCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleDenoisecfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAuthModecfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleMotionDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleHumanDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlePetDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleCryDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleSysCtrlCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleTimeCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleGuoBiaoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleGuoBiaoAddrCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlesensorfps(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleIrCtrlCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleCarDetectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleCarDetLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleLightCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAudioOutCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleDhcpNotify(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleGpioCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlewhiteledCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVglineCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVgrectCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAlarmAudioTypeCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAlarmExpandCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleMotorCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlePelcodCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlePreSetNewCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleDaynightCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleSim4gCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleFollowCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleOnvifInfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAudioAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleLightAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleIOAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVMaskAlarmCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleVMaskAlarmLinkCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleDriveOutCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleStreamNotify(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handlePrivCtrlCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleDevConf(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAppveCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAlarmInfoCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleConvergenceCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAiVqeV2Cfg(void *data, void *buf, int bufSize, int *bufLen, void *action);
    int handleAiSpeexCfg(void *data, void *buf, int bufSize, int *bufLen, void *action);

    int get_capability(ShowWebS *web);

#ifdef __cplusplus
}
#endif
#endif
