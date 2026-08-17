#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "debug.h"
#include "conf_nand.h"
#include <mtd/mtd-user.h>
#include "utils.h"
#include "logapi.h"

#define ENV_SIZE    (64 * 1024UL)

typedef struct {
    unsigned int crc;
    char data[ENV_SIZE-4];
} UBOOT_PARAM_S;

int fd_spi;

static mtd_info_t mtdInfo = {0, };
static UBOOT_PARAM_S *ubootParam = NULL;
static pthread_mutex_t mutex_spi = PTHREAD_MUTEX_INITIALIZER;

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

static int spi_flash_erase(int fd, uint32_t offset, uint32_t bytes)
{
    int memerase (int fd,struct erase_info_user *erase) {
        return (ioctl (fd,MEMERASE,erase));
    }
    int err;
    struct erase_info_user erase = {0};
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

static int spi_flash_read (int fd, uint32_t offset, const char *buf, size_t len)
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

static int spi_flash_write (int fd, uint32_t offset, const char *buf, size_t len)
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

static int NandInit(char *devName)
{
    memset(&mtdInfo, 0, sizeof(mtdInfo));

    // Open MTD device
    if (-1 == (fd_spi = open(devName, O_RDWR))) {
        DBG("open flash error %d %s\n", errno, strerror(errno));
        goto __exit;
    }

    // Fill in MTD device capability structure
    if (ioctl(fd_spi, MEMGETINFO, &mtdInfo)) {
        DBG("ioctl MEMGETINFO error %d %s\n", errno, strerror(errno));
        goto __exit;
    }

    // Make sure device page sizes are valid
	DBG("mtdInfo.oobsize : %d\n", mtdInfo.oobsize);
	DBG("mtdInfo.writesize : %d\n", mtdInfo.writesize);
/*
    if (!(0 == mtdInfo.oobsize && (256 == mtdInfo.writesize || 65536 == mtdInfo.writesize))) {
        DBG("Unknown flash (not normal NAND)\n");
        goto __exit;
    }*/

    // Print informative message
    DBG("Totol size %u, Block size %u, page size %u, OOB size %u\n",
        mtdInfo.size, mtdInfo.erasesize, mtdInfo.writesize, mtdInfo.oobsize);
    return SUCCESS;

__exit:
    if (0 < fd_spi) {
        close(fd_spi);
        fd_spi = -1;
    }
    return FAILURE;
}

static void NandUninit(void)
{
    if (0 < fd_spi) {
		fsync(fd_spi);
        close(fd_spi);
        fd_spi = -1;
    }
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
    DBG("in spi\n");

    int ret;

    if (NULL == (ubootParam = malloc(sizeof(UBOOT_PARAM_S)))) {
        return FAILURE;
    }

    memset(ubootParam, 0, sizeof(UBOOT_PARAM_S));

    if (SUCCESS != NandInit("/dev/mtd1")) {
        DBG("fail \n");
        return FAILURE;
    }

    ret = spi_flash_read(fd_spi, 0, (char *)ubootParam, sizeof(UBOOT_PARAM_S));
    return_val_if_fail(ret == SUCCESS, FAILURE);

    printenv(ubootParam);

    return SUCCESS;
}

static char *UbootGetEnv(char *envAddr, char *name)
{
    int i, nxt;

    for (i = 0; envAddr[i] != '\0'; i = nxt + 1) {
        char *val = NULL;

        for (nxt = i; envAddr[nxt] != '\0'; ++nxt) {
            if (nxt >= ENV_SIZE) {
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
    if (len > (envAddr + ENV_SIZE - 1 - env)) {
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

    if (SUCCESS != UbootSetEnv(ubootParam->data, "bootargs", bufTmp)) {
        goto __exit;
    }

    return SUCCESS;

__exit:
    return FAILURE;

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

int SetBootargs_spi(ArgOptS opts[])
{
    int ret = SUCCESS;

    pthread_mutex_lock(&mutex_spi);
    ret = UbootParamInit();
    goto_tag_if_fail(ret == SUCCESS, __exit);

    ret = UbootSetBootargsStruct(opts);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    if (SUCCESS != spi_flash_erase(fd_spi, 0, ENV_SIZE)) {
        LOG("%s spi_flash_erase failed\n", __FUNCTION__);
        ret = FAILURE;
    }
    if (SUCCESS != spi_flash_write(fd_spi, 0, (char *)ubootParam, ENV_SIZE)) {
        LOG("%s spi_flash_write failed\n", __FUNCTION__);
        ret = FAILURE;
    }

__exit:
    UbootParamUninit();
    pthread_mutex_unlock(&mutex_spi);
    return ret;
}

int uboot_set_bootargs_param_nor(char *name, char *value)
{
    return_val_if_fail(NULL != name && NULL != value, FAILURE);
    
    int ret = SUCCESS;

    pthread_mutex_lock(&mutex_spi);
    goto_tag_if_fail(NULL != value, __exit);

    ret = UbootParamInit();
    goto_tag_if_fail(ret == SUCCESS, __exit);

    // save ubootenv info
    ret = UbootSetEnv(ubootParam->data, name, value);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    ret = UbootSetBootargs(name, value);
    goto_tag_if_fail(ret == SUCCESS, __exit);

    if (SUCCESS != spi_flash_erase(fd_spi, 0, ENV_SIZE)) {
        LOG("%s spi_flash_erase failed\n", __FUNCTION__);
        ret = FAILURE;
    }
    if (SUCCESS != spi_flash_write(fd_spi, 0, (char *)ubootParam, ENV_SIZE)) {
        LOG("%s spi_flash_write failed\n", __FUNCTION__);
        ret = FAILURE;
    }

__exit:
    UbootParamUninit();
    pthread_mutex_unlock(&mutex_spi);
    return ret;
}


int config_uboot_env_spi(char* action, ArgOptS opts[], char *msgbuf)
{
    int  i = 0;
    int ret = SUCCESS;

    pthread_mutex_lock(&mutex_spi);
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
                LOG("spi UbootSetEnv of %s=%s failed\n", (char *)opts[i].pOpt, (char *)opts[i].pSetValue);
                ret = FAILURE;
            }
        }
        if (SUCCESS != spi_flash_erase(fd_spi, 0, ENV_SIZE)) {
            LOG("spi_flash_erase failed\n");
            ret = FAILURE;
        }
        if (SUCCESS != spi_flash_write(fd_spi, 0, (char *)ubootParam, ENV_SIZE)) {
            LOG("spi_flash_write failed\n");
            ret = FAILURE;
        }
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
    pthread_mutex_unlock(&mutex_spi);
    return ret;
}

