/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : dahuasocket.cpp
 * @Created Time : 2014-04-29
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "jsocket.h"
#include "debug.h"
#include "utils.h"

JSocket::JSocket()
    : fSockFd(-1), fSockType(0)
{
}

JSocket::JSocket(int fd)
    : fSockFd(fd), fSockType(0)
{
}

JSocket::~JSocket()
{
}

int JSocket::CreateSocket(int type)
{
    fSockType = type;
    
    fSockFd = socket(AF_INET, type, 0);
    if (fSockFd != -1 && errno != EINVAL)
        return fSockFd;

    fSockFd = socket(AF_INET, type, 0);
    if (fSockFd < 0)
        DBG("unable to create socket: \n");
    
    return fSockFd;
}

int JSocket::SetReuseSocket(int flag)  //befor 'bind' function
{
    if (setsockopt(fSockFd, SOL_SOCKET, SO_REUSEADDR, 
                            (const char*)&flag, sizeof flag) < 0)
    {
        ERR("setsockopt(SO_REUSEADDR) error \n");
        return -1;
    }
      
    return 0;
}

int JSocket::Bind(unsigned short port)
{
    struct sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if (bind(fSockFd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
         DBG("bind() error (port number: %hu):\n", port);
         return -1;
    }

    return 0;
}

int JSocket::Listen(int backlog)
{
    if (listen(fSockFd, backlog) != 0)
    {
        return -1;
    }
    
    return 0;
}

int JSocket::Aceept(struct sockaddr *addr, int* addrlen)
{
    int iSock = -1;
    int count = 0;
    
    do
    {
        if ((iSock = accept(fSockFd, addr, (socklen_t*)addrlen)) == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                perror("Aceept error:");
                count++;

                if(count >= 3)
                    break;
                
                continue;
            }
        }
        
        break;      
    }while(1);

    return iSock;
}

int JSocket::Connect(char *remoteAddr, unsigned short remotePort, int timeout)
{
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(remoteAddr); 
    servaddr.sin_port = htons(remotePort);

    DBG("svr ip : %s\n", j_inet_ntoa(servaddr.sin_addr));

    if(connect(fSockFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == 0)
    {
        DBG("Connect success!\n");
        return 0;
    }

    if(errno != EINPROGRESS)
    {
        ERR("connect failed!\n");
        return -1;
    }

    struct timeval tv;
    fd_set writefds;   

    FD_ZERO(&writefds);   
    FD_SET(fSockFd, &writefds);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    
    int ret = -1;

    do
    {
        ret = select(fSockFd + 1, NULL, &writefds, NULL, &tv);

        if(ret > 0)
        {
            socklen_t len = sizeof(int);
            ret = -1;
 
            getsockopt(fSockFd, SOL_SOCKET, SO_ERROR, &ret, &len);
            if(ret == 0)
            {
                DBG("Connect success!\n");
                break;
            }
            else
            {
                ERR("Connect failed!\n");
                return -1;
            }
        }
        
        if(ret == 0)
        {
            ERR("Connect timeout....\n");
            return -1;
        }
        else if(ret < 0)
        {
            if(errno == EINTR || errno == EAGAIN)   
            {
                ERR("Connect try again\n");
                continue;
            }
            else
            {
                ERR("Connect failed!\n");
                return -1;
            }
        }

        break;
    } while(1);

    return 0;
}

int JSocket::AddMemberShip(char *mutiIp)
{
    struct ip_mreq mreq;

    memset(&mreq, 0, sizeof(struct ip_mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(mutiIp);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);//((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;
    if(-1 == setsockopt(fSockFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))
    {
        ERR("setsockopt IP_ADD_MEMBERSHIP fail, errno=%d %s\n",  errno, strerror(errno));
        return -1;
    }   

    return 0;
}

int JSocket::DropMemberShip(char *mutiIp)
{
    struct ip_mreq mreq;

    memset(&mreq, 0, sizeof(struct ip_mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(mutiIp);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);//((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;
    if(-1 == setsockopt(fSockFd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)))
    {
        ERR("setsockopt IP_DROP_MEMBERSHIP fail, errno=%d %s\n",  errno, strerror(errno));
        return -1;
    }   

    return 0;
}

int JSocket::CloseSocket()
{
    int ret = 0;
    int count = 0;

    do
    {
        if(fSockFd > 0)
        {
            ret = close(fSockFd);
        }

        if(0 != ret) {
            if(errno == EINTR || errno == EAGAIN)
            {
                DBG("fSockFd : %d\n", fSockFd);
                perror("CloseSocket error :");
                count++;

                if(count >= 3)
                    break;          
                
                continue;
            }
        } else { 
            fSockFd = -1;
        }
        
        break;
    }
    while(1);

    return ret;
}

int JSocket::Send(const char *inData, const int len, int timeout)
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
        FD_SET(fSockFd, &write);    

        ret = select(fSockFd + 1, NULL, &write, NULL, &tv);
        if(ret == 0)
        {
            ERR("write [%d] timeout...\n", fSockFd);
            return -1;          
        }
        else if(ret < 0)
        {
            if(errno != EINTR || errno != EAGAIN)
            {
                ERR("write socket [%d] error!\n", fSockFd);
                break;
            }
            else
            {
                usleep(0);
                continue;
            }
        }
        
        ret = send(fSockFd, inData + sendLen, len - sendLen, 0);
        if(ret != -1)
        {
            sendLen += ret; 
            continue;
        }

        if(errno != EINTR || errno != EAGAIN)
        {
            ERR("Send() socket %d error \n", fSockFd);
            perror("error :");
            return -1;
        }

        usleep(0);
    }

    //DBG("Send data : \n%s\n", in);

    return sendLen;
}

int JSocket::Recv(char *DataBuf, size_t inBufLen)
{
    int iRecvDataLen = 0;
    int count = 0;
    
    do
    {
        iRecvDataLen = recv(fSockFd, DataBuf, inBufLen, 0);
        if(iRecvDataLen == -1)
        {
            if(errno == EINTR || errno == EAGAIN)
            {
                perror("Recv error:");
                count++;

                if(count >= 3)
                    break;
                
                continue;
            }
        }
        
        break;      
    }while(1);

    return iRecvDataLen;
}

int JSocket::Recv(char *dataBuf, size_t size, int timeout)
{
    int ret = 0;

    do
    {
        struct timeval tv;
        fd_set read;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        FD_ZERO(&read);
        FD_SET(fSockFd, &read);

        ret = select(fSockFd + 1, &read, NULL, NULL, &tv);
        if(ret == 0)
        {
            ERR("Recv timeout...\n");
            break;
        }
        else if(ret < 0)
        {
            if(errno != EINTR || errno != EAGAIN)
            {
                ERR("Read socket [%d] error!\n", fSockFd);
                break;
            }
            else
            {
                usleep(0);
                continue;
            }
        }

        ret = recv(fSockFd, dataBuf, size, 0);
        if(ret == -1 && (errno == EINTR || errno == EAGAIN))
        {
            ERR("Recv again...\n");
            continue;
        }

        break;
    }
    while(1);

    return ret;
}

int JSocket::UdpSend(const char *inData, const int inLength, struct sockaddr_in sockaddr)
{
    int ret = -1;
    int count=0;

    do
    {
        ret = sendto(fSockFd, inData, inLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        if(ret < 0) 
        {
            if(errno == EINTR || errno == EAGAIN) 
            {
                DBG("sendto() socket %d error!errno[%d]\n", fSockFd, errno);
                count++;
                if(count < 3)
                    continue;
            }
        }

        break;
    }
    while(1);

    return ret;
}

int JSocket::UdpSend(const char *inData, const int inLength, char *host, int port)
{
    int ret = -1;
    int count=0;

    struct sockaddr_in sockaddr;

    bzero(&sockaddr, sizeof(sockaddr));
    
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(host);
    sockaddr.sin_port = htons(port);

    do
    {
        ret = sendto(fSockFd, inData, inLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        if(ret < 0) 
        {
            if(errno == EINTR || errno == EAGAIN) 
            {
                DBG("sendto() socket %d error!errno[%d]\n", fSockFd, errno);
                count++;
                if(count < 3)
                    continue;
            }
        }

        break;
    }
    while(1);

    return ret;
}

int JSocket::UdpRecv(char *DataBuf, size_t inBufSize, struct sockaddr_in& sockaddr)
{
    int ret = -1;
    int count=0;

    int addrlen = sizeof(sockaddr);

    bzero(&sockaddr, sizeof(sockaddr));

    do 
    {
        ret = recvfrom(fSockFd, DataBuf, inBufSize, 0, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        if(ret < 0) 
        {
            if(errno == EINTR || errno == EAGAIN) 
            {
                DBG("recvfrom() socket %d error!errno[%d]\n", fSockFd, errno);
                count++;
                if(count < 3)
                    continue;
            }
        }

        break;
    }
    while(1);

    return ret;
}

int JSocket::GetPeerIp(char *ip, int size)
{
    struct sockaddr_in ClientAddr;
    socklen_t ClientAddrLen = sizeof(ClientAddr);
    
    int ret = getpeername(fSockFd, (struct sockaddr*)&ClientAddr, &ClientAddrLen);  
    if(ret != 0) {
        ERR("getpeername failed!\n");
        return -1;
    }

    snprintf(ip, size, "%s", j_inet_ntoa(ClientAddr.sin_addr));

    return 0;
}

int JSocket::SetSocketNoBlock()
{
    int curFlags = fcntl(fSockFd, F_GETFL, 0);
    
    return fcntl(fSockFd, F_SETFL, curFlags|O_NONBLOCK) >= 0;
}

int JSocket::SetSocketSendBufSize(int inNewSize)
{
    socklen_t sizeSize = sizeof(inNewSize);
    
    if(setsockopt(fSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&inNewSize, sizeSize) < 0)
    {
        ERR("SetSocketSendBufSize error\n");
        return -1;
    }
    
    return 0;
}

int JSocket::SetSocketRecvBufSize(int inNewSize)
{
    socklen_t sizeSize = sizeof(inNewSize);
    
    if(setsockopt(fSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&inNewSize, sizeSize) < 0)
    {
        ERR("SetSocketRcvBufSize error\n");
        return -1;
    }
    
    return 0;
}

unsigned int JSocket::GetSendBufferSize()
{
    unsigned curSize = 0;
    socklen_t sizeSize = sizeof(curSize);
    
    if (getsockopt(fSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&curSize, &sizeSize) < 0)
    {
        ERR("GetSendBufferSize() error\n");
        return 0;
    }

    return curSize;
}

unsigned int JSocket::GetRecvBufferSize()
{
    unsigned curSize = 0;
    socklen_t sizeSize = sizeof(curSize);
    
    if (getsockopt(fSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&curSize, &sizeSize) < 0)
    {
        ERR("GetRecvBufferSize() error\n");
        return 0;
    }

    return curSize;
}
