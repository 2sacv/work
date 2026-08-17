/*
 *       Filename:  uart.h
 *    Description:  
 *        Version:  1.0
 *        Created:  03/02/2026 08:25:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _UART_H
#define _UART_H
#ifdef __cplusplus 
extern "C" {
#endif

int uart_fd_open(int *fd, const char *path_name, int baudrate);
int uart_fd_close(int *fd);

#ifdef __cplusplus
}
#endif
#endif
