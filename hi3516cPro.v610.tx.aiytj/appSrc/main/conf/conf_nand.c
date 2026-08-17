/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : conf_nand.c
 * @Created Time : 2014-07-07
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#define _XOPEN_SOURCE 500   // for pread

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include "ss_mpi_sys.h"

#include "debug.h"
#include "conf_nand.h"
#include "conf_spi.h"
#include "soft_check.h"
#include "utils.h"
#include "logapi.h"
#include "system_ctrl.h"
#include "confapi.h"

//#define ENV_SIZE    (128 * 1024 - 4)  //爱芯uboot未使用flags ，无需多减1
#define ENV_SIZE    0xc0000

typedef struct {
    unsigned int crc;
//  unsigned char   flags; //爱芯uboot未使用flags
    char data[ENV_SIZE-4];
} UBOOT_PARAM_S;

typedef struct {
    int fdNand;
    int iBlockStart;        // 此分区起始块
} NAND_MNG_S;

static NAND_MNG_S nandMng = {-1, 0};
static mtd_info_t mtdInfo = {0, };
//static NAND_MNG_S nandMng_bak = {-1, 0};
//static mtd_info_t mtdInfo_bak = {0, };

static UBOOT_PARAM_S *ubootParam = NULL;
static pthread_mutex_t mutex_nand = PTHREAD_MUTEX_INITIALIZER;


static unsigned long crc_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);


static unsigned int JCOCRC32(unsigned int crc, const char *buf, unsigned int len)
{
    crc = crc ^ 0xffffffffL;

    while (len >= 8) {
        DO8(buf);
        len -= 8;
    }

    if (len) {
        do {
            DO1(buf);
        } while (--len);
    }

    return crc ^ 0xffffffffL;
}

#if 0
static int _NandInit(NAND_MNG_S *mng, mtd_info_t *info, char *devName)
{
    // Open MTD device
    if (-1 == (mng->fdNand = open(devName, O_RDWR))) {
        DBG("open flash error %d %s\n", errno, strerror(errno));
        goto cleanup;
    }

    // Fill in MTD device capability structure
    if (ioctl(mng->fdNand, MEMGETINFO, info)) {
        DBG("ioctl MEMGETINFO error %d %s\n", errno, strerror(errno));
        goto cleanup;
    }

    // Make sure device page sizes are valid

    if (!(64 == info->oobsize && 2048 == info->writesize) &&
        !(32 == info->oobsize && 1024 == info->writesize) &&
        !(16 == info->oobsize && 512 == info->writesize) &&
        !(8 == info->oobsize && 256 == info->writesize)) {
        DBG("Unknown flash (not normal NAND)\n");
        goto cleanup;
    }

    // Print informative message
    DBG("Totol size %u, Block size %u, page size %u, OOB size %u\n",
        info->size, info->erasesize, info->writesize, info->oobsize);
    return SUCCESS;

cleanup:
    if (0 < mng->fdNand) {
        close(mng->fdNand);
        mng->fdNand = -1;
    }
    return FAILURE;
}
#endif

static int NandInit(char *devName)
{
    memset(&mtdInfo, 0, sizeof(mtdInfo));

    // Open MTD device
    if (-1 == (nandMng.fdNand = open(devName, O_RDWR))) {
        DBG("open flash error %d %s\n", errno, strerror(errno));
        goto cleanup;
    }

    // Fill in MTD device capability structure
    if (ioctl(nandMng.fdNand, MEMGETINFO, &mtdInfo)) {
        DBG("ioctl MEMGETINFO error %d %s\n", errno, strerror(errno));
        goto cleanup;
    }

    // Make sure device page sizes are valid

    if (!(64 == mtdInfo.oobsize && 2048 == mtdInfo.writesize) &&
        !(32 == mtdInfo.oobsize && 1024 == mtdInfo.writesize) &&
        !(16 == mtdInfo.oobsize && 512 == mtdInfo.writesize) &&
        !(8 == mtdInfo.oobsize && 256 == mtdInfo.writesize)) {
        DBG("Unknown flash (not normal NAND)\n");
        goto cleanup;
    }

    // Print informative message
    DBG("Totol size %u, Block size %u, page size %u, OOB size %u\n",
        mtdInfo.size, mtdInfo.erasesize, mtdInfo.writesize, mtdInfo.oobsize);
    return SUCCESS;

cleanup:
    if (0 < nandMng.fdNand) {
        close(nandMng.fdNand);
        nandMng.fdNand = -1;
    }
    return FAILURE;
}


static void NandUninit(void)
{
    if (0 < nandMng.fdNand) {
        close(nandMng.fdNand);
        nandMng.fdNand = -1;
    }
/*
    if (0 < nandMng_bak.fdNand) {
        close(nandMng_bak.fdNand);
        nandMng_bak.fdNand = -1;
    }*/
}

#if 0
static void printenv(UBOOT_PARAM_S *hEnv)
{
    int i;
    int endCnt = 0;

    for(i=0; i < 1024; i++) {
        if(endCnt >= 2)
            break;

        if(hEnv->data[i] == 0) {
            endCnt ++;
            printf("\n");
        } else {
            putchar(hEnv->data[i]);
            endCnt = 0;
        }
    }
}
#endif

static int nand_flash_erase(int fd, uint32_t offset, uint32_t bytes)
{
    int memerase (int fd,struct erase_info_user *erase) {
        return (ioctl (fd,MEMERASE,erase));
    }
    int err;
    struct erase_info_user erase;
    erase.start = offset;
    erase.length = bytes;
    err = memerase (fd,&erase);
    if (err < 0) {
        perror ("MEMERASE");
        return -1;
    }
    fprintf (stderr,"Erased %d bytes from address 0x%.8x in flash\n",bytes,offset);
    return 0;
}

static int nand_flash_read (int fd, uint32_t offset, const char *buf, size_t len)
{
    int err;
    size_t size = len;

    if(offset != lseek(fd, offset, SEEK_SET)) {
        perror ("lseek()");
        return -1;
    }

    err = read(fd, (char *)buf, size);
    if (err < 0) {
        fprintf(stderr, "%s: read, size %d,\n", __FUNCTION__,size);
        perror("read()");
        return -1;
    }

    return 0;
}

static int nand_flash_write (int fd, uint32_t offset, const char *buf, size_t len)
{
    int err;

    if (offset != lseek(fd, offset, SEEK_SET)) {
        perror("lseek()");
        return -1;
    }

    err = write(fd, buf, len);
    if (err < 0) {
        fprintf(stderr, "%s: write, size %d\n", __FUNCTION__, len);
        perror("write()");
        return -1;
    }

    return 0;
}

static char *UbootEnvMatch (char * s1, char * s2)
{
    while (*s1 == *s2++)
        if (*s1++ == '=')
            return (s2);
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return (s2);
    return (NULL);
}

static void UbootCRCUpdate(void)
{
    ubootParam->crc = JCOCRC32(0, ubootParam->data, sizeof(ubootParam->data));
//  ubootParam->flags = 1;
    DBG("CRC:0x%x\n", ubootParam->crc);
}


static void UbootParamUninit(void)
{
    if (ubootParam) {
        free(ubootParam);
        ubootParam = NULL;
    }

    NandUninit();
}


static int UbootParamInit(void)
{
    if (NULL == (ubootParam = malloc(sizeof(UBOOT_PARAM_S)))) {
        return FAILURE;
    }
    memset(ubootParam, 0, sizeof(UBOOT_PARAM_S));

    if (SUCCESS != NandInit("/dev/mtd1")) {
        ERR("init mtd1 fail\n");
        return FAILURE;
    }

    /*
    if (SUCCESS != _NandInit(&nandMng_bak, &mtdInfo_bak, "/dev/mtd2")) {
        ERR("init mtd2 fail\n");
        return FAILURE;
    }*/

    int ret = nand_flash_read(nandMng.fdNand, 0, (char *)ubootParam, sizeof(UBOOT_PARAM_S));
    return_val_if_fail(ret == SUCCESS, FAILURE);
    //DBG("uboot_devid_set_nand %u\n", ubootParam->crc);
    //printenv(ubootParam);

    return SUCCESS;
}


static char *UbootGetEnv(char *envAddr, char *name)
{
    int i, nxt;

    for (i = 0; envAddr[i] != '\0'; i = nxt + 1) {
        char *val = NULL;

        for (nxt = i; envAddr[nxt] != '\0'; ++nxt) {
            if (nxt >= (ENV_SIZE-4)){
                return NULL;
            }
        }

        if (!(val = UbootEnvMatch(name, envAddr + i))) {
            continue;
        }

        return val;
    }

    return NULL;
}


static int UbootSetEnv(char *envAddr, char *name, char *value)
{
    int len = 0;
    char *env, *nxt = NULL;
    char *oldval = NULL;

    if (!envAddr || !name) {
        return FAILURE;
    }

    if (strchr(name, '=')) {
        DBG ("## Error: illegal character '=' in variable name \"%s\"\n", name);
        return FAILURE;
    }

    // search if variable with this name already exists
    for (env = envAddr; *env; env = nxt + 1) {
        for (nxt = env; *nxt; ++nxt) {
            ;
        }
        //DBG("env = %s\n", env);
        if ((oldval = UbootEnvMatch(name, env))) {
            break;
        }
    }

    // Delete any existing definition
    if (oldval) {
        if (*++nxt == '\0') {
            if (env > envAddr) {
                env--;
            } else {
                *env = '\0';
            }
        } else {
            for (;;) {
                *env = *nxt++;
                if ((*env == '\0') && (*nxt == '\0'))
                    break;
                ++env;
            }
        }
        *++env = '\0';
    }

    // Delete only ?
    if (!value) {
        UbootCRCUpdate();
        return SUCCESS;
    }

    // Append new definition at the end
    for (env = envAddr; *env || *(env + 1); ++env) {
        ;
    }
    if (env > envAddr) {
        ++env;
    }

    // Overflow when:   * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
    len = strlen(name) + 2;
    // add '=' for first arg, ' ' for all others
    len += strlen(value) + 1;
    if (len > (envAddr + ENV_SIZE - 5 - env)) {
        DBG ("## Error: environment overflow, \"%s\" deleted, len=%d\n", name, len);
        return FAILURE;
    }

    // copy name and value
    while ((*env = *name++) != '\0') {
        env++;
    }

    *env = '=';
    while ((*++env = *value++) != '\0') {
        ;
    }
    // end is marked with double '\0'
    *++env = '\0';

    UbootCRCUpdate();

    return SUCCESS;
}


static int UbootGetAllEnv(char *envAddr, char *msgbuf)
{
    int len = 0;
    char *env, *nxt = NULL;

    if (!envAddr || !msgbuf) {
        return FAILURE;
    }

    // search if variable with this name already exists
    for (env = envAddr; *env; env = nxt + 1) {
        for (nxt = env; *nxt; ++nxt) {
            ;
        }

        //DBG("env = %s\n", env);
        len += sprintf(msgbuf+len, "%s#", env);
    }

    return SUCCESS;
}

static int UbootSetBootargs(char *name, char *value)
{
    char *pBootargs = NULL;
    char *ptr = NULL;

    if (NULL == name || NULL == value) {
        goto __exit;
    }

    // get bootargs
    if (NULL == (pBootargs = UbootGetEnv(ubootParam->data, "bootargs"))) {
        ERR("get bootargs fail\n");
        goto __exit;
    }

    char bufTmp[1024];
    memset(bufTmp, 0, sizeof(bufTmp));

    if (NULL == (ptr = strstr(pBootargs, name))) {
        // add name=value to bootargs
        sprintf(bufTmp, "%s %s=%s", pBootargs, name, value);
    } else {
        // modify name's value of bootargs
        ptr += strlen(name) + 1;
        memcpy(bufTmp, pBootargs, ptr - pBootargs);
        strcat(bufTmp, value);

        if (NULL != (ptr = strstr(ptr, " "))) {
            // 拼接后续参数
            strcat(bufTmp, ptr);
        }
    }
    DBG("UbootSetBootargs %s\n", bufTmp);
    if (SUCCESS != UbootSetEnv(ubootParam->data, "bootargs", bufTmp)) {
        ERR("set bootargs fail\n");
        goto __exit;
    }

    return SUCCESS;

__exit:
    return FAILURE;

}

static int ReadCmdLine(ArgOptS opts[])
{
    FILE* stream;
    char FileBuf[1024];
    char *p_key = NULL;
    int Length = 0;
    int i;
    char key[32] = {0};

    memset(FileBuf,0,sizeof(FileBuf));
    stream = vpopen("cat /proc/cmdline","r");
    if(!stream) {
        ERR("cat /proc/cmdline fail!\n");
        return -1;
    }
    Length = fread(FileBuf, 1,sizeof(FileBuf), stream);
    vpclose(stream);

    if (Length < 0) {
        return -1;
    }

    for (i = 2; opts[i].argType != ArgTypesEnd; i++) {
        // UbootSetEnv(ubootParam->data, (char *)opts[i].pOpt, (char *)opts[i].pSetValue);
        sprintf(key, "%s=", opts[i].pOpt);
        p_key = strstr(FileBuf, key);
        if (p_key != NULL) {
            if (opts[i].argType == ArgTypesInt) {
                sscanf(p_key+strlen(key), "%d", (int *)opts[i].pSetValue);
            } else {
                sscanf(p_key+strlen(key), "%s", (char *)opts[i].pSetValue);
            }
        } else {
            SYSLOG("can't find %s in %s\n", key, FileBuf);
            // return FAILURE;
        }
    }
    return SUCCESS;
}

static int UbootSetBootargsStruct(ArgOptS opts[])
{
    int  i;
    char temp[1024] = {0,};
    char standby[1024] = {0,};
    char full[2048] = {0,};
    char *pBootargs = NULL;
    char *left_end = NULL;
    char *ptr1 = NULL;
    char optbuf[64] = {0};
    
    pBootargs = UbootGetEnv(ubootParam->data, "bootargs");
    if (NULL == pBootargs) {
        return FAILURE;
    }
    strcpy(temp, pBootargs);
    for (i = 2; opts[i].argType != ArgTypesEnd; i++) {
        strcpy(standby, temp);
        memset(optbuf, 0, sizeof(optbuf));
        sprintf(optbuf, "%s=", opts[i].pOpt);
        left_end = strstr(standby, optbuf);

        // full = left + key=val + rights
        if (NULL == left_end) { // not found, append
            if (opts[i].argType == ArgTypesInt) {
                snprintf(full, sizeof(full)-1, "%s %s=%d", standby, opts[i].pOpt, *(int *)opts[i].pSetValue);
            } else {
                snprintf(full, sizeof(full)-1, "%s %s=%s", standby, opts[i].pOpt, (char *)opts[i].pSetValue);
            }
        } else {
            if(left_end != standby){
                *(left_end-1) = '\0';

                if (opts[i].argType == ArgTypesInt) {
                    snprintf(full, sizeof(full)-1, "%s %s=%d", standby, opts[i].pOpt, *(int *)opts[i].pSetValue);
                } else {
                    snprintf(full, sizeof(full)-1, "%s %s=%s", standby, opts[i].pOpt, (char *)opts[i].pSetValue);
                }
            } else {
                if (opts[i].argType == ArgTypesInt) {
                    snprintf(full, sizeof(full)-1, "%s=%d", opts[i].pOpt, *(int *)opts[i].pSetValue);
                } else {
                    snprintf(full, sizeof(full)-1, "%s=%s", opts[i].pOpt, (char *)opts[i].pSetValue);
                }
            }

            ptr1 = strstr(left_end, " ");
            if (ptr1 != NULL) {
                strcat(full, ptr1);
            }
        }
    }

    DBG("bufTmp = %s\n", full);
    if (SUCCESS != UbootSetEnv(ubootParam->data, "bootargs", full)) {
        return FAILURE;
    }
    return SUCCESS;
}

static int SetBootargs_nand(ArgOptS opts[])
{
    int ret = SUCCESS;

    pthread_mutex_lock(&mutex_nand);
    ret = UbootParamInit();
    goto_tag_if_fail(ret == SUCCESS, __exit);

    ret = UbootSetBootargsStruct(opts);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    if (SUCCESS != nand_flash_erase(nandMng.fdNand, 0, ENV_SIZE)) {
        LOG("%s nand_flash_erase failed\n", __FUNCTION__);
        ret = FAILURE;
    }
    
    if (SUCCESS != nand_flash_write(nandMng.fdNand, 0, (char *)ubootParam, ENV_SIZE)) {
        LOG("%s nand_flash_write failed\n", __FUNCTION__);
        ret = FAILURE;
    }

__exit:
    UbootParamUninit();
    pthread_mutex_unlock(&mutex_nand);
    return SUCCESS;
}

static int uboot_set_bootargs_param_nand(char *name, char *value)
{
    int ret = SUCCESS;

    pthread_mutex_lock(&mutex_nand);
    goto_tag_if_fail(NULL != value, __exit);

    ret = UbootParamInit();
    goto_tag_if_fail(ret == SUCCESS, __exit);
    // save ubootenv info
    ret = UbootSetEnv(ubootParam->data, name, value);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    ret = UbootSetBootargs(name, value);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    if (SUCCESS != nand_flash_erase(nandMng.fdNand, 0, ENV_SIZE)) {
        LOG("%s nand_flash_erase failed\n", __FUNCTION__);
        ret = FAILURE;
    }
    if (SUCCESS != nand_flash_write(nandMng.fdNand, 0, (char *)ubootParam, ENV_SIZE)) {
        LOG("%s nand_flash_write failed\n", __FUNCTION__);
        ret = FAILURE;
    }
    /*
    if (SUCCESS != nand_flash_erase(nandMng_bak.fdNand, 0, ENV_SIZE)) {
        LOG("%s nand_flash_erase failed\n", __FUNCTION__);
        ret = FAILURE;
    }
    if (SUCCESS != nand_flash_write(nandMng_bak.fdNand, 0, (char *)ubootParam, ENV_SIZE)) {
        LOG("%s nand_flash_write failed\n", __FUNCTION__);
        ret = FAILURE;
    }*/

__exit:
    UbootParamUninit();
    pthread_mutex_unlock(&mutex_nand);
    return ret;
}



static int config_uboot_env_nand(char* action, ArgOptS opts[], char *msgbuf)
{
    int i = 0;
    int ret = SUCCESS;
    DBG("NAND:%s\n", msgbuf);
    pthread_mutex_lock(&mutex_nand);
    if(SUCCESS != UbootParamInit()) {
        ret = FAILURE;
        goto __exit;
    }

    if (!strncasecmp("list", action,strlen("list"))) {
        if (SUCCESS != UbootGetAllEnv(ubootParam->data, msgbuf)) {
            ret = FAILURE;
            goto __exit;
        }
    } else if(!strncasecmp("set", action,strlen("set"))) {
        for (i = 2; opts[i].argType != ArgTypesEnd; i++) {
            if (SUCCESS != UbootSetEnv(ubootParam->data, (char *)opts[i].pOpt, (char *)opts[i].pSetValue)) {
                LOG("nand UbootSetEnv of %s=%s failed\n", (char *)opts[i].pOpt, (char *)opts[i].pSetValue);
                ret = FAILURE;
            }
        }

        if (SUCCESS != nand_flash_erase(nandMng.fdNand, 0, ENV_SIZE)) {
            LOG("%s nand_flash_erase failed\n", __FUNCTION__);
            ret = FAILURE;
        }
        if (SUCCESS != nand_flash_write(nandMng.fdNand, 0, (char *)ubootParam, ENV_SIZE)) {
            LOG("%s nand_flash_write failed\n", __FUNCTION__);
            ret = FAILURE;
        }
        /*
        if (SUCCESS != nand_flash_erase(nandMng_bak.fdNand, 0, ENV_SIZE)) {
            LOG("%s nand_flash_erase failed\n", __FUNCTION__);
            ret = FAILURE;
        }
        if (SUCCESS != nand_flash_write(nandMng_bak.fdNand, 0, (char *)ubootParam, ENV_SIZE)) {
            LOG("%s nand_flash_write failed\n", __FUNCTION__);
            ret = FAILURE;
        }*/
    } else if (!strncasecmp("get", action,strlen("get"))) {
        char *ptr = NULL;
        for (i = 2; opts[i].argType != ArgTypesEnd; i++) {
            if (NULL != (ptr = UbootGetEnv(ubootParam->data, (char *)opts[i].pOpt))) {
                strcpy(opts[i].pSetValue, ptr);
            }
        }
    }

__exit:
    UbootParamUninit();
    pthread_mutex_unlock(&mutex_nand);

    return ret;
}

int config_uboot_env(char* action, ArgOptS opts[], char *msgbuf)
{
    int ret = SUCCESS;
    
    if (is_spiflash_board()) {
        if (SUCCESS != config_uboot_env_spi(action, opts, msgbuf)) {
            LOG("config_uboot_env failed\n");
            ret = FAILURE;
        }
    } else {
        ret = config_uboot_env_nand(action, opts, msgbuf);
    }
    return ret;
}

int uboot_mac_set(char *szMAC)
{
    int ret = SUCCESS;
    
    if (is_spiflash_board()) {
        if (SUCCESS != uboot_set_bootargs_param_nor("ethaddr", szMAC)) {
            LOG("uboot_mac_set failed\n");
            ret = FAILURE;
        }
    } else {
        ret = uboot_set_bootargs_param_nand("ethaddr", szMAC);
    }
    return ret;
}

int uboot_devid_set(char *szDevID)
{
    int ret = SUCCESS;
    
    if (is_spiflash_board()) {
        if (SUCCESS != uboot_set_bootargs_param_nor("device_id", szDevID)) {
            LOG("uboot_devid_set failed\n");
            ret = FAILURE;
        }
    } else {
        ret = uboot_set_bootargs_param_nand("device_id", szDevID);
    }
    return ret;
}

int uboot_p2pconf_set(char *name, char *szP2pConf)
{
    int ret = SUCCESS;
    
    if (is_spiflash_board()) {
        if (SUCCESS != uboot_set_bootargs_param_nor(name,szP2pConf)) {
            LOG("uboot_p2pconf_set failed\n");
            ret = FAILURE;
        }
    } else {
        ret = uboot_set_bootargs_param_nand(name,szP2pConf);
    }
    return ret;
}


const char *uboot_devid_get(char *devid, size_t len)
{
    if (devid == NULL) {
        ERR("%s param NULL\n", __func__);
        return NULL;
    }
    ArgOptS opts[] = {
        {"?"        , ARG_TYPE_ASK  , NULL      , NULL , 0  , NULL},
        {"act"      , ARG_TYPE_ACT  , "list|set", NULL , 0  , NULL},
        {"device_id", ArgTypesString, NULL      , devid, len, NULL},
        {"End"      , ArgTypesEnd   , NULL      , NULL , 0  , NULL},
    };
    config_uboot_env("get", opts, NULL);

    if (strlen(devid) <= 0) {
        DBG("%s error\n", __func__);
        return NULL;
    }
    return devid;
}

const char *uboot_mac_get(char *mac, int len)
{
    if (mac == NULL) {
        ERR("%s param NULL\n", __func__);
        return NULL;
    }
    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK  , NULL      , NULL, 0  , NULL},
        {"act"    , ARG_TYPE_ACT  , "list|set", NULL, 0  , NULL},
        {"ethaddr", ArgTypesString, NULL      , mac , len, NULL},
        {"End"    , ArgTypesEnd   , NULL      , NULL, 0  , NULL},
    };
    config_uboot_env("get", opts, NULL);

    if (strlen(mac) <= 0) {
        DBG("%s error\n", __func__);
        return NULL;
    }
    return mac;
}

const char *uboot_devinfo_get(char *devinfo, int len)
{
    if (devinfo == NULL) {
        ERR("%s param NULL\n", __func__);
        return NULL;
    }
    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK  , NULL      , NULL   , 0  , NULL},
        {"act"    , ARG_TYPE_ACT  , "list|set", NULL   , 0  , NULL},
        {"devinfo", ArgTypesString, NULL      , devinfo, len, NULL},
        {"End"    , ArgTypesEnd   , NULL      , NULL   , 0  , NULL},
    };
    config_uboot_env("get", opts, NULL);

    if (strlen(devinfo) <= 0) {
        DBG("%s error\n", __func__);
        return NULL;
    }
    return devinfo;
}

int uboot_devinfo_set()
{
    char oldinfo[128] = {0,};
    char devinfo[128] = {0,};
    char ethaddr[32] = {0,};
    char device_id[32] = {0,};
    int ret = SUCCESS;

    ArgOptS opts[] =
    {
        {"?"        , ARG_TYPE_ASK  , NULL      , NULL          , 0                 , NULL},
        {"act"      , ARG_TYPE_ACT  , "list|set", NULL          , 0                 , NULL},
        {"ethaddr"  , ArgTypesString, NULL      , ethaddr       , sizeof(ethaddr)   , NULL},
        {"device_id", ArgTypesString, NULL      , device_id     , sizeof(device_id) , NULL},
        {"devinfo"  , ArgTypesString, NULL      , oldinfo       , sizeof(oldinfo)   , NULL},
        {"End"      , ArgTypesEnd   , NULL      , NULL          , 0                 , NULL},
    };

    config_uboot_env("get", opts, NULL);

    if (strlen(ethaddr) <= 0) {
        SYSLOG("can't find macaddr from uboot env\n");
        return FAILURE;
    }

    if (strlen(device_id) <= 0) {
        SYSLOG("can't find devid from uboot env\n");
        return FAILURE;
    }

    if (0 > get_device_info_new(get_uid(), ethaddr, device_id, devinfo, sizeof(devinfo) - 1)) {
        LOG("secure get fail %s %s %s old %s\n", get_uid(), ethaddr, device_id, oldinfo);
        SYSLOG("secure get fail %s %s %s old %s\n", get_uid(), ethaddr, device_id, oldinfo);
        return FAILURE;
    }

    LOG("secure set %s %s %s new %s\n", get_uid(), ethaddr, device_id, devinfo);
    SYSLOG("secure set %s %s %s new %s\n", get_uid(), ethaddr, device_id, devinfo);
    
    if (is_spiflash_board()) {
        if (SUCCESS != uboot_set_bootargs_param_nor("devinfo", devinfo)) {
            LOG("WARN: uboot_devinfo_set failed\n");
            ret = FAILURE;
        }
    } else {
        ret = uboot_set_bootargs_param_nand("devinfo", devinfo);
    }

    return ret;
}

int set_bootargs(ArgOptS opts[])
{
    int ret = SUCCESS;
    
    if (is_spiflash_board()) {
        if (SUCCESS != SetBootargs_spi(opts)) {
            LOG("set_bootargs failed\n");
            ret = FAILURE;
        }
    } else {
        ret = SetBootargs_nand(opts);
    }
    return ret;
}

typedef struct {
    char *sensor;
    int width;
    int height;
} sensor_width_hei_t;

int config_bootargs(const char *action, ArgOptS opts[], BOOTARGS_CFG_S *outer)
{
    int ret = 0;
    int i = 0;
    char maxheightbuf[128] = {0};
    char maxheightstr[128] = {0};
    static int has_read = FALSE;
    static BOOTARGS_CFG_S inner = { .maxheight = 1080, };

    sensor_width_hei_t sensor_width_hei[] = {
        {"GC2053"  , 2034, 1296},
        {"CAM1080P", 1920, 1080},
        {"CAM720P" , 1280, 720 },
        {"SC1245A" , 1280, 720 },
        {"SC1245"  , 1280, 720 },
        {"SC2235"  , 1920, 1080},
        {"SC2232"  , 1920, 1080},
        {"SC2230"  , 1920, 1080},
        {"GC2053"  , 1920, 1080},
        {"c2399"   , 1920, 1080},
        {"GC5603"  , 2944, 1664},    
        {"SC230AI" , 2304, 1296},  
        {"SC200AI" , 2304, 1296}, 
        {"NONE"    , 1280, 720 },
    };

    ArgOptS maps[] = {
        {"?"        , ARG_TYPE_ASK  , NULL                         , NULL             , 0                       },
        {"act"      , ARG_TYPE_ACT  , "list|set"                   , NULL             , 0                       },
        {"sensor"   , ArgTypesString, NULL                         , outer->sensor    , sizeof(outer->sensor)   },
        {"flash"    , ArgTypesString, NULL                         , outer->flash     , sizeof(outer->flash)    },
        {"maxheight", ArgTypesInt   , "720|960|1080|1296|1440|1664|1856", &outer->maxheight, sizeof(outer->maxheight)},
        {"cpu"      , ArgTypesString, NULL                         , outer->cpu       , sizeof(outer->cpu)      },
        {"hdetect"  , ArgTypesInt   , "0|1"                        , &outer->hdetect  , sizeof(outer->hdetect)  },
        {"feature"  , ArgTypesInt   , "0~100"                      , &outer->feature  , sizeof(outer->feature)  },
        {"lang"     , ArgTypesInt   , "0~128"                      , &outer->lang    , sizeof(outer->lang)     },
        {"End"      , ArgTypesEnd   , NULL                         , NULL             , 0                       },
    };

    /*
    SysCustomS custom = {0};
    conf_get_capability(&custom);
    for (i = 0; i < ARRAY_SIZE(sensor_width_hei); i++) {
        if (sensor_width_hei[i].width == 1920 && sensor_width_hei[i].height == 1080) {
            if (290 > custom.pixels) {
                sensor_width_hei[i].width = 2048;
                sensor_width_hei[i].height = 1152;
            } else {
                sensor_width_hei[i].width = WIDTH_2M_3M;
                sensor_width_hei[i].height = HEIGHT_2M_3M;
            }
        }
    }*/

    if (opts != NULL) {
        for (i = 2; maps[i].argType != ArgTypesEnd; i++) {
            if (strcmp(maps[i].pOpt, opts[i].pOpt) != 0) {
                ERR("[%d, %s]param is not match", i, maps[i].pOpt);
                return FAILURE;
            }
        }
    }

    if (!strncasecmp("get", action,strlen("get"))) {
        if (has_read) {
            memcpy(outer, &inner, sizeof(BOOTARGS_CFG_S));
        } else {
            ret = ReadCmdLine(maps);
            if (ret != SUCCESS) {
                return FAILURE;
            }
            memcpy(&inner, outer, sizeof(BOOTARGS_CFG_S));
            /*
            if (290 > custom.pixels) {
                inner.maxheight = HEIGHT_1080P;
            }else{
                inner.maxheight = HEIGHT_2M_3M;
            }*/
            if (inner.feature == 0) {
                DBG("___ reset feature from %d to 1\n", inner.feature);
                inner.feature = 1;
            }

            if (inner.maxheight == 0) {
                /* when not find, set a height default */
                int i;
                for (i = 0; i < ARRAY_SIZE(sensor_width_hei); i++) {
                    if (0 == strcmp(sensor_width_hei[i].sensor, inner.sensor)) {
                        inner.maxheight = sensor_width_hei[i].height;
                        break;
                    }
                }
            }
            if (CPU_T31X == system_get_cpu_type())
                inner.maxheight = 1920;
            if ((SENSOR_GC4663 == system_get_snsr_type()) || (SENSOR_OS04D10 == system_get_snsr_type()) 
                || (SENSOR_SP4329 == system_get_snsr_type()) || (SENSOR_SC465SL ==  system_get_snsr_type())
                || (SENSOR_SC4336P ==  system_get_snsr_type()))
                inner.maxheight = 1440;
            if (SENSOR_GC5603 == system_get_snsr_type())
                inner.maxheight = 1856;
            if ((SENSOR_SC230AI == system_get_snsr_type()) || (SENSOR_SC200AI == system_get_snsr_type()))
                inner.maxheight = 1440;
            if (SENSOR_SC235 == system_get_snsr_type())
                inner.maxheight = 1296;

            if (LoadFile("/ipc/etc/maxheight", maxheightbuf, sizeof(maxheightbuf)-1) > 0){
                snprintf(maxheightstr,sizeof(maxheightstr)-1,"%s",maxheightbuf+strlen("maxheight="));
                inner.maxheight = atoi(maxheightstr);
            }

            inner.hdetect = 1;
            memcpy(outer, &inner, sizeof(BOOTARGS_CFG_S));
            has_read = TRUE;
            SYSLOG("read sensor[%s] and maxheight[%d]\n", inner.sensor, inner.maxheight);
        }
    } else if (!strncasecmp("set", action,strlen("set"))) {
        int i;
        for (i = 0; i < ARRAY_SIZE(sensor_width_hei); i++) {
            /* check if maxheight <= sensor_width_hei[sensor_id].height */
            if (0 == strcmp(sensor_width_hei[i].sensor, outer->sensor)) {
                SYSLOG("maxheight[%d] sensor-height[%d]\n",
                       outer->maxheight, sensor_width_hei[i].height);
                if (outer->maxheight > sensor_width_hei[i].height) {
                    SYSLOG("Warning: outer->maxheight > sensor_width_hei[i].height\n");
                }
                break;
            }
        }
        ret = set_bootargs(maps);
        inner.hdetect = outer->hdetect;
        inner.feature = outer->feature;
        inner.lang  = outer->lang;
        inner.maxheight = outer->maxheight;
    }

    return ret;
}

int is_spiflash_board()
{
    BOOTARGS_CFG_S outer = {{0,},};
    config_bootargs("get", NULL, &outer);
    return (0 == strcmp(outer.flash, "SF") ? 1 : 0);
}

static BOOTARGS_CFG_S get_bootargs()
{
    static int got = 0;
    static BOOTARGS_CFG_S outer = {{0,},};

    if (got) {
        goto __exit;
    }

    if (SUCCESS == config_bootargs("get", NULL, &outer)) {
        got = TRUE;
    }

__exit:
    return outer;
}

int get_feature()
{
    BOOTARGS_CFG_S outer = get_bootargs();
    return outer.feature;
}

int get_maxheight_2MTo3M()
{
    char resultbuf[256] = {0};
    static int maxheight = 0;
    if(maxheight > 0){
        return maxheight;
    }else{
        memset(resultbuf, 0, sizeof(resultbuf));
        if (ReadCmdResult((char*)"cat /proc/cmdline | xargs -n1 | awk -F'=' '/maxheight/{print $2}'", resultbuf, sizeof(resultbuf)) < 0) {
            SYSLOG("get maxheight fail\n");
            return -1;
        }
    }
    maxheight = atoi(resultbuf);
    maxheight = 1440;
    return maxheight;
}

const char *get_uid()
{
    // 读取uid,暂时写死，后续可使用 unique ID 如: cpu-id, flash-id
    return "unique_id";
}

const char *get_cpuid()
{
    int ret = 0;
    static const char *p = NULL;
    static char devid[MAX_CPUID_LEN] = {0};
    ot_unique_id unique_id={0};
    
    if (p != NULL) {
       return p;
    }

    ret = ss_mpi_sys_get_unique_id(&unique_id);
    if (ret != 0){
        ERR("read CPU_ID error\r\n");
        return "cpuid_failed";
    }

    snprintf(devid, MAX_CPUID_LEN, "%08x%08x" ,unique_id.id[0], unique_id.id[1]);
    DBG("get cpuid:%s\n", devid);
    p = devid;

    return p;
}

int uboot_txconf_set(char *name, char *szTxConf)
{
    int ret = SUCCESS;
    
    if (is_spiflash_board()) {
        if (SUCCESS != uboot_set_bootargs_param_nor(name,szTxConf)) {
            LOG("uboot_txconf_set failed\n");
            ret = FAILURE;
        }
    } else {
        ret = uboot_set_bootargs_param_nand(name,szTxConf);
    }
    return ret;
}

int uboot_ssid_set(char *szbuf)
{
    int ret = SUCCESS;

    if (is_spiflash_board()) {
        if (SUCCESS != uboot_set_bootargs_param_nor("ssid", szbuf)) {
            LOG("uboot_aliconf_set failed\n");
            ret = FAILURE;
        }
    } else {
        ret = uboot_set_bootargs_param_nand("ssid", szbuf);
    }
    return ret;
}

int uboot_ssid_get(char *ssid, int len)
{
    if (ssid == NULL) {
        ERR("%s param NULL\n", __func__);
        return FALSE;
    }
    
    ArgOptS opts[] = {
        {"?"      , ARG_TYPE_ASK  , NULL      , NULL   , 0  , NULL},
        {"act"    , ARG_TYPE_ACT  , "list|set", NULL   , 0  , NULL},
        {"ssid"   , ArgTypesString, NULL      , ssid   , len, NULL},
        {"End"    , ArgTypesEnd   , NULL      , NULL   , 0  , NULL},
    };

    config_uboot_env("get", opts, NULL);

    if (strlen(ssid) <= 0) {
        DBG("%s error\n", __func__);
        return FALSE;
    }
    return TRUE;
}

