/* 
 *       Filename:  aging8h.c
 *    Description:  
 *        Version:  1.0
 *        Created:  12/30/2025 09:30:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xuyx (), 
 *   Organization:  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"
#include "aging8h.h"
#include "factory_db.h"
#include "system_ctrl.h"

#define AGING_MAX (8)

/* 0, uninitd, not aging8h yet
 * 4~8, aging8h-pass
 * 10~18, has already burned once
 **/
static int g_aging8h = 0xbadf00d; // 尽可能减少文件的读写

int get_aging8h(void)
{
    if (g_aging8h == 0xbadf00d) {
        g_aging8h = 0;
        LoadFile2(F_AGING8H, "%d", &g_aging8h);
    }
    return g_aging8h;
}

/*
 * 4G 设备开启射频条件:
 * 1. ID 校验成功
 * 2. 存在 p2p_id 文件
 * 3. 老化时长达到 8h 或者烧录过 ID
 * */
int get_aging8h_pass(void)
{
    return (system_get_security() || is_okey(F_P2P_TRIPLE) || (get_aging8h() >= AGING_MAX));
}

/*
 * 只在老化无 id 时，进行处理
 * ID 校验失败，g_aging8h 范围为 0~8
 * ID 校验成功过一次后，g_aging8h 变更为 10~18
 * */ 
void set_aging8h(void)
{
    int uptime = 0;
    int val = get_aging8h();
    int secu = system_get_security();

    if (secu) {
        g_aging8h = val%BURNED_SIGN + BURNED_SIGN;    // 10~18
        goto __exit; 
    }

    if (val >= BURNED_SIGN) {  // 曾经烧录过，不再更新
        goto __exit;
    }

    uptime = system_get_uptime();
    if (uptime >= (val+1)*60*60) {
        g_aging8h = val+1;
        if (g_aging8h > AGING_MAX) {
            g_aging8h = AGING_MAX;
        }
    } else {
        g_aging8h = val;
    }

__exit:
    if (val != g_aging8h) {
        DumpFile2(F_AGING8H, "%d", g_aging8h);
        SYSLOG("val %d aging8h %d, secu:%d\n", val, g_aging8h, secu);
           LOG("val %d aging8h %d, secu:%d\n", val, g_aging8h, secu);
    } else {
        DBG("same v aging8h %d, secu:%d\n", val, secu);
    }
}

