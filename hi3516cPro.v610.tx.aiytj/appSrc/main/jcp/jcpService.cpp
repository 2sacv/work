/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpService.cpp
 * @Created Time : 2013-12-24
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "linux_list.h"

#include "debug.h"

#include "confapi.h"
#include "jcpService.h"
#include "utils.h"

#include "system_ctrl.h"
#include "jcpService.h"
#include "jcpCmd.h"
#include "cmdline_parse.h"
#include "jcpCmdImplement.h"
#include "jconfig.h"
#include "js_scheduler.h"

#include "HashTable.hh"
#include "g_log.h"
#include "g_run.h"

static char quit = 0;
static int authorized = FALSE;
static HashTable* hashtable = NULL;
static JSTCHandle  g_hdl_jcp = NULL;
static JSScheduler g_sch_jcp = NULL;
static int g_sock_jcp = 0;

static int create_nonblock_tcp_socket(int port)
{
    int                 opt = 1;
    struct sockaddr_in  sock_addr;
    socklen_t           sock_addrlen;
    int                 err;
    int fflag;
    int val;
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0) {
        SYSLOG("socket error :%s\n", strerror(errno));
        return -1;
    }

    SYSLOG("success create socket (socket:%d)\n", sockfd);

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(port);

    err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int));
    if(err < 0) {
        SYSLOG("setsockopt error :%s\n", strerror(errno));
    }

    err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(int));
    if(err < 0) {
        SYSLOG("setsockopt error :%s\n", strerror(errno));
    }

    sock_addrlen = sizeof(struct sockaddr);
    err = bind(sockfd, (struct sockaddr *)&sock_addr, sock_addrlen);
    if(err < 0) {
        SYSLOG("socket(%d), bind (port:%d) error[%d] :%s\n", sockfd, port, errno, strerror(errno));
        goto cleanup;
    }

    fflag = fcntl(sockfd, F_GETFL, 0);
    fflag = fcntl(sockfd, F_SETFL, fflag|O_NONBLOCK);

    val = fcntl(sockfd,F_GETFD);
    val |= FD_CLOEXEC;
    fcntl(sockfd,F_SETFD,val);
    
    if(fflag == -1)
        SYSLOG("fcntl error :%s\n", strerror(errno));

    if (listen(sockfd, 5) != 0) {
        SYSLOG("listen error :%s\n", strerror(errno));
        goto cleanup;
    }

    return sockfd;

cleanup:
    close(sockfd);
    return -1;
}


static int exec_jcp(char *resp, int resplen, int argc, char *argv[])
{

    if(resp == NULL || argv == NULL) {
        sprintf(resp, "exec_jcp parameter error!");
        return FAILURE;
    }

    JcpPrcessFunc handle = (JcpPrcessFunc)hashtable->Lookup(argv[0]);

    if (handle != NULL) {
        return handle(resp, resplen, argc, argv);
    } else {
        sprintf(resp, "no such command! %s", argv[0]);
        return FAILURE;
    }

}

int max_jcp_dgram = JCP_MAX_LEN-512;

/*
 * 1. 在 respMsg 自动追加 '\r\n'
 * 2. snprintf() 在 n 越界时，返回的是 fmt 后的真实长度，而不是 n
 * 3. 省掉 output[JCP_MAX_LEN] 4KB
 **/
#define SUCC_TAG "[Success]"
#define FAIL_TAG "[Error]"
static int exec_jcp_cmdline(char *cmdline, int jcpLen, char *respMsg, int bufSize)
{
    int ret = SUCCESS;
    int argc = 0;
    char **argv = NULL;
    int tag_len = strlen(SUCC_TAG);
    int64_t ms_start = 0;

    if (get_g_run(jcp, RUN_JCP_SINGL_TIME)) {
        ms_start = mono_msec();
    }

    if (cmdline_parse_argv(cmdline, &argc, &argv) < 0) {
        snprintf(respMsg, bufSize-1, "[Fail]@%s|%d|\r\n", __FUNCTION__, __LINE__);
        ret = FAILURE;
        goto __out;
    }

    // 直接使用 respMsg + tag_len 作为输出缓冲区
    if (SUCCESS == exec_jcp(respMsg + tag_len, bufSize - tag_len - 3, argc, argv)) {
        memcpy(respMsg, SUCC_TAG, tag_len);
        strcat(respMsg, "\r\n");
        
        int nwrite = strlen(respMsg);
        if (nwrite > max_jcp_dgram) {
            max_jcp_dgram = nwrite;
            SYSLOG("got big output[%d] @cmd: %s\n", nwrite, cmdline);
        }
        if (nwrite > (bufSize-1)) {
            ERR("[%s] bufSize %d is not enough\r\n", cmdline, bufSize);
        }
    } else {
        int error_len = strlen(FAIL_TAG);
        memmove(respMsg + error_len, respMsg + tag_len, strlen(respMsg+tag_len)+1);
        memcpy(respMsg, FAIL_TAG, error_len);
        strcat(respMsg, "\r\n");
        ret = FAILURE;
    }

__out:
    if (get_g_run(jcp, RUN_JCP_SINGL_TIME)) {
        printf("____________ %s spend %.3fs\n", __func__, (mono_msec()-ms_start)/1000.0);
    }
    cmdline_free_argv(argc, argv);
    return ret;
}

static int handleFireJcpcmd(const char *cmdline, char *respbuf, int bufsize)
{
    static pthread_mutex_t jcp_lock = PTHREAD_MUTEX_INITIALIZER;
	char tmpArr[JCP_MAX_INPUT] = {0};
	char output[JCP_MAX_LEN] = {0};
	char *p = NULL;
	char *s = NULL;
	int numbers = 0;
    int ret = SUCCESS;

    printf("I am recving: [%s]\n", cmdline);

    if (respbuf == NULL) {
        respbuf = output;
        bufsize = sizeof(output);
    }

    strncpy(tmpArr,cmdline,sizeof(tmpArr) - 1);    
	s = (char *)tmpArr;
	
	do{
		if((p = strstr(s,"\\\""))){
			DBG("p:%s\n",p);
			memmove(s + (strlen(s)-strlen(p)), p + 1, strlen(p)-1);
			s[strlen(s)-1] = '\0';
			DBG("s:%s\n",s);
		}
		numbers ++;
	}while(numbers<9);

    drop_tail_space(s);
	
    pthread_mutex_lock(&jcp_lock);
    ret = exec_jcp_cmdline(s, strlen(s), respbuf, bufsize);
    pthread_mutex_unlock(&jcp_lock);

    if (ret == FAILURE || get_g_run(jcp, RUN_JCP_PRI_OUTPUT) ) {
        DBG("ret: %d\n%s\n", ret, respbuf);
    }

    return ret;
}

int jcpcmd_devbatch(char *respbuf, int buflen, int argc, char **argv)
{
    char *p = respbuf;
    char recv[JCP_MAX_LEN/2] = {0};
    int64_t ms_start = mono_msec();

    /*
     * 每次处理一对 {-jcp1 key1:key2:key3...}
     **/
    for (int i = 1; i < argc; i+=2) {
        if (argv[i][0] != '-') {
            break;                  // 一旦不成对，中止执行剩余的命令
        }
        char keys[256] = {0};
        int k_argc = 1;
        int k_full = FALSE;
        char *k_arr[10] = {0};     // 最大只有 10 个 key

        /*
         * 转换 keys 为数组 k_arr
         **/
        strncpy(keys, argv[i+1], sizeof(keys)-1);
        k_arr[0] = keys;
        size_t sz = strlen(keys);

        if (sz == 5 && keys[0] == '_' && keys[1] == 'a' && keys[4] == '_') {
            k_full = TRUE;
        } else {
            for (size_t i = 1; i < sz; i++) {
                if (keys[i] == ':') {
                    keys[i] = '\0';
                    if (keys[i+1]) {
                        k_arr[k_argc++] = &keys[i+1];  // : 的下个字符地址
                    }
                }
            }
        }

        /*
         * 组装 jcpcmd -act list，并 exec_jcp()
         **/
        const char *p_cmd = argv[i]+1;
        const char *tmp_argv[3] = { p_cmd, "-act", "list" };
        char tag_id[12] = {0};
        char *p_dot = NULL; 
        int do_grp = false;

        p += snprintf(p, buflen-(p-respbuf)-1, "batch_cmd=%s;", p_cmd); // must b4 *p_dot=0;

        /* 
         * 识别 devvecfg.id_2 osdstrcfg.index_3 等指定 grp_nr,
         * ";id=" or "#id=" ARG_TYPE_LIST_ID
         **/
        if (NULL != (p_dot = strrchr(p_cmd, '.'))) {
            char *hif = strchr(p_dot, '_');
            if (!hif) {
                printf("got no _ in %s\n", p_cmd);
                break;
            }

            int grp_nr = atoi(hif+1);
            *hif = '=';

            do_grp = true;
            *p_dot = '\0';
            snprintf(tag_id, sizeof(tag_id)-1, "%c%s;", grp_nr ? '#' : ';', p_dot+1);
        }
        
        /*
         * 执行命令
         **/
        recv[0] = '\0';
        exec_jcp(recv, sizeof(recv)-1, ARRAY_SIZE(tmp_argv), (char **)tmp_argv);

        if (k_full) {
            if (strstr(recv, "=")) {
                p += snprintf(p, buflen-(p-respbuf)-1, "%s#", recv);
            } else {
                p += snprintf(p, buflen-(p-respbuf)-1, "ooh_shit=1;#");
            }
            continue; // 跳过逐Key轮询
        }

        if (get_g_run(jcp, RUN_JCP_BATCH_STEP)) {
            for (int ii = 0; ii < k_argc; ii++) {
                printf("%s:%d %s\n", p_cmd, ii, k_arr[ii]);
            }
            printf("chk %s @recv: %s\n", tag_id, recv);
        }
        
        char *savptr = NULL;
        char *token = NULL;
        int left = k_argc;
        if (do_grp) {
            char *p_grp = strstr(recv, tag_id);
            if (p_grp) {
                token = strtok_r(p_grp, ";", &savptr);
            } else {
                //printf("got no tag_id %s\n", tag_id);
                p += snprintf(p, buflen-(p-respbuf)-1, "grp_id=ooh_shit;#");
                continue;
            }
        } else {
            token = strtok_r(recv, ";", &savptr);
        }

        /*
         * 轮询筛选出 k_arr[] 中的 key=val ，并进行组装
         **/
		while(token != NULL) {
            for (int k = 0; k < k_argc && left; k++) {
                if (k_arr[k] && strstr(token, k_arr[k]) && token[strlen(k_arr[k])] == '=') {
                    p += snprintf(p, buflen-(p-respbuf)-1, "%s;", token);
                    k_arr[k] = NULL;
                    left--; // 所有 key 匹配完成，不再匹配剩余
                    break;  // 每个 token 只匹配一次
                }
            }
			token = strtok_r(NULL, ";", &savptr);
		}
        
        if (left) {
            p += snprintf(p, buflen-(p-respbuf)-1, "ooh_shit=1;");
        }
        p += snprintf(p, buflen-(p-respbuf)-1, "#");
    }

    if (get_g_run(jcp, RUN_JCP_BATCH_TIME)) {
        printf("____________ bat spend %.3fs\n %s\n", (mono_msec()-ms_start)/1000.0, respbuf);
    }

    return 0;
}

int jcpcmd_sendrecv(const char *cmdline, char *respbuf, int buflen)
{
    handleFireJcpcmd(cmdline, respbuf, buflen);
    return 0;
}

int jcpcmd_sendrecv2(char *respbuf, int buflen, const char *format, ...)
{
    size_t n;
    char cmdline[JCP_MAX_INPUT] = {0};
    va_list arg_list;

    va_start(arg_list, format);
    n = vsnprintf(cmdline, sizeof(cmdline)-1, format, arg_list);
    va_end(arg_list);

    if (n >= sizeof(cmdline)-1) {
        printf("fail: too long cmdline %s\n", cmdline);
        return -1;
    }

    handleFireJcpcmd(cmdline, respbuf, buflen);
    return 0;
}

static void resp_cmdline(int sock, int ev, void *data)
{
    char respbuf[JCP_MAX_LEN] = {0};
    char cmdline[JCP_MAX_LEN] = {0};

    struct sockaddr_in peer_addr; 
    size_t peer_addr_size = sizeof(struct sockaddr_in);

    int  sfd = accept(sock, (struct sockaddr *)&peer_addr, &peer_addr_size);
    int  peer_local = FALSE;

    // static __thread char buffer[18] of inet_ntoa is thread-safe
    if (0 == strcmp("127.0.0.1", inet_ntoa(peer_addr.sin_addr))) {
        peer_local = TRUE;
    }

    return_if_fail(sfd >= 0);

    recv(sfd, cmdline, sizeof(cmdline)-1, 0);
    drop_tail_space(cmdline);

    int nwrite = 0;
    int authrizing = FALSE;

    if (!peer_local && !is_test_ver() && !authorized &&
        0 != strcmp(cmdline, "list") && 0 != strncmp(cmdline, "xkcd", 4)) {
        if (strstr(cmdline, "checkuser -act set")) {
            authrizing = TRUE;
        } else {
            nwrite = snprintf(respbuf, sizeof(respbuf), "[Error] Failure of authentication\r\n");
            printf(respbuf);
            goto __clean;
        }
    }

    handleFireJcpcmd(cmdline, respbuf, sizeof(respbuf));
    nwrite = strlen(respbuf);

    if (!authorized && authrizing) {
        printf("auth return %s\n", respbuf);
        if (strstr(respbuf, "result=0")) {
            authorized = TRUE;
        }
    }

__clean:
    send(sfd, respbuf, nwrite, 0);
    close(sfd);
    return;
}

void clr_jcp_authorization(int id, void *p_src, int size, void *ctx)
{
    DBG("______________ clr_jcp_authorization\n");
    authorized = FALSE;
}

void set_jcp_authorization(void)
{
    DBG("______________ set_jcp_authorization\n");
    authorized = TRUE;
}

int init_server_jcpcmd(void *data)
{
    hashtable = HashTable::create(STRING_HASH_KEYS);
    for(int i = 0; JcpCmdAll[i].cmd != NULL; i++) {
        hashtable->Add(JcpCmdAll[i].cmd, (void*)JcpCmdAll[i].func);
    }

    g_sch_jcp = (JSScheduler)data;
    g_sock_jcp = create_nonblock_tcp_socket(9999);
    js_create_reader_r(g_sch_jcp, g_sock_jcp, JS_READABLE, resp_cmdline, NULL, &g_hdl_jcp);

    attach_config(JEvent_UserCfgChg,     clr_jcp_authorization, NULL);
    attach_config(JEvent_AuthModecfgChg, clr_jcp_authorization, NULL);

    return 0;
}

void uninit_server_jcpcmd(void)
{
    quit = 1;
    detach_config(JEvent_UserCfgChg,     clr_jcp_authorization, NULL);
    detach_config(JEvent_AuthModecfgChg, clr_jcp_authorization, NULL);

    delete hashtable;
}
