#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ot_common_aidetect.h"
#include "encode_noise.h"
#include "ptz_ctrl.h"

#include "g_stat.h"
#include "g_log.h"
#include "debug.h"
#include "utils.h"

static void update_prev_obj(sRepeatedObj *p_prev, ot_aidetect_object *p_obj,
                            int x_center, int y_center)
{
    p_prev->x_center = x_center;
    p_prev->y_center = y_center;
    p_prev->w = p_obj->detect_rect.width;
    p_prev->h = p_obj->detect_rect.height;
    p_prev->cnt_tolenrence = 0;
    p_prev->cnt_appeared = 0;
    p_prev->is_black = FALSE;
    p_prev->black_list_idx = -1;
}

static float rect_overlap_ratio(ot_rect *a, sBlackList *b)
{
    int x1 = MAX(a->x, b->x);
    int y1 = MAX(a->y, b->y);
    int x2 = MIN(a->x + a->width, b->x + b->w);
    int y2 = MIN(a->y + a->height, b->y + b->h);

    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }

    int inter_area = (x2 - x1) * (y2 - y1);
    int area_a = a->width * a->height;
    int area_b = b->w * b->h;
    int max_area = MAX(area_a, area_b);

    if (max_area <= 0) {
        return 0.0f;
    }

    return (float)inter_area / (float)max_area;
}


static void update_black_list(sBlackList black_list[][AIDET_FILTER_MAX_NUM],
                              int cls_index, sRepeatedObj *p_prev)
{
    int i = 0;
    ot_rect cur_rect;
    int found_empty = -1;
    int oldest_idx = 0;
    int oldest_age = 0;

    cur_rect.x = p_prev->x_center - (p_prev->w / 2);
    cur_rect.y = p_prev->y_center - (p_prev->h / 2);
    cur_rect.width = p_prev->w;
    cur_rect.height = p_prev->h;

    if (cur_rect.x == 0 && cur_rect.y == 0 && cur_rect.width == 0 && cur_rect.height == 0) {
        pri_ivx(LVL_LOOP, "data anomaly\n");
        return;
    }

    for (i = 0; i < AIDET_FILTER_MAX_NUM; i++) {
        sBlackList *p_bl = &black_list[cls_index][i];

        // 记录第一个空位
        if (p_bl->existence == FALSE) {
            if (found_empty < 0) {
                found_empty = i;
            }
            continue;
        }

        // 查重：已有条目和当前重合则跳过
        if (rect_overlap_ratio(&cur_rect, p_bl) >= HD_ALARM_ALLOW_OVERLAP) {
            pri_ivx(LVL_LOOP, "blacklist[%d][%d] duplicate, skip\n", cls_index, i);
            return;
        }

        // 记录最老的条目
        if (p_bl->age > oldest_age) {
            oldest_age = p_bl->age;
            oldest_idx = i;
        }
    }

    // 优先写空位，没有空位则淘汰最老的
    if (found_empty >= 0) {
        i = found_empty;
    } else {
        i = oldest_idx;
        pri_ivx(LVL_LOOP, "blacklist[%d] full, evict oldest [%d] age=%d\n",
                cls_index, i, oldest_age);
    }

    black_list[cls_index][i].x = cur_rect.x;
    black_list[cls_index][i].y = cur_rect.y;
    black_list[cls_index][i].w = cur_rect.width;
    black_list[cls_index][i].h = cur_rect.height;
    black_list[cls_index][i].existence = TRUE;
    black_list[cls_index][i].age = 0;

    pri_ivx(LVL_LOOP, "blacklist[%d][%d] record: x=%d y=%d w=%d h=%d\n",
            cls_index, i, cur_rect.x, cur_rect.y,
            cur_rect.width, cur_rect.height);
}

// 返回: 1 = 匹配黑名单(应过滤), 0 = 不匹配
static int repeat_rect_check(sBlackList black_list[][AIDET_FILTER_MAX_NUM],
                             int cls_index,
                             ot_rect *p_rect,
                             float targrt)
{
    for (int i = 0; i < AIDET_FILTER_MAX_NUM; i++) {
        sBlackList *p_bl = &black_list[cls_index][i];
        float ratio = 0.0f;

        if (p_bl->w == 0 || p_bl->h == 0) {
            continue;
        }

        ratio = rect_overlap_ratio(p_rect, p_bl);

        pri_ivx(LVL_LOOP, "blacklist[%d][%d]: "
                "cur(%d,%d,%d,%d) bl(%d,%d,%d,%d) overlap=%.2f target=%.2f\n",
                cls_index, i,
                p_rect->x, p_rect->y, p_rect->width, p_rect->height,
                p_bl->x, p_bl->y, p_bl->w, p_bl->h,
                ratio, targrt);

        if (ratio >= targrt) {
            p_bl->age = 0;
            p_bl->match_count++;
            return i;
        }
    }

    return -1;
}

// 判断当前目标是否已远离对应的黑名单区域
// 返回: 1 = 已离开, 0 = 仍在附近
static int is_moved(ot_rect *p_rect, sBlackList *p_bl)
{
    float ratio = 0.0f;

    // 数据被清除,视为已离开
    if (p_bl->w == 0 && p_bl->h == 0) {
        return TRUE;
    }

    ratio = rect_overlap_ratio(p_rect, p_bl);

    // 重合度 < HD_BLACKLIST_LEAVE_OVERLAP%，认定已远离
    if (ratio < HD_BLACKLIST_LEAVE_OVERLAP) {
        p_bl->age = 0;
        pri_ivx(LVL_LOOP, "is_moved: cur(%d,%d,%d,%d) bl(%d,%d,%d,%d) overlap=%.2f\n",
                p_rect->x, p_rect->y, p_rect->width, p_rect->height,
                p_bl->x, p_bl->y, p_bl->w, p_bl->h, ratio);
        return TRUE;
    }

    return FALSE;
}

void encode_ivp_object_filter(ot_aidetect_result_array *p_result, struct ivx_cfg* ivx_info)
{
    static sBlackList black_list[AIDET_MAX_TYPE][AIDET_FILTER_MAX_NUM] = {0};
    static sRepeatedObj obj_prev[AIDET_MAX_TYPE][AIDET_FILTER_MAX_NUM] = {0};
    static int trkid_baked[AIDET_MAX_TYPE][AIDET_FILTER_MAX_NUM] = {0};

    int stat_follow = get_follow_status();
    int stat_pc = get_person_center_status();
    int follow_idle = (FOLLOW_IDEL == stat_follow && FOLLOW_IDEL == stat_pc);

    int filter_enabled[E_IDX_OBJ_CNT] = {
        0,                      //face, default disable
        ivx_info->hdinfo.enable && ivx_info->hdinfo.thresh <= 66,
        ivx_info->carinfo.enable && ivx_info->carinfo.thresh <= 66,
        ivx_info->petinfo.enable && ivx_info->petinfo.thresh <= 66
    };

    int x_center = 0, y_center = 0;           //当前目标框中心点位置
    int dx_center = 0, dy_center = 0;         //当前和历史目标框中心点位置绝对差值
    int x_dist_square = 0, y_dist_square = 0; //dx_center²，dy_center²，用于计算直线距离
    int x_range = 0, y_range = 0;             //水平垂直允许浮动的范围
    int diff_dxy = 0;
    float occupy_dxy = 0.0;

    for (td_u32 idx_cls = 0; idx_cls < AIDET_MAX_TYPE; idx_cls++) {
        if (idx_cls != OT_AIDETECT_CLASS_HUMAN &&
            idx_cls != OT_AIDETECT_CLASS_PET) {
            continue;
        }

        // 黑名单老化：每帧所有条目 age++
        for (int i = 0; i < AIDET_FILTER_MAX_NUM; i++) {
            if (black_list[idx_cls][i].existence) {
                black_list[idx_cls][i].age++;
            }
        }
    }

    for (td_u32 idx_cls = 0; idx_cls < AIDET_MAX_TYPE; idx_cls++) {
        ot_aidetect_object_of_one_class *p_cls = &p_result->object_class[idx_cls];

        if (!follow_idle) {
            pri_hd(LVL_LOOP, "motor is following, follow: %d, stat_pc: %d\n",
                   stat_follow, stat_pc);
            break;
        }

        //只对人宠进行静态过滤
        if (idx_cls != OT_AIDETECT_CLASS_HUMAN &&
            idx_cls != OT_AIDETECT_CLASS_PET) {
            continue;
        }

        if (!filter_enabled[idx_cls]) {
            pri_hd(LVL_LOOP, "idx_cls %d filter not enabled\n", idx_cls);
            continue;
        }

        for (td_u32 idx_obj = 0; idx_obj < p_cls->object_num; idx_obj++) {
            ot_aidetect_object *p_obj = &p_cls->objects[idx_obj];
            sRepeatedObj *p_prev = &obj_prev[idx_cls][p_obj->track_id % AIDET_FILTER_MAX_NUM];
            int *p_baked = &trkid_baked[idx_cls][p_obj->track_id % AIDET_FILTER_MAX_NUM];
            pri_ivx(LVL_LOOP, "start filter track_id %d\n", p_obj->track_id);

            //track_status 为 new, 重置之前累积的数据
            if (OT_AIDETECT_TRACK_STATUS_NEW == p_obj->track_status) {
                if (*p_baked) {
                    pri_ivx(LVL_LOOP, "track_id %d not update yet\n", p_obj->track_id);
                    memset(p_prev, 0, sizeof(sRepeatedObj));
                    p_prev->black_list_idx = -1;
                    *p_baked = FALSE;
                }
                continue;
            //存活周期小于 CNT_TOLENRENCE_COUNT 帧
            } else if (OT_AIDETECT_TRACK_STATUS_DIE == p_obj->track_status && 
                       p_prev->cnt_appeared < CNT_TOLENRENCE_COUNT) {
                update_black_list(black_list, idx_cls, p_prev);
                *p_baked = FALSE;
                continue;
            }

            x_center = p_obj->detect_rect.x + p_obj->detect_rect.width / 2;
            y_center = p_obj->detect_rect.y + p_obj->detect_rect.height / 2;
            pri_ivx(LVL_LOOP, "x_center: %d, y_center: %d\n", x_center, y_center);

            //track_id 有备份过，判断静态累加量
            if (*p_baked) {
                if (p_prev->cnt_appeared < 10 * CNT_TOLENRENCE_COUNT) {
                    p_prev->cnt_appeared++;
                } else {
                    p_prev->cnt_appeared = CNT_TOLENRENCE_COUNT;
                }

                dx_center = p_prev->x_center - x_center;
                dy_center = p_prev->y_center - y_center;
                x_dist_square = dx_center * dx_center;
                y_dist_square = dy_center * dy_center;

                x_range = MAX(((p_obj->detect_rect.width + p_prev->w) / 2.0 *
                          HD_ALARM_ALLOW_RANGE_X), 1);
                y_range = MAX(((p_obj->detect_rect.height + p_prev->h) / 2.0 *
                          HD_ALARM_ALLOW_RANGE_Y), 1);
                diff_dxy = abs(abs(dx_center) - abs(dy_center));
                occupy_dxy = (1.0 * diff_dxy) / MAX(MAX(abs(dx_center), abs(dy_center)), 1.0);

                pri_ivx(LVL_LOOP, "dx_center: %d, dy_center: %d\n",
                        dx_center, dy_center);
                pri_ivx(LVL_LOOP, "x_square: %d, y_square: %d\n",
                        x_dist_square, y_dist_square);
                pri_ivx(LVL_LOOP, "x_range: %d, y_range: %d\n",
                        x_range, y_range);
                pri_ivx(LVL_LOOP, "diff_dxy: %d, occpuy_dxy: %.2f\n",
                        diff_dxy, occupy_dxy);

                //计算前后中心点位置差距，小于人形框宽高的 10%，则累加计数
                int moved = 0;
                if (x_dist_square <= x_range * x_range && y_dist_square <= y_range * y_range &&
                    (diff_dxy < PIXEL_MIN_MOVED || occupy_dxy < HD_ALARM_ALLOW_RANGE_DXY)) {
                    pri_ivx(LVL_LOOP, "track_id %d static count added: %d\n",
                            p_obj->track_id, p_prev->cnt_appeared);
                    if (p_prev->cnt_tolenrence < 10 * CNT_TOLENRENCE_COUNT) {
                        p_prev->cnt_tolenrence++;
                        if (p_prev->cnt_tolenrence ==  CNT_TOLENRENCE_COUNT) {
                            update_black_list(black_list, idx_cls, p_prev);
                        }
                    } else {
                        p_prev->cnt_tolenrence = CNT_TOLENRENCE_COUNT;
                    }
                    moved = FALSE;
                } else {
                    moved = TRUE;
                }

                //是黑名单目标且目标静止才继续和对应的 black_list 条目比对
                if (p_prev->is_black) {
                    sBlackList *p_bl = &black_list[idx_cls][p_prev->black_list_idx];
                    if (!moved) {
                        //静止状态下与黑名单位置的重合度判断
                        if (is_moved(&p_obj->detect_rect, p_bl)) {
                            pri_ivx(LVL_LOOP, "clear black_list[%u][%d]\n", idx_cls, p_prev->black_list_idx);
                            update_prev_obj(p_prev, p_obj, x_center, y_center);
                            memset(p_bl, 0, sizeof(sBlackList));
                        }
                    } else {
                        pri_ivx(LVL_LOOP, "clear track_id %d history,it not black\n", p_obj->track_id);
                        memset(p_prev, 0, sizeof(sRepeatedObj));
                        *p_baked = FALSE;
                    }
                //否则清除历史数据
                } else {
                    if (moved) {
                        pri_ivx(LVL_LOOP, "clear track_id %d history\n", p_obj->track_id);
                        memset(p_prev, 0, sizeof(sRepeatedObj));
                        *p_baked = FALSE;
                    }
                }
            //没备份过，首先和黑名单内进行匹配，否则进行第一次备份，不做处理
            } else {
                int idx = repeat_rect_check(black_list, idx_cls, &p_obj->detect_rect, HD_ALARM_ALLOW_OVERLAP);
                if (idx >= 0) {
                    // 命中黑名单，先标记过滤
                    update_prev_obj(p_prev, p_obj, x_center, y_center);
                    p_prev->black_list_idx = idx;
                    p_prev->cnt_tolenrence = CNT_TOLENRENCE_COUNT;
                    p_prev->is_black = TRUE;
                    *p_baked = TRUE;
                    pri_ivx(LVL_LOOP, "track_id %d hit blacklist[%u][%d], direct filter\n",
                            p_obj->track_id, idx_cls, idx);
                } else {
                    *p_baked = TRUE;
                    update_prev_obj(p_prev, p_obj, x_center, y_center);
                    pri_ivx(LVL_LOOP, "first record track_id %d\n", p_obj->track_id);
                }
            }

            if (p_prev->cnt_tolenrence >= CNT_TOLENRENCE_COUNT) {
                pri_ivx(LVL_LOOP, "filtered track_id %d\n", p_obj->track_id);
                memset(p_obj, 0, sizeof(ot_aidetect_object));
            }
        }
    }

    return;
}
