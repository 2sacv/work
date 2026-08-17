/******************************************************************************
    Copyright (C), 2017-2027, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_Pre_od.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2018-03-20   by cuiweixun    Description  : 嵌入式设备镜头遮挡检测模块头文件
                 定义了遮挡检测所需的数据结构、常量和接口函数。
                 核心思想：将图像划分为网格块，比较当前帧与参考帧的差异，
                 通过统计变化块数量、遮挡区域尺寸和图像清晰度(FV)来判断
                 镜头是否被遮挡，并区分遮挡类型(近距离遮挡、远距离遮挡、
                 遮挡撤销、镜头变位等)。
    History      :
******************************************************************************/

#ifndef __ENCODE_PRE_OD_H__
#define __ENCODE_PRE_OD_H__

#ifdef __cplusplus
extern  "C" {
#endif

#include "jconfstruct.h"
#include "encode_common.h"
#include "ot_common_video.h"

/* 遮挡检测网格列数：将图像水平方向划分为60列 */
#define VMask_DETECTION_GRID_COLUMN        	(60)
/* 遮挡检测网格行数：将图像垂直方向划分为30行 */
#define VMask_DETECTION_GRID_ROW           	(30)

/*
 * FV方向阈值：用于区分遮挡类型
 *   |obj_FV_dir| > FV_DIR_THRESHOLD: 近距离遮挡或遮挡撤销（FV剧烈变化）
 *   |obj_FV_dir| <= FV_DIR_THRESHOLD: 镜头变位或远距离遮挡（FV小幅变化）
 */
#define FV_DIR_THRESHOLD_CLOSE_OCCLUSION   (1000)

/*
 * 遮挡检测输入参数结构体
 * 包含灵敏度配置、采样计时器、检测阈值等参数
 */
typedef struct {
    long M_obj_FV;                    /* 当前帧的清晰度(Focus Value)，梯度平方和 */
    long M_obj_FVstatic;              /* 参考帧（采样帧）的清晰度，用于对比判断遮挡性质 */

    unsigned int M_obj_Static_Image_Limit;  /* 静止图像判定阈值：变化块数 <= 此值视为静止 */
    unsigned int M_obj_Mask_Min_Limit;      /* 连续疑似遮挡确认次数：连续检测到遮挡达到此数才报警 */
    unsigned int M_obj_Sample_T_Max_Limit;  /* 图像变化环境下采样周期上限（单位：帧数，300帧≈60秒） */
    unsigned int M_obj_Sample_T_Min_LimitA; /* 静止环境下正常采样周期（单位：帧数，20帧≈4秒） */
    unsigned int M_obj_Sample_T_Min_LimitB; /* 报警后快速采样周期（单位：帧数，5帧≈1秒），快速重建参考帧 */
    unsigned int M_obj_Sample_T_Max;        /* 图像变化环境计时器：记录连续有变化的帧数 */
    unsigned int M_obj_Sample_T_Min;        /* 静止环境计时器：记录连续无变化的帧数 */
    unsigned int M_obj_Sample_En;           /* 采样使能标志：1=已采样过参考帧可检测，0=未采样 */

    unsigned int init;                      /* 初始化标志：1=已初始化，0=未初始化 */
    unsigned int md_result_cnt;             /* 连续疑似遮挡计数器：累计检测到疑似遮挡的次数 */
    unsigned int sensitivity_dotY;          /* 块内像素亮度差阈值：两帧间单像素亮度差超过此值算有效变化点 */
    unsigned int sensitivity_block_Dot;     /* 块内最小有效点数：块内变化像素数超过此值，该块标记为变化 */
    unsigned int sensitivity_Size_w;        /* 遮挡物最小宽度阈值（网格列数）：遮挡区域宽度需 >= 此值 */
    unsigned int sensitivity_Size_h;        /* 遮挡物最小高度阈值（网格行数）：遮挡区域高度需 >= 此值 */
    unsigned int sensitivity_blocks_check;  /* 触发遮挡检测的变化块数下限：当前帧与上一帧变化块数 >= 此值才开始遮挡检测 */
    unsigned int sensitivity_blocks_mask;   /* 遮挡有效块数下限：当前帧与参考帧变化块数 >= 此值才视为遮挡 */
    unsigned int block_w;                   /* 单个检测块的像素宽度 = RAW_W / 网格列数 */
    unsigned int block_h;                   /* 单个检测块的像素高度 = RAW_H / 网格行数 */
} PreODParamsIn_t;

/*
 * 遮挡检测工作区结构体
 * 在单次检测过程中累积的中间结果，每次检测前清零
 */
typedef struct {
    unsigned int M_obj_x_min;    /* 变化块的最小列号（遮挡区域左边界） */
    unsigned int M_obj_x_max;    /* 变化块的最大列号（遮挡区域右边界） */
    unsigned int M_obj_y_min;    /* 变化块的最小行号（遮挡区域上边界） */
    unsigned int M_obj_y_max;    /* 变化块的最大行号（遮挡区域下边界） */
    unsigned int M_obj_Acnt;     /* 孤立变化块计数：左右邻居均无变化的块数，用于分析遮挡形态 */
    unsigned int M_obj_cnt;      /* 变化块总数：标记为1的网格块数量 */
    unsigned int M_obj_x;        /* 变化块列号累加和，用于计算遮挡中心X坐标 */
    unsigned int M_obj_y;        /* 变化块行号累加和，用于计算遮挡中心Y坐标 */
    unsigned int M_obj_h;        /* 遮挡区域高度（网格行数）= y_max - y_min */
    unsigned int M_obj_w;        /* 遮挡区域宽度（网格列数）= x_max - x_min */
} PreODWork_t;

/*
 * 遮挡检测图像缓存结构体
 * 存储参考帧灰度图和块分布映射表
 */
typedef struct {
    unsigned char Od_SamplegYBuf[RAW_W * RAW_H];               /* 参考帧灰度(Y)缓冲区：保存采样时刻的图像，用于与当前帧对比 */
    unsigned char ODBlockMap[VMask_DETECTION_GRID_ROW][VMask_DETECTION_GRID_COLUMN]; /* 块分布映射表：1=该块检测到变化，0=无变化 */
} PreODImage_t;

/*
 * 遮挡检测输出参数结构体
 * 包含检测结果：是否遮挡、遮挡位置/尺寸、清晰度变化方向等
 */
typedef struct {
    unsigned short md_result;     /* 遮挡报警结果：1=检测到遮挡，0=无遮挡 */
    unsigned short obj_cnt;       /* 变化块总数 */
    unsigned short obj_x;         /* 遮挡中心X坐标（网格列号均值） */
    unsigned short obj_y;         /* 遮挡中心Y坐标（网格行号均值） */
    unsigned short obj_h;         /* 遮挡区域高度（网格行数） */
    unsigned short obj_w;         /* 遮挡区域宽度（网格列数） */
    long obj_FV;                  /* 当前帧清晰度(Focus Value) */
    long obj_FV_static;           /* 参考帧清晰度 */
    long obj_FV_dir;              /* 清晰度变化放大差异指数 = 100*(FV - FVstatic) / (min(FV, FVstatic))
                                     分母使用较小值以放大差异，使结果可超过±100，便于区分遮挡类型
                                     正值大(>+FV_DIR_THRESHOLD): 近距离遮挡撤销（图像变清晰）
                                     负值大(<-FV_DIR_THRESHOLD): 近距离遮挡（图像变模糊）
                                     正值小(0~+FV_DIR_THRESHOLD): 镜头变位或远距离遮挡撤销
                                     负值小(-FV_DIR_THRESHOLD~0): 镜头变位或远距离遮挡 */
} PreODParamsOut_t;

/*
 * 遮挡检测模块主结构体
 * 聚合了配置、工作区、图像缓存和输出等所有子结构体
 */
typedef struct {
    VMaskAlarmS tVMaskAlarm;      /* 遮挡报警配置：使能开关和灵敏度阈值(0~100) */
    PreODWork_t param_Work;       /* 工作区：单次检测中间结果 */
    PreODImage_t ImageBuf;        /* 图像缓存：参考帧和块映射表 */
    PreODParamsOut_t param_OutPut;/* 输出参数：检测结果 */
    PreODParamsIn_t param_InPut;  /* 输入参数：灵敏度配置和计时器 */
    unsigned int PrintEn;         /* 调试打印使能 */
    unsigned int toggle;          /* 交替标志（预留） */
} TEncodePre_OD;

/*
 * 帧信息包装结构体
 * 封装了视频帧指针和一份灰度图拷贝
 * copygYBuf用于保存上一帧图像，与当前帧做快速对比
 */
typedef struct myIMPFrameInfo_t {
	ot_video_frame_info *Frame;          /* 视频帧信息指针，包含帧数据地址和属性 */
	unsigned char copygYBuf[RAW_W*RAW_H]; /* 上一帧灰度图拷贝，用于与当前帧对比检测变化 */
} myIMPFrameInfo_t;

/*
 * 遮挡检测模块初始化
 * 根据灵敏度阈值(0~100)计算各项检测参数
 * @param pVMaskAlarm 遮挡报警配置（使能开关 + 灵敏度阈值）
 * @return S_OK 初始化成功
 */
int encode_Pre_OD_init(VMaskAlarmS *pVMaskAlarm);

/*
 * 拷贝灰度图到参考帧缓冲区
 * 将外部传入的Y亮度缓冲区复制为遮挡检测的参考帧，
 * 并同步设置采样使能和计算参考帧清晰度。
 * @param YBuf 灰度图像数据指针
 * @return S_OK 参考帧更新成功
 */
int encode_Pre_OD_Copy_pImage(unsigned char *YBuf);

/*
 * 遮挡检测主运行函数
 * 每帧调用一次，执行完整的遮挡检测流程：
 *   1. 与上一帧对比检测图像变化
 *   2. 判断是否需要更新参考帧
 *   3. 与参考帧对比判断是否遮挡
 * @param frame   当前帧信息（含视频帧指针和上一帧拷贝）
 * @param ParamsOut 检测结果输出
 * @return TRUE 检测到遮挡，FALSE 无遮挡
 */
BOOL encode_Pre_OD_Run(myIMPFrameInfo_t *frame, PreODParamsOut_t *ParamsOut);

/*
 * 打印遮挡检测诊断信息
 * 输出当前检测结果和遮挡类型判断
 * @return S_OK
 */
int encode_Pre_OD_Print_Message(void);

/* 用于外部模块清除累计的检测数据，比如电机转动时 */
void encode_Pre_OD_Clear(void);

#ifdef __cplusplus
}
#endif
#endif
