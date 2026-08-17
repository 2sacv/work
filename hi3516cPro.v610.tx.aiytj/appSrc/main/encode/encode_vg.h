#ifndef __ENCODE_VG_H__
#define __ENCODE_VG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HISTORY 10
#define MAX_OBJECTS 30

typedef struct {
    int top_x;      //矩形框中线信息
    int top_y;
    int bottom_x;
    int bottom_y;
    int mid_x;
    int mid_y;
    int A;          //直线一般方程 Ax + By + C 参数
    int B;
    int C;
} sLine;

// 单目标信息
// 检测坐标系 1920 * 1080 (以构建线段和区域的坐标系为准)
typedef struct {
    int track_id;
    int across_flag;                //与线或区域的关系
    int region_status[MAX_HISTORY];
    int lost_count;                 //目标丢失次数
    int start_x;                    //矩形框信息
    int start_y;
    int end_x;
    int end_y;
} sObject;

//  多目标信息
typedef struct {
    int objects_num;
    sObject object_info[MAX_OBJECTS];
} sObjects;

// 区域位置枚举（线和矩形检测）
typedef enum {
    A_SIDE  = 1,        // 区域A
    B_SIDE  = 2,        // 区域B
    INSIDE  = 3,        // 区域内
    OUTSIDE = 4,        // 区域外
} eRegionPosition;


int is_object_line_crossing(sObjects *objects_filter_info);     //判断是否产生越界侦测报警
int is_object_rect_crossing(sObjects *objects_filter_info);     //判断是否产生区域侦测报警


#ifdef __cplusplus
}
#endif
#endif
