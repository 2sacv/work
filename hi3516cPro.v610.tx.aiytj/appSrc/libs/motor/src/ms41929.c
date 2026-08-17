/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : pwm.h
 * Created Time : 2017-12-28
 * Version      : 1.0
 * Author       : 田军
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
#include "ms41929.h"

#define GPIO_MT_RST                     (32*1+13) 
#define GPIO_MT_VDFZ                    (32*1+11) 
#define READ_REG_ID					    0
#define WRITE_REG_ID					1
#define MS41929_DEVICE_NAME				"/dev/ms41929"

typedef struct {
    unsigned char   addr;               
    unsigned short  value;          
    unsigned int    usleep;             
} _reg_array;

static int fd_ms41929 = -1;

static void usecond_nanosleep(long usecond)
{

}

int ms41929_get_focus_state(void)
{
    int ret = 0;
    //ret = !gpio_intr_no_wait(AN41908_MT_PLS2_GPIO);
    return ret;
}

int ms41929_get_zoom_state(void)
{
    int ret = 0;
    //ret = !gpio_intr_no_wait(AN41908_MT_PLS1_GPIO);
    return ret;
}

int ms41929_clear_focus_isr(void)
{
    int ret = 0;
    //ret = gpio_intr_clera_isr(AN41908_MT_PLS2_GPIO);
    return ret;
}

int ms41929_clear_zoom_isr(void)
{
    int ret = 0;
    //ret = gpio_intr_clera_isr(AN41908_MT_PLS1_GPIO);
    return ret;
}

int ms41929_vdfz(void) 
{
    int ret = 0;
	gpio_set_value(GPIO_MT_VDFZ, 1);
	//usecond_nanosleep(40);
	gpio_set_value(GPIO_MT_VDFZ, 0);
    return ret;
}

int ms41929_write_reg(unsigned char addr,unsigned short value)
{
    int ret = 0;
    _reg_array param = {0};

    if (fd_ms41929 >= 0){
        param.addr = addr;
        param.value = value;
        ret = ioctl(fd_ms41929,WRITE_REG_ID,&param);
    }
    return ret;
}

int  ms41929_read_reg(unsigned char addr,unsigned short *value)
{
    int ret = 0;
    _reg_array param = {0};

    if (!value )      {
        printf("param is error please check code \n");
        return -1;
    }

    if (fd_ms41929 >= 0){
        param.addr = addr;
        param.value = value;
        ret = ioctl(fd_ms41929,READ_REG_ID,&param);
        if (0 == ret) {
            *value = param.value;
		}
    }
    return ret;
}

static int m41929_dump_reg(void)
{
    int ret = 0;
	int i;
	int len = 0;
	uint16_t value = 0;
	uint8_t addr = 0x0b;

	ms41929_read_reg(addr, &value);
	printf("\033[1;31m""ms41929_reg[%#x]=>%#x\n""\033[0000m", addr, value);
   	for(i = 0x20; i < 0x2B; i++ ) {
 		addr = i;
		if (addr == 0x26)
			continue;
		ms41929_read_reg(addr, &value);
		usecond_nanosleep(1000);
		printf("\033[1;31m""ms41929_reg[%2x]=>%4x\n", addr, value);
	}
	printf("\n""\033[0000m");
    
    return ret;
}


static void ms41929_reg_init(void)
{
	uint16_t val = 0;
	
	val = 0x0000;
	ms41929_write_reg(0x0b, val);	 //VD_FZ 极性选择.  MODEL_FZ=0x0000 ，极性基于 VD_FZ 的上升沿。MODEL_FZ=0x0100，极性基于 VD_FZ 的下降沿。 
	usecond_nanosleep(10000);

	val = 0x1E03;
	ms41929_write_reg(0x20, val);	 // PWMMODE[4:0]=30	   PWMRES[1:0]=0; DT1[7:0] =3;
	usecond_nanosleep(1000);
	
	//垂直电机
	val = 0x0002;
	ms41929_write_reg(0x22, val);	 //相位校正0度， DT2A[7:0]=2 ; DT2A延时= 2*303.4us 
	usecond_nanosleep(1000);
	val = 0xd8d8;
	ms41929_write_reg(0x23, val);	 //设置AB占空比为:  [PPWA[7:0]/( PWMMODE[4:0]*8)=0.9]	    0xd8d8 = 90% ;	0xf0f0=100%
	usecond_nanosleep(1000);
	val = 0x0900;
	ms41929_write_reg(0x24, val);	 //AB 256细分	设定电流方向反，计数0 
	usecond_nanosleep(1000);
	val = 0x03ff;
	ms41929_write_reg(0x25, val);  	//设置STEP为3.64ms  0x07ff
	usecond_nanosleep(1000);
	
	//水平电机
	val = 0x0002;
	ms41929_write_reg(0x27, val);	 //相位校正0度， DT2B[7:0]=2 ; DT2B延时= 2*303.4us 
	usecond_nanosleep(1000);

	val = 0xd8d8;
	ms41929_write_reg(0x28, val);	 //设置CD占空比为 :  [PPWC[7:0]/( PWMMODE[4:0]*8)=0.9]	    0xd8d8 = 90% ;	0xf0f0=100%
	usecond_nanosleep(1000);

	val = 0x0900;
	ms41929_write_reg(0x29, val);	 //CD 256细分	设定电流方向反，计数0 
	usecond_nanosleep(1000);

	val = 0x03ff;
	ms41929_write_reg(0x2a, val);  //设置STEP为3.64ms  0x07ff
	usecond_nanosleep(1000);

	val = 0x0087;
	ms41929_write_reg(0x21, val);  //设置STEP为3.64ms  0x07ff
	usecond_nanosleep(1000);
	
	//IR-CUT
	val = 0x0004;
	ms41929_write_reg(0x2c, val); 
	usecond_nanosleep(1000);	
	
}
int ms41929_init(void)
{
    int ret = 0;
    int fd = -1;
    
    gpio_set_value(GPIO_MT_RST,0);
    usecond_nanosleep(500000);
    gpio_set_value(GPIO_MT_RST,1);
	gpio_set_direction(GPIO_MT_VDFZ,0);
    gpio_set_value(GPIO_MT_VDFZ,0);

    if (fd_ms41929 < 0){

        fd = open(MS41929_DEVICE_NAME,O_WRONLY);
        if (fd < 0){
            printf("open %s is error \n",MS41929_DEVICE_NAME);
            ret = -1;    
        }else{
            fd_ms41929 = fd;
        }    
    } 
    ms41929_reg_init();
    return ret;
}

int ms41929_uninit(void)
{
    int ret = 0;
    if (fd_ms41929 < 0){
        close(fd_ms41929);
        fd_ms41929 = -1;
    }
    return ret;
}
