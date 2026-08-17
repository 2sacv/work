/*
 *       Filename:  lamp_smart_photo_sens.h
 *    Description:  
 *        Version:  1.0
 *        Created:  05/05/2022 08:53:52 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef __LAMP_SMART_PHOTO_SENS_H__
#define __LAMP_SMART_PHOTO_SENS_H__
#ifdef __cplusplus 
extern "C" {
#endif

#define REDUCE_LIGHT_GRADE (-1)
#define IMPROVE_LIGHT_GRADE (1)

typedef struct {
    uint32_t   EV_on;        //EV 小于此值时，开启灯光
    uint32_t   EV_off;       //EV 大于此值时，关闭灯光
    uint32_t   EV_max;       //最大的 EV，用于从 api 获取 EV 值时将 EV 比例缩小到一定范围并取正比例
    uint32_t   EV_min;       //
    float EV_deci;      //1~10, 当前EV 占 EV_avg 的成数，越大切得越快
    float bias_force9t; //进入防反复切时，EV 相较于 EV_force9t 允许的浮动范围
    float bias_stable;  //判断 EV 是否稳定，EV - EV_avg 允许的最大差值
} PhotoSensThreshold;

typedef struct {
    uint64_t EV0;              //jz曝光值
    uint32_t EV;                    //曝光值
    uint32_t EV_prev;               //上一次曝光值
    uint32_t EV_avg;                //曝光平均值
    uint32_t EV_avg_prev;           //上一次曝光平均值
    uint32_t EV_force9t;            //反复切时，晚上记录的 EV 值
    int rgain;                 //红色增益值
    int bgain;                 //蓝色增益值
    int color_temp;            //色温
    int is_day;                //此时日夜状态
    int is_day_prev;           //上一次日夜状态
} PhotoSensRun;

//切换到全彩模式时，调用此初始化
int lamp_photosens_full_color_init(void);

//切换到红外或双光源模式时，调用此初始化
int lamp_photosens_infrared_init(void);

//外部切换模式时，供其清除内部防反复切状态
void lamp_photosens_clean_status(void);

/*
 * @desc: 软光敏获取日夜状态
 * @return: -1, 失败; 0 晚上; 1 白天
*/
int lamp_photosens_get_daynight(int *is_day, int *is_force_night);

int is_photosens_day(void);

#ifdef __cplusplus
}
#endif
#endif // __LAMP_SMART_PHOTO_SENS_H__

