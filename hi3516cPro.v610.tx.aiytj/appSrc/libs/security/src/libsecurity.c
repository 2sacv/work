/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name   : libsecurity.c
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2015.04.24
    Description : 
    History     :
                    Create by tianjun 2015.04.24
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <memory.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/types.h>
#include "libsecurity.h"

#define MAC_ALL_COUNT                   4       
#define SECURITY_DEVICE_NAME            "/dev/w25r128fv"


#define LDBG(fmt, args...) \
    do{  \
        fprintf(stdout, "[-DBG-] [%s:%5d] --- " fmt,__FILE__,__LINE__,## args);  \
    }while(0)

#define LERR(fmt, args...) \
    do{ \
        fprintf(stderr, "[*ERR*] [%s:%5d] *** " fmt, __FILE__,__LINE__,## args);  \
    }while(0)

typedef enum security_option_flags
{
    CTRL_CHECK_VERSION = 9,
    CTRL_WAKE_UP = 10,
    CTRL_READ_CONFIG,
    CTRL_WRITE_FACTORY,
    CTRL_MAC_CHALLENGE_ALL,
    CTRL_MAC_CHALLENGE_SINGLE,
    CTRL_WRITE_PRODUCT_SERIAL,
    CTRL_READ_PRODUCT_SERIAL,
    CTRL_READ_CHIP_SERIAL,
    CTRL_READ_MANUFACTURE,
    CTRL_WRITE_MANUFACTURE,
    CTRL_PASS_WD,
} ESecurityOptionFlags;


typedef struct secutity_options
{
    ESecurityOptionFlags eOptionFlags;
    unsigned char UserData[128];
} TSecurityOptions;

int security_is_ok(void)
{
    int ret = 0;
    int fd = -1;
    unsigned char data[4]; 
    memset(data,0,sizeof(data));

    if (fd < 0)
    {
        fd = open(SECURITY_DEVICE_NAME, O_RDONLY);  
        if (fd < 0)
        {
            return -1;
        }
    }

    ret = ioctl(fd, CTRL_MAC_CHALLENGE_ALL,0);
    
    if (fd >=0)
    {
        close(fd);
        fd = -1;
    }
    
    return ret;
}


int security_get_id(unsigned char *pSerial)
{
    int ret = 0;
    int fd = -1;
    int retry = 0;
    unsigned char data[11];
    memset(data,0,sizeof(data));
   
    if (fd < 0)
    {
        fd = open(SECURITY_DEVICE_NAME, O_RDONLY);
        if (fd < 0)
        {
            return -1;
        }
    }

    for (retry=0;retry<4;retry++)
    {
        if (fd >= 0)
        {
            ret = ioctl(fd, CTRL_READ_PRODUCT_SERIAL,data);
        }
        if (0 == ret)
        {   
            break;
        }
    }
       
    if (fd >=0)
    {
        close(fd);
        fd = -1;
    }
    
    if (0 == ret)
    {
        if (pSerial)
        {
            memcpy(pSerial,data,sizeof(data));
        }
        
    }

    return ret;
}

int security_set_id(unsigned char *pSerial)
{
    int ret = 0;
    int fd = -1;
    int retry = 0;
    unsigned char data[11];
    if (pSerial)
    {
        memcpy(data,pSerial,sizeof(data));
    }
    else
    {
        return -1;
    }
   
    if (fd < 0)
    {
        fd = open(SECURITY_DEVICE_NAME, O_RDONLY);
        if (fd < 0)
        {
            return -1;
        }
    }

    for (retry=0;retry<4;retry++)
    {
        if (fd >= 0)
        {
            ret = ioctl(fd, CTRL_WRITE_PRODUCT_SERIAL,data);
        }
        if (0 == ret)
        {   
            break;
        }
    }
       
    if (fd >=0)
    {
        close(fd);
        fd = -1;
    }

    return ret;
}

int gd25q_get_id(unsigned char *pID)
{
    int ret = 0;
    int fd = -1;
    int i = 0;
    int retry = 0;
    unsigned char unique[24] = {0}; 

    for (retry=0;retry<4;retry++)
    {
        if (-1 != (fd = open("/dev/gd25q", O_RDONLY)))
        {
            break;
        }
        usleep(1000 * 200);
    }
    if (-1 == fd)
    {
        return -1;
    }

    for (retry=0;retry<4;retry++)
    {
        if (0 == (ret = ioctl(fd,0,unique)))
        {
            /*
            printf("UNIQUE-ID:");
            for (i=0;i<24;i++)
            {
                printf("%02X",unique[i]);
            }
            printf("\n");
            */

            for (i=0;i<4;i++)
            {
                unique[i] = 0x00;               
            }
            for (i=12;i<24;i++)
            {
                unique[i] = 0x00;               
            }

            if (pID)
            {   
                memcpy(pID,unique,sizeof(unique));
            }
            break;
        }
        usleep(1000 * 200);
    }
       
    if (-1 != fd)
    {
        close(fd);
        fd = -1;
    }
    
    return ret;
}

