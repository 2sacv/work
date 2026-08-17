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
#include "gpio.h"
#include "io.h"
#include "utils.h"
#include "debug.h"

static int gpio_pin_init(int id)
{
    int ret = 0;
    ret = gpio_open_export(id);
    if (ret < 0){
        ERR("Warning: GPIO %d exported\n", id);
    }

    return ret;
}

int gpio_init(void)
{
	int ret = 0;
    int i = 0;
	
    int id[] = {
        E_GPIO_4G,
        E_GPIO_SIM,
        E_GPIO_SD_PWR,
        E_GPIO_SD_CD,
        E_GPIO_SD_CMD,
    	E_GPIO_SD_D0,
    	E_GPIO_SD_D1,
    	E_GPIO_SD_D2,
    	E_GPIO_SD_D3        
    };

    for (i = 0; i < ARRAY_SIZE(id) ; i++) {
        gpio_pin_init(id[i]);
    }
	
    return ret;
}

int gpio_uninit(void)
{
	return 0;
}

int gpio_set_value(int id, int value)
{
	int ret =0;
    ret = gpio_open_set_value(id, value);

	return ret;
}

int gpio_get_value(int id, int *value)
{
    gpio_open_get_value(id, value);

    return 0;
}
