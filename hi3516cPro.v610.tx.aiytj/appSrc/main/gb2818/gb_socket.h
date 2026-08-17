/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_socket.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 套接字管理:
                    - 连接指定 ip 和端口 ，允许域名
                    - 支持 tcp 和 udp
                    - 套接字属性设置
                    - 数据的发送和接收
                    - 作为 TCP 服务端侦听
*/

#ifndef GB_SOCKET_H_
#define GB_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <unistd.h>

#include "gb_common.h"

#define PORT_MAX 65535  // port 是无符号短整形，最大 65535

enum NetworkProtocols  {
    GB_UNSUPPORTED = -1,  // 不支持的类型
    GB_TCP = SOCK_STREAM,
    GB_UDP = SOCK_DGRAM,
};

class GbSocket {
private:
    enum SocketStatus{
        SOCKET_UNINIT = 0,
        SOCKET_CREATE,
        SOCKET_BIND,
        SOCKET_CONNECT,
        SOCKET_LISTEN,
    };

public:

    GbSocket(const char *dst_ip = NULL, uint32_t dst_port = 0, NetworkProtocols protocol = GB_TCP);
    ~GbSocket();

    /*获取和设置参数*/
    int set_socket(int socket);
    int get_socket() const {
        return socket_;
    }
    int set_socket(GbSocket &socket);

    int set_dst_ip(const char *ip_address); // 可以设置域名
    const char *get_dst_ip() const {
        return dst_ip_.c_str();
    }

    int set_dst_port(uint32_t port);
    uint32_t get_dst_port() const {
        return dst_port_;
    }

    int set_network_protocols(NetworkProtocols protocol);
    NetworkProtocols get_network_protocols() const {
        return protocol_;
    }

    const char *GetNetProtocolsStr() const {
        return protocol_ == GB_TCP ? "TCP" : "UDP";
    }

    int CreateSocket();
    int CreateSocket(NetworkProtocols protocol);
    int SetReuseAddr(int flag);  // 可选 设置地址可重用
    int SetSocketNoBlock();    // 可选 设置非阻塞
    int SetSocketSendBufSize(int new_size); // 可选 设置发送 buff
    int SetSocketRecvBufSize(int new_size); // 可选 设置接收 buff
    unsigned int GetSendBufferSize();   // 获取发送 buff size
    unsigned int GetRecvBufferSize();   // 获取接受 buff size
    int Bind(uint32_t port, const char* local_ip = NULL); // 可选 绑定到指定端口 ip
    int ServerConnect(uint32_t timeout_s);
    int ServerConnect(const char* address, uint32_t port, uint32_t timeout_s);
    int ServerConnect(NetworkProtocols protocol, uint32_t local_port, const char* address, uint32_t port, uint32_t timeout_s);
    int Send(const char *buf, uint32_t data_size, uint32_t timeout_s = 0);
    int Recv(char *buf, uint32_t buf_size, uint32_t timeout_s = 0);
    void CloseSocket();

    NetworkProtocols QurySocketType(int socket);

    // 服务器使用
    int Listen(int backlog);
    int Aceept(GbSocket &client);

    // 状态参数查询
    int IsConnect() {
        if (socket_ != -1)
            return true;

        return false;
    }

private:
    // inet_addr 转换不了的就不是正常的 ip 地址，可能是域名或者错误的地址
    bool IpAddressValid(const char *address) {
        if (inet_addr(address) != INADDR_NONE)
            return true;

        return false;
    }

    int TcpConnect(uint32_t timeout_s);
    int TcpSend(const char *buf, uint32_t data_size);
    int TcpSend(const char *buf, uint32_t data_size, uint32_t timeout_s);
    int TcpRecv(char *buf, uint32_t data_size);
    int TcpRecv(char *buf, uint32_t data_size, uint32_t timeout_s);
    /*udp 相关接口*/
    int UdpSend(const char *buf, uint32_t buf_size);
    int UdpRecv(char *buf, uint32_t buf_size);
    int UdpServerRecv(char *buf, uint32_t buf_size);

private:
    std::string dst_ip_;     // 目标ip，对于客户端是服务器 ip，对于服务器是设备 ip
    uint32_t dst_port_;          // 目标服务器端口
    NetworkProtocols protocol_;  // 连接协议

    int socket_;           // 套接字
    SocketStatus socket_status_;
};

#endif