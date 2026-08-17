/******************************************************************************
    Copyright (C), 2008-2028, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : motor.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2024-12-24
    Description  :
    History      :
                        created by tianjun. 2015-06-14
******************************************************************************/

#ifndef __JABSCO_MOT_H__
#define __JABSCO_MOT_H__

#define MAX_MOTOR_NUM 2  // 最大电机数量

#define MOTOR_CUR_STEP (-1)  // 表示当前步数，用于调用预置位时不改变当前马达位置

typedef enum {
    DIRECTION_DOWN = -1, // 步数减少方向
    DIRECTION_STOP = 0,  // 停止
    DIRECTION_UP = 1,   // 步数增加方向
} Direction_t;
// 前10的枚举有特殊定义,不要使用
typedef enum { // ioctl cmd 命令号

    MOTOR_IOCTL_INIT = 10,   // 初始化
    MOTOR_IOCTL_SET_PARAM,  // 设置参数
    MOTOR_IOCTL_GET_PARAM,  // 获取参数
    MOTOR_IOCTL_MOVE,       // 马达移动命令，
    MOTOR_IOCTL_CALL_PRESET, // 移动到指定位置
    MOTOR_IOCTL_GET_STATUS,  // 获取当前运动状态
    MOTOR_IOCTL_SET_ZERO,  // 设置零点，用来计算最大步数
    IRCUT_IOCTL_P = 20,   //
    IRCUT_IOCTL_N = 21,        //
} MotorIoctl_t;

typedef struct {
    uint32_t edge_steps[MAX_MOTOR_NUM];  // 防碰边预留步数
    uint32_t max_steps[MAX_MOTOR_NUM];   // 最大步数
    Direction_t init_dir[MAX_MOTOR_NUM]; // 初始化方向配置
    int single_motor_rotation; // 单电机旋转 true:打开    false:关闭
} MotorInitParam;

typedef struct {
    int speed[MAX_MOTOR_NUM]; // 目标速度
    Direction_t dir[MAX_MOTOR_NUM]; // 方向
} MotorMove;

typedef struct {
    int speed[MAX_MOTOR_NUM]; // 目标速度
    int steps[MAX_MOTOR_NUM]; // 目标坐标，不是运动到目标的距离
} MotorCallPreset;

typedef struct {
    Direction_t dir[MAX_MOTOR_NUM]; // 方向
    int steps[MAX_MOTOR_NUM]; // 当前坐标    
    int dst_steps[MAX_MOTOR_NUM]; // 目标坐标
} MotorStatus;

typedef union {
    MotorInitParam init_param;
    MotorMove move;
    MotorCallPreset preset;
    MotorStatus status;
} MotorIoctl;

typedef enum {
    WORK_MODE_NONE = 0,  // 未初始化
    WORK_MODE_INIT,  // 初始化
    WORK_MODE_MOVE,  // 马达移动工作模式
} WorkMode_t;

typedef enum {
    E_IRCUT_START   = 0,
    E_IRCUT_WAIT    = 1,
    E_IRCUT_STOP    = 2,
} eIrcutMode;

typedef struct {
    // gpio 引脚定义
    uint32_t gpio1;  // 马达 4 个脚，分别定义 1 2 3 4
    uint32_t gpio2;
    uint32_t gpio3;
    uint32_t gpio4;
} MotorGpio;

typedef struct { // 步进信息
    int max_steps; // 马达实际最大步数，初始化的时候和正常运行的时候最大步数不同
    int tiks;
    int cur_step; // 当前步数
    int cur_speed;   // 当前速度，用来计算 tiks
    Direction_t cur_dir; // 运动方向

    int dst_speed; // 目标速度
    int dst_steps; // 目标坐标，不是运动到目标的距离
} MotorRun;

typedef struct {
    uint32_t edge_steps;  // 防碰边预留步数
    uint32_t max_steps;   // 最大步数
    Direction_t init_dir; // 初始化方向配置
} MotorParam;

typedef struct { // 马达信息
    MotorGpio gpio;
    MotorParam param; // 马达参数
    MotorRun run;
} MotorInfo;

typedef struct {
    WorkMode_t work_mode; // 工作模式
    eIrcutMode ircut_mode;
    uint32_t init_motor_no; // 当前初始化马达编号
    uint32_t init_stage;  // 当前马达初始化阶段，比如转半圈回一圈，再转半圈，加上每一次转动之前的参数计算，就是六个阶段
    
    int is_single_motor_rotation;  // 是否单马达旋转

    MotorInfo motor[MAX_MOTOR_NUM];
} MotorLoopInfo;

typedef struct {  // 马达驱动设备信息
	struct platform_device *pldev;
    struct device	 *dev;
	struct miscdevice mdev;
    unsigned int *timer_hanlde;

    MotorLoopInfo motor_loop;
} MotorDevInfo;

#endif
