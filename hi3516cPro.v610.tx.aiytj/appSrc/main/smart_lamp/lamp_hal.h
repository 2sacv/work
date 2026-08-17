/*
 *       Filename:  lamp_hal.h
 *    Description:  
 *        Version:  1.0
 *        Created:  11/07/2025 11:39:08 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  wangr (), 
 *   Organization:  
 */
#ifndef __LAMP_HAL_H__
#define __LAMP_HAL_H__
#ifdef __cplusplus 
extern "C" {
#endif

#define GPIO_INFRARED       (6)
#define GPIO_INFRARED_ON    (1)
#define GPIO_INFRARED_OFF   (0)

#define GPIO_WHITE          (8)
#define GPIO_WHITE_ON       (1)
#define GPIO_WHITE_OFF      (0)

#define GPIO_RED_BLUE       (1)
#define GPIO_RED_BLUE_ON    (1)
#define GPIO_RED_BLUE_OFF   (0)

int set_lamp_status(int gpio, int value);
int set_ircut_status(int is_day);
int lamp_gpio_init(void);

#ifdef __cplusplus
}
#endif
#endif // __LAMP_HAL_H__

