/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : recordEmail.cpp
 * @Created Time : 2014-03-05
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "debug.h"
#include "utils.h"
#include "record_email_lib.h"
#include "record_base64.h"
#include "net_check.h"

#define  MAILPORT 25
#define  SSLPORT  465
#define  TLSPORT  587
#define  SMTPBUFLEN  (812 * 1024)
#define  SENDSIZE 256
#define  RECVSIZE 256

typedef struct
{
    int        fd;
    int        timeout;
    SSL*       ssl;
    SSL_CTX*   ctx;
}SockeHandleS;

typedef enum {
    AUTH_NORMAL,
    AUTH_SSL,
    AUTH_TLS,          //STARTTLS
}AUTH_METHOD;

typedef struct {
    const char* smtp_ip;
    AUTH_METHOD method;
    int port;
}email_info_t;

email_info_t email_info[] = {
    {"smtp-mail.outlook.com", AUTH_TLS, TLSPORT},
    {"smtp.mail.yahoo.cn"	, AUTH_TLS, TLSPORT},
    {"smtp.163.com"			, AUTH_SSL, SSLPORT},
    {"smtp.qq.com"			, AUTH_SSL, SSLPORT},
    {"smtp.exmail.qq.com"	, AUTH_TLS, TLSPORT},
    {"smtp.gmail.com"		, AUTH_TLS, TLSPORT}, //port ban
    {"smtp.21cn.com"		, AUTH_SSL, SSLPORT},
    {"smtp.sina.com.cn"		, AUTH_SSL, SSLPORT},
    {"smtp.sina.com"		, AUTH_SSL, SSLPORT},
    {"smtp.vip.sina.com"	, AUTH_SSL, SSLPORT},
    {"smtp.sohu.com"		, AUTH_SSL, SSLPORT},
    {"smtp.126.com"			, AUTH_NORMAL, MAILPORT},
    {"smtp.139.com"			, AUTH_SSL, SSLPORT},
    {"smtp.live.com"		, AUTH_TLS, TLSPORT},
    {"smtp.263.net"			, AUTH_NORMAL, MAILPORT},
    {"smtp.263.net.cn"		, AUTH_SSL, SSLPORT},
    {"smtp.x263.net"		, AUTH_NORMAL, MAILPORT},
    {"smtp.china.com"		, AUTH_SSL, SSLPORT},
    {"smtp.tom.com"			, AUTH_NORMAL, MAILPORT},
    {"smtp.etang.com"		, AUTH_NORMAL, MAILPORT},
    {"smtp.mail.yahoo.com"	, AUTH_SSL, SSLPORT},
};

static const char *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", };
static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", };

static int createSocket();
static int closeSocket(int fd);
static int setSocketNonBlock(int fd);
static int connectSock(int fd, char* remoteAddr, unsigned short remotePort, int sec);

static int sendData(int fd, const char *in, const int len, int timeout);
static int recvData(int fd, char *dataBuf, size_t size, int sec);

static int sslSend(SockeHandleS* sh, const char* in, const int len);
static int sslRecv(SockeHandleS* sh, char *dataBuf, int size, int sec);

static int emailSend(SockeHandleS* sh, const char* in, const int len);
static int emailRecv(SockeHandleS* sh, char *dataBuf, int size);

static int mailShowCerts(SSL * ssl);
static int mailSSLConnect(SockeHandleS *sh);
static int sslCreateNew(SockeHandleS *sh);
static int tlsCreateNew(SockeHandleS *sh, char *server_ip);
static void emailReclaim(SockeHandleS* sh);
static void mailGenerateDate(char *szDate, int len);

static int mailFullySendFile(SockeHandleS *sh, char *path, char *buf, int len);
static int mailFullySendBuf(SockeHandleS *sh, char *flame, int size, char *buf, int len);

static int handShakeSignal(SockeHandleS* sh, char *server_ip);
static int handStarttls(SockeHandleS* sh);
static int handAuthlogin(SockeHandleS* sh);
static int handUserName(SockeHandleS *sh, char *username);
static int handPassword(SockeHandleS *sh, char *passwd);
static int handMailFrom(SockeHandleS *sh, char *username);
static int handMailTo(SockeHandleS *sh, char* dstUser);
static int handDataFlag(SockeHandleS* sh);

static int MailReadFully(int fd, void *buf, int nbytes);

void mailSSLInit()
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    return ;
}

int mailUpData(char *filePath, char *buf, int size, char *filename, MailInfoS mus, int timeout)
{
    DBG("_______ mail begin\n");
    int ret = 0;
    int lineno = 0;
    int i = 0;
    SockeHandleS sh;
    char sendBuf[SENDSIZE*3] = {0};
    char recvBuf[RECVSIZE] = {0};
    char szDate[64] = {0};
    char *p = NULL;
    int email_port = MAILPORT;
    AUTH_METHOD auth_method = AUTH_NORMAL;

    time_t now;
    struct tm tml;

    now = time(NULL);
    localtime_r(&now, &tml);

    bzero(&sh, sizeof(sh));

    sh.timeout = timeout;

    sh.fd = createSocket();
    if(sh.fd < 0) {
        DBG("_______ mail fail\n");
        return MAIL_FAIL_UNKNOW;
    }

    setSocketNonBlock(sh.fd);

    DBG("mus.svrAddr : %s sh.timeout:%d\n", mus.svrAddr, sh.timeout);

    for (i = 0; i < ARRAY_SIZE(email_info); i++) {
        if (0 == strncmp(mus.svrAddr, email_info[i].smtp_ip, sizeof(mus.svrAddr))) {
            email_port = email_info[i].port;
            auth_method = email_info[i].method;
            break;
        }
    }

    if(connectSock(sh.fd, mus.svrAddr, email_port, sh.timeout) < 0) {
        DBG("%s Have no available MAIL service\n", mus.svrAddr);
        lineno = __LINE__; ret = MAIL_FAIL_SVR; goto cleanUp;
    }

    if (AUTH_NORMAL == auth_method || AUTH_SSL == auth_method) {
        if (AUTH_SSL == auth_method) {
            if (sslCreateNew(&sh) < 0) {
                DBG("ssl error!\n");
                lineno = __LINE__; ret = MAIL_FAIL_SSL; goto cleanUp;
            }

            if(mailSSLConnect(&sh) < 0 ) {
                DBG("SSL_Conncet failed!\n");
                lineno = __LINE__; ret = MAIL_FAIL_SSL; goto cleanUp;
            }

            if(mailShowCerts(sh.ssl) < 0) {
                lineno = __LINE__; ret = MAIL_FAIL_SSL; goto cleanUp;
            }
        }

        //服务器欢迎信息
        if(emailRecv(&sh, recvBuf, sizeof(recvBuf)-1) <= 0) {
            lineno = __LINE__; ret = MAIL_FAIL_SVR; goto cleanUp;
        }

        DBG("Welcome info : \n%s\n", recvBuf);

        //SMTP协议的握手信号
        if(handShakeSignal(&sh, mus.svrAddr) < 0) {
            lineno = __LINE__; ret = MAIL_FAIL_SVR; goto cleanUp;
        }
    } else {
        DBG("Use STARTTLS TCP\n");
        if(tlsCreateNew(&sh, mus.svrAddr) < 0) {
            DBG("ssl error!\n");
            lineno = __LINE__; ret = MAIL_FAIL_SSL; goto cleanUp;
        }

        if(mailSSLConnect(&sh) < 0 ) {
            DBG("SSL_Conncet failed!\n");
            lineno = __LINE__; ret = MAIL_FAIL_SSL; goto cleanUp;
        }

        if(mailShowCerts(sh.ssl) < 0) {
            DBG("SSL certification failed!\n");
            lineno = __LINE__; ret = MAIL_FAIL_SSL; goto cleanUp;
        }

        //SMTP协议的握手信号(STARTTLS之后还要再握手)
        if(handShakeSignal(&sh, mus.svrAddr) < 0) {
            lineno = __LINE__; ret = MAIL_FAIL_SVR; goto cleanUp;
        }
    }

    DBG("Connect to mail server [%s] success!\n", mus.svrAddr);

    if(handAuthlogin(&sh) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_USER_PASSWD; goto cleanUp;
    }

    if(handUserName(&sh, mus.user) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_USER_PASSWD; goto cleanUp;
    }

    if(handPassword(&sh, mus.passwd) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_USER_PASSWD; goto cleanUp;
    }

    if(handMailFrom(&sh, mus.user) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_USER_PASSWD; goto cleanUp;
    }

    if(handMailTo(&sh, mus.dstUser) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_USER_PASSWD; goto cleanUp;
    }

    if(handDataFlag(&sh) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_USER_PASSWD; goto cleanUp;
    }

    mailGenerateDate(szDate, sizeof(szDate));

    bzero(sendBuf, sizeof(sendBuf));
    p = sendBuf;
    //p = accessory;

    // make email head
    p += sprintf(p, "To: %s\r\n", mus.dstUser);
    p += sprintf(p, "From: %s\r\n", mus.user);
    p += sprintf(p, "Date: %s\r\n", szDate);
    p += sprintf(p, "Subject: %s\r\n", mus.alarmType);
    p += sprintf(p, "Content-Type: multipart/mixed; boundary=_----=_AAAAA0123456789\r\n");
    p += sprintf(p, "\r\n--_----=_AAAAA0123456789\r\n");
    p += sprintf(p, "Content-Type: text/plain; charset=UTF-8\r\n");
    p += sprintf(p, "\r\nLocal Time: %d.%02d.%02d-%02d:%02d:%02d\r\n%s\r\n",
                    tml.tm_year + 1900,
                    tml.tm_mon + 1,
                    tml.tm_mday,
                    tml.tm_hour,
                    tml.tm_min,
                    tml.tm_sec,
                    mus.alarmType);
    if(filePath == NULL && buf == NULL && filename == NULL){
        if(strncmp(mus.svrAddr, "smtp-mail.outlook.com", strlen(mus.svrAddr)) == 0
            || strncmp(mus.svrAddr, "smtp.live.com", strlen(mus.svrAddr)) == 0
            || strstr(mus.dstUser, "@hotmail.com") != NULL) {
            p += sprintf(p, ".\r\n");
        } else {
            p += sprintf(p, "\r\n\r\n--_----=_AAAAA0123456789\r\n.\r\n");
        }
    }

    //DBG("\n\n\n\n __ mid Big array %d \n\n\n\n", strlen(sendBuf));

    if(emailSend(&sh, sendBuf, strlen(sendBuf)) < 0) {
        lineno = __LINE__; ret = MAIL_FAIL_ANTI_SPAM; goto cleanUp;
    }

    // make email attatch file head
    if(filePath == NULL && buf == NULL && filename == NULL){
        ;
    } else {
        char *accessory = (char *)calloc(1, SMTPBUFLEN);
        if (NULL == accessory) {
            DBG("calloc accessory failed!\n");
            lineno = __LINE__; ret = MAIL_FAIL_ANTI_SPAM; goto cleanUp;
        }
        p = accessory;
        p += sprintf(p, "\r\n--_----=_AAAAA0123456789\r\n");
        p += sprintf(p, "Content-Transfer-Encoding: base64\r\n");
        p += sprintf(p, "Content-Type: application/x-msdownload; name=%s\r\n\r\n", filename);

        if (NULL == buf) {
            ret = mailFullySendFile(&sh, filePath, accessory, strlen(accessory));  //file
        } else {
            ret = mailFullySendBuf(&sh, buf, size, accessory, strlen(accessory));
        }

        //DBG("\n\n\n\n __ Big array %d \n\n\n\n", strlen(accessory));
        free(accessory);

        if(ret < 0) {
            DBG("Send mail content error!\n");
            lineno = __LINE__; ret = MAIL_FAIL_ANTI_SPAM; goto cleanUp;
        }
    }

    bzero(recvBuf, sizeof(recvBuf));
    if(emailRecv(&sh, recvBuf, sizeof(recvBuf)) <= 0 || (0 != strncmp(recvBuf, "250", 3))) {
        DBG("Didn't recv mail ACK cmd!\n");
        DBG("Recv : \n%s\n", recvBuf);
        lineno = __LINE__; ret = MAIL_FAIL_ANTI_SPAM; goto cleanUp;
    }
    DBG("Recv : \n%s\n", recvBuf);
    DBG("Mail succ, filepath:%s\n", filePath);

    emailReclaim(&sh);

    return 0;

cleanUp:

    DBG("Mail error@|%d|, retval: %d\n", lineno, ret);
    emailReclaim(&sh);

    return ret;
}

int createSocket()
{
    int fd = -1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        DBG("Socket create failed!:%s\n", strerror(errno));
    }

    return fd;
}

int closeSocket(int fd)
{
    int ret = -1;

    do
    {
        if(fd > 0)
        {
            ret = close(fd);
        }

        if(errno == EINTR || errno == EAGAIN)
        {
            perror("CloseSocket error :");
            continue;
        }

        break;
    }
    while(1);

    return ret;
}

int setSocketNonBlock(int fd)
{
    int curFlags = fcntl(fd, F_GETFL, 0);

    return fcntl(fd, F_SETFL, curFlags|O_NONBLOCK) >= 0;
}

int mailSSLConnect(SockeHandleS *sh)
{
    int ret = 0;
    int times = 0;

    do {
        ret = SSL_connect(sh->ssl);
        if (ret > 0) {
            DBG("SSL connect success!\n");
            break;
        } else if (errno == EAGAIN || errno == EINTR) {
            DBG("SSL reconnecting...\n");
            usleep(100 * 1000);
            continue;
        }
        break;
    } while(times++ < 10);

    return ret;
}

static int email_www_reachable(void)
{
    int i = 0;

    static const char *list[] = {
        "smtp.qq.com"           ,
        "smtp.exmail.qq.com"    ,
    };

    for(i=0; i < ARRAY_SIZE(list); i++){
        if(is_alive_name(list[i]) == FALSE) {
            return FALSE;
        }
    }

    return TRUE;
}

int connectSock(int fd, char *remoteAddr, unsigned short remotePort, int sec)
{
    struct sockaddr_in servaddr;
    struct hostent *hostinfo;

    int ret;

    if(email_www_reachable() == FALSE) {
        WAR("netwoke error exit\n");
        return -1;
    }

    hostinfo = gethostbyname(remoteAddr);
    if(!hostinfo)
    {
        DBG("no host:%s , error:%s\n", remoteAddr, strerror(errno));
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr = *((struct in_addr *)*hostinfo->h_addr_list);
    servaddr.sin_port = htons(remotePort);

    DBG("svr ip : %s,remotePort : %d\n", j_inet_ntoa(servaddr.sin_addr),remotePort);

    if(connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == 0)
    {
        DBG("Connect success!\n");
        return 0;
    }

    if(errno != EINPROGRESS)
    {
        DBG("connect failed!\n");
        return -1;
    }

    struct timeval tv;
    fd_set writefds;

    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    ret = -1;

    do
    {
        ret = select(fd + 1, NULL, &writefds, NULL, &tv);

        if(ret > 0)
        {
            socklen_t len = sizeof(int);
            ret = -1;

             //下面的一句一定要，主要针对防火墙
             getsockopt(fd, SOL_SOCKET, SO_ERROR, &ret, &len);
            if(ret == 0)
            {
                DBG("Connect success!\n");
                break;
            }
            else
            {
                DBG("Connect failed!\n");
                return -1;
            }
        }

        if(ret == 0)
        {
            DBG("Connect timeout....\n");
            return -1;
        }
        else if(ret < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
            {
                DBG("Connect try again\n");
                continue;
            }
            else
            {
                DBG("Connect failed!\n");
                return -1;
            }
        }

        break;
    } while(1);

    return 0;
}

int sendData(int fd, const char *in, const int len, int timeout)
{
    int ret = 0;
    int sendLen = 0;

    while(sendLen < len)
    {
        struct timeval tv;
        fd_set write;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        FD_ZERO(&write);
        FD_SET(fd, &write);

        ret = select(fd + 1, NULL, &write, NULL, &tv);
        if(ret == 0)
        {
            DBG("write fd timeout...\n");
            return -1;
        }
        else if(ret < 0)
        {
            if(errno != EINTR || errno != EAGAIN)
            {
                DBG("write socket [%d] error!\n", fd);
                break;
            }
            else
            {
                continue;
            }
        }

        ret = send(fd, in + sendLen, len - sendLen, 0);
        if(ret != -1)
        {
            sendLen += ret;
            continue;
        }

        if(errno != EINTR || errno != EAGAIN)
        {
            DBG("Send() socket %d error \n", fd);
            perror("error :");
            return -1;
        }
    }

    //DBG("Send data : \n%s\n", in);

    return sendLen;
}

int recvData(int fd, char *dataBuf, size_t size, int sec)
{
    int ret = 0;

    do
    {
        struct timeval tv;
        fd_set read;
        tv.tv_sec = sec;
        tv.tv_usec = 0;

        FD_ZERO(&read);
        FD_SET(fd, &read);

        ret = select(fd + 1, &read, NULL, NULL, &tv);
        if(ret == 0)
        {
            DBG("Recv timeout...\n");
            break;
        }
        else if(ret < 0)
        {
            if(errno != EINTR || errno != EAGAIN)
            {
                DBG("Read socket [%d] error!\n", fd);
                break;
            }
            else
            {
                continue;
            }
        }

        ret = recv(fd, dataBuf, size, 0);
        if(ret == -1 && (errno == EINTR || errno == EAGAIN))
        {
            DBG("Recv again...\n");
            continue;
        }

        break;
    }
    while(1);

    return ret;
}

int sslSend(SockeHandleS* sh, const char* in, const int len)
{
    int ret = 0;
    int sendLen = 0;

    while(sendLen < len)
    {
        struct timeval tv;
        fd_set write;
        tv.tv_sec = sh->timeout;
        tv.tv_usec = 0;

        FD_ZERO(&write);
        FD_SET(sh->fd, &write);

        ret = select(sh->fd + 1, NULL, &write, NULL, &tv);
        if(ret == 0)
        {
            DBG("write fd timeout...\n");
            return -1;
        }
        else if(ret < 0)
        {
            if(errno != EINTR || errno != EAGAIN)
            {
                DBG("write socket [%d] error!\n", sh->fd);
                break;
            }
            else
            {
                continue;
            }
        }

        ret = SSL_write(sh->ssl, in + sendLen, len - sendLen);
        if(ret > 0)
        {
            sendLen += ret;
            continue;
        }

        if(ret == 0)
        {
            sendLen = ret;
            break;
        }

        if(SSL_get_error(sh->ssl, ret) == SSL_ERROR_SYSCALL || errno == EAGAIN
                || errno == EINTR)
        {
            DBG("SSL_write() socket error try again..[%d]\n", errno);
            continue;
        }
        else
        {
            DBG("SSL_write error! [%d]\n", errno);
            return ret;
        }
    }

    //DBG("SSL Send data : \n%s\n", in);

    return sendLen;
}

int sslRecv(SockeHandleS* sh, char *dataBuf, int size, int sec)
{
    int ret = 0;

    do
    {
        struct timeval tv;
        fd_set read;
        tv.tv_sec = sec;
        tv.tv_usec = 0;

        FD_ZERO(&read);
        FD_SET(sh->fd, &read);

        ret = select(sh->fd + 1, &read, NULL, NULL, &tv);
        if(ret == 0)
        {
            DBG("Recv timeout...\n");
            break;
        }
        else if(ret < 0)
        {
            if(errno != EINTR || errno != EAGAIN)
            {
                DBG("Read socket [%d] error!\n", sh->fd);
                break;
            }
            else
            {
                continue;
            }
        }

        ret = SSL_read(sh->ssl, dataBuf, size);  //fully read ?
        if(ret == -1 && (errno == EAGAIN || (SSL_get_error(sh->ssl, ret) == SSL_ERROR_SYSCALL) || (SSL_get_error(sh->ssl, ret) == SSL_ERROR_WANT_READ)))
        {
            DBG("SSL Recv again...[%d]\n", errno);
            continue;
        }

        break;
    }
    while(1);

    return ret;
}

int emailSend(SockeHandleS* sh, const char *in, const int len)
{
    int ret = 0;

    sh->ssl != NULL ? ret = sslSend(sh, in, len) : ret = sendData(sh->fd, in, len, sh->timeout);

    return ret;
}

int emailRecv(SockeHandleS* sh, char *dataBuf, int size)
{
    int ret = 0;

    sh->ssl != NULL ? ret = sslRecv(sh, dataBuf, size, sh->timeout) :
        ret = recvData(sh->fd, dataBuf, size, sh->timeout);

    return ret;
}

int mailShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL)
    {
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        OPENSSL_free(line);

        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        OPENSSL_free(line);

        X509_free(cert);
    }
    else
    {
        DBG("no certificate info:\n");
        return -1;
    }

    return 0;
}

int sslCreateNew(SockeHandleS *sh)
{
    sh->ctx = SSL_CTX_new(SSLv23_client_method());
    if(sh->ctx == NULL)
    {
        DBG("SSL_CTX_new error\n");
        ERR_print_errors_fp(stdout);
        return -1;
    }

    sh->ssl = SSL_new(sh->ctx);
    if(NULL == sh->ssl)
    {
        DBG("SSL_new failed!\n");
        return -1;
    }

    if(SSL_set_fd(sh->ssl, sh->fd) != 1)
    {
        DBG("SSL_set_fd failed!\n");
        return -1;
    }

    return 0;
}

int tlsCreateNew(SockeHandleS *sh, char *server_ip)
{
    char recvBuf[512] = {0};
    sh->ctx = SSL_CTX_new(SSLv23_client_method());
    if (sh->ctx == NULL) {
        DBG("SSL_CTX_new error\n");
        ERR_print_errors_fp(stdout);
        return -1;
    }
    //服务器欢迎信息
    if(emailRecv(sh, recvBuf, sizeof(recvBuf)-1) <= 0) {
        return -1;
    }
    DBG("Welcome info : \n%s\n", recvBuf);

    //SMTP协议的握手信号
    if(handShakeSignal(sh, server_ip) < 0) {
        return -1;
    }

    if (handStarttls(sh) < 0) {
        return -1;
    }

    sh->ssl = SSL_new(sh->ctx);
    if(NULL == sh->ssl) {
        DBG("SSL_new failed!\n");
        return -1;
    }

    if(SSL_set_fd(sh->ssl, sh->fd) != 1) {
        DBG("SSL_set_fd failed!\n");
        return -1;
    }

    return 0;
}

void emailReclaim(SockeHandleS* sh)
{
    if(sh->ssl)
    {
        SSL_shutdown(sh->ssl);
        SSL_free(sh->ssl);
        SSL_CTX_free(sh->ctx);
    }

    closeSocket(sh->fd);
}

int mailCmdSendRecv(SockeHandleS* sh, char *in, int len, char *out, int size)
{
    DBG("Send : \n%s\n", in);
    if(emailSend(sh, in, len) <= 0)
    {
        return -1;
    }

    if(emailRecv(sh, out, size) <= 0)
    {
        return -1;
    }

    DBG("Recv : \n%s\n", out);

    return 0;
}

int handShakeSignal(SockeHandleS* sh, char *server_ip)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    sprintf(sendBuf, "HELO %s\r\n", server_ip);

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    //DBG("\n\n\n\n __ Big array %d \n\n\n\n", strlen(recvBuf));

    if(strncmp(recvBuf, "250",3) != 0)
    {
        DBG("Handshake failed!\n");
        return -1;
    }

    return 0 ;
}

int handStarttls(SockeHandleS* sh)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    sprintf(sendBuf, "%s", "STARTTLS\r\n");

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    if(strncmp(recvBuf, "220",3) != 0)
    {
        DBG("Handshake failed!\n");
        return -1;
    }

    return 0;
}

int handAuthlogin(SockeHandleS *sh)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    sprintf(sendBuf, "%s", "auth login\r\n");

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    if(strncmp(recvBuf, "334", 3) != 0)
    {
        DBG("Auth login failed!\n");
        return -1;
    }

    return 0;
}

int handUserName(SockeHandleS *sh, char *username)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    bzero(sendBuf, sizeof(sendBuf));
    char *base64 = recbase64Encode(username, strlen(username));
    sprintf(sendBuf, "%s\r\n", base64);
    delete[] base64;

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    if(strncmp(recvBuf, "334", 3) != 0)
    {
        DBG("Username commit failed!\n");
        return -1;
    }

    return 0;
}

int handPassword(SockeHandleS *sh, char *passwd)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    bzero(sendBuf, sizeof(sendBuf));
    char *base64 = recbase64Encode(passwd, strlen(passwd));
    sprintf(sendBuf, "%s\r\n", base64);
    delete[] base64;

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }  //返回235表示成功

    if(strncmp(recvBuf, "235", 3) != 0)
    {
        DBG("Passwd auth failed!\n");
        return -1;
    }

    return 0;
}

int handMailFrom(SockeHandleS *sh, char* username)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    sprintf(sendBuf, "MAIL FROM:<%s>\r\n", username);

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    if(strncmp(recvBuf, "250", 3) != 0)
    {
        DBG("Mailform commit failed!\n");
        return -1;
    }

    return 0;
}

int handMailTo(SockeHandleS *sh, char* dstUser)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    sprintf(sendBuf, "RCPT TO:<%s>\r\n", dstUser);

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    if(strncmp(recvBuf, "250", 3) != 0)
    {
        DBG("MailTo commit failed!\n");
        return -1;
    }

    return 0;
}

int handDataFlag(SockeHandleS *sh)
{
    char sendBuf[SENDSIZE] = {0};
    char recvBuf[RECVSIZE] = {0};

    sprintf(sendBuf, "DATA\r\n");

    if(mailCmdSendRecv(sh, sendBuf, strlen(sendBuf), recvBuf, sizeof(recvBuf)) < 0)
    {
        return -1;
    }

    if(strncmp(recvBuf, "354", 3) != 0)
    {
        DBG("\"DATA\" commit  failed!\n");
        return -1;
    }

    return 0;
}

void mailGenerateDate(char *szDate, int len)
{
    struct tm tm;
    int dir;
    int minutes;
    time_t now;
    struct timezone tz;
    struct timeval tv;

    memset(szDate, 0, len);

    if (-1 == (now = time(NULL)))
    {
        return ;
    }

    gettimeofday(&tv, &tz);
    gmtime_r(&now, &tm);
    minutes = tz.tz_minuteswest / 60;

    dir = (minutes > 0) ? '+' : '-';
    if (0 > minutes)
    {
        minutes = -minutes;
    }

    sprintf (szDate,"%s, %d %s %d %02d:%02d:%02d %c%02d%02d",
              days[tm.tm_wday],
              tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900,
              tm.tm_hour, tm.tm_min, tm.tm_sec,
              dir, minutes / 60, minutes % 60);

    return ;
}

int mailFullySendFile(SockeHandleS *sh, char *path, char *buf, int len)
{
    const char *endFlag = "\r\n\r\n--_----=_AAAAA0123456789\r\n.\r\n";
    int ret = 0;
    char *p = buf + len;

    int fd = 0, readLen = 0;
    char readBuf[3 * 2048] = {0};  //

    fd = open(path, O_RDONLY);
    if(-1 == fd)
    {
        DBG("File [%s] open failed! :%s\n", path, strerror(errno));
        return -1;
    }

    do
    {
        readLen = MailReadFully(fd, readBuf, sizeof(readBuf));
        if(0 > readLen)
        {
            DBG("Read error!\n");
            close(fd);
            return -1;
        }
        else if(0 == readLen)  //file end
        {
            break;
        }

        char *base64 = recbase64Encode(readBuf, readLen);

        p += sprintf(p, "%s", base64);
        ret = emailSend(sh, buf, p - buf);
        if(ret <= 0)
        {
            DBG("Send error!\n");
            close(fd);
            delete[] base64;
            return -1;
        }

        p = buf;
        bzero(buf, SMTPBUFLEN);
        bzero(readBuf, sizeof(readBuf));
        delete[] base64;
    }
    while(0 < readLen);

    p += sprintf(p, "%s", endFlag);
    ret = emailSend(sh, buf, p - buf);

    if(0 < fd)
        close(fd);

    return ret;
}

int mailFullySendBuf(SockeHandleS *sh, char *flame, int size, char *buf, int len)
{
    const char *endFlag = "\r\n\r\n--_----=_AAAAA0123456789\r\n.\r\n";
    int ret = 0;
    char *p = buf + len;

    char *base64 = recbase64Encode(flame, size);
    char *pp = base64;

    if(strlen(base64) + len + strlen(endFlag) > SMTPBUFLEN)
    {
        int bufLeft = SMTPBUFLEN - len;

        do
        {
            pp += snprintf(p, bufLeft, "%s", pp);

            ret = emailSend(sh, buf, SMTPBUFLEN);
            if(ret <= 0)
            {
                DBG("Send error!\n");
                break;
            }

            bzero(buf, SMTPBUFLEN);
            p = buf;
            bufLeft = SMTPBUFLEN;
        }while((base64 + strlen(base64) - pp) + strlen(endFlag) > SMTPBUFLEN);

        if((base64 + strlen(base64) - pp) > 0)
            p += sprintf(p, "%s", pp);
        p += sprintf(p, "%s", endFlag);

        ret = emailSend(sh, buf, p - buf);
    }
    else
    {
        p += sprintf(p, "%s", base64);
        p += sprintf(p, "%s", endFlag);

        ret = emailSend(sh, buf, p - buf);
    }

    delete[] base64;

    return ret;
}

int MailReadFully(int fd, void *buf, int nbytes)
{
    int nread = 0;

    while(nread < nbytes)
    {
        int r;

        r = read(fd, (char*) buf + nread, nbytes - nread);
        if(0 > r)
        {
            if(errno == EINTR || errno == EAGAIN)
            {
                sleep(1);
                continue;
            }
            else
            {
                return r;
            }
        }
        else if(0 == r)
        {
            break;
        }

        nread += r;
    }

    return nread;
}

