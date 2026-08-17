#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/mfd/core.h>
#include <linux/mempolicy.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/proc_fs.h>
#include <linux/gpio/consumer.h>

#define CLK   60  // CLK引脚
#define EN    10  // EN引脚 
#define DATA  61  // DATA引脚
#define RST   11  // RST引脚

unsigned int OUT1 = 0x8000; //1000 0000 00   00 0000
unsigned int OUT2 = 0x4000; //0100 0000 00   00 0000
unsigned int OUT3 = 0x2000; //0010 0000 00   00 0000
unsigned int OUT4 = 0x1000; //0001 0000 00   00 0000
unsigned int OUT5 = 0x0800; //0000 1000 00   00 0000
unsigned int OUT6 = 0x0400; //0000 0100 00   00 0000
unsigned int OUT7 = 0x0200; //0000 0010 00   00 0000
unsigned int OUT8 = 0x0100;;//0000 0001 00   00 0000

unsigned int OUTA = 0x00B0; //0000 0000 10   11 0000
unsigned int OUTB = 0x0070; //0000 0000 01   11 0000

unsigned int Stop_IRCUT = 0x0000;   //0000 0000 0000 0000
unsigned int Clear_IRCUT = 0xFF00;  //1111 1111 0000 0000

unsigned int ircut_status = 0x0000;
unsigned int motor_status = 0x0000;

static void send_data( unsigned int data_value);

static int init_gpio(int gpio)
{
    char lable[32] = {0};
    int ret = 0;
    
    snprintf(lable, sizeof(lable), "5809 gpio%d", gpio);
    ret = gpio_request(gpio, lable);
    if (ret < 0) {
        printk("request gpio%d fail\n", gpio);
        return ret;
    }
    printk("init gpio%d succcess\n", gpio);
    return gpio_direction_output(gpio, 0);
}

void init_xtm5809(void)
{
    init_gpio(CLK);
    init_gpio(EN);
    init_gpio(DATA);
    //init_gpio(RST);
    send_data(0);
}

static void clk_high(void)
{
    gpio_set_value(CLK, 1);
    udelay(20);
}

static void clk_low(void)
{
    gpio_set_value(CLK, 0);
    udelay(20);
}

static void enable_en(void)
{
    gpio_set_value(EN, 0);
}

static void disable_en(void)
{
    gpio_set_value(EN, 1);
}

static void data_high(void)
{
    gpio_set_value(DATA, 1);
}

static void data_low(void)

{
    gpio_set_value(DATA, 0);
}

static void enable_rst(void)
{
    //gpio_set_value(RST, 0);
}

static void disable_rst(void)
{
    //gpio_set_value(RST, 1);
}

/* CLK 25Khz */

static void send_data( unsigned int data_value)
{

    unsigned int i;
    unsigned char cycle_cnt;
    cycle_cnt = 10;

    enable_rst();
    enable_en();

    for ( i= 0; i < cycle_cnt; i++ ) {
        if ((data_value & 0x8000) == 0x8000) {  // 23us 
            data_high();
        } else {
            data_low();
        }

        clk_high();
        data_value = data_value << 1;
        clk_low();
    }
    disable_en();
}

/****************************************************************************************
Descriptions:

电机接线
橙 A 
黄 B
粉 C
蓝 D

4相8个节拍控制 

反转 (CCW 逆时针方向 )

A → AB → B → BC → C → CD → D → DA → A

正转

A → AD → D → DC → C → CB → B → BA → A

*****************************************************************************************/

void motor_stop(void)
{
    send_data(0);
}

void step_motor(int motor_no,uint32_t step)
{
    uint32_t cmd[2][8] = {
        {OUT5, OUT5|OUT8, OUT8, OUT8|OUT7, OUT7, OUT7|OUT6, OUT6, OUT6|OUT5},
        {OUT1, OUT1|OUT4, OUT4, OUT4|OUT3, OUT3, OUT3|OUT2, OUT2, OUT2|OUT1}
    };

    motor_status = cmd[motor_no][step % 8] | ircut_status;
    send_data(motor_status);
}

void ircut_send_data(void)
{
    send_data(motor_status);
}

void ir_cut(int cut_type)
{
    if(cut_type == 20) {  // OUTA
        ircut_status = OUTA;
        motor_status = motor_status | OUTA;
    } else if (cut_type == 21) { // OUTB
        ircut_status = OUTB;
        motor_status = motor_status | OUTB;
    } else { // Stop_IRCUT 
        ircut_status = Stop_IRCUT;
        motor_status = motor_status & Clear_IRCUT;
    }
}

/*  XTM5809 IR_CUT 的开启和关闭 (电机正转 反转)，注意区别 EN管脚使能和两路步进电机控制逻辑的区别:

1、IR-CUT控制逻辑发完OUTA或者OUTB开启指令后，EN管脚拉高(disable)将无法控制OUT的关闭；两路步进电机(OUT1-OUT8) OUT输出会
随着EN管脚拉高停止输出。

2、IR-CUT控制时间，不受EN管脚拉高(disable)再拉低控制，会一直保持并锁存(EN再次拉低无效，不重新发Stop会一直开启)，所以需要重新
发Stop指令，这个也是和XTM5808的区别。

3、如果在控制2路步进电机(OUT1-OUT8)这个过程中，需要控制IR-CUT，需要注意控制逻辑的指令对应的bit 不能发生改变
(用 | 处理，只改变需要控制的bit位) ，同时需要在IR-CUT开启完成后，单独发送 Stop指令；

其实IR-CUT开启时间较短，并且开启频率低，在实际应用比较中 同时控制步进+ 控制IR-CUT的场景很少，XTM5808不支持这种同时控制，
而XTM5809可以支持；

*/

