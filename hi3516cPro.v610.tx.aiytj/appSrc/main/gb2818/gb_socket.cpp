/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_socket.cpp
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>

#include "gb_socket.h"

GbSocket::GbSocket(const char *dst_ip, uint32_t dst_port, NetworkProtocols protocol)
: dst_port_(dst_port), protocol_(protocol), socket_(-1), socket_status_(SOCKET_UNINIT)
{
    if (dst_ip != NULL) {
        dst_ip_ = dst_ip;
    }
}

GbSocket::~GbSocket()
{
    CloseSocket();
}

/*设置套接字，设置过一次后就不允许再设置了，调用 close 之后才能重新设置*/
int GbSocket::set_socket(int socket)
{
    if (socket_status_ < SOCKET_CREATE) {
        NetworkProtocols protocol = QurySocketType(socket);
        if (protocol != GB_UNSUPPORTED) {
            socket_ = socket;
            socket_status_ = SOCKET_CREATE;
            protocol_ = protocol;
            return SUCCESS;
        }

        GB_ERR("Unsupported socket type\n");
        return FAILURE;
    }

    GB_ERR("Socket is create, forbid set socket\n");
    return FAILURE;
}

int GbSocket::set_socket(GbSocket &socket)
{
    if (socket_status_ < SOCKET_CREATE) {

        dst_ip_ = socket.dst_ip_;
        dst_port_ = socket.dst_port_;
        protocol_ = socket.protocol_;
        socket_ = socket.socket_;
        socket_status_ = socket.socket_status_;
        return SUCCESS;
    }

    GB_ERR("Socket is create, forbid set socket\n");
    return FAILURE;
}

/*address 可以是域名，会自动解析*/
int GbSocket::set_dst_ip(const char *address) {
    if (address == NULL) {
        GB_ERR("address is NULL\n");
        return FAILURE;
    }

    if (socket_status_ >= SOCKET_CONNECT) {
        GB_ERR("socket is connect, forbid set dst ip\n");
        return FAILURE;
    }

    /*检测是iP 还是域名*/
    if (IpAddressValid(address)) {
        dst_ip_ = address;
        return SUCCESS;
    }

    // 非有效 ip 地址，可能是域名，尝试进行解析
    struct addrinfo hints = {0};
    struct addrinfo *res;
    int ret = 0;

    hints.ai_family = AF_INET; /* Allow IPv4 */
    hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
    hints.ai_protocol = 0; /* Any protocol */
    hints.ai_socktype = SOCK_STREAM;
    ret = getaddrinfo(address, NULL,&hints,&res);
    if (ret < 0) {
        GB_ERR("wrong address:%s\n", address);
        return FAILURE;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    char ipbuf[16] = {0};
    //int port = 0;

    inet_ntop(AF_INET, &addr->sin_addr, ipbuf, 16);
    dst_ip_ = ipbuf;

    GB_DBG("domain name:%s, ip address:%s\n", address, ipbuf);

    freeaddrinfo(res);

    return SUCCESS;
}

/*设置目标端口，套接字连接之后不允许再设置，调用 close 后解除限制*/
int GbSocket::set_dst_port(uint32_t port)
{
    if (socket_status_ < SOCKET_CONNECT) {
        if (port > PORT_MAX) {
            GB_ERR("port[%u] more than short max\n", port);
            return FAILURE;
        }

        dst_port_ = port;
        return SUCCESS;
    }

    GB_ERR("socket is connect, forbid set dst port\n");
    return FAILURE;
}

/*设置连接类型，套接字创建后部运行再设置，调用 close 后解除*/
int GbSocket::set_network_protocols(NetworkProtocols protocol)
{
    if (protocol != GB_UNSUPPORTED && socket_status_ < SOCKET_CREATE) {
        protocol_ = protocol;

        return SUCCESS;
    }

    GB_ERR("socket is create, forbid set network protocols\n");
    return FAILURE;
}

/**
 * 创建 socket. 需要先调用 set_network_protocols 设置连接类型，否则默认是 TCP
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::CreateSocket()
{
    if (socket_status_ > SOCKET_UNINIT) {
        GB_ERR("socket already create\n");
        return FAILURE;
    }

    socket_ = socket(AF_INET, protocol_, 0);
    if(socket_ < 0) {
        GB_ERR("Failed to create socket:%s\n", strerror(errno));
        return FAILURE;
    }

    socket_status_ = SOCKET_CREATE;
    GB_INFO("Socket created successfully:%d\n", socket_);
    return SUCCESS;
}

/**
 * 创建 socket. 集成设置连接类型
 *
 * @param[type] 连接类型 GB_TCP/GB_UDP
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::CreateSocket(NetworkProtocols protocol)
{
    if (set_network_protocols(protocol) != SUCCESS) {
        return FAILURE;
    }

    return CreateSocket();
}

/*设置地址可重用, 只能创建之后, 其它操作之前设置*/
int GbSocket::SetReuseAddr(int flag)
{
    /*
        flag 为 1 表示使能
                0 表示非使能
    */
    if (socket_status_ != SOCKET_CREATE) {
        GB_ERR("socket unitit or already bind\n");
        return FAILURE;
    }

    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&flag, sizeof flag) < 0) {
        GB_ERR("setsockopt(SO_REUSEADDR) error:%s\n", strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

/*设置非阻塞, 只能创建之后, 其它操作之前设置*/
int GbSocket::SetSocketNoBlock()
{
    if (socket_status_ != SOCKET_CREATE) {
        GB_ERR("socket unitit or already bind\n");
        return FAILURE;
    }

    int cur_flags = fcntl(socket_, F_GETFL, 0);
    if (fcntl(socket_, F_SETFL, cur_flags|O_NONBLOCK) < 0) {
        GB_ERR("Set socket no block error:%s\n", strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

/*设置发送 buff 大小, 只能创建之后, 其它操作之前设置*/
int GbSocket::SetSocketSendBufSize(int new_size)
{
    if (socket_status_ != SOCKET_CREATE) {
        GB_ERR("socket unitit or already bind\n");
        return FAILURE;
    }

    if(setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char*)&new_size, sizeof(new_size)) < 0) {
        GB_ERR("Set socket send buffer size error:%s\n", strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

/*设置接收 buff 大小, 只能创建之后, 其它操作之前设置*/
int GbSocket::SetSocketRecvBufSize(int new_size)
{
    if (socket_status_ != SOCKET_CREATE) {
        GB_ERR("socket unitit or already bind\n");
        return FAILURE;
    }

    if(setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (char*)&new_size, sizeof(new_size)) < 0) {
        GB_ERR("Set socket rcv buffer size error:%s\n", strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

/*获取发送 buff 大小, 获取失败返回 0*/
uint32_t GbSocket::GetSendBufferSize()
{
    if (socket_status_ < SOCKET_CREATE) {
        GB_ERR("socket don't create\n");
        return 0;
    }

    unsigned cur_size = 0;
    socklen_t data_size = sizeof(cur_size);

    if (getsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char*)&cur_size, &data_size) < 0) {
        ERR("Get send buffer size error:%s\n", strerror(errno));
        return 0;
    }

    return cur_size;
}

/*获取接收 buff 大小, 获取失败返回 0*/
uint32_t GbSocket::GetRecvBufferSize()
{
    if (socket_status_ < SOCKET_CREATE) {
        GB_ERR("socket don't create\n");
        return 0;
    }

    unsigned cur_size = 0;
    socklen_t data_size = sizeof(cur_size);

    if (getsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (char*)&cur_size, &data_size) < 0) {
        ERR("Get send buffer size() error:%s\n", strerror(errno));
        return 0;
    }

    return cur_size;
}

/**
 * socket 绑定到本地 ip 端口，如果 ip 和 端口有多个套接字使用, 需要设置地址可重用
 *
 * @param[port] 要绑定的本地端口
 * @param[local_ip] 要绑定的本地 IP 可以不给, 默认值 NULL 使用 INADDR_ANY 表示绑定到所有网络接口
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::Bind(uint32_t port, const char* local_ip)
{
    if (socket_status_ != SOCKET_CREATE) {
        GB_ERR("socket unitit or already bind\n");
        return FAILURE;
    }

    // 绑定的时候端口是允许给 0 的，系统会自行选一个空闲的端口
    if (port > PORT_MAX) {
        GB_ERR("port[%u] more than short max\n", port);
        return FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = local_ip ? inet_addr(local_ip) : INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(socket_, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
         GB_ERR("bind port[%u] error:%s\n", port, strerror(errno));
         return FAILURE;
    }

    socket_status_ = SOCKET_BIND;
    return SUCCESS;
}

/**
 * 如果套接字类型是 TCP, connect 会连接目标服务器和端口
 * 如果套接字类型是 UDP, connect 会检查是否有设置目标服务器
 * 这个函数调用之前需要先设置目标服务器地址和端口
 *
 * @param[timeout_s] 超时时间，只有当设置了套接字非阻塞才有用 单位 秒
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::ServerConnect(uint32_t timeout_s)
{
    if (socket_status_ < SOCKET_CREATE || socket_status_ > SOCKET_BIND) {
        GB_ERR("socket unitit or already connect\n");
        return FAILURE;
    }

    // 检查目标服务器地址和端口是否有设置
    if (!IpAddressValid(dst_ip_.c_str())) {
        GB_ERR("dst ip address error:%s\n", dst_ip_.c_str());
        return FAILURE;
    }

    // 目标服务器不会使用 0 端口，端口为 0 表示是默认值，没有设置端口
    if (dst_port_ == 0 || dst_port_ > PORT_MAX) {
        GB_ERR("dst port error:%d\n", dst_port_);
        return FAILURE;
    }

    GB_DBG("start %s connect\n", GetNetProtocolsStr());

    if (protocol_ == GB_UDP) {
        socket_status_ = SOCKET_CONNECT;
        return SUCCESS;
    }

    return TcpConnect(timeout_s);
}

/**
 * 如果套接字类型是 TCP, connect 会连接目标服务器和端口
 * 如果套接字类型是 UDP, connect 会检查是否有设置目标服务器
 *
 * @param[address] 目标服务器地址
 * @param[port] 目标服务器端口
 * @param[timeout_s] 超时时间，单位 秒
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::ServerConnect(const char* address, uint32_t port, uint32_t timeout_s)
{
    if (socket_status_ < SOCKET_CREATE || socket_status_ > SOCKET_BIND) {
        GB_ERR("socket unitit or already connect\n");
        return FAILURE;
    }

    if (set_dst_ip(address) != SUCCESS) {
        return FAILURE;
    }

    if (set_dst_port(port) != SUCCESS) {
        return FAILURE;
    }

    return ServerConnect(timeout_s);
}
/**
 * 连接服务器的一整套流程，包括套接字创建，绑定和连接
 * 默认包含设置套接字绑定和可重用，默认不包含套接字非阻塞，所以 timeout_s 基本没用
 *
 * @param[protocol] 协议类型 TCP/UDP
 * @param[local_port] 当地端口，绑定用的
 * @param[address] 目标服务器地址
 * @param[port] 目标服务器端口
 * @param[timeout_s] 超时时间，单位 秒
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::ServerConnect(NetworkProtocols protocol, uint32_t local_port, const char* address, uint32_t port, uint32_t timeout_s)
{
    if (socket_status_ != SOCKET_UNINIT) {
        GB_ERR("socket status error\n");
    }

    if (CreateSocket(protocol) != SUCCESS) {
        return FAILURE;
    }

    if (SetReuseAddr(1) != SUCCESS) {
        return FAILURE;
    }

    if (protocol == SOCK_STREAM) {
        SetSocketNoBlock();
    }

    if (Bind(local_port) != SUCCESS) {
        return FAILURE;
    }

    return ServerConnect(address, port, timeout_s);
}

/**
 * 发送数据，会自动根据协议不同调用不同的发送函数
 *
 * @param[buf] 要发送的数据 buff
 * @param[data_size] 要发送的数据大小
 * @param[timeout_s] 超时时间，单位 秒
 *
 * @return 成功返回发送的数据，失败返回 FAILURE
 */
int GbSocket::Send(const char *buf, uint32_t data_size, uint32_t timeout_s)
{
    int ret = 0;
    if (socket_status_ != SOCKET_CONNECT) {
        GB_ERR("send fail, socket not connect\n");
        return FAILURE;
    }

    if (protocol_ == GB_TCP) {
        if (timeout_s != 0)
            ret = TcpSend(buf, data_size, timeout_s);
        else
            ret = TcpSend(buf, data_size);
    } else if (protocol_ == GB_UDP) {
        ret = UdpSend(buf, data_size);
    } else {
        GB_ERR("unsupported protocol");
        ret = FAILURE;
    }

    if (ret < 0) {
        return FAILURE;
    }

    return ret;
}

/**
 * 接收数据，会自动根据协议不同调用不同的接收函数
 *
 * @param[buf] 存储接收数据的 buff
 * @param[data_size] buff 大小
 * @param[timeout_s] 超时时间，单位 秒
 *
 * @return 成功返回接收的数据大小，失败返回 FAILURE
 */
int GbSocket::Recv(char *buf, uint32_t buf_size, uint32_t timeout_s)
{
    int ret = 0;
    if (socket_status_ != SOCKET_CONNECT) {
        GB_ERR("send fail, socket not connect\n");
        return FAILURE;
    }

    if (protocol_ == GB_TCP) {
        if (timeout_s != 0)
            ret = TcpRecv(buf, buf_size, timeout_s);
        else
            ret = TcpRecv(buf, buf_size);
    } else if (protocol_ == GB_UDP) {
        ret = UdpRecv(buf, buf_size);
    } else {
        GB_ERR("unsupported protocol");
        ret = FAILURE;
    }

    if (ret < 0) {
        return FAILURE;
    }

    return ret;
}

/*关闭套接字，不会清理已经设置的参数*/
void GbSocket::CloseSocket()
{
    if (socket_ > 0) {
        GB_INFO("close socket[%d]\n", socket_);
        close(socket_);
        socket_ = -1;
    }

    // 状态恢复
    socket_status_ = SOCKET_UNINIT;
}

/**
 * 监听套接字，监听之前套接字必须绑定，切套接字类型必须是 TCP
 *
 * @param[backlog] 内核为指定套接字维护的已完成连接和未完成连接队列的最大长度
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::Listen(int backlog)
{
    if (socket_status_ != SOCKET_BIND) {
        GB_ERR("listen fail, socket not bind\n");
        return FAILURE;
    }

    if (protocol_ != GB_TCP) {
        GB_ERR("listen fail, socket not TCP\n");
        return FAILURE;
    }

    if (listen(socket_, backlog) != 0) {
        GB_ERR("listen error:%s\n", strerror(errno));
        return FAILURE;
    }

    socket_status_ = SOCKET_LISTEN;
    return SUCCESS;
}
/**
 * 接受连接，只有套接字处于侦听状态才能接收
 *
 * @param[client] socket 类，用来存放对端套接字
 *
 * @return 成功返回 SUCCESS，失败返回 FAILURE
 */
int GbSocket::Aceept(GbSocket &client)
{
    if (socket_status_ != SOCKET_LISTEN) {
        GB_ERR("accept fail, socket not listen\n");
        return FAILURE;
    }

    struct sockaddr addr = {0};
    int addr_len = sizeof(struct sockaddr);
    int client_sock = -1;
    int count = 0;

    do {
        if ((client_sock = accept(socket_, &addr, (socklen_t*)&addr_len)) == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                perror("Aceept error:");

                if(count++ >= 3)
                    break;

                continue;
            }
        }
        break;
    }while(1);

    // 接收成功处理
    if (client_sock >= 0) {
        // ipv4
        if (addr.sa_family == AF_INET) {
            sockaddr_in *client_addr = (sockaddr_in *)&addr;
            char ip_str[INET_ADDRSTRLEN]; // 用于存放IP地址的字符串

            inet_ntop(AF_INET, &(client_addr->sin_addr), ip_str, INET_ADDRSTRLEN); // 解析对端 IP

            client.set_dst_ip(ip_str);
            client.set_dst_port(ntohs(client_addr->sin_port));
            client.set_network_protocols(GB_TCP); // 只有 tcp 需要 accept
            client.set_socket(client_sock);
            client.socket_status_ = SOCKET_CONNECT; // 客户端 socket 创建成功就是连接状态

            GB_DBG("accept ip:[%s] port[%d] socket[%d]\n", client.get_dst_ip(), client.get_dst_port(), client.get_socket());
            return SUCCESS;
        } else {
            GB_ERR("unsupported protocol family\n");
            close(client_sock);
            client_sock = -1;
        }
    }

    GB_ERR("Aeccept error\n");
    return FAILURE;
}

/**
 * TCP 数据发送，不带超时时间，如果设置了非阻塞，最好不要调用这个函数，有可能会少发数据
 *
 * @param[buf] 发送数据 buff
 * @param[data_size] 发送数据大小
 *
 * @return 成功返回发送数据的长度，失败返回负数
 */
int GbSocket::TcpSend(const char *buf, uint32_t data_size)
{
    int ret = 0;
    int retries = 0;
    do {
        ret = send(socket_, buf, data_size, 0);
        if (ret == -1) {
            if (errno != EINTR && errno != EAGAIN ) {
                GB_ERR("socket %d ,send error:%s\n", socket_, strerror(errno));
                break;
            } else if (retries++ < 5) {
                // EAGAIN 表示套接字非阻塞，发送暂时无法完成，需要稍后再试
                // EINTR 表示系统调用被中断，需要重试
                // 这里默认重发五次，五次还是超时的话就退出
                usleep(0); // 让出 cpu 给其它线程
                continue;
            } else {
                GB_ERR("send error:%s\n", strerror(errno));
                break;
            }
        }

        break;
    }while(1);

    return ret;
}

/**
 * TCP 数据发送，带超时时间和 select 监听，发送被打断可以续发有两种情况，一个是套接字非阻塞，一个是系统繁忙
 * 系统繁忙出现的比较少，所以这个函数最好是设置了套接字非阻塞再调用
 *
 * @param[buf] 发送数据 buff
 * @param[data_size] 发送数据大小
 * @param[timeout_s] 超时时间，单位 秒
 *
 * @return 成功返回发送数据的长度，失败返回负数
 */
int GbSocket::TcpSend(const char *buf, uint32_t data_size, uint32_t timeout_s)
{
    int ret = 0;
    uint32_t send_len = 0;

    while(send_len < data_size) {
        struct timeval tv;
        fd_set write;
        tv.tv_sec = timeout_s;
        tv.tv_usec = 0;

        FD_ZERO(&write);
        FD_SET(socket_, &write);

        ret = select(socket_ + 1, NULL, &write, NULL, &tv);
        if(ret == 0) {
            GB_ERR("socket[%d] select write timeout...\n", socket_);
            send_len = -1;
            break;
        } else if(ret < 0) {
            if(errno != EINTR || errno != EAGAIN) {
                GB_ERR("socket [%d] write  error!\n", socket_);
                send_len = ret;
                break;
            } else {
                usleep(0); // 让出 cpu 给其它线程
                continue;
            }
        }

        ret = send(socket_, buf + send_len, data_size - send_len, 0);
        if(ret != -1) {
            send_len += ret;
            continue;
        }

        if(errno != EINTR || errno != EAGAIN) {
            GB_ERR("socket[%d] send error:%s\n", socket_, strerror(errno));
            send_len = -1;
            break;
        }

        usleep(0);
    }

    return send_len;
}
/*TCP接收数据成功返回接收数据的长度，失败返回负数*/
int GbSocket::TcpRecv(char *buf, uint32_t buf_size)
{
    int recv_len = 0;
    int count = 0;

    do {
        recv_len = recv(socket_, buf, buf_size, 0);
        if (recv_len == -1) {
            if(errno == EINTR) {
                count ++;
                if (count >= 3) {
                    break;
                }

                continue;
            }
        }

        break;
    }while(1);

    return recv_len;
}
/**
 * TCP 接收数据，带超时时间和 select 监听
 *
 * @param[buf] 接受数据 buff
 * @param[data_size] buff 大小
 * @param[timeout_s] 超时时间，单位 秒
 *
 * @return 成功返回接收数据的长度，失败返回负数
 */
int GbSocket::TcpRecv(char *buf, uint32_t buf_size, uint32_t timeout_s)
{
    int ret = 0;

    do {
        struct timeval tv;
        fd_set read;
        tv.tv_sec = timeout_s;
        tv.tv_usec = 0;

        FD_ZERO(&read);
        FD_SET(socket_, &read);

        ret = select(socket_ + 1, &read, NULL, NULL, &tv);
        if(ret == 0) {
            GB_ERR("socket[%d] select read timeout...\n", socket_);
            break;
        } else if(ret < 0) {
            if(errno != EINTR || errno != EAGAIN) {
                GB_ERR("socket [%d] read  error!\n", socket_);
                break;
            } else {
                usleep(0); // 让出 cpu 给其它线程
                continue;
            }
        }

        ret = recv(socket_, buf, buf_size, 0);
        if(ret == -1 && (errno == EINTR || errno == EAGAIN)) {
            GB_ERR("Recv again...\n");
            continue;
        }

        break;
    } while (1);

    return ret;
}

/*udp 发送数据，成功返回发送的数据, 失败返回 FAILURE*/
int GbSocket::UdpSend(const char *buf, uint32_t data_size)
{
    struct sockaddr_in address = {0};
    ssize_t send_bytes;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(get_dst_ip());
    address.sin_port = htons(get_dst_port());

    send_bytes = sendto(socket_, buf, data_size, 0, (struct sockaddr *)&address, sizeof(address));
    if(send_bytes < 0) {
        GB_ERR("sendto error:%s\n", strerror(errno));
        return FAILURE;
    }

    if ((uint32_t)send_bytes != data_size) {
        GB_ERR("socket:%d sendbytes:%d != len:%d,error:%s\n", socket_, send_bytes, data_size, strerror(errno));
    }

    return send_bytes;
}

/*udp 接收数据，成功返回接收数据大小，失败返回 FAILURE*/
int GbSocket::UdpRecv(char *buf, uint32_t buf_size)
{
    int recv_len;
    struct sockaddr_in address;
    int addr_len = sizeof(address);

    recv_len = recvfrom(socket_, buf, buf_size, 0, (struct sockaddr*)&address, (socklen_t *)&addr_len);
    if(recv_len < 0) {
        GB_DBG("socket:%d recvfrom error: %s\n", socket_, strerror(errno));
        return FAILURE;
    }
#if 0 // udp 一般不关心对端 ip, 这里对端 ip 校验屏蔽
    char ip_str[INET_ADDRSTRLEN]; // 用于存放IP地址的字符串
    // 对端 ip 校验判断
    if (!dst_ip_.empty()) {
        inet_ntop(AF_INET, &address.sin_addr, ip_str, 16);
        if (dst_ip_ != ip_str) {
            GB_ERR("dst ip address[%s] error, expect[%s]\n", ip_str, dst_ip_.c_str());
            return FAILURE;
        }
    }
    //  对端端口校验判断，
    if (dst_port_ > 0) {
        uint32_t port = ntohs(address.sin_port);
        if (dst_port_ != port) {
            GB_ERR("dst port[%u] error, expect[%u]\n", port, dst_port_);
            return FAILURE;
        }
    }
#endif

    return recv_len;
}

/*查询套接字类型，出错或者未知返回 GB_UNSUPPORTED, 其余返回套接字类型*/
NetworkProtocols GbSocket::QurySocketType(int socket)
{
    int optval;
    socklen_t optlen = sizeof(optval);

    // 获取套接字类型
    if (getsockopt(socket, SOL_SOCKET, SO_TYPE, &optval, &optlen) == -1) {
        GB_ERR("getsockopt failed\n");
        return GB_UNSUPPORTED;
    }

    NetworkProtocols protocol = GB_UNSUPPORTED;
    // 打印套接字类型
    switch (optval) {
        case SOCK_STREAM:
            protocol = GB_TCP;
            break;
        case SOCK_DGRAM:
            protocol = GB_UDP;
            break;
        default:
            GB_DBG("Unsupported type:%d\n", optval);
            break;
    }

    return protocol;
}
/*TCP 连接服务器，带超时和 select 监听，成功返回 SUCCESS 失败返回 FAILURE*/
int GbSocket::TcpConnect(uint32_t timeout_s)
{
    if (socket_status_ < SOCKET_CREATE || socket_status_ > SOCKET_BIND) {
        GB_ERR("socket unitit or already connect\n");
        return FAILURE;
    }

    if (protocol_ != GB_TCP) {
        GB_ERR("network protocol is error\n");
        return FAILURE;
    }

    // 连接服务器
    struct sockaddr_in dst_addr = {0};

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = inet_addr(get_dst_ip());
    dst_addr.sin_port = htons(dst_port_);

    if(connect(socket_, (struct sockaddr*)&dst_addr, sizeof(dst_addr)) == 0) {
        GB_DBG("Connect success!\n");
        socket_status_ = SOCKET_CONNECT;
        return SUCCESS;
    }

    /*
        EINPROGRESS 表示连接仍在进行中，设置非阻塞时会返回这个错误码，
        后续需要调用 select 监听，直到连接建立或者超时
    */
    if(errno != EINPROGRESS) {
        ERR("connect failed! dst ip:%s port:%d\n", get_dst_ip(), dst_port_);
        return FAILURE;
    }

    struct timeval tv;
    fd_set writefds;

    FD_ZERO(&writefds);
    FD_SET(socket_, &writefds);
    tv.tv_sec = timeout_s;
    tv.tv_usec = 0;

    int result = FAILURE;
    int ret = 0;
    do {
        ret = select(socket_ + 1, NULL, &writefds, NULL, &tv);

        if(ret > 0) {
            socklen_t len = sizeof(int);
            getsockopt(socket_, SOL_SOCKET, SO_ERROR, &ret, &len);
            if(ret == 0) {
                socket_status_ = SOCKET_CONNECT;
                result = SUCCESS;
                GB_DBG("Connect success!\n");
                break;
            } else {
                GB_ERR("Connect failed!\n");
                break;
            }
        }

        if(ret == 0) {
            GB_ERR("Connect timeout....\n");
            break;
        } else if(ret < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                GB_DBG("Connect try again\n");
                continue;
            } else {
                GB_ERR("Connect failed!\n");
                break;
            }
        } else {
            socklen_t len = sizeof(int);
            int err = -1;
            // SO_ERROR:获取与套接字相关的错误状态 主要针对设置套接字非阻塞的情况
            getsockopt(socket_, SOL_SOCKET, SO_ERROR, &err, &len);
            if(err == 0) {
                GB_DBG("connect sucess\n");
                socket_status_ = SOCKET_CONNECT;
                ret = SUCCESS;
                break;
            }
            GB_DBG("connect error because of firewall\n");
        }

        break;
    } while(1);

    return result;
}