/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : dahuasocket.h
 * @Created Time : 2014-04-29
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _JCO_SOCKET_H_
#define _JCO_SOCKET_H_

class JSocket
{
public:
	JSocket();
	JSocket(int fd);
	~JSocket();

	int CreateSocket(int type);
	int SetReuseSocket(int flag);
	int Bind(unsigned short port);
	int Listen(int backlog);
	int Aceept(struct sockaddr* addr, int* addrlen);
	int Connect(char* remoteAddr, unsigned short remotePort, int timeout);
	int AddMemberShip(char *mutiIp);
	int DropMemberShip(char *mutiIp);
	int CloseSocket();
	int Send(const char* inData, const int len, int timeout);
	int Recv(char* DataBuf, size_t inBufLen);	
	int Recv(char *dataBuf, size_t size, int timeout);

	int UdpSend(const char * inData, const int inLength, struct sockaddr_in sockaddr);
	int UdpSend(const char* inData, const int inLength, char* host, int port);
	int UdpRecv(char* DataBuf, size_t inBufSize, struct sockaddr_in& sockaddr);

	int GetPeerIp(char *ip, int size);
	
	int SetSocketNoBlock();
	int SetSocketSendBufSize(int inNewSize);
	int SetSocketRecvBufSize(int inNewSize);
	
	unsigned int GetSendBufferSize(); 
	unsigned int GetRecvBufferSize();

	int GetSocketNum(){return fSockFd;};

	int GetSockType(){return fSockType;};

private:
	int   fSockFd;
	int   fSockType;
};

#endif

