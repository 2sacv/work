#ifndef __GPIO_H__
#define __GPIO_H__


#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_LOW 0
#define GPIO_HIGH 1

struct gpio {
	int 		  id;
	unsigned char direction;
	int 		  value;
};


int gpio_init(void);
int gpio_uninit(void);
int gpio_set_direction(int gpio,char *direction);
int gpio_set_value(int gpio,int value);
int gpio_get_value(int gpio,int *value);

#ifdef __cplusplus
}
#endif

#endif //__GPIO_INGENIC_IPC_H__
