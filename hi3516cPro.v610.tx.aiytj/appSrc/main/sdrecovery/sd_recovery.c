/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2024-01-25
 * Version      : 1.0
 * Author       : wuhy
 * Description  :
 */
#define  _XOPEN_SOURCE 500
#include <unistd.h>
#include <sys/vfs.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"
#include "debug.h"
#include "jconfstruct.h"
#include "record_disk.h"
#include "confapi.h"
#include "delay_exec.h"
#include "system_sch.h"
#include "factory_db.h"
#include "conf_nand.h"
#include "system_ctrl.h"
#include "jconfig.h"
#include "g_sys.h"
#include "g_run.h"
#include "g_stat.h"
#include "g_log.h"
#include "sd_recovery.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define DO_CRC(x) crc = tab[(crc ^ (x)) & 255] ^ (crc >> 8)
#else
# define DO_CRC(x) crc = tab[((crc >> 24) ^ (x)) & 255] ^ (crc << 8)
#endif

#define MAGIC        0xBADBEEF0
#define ALIGNMENT_SIZE 32768

struct firmware{
    char     devid[16];    // 绑定设备
    uint32_t ask;          // uboot 询问
    uint32_t ans;          // 应用层应答
    uint32_t tik;          // 修复次数
    uint32_t crc32;        // 校验数据完整，只计算 pack
    uint32_t pack_size;    // 包数据大小
    uint32_t magic;        // 结构体校验
    char     pack[];       // 包数据
};

static JSTCHandle	   g_check_timer = NULL;

/*从 uboot 里面拷贝出来的 crc 校验表*/
const uint32_t crc_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL,
    0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
    0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L, 0x1db71064L, 0x6ab020f2L,
    0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
    0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
    0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL, 0x35b5a8faL, 0x42b2986cL,
    0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
    0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L,
    0xcfba9599L, 0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L, 0x01db7106L,
    0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
    0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL,
    0x91646c97L, 0xe6635c01L, 0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
    0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
    0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L,
    0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
    0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL,
    0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L,
    0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
    0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L, 0xe3630b12L, 0x94643b84L,
    0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
    0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
    0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L, 0xd6d6a3e8L, 0xa1d1937eL,
    0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
    0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L,
    0x316e8eefL, 0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL, 0xb2bd0b28L,
    0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
    0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL,
    0x72076785L, 0x05005713L, 0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
    0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
    0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L,
    0x616bffd3L, 0x166ccf45L, 0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
    0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL,
    0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L,
    0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
    0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

uint32_t crc32_no_comp(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    const uint32_t *tab = crc_table;
    const uint32_t *b =(const uint32_t *)buf;
    size_t rem_len;

    /* Align it */
    if (((long)b) & 3 && len) {
        uint8_t *p = (uint8_t *)b;
        do {
          DO_CRC(*p++);
        } while ((--len) && ((long)p)&3);
        b = (uint32_t *)p;
    }

    rem_len = len & 3;
    len = len >> 2;
    for (--b; len; --len) {
        /* load data 32 bits wide, xor data 32 bits wide. */
        crc ^= *++b; /* use pre increment for speed */
        DO_CRC(0);
        DO_CRC(0);
        DO_CRC(0);
        DO_CRC(0);
    }
    len = rem_len;
    /* And the last few bytes */
    if (len) {
        uint8_t *p = (uint8_t *)(b + 1) - 1;
        do {
          DO_CRC(*++p); /* use pre increment for speed */
        } while (--len);
    }

    return crc;
}

uint32_t get_crc32(uint32_t crc, const uint8_t *p, uint32_t len)
{
    return crc32_no_comp(crc ^ 0xffffffffL, p, len) ^ 0xffffffffL;
}

/**
 * 获取 MTD 设备的 CRC32 值，fd 为 0 时只计算 crc
 *
 * @param mtd_crc 存储 CRC32 值的指针
 * @param fd      需要将读取 mtd 数据写入文件的 fd
 *
 * @return 成功返回 0，失败返回负值
 */

#define MMC_BUFSZ (512 * 1024)

int get_mtd_crc(uint32_t *mtd_crc, int fd)
{
    const char *mtd_list[] = {"/dev/mtd2", "/dev/mtd3", "/dev/mtd4", "/dev/mtd5"}; // 需要保证是连续的分区
    int fd_spi = -1;
    mtd_info_t mtdInfo = {0};
    uint32_t flash_read = 0;
    int read_bytes = 0;
    int ret = 0, m_crc = 0;
    char *flash_buf = (char *)system_malloc(MMC_BUFSZ);

    if (mtd_crc == NULL)
        return -1;

    for (int i = 0; i < ARRAY_SIZE(mtd_list); i++) {
        // Open MTD device
        if (-1 == (fd_spi = open(mtd_list[i], O_RDONLY))) {
            DBG("open flash error %s\n", mtd_list[i]);
            ret = -1;
            goto __exit;
        }

        // Fill in MTD device capability structure
        if (ioctl(fd_spi, MEMGETINFO, &mtdInfo)) {
            DBG("ioctl MEMGETINFO error %s\n", mtd_list[i]);
            ret = -1;
            goto __exit;
        }

        flash_read = 0;
        read_bytes = 0;
        while(flash_read < mtdInfo.size) {
            if ((ret = Readfully(fd_spi, flash_buf, MMC_BUFSZ)) <= 0) {
                ERR("read %s fail\n", mtd_list[i]);
                ret = -1;
                goto __exit;
            }
            read_bytes = ret;
            *mtd_crc = get_crc32(*mtd_crc, (const uint8_t *)flash_buf, read_bytes);
            usleep(500*1000);
            if (1==((++m_crc)%4)) printf("%s do crc[%02d/32] @%.2lf\n", __func__, m_crc, mono_stamp());

            if (fd) {
                ret = Writefully(fd, flash_buf, read_bytes);
                if (ret != read_bytes) {
                    ERR("fail to write the file:%s\n", F_FIRMWARE_BIN);
                    ret = -1;
                    goto __exit;
                }
            }

            flash_read += read_bytes;
        }

        close(fd_spi);
        fd_spi = -1;
    }

__exit:
    if (fd_spi > 0) close(fd_spi);
    if (flash_buf) free(flash_buf);

    return ret;
}

void cb_check_firmware(void *data)
{
    int fd = -1;
    int ret = 0, i_crc = 0;
    uint32_t off_firmware = 0;
    uint32_t firmware_crc = 0x123456;
    uint32_t mtd_crc = 0x123456;
    char dev_devid[16] = {0};
    struct firmware fw = {0};
    char curDevPath[128] = {0};
    int is_presence = 0;
    char *mmc_buf = (char *)system_malloc(MMC_BUFSZ);
    double time_start = mono_stamp();

    DBG("start check firmware\n");

    if (!use_exfat() || get_g_sys(factest) || get_sdstat()) {
        DBG("condition not met, exit check_firmware\n");
        return;
    }

    ret = storage_get_mmcpath(curDevPath);
    if (ret < 0 || strncmp(curDevPath, FACTORY_DB_PATH, sizeof(FACTORY_DB_PATH)) != 0) {
        ERR("storage get mmcpath failed\n");
        return ;
    }

    is_presence = is_okey(F_FIRMWARE_BIN);

    fd = open(F_FIRMWARE_BIN, O_RDWR | O_CREAT, 0755);
    if (fd < 0) {
        ERR("fail to open the file:%s %s\n", F_FIRMWARE_BIN, strerror(errno));
        return ;
    }

    uboot_devid_get(dev_devid, sizeof(dev_devid));

    do {
        if (!is_presence) break;

        if (lseek(fd, 0, SEEK_SET) < 0) {
            ERR("fail to lseek\n");
            break;
        }

        ret = Readfully(fd, &fw, sizeof(struct firmware));
        if (ret != sizeof(struct firmware)){
            DBG("pread error:%s\n", strerror(errno));
            break;
        }

        // check magic
        if (fw.magic != MAGIC) {
            DBG("bad magic\n");
            break;
        }

        // check devid
        if (strncmp(fw.devid, dev_devid, MAX_ID_LEN) != 0) {
            DBG("devid(%s) != dev devid(%s)\n", fw.devid, dev_devid);
            break;
        }

        // 备份文件先核对再决定要不要备份，正常情况下需要写入的情况比较少
        // get firmware crc
        if (lseek(fd, sizeof(struct firmware), SEEK_SET) < 0) { // skip head
            ERR("fail to lseek\n");
            break;
        }

        int read_byte = 0;
        while (read_byte < fw.pack_size) {
            int dst_size = (fw.pack_size - read_byte > MMC_BUFSZ) ? MMC_BUFSZ : fw.pack_size - read_byte;
            ret = Readfully(fd, mmc_buf, dst_size);
            if (ret <= 0) {
                ERR("ret:%d,read error\n", ret);
                break;
            }
            firmware_crc = get_crc32(firmware_crc, (const uint8_t *)mmc_buf, ret);
            read_byte += ret;
            usleep(500*1000);
            if (1==((++i_crc)%4)) printf("%s do crc[%02d/32] @%.2lf\n", __func__, i_crc, mono_stamp());
        }

        if (fw.crc32 != firmware_crc) {
            DBG("dev(%u) != calc(%u)\n", fw.crc32, firmware_crc);
            break;
        }

        // get mtd crc
        if (get_mtd_crc(&mtd_crc, 0) < 0) {
            ERR("get mtd crc error\n");
            goto __exit;
        }

        if (firmware_crc == mtd_crc) {
            DBG("crc is equal, exit\n");
            goto __exit;
        }
        DBG("dev(%u) != mtd(%u), U may upgraded\n", firmware_crc, mtd_crc);
    } while (0);

    // backup mtd to firmware.bin
    memset(&fw, 0, sizeof(struct firmware));
    memcpy(fw.devid, dev_devid, MAX_ID_LEN);

    // write mtd to firmware
    if (lseek(fd, sizeof(struct firmware), SEEK_SET) < 0) { // skip head
        ERR("fail to lseek\n");
        goto __exit;
    }
    mtd_crc      = 0x123456;
    if (get_mtd_crc(&mtd_crc, fd) < 0) {
        ERR("write mtd to %s fail\n", F_FIRMWARE_BIN);
        goto __exit;
    }

    off_firmware = lseek(fd, 0, SEEK_CUR);

    fw.magic = MAGIC;
    fw.crc32 = mtd_crc;
    fw.pack_size = off_firmware - sizeof(struct firmware);

    lseek(fd, 0, SEEK_SET);
    ret = pwrite(fd, &fw, sizeof(struct firmware), 0);  // write head
    if (ret != sizeof(struct firmware)) {
        ERR("fail to write the file:%s\n", F_FIRMWARE_BIN);
    }

    // 计算新的大小，使其为 32K 的倍数,uboot 内如果未 32K 对齐，会导致卡死
    off_t new_size = (off_firmware + ALIGNMENT_SIZE - 1) & ~(ALIGNMENT_SIZE - 1);

    ftruncate(fd, new_size);
    fsync(fd);

__exit:
    if (fd > 0) close(fd);
    if (mmc_buf) free(mmc_buf);

    WAR("%s spend %lf secs\n", __func__, mono_stamp() - time_start);

    return;
}

void cb_firmware_answer(void *data)
{
    struct firmware fw = {0};
    int is_updating = *(int *)data;
    int ret = 0;
    int fd = -1;

    if (!system_get_security() || !use_exfat() || get_g_sys(factest) || get_sdstat()) {
        DBG("Condition not met, exit firmware_answer\n");
        return;
    }

    do {
        if (!is_okey(F_FIRMWARE_BIN)) {
            DBG("not found %s\n", F_FIRMWARE_BIN);
            ret = 0;
            break;
        }

        fd = open(F_FIRMWARE_BIN, O_RDWR);
        if (fd < 0) {
            ERR("fail to open the file:%s\n", F_FIRMWARE_BIN);
            ret = -1;
            break;
        }

        if (lseek(fd, 0, SEEK_SET) < 0) {
            ERR("fail to lseek\n");
            ret = -1;
            break;
        }

        ret = Readfully(fd, &fw, sizeof(fw));
        if (ret != sizeof(fw)) {
            ERR("fail to read the file:%s %s\n", F_FIRMWARE_BIN, strerror(errno));
            ret = -1;
            break;
        }

        if (is_updating) {
            fw.ans = fw.ask = 0;
        } else {
            fw.ans = fw.ask;
        }

        DBG("ask:%u ans:%u tik:%u\n", fw.ask, fw.ans, fw.tik);

        if (lseek(fd, 0, SEEK_SET) < 0) {
            ERR("fail to lseek\n");
            ret = -1;
            break;
        }

        ret = Writefully(fd, &fw, sizeof(fw));
        if (ret != sizeof(fw)) {
            ERR("fail to write the file:%s\n", F_FIRMWARE_BIN);
            ret = -1;
            break;
        }
        fsync(fd);
    } while(0);

    /* 165s+35s 启动时间 = 200s，避开三路拉流高峰 */
    if (!is_updating) {
        js_delete_timer_r(&g_check_timer);
        js_create_timer_r(sch_disk, 165 * 1000, 0, cb_check_firmware, NULL, &g_check_timer);
    }

    if (fd > 0) close(fd);

    return ;
}

void firmware_answer(int is_updating)
{
    js_run_function(sch_disk, cb_firmware_answer, &is_updating, 1);
    return ;
}

static void record_disk_updatebegin_handle(int id, void *p_src, int size, void *data)
{
    DBG("record disk update handle\n");
    firmware_answer(true);

    return;
}

void init_sd_recovery(JSScheduler sch)
{
    if (sch == NULL) {
        ERR("sd recovery mode init fail\n");
        return;
    }

    if (is_okey(F_SD_TEST)) {
        if (is_okey(F_FIRMWARE_BIN)) {
            remove(F_FIRMWARE_BIN);
        }
        return;
    }
    DBG("init sd recovery mode\n");

    attach_config(JEvent_UpdateBegin  , record_disk_updatebegin_handle, NULL);

    return;
}

void uninit_sd_recovery()
{
    DBG("uninit sd recovery mode\n");

    detach_config(JEvent_UpdateBegin  , record_disk_updatebegin_handle, NULL);

    js_delete_timer_r(&g_check_timer);

    js_delete_scheduler(sch_disk);
    sch_disk = NULL;
    return;
}
