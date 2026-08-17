/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : utils.c
 * Created Time : 2014-02-26
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <string.h>
#include <netdb.h>
#include <dirent.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <pthread.h>
#include <math.h>
#include <fnmatch.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <poll.h>
#include <glob.h>

#include "utils.h"
#include "debug.h"
//#include "iconv.h"

#define INET_IPADDR     (32)
#define CRYPT_SIZE      (32)
#define MAC_ADDR_LEN 12  // 输入MAC地址的长度（无分隔符）
#define MAC_STR_LEN 18   // 输出带冒号的MAC地址长度（包括空字符）
#define PATH_MAX 4096

static pthread_key_t pt_key_ntoa = -1;
static pthread_once_t pt_once_ntoa = PTHREAD_ONCE_INIT;
static pthread_key_t pt_key_crypt = -1;
static pthread_once_t pt_once_crypt = PTHREAD_ONCE_INIT;
static pthread_mutex_t pt_mutex_crypt = PTHREAD_MUTEX_INITIALIZER;      // use lock as no crypt_r in gcc lib

static void ptkey_ntoa_init(void)
{
    pthread_key_create(&pt_key_ntoa, NULL);
}

static void ptkey_crypt_init(void)
{
    pthread_key_create(&pt_key_crypt, NULL);
}

int gettid(void)
{
    return syscall(SYS_gettid);
}

/*
 * /proc/cmdline ' ' seperator string parser, key must end with '='
 * Retval: -1 failure
 *         >0 match number
 **/
int get_val(const char *haystack, const char *key, char *val)
{
    if (key == NULL || val == NULL) {
        return -1;
    }

    char *p = strstr(haystack, key);

    if (p == NULL) {
        return -1;
    }

    p += strlen(key);

    return sscanf(p, "%s", val);
}

int get_val2(const char *haystack, const char *key, const char *fmt, void *val)
{
    if (key == NULL || val == NULL) {
        return -1;
    }

    char *p = strstr(haystack, key);

    if (p == NULL) {
        return -1;
    }

    p += strlen(key);

    return sscanf(p, fmt, val);
}

void get_val3(const char *path, struct val3_map maps[], size_t sz)
{
    char buf[16*256] = {0};
    LoadFile(path, buf, sizeof(buf) - 1);
    for (int i = 0; i < sz; i++) {
        get_val2(buf, maps[i].key, maps[i].fmt, maps[i].p_val);
    }
}

void drop_tail_space(char *str)
{
    if (str == NULL) {
        return;
    }

    // 找到字符串的末尾
    char *end = str + strlen(str) - 1;

    // 从字符串末尾开始遍历，直到遇到非空白字符或到达字符串开头
    while (end > str && isspace(*end)) {
        // 将空白字符替换为字符串结束符 '\0'
        *end = '\0';
        --end;
    }
}

void replace_str(char *str, const char *find, const char *replace)
{
    int find_len = strlen(find);
    int replace_len = strlen(replace);

    char *ptr = strstr(str, find);
    while (ptr != NULL) {
        // 将找到的子字符串替换为指定的字符串
        memmove(ptr + replace_len, ptr + find_len, strlen(ptr + find_len) + 1);
        memcpy(ptr, replace, replace_len);
        // 继续查找下一个子字符串
        ptr = strstr(ptr + replace_len, find);
    }
}

/*======================================================================
    Write the requested buffer completely, accounting for interruptions
======================================================================*/
int Writefully(int fd, const void* buf, int nbytes)
{
    int nwritten;

    nwritten = 0;
    while (nwritten < nbytes)
    {
        int r;

        r = write(fd, (char*)buf + nwritten, nbytes - nwritten);
        if (0 > r)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                usleep( 10 * 1000 );
                continue;
            }
            else
            {
                printf("%s error:%s %d\n", __FUNCTION__, strerror(errno), fd);
                return r;
            }
        }
        else if (0 == r)
        {
            break;
        }

        nwritten += r;
    }

    return nwritten;
}


/*======================================================================
    Read the requested buffer completely, accounting for interruptions
======================================================================*/
int Readfully(int fd, void* buf, int nbytes)
{
    int nread = 0;
    while ( nread < nbytes )
    {
        int r = read(fd, (char*) buf + nread, nbytes - nread);
        if (0 > r)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                usleep( 10 * 1000 );
                continue;
            }
            else
            {
                return r;
            }
        }
        else if (0 == r)
        {
            break;
        }
        nread += r;
    }

    return nread;
}

int TouchFile(const char *dstFile)
{
    int fd = open(dstFile, O_RDWR | O_CREAT, 0666);

    if (fd >= 0) {
        close(fd);
        return SUCCESS;
    }

    SYSLOG("%s %s fail\n", __func__, dstFile);

    return FAILURE;
}


/*======================================================================
    writeFile is instead of system("echo \"contenx\" > dst");
======================================================================*/
int Write2File(const char *dstFile, const char *buf, int len)
{
    int fddst = -1;
    int ret = FAILURE;

    fddst = open(dstFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (-1 == fddst) {
        printf("open dst file %s fail\n", dstFile);
        ret = FAILURE;
        goto cleanup;
    }

    ret = Writefully(fddst, buf, len);
    if (0 > ret || len != ret) {
        printf("%s at write_fully fail, %d %d\n", __FUNCTION__, len, ret);
        ret = FAILURE;
        goto cleanup;
    }

    ret = SUCCESS;

cleanup:
    if (0 < fddst) {
        fsync(fddst);
        close(fddst);
        fddst = -1;
    }

    return ret;
}

/*======================================================================
    LoadFile is for read little file such as /proc/cmdline
======================================================================*/
int LoadFile(const char *srcFile, char *buf, int len)
{
    int nr = 0;
    int fdsrc;

    if (len <= 0) {
        DBG("buf len fail %d\n", len);
        return FAILURE;
    }

    fdsrc = open(srcFile, O_RDONLY);
    if (-1 == fdsrc) {
        DBG("open src file %s fail\n", srcFile);
        return FAILURE;
    }

    nr = Readfully(fdsrc, buf, len);
    close(fdsrc);

    return nr;
}

/*======================================================================
    appendFile is instead of system("echo \"contenx\" >> dst");
======================================================================*/
int AppendFile(const char *dstFile, const char *buf)
{
    int fddst = -1;
    int ret = FAILURE;
    int len = strlen(buf);

    fddst = open(dstFile, O_CREAT|O_WRONLY|O_APPEND);
    if (-1 == fddst) {
        SYSLOG("open dst file %s fail\n", dstFile);
        ret = FAILURE;
        goto cleanup;
    }

    ret = Writefully(fddst, buf, len);
    if (0 > ret || len != ret) {
        printf("%s at write_fully %s fail, %d %d\n", __FUNCTION__, dstFile, len,
               ret);
        ret = FAILURE;
        goto cleanup;
    }

    ret = SUCCESS;

cleanup:
    if (0 < fddst) {
        fsync(fddst);
        close(fddst);
        fddst = -1;
    }

    return ret;
}

int CopyFile(const char *dstFile, const char *srcFile)
{
    int fdsrc = -1;
    int fddst = -1;
    char buf[2048];
    int ret = FAILURE;
    int len = 0;

    fdsrc = open(srcFile, O_RDONLY);
    if (-1 == fdsrc)
    {
        DBG("open src file %s fail\n", srcFile);
        ret = FAILURE;
        goto cleanup;
    }

    //fddst = open(dstFile, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    fddst = creat(dstFile, 0755);
    if (-1 == fddst)
    {
        DBG("open dst file %s fail %s\n", dstFile, strerror(errno));
        ret = FAILURE;
        goto cleanup;
    }

    do
    {
        len = Readfully(fdsrc, buf, sizeof(buf));
        if (0 > len)
        {
            DBG("%s at read_fully fail\n", __FUNCTION__);
            ret = FAILURE;
            goto cleanup;
        }
        else if (0 == len)
        {// end of file
            break;
        }

        ret = Writefully(fddst, buf, len);
        if (0 > ret || len != ret)
        {
            DBG("%s at write_fully fail, %d %d\n", __FUNCTION__, len, ret);
            ret = FAILURE;
            goto cleanup;
        }
    } while (0 < len);

    ret = SUCCESS;

cleanup:
    if (0 < fdsrc)
    {
        close(fdsrc);
        fdsrc = -1;
    }

    if (0 < fddst)
    {
        close(fddst);
        fddst = -1;
    }

    return ret;
}


int UtilSystemCmd(const char *szCmd)
{
    pid_t pid = 0;
    int ret = SUCCESS;
    int nRetry = 5;

    if (NULL == szCmd)
    {
        return FAILURE;
    }

    while (0 < nRetry--)
    {
        if (0 > (pid = vfork()))
        {
            sleep(1);
        }
        else
        {
            break;
        }
    }

    if (0 > pid)
    {
        DBG("fork failed retry=%d\n", nRetry);
        ret = FAILURE;
    }
    else if (0 == pid)
    {
        int fd;

        for(fd=3; fd < getdtablesize(); fd++)
            close(fd);

        execl("/bin/sh", "sh", "-c", szCmd, NULL);
        exit(127);
    }
    else
    {
#define FORK_TIMEOUT
#ifdef FORK_TIMEOUT
        // 60s 超时功能，防止阻塞
        int i, i_60s = 35 /* 3.5*(0+35)/2 = 59.5 */;
        for (i = 0; i < i_60s; i++) {
            // int ok = waitpid(pid, &ret, WNOHANG);
            // printf("%d ok %d hello-2 %s\n", pid, ok, szCmd);
            usleep(i*100*1000);
            if (waitpid(pid, &ret, WNOHANG) > 0) {  // 有进程退出时返回 pid
                break;
            }
        }

        if (i >= i_60s) {
            printf("kill-2 %s %d\n", szCmd, pid);
            kill(pid+1, SIGKILL);
            kill(pid, SIGKILL);
        }
#else
        while (0 > waitpid(pid, &ret, 0)) {
            if (EINTR != errno) {
                ret = FAILURE;
                break;
            }
        }
#endif
    }

    return ret;
}

int UtilSystemCmd2(const char *format, ...)
{
    int n;
    char cmdline[512] = {0};
    va_list arg_list;

    va_start(arg_list, format);

    n = vsnprintf(cmdline, sizeof(cmdline)-1, format, arg_list);

    va_end(arg_list);

    if (n >= sizeof(cmdline)-1) {
        printf("fail: too long cmdline %s\n", cmdline);
        return -1;
    }
    //printf("cmdline: %s\n", cmdline);
    return UtilSystemCmd(cmdline);
}

int ReadCmdResult(const char* szCmd, char *read,int len)
{
    FILE *pstruPopen = NULL;
    char aszLoad[512] = {0,};
    //int s32ReadSize = 0;
    if((!szCmd) || (!read))
    {
        return FAILURE;
    }

    strcpy(aszLoad, szCmd);
    strcat(aszLoad, " 2>&1");

    pstruPopen = vpopen(aszLoad, "r");
    if(NULL != pstruPopen)
    {
        int n;
        memset(read,0,len);
        n = fread(read, 1, len, pstruPopen);
        //DBG("aBuffer : %s[%s]\n", aBuffer, read);
        vpclose(pstruPopen);
        return n;
    } else {
        return FAILURE;
    }
}

int GetAppCount(char *szAppName)
{
    int dirfd = -1;
    DIR *dirp = NULL;
    struct dirent *dp = NULL;
    int appCnt = 0;
    char pTmp[512];
    struct stat st;
    int fd = -1;
    char szFile[512];
    char *pPid = NULL;
    char *pName = NULL;
    char *p = NULL;
    int pid = 0;

    if (NULL == szAppName)
    {
        return FAILURE;
    }

    dirfd = open("/proc", O_RDONLY | O_DIRECTORY);
    if (dirfd <= 0) {
        ERR("open :%s error\n", "/proc");
        return FAILURE;
    }
    dirp = fdopendir(dirfd);
    if (dirp == NULL) {
        ERR("fdopendir :%s error\n", "/proc");
        if(fd) {
            close(fd);
        }
        return FAILURE;
    }
    rewinddir(dirp);

    for (;;)
    {
        dp = readdir(dirp);
        if (NULL == dp)
        {// get to end
            break;
        }

        if ('.' == dp->d_name[0])
        {// avoid . ..
            continue;
        }

        if ('0' > dp->d_name[0] || '9' < dp->d_name[0])
        {// avoid not pid dir
            continue;
        }

        snprintf(pTmp, sizeof(pTmp), "/proc/%s", dp->d_name);
        if (-1 == stat(pTmp, &st))
        {
            continue;
        }

        if (!S_ISDIR(st.st_mode))
        {// file
            continue;
        }

        snprintf(pTmp, sizeof(pTmp), "/proc/%s/stat", dp->d_name);
        if (-1 == stat(pTmp, &st))
        {
            continue;
        }

        if (!S_ISREG(st.st_mode))
        {// not file
            continue;
        }

        // read file /proc/pid/stat
        if (0 > (fd = open(pTmp, O_RDONLY)))
        {
            printf("open file %s fail\n", pTmp);
            continue;
        }

        memset(szFile, 0, sizeof(szFile));
        if (0 >= read(fd, szFile, sizeof(szFile)))
        {
            close(fd);
            fd = -1;
            continue;
        }

        // get "app_name" string
        if (NULL == (pPid = strtok_r(szFile, "(", &p)))
        {
            close(fd);
            fd = -1;
            continue;
        }

        if (NULL == (pName = strtok_r(NULL, "(", &p)))
        {
            close(fd);
            fd = -1;
            continue;
        }

        if (NULL == (p = strchr(pName, ')')))
        {
            close(fd);
            fd = -1;
            continue;
        }
        *p = '\0';

        if (strcmp(pName, szAppName))
        {
            close(fd);
            fd = -1;
            continue;
        }

        sscanf(pPid, "%d ", &pid);
        close(fd);
        fd = -1;

        if (-1 == kill(pid, 0))
        {
            ERR("GetAppCnt no such process %s\n", szAppName);
            continue;
        }

        appCnt++;
    }

    if (dirp)
    {
        closedir(dirp);
        dirp = NULL;
    }
    if (dirfd)
    {
        close(dirfd);
        dirfd = -1;
    }

    return appCnt;
}

int timestr_to_intarray(char *str, unsigned int *arry)
{
    if (str == NULL || NULL == arry) {
        return -1;
    }

    sscanf(str, "0:%u,1:%u,2:%u,3:%u,4:%u,5:%u,6:%u,",
        arry, arry+1, arry+2, arry+3,
        arry+4, arry+5, arry+6);

    return 0;
}

int intarray_to_timestr(char *str, unsigned int *arry)
{
    if (str == NULL || NULL == arry) {
        return -1;
    }

    sprintf(str, "0:%u,1:%u,2:%u,3:%u,4:%u,5:%u,6:%u,",
        arry[0], arry[1], arry[2], arry[3],
        arry[4], arry[5], arry[6]);

    return 0;
}


/*====================================================================
 discrib: 判断当前时间是否在参数所给的时间段内
 param:
    int TimeJudge(      -OUT  FALSE(0) 不在时间段内，TRUE(1) 在
        unsigned int *tda)  -IN  time day array，星期时间数组，每天用一个
        int类型表示，最高位(BIT32)表示当天是否有效，低24位
        每位分别表示0~23点是否有效
=====================================================================*/
BOOL TimeJudge(unsigned int *tda)
{
    time_t now;
    struct tm tm_now;

    now = time(0);
    if (-1 == now)
    {
        return FALSE;
    }

    localtime_r(&now, &tm_now);

    // 判断天
    if (!((tda[tm_now.tm_wday] >> 31) & 0x01))
    {
        return FALSE;
    }

    // 判断小时
    if ((tda[tm_now.tm_wday] >> tm_now.tm_hour) & 0x01)
    {
        return TRUE;
    }

    return FALSE;
}



int bytes_of_file(const char *filepath)
{
    struct stat stat_info;
    if(0 == stat(filepath, &stat_info)) {
        return  stat_info.st_size;
    }

    return -1;
}

char *j_inet_ntoa(struct in_addr in)
{
    char *pIPAddr = NULL;

    pthread_once(&pt_once_ntoa, ptkey_ntoa_init);

    if (NULL == (pIPAddr = (char *)pthread_getspecific(pt_key_ntoa)))
    {
        pIPAddr = (char *)malloc(INET_IPADDR);
        pthread_setspecific(pt_key_ntoa, pIPAddr);
    }

    if (pIPAddr)
    {
        memset(pIPAddr, 0, INET_IPADDR);
        inet_ntop(AF_INET, &in, pIPAddr, INET_IPADDR - 1);
        return pIPAddr;
    }
    else
    {
        return inet_ntoa(in);
    }
}

extern char* crypt( const char* key, const char* setting);
char *j_crypt(const char *key, const char *salt)
{
    char *pCrypt = NULL;

    pthread_once(&pt_once_crypt, ptkey_crypt_init);

    if (NULL == (pCrypt = (char *)pthread_getspecific(pt_key_crypt)))
    {
        pCrypt = (char *)malloc(CRYPT_SIZE);
        pthread_setspecific(pt_key_crypt, pCrypt);
    }

    if (pCrypt)
    {
        memset(pCrypt, 0, CRYPT_SIZE);
        pthread_mutex_lock(&pt_mutex_crypt);
        strncpy(pCrypt, crypt(key, salt), CRYPT_SIZE - 1);
        pthread_mutex_unlock(&pt_mutex_crypt);
        return pCrypt;
    }
    else
    {
        return crypt(key, salt);
    }
}

#ifndef __VALGRIND
void *system_malloc(int size)
{
    void *pPtr = 0;
    int retry = 0;

    //DBG("system_malloc size:%d start \n",size);
    if (size <= 0)
    {
        return pPtr;
    }
    pPtr = malloc((size_t)size);

    if (NULL == pPtr)
    {
        for(retry=0;retry<3;retry++)
        {
            ERR("malloc [%d] memory size:[%d] is ERROR \n",retry,size);
            DropCache(__func__);
            usleep(50000);
            pPtr = malloc((size_t)size);
            if (pPtr)
            {
                break;
            }
        }


    }

    if (pPtr)
    {
        memset(pPtr,0,size);
        //DBG("system_malloc size:%d end \n",size);
    }
    else
    {
        ERR("system_malloc size:%d end \n",size);
    }
    return pPtr;
}

void *system_valloc(int size)
{
    void *pPtr = valloc((size_t)size);
    int retry = 0;
    if (NULL == pPtr)
    {
        for(retry=0;retry<3;retry++)
        {
            ERR("valloc [%d] memory size:[%d] is ERROR \n",retry,size);
            DropCache(__func__);
            usleep(50000);
            pPtr = valloc((size_t)size);
            if (pPtr)
            {
                break;
            }
        }


    }

    if (pPtr)
    {
        memset(pPtr,0,size);
    }
    return pPtr;
}


void system_free(void *pPtr)
{
    if (pPtr)
    {
        free(pPtr);
    }
    return;
}
#endif

size_t system_fwrite(const void *ptr, size_t size, size_t nmemb,FILE *stream)
{
    size_t ret = 0;
    size_t i = 0;
    int retry = 0;
    do
    {
        ret = fwrite(ptr+i*size, size, nmemb-i, stream);
        if (ret > 0)
        {
            i += ret;
        }
        else
        {
            usleep(10000);
            if (retry++ < 5)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    while(i < nmemb);

    return i;
}

size_t system_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t ret = 0;
    size_t i = 0;
    int retry = 0;
    do
    {
        ret = fread(ptr+i*size, size,nmemb-i,stream);
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                usleep(10000);
                if (retry++ < 5)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        else if (0 == ret)
        {
            break;
        }
        else
        {
            i += ret;
            if (feof(stream))
            {
                break;
            }
        }
    }
    while(i < nmemb);

    return i;
}

char* safe_strncpy(char *dst, const char *src, size_t size)
{
    if (!size) return dst;
    dst[--size] = '\0';
    return strncpy(dst, src, size);
}

static long openmax = 0;
static pid_t *childpid = NULL;  // ptr to array allocated at run-time
static int maxfd = 0;           // from our open_max(), {Prog openmax}

long open_max(void)
{
#define OPEN_MAX_GUESS 1024
    if (openmax == 0)
    {// first time through
        errno = 0;
        if ((openmax = sysconf(_SC_OPEN_MAX)) < 0)
        {
            if (errno == 0)
                openmax = OPEN_MAX_GUESS;    // it's indeterminate
            else
                printf("sysconf error for _SC_OPEN_MAX");
        }
    }

    return(openmax);
}

FILE *vpopen(const char* cmdstring, const char *type)
{
    int pfd[2];
    FILE *fp = NULL;
    pid_t pid;
    int i = 0;

    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0)
    {
        errno = EINVAL;
        return (NULL);
    }

    if (childpid == NULL)
    {// first time through allocate zeroed out array for child pids
        maxfd = open_max();
        if ((childpid = (pid_t *)calloc(maxfd, sizeof(pid_t))) == NULL)
            return (NULL);
    }

    if (pipe(pfd) != 0)
    {
        return NULL;
    }

    if ((pid = vfork()) < 0)
    {
        return (NULL);   // errno set by fork()
    }
    else if (pid == 0)
    {// child
        if (*type == 'r')
        {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO)
            {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        }
        else
        {
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO)
            {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }

        // close all descriptors in childpid[]
        for (i = 0; i < maxfd; i++)
        {
            if (childpid[i] > 0)
                close(i);
        }

        execl("/bin/sh", "sh", "-c", cmdstring, (char *) 0);
        _exit(127);
    } else {
        // 60s 超时功能，防止阻塞
        int i, stat = 0, i_60s = 35 /* 3.5*(0+35)/2 = 59.5 */;
        for (i = 0; i < i_60s; i++) {
            // int ok = waitpid(pid, &ret, WNOHANG);
            // printf("%d ok %d hello-2 %s\n", pid, ok, szCmd);
            usleep(i*100*1000);
            if (waitpid(pid, &stat, WNOHANG) > 0) {  // 有进程退出时返回 pid
                break;
            }
        }

        if (i >= i_60s) {
            printf("kill-2 %s %d\n", cmdstring, pid);
            kill(pid+1, SIGKILL);
            kill(pid, SIGKILL);
        }
    }

    if (*type == 'r')
    {
        close(pfd[1]);
        if ((fp = fdopen(pfd[0], type)) == NULL)
            return (NULL);
    }
    else
    {
        close(pfd[0]);
        if ((fp = fdopen(pfd[1], type)) == NULL)
            return (NULL);
    }

    childpid[fileno(fp)] = pid; // remember child pid for this fd
    return(fp);
}

int vpclose(FILE *fp)
{
    int fd, stat = SUCCESS;
    pid_t pid;

    if (childpid == NULL)
        return(-1);     // popen() has never been called

    fd = fileno(fp);
    if ((pid = childpid[fd]) == 0)
        return(-1);     // fp wasn't opened by popen()

    childpid[fd] = 0;
    if (fclose(fp) == EOF)
        return (-1);

    while (waitpid(pid, &stat, 0) < 0)
    {
        if (errno != EINTR)
            return (-1); // error other than EINTR from waitpid()
    }

    return(stat);   // return child's termination status
}

/*
 *  1. **信号处理能力**：`nanosleep()` 允许在休眠过程中被信号中断，并能够返回剩余的睡眠时间，这使得它在需要处理异步事件时非常有用。
 *  2. **减少CPU占用**：`usleep()` 实现可能会让进程让出CPU使用权，导致实际延迟时间大于指定的时间，而 `nanosleep()` 则提供了更精确的控制，减少了不必要的CPU占用。
 *  3. `nanosleep()` 在时间精度、信号处理、标准化和CPU资源利用方面相比 `usleep()` 有明显优势。
 **/
void ms_sleep(time_t msec)
{
    int i;
    struct timespec ts;

    ts.tv_sec  = (msec/1000);
    ts.tv_nsec = (msec%1000)*1000*1000;

    for (i = 0; i < 5; i++) {
        if(nanosleep(&ts, NULL) == -1) {
            perror("Failed to nanosleep\n");
            continue;
        }
        break;
    }
}

int ms_clock_is_timeup(struct timespec *clock, int ms_timeout)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // 确保不 long 溢出
    int64_t ms_diff = ts.tv_sec*1000LL + ts.tv_nsec/1000000
                    - clock->tv_sec*1000LL - clock->tv_nsec/1000000;

    if (ms_diff >= ms_timeout || ms_diff < 0) {
        clock->tv_sec = ts.tv_sec;
        clock->tv_nsec = ts.tv_nsec;
        return TRUE;
    } else {
        return FALSE;
    }
}

int ms_clock_is_timeup2(struct timespec *clock, int ms_timeout, int *sec_left)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // 确保不 long 溢出
    int64_t ms_diff = ts.tv_sec*1000LL + ts.tv_nsec/1000000
                    - clock->tv_sec*1000LL - clock->tv_nsec/1000000;

    if (ms_diff >= ms_timeout || ms_diff < 0) {
        clock->tv_sec = ts.tv_sec;
        clock->tv_nsec = ts.tv_nsec;
        return TRUE;
    } else {
        if (sec_left) {
            *sec_left = (ms_timeout-ms_diff)/1000;
        }
        return FALSE;
    }
}

void ms_clock_reset(struct timespec *clock)
{
    clock_gettime(CLOCK_MONOTONIC, clock);
}

/* 不更新 prev */
int64_t ms_since_previous(struct timespec *prev)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // 确保不 long 溢出
    int64_t ms_diff = ts.tv_sec*1000LL + ts.tv_nsec/1000000
                    - prev->tv_sec*1000LL - prev->tv_nsec/1000000;

    if (ms_diff > 0x7FFFFFFF) {
        printf("%s overflow\n", __func__);
    }

    return ms_diff;
}

/* 更新 prev */
int64_t ms_since_previous2(struct timespec *prev)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // 确保不 long 溢出
    int64_t ms_diff = ts.tv_sec*1000LL + ts.tv_nsec/1000000
                    - prev->tv_sec*1000LL - prev->tv_nsec/1000000;

    if (ms_diff > 0x7FFFFFFF) {
        printf("%s overflow\n", __func__);
    }
    *prev = ts;
    return ms_diff;
}

time_t sec_since_previous(struct timespec *prev)
{
    struct timespec curr;
    clock_gettime(CLOCK_MONOTONIC, &curr);

    return (curr.tv_sec - prev->tv_sec);
}

/*
 * 对 elapsed_time 秒数4舍5入
 **/
time_t sec_since_previous2(struct timespec *prev)
{
    struct timespec curr;
    clock_gettime(CLOCK_MONOTONIC, &curr);

    double elapsed_time = (curr.tv_sec - prev->tv_sec) + (curr.tv_nsec - prev->tv_nsec) / 1e9;

    return (time_t)round(elapsed_time);
}

double mono_stamp()
{
    struct timespec curr = {0};

    clock_gettime(CLOCK_MONOTONIC, &curr);

    return (curr.tv_sec + 1.0 * curr.tv_nsec / (1000*1000*1000));
}

int64_t mono_msec()
{
    struct timespec curr = {0};

    clock_gettime(CLOCK_MONOTONIC, &curr);

    return (curr.tv_sec * 1000LL + curr.tv_nsec / (1000*1000));
}

int64_t mono_usec()
{
    struct timespec curr = {0};

    clock_gettime(CLOCK_MONOTONIC, &curr);

    return (curr.tv_sec * 1000 * 1000LL + curr.tv_nsec / 1000);
}

time_t mono_sec()
{
    struct timespec curr;
    clock_gettime(CLOCK_MONOTONIC, &curr);
    return curr.tv_sec;
};

char *get_timestr()
{
    static char outstr[64];
    time_t  t;
    struct tm p_tm;

    t = time(NULL);
    localtime_r(&t, &p_tm);
    if (strftime(outstr, sizeof(outstr), "%F %T", &p_tm) == 0) {
        goto __errout;
    }

    return outstr;

__errout:
    strncpy(outstr, "localtime_r error!", sizeof(outstr));
    return outstr;
}

char *get_timestr2(time_t epo, char *outstr, size_t sz)
{
    time_t  t;
    struct tm p_tm;

    if (epo) {
        t = epo;
    } else {
        t = time(NULL);
    }

    localtime_r(&t, &p_tm);
    if (strftime(outstr, sz-1, "%F %T", &p_tm) == 0) {
        goto __errout;
    }

    return outstr;

__errout:
    printf("%s err @epo %lld\n", __func__, t);
    strcpy(outstr, "localtime_r error!");
    return outstr;
}

/*
 * 音视频帧时间戳，新建 AOV 录像文件时，录像文件以首I帧时间戳命名，回放时 osd 显示同步性更好
 **/
double get_usec_of_day(void)
{
    struct timeval time_now;
    double time;

    gettimeofday(&time_now, NULL);
    time = (double)(time_now.tv_sec *1000000.0 + time_now.tv_usec);

    return time;
}

int64_t get_secs_of_today_midngt(void)
{
    struct tm tm_now;
    time_t now = time(NULL);

    localtime_r(&now, &tm_now);

    tm_now.tm_hour = 0;
    tm_now.tm_min = 0;
    tm_now.tm_sec = 0;

    return mktime(&tm_now);
}

int DumpFile(const char *dstFile, const char *buf, int len)
{
    int nr = 0;
    int fd;

    if (len <= 0) {
        DBG("buf len fail %d\n", len);
        return FAILURE;
    }

    fd = open(dstFile, O_CREAT|O_WRONLY|O_TRUNC);
    if (-1 == fd) {
        DBG("open src file %s fail\n", dstFile);
        return FAILURE;
    }

    nr = Writefully(fd, buf, len);
    fsync(fd);
    close(fd);

    return nr;
}

int DumpFile2(const char *dstFile, const char *format, ...)
{
    int result = 0; // 用于存储写入的字符数
    FILE *file;
    va_list args;

    // 打开文件，准备写入
    file = fopen(dstFile, "w");
    if (file == NULL) {
        perror("fopen");
        return -1; // 打开文件失败
    }

    // 读取变长参数，准备进行格式化写入
    va_start(args, format);
    result = vfprintf(file, format, args);
    va_end(args);

    // 检查是否有错误发生
    if (result < 0) {
        perror("vfprintf");
    }

    // 刷新文件流，确保所有缓存的数据都被写入到文件中
    if (fflush(file) != 0) {
        perror("fflush");
        result = -1;
    }

    // 同步文件到磁盘, 特殊文件 /dev/kmsg 会报错
    if (dstFile != strstr(dstFile, "/dev/")) {
        if (fsync(fileno(file)) != 0) {
            perror("fsync");
            result = -1;
        }
    }

    // 关闭文件
    if (fclose(file) != 0) {
        perror("fclose");
        result = -1;
    }

    return result;
}

int DumpKmsg(const char *format, ...)
{
    int len = 0;
    int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    if (fd > 0) {
        char buffer[1024] = {0};

        va_list args;
        va_start(args, format);
        len = vsnprintf(buffer, sizeof(buffer) - 1, format, args);  // 预留终止符
        va_end(args);

        write(fd, buffer, len);

        close(fd);
    }

    return len;
}

int DropCache(const char *func)
{
    // 第一步：调用 sync() 确保所有脏数据写入磁盘
    sync();

    // 第二步：将 3 写入 /proc/sys/vm/drop_caches 清理缓存
    int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
    if (fd == -1) {
        perror("Failed to open /proc/sys/vm/drop_caches");
        return 1;
    }

    // 将 "3" 转换为字符串写入
    const char *data = "3";
    if (write(fd, data, strlen(data)) == -1) {
        perror("Failed to write to /proc/sys/vm/drop_caches");
        close(fd);
        return 1;
    }

    // 关闭文件描述符
    close(fd);

    // 打印提示
    printf("%s from %s\n", __func__, func);

    return 0;
}

int CompactMemo(const char *from)
{
    const char *compact_file = "/proc/sys/vm/compact_memory";
    int fd;

    fd = open(compact_file, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "fail open %s: %s\n", compact_file, strerror(errno));
        return 1;
    }

    // 写入 '1' 触发内存合并
    ssize_t ret = write(fd, "1", 1);
    if (ret != 1) {
        fprintf(stderr, "fail write %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    close(fd);
    printf("%s from %s\n", __func__, from);
    return 0;
}

int CatFile(const char *file_name, char *buf, int buf_len)
{
    int rd_bytes = 0;
    int ret = -1;

    if (NULL == buf || buf_len <= 0) {
        return -1;
    }

    do {
        rd_bytes = LoadFile(file_name, buf, buf_len - 1);
        if (rd_bytes <= 0) {
            break;
        }

        buf[buf_len] = '\0';
        printf("%s\n", buf);

        ret = 0;
    } while (0);

    return ret;
}

/**
 *                          4KB    8KB   16KB   32KB   64KB  128KB  256KB  512KB 1024KB 2048KB 4096KB
 *                            0      1      2      3      4      5      6      7      8      9     10
 * Node 0, zone   Normal     20     41     39     15     10      8      3      1      1      0      0
 * 
 * 7 为 检测 512KB 的块
 * retval: /proc/buddyinfo 中 512KB 的块数量
 */
int get_num_of_kbytes(int i_bud)
{
    int num = 0;
    int arr[16] = {0};
    char bud[128] = {0};
    FILE *fp = NULL;

    do {
        fp = fopen("/proc/buddyinfo", "r");
        if (fp == NULL) {
            break;
        }

        if (NULL == fgets(bud, sizeof(bud), fp)) {
            break;
        }

        // printf("%s", bud);
        sscanf(bud, "%*s%*s%*s%*s%d%d%d%d%d%d%d%d%d%d%d", 
                    &arr[0], &arr[1], &arr[2], &arr[3], &arr[4],
                    &arr[5], &arr[6], &arr[7], &arr[8], &arr[9], &arr[10]);

        for (int i = i_bud; i <= 10; i++) {
            num += arr[i] * (1<<(i-i_bud));
        }
    } while(0);

    if(fp) {
        fclose(fp);
        fp = NULL;
    }

    // printf("%s 512KB blocks: %d\n", __func__, num);
    return num;
}

int rmdir_recursive(const char *path)
{
    DIR    *d = opendir(path);
    size_t  path_len = strlen(path);
    int     r = -1;

    if (d) {
        struct dirent *p;

        r = 0;

        while (!r && (p = readdir(d))) {
            int     r2 = -1;
            char   *buf;
            size_t  len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        r2 = rmdir_recursive(buf);
                    } else {
                        r2 = unlink(buf);
                    }
                }

                free(buf);
            }

            r = r2;
        }

        closedir(d);
    }

    if (!r) {
        r = rmdir(path);
    }

    return r;
}

int gpio_open_export(int gpio)
{
    int gpiofd = -1;
    char buf[8] = {0,};
    if(-1 == gpio) {
        return 0;
    }
    sprintf(buf,"%d",gpio);
    gpiofd = open("/sys/class/gpio/export",O_WRONLY);
    if (gpiofd == -1)
    {
        ERR( "Fail open device export!\n");
        return -1;
    }

    write(gpiofd,buf,strlen(buf));
    close(gpiofd);

    return 0;
}


int gpio_open_unexpot(int gpio)
{
    int gpiofd = -1;
    char buf[8] = {0,};
    if(-1 == gpio) {
        return 0;
    }
    sprintf(buf,"%d",gpio);
    gpiofd = open("/sys/class/gpio/unexport",O_WRONLY);
    if (gpiofd == -1)
    {
        ERR( "Fail open device unexport!\n");
        return -1;
    }

    write(gpiofd,buf,strlen(buf));
    close(gpiofd);

    return 0;
}

int gpio_open_set_direction(int gpio, const char* direct)
{
    int fd = -1;
    char path[64] = {0,};
    if(-1 == gpio) {
        return 0;
    }
    sprintf(path,"/sys/class/gpio/gpio%d/direction",gpio);
    fd = open(path,O_WRONLY);
    if (fd == -1)
    {
        ERR( "Fail open device:%s!\n", path);
        return -1;
    }

    write(fd,direct,strlen(direct));
    close(fd);
    return 0;
}

int gpio_open_get_direction(int gpio, char* direct)
{
    int fd = -1;
    char path[64] = {0,};

    if(-1 == gpio) {
        return 0;
    }

    sprintf(path,"/sys/class/gpio/gpio%d/direction",gpio);
    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        ERR( "Fail open device:%s!\n", path);
        return -1;
    }

    read(fd, direct,3);
    close(fd);

    return 0;
}

int gpio_open_get_value(int gpio,int *value)
{
    int fd;
    char path[64] = {0,};
    char ch;

    if(-1 == gpio) {
        return 0;
    }

    snprintf(path, sizeof(path)-1, "/sys/class/gpio/gpio%d/value", gpio);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ERR( "Fail open device:%s!\n", path);
        return fd;
    }

    read(fd, &ch, 1);
    if (ch != '0') {
        *value = 1;
    } else {
        *value = 0;
    }

    close(fd);

    return 0;
}

int gpio_open_set_value(int gpio,int value)
{
    FILE* fd = NULL;
    char buf[8] = {0,};
    char path[64] = {0,};
    if(-1 == gpio) {
        return 0;
    }

    sprintf(buf,"%d",value);
    snprintf(path,sizeof(path),"/sys/class/gpio/gpio%d/value",gpio);
    //printf("iogp_______ %.3lf gpio %d  val %d\n", get_usec_of_day()/1000000, gpio, value);

    fd = fopen(path,"wb");
    if (fd == NULL)
    {
        ERR( "Fail open device:%s!\n", path);
        return -1;
    }

    fwrite(buf,1,1,fd);

    fclose(fd);

    return 0;
}

int gpio_open_set_pullup(int gpionum)
{
#if 0
    char cmdbuf[64] = {0,};
    int baseaddr = 0x10010000;
    int value = 0;

    if(gpionum < 0 || gpionum > 32*4 -1)
        return -1;

    baseaddr += 0x1000 * (gpionum/32);
    baseaddr += 0x114;

    value = 1 << (gpionum % 32);

    sprintf(cmdbuf, "devmem 0x%x 32 0x%x", baseaddr, value);
    UtilSystemCmd(cmdbuf);
    DBG("run : %s\n", cmdbuf);
#endif
    return 0;
}

int gpio_open_clear_pullup(int gpionum)
{
#if 0
    char cmdbuf[64] = {0,};
    int baseaddr = 0x10010000;
    int value = 0;

    if(gpionum < 0 || gpionum > 32*4 -1)
        return -1;

    baseaddr += 0x1000 * (gpionum/32);
    baseaddr += 0x118;

    value = 1 << (gpionum % 32);

    sprintf(cmdbuf, "devmem 0x%x 32 0x%x", baseaddr, value);
    UtilSystemCmd(cmdbuf);
    DBG("run : %s\n", cmdbuf);
#endif
    return 0;
}

int gpio_open_set_pulldown(int gpionum)
{
#if 0
    char cmdbuf[64] = {0,};
    int baseaddr = 0x10010000;
    int value = 0;

    if(gpionum < 0 || gpionum > 32*4 -1)
        return -1;

    baseaddr += 0x1000 * (gpionum/32);
    baseaddr += 0x124;

    value = 1 << (gpionum % 32);

    sprintf(cmdbuf, "devmem 0x%x 32 0x%x", baseaddr, value);
    UtilSystemCmd(cmdbuf);
    DBG("run : %s\n", cmdbuf);
#endif
    return 0;
}

int gpio_open_clear_pulldown(int gpionum)
{
#if 0
    char cmdbuf[64] = {0,};
    int baseaddr = 0x10010000;
    int value = 0;

    if(gpionum < 0 || gpionum > 32*4 -1)
        return -1;

    baseaddr += 0x1000 * (gpionum/32);
    baseaddr += 0x128;

    value = 1 << (gpionum % 32);

    sprintf(cmdbuf, "devmem 0x%x 32 0x%x", baseaddr, value);
    UtilSystemCmd(cmdbuf);
    DBG("run : %s\n", cmdbuf);
#endif
    return 0;
}

int LoadFile2(const char *srcFile, const char *format, ...)
{
    int nr = 0;
    va_list arg_list;
    FILE *fp = fopen(srcFile, "r");

    if (fp == NULL) {
        printf("error %s @%s\n", strerror(errno), srcFile);
        return -1;
    }

    va_start(arg_list, format);
    nr = vfscanf(fp, format, arg_list);
    va_end(arg_list);

    fclose(fp);

    return nr;
}

int is_okey(const char *file)
{
    return access(file, F_OK) == 0;
}

/**
 * 使用 glob 展开通配符, 匹配符合条件的路径, 再使用 access 判断
 * retval: 1 存在
 * retval: 0 不存在
 */
int is_okey2(const char * file)
{
    int ret = 0;
    glob_t g;
    if (glob(file, GLOB_NOSORT, NULL, &g) == 0 && g.gl_pathc > 0) {
        ret = (access(g.gl_pathv[0], F_OK) == 0);
    }

    globfree(&g);
    return ret;
}

/*
 * stat FILE, to check
 * if Device No. diff with Papa, or inode is the same with papa, is a mountpoint
 **/
int is_mountpoint(const char *dir)
{
    if (NULL == dir) {
        DBG("dir is NULL\n");
        return FALSE;
    }

    struct stat st;
    struct stat st0;


    char papa[128] = {0};
    int nr = snprintf(papa, sizeof(papa)-1, "%s/..", dir);

    if (nr <= 0 || nr >= sizeof(papa)) {
        DBG("[%s] size error\n", papa);
        return FALSE;
    }

    int is_mnt = FALSE;

    if (stat(dir, &st) == 0 && stat(papa, &st0) == 0) {
        is_mnt = (st0.st_dev != st.st_dev) || (st0.st_ino == st.st_ino);
    }

    return is_mnt;
}

/**
 * @brief 检查指定目录中是否存在符合给定模式的文件
 *
 * @param  dir_path  目录路径, 必须是一个有效的目录路径
 * @param  pattern   文件名模式, 支持通配符(如 *, ?, [ ])
 * @param  full_path 如果不为 NULL, 存储匹配到的文件的完整路径
 *
 * @retval 1 目录中存在符合模式的文件
 * @retval 0 目录中不存在符合模式的文件; 目录无法打开, 此时会打印错误信息
 */
int is_pattern_exist(const char *dir_path, const char *pattern, char *full_path)
{
    DIR *dir;
    struct dirent *entry;

    // Open the directory
    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return FALSE;
    }

    // Iterate through the directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Check if the filename matches the pattern
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            if (full_path) {
                // Construct the full path to the file
                snprintf(full_path, PATH_MAX, "%s/%s", dir_path, entry->d_name);
            }

            closedir(dir);
            return TRUE; // File exists with the given pattern
        }
    }

    closedir(dir);
    return FALSE; // No file matches the pattern
}

/*
 * > 100, error, 255 for sdstat REPORT
 * 0~100, used_percent
 */
int df_used_p(const char *path) 
{
    struct statfs s = {.f_blocks=0};

    if (statfs(path, &s) != 0 || s.f_blocks == 0) {
        printf("path %s statfs failed f_blocks:%llu\n", path, s.f_blocks);
        return 101;
    }

    unsigned long blk_used = s.f_blocks - s.f_bfree;
    int used_p = 0;
    if (blk_used + s.f_bavail) {
        used_p = (blk_used * 100ULL + (blk_used + s.f_bavail)/2) / (blk_used + s.f_bavail);
    }

    printf("%s used_p:%d\n", path, used_p);

    return used_p;
}

char *itoa10(int i, char *a)
{
    if (a == NULL) {
        return a;
    }

    sprintf(a, "%d", i);

    return a;
}

int atoi10(const char *a)
{
    if (a == NULL) {
        return 0;
    }

    int i = 0;
    sscanf(a, "%d", &i);
    return i;
}

int fsck_exfat()
{
    static int ret = 0xBADF00D;

    if (ret == 0xBADF00D) {
        ret = is_okey("/ipc/bin/fsck.exfat");
    }
    return ret;
}

int use_exfat()
{
    if (fsck_exfat()) {
        char buf[1024] = {0};
        LoadFile("/proc/mounts", buf, sizeof(buf));

        if (NULL != strstr(buf, "exfat")) {
            return TRUE;
        }
    }
    return FALSE;
}

void hex_printf(const char *tag, char *str, size_t size)
{
    printf("tag: %s len: %d\n", tag, size);
    for (int j = 0; j < size; j++) {
        printf("%02hhx ", str[j]);
        if (15 == (j % 16)) {
            printf("\n");
        }
    }
    printf("\n");

    return;
}

int get_cpu_temperature()
{
    FILE *fp;
    char temperature[32] = {0};
    char *temp_ptr = NULL;

    fp = popen("lzbox temp", "r");
    if(fp == NULL)
    {
        ERR("popen lzbox temp failed\n");
        return FAILURE;
    }

    if(fgets(temperature, sizeof(temperature), fp) == NULL)
    {
        ERR("get cpu temperature errno\n");
        pclose(fp);
        return FAILURE;
    }

    pclose(fp);

    temp_ptr = temperature;
    while(*temp_ptr && !isdigit(*temp_ptr)) temp_ptr++;

    if(*temp_ptr == '\0')
    {
        ERR("Failed to parse temperature from output: %s\n", temperature);
        return FAILURE;
    }

    return atoi(temp_ptr);
}

uint32_t get_rgb_color(int color)
{
    sColor2RGB g_rgb_maps[] = {
        {E_COLOR_BLACK      , 0x000000},
        {E_COLOR_DARK_GRAY3 , 0x242424},
        {E_COLOR_DARK_GRAY2 , 0x494949},
        {E_COLOR_DARK_GRAY1 , 0x6D6D6D},
        {E_COLOR_LIGHT_GRAY3, 0x929292},
        {E_COLOR_LIGHT_GRAY2, 0xB6B6B6},
        {E_COLOR_LIGHT_GRAY1, 0xDBDBDB},
        {E_COLOR_WHITE      , 0xFFFFFF},
        {E_COLOR_RED        , 0xFF0000},
        {E_COLOR_GREEN      , 0x00FF00},
        {E_COLOR_BLUE       , 0x0000FF},
        {E_COLOR_LIGHT_BLUE , 0x0080FF},
        {E_COLOR_YELLOW     , 0xFFFF00},
        {E_COLOR_PURPLE_RED , 0xFF00FF}
    };
    int idx = 0;
    uint32_t rgb = 0, found_matched = FALSE;

    for (idx = 0; idx < ARRAY_SIZE(g_rgb_maps); idx++) {
        if (color == g_rgb_maps[idx].color) {
            rgb = g_rgb_maps[idx].rgb;
            found_matched = TRUE;
            break;
        }
    }

    if (!found_matched) {
        ERR("invalid color %d\n", color);
    }

    return rgb;
}
