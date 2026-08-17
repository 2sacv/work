/* 
 *       Filename:  uart.c
 *    Description:  
 *        Version:  1.0
 *        Created:  03/02/2026 08:25:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "debug.h"

typedef struct {
    int baudrate;
    int termio_num;
} sBRNum2TermMaps;

static sBRNum2TermMaps g_br2term_maps[] = {
    {0,       B0},
    {50,      B50},
    {75,      B75},
    {110,     B110},
    {134,     B134},
    {150,     B150},
    {200,     B200},
    {300,     B300},
    {600,     B600},
    {1200,    B1200},
    {1800,    B1800},
    {2400,    B2400},
    {4800,    B4800},
    {9600,    B9600},
    {19200,   B19200},
    {38400,   B38400},
    {57600,   B57600},
    {115200,  B115200},
    {230400,  B230400},
    {460800,  B460800},
    {500000,  B500000},
    {576000,  B576000},
    {921600,  B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {1500000, B1500000},
    {2000000, B2000000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000}
};

static int get_baudrate_term(int baudrate)
{
    size_t idx = 0;

    for (idx = 0; idx < ARRAY_SIZE(g_br2term_maps); idx++) {
        if (baudrate == g_br2term_maps[idx].baudrate) {
            COLOR_G("choose baudrate %d %d\n",
                    baudrate, g_br2term_maps[idx].termio_num);
            return g_br2term_maps[idx].termio_num;
        }
    }

    return FAILURE;
}

int uart_fd_open(int *fd, const char *path_name, int baudrate)
{
    return_val_if_fail(NULL != fd, FAILURE);
    return_val_if_fail(NULL != path_name, FAILURE);

	struct termios term = {0};
    int ret = SUCCESS;
    int baudrate_term = 0;

    baudrate_term = get_baudrate_term(baudrate);
    goto_exit_if_fail(baudrate_term > 0, exit, ret = FAILURE,
                      "failed to open fd baudrate %d\n", baudrate);

    if (*fd > 0) {
        ret = SUCCESS;
        WAR("fd has been inited\n");
        goto exit;
    }

    *fd = open(path_name, O_RDWR | O_NOCTTY | O_SYNC);
    if (*fd < 0) {
        SYSLOG("open uart failed, err:%s\n", strerror(errno));
        ret = FAILURE;
        goto exit;
    }

    ret = tcgetattr(*fd, &term);
    goto_tag_if_fail(SUCCESS == ret, exit);

    // 设置波特率
    cfsetispeed(&term, baudrate_term);
    cfsetospeed(&term, baudrate_term);
    
    // 8位数据位
    term.c_cflag &= ~CSIZE;
    term.c_cflag |= CS8;
    
    // 1位停止位
    term.c_cflag &= ~CSTOPB;
    
    // 无校验位
    term.c_cflag &= ~PARENB;
    
    // 启用接收，忽略调制解调器状态
    term.c_cflag |= (CLOCAL | CREAD);
    
    // 禁用硬件流控
    term.c_cflag &= ~CRTSCTS;
    
    // 原始输入模式
    term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // 禁用软件流控
    term.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    // 禁用输入处理
    term.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    
    // 原始输出模式
    term.c_oflag &= ~OPOST;
    
    // 设置读取行为
    term.c_cc[VMIN] = 0;      // 最少读取0字符
    term.c_cc[VTIME] = 1;     // 100ms 超时

    ret = tcsetattr(*fd, TCSANOW, &term);
    goto_tag_if_fail(SUCCESS == ret, exit);

    ret = tcflush(*fd, TCIOFLUSH); //打开串口前清除串口缓冲数据
    goto_tag_if_fail(SUCCESS == ret, exit);

    ret = SUCCESS;

exit:

    if (FAILURE == ret && *fd > 0) {
        close(*fd);
        *fd = -1;
    }

    return ret;
}

int uart_fd_close(int *fd)
{
    return_val_if_fail(NULL != fd, FAILURE);

    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }

    return SUCCESS;
}

