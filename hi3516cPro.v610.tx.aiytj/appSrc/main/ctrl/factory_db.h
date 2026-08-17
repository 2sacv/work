#ifndef _FACTORY_DB_H
#define _FACTORY_DB_H
#ifdef __cplusplus
extern "C" {
#endif

#define FACTORY_DB_DEV         "/dev/mmcblk0p1"
#define FACTORY_DB_PATH        "/mnt"
#define FACTORY_DB_FILE        "/mnt/id_AliHotdot.db"
#define FACTORY_IP             "/mnt/ip.txt"
#define FACTORY_FFW_FILE       "/mnt/ffw.txt"
#define FACTORY_SSID_FILE      "/mnt/ssid.txt"
#define FACTORY_FACTEST        "/mnt/factest.txt"
#define FACTORY_TYPE_FILE      "/mnt/type.txt"
#define FACTORY_UPSUCC_FILE    "/mnt/upsucc_audio.txt"
#define FACTORY_SDFIRELOG      "/mnt/sd.fire.log"
#define FACTORY_SDTARLOG       "/mnt/sd.tar.log"
#define AGING_TEST_FILE        "/mnt/aging_test.list"

#define F_SD_AUTH              "/mnt/.devid"
#define F_SD_TEST              "/mnt/sd_test.txt"
#define F_AGING_TEST           "/mnt/aging_test.list"
#define F_FIRMWARE_BIN         "/mnt/firmware.bin"
#define F_TENCENT_DEBUG        "/mnt/tencent_dbg.txt"
#define F_TENCENT_VERBOSE      "/mnt/tencent_vbs.txt"
#define F_ETH_ENABLE           "/mnt/ethenable.txt"
#define F_CID_TEMP             "/tmp/cid.txt"
#define F_UPGRADE_ROLLBACK     "/tmp/rollback"
#define F_LOCAL_TYPE           "/ipc/etc/type.txt"
#define F_MANUAL_OTA           "/opt/conf/manual_ota"
#define F_UPGRADE_WIN          "/opt/conf/upgrade_win"
#define F_UPGRADE_OTA          "/opt/conf/upgrade_ota"
#define F_DBG_MEMINFO          "/opt/dbg_meminfo"

#define F_P2P_TRIPLE           "/opt/conf/tencent.conf"
#define F_OPT_CPUID            "/opt/conf/4g/cpuid"
#define F_AGING8H              "/opt/etc/aging8h"
#define F_TX_DBGLOG            "/opt/tx_dbglog"
#define F_SD_REPORT            "/opt/log/sd_report"

#define BASENAME_MMI           "/opt/conf/aliyun_mmi/"
#define F_ALIYUN_MMI_ENABLE    BASENAME_MMI"enable"
#define F_MMI_TRIPLE           BASENAME_MMI"mmi.conf"
#define F_MMI_CERT             BASENAME_MMI"cert.bin"
#define F_WS_CERT              "/ipc/etc/cert/rootr46.pem"

#define PRODUCT_KEY_LENGTH     24
#define DEVICE_NAME_LENGTH     36
#define DEVICE_SECRET_LENGTH   72
#define PRODUCT_SECRET_LENGTH  72
#define LEN_MD5                32
#define LEN_ID                 11

typedef struct record{
    char product_key[PRODUCT_KEY_LENGTH];
    char device_name[DEVICE_NAME_LENGTH];
    char device_secret[DEVICE_SECRET_LENGTH];
    char product_secret[PRODUCT_SECRET_LENGTH];
    char reserver[56];   //预留位，凑足265+4位，保持结构体长度与原先相等

    char aliid_md5sum[LEN_MD5+4];
    char devid[LEN_ID+1];
    char ethmac[18];
    char wifimac[18];
}record_st;

/* DESC  初始化文件，total置0
 * RET   0 成功
 *       1 失败
 **/
int init_sdcard();

/* DESC  退出sdcard
 * RET   0 成功
 **/

int quit_sdcard();

/* total 当前总record数目
 * used  当前已经使用数目
 **/
int stat_sdcard(int *total, int *used);

/* DESC assign db.used = index
 **/
int index_sdcard(const char *index);

/* DESC  更新total
 * RET   0 成功
 *       1 失败，最大数目条达到1000才成功，否则失败。
 **/
int pack_sdcard();

/* DESC  取一条记?
 * RET   0 成功
 *       1 失败，已经全部用完，使用stat_sdcard()进行判断。
 **/
int fetch_record(struct record *rec);

/* DESC  确定使用fentch_record()到的数据，回写wifimac
 * RET   0 成功
 *       1 失败，已经全部用完，使用stat_sdcard()进行判断。
 **/
int use_record(const char *wifimac);

/* DESC  解密数据
 * RET   0 成功
 *       1 失败，已经全部用完，使用stat_sdcard()进行判断。
 **/
int str_decrypt_tfid(char *szDecCrypt, const char *szSrc, const int nSrc, const char *szKey);
int decrypt_record(const char *key, struct record *rec, struct record *rec2);

/* DESC  获取加密key
 * RET   0 成功
 *       1 失败，没有sdcard
 **/
int get_key(char *key);

#ifdef __cplusplus
}
#endif
#endif
