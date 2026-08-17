#ifdef PLATFORM_TENCENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

#include "debug.h"
#include "jcpService.h"
#include "utils.h"
#include "jconfig.h"
#include "jevent.h"
#include "encrypt.h"
#include "encode_audio_queue.h"
#include "delay_exec.h"
#include "confapi.h"
#include "update.h"
#include "encode_main.h"
#include "system_main.h"
#include "factory_db.h"

#include "iv_ota.h"

#include "tencent_ota_update.h"
#include "qcloud_iot_import.h"

static int ota_thread_exited = FALSE; // ota_thread是否退出

int tencent_ota_thread_exited(void)
{
    return ota_thread_exited;
}

static int tencent_start_ota_update(void)
{
    int ret = de_encrypt_file(UPDATE_TMP_FILE);
    if (ret < 0) {
        ERR("de encrypt file:%s fail\n", UPDATE_TMP_FILE);
        remove(UPDATE_TMP_FILE);
        DELAY_REBOOT_LINUX();
        return -1;
    }

    if (SUCCESS == JCOUpdateBegin()) {
        //发送升级进度
        iv_ota_update_progress(IV_OTA_PROGRESS_TYPE_WRITE_FLASH, 0);
        HAL_SleepMs(1000);
        //iv_ota_update_progress(IV_OTA_PROGRESS_TYPE_SUCCESS, 0);
        iv_ota_exit();
        JCOUpdateJoin();

        SYSLOG("OTA upgrade %s success\n", UPDATE_TMP_FILE);
        reboot(LINUX_REBOOT_CMD_RESTART);
    }

    return 0;
}

static void tencent_ota_firmware_update_cb(char *pFirmwarePath, uint32_t u32FirmwareLen)
{
    int ret = 0;
    char cmd[128] = {0};
    DBG("firmware file:%s,len:%d\n", pFirmwarePath, u32FirmwareLen);

    // /tmp/FB33.4NOZ0.LVNM.20230106.1642.bin 改名为/tmp/upguade.tgz
    snprintf(cmd, sizeof(cmd)-1, "mv %s %s", pFirmwarePath, OTA_FIRMWARE_PATH);
    UtilSystemCmd(cmd);

    ret = tencent_start_ota_update();
    if(ret == -1){
        iv_ota_update_progress(IV_OTA_PROGRESS_TYPE_FAIL, IV_OTA_FAIL_TYPE_WRITE_FLASH);
        iv_ota_exit();
        DELAY_REBOOT_LINUX();
        return;
    }

    return;
}

static int tencent_prepare_fota_update(void)
{
    /* ota 升级不播报升级语音，app 手动 ota 视为普通升级以便播报升级语音 */
    if (is_okey(F_MANUAL_OTA)) {
        encode_audio_queue_push_amr(AUDIO_UPGRADING_NO_OFF, FALSE);
        remove(F_MANUAL_OTA);
    } else {
        TouchFile(F_UPGRADE_OTA);
    }

    system_upmedia_uninit();
    send_conf_nake(JEvent_UpdateBegin);
    uninit_encode_wait();

    SYSLOG("_____tencent ota release over_____\n");

    return 0;
}

/* callback for ota preparation, it should return 0 (means prepare OK)
 * to make ota continue, otherwise ota will stop 
 */
static int tencent_ota_prepare_cb(char * firmware_name, uint32_t firmware_len)
{
    static int ota_running = FALSE;
    DBG("tencent_ota_prepare_cb\n");
    if (ota_running == FALSE) {
        ota_running = TRUE;
        secs_delay_reboot(10*60, __func__);
        tencent_prepare_fota_update();
    }

    return SUCCESS;
}

/**
 * cb 实测 30ms ~ 40ms 调用一次
 * JEvent_TencentReset 后会断点续传
 * 经与腾讯沟通, 从服务器读取数据成功后, 且更新本地文件成功后
 * 才会调用 iv_ota_download_size_cb 来通知用户
 * 如果读超时, 就 ota tray again 再次尝试 ota下载
 */
static void ota_download_size_cb(uint32_t size)
{
#if 0
    static uint32_t reset_cnt = 0;
    static uint32_t block_cnt = 0;
    static uint32_t prev_size = 0;

    if (prev_size == size) {
        block_cnt++;
    } else {
        block_cnt = 0;
    }

    prev_size = size;
    if (block_cnt >= 5 || pop_g_run(tencent, RUN_OTA_BLOCK)) {
        SYSLOG("ota_download_size is block, size: %u, reset_cnt: %u\n", size, reset_cnt);
        if (reset_cnt >= 3) {
            DELAY_REBOOT_LINUX();
        } else {
            reset_cnt++;
            send_event(JEvent_TencentReset);
        }
        block_cnt = 0;
    }
#endif
}

/**
 ota_thread_exit_cb: 表示 ota_thread 已退出

 * mqtt_online: 0/1 表示 线程退出时的mqtt状态
 */
static void ota_thread_exit_cb(iv_ota_thread_exit_status_s ota_thread_status)
{
    DBG("mqtt_online: %d\n", ota_thread_status.mqtt_online);
    ota_thread_exited = TRUE;
}

int tencent_ota_init(void)
{
    DBG("tencent_ota_init\n");
    int ret = 0;
    iv_ota_init_parm_s stUpgradeInitParm;
    memset(&stUpgradeInitParm, 0, sizeof(iv_ota_init_parm_s));
    stUpgradeInitParm.iv_ota_firmware_update_cb = tencent_ota_firmware_update_cb;
    stUpgradeInitParm.iv_ota_prepare_cb = tencent_ota_prepare_cb;
    stUpgradeInitParm.iv_ota_download_size_cb = ota_download_size_cb;
    stUpgradeInitParm.iv_ota_thread_exit_cb = ota_thread_exit_cb;
    // OTA升级包保存路径
    strncpy(stUpgradeInitParm.firmware_path, "/tmp", sizeof(stUpgradeInitParm.firmware_path));
    strncpy(stUpgradeInitParm.firmware_version, get_fw_ver(),
            sizeof(stUpgradeInitParm.firmware_version));
    ret = iv_ota_init(&stUpgradeInitParm);
    if (ret < 0) {
        ERR("iv_ad_ap_init error:%d\n", ret);
        return -1;
    }
    ota_thread_exited = FALSE;

    return 0;
}

#endif
