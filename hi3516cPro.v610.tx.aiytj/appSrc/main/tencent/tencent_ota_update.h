#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_OTA_UPDATE_H_
#define _TENCENT_OTA_UPDATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_FIRMWARE_PATH    "/tmp/upgrade.tgz"

int tencent_ota_thread_exited(void);
int tencent_ota_init(void);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_OTA_UPDATE_H_
#endif //PLATFORM_TENCENT

