/* 
 *       Filename:  g_sys.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2023年03月30日 14时42分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), hul 
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

#include "debug.h"
#include "utils.h"
#include "g_sys.h"

static sSys g_sys  = {0};
static sMod g_log  = {0};
static sMod g_run  = {0};
static sMod g_stat = {0};

// sys
void __set_g_sys(long offset, int val)
{
    *((int*)((void *)&g_sys+offset)) = val;
}

int __get_g_sys(long offset)
{
    return *((int*)((void *)&g_sys+offset));
}

// log
void __set_g_log(long offset, int val)
{
    *((int*)((void *)&g_log+offset)) = val;
}

int __get_g_log(long offset)
{
    return *((int*)((void *)&g_log+offset));
}

// stat
void __set_g_stat(long offset, int bit)
{
    *((int*)((void *)&g_stat+offset)) |= bit;
}

void __clr_g_stat(long offset, int bit)
{
    *((int*)((void *)&g_stat+offset)) &= ~bit;
}

int __get_g_stat(long offset, int bit)
{
    return (*((int*)((void *)&g_stat+offset)) & bit);
}

int __pop_g_stat(long offset, int bit)
{
    int tmp = (*((int*)((void *)&g_stat+offset)) & bit);
    *((int*)((void *)&g_stat+offset)) &= ~bit;
    return tmp;
}

// run
void __set_g_run(long offset, int bit)
{
    *((int*)((void *)&g_run+offset)) |= bit;
}

void __clr_g_run(long offset, int bit)
{
    *((int*)((void *)&g_run+offset)) &= ~bit;
}

int __get_g_run(long offset, int bit)
{
    return (*((int*)((void *)&g_run+offset)) & bit);
}

int __pop_g_run(long offset, int bit)
{
    int tmp = (*((int*)((void *)&g_run+offset)) & bit);
    *((int*)((void *)&g_run+offset)) &= ~bit;
    return tmp;
}

// load&dump
void load_g_sys(void *ptr)
{
    memcpy(ptr, &g_sys, sizeof(g_sys));
}

void dump_g_sys(void *ptr)
{
    memcpy(&g_sys, ptr, sizeof(g_sys));
}

void load_g_log(void *ptr)
{
    memcpy(ptr, &g_log, sizeof(g_log));
}

void dump_g_log(void *ptr)
{
    memcpy(&g_log, ptr, sizeof(g_log));
}

void load_g_stat(void *ptr)
{
    memcpy(ptr, &g_stat, sizeof(g_stat));
}

void dump_g_stat(void *ptr)
{
    memcpy(&g_stat, ptr, sizeof(g_stat));
}

void load_g_run(void *ptr)
{
    memcpy(ptr, &g_run, sizeof(g_run));
}

void dump_g_run(void *ptr)
{
    memcpy(&g_run, ptr, sizeof(g_run));
}

void init_g_sys(void)
{
    char buf[1024] = {0};

    if (LoadFile("/tmp/usb_dev", buf, sizeof(buf)-1) > 0) {
        if (buf[0] == '4') {
            set_g_sys(usb_4g);
        } else if (buf[0] == 'w') {
            set_g_sys(usb_wifi);
        } else if (buf[0] == 'a') {
            set_g_sys(usb_asix);
        }
    }

    memset(buf, 0, sizeof(buf));
    LoadFile("/proc/cpuinfo", buf, sizeof(buf)-1);
    
    if (strstr(buf, "Ingenic")) {
        set_g_sys(jz);
    } else if (strstr(buf, "FH8")) {
        set_g_sys(fh);
    } else if (strstr(buf, "Augentix")) {
        set_g_sys(df);
    } else if (strstr(buf, "HI35XX")) {
        set_g_sys(hs);
    } else {
        set_g_sys(ax);
    }

    memset(buf, 0, sizeof(buf));
    if (LoadFile("/ipc/etc/maxheight", buf, sizeof(buf)-1) > 0) {
        g_sys.maxheight = atoi(buf);
    }
}

