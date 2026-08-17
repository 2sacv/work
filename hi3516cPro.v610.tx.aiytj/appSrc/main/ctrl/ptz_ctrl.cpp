#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "ot_common.h"
#include "ss_mpi_vi.h"
#include "ss_mpi_vpss.h"

#include "ptz_ctrl.h"
#include "utils.h"
#include "confapi.h"
#include "jcpService.h"
#include "cmdstat.h"
#include "ptz_follow.h"
#include "encode_osd.h"
#include "encode_main.h"
#include "encode_vi.h"
#include "factory_db.h"
#include "g_stat.h"
#include "circular_queue.h"
#include "system_sch.h"

/*线性映射公式，把 xmin~xmax 映射到 ymin~ymax 然后根据 x 求取 y 值*/
#define LINEAR_MAP(x, xmin, xmax, ymin, ymax) (((ymax) - (ymin)) * ((x) - (xmin)) / ((xmax) - (xmin)) + (ymin))

static PtzCfg cfg = {0};
static PtzCfg raw = {0};
static PtzRun run = {.motor_fd = -1, .save_position_tik = -1};
static PtzCfg *g_cfg_ptz = &cfg;
static PtzCfg *g_raw_ptz = &raw;
static PtzRun *g_run_ptz = &run;

int start_sequence(int64_t timestamp);

/*****************************************************************/
/*pelco 指令队列                                                  */
/*****************************************************************/
static CircularQueue<PelcoCmd, 32> g_pelco_cmd_queue;
bool peclo_cmd_enqueue(PelcoCmd *cmd)
{
    int ret = 0;
    pthread_spin_lock(&g_run_ptz->spinlock);
    ret = g_pelco_cmd_queue.Enqueue(*cmd);
    pthread_spin_unlock(&g_run_ptz->spinlock);
    return ret;
}
bool pelco_cmd_dequeue(PelcoCmd *cmd)
{
    int ret = 0;
    pthread_spin_lock(&g_run_ptz->spinlock);
    ret = g_pelco_cmd_queue.Dequeue(*cmd);
    pthread_spin_unlock(&g_run_ptz->spinlock);
    return ret;
}
void pelco_cmd_clear()
{
    pthread_spin_lock(&g_run_ptz->spinlock);
    g_pelco_cmd_queue.ClearQueue();
    pthread_spin_unlock(&g_run_ptz->spinlock);
}
bool pelco_cmd_is_empty()
{
    return g_pelco_cmd_queue.IsEmpty();
}

/*****************************************************************/
/*pelco 执行结果队列                                                */
/*****************************************************************/
static CircularQueue<PelcoReponse, 4> g_pelco_response_queue;
bool peclo_response_enqueue(PelcoReponse *res)
{
    return g_pelco_response_queue.Enqueue(*res);
}
bool pelco_response_dequeue(PelcoReponse *res)
{
    return g_pelco_response_queue.Dequeue(*res);
}
void pelco_response_clear()
{
    g_pelco_response_queue.ClearQueue();
}
bool pelco_response_is_empty()
{
    return g_pelco_response_queue.IsEmpty();
}
/*
    添加一条 pelco 指令执行回复
    timestamp: 指令时间戳，与执行指令一一对应，小于等于 0 表示无效
    result: 执行结果，SUCCESS or FAILURE
    payload: 需要返回的文本内容
*/
int add_pelco_response(int64_t timestamp, int result, const char *payload)
{
    if (timestamp <= 0) {
        return FAILURE;
    }

    PelcoReponse res = {0};
    res.timestamp = timestamp;
    res.result = result;
    if (payload != NULL) {
        strncpy(res.payload, payload, sizeof(res.payload) - 1);
    }

    peclo_response_enqueue(&res);

    return SUCCESS;
}

/*
    获取 pelco 指令执行回复
    @timestamp: 指令时间戳，与执行指令一一对应，小于等于 0 表示无效
    @buf: 装载对应指令的执行回复
    @buflen: 缓冲区大小
    @timeout: 等待回复超时时间, 单位: ms

    @return 获取到对应回复返回执行结果，其余超时返回失败
*/
int get_pelco_response(int64_t timestamp, char *buf, int buflen, int timeout)
{
    if (timestamp <= 0) {
        return FAILURE;
    }

    int t = 0;
    PelcoReponse res = {0};

    while (t < timeout) {
        if (pelco_response_dequeue(&res)) {
            if (res.timestamp == timestamp) {
                DBG("wait time[%d ms] timestamp[%lld] get result[%d]\n", t, timestamp, res.result);
                strncpy(buf, res.payload, buflen - 1);
                return res.result;
            }
        }

        /*根据 loop 间隔进行查询，保证快速获取结果*/
        usleep(PTZ_LOOP_TIME * 1000);
        t += PTZ_LOOP_TIME;
    }

    strncpy(buf, "get result timeout!", buflen - 1);
    return FAILURE;
}

/*****************************************************************/
/* 移动跟踪指令队列                                               */
/*****************************************************************/
static CircularQueue<FollowPreset, 32> g_follow_cmd_queue;
bool follow_cmd_enqueue(FollowPreset *cmd)
{
    return g_follow_cmd_queue.Enqueue(*cmd);
}
bool follow_cmd_dequeue(FollowPreset *cmd)
{
    return g_follow_cmd_queue.Dequeue(*cmd);
}
void follow_cmd_clear()
{
    g_follow_cmd_queue.ClearQueue();
}
bool follow_cmd_is_empty()
{
    return g_follow_cmd_queue.IsEmpty();
}
/*****************************************************************/
/* 人形居中跟踪指令队列                                             */
/*****************************************************************/
static CircularQueue<PersonCenterPreset, 32> g_person_center_cmd_queue;
bool person_center_cmd_enqueue(PersonCenterPreset *cmd)
{
    return g_person_center_cmd_queue.Enqueue(*cmd);
}
bool person_center_cmd_dequeue(PersonCenterPreset *cmd)
{
    return g_person_center_cmd_queue.Dequeue(*cmd);
}
void person_center_cmd_clear()
{
    g_person_center_cmd_queue.ClearQueue();
}
bool person_center_cmd_is_empty()
{
    return g_person_center_cmd_queue.IsEmpty();
}
/*****************************************************************/
/* 函数声明                                               */
/*****************************************************************/
bool ptz_is_run();
bool ptz_is_run2(MotorStatus *status);
static int ptz_config_init();
bool is_preset_vaild(int no);
int person_center_save_guard();
void refresh_person_center_guard_timer();
/*****************************************************************/
/* ioctl 相关操作                                         */
/*****************************************************************/
static int call_motor_ioctrl(MotorIoctl_t cmd, MotorIoctl *param)
{
    int ret = 0;
    if (g_run_ptz->motor_fd > 0) {
        pthread_spin_lock(&g_run_ptz->spinlock);
        ret = ioctl(g_run_ptz->motor_fd, cmd, param);
        if (ret < 0) {
            ERR("ioctl fail\n");
        }
        pthread_spin_unlock(&g_run_ptz->spinlock);
    }

    return ret;
}

void switch_ircut(MotorIoctl_t cmd)
{
    MotorIoctl param = {0,};
    call_motor_ioctrl(cmd, &param);

    return;
}

static inline int motor_start_init()
{
    return call_motor_ioctrl(MOTOR_IOCTL_INIT, NULL);
}

static int set_motor_init_param(MotorInitParam *init_param)
{
    MotorIoctl ioctl_param = {0};

    memcpy(&ioctl_param.init_param, init_param, sizeof(ioctl_param.init_param));

    return call_motor_ioctrl(MOTOR_IOCTL_SET_PARAM, &ioctl_param);
}

static int get_motor_init_param(MotorInitParam *init_param)
{
    MotorIoctl ioctl_param = {0};

    int ret = call_motor_ioctrl(MOTOR_IOCTL_GET_PARAM, &ioctl_param);
    if (ret < 0) {
        return FAILURE;
    }

    memcpy(init_param, &ioctl_param.init_param, sizeof(ioctl_param.init_param));

    return SUCCESS;
}

int get_motor_status(MotorStatus *status)
{
    MotorIoctl ioctl_param = {0};

    int ret = call_motor_ioctrl(MOTOR_IOCTL_GET_STATUS, &ioctl_param);
    if (ret < 0) {
        return FAILURE;
    }

    memcpy(status, &ioctl_param.status, sizeof(ioctl_param.status));
    return SUCCESS;
}

static int set_motor_move(MotorMove *move)
{
    MotorIoctl ioctl_param = {0};

    memcpy(&ioctl_param.move, move, sizeof(ioctl_param.move));

    return call_motor_ioctrl(MOTOR_IOCTL_MOVE, &ioctl_param);
}

static int call_motor_preset(Preset *preset, int speed)
{
    MotorIoctl ioctl_param = {0};

    for (int i = 0; i < MAX_MOTOR_NUM; ++i) {
        ioctl_param.preset.steps[i] = preset->steps[i];
        ioctl_param.preset.speed[i] = speed;
    }
    return call_motor_ioctrl(MOTOR_IOCTL_CALL_PRESET, &ioctl_param);
}

/*设置马达当前位置为 0 点，确保马达停下再调用，设置该参数驱动内部是非线程安全的*/
int set_motor_zero()
{
    return call_motor_ioctrl(MOTOR_IOCTL_SET_ZERO, NULL);
}
/*****************************************************************/
/* 预置位配置文件操作                                        */
/*****************************************************************/
int ptz_read_preset_file()
{
    int fd = -1;
    fd = open(PTZ_PRESET_BIN_FILE, O_RDONLY);
    if (fd < 0) {
        return FAILURE;
    }
    int read_size = Readfully(fd, &g_run_ptz->ptz_preset, sizeof(g_run_ptz->ptz_preset));
    close(fd);

    if (read_size != sizeof(g_run_ptz->ptz_preset)) {
        ERR("read size error, remove\n");
        remove(PTZ_PRESET_BIN_FILE);
        bzero(&g_run_ptz->ptz_preset, sizeof(g_run_ptz->ptz_preset));
    }
    return SUCCESS;
}

int ptz_write_preset_file()
{
    int fd = -1;
    fd = open(PTZ_PRESET_BIN_FILE, O_RDWR | O_CREAT, S_IRWXU);
    if (fd < 0) {
        return FAILURE;
    }

    PtzPreset ptz_preset;
    Readfully(fd, &ptz_preset, sizeof(ptz_preset));

    if (memcmp(&ptz_preset, &g_run_ptz->ptz_preset, sizeof(ptz_preset)) != 0) {
        // 有差异才写，减少写文件次数
        lseek(fd, 0, SEEK_SET);
        Writefully(fd, &g_run_ptz->ptz_preset, sizeof(g_run_ptz->ptz_preset));
        fsync(fd);
    }

    close(fd);

    return SUCCESS;
}
#if defined(DIGITAL_ZOOM)
/*****************************************************************/
/* 数字变倍                                           */
/*****************************************************************/
/*显示当前倍数 OSD*/
int dzoom_show_osd()
{
    static int zoom = 0;
    DigitarZoom *dzoom = &g_run_ptz->dzoom;
    if (dzoom->osd_enable && dzoom->enable) {
        float show_zoom = LINEAR_MAP(dzoom->cur_zoom, DZOOM_MIN_ZOOM, DZOOM_MAX_ZOOM, DZOOM_MIN_ZOOM, dzoom->max_display_zoom) / 100.0; // 倍数都是放大 100 倍处理的，显示的时候要除回来
        char osdstr[16] = {0};

        if(0.05 <= fabsf(zoom - show_zoom)) {
            sprintf(osdstr,"%.1fX", show_zoom);
            dbg_ptz("encode_osd_zoom_change, osdstr:%s\n", osdstr);
            encode_osd_zoom_change(1, osdstr);
        }
    }

    return SUCCESS;
}

/*打开数字变倍 osd 使能*/
int dzoom_osd_enable()
{
    g_run_ptz->dzoom.osd_enable = true;
    dzoom_show_osd();

    return SUCCESS;
}

/*设置数字变倍模式为空闲*/
int dzoom_idle()
{
    if (g_run_ptz->dzoom.mode == DZOOM_MODE_MANUAL) { // 手动控制结束，需要保存当前人形居中看守位
        person_center_save_guard();
    }

    g_run_ptz->dzoom.dst_center_x = g_run_ptz->dzoom.cur_center_x;
    g_run_ptz->dzoom.dst_center_y = g_run_ptz->dzoom.cur_center_y;
    g_run_ptz->dzoom.dst_zoom = g_run_ptz->dzoom.cur_zoom;
    g_run_ptz->dzoom.mode = DZOOM_MODE_IDLE;

    return SUCCESS;
}

/*获取当前数字变倍模式*/
int dzoom_get_mode()
{
    return g_run_ptz->dzoom.mode;
}

/*获取当前倍数，实际倍数放大 100 倍的结果*/
int is_dzoom_enable()
{
    return g_run_ptz->dzoom.enable;
}

/*获取 ptz 初始化状态*/
int is_ptz_init(void)
{
    return g_run_ptz->is_init;
}

/*关闭数字变倍 osd 使能*/
int dzoom_osd_disable()
{
    OsdExpandS osd_expand = {0};
    conf_get_osdexpandcfg(&osd_expand);

    osd_expand.cusosd[DZOOM_OSD_CHN].id     = DZOOM_OSD_CHN;
    osd_expand.cusosd[DZOOM_OSD_CHN].enable = 0;

    conf_set_osdexpandcfg(osd_expand);

    g_run_ptz->dzoom.osd_enable = false;
    return SUCCESS;
}
/**
 * @brief 获取当前倍数的画面裁剪宽度，基于 1080 坐标系
 *
 * @param cur_zoom 当前倍数
 * @return int 当前画面裁剪宽度
 */
int dzoom_get_zoom_width(int cur_zoom)
{
    /*
        1. 首先计算总的宽度变化范围: WIDTH_RANGE = BASE_WIDTH - BASE_WIDTH * 100.0 / DZOOM_MAX_ZOOM
        2. 宽度变化范围除以总的倍数范围，可以得到一个刻度对应的宽度：WIDTH_RANGE / (DZOOM_MAX_ZOOM - DZOOM_MIN_ZOOM)
        3. 总的宽度减去当前倍数乘以每个倍数对应的刻度，即可得到当前倍数的宽度
    */
    float width_per_zoom = (BASE_WIDTH - BASE_WIDTH * 100.0 / DZOOM_MAX_ZOOM) / (DZOOM_MAX_ZOOM - DZOOM_MIN_ZOOM);
    return (int)(BASE_WIDTH - width_per_zoom * (cur_zoom - DZOOM_MIN_ZOOM)) & ~1; // 保证为偶数，且向下取整
}

/**
 * @brief 获取当前倍数的画面裁剪高度，基于 1080 坐标系
 *
 * @param cur_zoom 当前倍数
 * @return int 当前画面裁剪高度
 */
int dzoom_get_zoom_height(int cur_zoom)
{
    float height_per_zoom = (BASE_HEIGHT - BASE_HEIGHT * 100.0 / DZOOM_MAX_ZOOM) / (DZOOM_MAX_ZOOM - DZOOM_MIN_ZOOM);
    return (int)(BASE_HEIGHT - height_per_zoom * (cur_zoom - DZOOM_MIN_ZOOM)) & ~1; // 保证为偶数，且向下取整
}

/**
 * @brief 调用系统接口，按照指定的起始点坐标和倍数进行画面裁剪
 *
 * @param zoom_left 当前裁剪画面起始点 x 轴坐标，基于 1080 分辨率
 * @param zoom_top 当前裁剪画面起始点 y 轴坐标，基于 1080 分辨率
 * @param cur_zoom 倍数
 * @return int SUCCESS or FAILURE
 */
int dzoom_to_zoom(int zoom_left, int zoom_top, int cur_zoom)
{
    if(FALSE == encode_get_init_status()) {
        return 0;
    }

    int ret = 0;
    ot_vpss_grp grp = 0;
    ot_vpss_crop_info crop_info;
    int width = 0, height = 0;
    int left = 0, top = 0;
    int zoom_width = 0, zoom_height = 0;
    ot_vi_pipe vi_pipe = 0;
    ot_vi_pipe_attr pipe_attr;
    memset(&pipe_attr, 0, sizeof(ot_vi_pipe_attr));

    ret = ss_mpi_vi_get_pipe_attr(vi_pipe, &pipe_attr);
    if(S_OK != ret) {
        ERR("ss_mpi_vi_get_pipe_attr failed with %#x!\n", ret);
        return FAILURE;
    }

    ret = ss_mpi_vpss_get_grp_crop(grp, &crop_info);
    if(S_OK != ret) {
        ERR("ss_mpi_vpss_get_grp_attr failed with %#x!\n", ret);
        return FAILURE;
    }

    zoom_width = dzoom_get_zoom_width(cur_zoom);
    zoom_height = dzoom_get_zoom_height(cur_zoom);

    width = pipe_attr.size.width * zoom_width / BASE_WIDTH;
    height = pipe_attr.size.height * zoom_height / BASE_HEIGHT;

    left = (int)(zoom_left * pipe_attr.size.width / BASE_WIDTH);
    top = (int)(zoom_top * pipe_attr.size.height / BASE_HEIGHT);

    left = RANGE(left, 0, pipe_attr.size.width - width);
    top = RANGE(top, 0, pipe_attr.size.height - height);

    left &= 0xfffffffe; /* 2 对齐*/
    top &= 0xfffffffe; /* 2 对齐*/
    width &= 0xfffffffe; /* 2 对齐*/
    height &= 0xfffffffe; /* 2 对齐*/

    crop_info.crop_mode = OT_COORD_ABS;

    crop_info.crop_rect.x = left;
    crop_info.crop_rect.y = top;
    crop_info.crop_rect.width  = width;
    crop_info.crop_rect.height = height;
    if(cur_zoom  == DZOOM_MIN_ZOOM) {
        crop_info.enable = TD_FALSE;
    } else {
        crop_info.enable = TD_TRUE;
    }

    dbg_ptz("crop_info.enable:%d, x:%d, y:%d, w:%d, h:%d\n", crop_info.enable, crop_info.crop_rect.x, crop_info.crop_rect.y
                                                    , crop_info.crop_rect.width, crop_info.crop_rect.height);
    ret = ss_mpi_vpss_set_grp_crop(grp, &crop_info);
    if(S_OK != ret) {
        ERR("ss_mpi_vpss_set_grp_crop failed with %#x!\n", ret);
        return FAILURE;
    }

    return SUCCESS;
}

/*数字变倍初始化，设置最大倍数，变倍速度，最大显示倍数等*/
int dzoom_init()
{
    DigitarZoom *dzoom = &g_run_ptz->dzoom;
    if (is_preset_vaild(PRESET_ENABLE_DZOOM)) {
        dzoom->enable = true;
    }
    if (is_preset_vaild(PRESET_ZOOM_OSD)) {
        dzoom->osd_enable = true;
    }
#if DIGITAL_ZOOM < 2 || DIGITAL_ZOOM > 100
    DBG("DIGITAL_ZOOM value error");
#endif
    dzoom->max_display_zoom = DIGITAL_ZOOM * 100;
    dzoom->mode = DZOOM_MODE_IDLE;
    dzoom->speed_zoom = DZOOM_MANUAL_ZOOM_SPEED;
    dzoom->cur_zoom = DZOOM_MIN_ZOOM;
    dzoom->dst_zoom = DZOOM_MIN_ZOOM;

    dzoom->cur_zoom_left = 0;
    dzoom->cur_zoom_top = 0;
    dzoom->cur_center_x = BASE_WIDTH / 2;
    dzoom->cur_center_y = BASE_HEIGHT / 2;
    dzoom->dst_center_x = BASE_WIDTH / 2;
    dzoom->dst_center_y = BASE_HEIGHT / 2;
    dzoom->speed_x = DZOOM_MANUAL_COORDINATE_SPEED;
    dzoom->speed_y = DZOOM_MANUAL_COORDINATE_SPEED;

    dzoom_to_zoom(dzoom->cur_zoom_left, dzoom->cur_zoom_top, dzoom->cur_zoom);
    dzoom_show_osd();

    return SUCCESS;
}

/*打开数字变倍使能*/
int dzoom_enable()
{
    g_run_ptz->dzoom.enable = true;
    dzoom_init();

    return SUCCESS;
}

/*关闭数字变倍使能*/
int dzoom_disable()
{
    g_run_ptz->dzoom.enable = false;
    g_run_ptz->dzoom.mode = DZOOM_MODE_IDLE;
    g_run_ptz->dzoom.cur_zoom = DZOOM_MIN_ZOOM;
    g_run_ptz->dzoom.cur_zoom_left = 0;
    g_run_ptz->dzoom.cur_zoom_top = 0;
    g_run_ptz->dzoom.cur_center_x = BASE_WIDTH / 2;
    g_run_ptz->dzoom.cur_center_y = BASE_HEIGHT / 2;

    dzoom_to_zoom(g_run_ptz->dzoom.cur_zoom_left, g_run_ptz->dzoom.cur_zoom_top, g_run_ptz->dzoom.cur_zoom);
    dzoom_osd_disable();
    dzoom_idle();

    return SUCCESS;
}

int dzoom_loop()
{
    DigitarZoom *dzoom = &g_run_ptz->dzoom;
    if (!is_dzoom_enable()) {
        return 0;
    }
    // 数字变倍只有调预置位一种模式，手动变倍也是一种调预置位的情况，只是目标是最大或者最小倍数
    if (dzoom->mode != DZOOM_MODE_IDLE) {
        if (dzoom->cur_zoom == dzoom->dst_zoom
                && dzoom->cur_center_x == dzoom->dst_center_x
                    && dzoom->cur_center_y == dzoom->dst_center_y) {
            dzoom_idle(); // 达到目标位置，置为空闲
        } else {
            // 处理变倍运动
            int is_dzoom_in = 0; // 放大
            int is_dzoom_out = 0; // 缩小
            if (dzoom->cur_zoom < dzoom->dst_zoom) { // 向高倍数移动
                dzoom->cur_zoom = MIN(dzoom->cur_zoom + dzoom->speed_zoom, dzoom->dst_zoom);
                is_dzoom_in = 1;
            } else if (dzoom->cur_zoom > dzoom->dst_zoom) { // 向低倍移动
                dzoom->cur_zoom = MAX(dzoom->cur_zoom - dzoom->speed_zoom, dzoom->dst_zoom);
                is_dzoom_out = 1;
            }

            int dzoom_width = dzoom_get_zoom_width(dzoom->cur_zoom);
            int dzoom_height = dzoom_get_zoom_height(dzoom->cur_zoom);

            if (is_dzoom_in) { // 放大
                // 以目标中心点加上目标裁剪宽高，计算起始点
                int dst_left = dzoom->dst_center_x - dzoom_width / 2;
                int dst_top = dzoom->dst_center_y - dzoom_height / 2;
                // 范围校验
                dst_left = RANGE(dst_left, 0, BASE_WIDTH - dzoom_width);
                dst_top = RANGE(dst_top, 0, BASE_HEIGHT - dzoom_height);

                int x_step_max = (BASE_WIDTH - BASE_WIDTH * 100.0 / DZOOM_MAX_ZOOM) / (DZOOM_MAX_ZOOM - DZOOM_MIN_ZOOM) * dzoom->speed_zoom;
                int y_step_max = (BASE_HEIGHT - BASE_HEIGHT * 100.0 / DZOOM_MAX_ZOOM) / (DZOOM_MAX_ZOOM - DZOOM_MIN_ZOOM) * dzoom->speed_zoom;

                // 限制一下，一次运动范围不能过大
                if (abs(dzoom->cur_zoom_left - dst_left) > x_step_max) {
                    if (dzoom->cur_zoom_left > dst_left) {
                        dzoom->cur_zoom_left -= x_step_max;
                    } else {
                        dzoom->cur_zoom_left += x_step_max;
                    }
                } else {
                    dzoom->cur_zoom_left = dst_left;
                }

                if (abs(dzoom->cur_zoom_top - dst_top) > y_step_max) {
                    if (dzoom->cur_zoom_top > dst_top) {
                        dzoom->cur_zoom_top -= y_step_max;
                    } else {
                        dzoom->cur_zoom_top += y_step_max;
                    }
                } else {
                    dzoom->cur_zoom_top = dst_top;
                }
            } else if (is_dzoom_out) { // 缩小，以当前中心点为坐标进行，缩小本来视野就会变大，坐标没必要再移动，跟着放大进行偏移就行
                // 以目标中心点加上目标裁剪宽高，计算起始点
                int dst_left = dzoom->cur_center_x - dzoom_width / 2;
                int dst_top = dzoom->cur_center_y - dzoom_height / 2;
                // 范围校验
                dzoom->cur_zoom_left  = RANGE(dst_left, 0, BASE_WIDTH - dzoom_width);
                dzoom->cur_zoom_top = RANGE(dst_top, 0, BASE_HEIGHT - dzoom_height);
            } else { // 坐标平移
                int dst_zoom_left = dzoom->dst_center_x - dzoom_width / 2;
                int dst_zoom_top = dzoom->dst_center_y - dzoom_height / 2;

                if (dzoom->cur_zoom_left < dst_zoom_left) {
                    dzoom->cur_zoom_left = MIN(dzoom->cur_zoom_left + dzoom->speed_x, dst_zoom_left);
                    dzoom->cur_zoom_left = MIN(dzoom->cur_zoom_left, BASE_WIDTH - dzoom_width);
                } else if (dzoom->cur_zoom_left > dst_zoom_left) {
                    dzoom->cur_zoom_left = MAX(dzoom->cur_zoom_left - dzoom->speed_x, dst_zoom_left);
                    dzoom->cur_zoom_left = MAX(dzoom->cur_zoom_left, 0);
                }

                if (dzoom->cur_zoom_top < dst_zoom_top) {
                    dzoom->cur_zoom_top = MIN(dzoom->cur_zoom_top + dzoom->speed_y, dst_zoom_top);
                    dzoom->cur_zoom_top = MIN(dzoom->cur_zoom_top, BASE_HEIGHT - dzoom_height);
                } else if (dzoom->cur_zoom_top > dst_zoom_top) {
                    dzoom->cur_zoom_top = MAX(dzoom->cur_zoom_top - dzoom->speed_y, dst_zoom_top);
                    dzoom->cur_zoom_top = MAX(dzoom->cur_zoom_top, 0);
                }
            }
            dzoom->cur_center_x = dzoom->cur_zoom_left + dzoom_width / 2;
            dzoom->cur_center_y = dzoom->cur_zoom_top + dzoom_height / 2;
            dzoom_to_zoom((int)dzoom->cur_zoom_left, (int)dzoom->cur_zoom_top, dzoom->cur_zoom);

            if (is_dzoom_in || is_dzoom_out) {
                dzoom_show_osd();
            }
        }
    }
    return SUCCESS;
}

/**
 * @brief 设置数字变倍位置调用，会进行范围校验
 *
 * @param mode 运行模式，分为手动控制，跟踪，呼叫预置位
 * @param dst_zoom 目标倍数
 * @param dst_center_x 目标裁剪中心 x 坐标，基于 1080 坐标系
 * @param dst_center_y 目标裁剪中心 y 坐标，基于 1080 坐标系
 */
void dzoom_set_dst(int mode, int dst_zoom, int dst_center_x, int dst_center_y)
{
    dst_zoom = RANGE(dst_zoom, DZOOM_MIN_ZOOM, DZOOM_MAX_ZOOM);
    int dzoom_width = dzoom_get_zoom_width(dst_zoom);
    int dzoom_height = dzoom_get_zoom_height(dst_zoom);
    dst_center_x = RANGE(dst_center_x, dzoom_width / 2, BASE_WIDTH - dzoom_width / 2);
    dst_center_y = RANGE(dst_center_y, dzoom_height / 2, BASE_HEIGHT - dzoom_height / 2);

    if (dzoom_get_mode() == DZOOM_MODE_IDLE || dzoom_get_mode() == mode) {
        // 如果当前模式和目标模式相同或者是当前为空闲模式，需要判断位置是否变化
        if (g_run_ptz->dzoom.dst_zoom == dst_zoom
                && g_run_ptz->dzoom.dst_center_x == dst_center_x
                    && g_run_ptz->dzoom.dst_center_y == dst_center_y) {
            return ; // 目标值没变，不需要进行变更
        }
    }
    g_run_ptz->dzoom.mode = mode;
    g_run_ptz->dzoom.dst_zoom = dst_zoom;

    g_run_ptz->dzoom.dst_center_x = dst_center_x;
    g_run_ptz->dzoom.dst_center_y = dst_center_y;

    return ;
}


void dzoom_set_speed(int speed_zoom, float speed_x, float speed_y)
{
    g_run_ptz->dzoom.speed_zoom = RANGE(speed_zoom, DZOOM_SPEED_ZOOM_MIN, DZOOM_SPEED_ZOOM_MAX);
    g_run_ptz->dzoom.speed_x = RANGE(speed_x, DZOOM_SPEED_COORDINATE_MIN, DZOOM_SPEED_COORDINATE_MAX);
    g_run_ptz->dzoom.speed_y = RANGE(speed_y, DZOOM_SPEED_COORDINATE_MIN, DZOOM_SPEED_COORDINATE_MAX);
}

/*按照设定的变倍速度，运行到 dst_zoom 倍数*/
int dzoom_call_preset(int dst_zoom)
{
    if (!is_dzoom_enable()) {
        return FAILURE;
    }

    dst_zoom = RANGE(dst_zoom, DZOOM_MIN_ZOOM, DZOOM_MAX_ZOOM);

    dzoom_set_dst(DZOOM_MODE_MANUAL, dst_zoom, BASE_WIDTH / 2, BASE_HEIGHT / 2);
    dzoom_set_speed(DZOOM_MANUAL_ZOOM_SPEED, DZOOM_MANUAL_COORDINATE_SPEED, DZOOM_MANUAL_COORDINATE_SPEED);
    return FAILURE;
}
/*获取当前倍数，实际倍数放大 100 倍的结果*/
int dzoom_get_cur_zoom()
{
    return g_run_ptz->dzoom.cur_zoom;
}
/*获取当前裁剪的 x 坐标，基于原画面的坐标*/
int dzoom_get_cur_zoom_left()
{
    return g_run_ptz->dzoom.cur_zoom_left;
}
/*获取当前裁剪的 y 坐标，基于原画面的坐标*/
int dzoom_get_cur_zoom_top()
{
    return g_run_ptz->dzoom.cur_zoom_top;
}
/*获取当前裁剪的中心点 x 坐标*/
int dzoom_get_cur_center_x()
{
    return g_run_ptz->dzoom.cur_center_x;
}
/*获取当前裁剪的中心点 y 坐标*/
int dzoom_get_cur_center_y()
{
    return g_run_ptz->dzoom.cur_center_y;
}
/*获取目标倍数，实际倍数放大 100 倍的结果*/
int dzoom_get_dst_zoom()
{
    return g_run_ptz->dzoom.dst_zoom;
}
/*获取目标裁剪的 x 坐标，基于原画面的坐标*/
int dzoom_get_dst_center_x()
{
    return g_run_ptz->dzoom.dst_center_x;
}
/*获取目标裁剪的 y 坐标，基于原画面的坐标*/
int dzoom_get_dst_center_y()
{
    return g_run_ptz->dzoom.dst_center_y;
}

#endif
/*自适应速度调整*/
int adaptive_speed_adjustment(int speed)
{
    int max_speed = MOTOR_MAX_SPEED;
    /*高倍数下，马达速度过快会导致图像模糊，拖影等，需要根据倍数调整一个合适的值*/
#if defined(DIGITAL_ZOOM)
    /*当前数字变倍是 1.0~2.0，用最大马达速度除以当前倍数，获取当前倍数下的最大马达速度*/
    float cur_zoom = dzoom_get_cur_zoom() / 100.0;
    max_speed /= cur_zoom;
    if (max_speed <= 0) {
        max_speed = 1;
    }
#endif
    if (speed > max_speed) {
        speed = max_speed;
    }

    return speed;
}

/*获取数字变倍是否空闲*/
bool dzoom_is_idle()
{
#if defined(DIGITAL_ZOOM)
    if (!is_dzoom_enable()) {
        return true;
    }
    return g_run_ptz->dzoom.mode == DZOOM_MODE_IDLE;
#else
    return true;
#endif
}

/*****************************************************************/
/* 预置位相关操作                                          */
/*****************************************************************/
/*判断指定预置位是否有效*/
bool is_preset_vaild(int no)
{
    if (no < 0 || no >= PRESET_MAX) {
        ERR("preset no error");
        return false;
    }

    return g_run_ptz->ptz_preset.preset[no].flag == PRESET_VAILD;
}

/*判断跟踪看守位是否有效*/
bool is_follow_preset_vaild(int no)
{
    if (no < PRESET_FOLLOW_PRESET_MIN || no > PRESET_FOLLOW_PRESET_MAX) {
        return false;
    }

    return g_run_ptz->ptz_preset.preset[no].flag == PRESET_VAILD;
}

int set_preset(int no)
{
    if (no < 0 || no >= PRESET_MAX) {
        ERR("preset no[%d] error, scope 0~%d\n", no, PRESET_MAX - 1);
        return FAILURE;
    }
    Preset *preset = &g_run_ptz->ptz_preset.preset[no];

    MotorStatus status;
    get_motor_status(&status);
    preset->flag = PRESET_VAILD;
    for (int i = 0; i < MAX_MOTOR_NUM; ++i) {
        preset->steps[i] = status.steps[i];
    }

    if (PRESET_VIDEO_MASK == no) {
        pri_vidmask(LVL_DBG, "set videomask step: h: %d, v: %d\n",
                    preset->steps[PAN_MOTOR], preset->steps[TILT_MOTOR]);
    }

#if defined(DIGITAL_ZOOM)
    preset->dzoom_pos = (uint16_t)dzoom_get_cur_zoom();
    if (no == PRESET_ZOOM_OSD) {
        dzoom_osd_enable();
    } else if (no == PRESET_ENABLE_DZOOM) {
        dzoom_enable();
    }
#endif
    ptz_write_preset_file();

    return SUCCESS;
}

int delete_preset(int no)
{
    if (no < 0 || no >= PRESET_MAX) {
        ERR("preset no[%d] error, scope 0~%d\n", no, PRESET_MAX - 1);
        return FAILURE;
    }
#if defined(DIGITAL_ZOOM)
    if (no == PRESET_ZOOM_OSD) {
        dzoom_osd_disable();
    } else if (no == PRESET_ENABLE_DZOOM) {
        dzoom_disable();
    }
#endif
    Preset *preset = &g_run_ptz->ptz_preset.preset[no];
    memset(preset, 0, sizeof(Preset));
    ptz_write_preset_file();

    return SUCCESS;
}

int call_preset(int no)
{
    // 目前允许客户使用的预置位数量有限制
    if (no < 0 || no >= PRESET_MAX) {
        ERR("preset no[%d] error, scope 0~%d\n", no, PRESET_MAX - 1);
        return FAILURE;
    }
    if (g_run_ptz->ptz_preset.preset[no].flag == PRESET_VAILD) {
        call_motor_preset(&g_run_ptz->ptz_preset.preset[no], MOTOR_MAX_SPEED);
#if defined(DIGITAL_ZOOM)
        dzoom_call_preset(g_run_ptz->ptz_preset.preset[no].dzoom_pos);
#endif
#if defined(OPTICS_ZOOM)
        // 光学变倍
#endif

        //隐私遮挡关闭后，恢复上次的状态
        if (PRESET_VIDEO_MASK == no &&
            PD_SEQUENCE == g_run_ptz->ptz_preset.pd_func) {
            start_sequence(0);
        }

        return SUCCESS;
    }

    return FAILURE;
}

Preset *get_preset(int no)
{
    if (no < 0 || no >= PRESET_MAX) {
        ERR("preset no[%d] error, scope 0~%d\n", no, MAX_USER_PRESET - 1);
        return NULL;
    }

    return &g_run_ptz->ptz_preset.preset[no];
}

/*拷贝预置位 no1 到 no2*/
int copy_preset(int no1, int no2)
{
    if (no1 < 0 || no1 >= PRESET_MAX) {
        ERR("preset no[%d] error, scope 0~%d\n", no1, MAX_USER_PRESET - 1);
        return FAILURE;
    }

    if (no2 < 0 || no2 >= PRESET_MAX) {
        ERR("preset no[%d] error, scope 0~%d\n", no2, MAX_USER_PRESET - 1);
        return FAILURE;
    }

    memcpy(get_preset(no2), get_preset(no1), sizeof(Preset));
    ptz_write_preset_file();
    
    return SUCCESS;
}
/*****************************************************************/
/* 巡航                                                     */
/*****************************************************************/
int start_sequence(int64_t timestamp)
{
    SequenceCfg *sequence = &g_run_ptz->sequence;

    // 首先发现一个可用的预置位
    sequence->cur_preset_no = -1;
    int vaild_preset_count = 0;
    for (int i = 1; i <= MAX_SEQUENCE_STEP; ++i) {
        if (is_preset_vaild(i)) {
            if (sequence->cur_preset_no == -1) {
                // 取第一个有效位置
                sequence->cur_preset_no = i;
            }
            vaild_preset_count++;
        }
    }

    if (vaild_preset_count > 1) {
        // 大于一个有效位置才开始巡航
        sequence->enable = true;
        call_preset(sequence->cur_preset_no);
        ms_clock_is_timeup(&sequence->wait_time, 0); // 刷新定时器
        ms_clock_is_timeup(&sequence->sequence_time, 0);

        g_run_ptz->ptz_preset.pd_func = PD_SEQUENCE;
        ptz_write_preset_file();
        add_pelco_response(timestamp, SUCCESS, NULL);
        return SUCCESS;
    }

    add_pelco_response(timestamp, FAILURE, "not enough presets available!");
    return FAILURE;
}

/*
 * 停止巡航后回到预置位列表里的第一个预置位
 */
int stop_sequence(PelcoCmd *cmd)
{
    SequenceCfg *sequence = &g_run_ptz->sequence;
    if (sequence->enable) {
        sequence->enable = false;
        for (int i = 1; i <= MAX_SEQUENCE_STEP; ++i) {
            if (is_preset_vaild(i)) {
                call_preset(i);
                break;
            }
        }

        if (NULL != cmd) {
            if (!cmd->skip_pd_save) {
                g_run_ptz->ptz_preset.pd_func = PD_POSITION;// 掉电函数从掉电巡航恢复变更为掉电位置恢复
            }
        } else {
            g_run_ptz->ptz_preset.pd_func = PD_POSITION;// 掉电函数从掉电巡航恢复变更为掉电位置恢复
        }
        set_preset(PRESET_PD_POSITON);
    }

    return SUCCESS;
}

void loop_sequence()
{
    static int tik = 0;
    SequenceCfg *sequence = &g_run_ptz->sequence;

    if (++tik < 1000 / PTZ_LOOP_TIME) {
        // 巡航时间以秒为单位，一秒进来一次即可
        return ;
    }
    tik = 0;

    if (!sequence->enable) {
        return ;
    }

    if (ms_clock_is_timeup(&sequence->sequence_time, g_cfg_ptz->motor_cfg.SeqTimes * 60 * 1000)) {
        // 退出巡航
        DBG("timeout exit sequence\n");
        stop_sequence(NULL);
        return ;
    }

    if (ms_clock_is_timeup(&sequence->wait_time, g_cfg_ptz->motor_cfg.WaitTime * 1000)) {
        // 切换下一个预置位
        int next_preset = sequence->cur_preset_no + 1;
        for (int i = 0; i < MAX_SEQUENCE_STEP; ++i, ++next_preset) {
            if (next_preset > MAX_SEQUENCE_STEP) {
                next_preset = 1;
            }
            if (is_preset_vaild(next_preset)) {
                break;
            }
        }

        if (next_preset == sequence->cur_preset_no) {
            // 找不到下一个位置，巡航结束
            ERR("sequence not find vaild preset, exit\n");
            stop_sequence(NULL);
            return ;
        }

        sequence->cur_preset_no = next_preset;
        call_preset(sequence->cur_preset_no);
    }

    return ;
}

/*返回巡航剩余时间，单位:秒 最小为 0*/
time_t sequence_time_left()
{
    time_t time_left;
    SequenceCfg *sequence = &g_run_ptz->sequence;
    if (sequence_is_enable()) {
        time_left = g_cfg_ptz->motor_cfg.SeqTimes * 60 - sec_since_previous(&sequence->sequence_time);
    } else {
        time_left = 0;
    }

    return RANGE(time_left, 0, g_cfg_ptz->motor_cfg.SeqTimes * 60);
}

bool sequence_is_enable()
{
    return g_run_ptz->sequence.enable;
}

bool pan_scan_is_enable()
{
    return g_run_ptz->panoramic_scan.pan_scan_enable;
}

bool tilt_scan_is_enable()
{
    return g_run_ptz->panoramic_scan.tilt_scan_enable;
}

/*****************************************************************/
/* 移动跟踪                                                       */
/*****************************************************************/
int follow_call_position(FollowPreset *f_preset)
{
    //call_motor_preset(&f_preset->preset, adaptive_speed_adjustment(f_preset->speed));
#if defined(DIGITAL_ZOOM)
    if (g_cfg_ptz->follow_cfg.zoom) {
        dzoom_call_preset(f_preset->preset.dzoom_pos);
    }
#endif
    return SUCCESS;
}

bool follow_is_enable()
{
    return g_cfg_ptz->follow_cfg.enable;
}

/*调用跟踪看守位*/
int follow_call_guard()
{
    DBG("%s preset:%d\n", __func__, g_cfg_ptz->follow_cfg.preset);
    if (is_follow_preset_vaild(g_cfg_ptz->follow_cfg.preset)) {
        call_preset(g_cfg_ptz->follow_cfg.preset);
    } else {
        call_preset(PRESET_FOLLOW_DEFAULT_POS);
    }

    return SUCCESS;
}

/*刷新跟踪看守定时*/
void refresh_follow_guard_timer()
{
    ms_clock_reset(&g_run_ptz->follow.guard_time);
}

void set_follow_status(FollowStatus_t status)
{
    g_run_ptz->follow.status = status;
    return ;
}

FollowStatus_t get_follow_status()
{
    return g_run_ptz->follow.status;
}

int follow_pelco_handle()
{
    //非跟踪模式全局看守卫开启也得刷新状态，这里无需进行跟踪开启判断
    if (follow_is_enable() || g_cfg_ptz->follow_cfg.preset) {
        set_follow_status(FOLLOW_MANUAL); // 设置跟踪为手动状态
    }

    if (!follow_cmd_is_empty()) {
        follow_cmd_clear();
    }

    refresh_follow_guard_timer(); // 刷新跟踪看守定时

    return 0;
}

bool is_follow_guard_timer_timeup()
{
    FollowParam *follow = &g_run_ptz->follow;
    follow_info_t *follow_cfg = &g_cfg_ptz->follow_cfg;
    if (follow_is_enable()) {
        if (follow->status != FOLLOW_IDEL) {
            if (ms_clock_is_timeup(&follow->guard_time, follow_cfg->idle * 1000)) {
                return true;
            }
        }
    } else {
        if (follow_cfg->preset && !ptz_is_run()) {
            if (ms_clock_is_timeup(&follow->guard_time, follow_cfg->idle * 1000)) {
                return true;
            }
        }
    }

    return false;
}

int loop_follow()
{
    FollowPreset cmd = {0};
    if (follow_cmd_dequeue(&cmd)) {
        follow_call_position(&cmd);
        refresh_follow_guard_timer();
        set_follow_status(FOLLOW_RUN);
    } else if (is_follow_guard_timer_timeup()) {
        /*调用看守位，停止定时器*/
        follow_call_guard();
        set_follow_status(FOLLOW_IDEL);
    }

    return SUCCESS;
}

int ptz_follow_handing(int tik, int trkid, int x, int y, int width, int height)
{
    /*
     *  跟踪的定义：远离中心点，or边界游荡
     */
    static int64_t ptz_stop_time = 0;

    if (!g_run_ptz->is_init) {
        return FAILURE;
    }

    /*获取当前位置*/
    MotorStatus status;
    get_motor_status(&status);

    if (get_follow_status() != FOLLOW_RUN) {
        /*云台非处于跟踪状态下，属于手动控制，云台转动不允许跟踪*/
        if (ptz_is_run2(&status) || !dzoom_is_idle()) {
            ptz_stop_time = mono_msec();
            return FAILURE;
        } else if (mono_msec() - ptz_stop_time < 500) {
            /*云台停止后等待 500ms 再进行跟踪，因为手动控制一停就跟踪效果不好*/
            return FAILURE;
        }
    }

    add_item(tik, trkid, &status, x, y, width, height);

    FollowPreset f_preset = { {0} };

    if (is_leaving_center(g_cfg_ptz, g_run_ptz, &f_preset)) {
        follow_cmd_enqueue(&f_preset);  // 在 loop_follow() 中设置状态，但不执行 PRESET 操作

        call_motor_preset(&f_preset.preset, f_preset.speed);
    }

    return SUCCESS;
}

/*****************************************************************/
/* 人形居中                                                       */
/*****************************************************************/
bool person_center_is_enable()
{
    return g_run_ptz->person_center.enable;
}

FollowStatus_t get_person_center_status()
{
    return g_run_ptz->person_center.status;
}

void set_person_center_status(FollowStatus_t status)
{
    g_run_ptz->person_center.status = status;
    return ;
}

inline void set_target_info(FollowTargetInfo *info, int x, int y, int width, int height, int64_t timestamp)
{
    info->x = x;
    info->y = y;
    info->width = width;
    info->height = height;

    info->timestamp = timestamp;
}

int ptz_person_center_handing(int x, int y, int width, int height, int track_id)
{
    static PersonCenterTrackInfo track_info = {0};
    static int64_t ptz_stop_time = 0;

    if (get_person_center_status() != FOLLOW_RUN) {
        /*云台非处于跟踪状态下，属于手动控制，云台转动不允许跟踪*/
        if (ptz_is_run() || !dzoom_is_idle()) {
            ptz_stop_time = mono_msec();
            return FAILURE;
        } else if (mono_msec() - ptz_stop_time < 1000) {
            /*云台停止后等待一段时间再进行跟踪，因为手动控制一停就跟踪效果不好*/
            return FAILURE;
        }
    }

    //PTZ_INFO("x:%d y:%d w:%d h:%d cropt_x:%d cropt_y:%d\n", x, y, width, height, dzoom_get_cur_zoom_left(), dzoom_get_cur_zoom_top());
    int cur_dzoom = dzoom_get_cur_zoom();
    int dst_dzoom = dzoom_get_dst_zoom();

    // 还原宽高和坐标
    int raw_x = dzoom_get_cur_zoom_left() + x * 100 / cur_dzoom;
    int raw_y = dzoom_get_cur_zoom_top() + y * 100 / cur_dzoom;
    int raw_width = width * 100 / cur_dzoom;
    int raw_height = height * 100 / cur_dzoom;

    PTZ_INFO("raw_x:%d raw_y:%d raw_w:%d raw:h:%d\n", raw_x, raw_y, raw_width, raw_height);

    int64_t cur_timestamp = mono_msec();
    if (track_info.track_id != track_id) {
        // id 更新，刷新目标位置
        memset(&track_info, 0, sizeof(track_info));
        track_info.average_width = raw_width;
        track_info.track_id = track_id;
        set_target_info(&track_info.last_pos, raw_x, raw_y, raw_width, raw_height, cur_timestamp);
        return SUCCESS;
    }

    // 根据宽度计算目标倍数
    float a = 0.2; // a 越大，响应越快，噪声过滤效果越差
    track_info.average_width = (1 - a) * track_info.average_width + a * raw_width;
    dst_dzoom = LINEAR_MAP(track_info.average_width, PERSON_CENTER_ZOOM_IN, PERSON_CENTER_ZOOM_OUT, DZOOM_MIN_ZOOM, DZOOM_MAX_ZOOM);
    dst_dzoom = RANGE(dst_dzoom, DZOOM_MIN_ZOOM, DZOOM_MAX_ZOOM);
    PTZ_INFO("track_id:%d average_width:%d dst_zoom:%d\n", track_id, track_info.average_width, dst_dzoom);

    do {
        if (track_info.follow_last_pos.timestamp == 0) {
            if (abs(BASE_WIDTH / 2 - raw_x) < BASE_WIDTH * 1.5 / 5) { // 还未跟踪，且位于边缘 1/4 内的，不进行跟踪，需要走近才进行跟踪
                break;
            }
        } else if (cur_timestamp - track_info.follow_last_pos.timestamp > 4 * PTZ_LOOP_TIME) { // 刚刚下发过跟踪指令，给一点时间进行跟踪
            int need_follow = false;
            if (abs(track_info.follow_last_pos.x - raw_x) > raw_width / 2) { // 水平身位过滤
                PTZ_INFO("x out of range, time:%lld\n", cur_timestamp - track_info.follow_last_pos.timestamp);
                need_follow = true;
            }

            if (abs(track_info.follow_last_pos.y - raw_y) > raw_height / 4) { // 垂直身位过滤
                PTZ_INFO("y out of range, time:%lld\n", cur_timestamp - track_info.follow_last_pos.timestamp);
                need_follow = true;
            }

            int zoom_range = LINEAR_MAP(cur_dzoom, 150, DZOOM_MAX_ZOOM, 8, 3);
            zoom_range = RANGE(zoom_range, 3, 8);
            if (abs(dst_dzoom - dzoom_get_dst_zoom()) > zoom_range
                    || (dst_dzoom != dzoom_get_dst_zoom() && (dst_dzoom == DZOOM_MIN_ZOOM || dst_dzoom == DZOOM_MAX_ZOOM))) {
                PTZ_INFO("z  out of range, time:%lld\n", cur_timestamp - track_info.follow_last_pos.timestamp);
                need_follow = true;
            } else {
                dst_dzoom = dzoom_get_dst_zoom();
            }

            if (need_follow) {
                break;
            }
        }

        /* 不跟踪，更新参数后退出 */
        set_target_info(&track_info.last_pos, raw_x, raw_y, raw_width, raw_height, cur_timestamp);
        if (cur_timestamp - track_info.follow_last_pos.timestamp > 1000) {
            /* 如果人一直在画面中，要定时刷新看守，防止回看守位 */
            PersonCenterPreset cmd = {.refresh = true};
            person_center_cmd_enqueue(&cmd);
        }
        return SUCCESS;
    } while (0);

    //PTZ_INFO("track_id:%d raw_x:%d raw_y:%d raw_w:%d raw:h:%d dst_zoom:%d\n", track_id, raw_x, raw_y, raw_width, raw_height, dst_dzoom);

    // 校正中心坐标，不能超过裁剪范围
    int dzoom_width = dzoom_get_zoom_width(dst_dzoom);
    int dzoom_height = dzoom_get_zoom_height(dst_dzoom);
    int dst_center_x = RANGE(raw_x, dzoom_width / 2, BASE_WIDTH - dzoom_width / 2);
    int dst_center_y = RANGE(raw_y, dzoom_height / 2, BASE_HEIGHT - dzoom_height / 2);

    // 计算运动速度
    float speed_x = 0;
    float speed_y = 0;
    int cur_center_x = dzoom_get_cur_center_x();
    int cur_center_y = dzoom_get_cur_center_y();
    int dx = abs(cur_center_x - dst_center_x);
    if (dx > 2 * raw_width) { // 超出一个定范围的按照距离计算速度
        int time_ms = LINEAR_MAP(dx, raw_width, 2 * dzoom_width, 400, 1000); // 根据实际运动的距离，动态给定时间
        speed_x = dx * PTZ_LOOP_TIME * 1.0 / time_ms;
    } else {
        /* 人的行走速度大概是 1.2~1.4m/s, 过滤框的范围是 1/2 大概相当 0.25m 左右，大概时间是 208~178ms 左右*/
        if (cur_timestamp - track_info.follow_last_pos.timestamp < 400) {
            // 最后一次跟踪到现在的时间在一定范围内，表明一直在运动，使用平均速度作为跟踪速度
            speed_x = dx * PTZ_LOOP_TIME * 1.0 / (cur_timestamp - track_info.follow_last_pos.timestamp);
        } else {
            // 距离上次跟踪时间较久，中间可能有停止，使用尾速作为跟踪速度
            speed_x = dx * PTZ_LOOP_TIME * 1.0 / (cur_timestamp - track_info.last_pos.timestamp);
        }
    }

    int dy = abs(cur_center_y - dst_center_y);
    if (dy > raw_height / 2) {
        int time_ms = LINEAR_MAP(dy, raw_height / 2, dzoom_height, 200, 1000); // 根据实际运动的距离，动态给定时间
        speed_y = dy * PTZ_LOOP_TIME * 1.0 / time_ms;
    } else {
        /* 人的行走速度大概是 1.2~1.4m/s, 过滤框的范围是 1/2 大概相当 0.25m 左右，大概时间是 208~178ms 左右*/
        if (cur_timestamp - track_info.follow_last_pos.timestamp < 400) {
            // 最后一次跟踪到现在的时间在一定范围内，表明一直在运动，使用平均速度作为跟踪速度
            speed_y = dy * PTZ_LOOP_TIME * 1.0 / (cur_timestamp - track_info.follow_last_pos.timestamp);
        } else {
            // 距离上次跟踪时间较久，中间可能有停止，使用尾速作为跟踪速度
            speed_y = dy * PTZ_LOOP_TIME * 1.0 / (cur_timestamp - track_info.last_pos.timestamp);
        }
    }

    speed_x = RANGE(speed_x, DZOOM_SPEED_COORDINATE_MIN, DZOOM_SPEED_COORDINATE_MAX);
    speed_y = RANGE(speed_y, DZOOM_SPEED_COORDINATE_MIN, DZOOM_SPEED_COORDINATE_MAX);

    PersonCenterPreset cmd = {0};
    cmd.dst_center_x = raw_x;
    cmd.dst_center_y = raw_y;
    cmd.speed_x = speed_x;
    cmd.speed_y = speed_y;
    cmd.dst_zoom = dst_dzoom;
    person_center_cmd_enqueue(&cmd);
    PTZ_INFO("x:%d y:%d speed_x:%f speed_y:%f zoom:%d\n", raw_x, raw_y, speed_x, speed_y, dst_dzoom);

    set_target_info(&track_info.last_pos, raw_x, raw_y, raw_width, raw_height, cur_timestamp);
    set_target_info(&track_info.follow_last_pos, raw_x, raw_y, raw_width, raw_height, cur_timestamp);

    return SUCCESS;
}

int person_center_call_position(PersonCenterPreset *cmd)
{
#if defined(DIGITAL_ZOOM)
    if (g_run_ptz->dzoom.mode == DZOOM_MODE_IDLE || g_run_ptz->dzoom.mode == DZOOM_MODE_FOLLOW) {
        dzoom_set_dst(DZOOM_MODE_FOLLOW, cmd->dst_zoom, cmd->dst_center_x, cmd->dst_center_y);
        dzoom_set_speed(1, cmd->speed_x, cmd->speed_y);
    }
#endif
    return SUCCESS;
}

/*调用人形居中看守位*/
int person_center_call_guard()
{
#if defined(DIGITAL_ZOOM)
    PersonCenterPreset *preset = &g_run_ptz->person_center.guard_preset;
    dzoom_set_dst(DZOOM_MODE_CALL_GURAD, preset->dst_zoom, preset->dst_center_x, preset->dst_center_y);
    dzoom_set_speed(DZOOM_MANUAL_ZOOM_SPEED, DZOOM_MANUAL_COORDINATE_SPEED, DZOOM_MANUAL_COORDINATE_SPEED);
#endif

    return SUCCESS;
}

/*设置人形居中看守位，手动数字变倍之后需要更新*/
int person_center_save_guard()
{
    if (is_follow_preset_vaild(g_cfg_ptz->follow_cfg.preset)) {
        // 如果全局看守位有效，用全局看守位的倍数作为人形居中的看守倍数
        Preset *preset = get_preset(g_cfg_ptz->follow_cfg.preset);
        g_run_ptz->person_center.guard_preset.dst_center_x = BASE_WIDTH / 2;
        g_run_ptz->person_center.guard_preset.dst_center_y = BASE_HEIGHT / 2;
        g_run_ptz->person_center.guard_preset.dst_zoom = preset->dzoom_pos;
    } else {
        g_run_ptz->person_center.guard_preset.dst_center_x = BASE_WIDTH / 2;
        g_run_ptz->person_center.guard_preset.dst_center_y = BASE_HEIGHT / 2;
        g_run_ptz->person_center.guard_preset.dst_zoom = g_run_ptz->dzoom.cur_zoom;
    }

    return SUCCESS;
}

/*调用人形居中 pelco 指令到来处理，恢复变倍，修改跟踪状态等*/
int person_center_pelco_handle()
{
    if (person_center_is_enable()) {
        set_person_center_status(FOLLOW_MANUAL); // 设置人形居中为手动状态
        //person_center_call_guard();
        if (!person_center_cmd_is_empty()) {
            person_center_cmd_clear();
        }
        refresh_person_center_guard_timer();
    }

    return SUCCESS;
}

bool is_person_center_guard_timer_timeup()
{
    PersonCenter *person_center = &g_run_ptz->person_center;
    if (person_center->status != FOLLOW_IDEL) {
        if (dzoom_is_idle()) {
            if (ms_clock_is_timeup(&person_center->guard_time, 4 * 1000)) {
                return true;
            }
        } else {
            refresh_person_center_guard_timer();
        }
    }

    return false;
}

/*刷新人形居中看守定时*/
void refresh_person_center_guard_timer()
{
    if (person_center_is_enable()) {
        ms_clock_reset(&g_run_ptz->person_center.guard_time);
    }
    return ;
}

int loop_person_center()
{
    PersonCenterPreset cmd = {0};
    if (person_center_cmd_dequeue(&cmd)) {
        if (cmd.refresh != true) {
            person_center_call_position(&cmd);
        }
        refresh_person_center_guard_timer();
        set_person_center_status(FOLLOW_RUN);
    } else if (is_person_center_guard_timer_timeup()) {
        /*调用看守位，停止定时器*/
        person_center_call_guard();
        set_person_center_status(FOLLOW_IDEL);
    }

    return SUCCESS;
}

/*****************************************************************/
/* 全景扫描                                                       */
/*****************************************************************/
int scan_call_position(FollowPreset *f_preset)
{
    call_motor_preset(&f_preset->preset, adaptive_speed_adjustment(f_preset->speed));
#if defined(DIGITAL_ZOOM)
    if (g_cfg_ptz->follow_cfg.zoom) {
        dzoom_call_preset(f_preset->preset.dzoom_pos);
    }
#endif
    return SUCCESS;
}


int set_motor_stop()
{
    MotorMove move = {0};
    for (int i = 0; i < MAX_MOTOR_NUM; ++i) {
        move.dir[i] = DIRECTION_STOP;
    }
    set_motor_move(&move);
    return SUCCESS;
}

int stop_panoramis_scan()
{
    PanoramicScanCfg *scan = &g_run_ptz->panoramic_scan;
    if (scan->pan_scan_enable || scan->tilt_scan_enable) {
        scan->pan_scan_enable = false;
        scan->tilt_scan_enable = false;
        set_motor_stop();
    }

    return SUCCESS;
}

bool panoramis_scan_is_enable()
{
    return g_run_ptz->panoramic_scan.pan_scan_enable || g_run_ptz->panoramic_scan.tilt_scan_enable;
}

/*开始水平扫描*/
int start_pan_scan()
{
    PanoramicScanCfg *scan = &g_run_ptz->panoramic_scan;

    // 水平扫描可以打断垂直扫描
    if (scan->tilt_scan_enable) {
        scan->tilt_scan_enable = false;
    }

    // 保存预置位，开始水平扫描
    set_preset(PRESET_PANORAMIC);

    scan->pan_scan_enable = true;
    // 水平扫描一圈，左右碰边各一次
    scan->pan_scan_count = 2;

    return SUCCESS;
}

/*开始垂直扫描*/
int start_tilt_scan()
{
    PanoramicScanCfg *scan = &g_run_ptz->panoramic_scan;

    // 垂直扫描可以打断水平扫描
    if (scan->pan_scan_enable) {
        scan->pan_scan_enable = false;
    }

    // 保存预置位，开始垂直扫描
    set_preset(PRESET_PANORAMIC);

    scan->tilt_scan_enable = true;
    // 水平扫描三圈，上下各碰边各三次
    scan->tilt_scan_count = 3 * 2;

    return SUCCESS;
}

void loop_panoramis_scan()
{
    static int tik = 0;
    if (++tik < 500 / PTZ_LOOP_TIME) {
        // 全景扫描对间隔要求不高 500ms 判断一次就行
        return ;
    }
    tik = 0;

    if (ptz_is_run()) { // 等待马达停止
        return ;
    }

    PanoramicScanCfg *scan = &g_run_ptz->panoramic_scan;
    if (scan->pan_scan_enable) {
        if (scan->pan_scan_count > 0) {
            if (scan->pan_scan_count % 2 == 0) { // 左转
                FollowPreset f_preset = {0};
                f_preset.preset.steps[PAN_MOTOR] = 0;
                f_preset.preset.steps[TILT_MOTOR] = MOTOR_CUR_STEP;
                f_preset.preset.dzoom_pos = dzoom_get_cur_zoom();
                f_preset.speed = MOTOR_MAX_SPEED;
                scan_call_position(&f_preset);
            } else { // 右转
                FollowPreset f_preset = {0};
                f_preset.preset.steps[PAN_MOTOR] = g_run_ptz->init_param.max_steps[PAN_MOTOR];
                f_preset.preset.steps[TILT_MOTOR] = MOTOR_CUR_STEP;
                f_preset.preset.dzoom_pos = dzoom_get_cur_zoom();
                f_preset.speed = MOTOR_MAX_SPEED;
                scan_call_position(&f_preset);
            }
            scan->pan_scan_count--;
        } else {
            // 调预置位，退出
            scan->pan_scan_enable = 0;
            call_preset(PRESET_PANORAMIC);
        }
    } else if (scan->tilt_scan_enable) {
        if (scan->tilt_scan_count > 0) {
            if (scan->tilt_scan_count % 2 == 0) { // 上转
                FollowPreset f_preset = {0};
                f_preset.preset.steps[PAN_MOTOR] = MOTOR_CUR_STEP;
                f_preset.preset.steps[TILT_MOTOR] = 0;
                f_preset.preset.dzoom_pos = dzoom_get_cur_zoom();
                f_preset.speed = MOTOR_MAX_SPEED;
                scan_call_position(&f_preset);
            } else {
                FollowPreset f_preset = {0};
                f_preset.preset.steps[PAN_MOTOR] = MOTOR_CUR_STEP;
                f_preset.preset.steps[TILT_MOTOR] = g_run_ptz->init_param.max_steps[PAN_MOTOR];
                f_preset.preset.dzoom_pos = dzoom_get_cur_zoom();
                f_preset.speed = MOTOR_MAX_SPEED;
                scan_call_position(&f_preset); // 右转
            }
            scan->tilt_scan_count--;
        } else {
            // 调预置位，退出
            scan->tilt_scan_enable = 0;
            call_preset(PRESET_PANORAMIC);
        }
    }
}

/*****************************************************************/
/*预置位联动设备设置相关                                           */
/*****************************************************************/
/*双光源模式切换，0黑白模式 1全彩模式 2双光源模式*/
static void lamp_switch(int shinemod)
{
    char jcp_result[JCP_MAX_LEN] = {0};

    int mod = abs(shinemod) % 3;

    jcpcmd_sendrecv2(jcp_result, sizeof(jcp_result), "lightextcfg -act set -devtype %d -irswitchmode 2", mod);
    DBG("jcp result:%s\n",jcp_result);

    return;
}

/*视频镜像切换，0正常 1水平镜像 2垂直镜像  3对角镜像*/
static void mirror_switch(int mirror_mod)
{
    char jcp_result[JCP_MAX_LEN] = {0};

    int mod = abs(mirror_mod) % 4;

    jcpcmd_sendrecv2(jcp_result, sizeof(jcp_result), "vicfg -act set -reverse %d", mod);
    DBG("jcp result:%s\n",jcp_result);

    return;
}

/*强光抑制切换，value要切换的强光抑制值，范围1~100*/
static void suppress_set(unsigned int value)
{
    char jcp_result[JCP_MAX_LEN] = {0};

    jcpcmd_sendrecv2(jcp_result, sizeof(jcp_result), "vicfg -act set -suppress %d", value);
    DBG("jcp result:%s\n",jcp_result);

    return;
}

/*dhcp打开关闭，value 0:关闭 1:打开*/
static void dhcp_set(unsigned int value)
{
    char jcp_result[JCP_MAX_LEN] = {0};
    jcpcmd_sendrecv2(jcp_result, sizeof(jcp_result), "ethcfg -act set -ethdhcp %d", value);
    DBG("jcp result:%s\n",jcp_result);

    return;
}
/*****************************************************************/
/* 马达控制业务逻辑                                           */
/*****************************************************************/
int get_pan_motor_position(void)
{
    MotorStatus status;
    get_motor_status(&status);
    return status.steps[PAN_MOTOR];
}

int get_tilt_motor_position(void)
{
    MotorStatus status;
    get_motor_status(&status);
    return status.steps[TILT_MOTOR];
}

void motor_reinit()
{
    /*马达重新初始化*/
    struct cmdstat *ctx = g_run_ptz->ctx;
    if (ctx != NULL) {
        CPY2CMD(CMD_REINIT_PTZ);
    }
    return ;
}
const char *x_scope()
{
    static char x[16] = {0};

    if (x[0] == 0) {
        snprintf(x, sizeof(x), "0~%d", g_run_ptz->init_param.max_steps[PAN_MOTOR]);
    }
    return x;
}
const char *y_scope()
{
    static char y[16] = {0};
    if (y[0] == 0) {
        snprintf(y, sizeof(y), "0~%d", g_run_ptz->init_param.max_steps[TILT_MOTOR]);
    }
    return y;
}

int debug_to_preset(int pan_steps, int tilt_steps)
{
    Preset preset = {0};
    preset.steps[PAN_MOTOR] = pan_steps;
    preset.steps[TILT_MOTOR] = tilt_steps;
    call_motor_preset(&preset, MOTOR_MAX_SPEED);

    return SUCCESS;
}

int handle_power_down_func()
{
    int ret = 0;
    switch (g_run_ptz->ptz_preset.pd_func) {
    case PD_NONE:
        ptz_move_motor(E_MOVE_DOWN, 0, FALSE);
        break;
    case PD_POSITION:
        ret = call_preset(PRESET_PD_POSITON);
        break;
    case PD_PRESET:
        ret = call_preset(g_run_ptz->ptz_preset.pd_preset);
        break;
    case PD_SEQUENCE:
        ret = start_sequence(0);
        break;
    default:
        break;
    }
    return ret;
}
/*水平马达转动翻转*/
Direction_t handle_pan_reverse(Direction_t dir)
{
    /*1=正常;2 =上下反;3 =左右反;其它=全反*/
    /*翻转的要求是线序和我们相同，正常安装(球机一般是倒装，摇头机是正装)，配置设置正常，云台转动正常*/
    /*w42a 的云台线序，3 和 4 需要左右翻转*/
    if (g_cfg_ptz->motor_cfg.reverse == 3 || g_cfg_ptz->motor_cfg.reverse == 4) {
        return (Direction_t)(dir * -1);
    }

    return (Direction_t)dir;
}
/*垂直马达转动翻转*/
Direction_t handle_tilt_reverse(Direction_t dir)
{
    /*w42a 的云台线序，2 和 4 需要上下翻转*/
    if (g_cfg_ptz->motor_cfg.reverse == 2 || g_cfg_ptz->motor_cfg.reverse == 4) {
        return (Direction_t)(dir * -1);
    }

    return (Direction_t)dir;
}

int handle_pelco_type1(PelcoCmd *cmd)
{
    static int64_t last_move_time = 0;
    MotorMove move = {0};
    switch (cmd->cmd) {
    /*cmd 1~8 是水平和垂直马达控制，其中垂直马达使用 data2 作为速度，水平马达使用 data1 作为速度*/
    /*我们的马达一般左上为 0 0 点*/
    case 1: // up
        move.speed[TILT_MOTOR] = cmd->data2;
        move.dir[TILT_MOTOR] = DIRECTION_DOWN;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 2: // down
        move.speed[TILT_MOTOR] = cmd->data2;
        move.dir[TILT_MOTOR] = DIRECTION_UP;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 3: // left
        move.speed[PAN_MOTOR] = cmd->data1;
        move.dir[PAN_MOTOR] = DIRECTION_DOWN;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 4: // right
        move.speed[PAN_MOTOR] = cmd->data1;
        move.dir[PAN_MOTOR] = DIRECTION_UP;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 5: // right up
        move.speed[PAN_MOTOR] = cmd->data1;
        move.dir[PAN_MOTOR] = DIRECTION_UP;
        move.speed[TILT_MOTOR] = cmd->data2;
        move.dir[TILT_MOTOR] = DIRECTION_DOWN;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 6: // right down
        move.speed[PAN_MOTOR] = cmd->data1;
        move.dir[PAN_MOTOR] = DIRECTION_UP;
        move.speed[TILT_MOTOR] = cmd->data2;
        move.dir[TILT_MOTOR] = DIRECTION_UP;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 7: // left up
        move.speed[PAN_MOTOR] = cmd->data1;
        move.dir[PAN_MOTOR] = DIRECTION_DOWN;
        move.speed[TILT_MOTOR] = cmd->data2;
        move.dir[TILT_MOTOR] = DIRECTION_DOWN;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 8: // left down
        move.speed[PAN_MOTOR] = cmd->data1;
        move.dir[PAN_MOTOR] = DIRECTION_DOWN;
        move.speed[TILT_MOTOR] = cmd->data2;
        move.dir[TILT_MOTOR] = DIRECTION_UP;
        last_move_time = mono_msec();
        goto motor_contrl;
    case 9: // 全部停止
        if (mono_msec() - last_move_time < 200) {
            peclo_cmd_enqueue(cmd); // 停止命令和运动命令间隔小于一定时间，将命令重新推回队列，延迟执行，解决快速点击云台运动幅度过小导致看不出来问题
            break;
        } else {
            move.dir[PAN_MOTOR] = DIRECTION_STOP;
            move.dir[TILT_MOTOR] = DIRECTION_STOP;
            dzoom_idle();
        }
        goto motor_contrl;
    /*10~18 属于 变倍 变焦 光圈控制*/
    case 10: // zoom+
        // 变倍加目标是跑到最大倍数
        dzoom_call_preset(DZOOM_MAX_ZOOM);
        break;
    case 11: // zoom-
        // 变倍减目标是跑到最小倍数
        dzoom_call_preset(DZOOM_MIN_ZOOM);
        break;
    case 12: // zoom stop
        // 停止变倍，退出调用预置位模式
        dzoom_idle();
        break;
    case 13: // focus-
        break;
    case 14: // focus+
        break;
    case 15: // focus stop
        break;
    case 16: // iris open
        break;
    case 17: // iris close
        break;
    case 18: // iris stop
        break;
    default:
        ERR("type 1, unkown cmd:%d\n", cmd->cmd);
        break;
    }

    return SUCCESS;
motor_contrl:
    move.dir[TILT_MOTOR] = handle_tilt_reverse(move.dir[TILT_MOTOR]);
    move.dir[PAN_MOTOR] = handle_pan_reverse(move.dir[PAN_MOTOR]);

    move.speed[PAN_MOTOR] = adaptive_speed_adjustment(move.speed[PAN_MOTOR]);
    move.speed[TILT_MOTOR] = adaptive_speed_adjustment(move.speed[TILT_MOTOR]);

    set_motor_move(&move);
    return SUCCESS;
}

int handle_pelco_type2(PelcoCmd *cmd)
{
    switch (cmd->cmd) {
    /*设置 调用 删除 预置位都使用 data2 作为预置位编号*/
    case 1: // 设置预置位
        // 客户设置预置位有范围限制
        if (cmd->data2 > 0 &&
            (cmd->data2 < MAX_USER_PRESET ||
             cmd->data2 == PRESET_ZOOM_OSD ||
             cmd->data2 == PRESET_ENABLE_DZOOM ||
             cmd->data2 == PRESET_VIDEO_MASK)) {
            set_preset(cmd->data2);
        }
        break;
    case 2: // 调用预置位
        if (cmd->data2 == PRESET_PTZ_INIT) {
            ptz_config_init();
        } else if (cmd->data2 == PRESET_PTZ_DEFAULT) {
            ptz_config_default();
        } else if (cmd->data2 == PRESET_LAMP_IR) {
            lamp_switch(0);
        } else if (cmd->data2 == PRESET_LAMP_WHITE) {
            lamp_switch(1);
        } else if (cmd->data2 == PRESET_LAMP_DBL) {
            lamp_switch(2);
        } else if (cmd->data2 == PRESET_MIRROR_NORMAL) {
            mirror_switch(0);
        } else if (cmd->data2 == PRESET_MIRROR_HORIZONTAL) {
            mirror_switch(1);
        } else if (cmd->data2 == PRESET_MIRROR_VERTICAL) {
            mirror_switch(2);
        } else if (cmd->data2 == PRESET_MIRROR_DIAGONAL) {
            mirror_switch(3);
        } else if (cmd->data2 == PRESET_SUPPRESS_FIFTY) {
            suppress_set(50);
        } else if (cmd->data2 == PRESET_SUPPRESS_ONEHUNDRED) {
            suppress_set(100);
        } else if (cmd->data2 == PRESET_OPEN_DHCP) {
            dhcp_set(1);
        } else if (cmd->data2 == PRESET_CLOSE_DHCP) {
            dhcp_set(0);
        } else if (cmd->data2 == PRESET_START_SEQUENCE) {
            start_sequence(cmd->timestamp); // 开始巡航
        } else {
            // 正常预置位调用
            if (call_preset(cmd->data2) == SUCCESS) {
                if (g_run_ptz->ptz_preset.pd_func != PD_PRESET ||
                        g_run_ptz->ptz_preset.pd_preset != (uint32_t)cmd->data2) {
                    // 切换到掉电预置位恢复模式
                    if (!cmd->skip_pd_save) {
                        g_run_ptz->ptz_preset.pd_func = PD_PRESET;
                    }
                    g_run_ptz->ptz_preset.pd_preset = cmd->data2;
                    ptz_write_preset_file();
                }
            }
        }
        break;
    case 3: // 删除预置位
        // 客户设置预置位有范围限制
        if (cmd->data2 > 0 && (cmd->data2 < MAX_USER_PRESET || cmd->data2 == PRESET_ZOOM_OSD || cmd->data2 == PRESET_ENABLE_DZOOM)) {
            delete_preset(cmd->data2);
        }
        break;
    default:
        ERR("type 2, unkown cmd:%d\n", cmd->cmd);
        break;
    }

    return SUCCESS;
}

int handle_pelco_type3(PelcoCmd *cmd)
{
    switch (cmd->cmd) {
    case 1: // 开始巡航
        if (cmd->data2 == 10) { // 启动水平扫描
            start_pan_scan();
        } else if (cmd->data2 == 12) { // 启动垂直扫描
            start_tilt_scan();
        } else if (cmd->data2 == 11 || cmd->data2 == 13) { // 停止扫描
            stop_panoramis_scan();
        } else if (cmd->data2 == 1) {
            //查询巡航状态,jcp中处理,此处不处理
            DBG("inquiry sequence status\n");
        } else {
            start_sequence(cmd->timestamp); // 开始巡航
        }
        break;
    case 2: // 结束巡航
        // 因为任何预置位指令下发都会停止巡航，前面就已经调用了停止巡航，这里就不用调了
        break;
    default:
        ERR("type 3, unkown cmd:%d\n", cmd->cmd);
        break;
    }

    return SUCCESS;
}

int handle_pelco_cmd()
{
    PelcoCmd cmd;

    if (pelco_cmd_dequeue(&cmd)) {
        follow_pelco_handle();
        person_center_pelco_handle(); // 收到 pelco 指令，人形居中进行执行前的处理
        stop_panoramis_scan();
        stop_sequence(&cmd);
        if (cmd.type == 1) {
            // 云台运动，重置空闲保存位置计数器
            g_run_ptz->save_position_tik = 0;
            if (!cmd.skip_pd_save) {
                g_run_ptz->ptz_preset.pd_func = PD_POSITION;
            }
            /*type 1 手动控制云台旋转，变倍变焦*/
            handle_pelco_type1(&cmd);
        } else if (cmd.type == 2) {
            /*type 2 预置位相关*/
            handle_pelco_type2(&cmd);
        } else if (cmd.type == 3) {
            /*type 3 巡航*/
            handle_pelco_type3(&cmd);
        } else {
            ERR("unkown type:%d\n", cmd.type);
        }
    }

    return SUCCESS;
}

bool pd_position_is_change(Preset *pos_preset)
{
    if (pos_preset == NULL) {
        return true;
    }
    MotorStatus status;
    get_motor_status(&status);
    for (int i = 0; i < MAX_MOTOR_NUM; ++i) {
        if (pos_preset->steps[i] != status.steps[i]) {
            return true;
        }
    }

#if defined(DIGITAL_ZOOM)
    if (pos_preset->dzoom_pos != dzoom_get_cur_zoom()) {
        return true;
    }
#endif
    return false;
}

int loop_pd_position()
{
    if (is_follow_guard_timer_timeup()) {
        if (!person_center_is_enable() ||
                (person_center_is_enable() && get_person_center_status() == FOLLOW_IDEL)) {
            /* 调用看守位，停止定时器 */
            follow_call_guard();
            set_follow_status(FOLLOW_IDEL);
        }

    }

    if (g_run_ptz->ptz_preset.pd_func == PD_POSITION) {
        if (++g_run_ptz->save_position_tik > 10 * 1000 / PTZ_LOOP_TIME) {
            g_run_ptz->save_position_tik = 0;

            if (person_center_is_enable() && get_person_center_status() != FOLLOW_IDEL) {
                return FAILURE; // 人形居中的情况下需要停止才保存掉电位置
            }

            Preset *pos_preset = get_preset(PRESET_PD_POSITON);

            if (!ptz_is_run() && pd_position_is_change(pos_preset)) {
                DBG("pd postion is change, save current position\n");
                set_preset(PRESET_PD_POSITON);
            }
        }
    }

    return SUCCESS;
}

/*获取马达是否在运行*/
bool ptz_is_run()
{
    MotorStatus status;
    get_motor_status(&status);

    return ptz_is_run2(&status);
}
bool ptz_is_run2(MotorStatus *status)
{
    int is_run = 0;
    for (int i = 0; i < MAX_MOTOR_NUM; ++i) {
        // 单电机旋转时，只依赖 dir 判断的话，如果正好上一个电机停止，下一个电机还没开始转，会错误的判断为电机停止旋转
        is_run |= status->dir[i] || (status->steps[i] != status->dst_steps[i]);
    }
    return is_run;
}

/*云台配置恢复默认*/
int ptz_config_default()
{
    memset(&g_run_ptz->ptz_preset, 0, sizeof(g_run_ptz->ptz_preset));
#if defined(DIGITAL_ZOOM)
    set_preset(PRESET_ENABLE_DZOOM); // 默认打开数字变倍
    set_preset(PRESET_ZOOM_OSD); // 默认打开数字变倍 osd 显示
#endif
    ptz_write_preset_file();
    #if defined(OPTICS_ZOOM)
    // 呼叫光学变倍恢复默认
    #endif

    return SUCCESS;
}

static int set_motor_cfg()
{
    // MAX_MOTOR_NUM == 2
    g_run_ptz->init_param.edge_steps[PAN_MOTOR] = g_cfg_ptz->motor_cfg.h_Startstep;
    g_run_ptz->init_param.max_steps[PAN_MOTOR] = g_cfg_ptz->motor_cfg.h_maxstep;
    g_run_ptz->init_param.init_dir[PAN_MOTOR] = DIRECTION_DOWN;  // 初始化方向适配老版马达，碰边步数预留在同一边

    g_run_ptz->init_param.edge_steps[TILT_MOTOR] = g_cfg_ptz->motor_cfg.v_Startstep;
    g_run_ptz->init_param.max_steps[TILT_MOTOR] = g_cfg_ptz->motor_cfg.v_maxstep;
    g_run_ptz->init_param.init_dir[TILT_MOTOR] = DIRECTION_DOWN; // 初始化方向适配老版马达，碰边步数预留在同一边

    g_run_ptz->init_param.single_motor_rotation = true; // 默认打开单电机旋转

    set_motor_init_param(&g_run_ptz->init_param);
    get_motor_init_param(&g_run_ptz->init_param);

    return 0;
}


static int ptz_config_init()
{
    if (g_run_ptz->motor_fd < 0) {
        g_run_ptz->motor_fd = open(MOTOR_DEV, O_RDWR);
        if (g_run_ptz->motor_fd < 0) {
            ERR("open motor_dev error %d %s\n", errno, strerror(errno));
            return FAILURE;
        }
    }

    DBG("start init ptz config and motor driver\n");
    g_run_ptz->is_init = false;

    set_motor_cfg();
    motor_start_init();

    int ret = ptz_read_preset_file();
    if (ret == FAILURE && !is_okey(PTZ_PRESET_BIN_FILE)) {
        // 刚烧底包起来是没有这个文件的，可以在这里指定一些默认配置，比如数字变倍使能
#if defined(DIGITAL_ZOOM)
        set_preset(PRESET_ENABLE_DZOOM); // 默认打开数字变倍
        set_preset(PRESET_ZOOM_OSD); // 默认打开数字变倍 osd 显示
#endif
    }

#if defined(DIGITAL_ZOOM)
    dzoom_init();
#endif
    // 等待初始化完成，一分钟超时时间
    for (int i = 0; i < 60; ++i) {
        usleep(1000 * 1000);
        if (!ptz_is_run()) {
            break;
        }
    }

    // 清空指令缓冲区
    pelco_cmd_clear();

    handle_power_down_func();

    follow_cmd_clear();

    /* 如果上电默认存在看守位，需要调用一次看守位 */
    if (follow_is_enable() || g_cfg_ptz->follow_cfg.preset != 0) {
        follow_call_guard();
    }

    person_center_save_guard(); // 云台初始化完成保存当前位置为人形居中看守位

    g_run_ptz->is_init = true;

    return SUCCESS;
}

void loop_ptz(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd) {
        if (cmd & CMD_REINIT_PTZ) {
            int ret = ptz_config_init();
            if (ret == FAILURE) {
               CPY2CMD(CMD_REINIT_PTZ);
               return ;
            }
        }

        if (cmd & CMD_MOTOR_CFG) {
            set_motor_cfg();
        }

        if (cmd & CMD_VIDEO_REPORT) {
            // 分辨率改变，需要刷新数字变倍
            dzoom_to_zoom(dzoom_get_cur_zoom_left(), dzoom_get_cur_zoom_top(), dzoom_get_cur_zoom());
        }

        if (cmd & CMD_FOLLOW_ENABLE) {
            stop_sequence(NULL);  // 停止巡航

            if (is_follow_preset_vaild(g_cfg_ptz->follow_cfg.preset)) {
                /*当前看守位有效，运动到看守位*/
                call_preset(g_cfg_ptz->follow_cfg.preset);

                /*保存当前看守位作为默认看守位，防止中途看守位被删除*/
                copy_preset(g_cfg_ptz->follow_cfg.preset, PRESET_FOLLOW_DEFAULT_POS);
            } else {
                /*看守位无效，保存当前位置作为看守位*/
                set_preset(PRESET_FOLLOW_DEFAULT_POS);
            }
            set_follow_status(FOLLOW_IDEL);
        }

        if (cmd & CMD_FOLLOW_DISABLE || cmd & CMD_FOLLOW_PRESET) {
            /* 退出移动跟踪或设置看守位，调用一次看守位 */
            follow_call_guard();
            set_follow_status(FOLLOW_IDEL);
            if (!follow_cmd_is_empty()) {
                follow_cmd_clear();
            }
        }

        if (cmd & CMD_PERSON_CENTER_ENABLE) {
            g_run_ptz->person_center.enable = true;
            person_center_save_guard(); // 使能的时候保存当前数字变倍位置
        }

        if (cmd & CMD_PERSON_CENTER_DISABLE) {
            g_run_ptz->person_center.enable = false;
            person_center_call_guard(); // 退出的时候调用看守位
            set_person_center_status(FOLLOW_IDEL);
            if (!person_center_cmd_is_empty()) {
                person_center_cmd_clear();
            }
        }
    }

    if (!pelco_cmd_is_empty()) { // pelco 指令
        handle_pelco_cmd();
    } else if (panoramis_scan_is_enable()) { // 全景扫描
        loop_panoramis_scan();
    } else if (follow_is_enable()) { // 移动跟踪
        loop_follow();
    } else if (person_center_is_enable()) { // 人形居中
        loop_person_center();
        loop_pd_position(); // 人形居中需要定时保存掉电位置
    } else if (sequence_is_enable()) { // 巡航
        loop_sequence();
    } else { // 空闲，掉电位置保存
        loop_pd_position();
    }

    dzoom_loop(); // 数字变倍

    return ;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage & CMD_MOTOR_CFG) {
        if (g_cfg_ptz->motor_cfg.h_Startstep != g_raw_ptz->motor_cfg.h_Startstep
                || g_cfg_ptz->motor_cfg.v_Startstep != g_raw_ptz->motor_cfg.v_Startstep
                    || g_cfg_ptz->motor_cfg.h_maxstep != g_raw_ptz->motor_cfg.h_maxstep
                        || g_cfg_ptz->motor_cfg.v_maxstep != g_raw_ptz->motor_cfg.v_maxstep) {
            cmd_set_command(p_cmd, CMD_MOTOR_CFG);
        }
        memcpy(&g_cfg_ptz->motor_cfg, &g_raw_ptz->motor_cfg, sizeof(g_raw_ptz->motor_cfg));
    }

    if (p_cmd->cmd_stage & CMD_FOLLOW_CFG) {
        if (g_cfg_ptz->follow_cfg.enable != g_raw_ptz->follow_cfg.enable) {
            if (g_raw_ptz->follow_cfg.enable) {
                cmd_set_command(p_cmd, CMD_FOLLOW_ENABLE);
            } else {
                cmd_set_command(p_cmd, CMD_FOLLOW_DISABLE);
            }
        }

        if (g_raw_ptz->follow_cfg.preset != 0 &&
            g_cfg_ptz->follow_cfg.preset != g_raw_ptz->follow_cfg.preset) {
            cmd_set_command(p_cmd, CMD_FOLLOW_PRESET);
        }
        memcpy(&g_cfg_ptz->follow_cfg, &g_raw_ptz->follow_cfg, sizeof(g_raw_ptz->follow_cfg));
    }

    if (p_cmd->cmd_stage & CMD_VIDEO_REPORT) {
        cmd_set_command(p_cmd, CMD_VIDEO_REPORT);
    }

    if (p_cmd->cmd_stage & CMD_PERSON_CENTER_ENABLE) {
        cmd_set_command(p_cmd, CMD_PERSON_CENTER_ENABLE);
    }

    if (p_cmd->cmd_stage & CMD_PERSON_CENTER_DISABLE) {
        cmd_set_command(p_cmd, CMD_PERSON_CENTER_DISABLE);
    }

    return ;
}

void cb_motorcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_MOTOR_CFG, &g_raw_ptz->motor_cfg, p_src, size);
}

void cb_followcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_FOLLOW_CFG, &g_raw_ptz->follow_cfg, p_src, size);
}

void cb_hdcfg(int id, void *p_src, int size, void *ctx)
{
    HumanDetectionS *hdinfo = (HumanDetectionS *)p_src;
    if (hdinfo->person_center != g_run_ptz->person_center.enable) {
        if (hdinfo->person_center) {
            CPY2CMD(CMD_PERSON_CENTER_ENABLE);
        } else {
            CPY2CMD(CMD_PERSON_CENTER_DISABLE);
        }
    }
}

void cb_video_report(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_VIDEO_REPORT);
}

static void cb_stop_motor(void *usr_data)
{
	PelcoCmd pelco_ctrl = {0};

    pelco_ctrl.type = 1;
    pelco_ctrl.cmd = E_MOVE_STOP;

    peclo_cmd_enqueue(&pelco_ctrl);
}

void ptz_move_motor(eCmdType1 cmd, int ms_delay_stop, int skip_pd_save)
{
	PelcoCmd pelco_ctrl = {0};

    if (NULL != g_run_ptz->hdl_stop) {
        WAR("last pelco ctrl not finished, ignore this time\n");
        goto exit;
    }

    pelco_ctrl.type = 1;
    pelco_ctrl.cmd = cmd;
    pelco_ctrl.skip_pd_save = skip_pd_save;

    switch (cmd) {
    case E_MOVE_UP:
    case E_MOVE_DOWN: {
        pelco_ctrl.data2 = 32;
        break;
    }
    case E_MOVE_LEFT:
    case E_MOVE_RIGHT: {
        pelco_ctrl.data1 = 32;
        break;
    }
    default: {
        ERR("bad cmd %d\n", cmd);
        goto exit;
    }
    }

    peclo_cmd_enqueue(&pelco_ctrl);

    if (ms_delay_stop > 0) {
        js_create_once(g_run_ptz->hdl_stop, g_run_ptz->sch, ms_delay_stop,
                       cb_stop_motor, NULL);
    }

exit:

    return;
}

void ptz_call_preset(PresetNo_t preset_num, int skip_pd_save)
{
	PelcoCmd pelco_ctrl = {0};

    pelco_ctrl.type = 2;
    pelco_ctrl.cmd = 2;
    pelco_ctrl.data2 = preset_num;
    pelco_ctrl.skip_pd_save = skip_pd_save;

    peclo_cmd_enqueue(&pelco_ctrl);
}

void ptz_save_preset(PresetNo_t preset_num, int skip_pd_save)
{
	PelcoCmd pelco_ctrl = {0};

    pelco_ctrl.type = 2;
    pelco_ctrl.cmd = 1;
    pelco_ctrl.data2 = preset_num;
    pelco_ctrl.skip_pd_save = skip_pd_save;

    peclo_cmd_enqueue(&pelco_ctrl);
}

void ptz_init()
{
    DBG("Pan-Tilt Zoom init\n");
    static struct cmdstat cmdstat_ptz;
    static struct cmdstat *ctx = &cmdstat_ptz;
    cmdstat_ptz.diff_cfg2cmd = diff_cfg2cmd;
    g_run_ptz->ctx = ctx;

    g_run_ptz->sch = js_create_scheduler((char *)"sch_ptz");

    pthread_spin_init(&g_run_ptz->spinlock, PTHREAD_PROCESS_PRIVATE);

    // 初始化人形居中使能
    HumanDetectionS hdinfo = {0};
    conf_get_humandetectioncfg(&hdinfo);
    g_run_ptz->person_center.enable = hdinfo.person_center;

    conf_get_motorcfg(&g_cfg_ptz->motor_cfg);
    conf_get_follow_cfg(&g_cfg_ptz->follow_cfg);

    // 注册事件
    attach_config(JEvent_MotorCfg, cb_motorcfg, (void *)ctx);
    attach_config(JEvent_Followcfg, cb_followcfg, (void *)ctx);
    attach_config(JEvent_HumanDetectCfgChg, cb_hdcfg        , (void *)ctx);

    attach_event(JEvent_DevVideoReport, cb_video_report, (void *)ctx);

    CPY2CMD(CMD_REINIT_PTZ);  // 上电先跑初始化

    js_create_timer_r(g_run_ptz->sch, 50, PTZ_LOOP_TIME, loop_ptz, ctx, &g_run_ptz->hdl_loop);

    return ;
}

void ptz_deinit()
{
    struct cmdstat *ctx = g_run_ptz->ctx;
    if (!g_run_ptz->is_init) {
        DBG("motor is not init\n");
        return;
    }
    g_run_ptz->is_init = false;
    DBG("Pan-Tilt Zoom deinit\n");

    detach_config(JEvent_MotorCfg, cb_motorcfg, (void *)ctx);
    detach_config(JEvent_Followcfg, cb_followcfg, (void *)ctx);
    detach_config(JEvent_HumanDetectCfgChg  , cb_hdcfg       , (void *)ctx);
    detach_event(JEvent_DevVideoReport, cb_video_report, (void *)ctx);

    js_delete_timer_r(&g_run_ptz->hdl_loop);
    if (NULL != g_run_ptz->sch) {
        js_delete_scheduler(g_run_ptz->sch);
        g_run_ptz->sch = NULL;
    }

    if (g_run_ptz->motor_fd >= 0) {
        close(g_run_ptz->motor_fd);
        g_run_ptz->motor_fd = -1;
    }

    return ;
}
