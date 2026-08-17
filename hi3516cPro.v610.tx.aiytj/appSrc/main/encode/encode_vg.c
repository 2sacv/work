#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jconfstruct.h"
#include "encode_typedef.h"
#include "encode_ivp_aidetect.h"
#include "encode_base_ivx.h"
#include "encode_vg.h"
#include "g_log.h"

static sObjects g_objects = {0};

static int g_across_mode = -1;

// 更新目标历史数据
void update_object_history_info(int across_flag, int cur_side, int index) 
{
    memmove(g_objects.object_info[index].region_status + 1,
            g_objects.object_info[index].region_status,
            (MAX_HISTORY - 1) * sizeof(int));
    g_objects.object_info[index].region_status[0] = cur_side;
    g_objects.object_info[index].across_flag = across_flag;
}

// 清除目标历史信息
void clear_object_info(int index)
{
    memset(g_objects.object_info[index].region_status, 0, sizeof(g_objects.object_info[index].region_status));
    g_objects.object_info[index].track_id = 0;
    g_objects.object_info[index].across_flag = -1;
    g_objects.object_info[index].lost_count = 0;
}

// 清除丢失指定帧次数目标的所有数据
void remove_lost_objects(int count)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (g_objects.object_info[i].track_id == 0) {
        continue;
        }

        if (g_objects.object_info[i].lost_count >= count) {
        clear_object_info(i);
        }
    }
}

// 创建目标
void create_object(int track_id, int across_flag, int cur_side)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (g_objects.object_info[i].track_id == 0) {
            g_objects.object_info[i].track_id = track_id;
            for (int k = 0; k < MAX_HISTORY; k++) {
                g_objects.object_info[i].region_status[k] = 0;
            }
            g_objects.object_info[i].region_status[0] = cur_side;
            g_objects.object_info[i].across_flag = across_flag;
            break;
        }
    }
}

// 查找目标，存在返回在数据数组中的索引，不存在返回 -1
int find_object_by_trackid(sObject *p)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (p->track_id == g_objects.object_info[i].track_id) {
            return i;
        }
    }
    return -1;
}

// 判断点是否在关注区域 ROI
int is_region_of_interest(int pos_x, int pos_y, sLine line)
{
    int x_min = (g_ivx_cfg->lineinfo.x0 < g_ivx_cfg->lineinfo.x1) ? g_ivx_cfg->lineinfo.x0 : g_ivx_cfg->lineinfo.x1;
    int x_max = (g_ivx_cfg->lineinfo.x0 > g_ivx_cfg->lineinfo.x1) ? g_ivx_cfg->lineinfo.x0 : g_ivx_cfg->lineinfo.x1;
    int y_min = (g_ivx_cfg->lineinfo.y0 < g_ivx_cfg->lineinfo.y1) ? g_ivx_cfg->lineinfo.y0 : g_ivx_cfg->lineinfo.y1;
    int y_max = (g_ivx_cfg->lineinfo.y0 > g_ivx_cfg->lineinfo.y1) ? g_ivx_cfg->lineinfo.y0 : g_ivx_cfg->lineinfo.y1;
    double tolerance = 1e-5;

    // 边界线垂直 or 平行判断
    if (line.A == 0) {          // 水平线
        if (pos_x >= x_min && pos_x <= x_max) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else if (line.B == 0) {   // 垂直线
        if (pos_y >= y_min && pos_y <= y_max) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    // 投影垂线方程参数
    double k = line.B / line.A;
    double b = pos_y - k * pos_x;

    // 计算交点坐标
    double denom = line.A + line.B * k;
    double x = -1 * (line.B * b + line.C) / denom;
    double y = k * x + b;

    // 判断是否在关注区域
    if (x <= x_max + tolerance && x >= x_min - tolerance && y <= y_max + tolerance && y >= y_min - tolerance) {
        return TRUE;
    } else {
        return FALSE;
    }
}

// 判断人形中线与边界线是否相交
int is_2line_crossed(sLine vgline, sLine human, sObject *p) 
{
    int x_min = (g_ivx_cfg->lineinfo.x0 < g_ivx_cfg->lineinfo.x1) ? g_ivx_cfg->lineinfo.x0 : g_ivx_cfg->lineinfo.x1;
    int x_max = (g_ivx_cfg->lineinfo.x0 > g_ivx_cfg->lineinfo.x1) ? g_ivx_cfg->lineinfo.x0 : g_ivx_cfg->lineinfo.x1;
    int y_min = (g_ivx_cfg->lineinfo.y0 < g_ivx_cfg->lineinfo.y1) ? g_ivx_cfg->lineinfo.y0 : g_ivx_cfg->lineinfo.y1;
    int y_max = (g_ivx_cfg->lineinfo.y0 > g_ivx_cfg->lineinfo.y1) ? g_ivx_cfg->lineinfo.y0 : g_ivx_cfg->lineinfo.y1;

    // 非垂直边界线：求交点
    double cross_x = human.top_x;
    double cross_y = (-(double)vgline.A * cross_x - vgline.C) / (double)vgline.B;

    // 判断交点是否在线段范围内
    if (cross_x >= x_min && cross_x <= x_max &&
        cross_y >= y_min && cross_y <= y_max &&
        cross_y >= human.top_y && 
        cross_y <= human.bottom_y) {
        p->across_flag = 1;
        return TRUE;
    }

    p->across_flag = 0;
    return FALSE;
}

int check_line_crossing_result(int index, int cross_mode) 
{
    int cur_side = g_objects.object_info[index].region_status[0];
    int prev_side = -1;
    for (int i = 1; i < MAX_HISTORY; i++) {     // 寻找有效数据
        if (cur_side != g_objects.object_info[index].region_status[i]) {
            prev_side = g_objects.object_info[index].region_status[i];
            break;
        }
    }

    if (prev_side == -1) {
        return FALSE;
    }

    int a2b = (prev_side == A_SIDE && cur_side == B_SIDE);
    int b2a = (prev_side == B_SIDE && cur_side == A_SIDE);

    switch (cross_mode) {
    case 0:
        return a2b;
    case 1:
        return b2a;
    case 2:
        return a2b || b2a;
    }

    return FALSE;
}

int is_vgline_alarmed(sObject *p, int is_crossed, int cur_side, int index)
{
    if (index != -1) {
        sObject *r = &g_objects.object_info[index];
        // 目标重现：重置 lost_count
         r->lost_count = 0;
        if (!is_crossed) {                  // 未交叉，正常更新数据
            update_object_history_info(p->across_flag, cur_side, index);
            return check_line_crossing_result(index, g_across_mode);
        } else {
            if (g_across_mode == 2) {       // 双向
                return TRUE;
            }

            if (r->across_flag == 0) {      // 交叉前未交叉，仅需判断未更新时的区域状态在哪一侧
                if ((r->region_status[0] == A_SIDE && g_across_mode == 0) || 
                    (r->region_status[0] == B_SIDE && g_across_mode == 1)) {
                    return TRUE;
                }
            } else if (r->across_flag == 1) {   // 交叉前依旧处于交叉状态
                if (g_across_mode == 2) {
                    return TRUE;
                }
                update_object_history_info(p->across_flag, cur_side, index);
                return check_line_crossing_result(index, g_across_mode);
            }
        }
    }else {
        if (!is_crossed) {
            create_object(p->track_id, 0, cur_side);
        } else {
            create_object(p->track_id, 1, cur_side);
        }
        return FALSE;
    }

    update_object_history_info(p->across_flag, cur_side, index);
    return FALSE;
}

void cross_mode_init(sLine vgline)
{
    // 界定穿越模式依据：单向穿越模式的方向向量的A点和B点一定在异侧，双向模式为同一点。
    int Line_value[2] = {
        vgline.A * g_ivx_cfg->lineinfo.dx0 + vgline.B * g_ivx_cfg->lineinfo.dy0 + vgline.C,
        vgline.A * g_ivx_cfg->lineinfo.dx1 + vgline.B * g_ivx_cfg->lineinfo.dy1 + vgline.C
    };
    if (Line_value[0] > 0 && Line_value[1] < 0) {
        g_across_mode = 0;      // A->B
    } else if (Line_value[0] < 0 && Line_value[1] > 0) {
        g_across_mode = 1;      // B->A
    } else if (g_ivx_cfg->lineinfo.dx0 == g_ivx_cfg->lineinfo.dx1 && g_ivx_cfg->lineinfo.dy0 == g_ivx_cfg->lineinfo.dy1) {
        g_across_mode = 2;      // A<->B
    }
}

void line_init(sLine *line, sLine *human, sObject *p)
{
    line->A = g_ivx_cfg->lineinfo.y1 - g_ivx_cfg->lineinfo.y0;
    line->B = g_ivx_cfg->lineinfo.x0 - g_ivx_cfg->lineinfo.x1;
    line->C = g_ivx_cfg->lineinfo.x1 * g_ivx_cfg->lineinfo.y0 - g_ivx_cfg->lineinfo.x0 * g_ivx_cfg->lineinfo.y1;

    human->top_x = p->start_x + (p->end_x - p->start_x) / 2;
    human->top_y = p->start_y;
    human->bottom_x = human->top_x;
    human->bottom_y = p->end_y;
    human->mid_x = human->top_x;
    human->mid_y = (p->start_y + p->end_y) / 2;
}

int is_object_line_crossing(sObjects *objects_filter_info) 
{
    static int detect_count = 0;
    detect_count++;
    if ((detect_count % 5) != 0) {
        return FALSE;
    } else {
        detect_count = 0;
    }

    if (!objects_filter_info || objects_filter_info->objects_num <= 0) {
        return FALSE;
    }

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (g_objects.object_info[i].track_id != 0) {
            g_objects.object_info[i].lost_count++;
        }
    }

    for (int i = 0; i < objects_filter_info->objects_num; i++) {

        sObject *p = &objects_filter_info->object_info[i];

        if (p->track_id <= 0) {
            continue;
        }

        sLine vgline = {0};
        sLine human = {0};
        line_init(&vgline, &human, p);
        cross_mode_init(vgline);
        
        if (!is_region_of_interest(human.top_x, human.top_y, vgline) &&
            !is_region_of_interest(human.bottom_x, human.bottom_y, vgline)) {
            if (!is_region_of_interest(human.mid_x, human.mid_y, vgline)) {
                continue;
            }
        }

        int cur_side = (vgline.A * human.mid_x + vgline.B * human.mid_y + vgline.C) >= 0 ? A_SIDE : B_SIDE;
        int index = find_object_by_trackid(p);
        int is_crossed = is_2line_crossed(vgline, human, p);    // 中线与越界线交叉情况获取
        if (is_vgline_alarmed(p, is_crossed, cur_side, index) == TRUE) {
            clear_object_info(index);
            return TRUE;
        }
    }

    remove_lost_objects(20);

    return FALSE;
}

int is_2rect_crossed(sLine *rectline, sObject *p, int *cur_side)
{
    // 计算区域划定方向：1 逆时针，2 顺时针
    int direction = ((rectline[0].A * g_ivx_cfg->rectinfo.x2 + rectline[0].B * g_ivx_cfg->rectinfo.y2 + rectline[0].C) > 0) ? 1 : 2;

    struct point {
        int x;
        int y;
    } point[4];
    point[0].x = p->start_x;
    point[0].y = p->start_y;
    point[1].x = p->end_x;
    point[1].y = p->start_y;
    point[2].x = p->start_x;
    point[2].y = p->end_y;
    point[3].x = p->end_x;
    point[3].y = p->end_y;

    //矩形顶点在区域内的数量
    int inside_num= 0;
    for (int i = 0; i < 4; i++) {
        int inside = TRUE;  // 假设当前点在区域内
        for (int k = 0; k < 4; k++) {
            int value = rectline[k].A * point[i].x + rectline[k].B * point[i].y + rectline[k].C;

            if (direction == 1) {
                if (value < 0) {    // 逆时针情况，都在线的左侧即在内
                    inside = FALSE;
                    break;          // 提前退出，不检查剩下的边
                }
            } else {
                if (value > 0) {    // 顺时针情况，都在线的右侧即在内
                    inside = FALSE;
                    break;
                }
            }
        }

        if (inside) {
            inside_num++;  // 只有当 4 条边都满足时才计数
        }
    }

    // 区域四个顶点坐标
    point[0].x = g_ivx_cfg->rectinfo.x0;
    point[0].y = g_ivx_cfg->rectinfo.y0;
    point[1].x = g_ivx_cfg->rectinfo.x1;
    point[1].y = g_ivx_cfg->rectinfo.y1;
    point[2].x = g_ivx_cfg->rectinfo.x2;
    point[2].y = g_ivx_cfg->rectinfo.y2;
    point[3].x = g_ivx_cfg->rectinfo.x3;
    point[3].y = g_ivx_cfg->rectinfo.y3;

    // 四个顶点和矩形框的包含数量
    int num = 0;
    for (int i = 0; i < 4; i ++) {
        if (point[i].x >= p->start_x && point[i].x <= p->end_x &&
            point[i].y >= p->start_y && point[i].y <= p->end_y) {
            num++;
        }
    }

    if ((inside_num >= 1 && inside_num <= 3) || (num >= 1 && num <= 3)) {
        p->across_flag = 1;     // 交叉
        return TRUE;
    } else if (inside_num == 4 ) {
        p->across_flag = 2;     // 矩形框在区域内（被包围）
    } else if (num == 4) {
        p->across_flag = 3;     // 矩形框包围区域
        *cur_side = INSIDE;
    } else {
        p->across_flag = 0;     // 不交叉且矩形框在区域外
    }

    return FALSE;
}

int check_rect_crossing_result(int index, int cross_mode) {
    int cur_side = g_objects.object_info[index].region_status[0];
    int prev_side = -1;
    for (int i = 1; i < MAX_HISTORY; i++) {     // 寻找有效数据
        if (cur_side != g_objects.object_info[index].region_status[i]) {
            prev_side = g_objects.object_info[index].region_status[i];
            break;
        }
    }

    if (prev_side == -1) {
        return FALSE;
    }

    int out2in = (prev_side == OUTSIDE && cur_side == INSIDE);
    int in2out = (prev_side == INSIDE && cur_side == OUTSIDE);

    switch (cross_mode) {
    case 0:
        return out2in;
    case 1:
        return in2out;
    case 2:
        return out2in || in2out;
    }

    return FALSE;
}

int is_vgrect_alarmed(sObject *p, int is_crossed, int cur_side, int index)
{
    if (index != -1) {
        sObject *r = &g_objects.object_info[index];
        // 目标重现：重置 lost_count
         r->lost_count = 0;
        if (!is_crossed) {  // 未交叉
            update_object_history_info(p->across_flag, cur_side, index);
            return check_rect_crossing_result(index, g_across_mode);
        } else {
            if (g_across_mode == 2) {   // 双向
                return TRUE;
            }

            if (r->across_flag == 0) {  // 交叉前处于区域外
                if ((r->region_status[0] == OUTSIDE && g_across_mode == 0) || 
                    (r->region_status[0] == INSIDE && g_across_mode == 1)) {
                    return TRUE;
                }
            }

            if (r->across_flag == 1) {  // 交叉前依旧处于交叉状态
                update_object_history_info(p->across_flag, cur_side, index);
                return check_rect_crossing_result(index, g_across_mode);
            } else if (r->across_flag == 2 || r->across_flag == 3) {    // 交叉前矩形框包围或被包围
                if (g_across_mode == 1) {
                    return TRUE;
                }
            }
        }
    } else {
        create_object(p->track_id, p->across_flag, cur_side);
        return FALSE;
    }

    update_object_history_info(p->across_flag, cur_side, index);
    return FALSE;
}

void rectline_init(sLine *rectline, sLine *human, sObject *p)
{
    rectline[0].A = g_ivx_cfg->rectinfo.y1 - g_ivx_cfg->rectinfo.y0;
    rectline[0].B = g_ivx_cfg->rectinfo.x0 - g_ivx_cfg->rectinfo.x1;
    rectline[0].C = g_ivx_cfg->rectinfo.x1 * g_ivx_cfg->rectinfo.y0 - g_ivx_cfg->rectinfo.x0 * g_ivx_cfg->rectinfo.y1;

    rectline[1].A = g_ivx_cfg->rectinfo.y2 - g_ivx_cfg->rectinfo.y1;
    rectline[1].B = g_ivx_cfg->rectinfo.x1 - g_ivx_cfg->rectinfo.x2;
    rectline[1].C = g_ivx_cfg->rectinfo.x2 * g_ivx_cfg->rectinfo.y1 - g_ivx_cfg->rectinfo.x1 * g_ivx_cfg->rectinfo.y2;

    rectline[2].A = g_ivx_cfg->rectinfo.y3 - g_ivx_cfg->rectinfo.y2;
    rectline[2].B = g_ivx_cfg->rectinfo.x2 - g_ivx_cfg->rectinfo.x3;
    rectline[2].C = g_ivx_cfg->rectinfo.x3 * g_ivx_cfg->rectinfo.y2 - g_ivx_cfg->rectinfo.x2 * g_ivx_cfg->rectinfo.y3;

    rectline[3].A = g_ivx_cfg->rectinfo.y0 - g_ivx_cfg->rectinfo.y3;
    rectline[3].B = g_ivx_cfg->rectinfo.x3 - g_ivx_cfg->rectinfo.x0;
    rectline[3].C = g_ivx_cfg->rectinfo.x0 * g_ivx_cfg->rectinfo.y3 - g_ivx_cfg->rectinfo.x3 * g_ivx_cfg->rectinfo.y0;

    human->mid_x = (p->start_x + p->end_x) / 2;
    human->mid_y = (p->start_y + p->end_y) / 2;
}

int judge_side(sLine *rectline, sLine human)
{
    int E_value[4] = {0};
    for (int i = 0; i< 4; i++) {
        E_value[i] = rectline[i].A * human.mid_x + rectline[i].B * human.mid_y + rectline[i].C;
    }

    // 计算区域划定方向：1 逆时针，2 顺时针
    int direction = ((rectline[0].A * g_ivx_cfg->rectinfo.x2 + rectline[0].B * g_ivx_cfg->rectinfo.y2 + rectline[0].C) > 0) ? 1 : 2;
    if ((E_value[0] > 0 && E_value[1] > 0 && E_value[2] > 0 && E_value[3] > 0) && direction == 1) {
        return INSIDE;
    } else if ((E_value[0] < 0 && E_value[1] < 0 && E_value[2] < 0 && E_value[3] < 0) && direction == 2) {
        return INSIDE;
    } else {
        return OUTSIDE;
    }
}

int is_object_rect_crossing(sObjects *objects_filter_info)
{
    static int detect_count = 0;
    detect_count++;
    if ((detect_count % 5) != 0) {
        return FALSE;
    } else {
        detect_count = 0;
    }

    if (!objects_filter_info || objects_filter_info->objects_num <= 0) {
        return FALSE;
    }

    g_across_mode = g_ivx_cfg->rectinfo.dir;

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (g_objects.object_info[i].track_id != 0) {
            g_objects.object_info[i].lost_count++;
        }
    }

    for (int i = 0; i < objects_filter_info->objects_num; i++) {
        sObject *p = &objects_filter_info->object_info[i];

        if (p->track_id <= 0) {
            continue;
        }

        sLine rectline[4] = {0};
        sLine human = {0};
        rectline_init(rectline, &human, p);
        int cur_side = judge_side(rectline, human);
        int index = find_object_by_trackid(p);
        int is_crossed = is_2rect_crossed(rectline, p, &cur_side);
        if (is_vgrect_alarmed(p, is_crossed, cur_side, index)) {
            clear_object_info(index); // 报警后清除
            return TRUE;
        }
    }

    remove_lost_objects(20);

    return FALSE;
}

