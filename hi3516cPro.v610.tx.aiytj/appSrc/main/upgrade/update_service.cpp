#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <limits.h>
#include <fcntl.h>
#include <asm/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vfs.h>            /* for statfs() */
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>
#include <openssl/md5.h>

#include "update_service.h"
#include "search_service.h"
#include "update.h"
#include "debug.h"
#include "system_ctrl.h"
#include "utils.h"
#include "confapi.h"
#include "upnp.h"
#include "delay_exec.h"
#include "jconfig.h"
#include "net_check.h"
#include "ddnsstrategy.h"
#include "conf_nand.h"
#include "conf_list.h"
#include "encrypt.h"
#include "encode_main.h"
#include <new>
#include <cstdlib>
#include "debug.h"
#include "system_sch.h"
#include "system_main.h"

#include "encodeapi.h"
#include "encode_audio_queue.h"

#define UPDATE_CMD_FILEINFO     (0x10)
#define UPDATE_CMD_STARTTRAN    (0x11)

JUpdateService::JUpdateService(JSScheduler sch):sch_update(sch),
    soc_update(NULL), hdl_unpack(NULL),
    hdl_reboot(NULL), fSockFd(-1), fUpdateSockFd(-1), fUpdateConNum(0), upgrade_type(0),
    m_filefd(-1), m_iRecvDataLen(0), m_fileRecvLen(0), m_checkDataHead(true),
    m_breakpointTrans(false), m_bStartUpdate(false)
{
    memset(m_databuf, 0, sizeof m_databuf);
    memset(&m_fileInfo, 0, sizeof m_fileInfo);
}

JUpdateService::~JUpdateService()
{
    uninitUpdateService();
}

void JUpdateService::StartService()
{
    static JSTCHandle hdl_up = NULL;
    js_create_once(hdl_up, sch_update, 6*1000, DoinitUpdateService, this);
}

void JUpdateService::DoinitUpdateService(void *instance)
{
    JUpdateService* session = (JUpdateService*)instance;

    if(session->initUpdateService() == 0) {
        SYSLOG("create listening succ\n");
    } else {
        SYSLOG("create listening fail\n");
        DELAY_REBOOT_LINUX();
    }
}

void JUpdateService::incomingUpatehandle(int fd, int events, void *instance)
{
    JUpdateService* session = (JUpdateService*)instance;
    session->doIncomingUpdatehandle();
}

void JUpdateService::doIncomingUpdatehandle()
{
    int isocket = -1;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if ((isocket = accept(fSockFd, (struct sockaddr*)&clientAddr, &clientAddrLen)) == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
            return;
        }
        SYSLOG("socket accept error :[%d]%s, \n", errno,strerror(errno));
        return;
    }

    int curFlags = fcntl(isocket, F_GETFL, 0);
    fcntl(isocket, F_SETFL, curFlags|O_NONBLOCK);
	int insendSize = 64 * 1024;
	int sizeSize = sizeof(insendSize);
	int inRecvSize = 64 * 1024;
	if (setsockopt(isocket, SOL_SOCKET, SO_SNDBUF, (char*)&insendSize, sizeSize) < 0)
	{
		ERR("SetSocketSendBufSize error \n");
    	return;
	}
	sizeSize = sizeof(inRecvSize);
	if (setsockopt(isocket, SOL_SOCKET, SO_RCVBUF, (char*)&inRecvSize, sizeSize) < 0)
	{
		ERR("SetSocketRecvBufSize error \n");
    	return;
	}
    if(fUpdateConNum > 0) {
        SYSLOG("it is updating, refuse connect......\n");
        close(isocket);
        return;
    } else {
        DropCache(__func__);
        SYSLOG("Accept connection...\n");
        fUpdateConNum ++;
        fUpdateSockFd = isocket;
        m_checkDataHead = true;

        UpdateS inner = {0};
        get_config(handleUpdateCfg, inner);
        upgrade_type = inner.type;
        if (UPDATE_CUSTOM != upgrade_type) {
            encode_audio_queue_push_amr(AUDIO_UPGRADING_NO_OFF, TRUE);
        }
        js_create_reader_r(sch_update, fUpdateSockFd, JS_READABLE, incomingUpatePkghandle, (void *)this, &hdl_unpack);
    }

    return;
}

int JUpdateService::initUpdateService()
{
    int                 opt = 1;
    struct sockaddr_in  sock_addr;
    socklen_t           sock_addrlen;
    int                 err;
    int curFlags;
    int up_port;

    // conf api
    up_port = 8006;
	int insendSize = 64 * 1024;
	int sizeSize = sizeof(insendSize);
	int inRecvSize = 64 * 1024;
    fSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if(fSockFd < 0) {
        SYSLOG("socket error :%s\n", strerror(errno));
        return -1;
    }
    SYSLOG("success create socket (socket:%d)\n", fSockFd);

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(up_port);

    err = setsockopt(fSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int));
    if(err < 0)
        SYSLOG("setsockopt SO_REUSEADDR error :%s\n", strerror(errno));

    err = setsockopt(fSockFd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(int));
    if(err < 0)
        SYSLOG("setsockopt SO_REUSEPORT error :%s\n", strerror(errno));

    sock_addrlen = sizeof(struct sockaddr);
    err = bind(fSockFd, (struct sockaddr *)&sock_addr, sock_addrlen);
    if(err < 0) {
        SYSLOG("socket(%d), bind (port:%d) error[%d] :%s\n", fSockFd, up_port, errno, strerror(errno));
        // DELAY_REBOOT_LINUX();
        goto cleanup;
    }

    SYSLOG("bind (port:%d)\n", up_port);

    curFlags = fcntl(fSockFd, F_GETFL, 0);
    curFlags = fcntl(fSockFd, F_SETFL, curFlags|O_NONBLOCK);
    if(curFlags == -1)
        SYSLOG("fcntl error :%s\n", strerror(errno));
	
	if (setsockopt(fSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&insendSize, sizeSize) < 0)
	{
		ERR("SetSocketSendBufSize error \n");
    	goto cleanup;
	}
	
	sizeSize = sizeof(inRecvSize);
	if (setsockopt(fSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&inRecvSize, sizeSize) < 0)
	{
		ERR("SetSocketRecvBufSize error \n");
    	goto cleanup;
	}
    if (listen(fSockFd, 5) != 0) {
        SYSLOG("listen error :%s\n", strerror(errno));
        goto cleanup;
    }

    SYSLOG("Start listening.................\n");
    js_create_reader_r(sch_update, fSockFd, JS_READABLE, incomingUpatehandle, (void *)this, &soc_update);
    return 0;

cleanup:
    close(fSockFd);
    fSockFd = -1;
    return -1;
}

void JUpdateService::uninitUpdateService()
{
    if (fUpdateSockFd > 0) {
        js_delete_reader_r(&hdl_unpack);
        close(fUpdateSockFd);
        fUpdateSockFd = -1;
    }

    if (fSockFd > 0) {
        js_delete_reader_r(&soc_update);
        close(fSockFd);
        fSockFd = -1;
    }

    if (m_filefd > 0) {
        close(m_filefd);
        m_filefd = -1;
    }

    fUpdateConNum = 0;
    m_checkDataHead = true;
    m_iRecvDataLen = 0;
    memset(m_databuf, 0, sizeof m_databuf);
    memset(&m_fileInfo, 0, sizeof m_fileInfo);
    m_fileRecvLen = 0;
    m_breakpointTrans = false;
    m_bStartUpdate = false;
}

void  JUpdateService::incomingUpatePkghandle(int fd, int events, void *instance)
{
    JUpdateService* session = (JUpdateService*)instance;
    session->doIncomingUpdatePkghandle();
}

void JUpdateService::doIncomingUpdatePkghandle()
{
    int ret = 0;
    int readBytes = recv(fUpdateSockFd, m_databuf + m_iRecvDataLen, sizeof(m_databuf) - m_iRecvDataLen, 0);
    if (readBytes <= 0) {
        SYSLOG("readBytes :%d \n", readBytes);
        if(readBytes == 0) {
            goto cleanup;
        }

        SYSLOG("recv error :%s\n", strerror(errno));
        if (EAGAIN == errno || EINTR == errno) {
            return;
        } else {
            goto cleanup;
        }
    }
    m_iRecvDataLen += readBytes;

    if(m_checkDataHead) {
        // try get the lock of update.
        //if (FAILURE == conf_set_update_type(UPDATE_LINUX)) {
        //    SYSLOG("another update is being\n");
        //     return;
        // }

        int len_head = sizeof(UPDATE_CMD_HEAD_S) + sizeof(UPDATE_FILE_INFO_S);
        if(m_iRecvDataLen < len_head)
            return;
        m_checkDataHead = false;
        SYSLOG("Receive header data 12 bytes\n");
        if (UPDATE_CUSTOM != upgrade_type) {
            secs_delay_reboot(6*60, __func__);
            if (UPDATE_MCU != upgrade_type) {
                SYSLOG("UNINIT SERVERs to accelerate updating...\n");
                system_upmedia_uninit();
                conf_set_update_progressbar(UPDATE_BEGIN);
            }
        }

        int head_trans = chk_header(m_databuf);
        if (0 == head_trans) {
            SYSLOG("Parse header data\n");
            m_breakpointTrans = true;
            struct stat st;
            int bp_trans = chk_breakpoint(m_databuf, 0, &m_fileInfo, &st);
            if (0 == bp_trans) {
                SYSLOG("breakpoint transmit\n");
                m_filefd = open(UPDATE_TMP_FILE, O_WRONLY, 0666);
                if(m_filefd < 0) {
                    SYSLOG("open fail\n");
                    goto cleanup;
                }
                lseek(m_filefd, 0, SEEK_END);

                *(unsigned long *)(m_databuf + sizeof(UPDATE_CMD_HEAD_S)) = htonl(st.st_size);
                m_fileRecvLen = st.st_size;
            } else {
                SYSLOG("new transmit\n");
                m_filefd = open(UPDATE_TMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if(m_filefd < 0) {
                    SYSLOG("open fail\n");
                    goto cleanup;
                }
                lseek(m_filefd, 0, SEEK_SET);

                *(unsigned long *)(m_databuf + sizeof(UPDATE_CMD_HEAD_S)) = htonl(0);
                m_fileRecvLen = 0;
            }

            Writefully(fUpdateSockFd, m_databuf, sizeof(UPDATE_CMD_HEAD_S) + 0x04);

            m_iRecvDataLen -= len_head;
            if(m_iRecvDataLen)
                memcpy(m_databuf, m_databuf+len_head, m_iRecvDataLen);
        } else {
            SYSLOG("No header data found, this is an update from HTTP\n");
            uninit_server_search();
            printf("uninit_server_search end\n");
            uninit_ip_adaptive();
            printf("uninit_ip_adaptive end\n");

            m_filefd = open(UPDATE_TMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if(m_filefd < 0) {
                SYSLOG("open fail\n");
                goto cleanup;
            }
            lseek(m_filefd, 0, SEEK_SET);
        }
    }

    ret = Writefully(m_filefd, m_databuf, m_iRecvDataLen);
    if (m_iRecvDataLen != ret) {
        SYSLOG("write file fail, len:%d written:%d\n", m_iRecvDataLen, ret);
        goto cleanup;
    }

    m_fileRecvLen += m_iRecvDataLen;
    memset(m_databuf, 0, m_iRecvDataLen);
    m_iRecvDataLen = 0;
    m_bStartUpdate = true;

    while (m_fileRecvLen > 4*1024*1024 && !uninit_encode_done()) {
        static int ii = 0;
        DBG("--- uninit encode waiting %ds...\n", ++ii);
        sleep(1);
    }
    return;

cleanup:

    SYSLOG("cleanup connection, fUpdateSockFd: %d\n", fUpdateSockFd);
    if (0 < m_fileRecvLen && UPDATE_CUSTOM != upgrade_type) {
        js_delete_reader_r(&soc_update);
        close(fSockFd);
        fSockFd = -1;
    }

    if (fUpdateSockFd > 0) {
        js_delete_reader_r(&hdl_unpack);
        close(fUpdateSockFd);
        fUpdateSockFd = -1;
    }

    if (m_filefd > 0) {
        close(m_filefd);
        m_filefd = -1;
    }

    if (m_breakpointTrans) {
        /*
        struct utimbuf ubuf;
        ubuf.actime = ubuf.modtime = m_fileInfo.mtime;
        utime(UPDATE_TMP_FILE, &ubuf);
        */
        struct timeval times[2] = {{0}};
        times[0].tv_sec = times[1].tv_sec = m_fileInfo.mtime;
        utimes(UPDATE_TMP_FILE, times);

        SYSLOG("m_breakpointTrans: %d, m_fileRecvLen : %d, m_fileInfo.size : %d\n",
               m_breakpointTrans, m_fileRecvLen, m_fileInfo.size);

        if(m_fileRecvLen < m_fileInfo.size)
            m_bStartUpdate = false;
    }

    if (UPDATE_CUSTOM != upgrade_type) {
        js_delete_reader_r(&soc_update);
    }

    DropCache(__func__);
    UtilSystemCmd("free");

    if(m_bStartUpdate) {
		ret = de_encrypt_file(UPDATE_TMP_FILE);
		if (ret < 0) {
			SYSLOG("de_encrypt_file fail, unlink %s\n", UPDATE_TMP_FILE);

			sleep(1);
			DELAY_REBOOT_LINUX();
		}
		SYSLOG("de_encrypt_file succ...\n");
		
        if (SUCCESS != JCOUpdateBegin()) {
            SYSLOG("file transmit size: %d\n", m_fileRecvLen);
            unlink(UPDATE_TMP_FILE);
            DELAY_RESET_APPS();
        }
    } else {
        SYSLOG("file transmit error: espacially, a ZERO byte file\n");
//        if (0 < m_fileRecvLen) {
            conf_set_update_progressbar(UPDATE_ERR_ENTRY);
            /* sleep 4s for webpage read status */    		
            sleep(1);
            DELAY_REBOOT_LINUX();
//        } else {
//            conf_set_update_progressbar(0);
//        }
    }

    fUpdateConNum = 0;
    m_checkDataHead = true;
    m_iRecvDataLen = 0;
    memset(m_databuf, 0, sizeof m_databuf);
    memset(&m_fileInfo, 0, sizeof m_fileInfo);
    m_fileRecvLen = 0;
    m_breakpointTrans = false;
    m_bStartUpdate = false;
    return;
}

void JUpdateService::RestartUpdateService()
{
    SYSLOG("RestartUpdateService().........\n");

    uninitUpdateService();
    StartService();
}

int JUpdateService::chk_header(char *buf)
{
    UPDATE_CMD_HEAD_S *ptr_cmd = (UPDATE_CMD_HEAD_S *)buf;
    UPDATE_CMD_HEAD_S cmd_head;

    cmd_head.cmd = ntohs(ptr_cmd->cmd);
    cmd_head.length = ntohs(ptr_cmd->length);

    if (!(UPDATE_CMD_FILEINFO == cmd_head.cmd && 0x08 == cmd_head.length))
        return -1;

    return 0;
}

int JUpdateService::chk_breakpoint(char *buf, int len, UPDATE_FILE_INFO_S *p_info, struct stat *p_st)
{
    UPDATE_FILE_INFO_S *info  = (UPDATE_FILE_INFO_S *)(buf + sizeof(UPDATE_CMD_HEAD_S));
    p_info->size = ntohl(info->size);
    p_info->mtime = ntohl(info->mtime);

    UPDATE_CMD_HEAD_S *ptr_cmd = (UPDATE_CMD_HEAD_S *)buf;
    ptr_cmd->cmd = htons(UPDATE_CMD_STARTTRAN);
    ptr_cmd->length = htons(0x04);

    if (-1 == stat(UPDATE_TMP_FILE, p_st))
        return -1;

    if ((unsigned int)p_st->st_mtime == p_info->mtime &&
        (unsigned int)p_st->st_size  < p_info->size) {
        SYSLOG("tag bp\n");
        return 0;
    }

    return -1;
}

static JUpdateService* updateSer = NULL;

void init_server_upgrade(void *data)
{
    updateSer = new JUpdateService(data);
    updateSer->StartService();
    return;
}

void restart_server_upgrade()
{
    if(updateSer) {
        updateSer->RestartUpdateService();
    }
}

