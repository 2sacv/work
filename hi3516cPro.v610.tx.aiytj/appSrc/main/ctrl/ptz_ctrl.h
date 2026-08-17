#ifndef PTZ_CTRL_H_
#define PTZ_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#include "jconfstruct.h"
#include "js_scheduler.h"
#include "g_log.h"

#define PTZ_PRESET_BIN_FILE  "/opt/conf/ptz_config.bin"
#define MOTOR_DEV           "/dev/motor"
#define PRESET_VAILD  0x3721
#define MOTOR_MAX_SPEED 63
#define MAX_USER_PRESET 33 // 允许用户设置的最大预置位数量
#define MAX_SEQUENCE_STEP 6 // 巡航预置位范围，app 只允许设置预置位 1~6

#define PTZ_LOOP_TIME 40 // 单位 ms，这个和数字变倍速度有关

#define PAN_MOTOR 0  // 水平马达编号
#define TILT_MOTOR 1 // 垂直马达编号
#define MAX_MOTOR_NUM 2  // 最大电机数量

#define BASE_WIDTH 1920   // 跟踪基准坐标系宽
#define BASE_HEIGHT 1080  // 跟踪基准坐标系高
#define X_FILTER_RANGE_MAX (BASE_WIDTH / 2 * 0.20)  // 跟踪水平过滤最大值
#define X_FILTER_RANGE_MIX (BASE_WIDTH / 2 * 0.05)  // 跟踪水平过滤最小值
#define Y_FILTER_RANGE_MAX (BASE_HEIGHT / 2 * 0.20)  // 跟踪垂直过滤最大值
#define Y_FILTER_RANGE_MIX (BASE_HEIGHT / 2 * 0.05)  // 跟踪垂直过滤最小值

// 实测 173cm 的人距离镜头 5m 时框的宽度为 150 pix，13 米时框的宽度为 60 pix，因此限制范围为 60~150
#define ZOOM_IN_LIMIT (int)(150)  // 跟踪目标宽度大于该值进行缩小
#define ZOOM_OUT_LIMIT (int)(60) // 跟踪目标宽度小于该值进行放大
#define FOLLOW_DZOOM_STEP 20 // 跟踪放大缩小一步的步数，0.2*100

// 人形居中，宽高限制
#define PERSON_CENTER_ZOOM_IN 150
#define PERSON_CENTER_ZOOM_OUT 60

#define DZOOM_MODE_IDLE 0
#define DZOOM_MODE_MANUAL 1
#define DZOOM_MODE_FOLLOW 2
#define DZOOM_MODE_CALL_GURAD 3

#define DZOOM_MIN_ZOOM 100 // 对应 1.00 倍，这个值不要改
#define DZOOM_MAX_ZOOM 175 // 对应 2.00 倍，放大倍数越大，细节丢失越严重，cpu 压力越高
#define DZOOM_SPEED_ZOOM_MIN 1 // 数字变倍最小速度
#define DZOOM_SPEED_ZOOM_MAX 5 // 数字变倍最大速度
#define DZOOM_SPEED_COORDINATE_MIN 0.5 // 数字变倍坐标移动最小速度
#define DZOOM_SPEED_COORDINATE_MAX 5 // 数字变倍坐标移动最大速度
#define DZOOM_MANUAL_ZOOM_SPEED 1 // 手动控制时的变倍速度
#define DZOOM_MANUAL_COORDINATE_SPEED 4 // 手动控制时的坐标移动速度
#define DZOOM_OSD_CHN 4 // 数字变倍占用通道，目前使用扩展 osd 4

#define MOTOR_CUR_STEP (-1)  // 表示当前步数，用于调用预置位时不改变当前马达位置

#define PTZ_INFO dbg_ptz

/*IOCTL 相关配置*/
typedef enum { // 马达运动方向，
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

typedef enum {
    E_MOVE_NONE       = 0,
    E_MOVE_UP         = 1,
    E_MOVE_DOWN       = 2,
    E_MOVE_LEFT       = 3,
    E_MOVE_RIGHT      = 4,
    E_MOVE_RIGHT_UP   = 5,
    E_MOVE_RIGHT_DOWN = 6,
    E_MOVE_LEFT_UP    = 7,
    E_MOVE_LEFT_DOWN  = 8,
    E_MOVE_STOP       = 9,
} eCmdType1;

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

typedef struct {
    int type;
    int cmd;
    int data1;
    int data2;
    int skip_pd_save;
    char packet[512];
    int64_t timestamp; /*pelco 指令时间戳*/
} PelcoCmd;

typedef struct {  // 用于 pelco 命令回复，只有一些特殊的命令需要回复
    int64_t timestamp; /*pelco 指令时间戳*/
    int result; // 执行结果 成功或者失败
    char payload[64]; // 如果失败，填写原因 
} PelcoReponse;

typedef enum {
    PD_NONE,     // 功能空，初始化之后未有任何操作处于该状态
    PD_POSITION, // 位置恢复，手动操作云台运动过后处于该状态
    PD_PRESET,   // 预置位恢复，调用预置位后处于该状态
    PD_SEQUENCE, // 巡航恢复，恢复巡航状态
} PdFunction_t; // power down recovery function 

typedef enum { // 预置位定义
    PRESET_PD_POSITON = 0, // 掉电恢复位置
    PRESET_FOLLOW_PRESET_MIN = 1, // 跟踪看守位最小值
    PRESET_FOLLOW_PRESET_MAX = 6, // 跟踪看守位最大值
    /*各功能预置位定义*/
    PRESET_FOLLOW_DEFAULT_POS = 200, // 移动跟踪默认看守位，当指定的看守位不存在时，该位置生效
    PRESET_PANORAMIC = 201, // 全景扫描预置位
    PRESET_VIDEO_MASK = 202, // 隐私遮挡预置位
    /*联动设置预置位定义*/
    PRESET_LAMP_IR = 205, // 设置黑白模式
    PRESET_LAMP_WHITE = 206, // 设置全彩模式
    PRESET_LAMP_DBL = 207, // 设置智能模式

    PRESET_MIRROR_NORMAL = 208, // 设置镜像正常
    PRESET_MIRROR_HORIZONTAL = 209, // 设置水平镜像
    PRESET_MIRROR_VERTICAL = 210, // 设置垂直镜像
    PRESET_MIRROR_DIAGONAL = 211, // 设置对角镜像

    PRESET_SUPPRESS_FIFTY = 212, // 设置强光抑制50
    PRESET_SUPPRESS_ONEHUNDRED = 213, // 设置强光抑制100

    PRESET_OPEN_DHCP = 214, // dhcp开
    PRESET_CLOSE_DHCP = 215, // dhcp关
    
    /*云台功能预置位定义*/
    PRESET_START_SEQUENCE = 220, // 开启巡航

    PRESET_PTZ_INIT = 250, // 云台初始化
    PRESET_PTZ_DEFAULT = 251, // 云台配置恢复默认
    PRESET_AUTO_FOCUS_ONECE = 253, // 光学变倍使用，聚焦一次
    PRESET_ZOOM_OSD = 254, // 变倍 osd 使能开关
    PRESET_ENABLE_DZOOM = 255, // 数字变倍开关

    PRESET_MAX = 256,
} PresetNo_t;

typedef struct { 
    uint32_t flag; // 标记预置位是否有效
    int steps[MAX_MOTOR_NUM]; // 马达的位置坐标 
    uint16_t zoom_pos;  // 光学变倍位置
    uint16_t dzoom_pos; // 数字变倍位置，原数是 float 型，放大 100 倍，方便处理
} Preset;

typedef struct {
    Preset preset; // 目标位置
    int speed;     // 速度值
} FollowPreset; // 移动跟踪预置位，带速度的预置位

typedef struct { 
    Preset   preset[PRESET_MAX];
    PdFunction_t pd_func;
    uint32_t pd_preset; // 存储最后调用的预置位，用于掉电恢复 
} PtzPreset;  // ptz 预置位配置，存储在文件 PTZ_PRESET_BIN_FILE

typedef struct {
    bool enable;
    int cur_preset_no; // 当前预置位号
    struct timespec wait_time; // 每个预置位停留时间
    struct timespec sequence_time; // 总的巡航时间，到点自动关闭
} SequenceCfg;

typedef struct {
    bool pan_scan_enable;
    bool tilt_scan_enable;

    int pan_scan_count;  // 水平扫描碰边次数
    int tilt_scan_count; // 垂直扫描碰边次数
} PanoramicScanCfg;

typedef struct {
    bool enable;
    int mode; // 运动模式
    float cur_zoom_left;  // 当前裁剪画面起始点 x 轴坐标，基于 1080 分辨率
    float cur_zoom_top;  // 当前裁剪画面起始点 y 轴坐标，基于 1080 分辨率
    int cur_center_x;  // 当前裁剪画面起始点 x 轴坐标，基于 1080 分辨率
    int cur_center_y;  // 当前裁剪画面起始点 y 轴坐标，基于 1080 分辨率
    int dst_center_x; // 目标裁剪中心点 x 坐标
    int dst_center_y; // 目标裁剪中心点 y 坐标
    float speed_x;  // x 轴坐标移动速度
    float speed_y;  // y 轴坐标移动速

    int speed_zoom; // 数字变倍步进值，两位小数精度，放大 100 倍处理
    int cur_zoom; // 当前变倍倍数，两位小数精度，放大 100 倍处理
    int dst_zoom; // 目标倍数，两位小数精度，放大 100 倍处理
    
    /*数字变倍显示*/
    bool osd_enable;
    int max_display_zoom; // 最大显示倍数，一位小数精度，为了和 cur_zoom 保持统一，放大 100 倍
} DigitarZoom;

typedef enum {
    FOLLOW_IDEL = 0,  // 空闲状态，处于看守位时位于空闲状态
    FOLLOW_RUN,   // 运动状态，运动过，不处于看守卫时位于该状态
    FOLLOW_MANUAL, // 手动状态，无论之前什么状态，手动转云台后，位于该状态
} FollowStatus_t;

typedef struct {
    bool enable;

    FollowStatus_t status;
    struct timespec guard_time; // 看守时间，到时间回看守位
} FollowParam;

enum {
    CMD_MOTOR_CFG = 1 << 0,
    CMD_FOLLOW_CFG = 1 << 1,
    CMD_VIDEO_REPORT = 1 << 2,
    CMD_PERSON_CENTER_ENABLE = 1 << 3,
    CMD_PERSON_CENTER_DISABLE = 1 << 4,

    CMD_REINIT_PTZ = 1 << 10, // 重新初始化
    CMD_FOLLOW_ENABLE = 1 << 11,
    CMD_FOLLOW_DISABLE = 1 << 12,
    CMD_FOLLOW_PRESET  = 1 << 13,
};

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int64_t timestamp;
} FollowTargetInfo; // 人形居中目标信息

typedef struct {
    int track_id;

    int average_width;
    FollowTargetInfo follow_last_pos;  // 跟踪之前的位置
    FollowTargetInfo last_pos; // 上一个位置
} PersonCenterTrackInfo; // 人形居中跟踪信息

typedef struct {
    int refresh; // 刷新标志，表明人在画面中，但是未动，为 true 刷新看守定时，但是不跟踪
    int dst_center_x;
    int dst_center_y;
    float speed_x;
    float speed_y;
    int dst_zoom;
} PersonCenterPreset; // 人形居中

typedef struct {
    bool enable;

    FollowStatus_t status;
    struct timespec guard_time; // 看守时间，到时间回看守位

    PersonCenterPreset guard_preset; // 人形居中看守位
} PersonCenter;

typedef struct {
    motor_t motor_cfg;
    follow_info_t follow_cfg;
} PtzCfg;

typedef struct {
    int motor_fd;
    int moved;  // 位置是否有变化，不重复调用预置位
    bool is_init;
    pthread_spinlock_t spinlock;
    PtzPreset ptz_preset;
    MotorInitParam init_param;

    DigitarZoom dzoom; // 数字变倍
    float show_zoom_prev;
    SequenceCfg sequence; // 巡航
    PanoramicScanCfg panoramic_scan; // 全景扫描
    int save_position_tik; // 保存位置定时
    FollowParam follow;
    PersonCenter person_center;

    JSScheduler sch;
    JSTCHandle hdl_loop;
    JSTCHandle hdl_stop;
    struct cmdstat *ctx;
} PtzRun;

void switch_ircut(MotorIoctl_t cmd);

bool peclo_cmd_enqueue(PelcoCmd *cmd); // pelco 指令入对
bool follow_cmd_enqueue(Preset *cmd);  // 跟踪指令入队
bool ptz_is_run();

void ptz_init();
int ptz_config_default(); 
void ptz_deinit();
bool sequence_is_enable();
time_t sequence_time_left();
bool pan_scan_is_enable();
bool tilt_scan_is_enable();

const char *x_scope();
const char *y_scope();
bool is_preset_vaild(int no); 
void motor_reinit();
int get_motor_status(MotorStatus *status);
int ptz_follow_handing(int tik, int tkid, int x, int y, int width, int height);
int ptz_person_center_handing(int x, int y, int width, int height, int track_id);
int debug_to_preset(int pan_steps, int tilt_steps);
int set_motor_zero();
FollowStatus_t get_follow_status();
FollowStatus_t get_person_center_status();

Direction_t handle_pan_reverse(Direction_t dir);
Direction_t handle_tilt_reverse(Direction_t dir);

int is_dzoom_enable();
int is_ptz_init(void);
int dzoom_get_cur_zoom();
int dzoom_show_osd();

int get_pelco_response(int64_t timestamp, char *buf, int buflen, int timeout);

void ptz_move_motor(eCmdType1 cmd, int ms_delay_stop, int skip_pd_save);

void ptz_save_preset(PresetNo_t preset_num, int skip_pd_save);

void ptz_call_preset(PresetNo_t preset_num, int skip_pd_save);

#ifdef __cplusplus
}
#endif

#endif
