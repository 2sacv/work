/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : encode_pwm.h
 * Created Time : 2017-12-28
 * Version      : 1.0
 * Author       : tianjun
 * Description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <dirent.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <linux/spi/spidev.h>
#include "ms41929.h"
#include "motor.h"



int motor_init(void)
{
    int ret = 0;
    ret = ms41929_init();
    return ret;
}

int motor_uninit(void)
{
    int ret = 0;
    ms41929_uninit();
    return ret;
}


