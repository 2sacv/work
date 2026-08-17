/* 
 *       Filename:  pwm.c
 *    Description:  
 *        Version:  1.0
 *        Created:  12/08/2022 09:28:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "debug.h"
#include "system_ctrl.h"
#include "jpwm.h"
#include "g_log.h"

/*/sys/class/pwm/pwmchip0/
  /sys/class/pwm/pwmchip0/pwm0  依据pinmux配置对应红外灯控制口，基本无需做调整
  /sys/class/pwm/pwmchip0/pwm1  依据pinmux配置对应白光灯，暖光灯控制，需要基于软光敏进行自动调节
*/

#define PWM_DEV_PATH        "/sys/class/pwm/pwmchip0/"
#define PWM_CHN_HEAD        "pwm"
#define PWM_DEV_ENABLE      "enable"
#define PWM_DEV_PERIOD      "period"
#define PWM_DEV_EXPORT      "export"
#define PWM_DEV_UNEXPORT    "unexport"
#define PWM_DEV_POLARITY    "polarity"
#define PWM_DEV_DUTY_CYCLE  "duty_cycle"

#define MAX_PATH_LEN    (128)

int pwm_open_export(int chn, int exports)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(TRUE == exports || FALSE == exports, FAILURE);

    int ret = 0, fd = -1;
    char chn_buf[8] = {0};
    char pwm_export[MAX_PATH_LEN] = {0};

    do {
        if (TRUE == exports) {
            snprintf(pwm_export, sizeof(pwm_export) - 1, "%s%s", 
                     PWM_DEV_PATH, PWM_DEV_EXPORT);
        } else {
            snprintf(pwm_export, sizeof(pwm_export) - 1, "%s%s", 
                     PWM_DEV_PATH, PWM_DEV_UNEXPORT);
        }
        dbg_pwm("pwm_export path:%s\n", pwm_export);

        ret = access(pwm_export, W_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_export);
            break;
        }

        fd = open(pwm_export, O_WRONLY);
        if (fd < 0) {
            ERR("open pwm%d exports error!\n", chn);
            ret = FAILURE;
            break;
        }

        snprintf(chn_buf, sizeof(chn_buf) - 1, "%d", chn);
        dbg_pwm("chn:%s\n", chn_buf);

        ret = write(fd, chn_buf, strlen(chn_buf) + 1);
        if (FAILURE == ret) {
            ERR("write pwm%d exports failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("write pwm%d exports failed, nothing was written\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_enable_chn(int chn, int enable)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(TRUE == enable || FALSE == enable, FAILURE);

    int ret = 0, fd = -1;
    char pwm_enable[MAX_PATH_LEN] = {0};
    char enable_buf[8] = {0};

    do {
        snprintf(pwm_enable, sizeof(pwm_enable) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_ENABLE);
        dbg_pwm("pwm_enable path:%s\n", pwm_enable);

        ret = access(pwm_enable, W_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_enable);
            break;
        }

        fd = open(pwm_enable, O_WRONLY);
        if (fd < 0) {
            ERR("open pwm%d enable error!\n", chn);
            ret = FAILURE;
            break;
        }

        snprintf(enable_buf, sizeof(enable_buf) - 1, "%d", enable);
        dbg_pwm("enable:%s\n", enable_buf);

        ret = write(fd, enable_buf, strlen(enable_buf) + 1);
        if (FAILURE == ret) {
            ERR("write pwm%d enable failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("write pwm%d enable failed, nothing was written\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_get_chn_enable(int chn, int *enable)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(NULL != enable, FAILURE);

    int ret = 0, fd = -1;
    char pwm_enable[MAX_PATH_LEN] = {0};
    char enable_buf[8] = {0};

    do {
        snprintf(pwm_enable, sizeof(pwm_enable) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_ENABLE);
        dbg_pwm("pwm_enable path:%s\n", pwm_enable);

        ret = access(pwm_enable, R_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_enable);
            break;
        }

        fd = open(pwm_enable, O_RDONLY);
        if (fd < 0) {
            ERR("open pwm%d enable error!\n", chn);
            ret = FAILURE;
            break;
        }

        do {
            ret = read(fd, enable_buf, sizeof(enable_buf) - 1);
        } while(FAILURE == ret && (EINTR == errno || EAGAIN == errno));

        if (FAILURE == ret) {
            ERR("read pwm%d enable failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("read end of pwm%d enable, nothing was read\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }

        sscanf(enable_buf, "%d", enable);
        dbg_pwm("read enable:%d\n", *enable);
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_set_period(int chn, int period)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(period >= 0, FAILURE);

    int ret = 0, fd = -1, duty_cycle = 0;
    char pwm_period[MAX_PATH_LEN] = {0};
    char period_buf[8] = {0};

    do {
        ret = pwm_get_duty_cycle(chn, &duty_cycle);
        if (FAILURE == ret) {
            ERR("pwm get duty cycle failed\n");
            ret = FAILURE;
            break;
        } else if (period < duty_cycle) {
            ERR("period is smaller than dutycycle, invalid argument\n");
            ret = FAILURE;
            break;
        }

        snprintf(pwm_period, sizeof(pwm_period) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_PERIOD);
        dbg_pwm("pwm_period path:%s\n", pwm_period);

        ret = access(pwm_period, W_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_period);
            break;
        }

        fd = open(pwm_period, O_WRONLY);
        if (fd < 0) {
            ERR("open pwm%d period error!\n", chn);
            ret = FAILURE;
            break;
        }

        snprintf(period_buf, sizeof(period_buf) - 1, "%d", period);
        dbg_pwm("period:%s\n", period_buf);

        ret = write(fd, period_buf, strlen(period_buf) + 1);
        if (FAILURE == ret) {
            ERR("write pwm%d period failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("write pwm%d period failed, nothing was written\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_get_period(int chn, int *period)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(NULL != period, FAILURE);

    int ret = 0, fd = -1;
    char pwm_period[MAX_PATH_LEN] = {0};
    char period_buf[8] = {0};

    do {
        snprintf(pwm_period, sizeof(pwm_period) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_PERIOD);
        dbg_pwm("pwm_period path:%s\n", pwm_period);

        ret = access(pwm_period, R_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_period);
            break;
        }

        fd = open(pwm_period, O_RDONLY);
        if (fd < 0) {
            ERR("open pwm%d period error!\n", chn);
            ret = FAILURE;
            break;
        }

        do {
            ret = read(fd, period_buf, sizeof(period_buf) - 1);
        } while(FAILURE == ret && (EINTR == errno || EAGAIN == errno));

        if (FAILURE == ret) {
            ERR("read pwm%d period failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("read end of pwm%d period, nothing was read\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }

        sscanf(period_buf, "%d", period);
        dbg_pwm("read period:%d\n", *period);
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_set_polarity(int chn, const char *polarity)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT && NULL != polarity, FAILURE);
    return_val_if_fail(0 == strncmp(polarity, POLARITY_NORMAL, strlen(polarity)) || 
                       0 == strncmp(polarity, POLARITY_INVERSED, strlen(polarity)), FAILURE);

    int ret = 0, fd = -1;
    char pwm_polarity[MAX_PATH_LEN] = {0};

    do {
        snprintf(pwm_polarity, sizeof(pwm_polarity) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_POLARITY);
        dbg_pwm("pwm_polarity path:%s\n", pwm_polarity);

        ret = access(pwm_polarity, W_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_polarity);
            break;
        }

        fd = open(pwm_polarity, O_WRONLY);
        if (fd < 0) {
            ERR("open pwm%d polarity error!\n", chn);
            ret = FAILURE;
            break;
        }

        ret = write(fd, polarity, strlen(polarity) + 1);
        if (FAILURE == ret) {
            ERR("write pwm%d polarity failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("write pwm%d polarity failed, nothing was written\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_get_polarity(int chn, char *polarity, int size)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(NULL != polarity, FAILURE);
    return_val_if_fail(size > strlen(POLARITY_INVERSED), FAILURE);

    int ret = 0, fd = -1;
    char pwm_polarity[MAX_PATH_LEN] = {0};
    char polarity_buf[16] = {0};

    do {
        snprintf(pwm_polarity, sizeof(pwm_polarity) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_POLARITY);
        dbg_pwm("pwm_polarity path:%s\n", pwm_polarity);

        ret = access(pwm_polarity, R_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_polarity);
            break;
        }

        fd = open(pwm_polarity, O_RDONLY);
        if (fd < 0) {
            ERR("open pwm%d polarity error!\n", chn);
            ret = FAILURE;
            break;
        }

        do {
            ret = read(fd, polarity_buf, sizeof(polarity_buf) - 1);
        } while(FAILURE == ret && (EINTR == errno || EAGAIN == errno));

        if (FAILURE == ret) {
            ERR("read pwm%d polarity failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("read end of pwm%d polarity, nothing was read\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }

        sscanf(polarity_buf, "%s", polarity);
        DBG("read polarity:%s\n", polarity);
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_set_duty_cycle(int chn, int duty_cycle)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(duty_cycle >= 0, FAILURE);

    int ret = 0, fd = -1, period = 0, chn_enabled = 0;
    char pwm_dutycycle[MAX_PATH_LEN] = {0};
    char dutycycle_buf[8] = {0};

    do {
        ret = pwm_get_period(chn, &period);
        if (FAILURE == ret) {
            ERR("pwm get period failed\n");
            break;
        } else if (duty_cycle > period) {
            ERR("dutycycle is bigger than period, invalid argument\n");
            ret = FAILURE;
            break;
        }

        ret = pwm_get_chn_enable(chn, &chn_enabled);
        if (FAILURE == ret) {
            ERR("pwm get chn enable failed\n");
            break;
        }

        if (duty_cycle > 0 && !chn_enabled) {
            ret = pwm_enable_chn(chn, TRUE);
            if (FAILURE == ret) {
                ERR("pwm enable chn failed\n");
                break;
            }
        }

        snprintf(pwm_dutycycle, sizeof(pwm_dutycycle) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_DUTY_CYCLE);
        ret = access(pwm_dutycycle, W_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_dutycycle);
            break;
        }

        fd = open(pwm_dutycycle, O_WRONLY);
        if (fd < 0) {
            ERR("open pwm%d dutycycle error!\n", chn);
            ret = FAILURE;
            break;
        }

        snprintf(dutycycle_buf, sizeof(dutycycle_buf) - 1, "%d", duty_cycle);
        dbg_pwm("dutycycle:%s\n", dutycycle_buf);

        ret = write(fd, dutycycle_buf, strlen(dutycycle_buf) + 1);
        if (FAILURE == ret) {
            ERR("write pwm%d dutycycle failed: %s\n", chn, strerror(errno));
            break;
        } else if (0 == ret) {
            ERR("write pwm%d dutycycle failed, nothing was written\n", chn);
            ret = FAILURE;
            break;
        }

        if (0 == duty_cycle && chn_enabled) {
            ret = pwm_enable_chn(chn, FALSE);
            if (FAILURE == ret) {
                ERR("pwm disable chn failed\n");
                break;
            }
        }

        ret = SUCCESS;
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}

int pwm_get_duty_cycle(int chn, int *duty_cycle)
{
    return_val_if_fail(chn >= PWM_CHN0 && chn < PWM_CHN_CNT, FAILURE);
    return_val_if_fail(NULL != duty_cycle, FAILURE);

    int ret = 0, fd = -1;
    char pwm_dutycycle[MAX_PATH_LEN] = {0};
    char dutycycle_buf[8] = {0};

    do {
        snprintf(pwm_dutycycle, sizeof(pwm_dutycycle) - 1, "%s%s%d/%s", 
                 PWM_DEV_PATH, PWM_CHN_HEAD, chn, PWM_DEV_DUTY_CYCLE);

        ret = access(pwm_dutycycle, R_OK);
        if (FAILURE == ret) {
            ERR("file %s doesn't exist!\n", pwm_dutycycle);
            break;
        }

        fd = open(pwm_dutycycle, O_RDONLY);
        if (fd < 0) {
            ERR("open pwm%d dutycycle error!\n", chn);
            ret = FAILURE;
            break;
        }

        do {
            ret = read(fd, dutycycle_buf, sizeof(dutycycle_buf) - 1);
        } while(FAILURE == ret && (EINTR == errno || EAGAIN == errno));

        if (FAILURE == ret) {
            ERR("read pwm%d dutycycle failed: %s\n", chn, strerror(errno));
            ret = FAILURE;
            break;
        } else if (0 == ret) {
            ERR("read end of pwm%d dutycycle, nothing was read\n", chn);
            ret = FAILURE;
            break;
        } else {
            ret = SUCCESS;
        }

        sscanf(dutycycle_buf, "%d", duty_cycle);
        DBG("read duty_cycle:%d\n", *duty_cycle);
    } while(0);

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    return ret;
}
