/******************************************************************************
    Copyright (C), 2017-2027, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name   : encode_Pre_od.c
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2018-03-20   by cuiweixun
    Description : 嵌入式设备镜头遮挡检测核心实现
                 本模块实现基于图像对比的镜头遮挡检测算法，主要流程：
                 1. 将图像划分为40x20网格，以块为单位进行检测
                 2. 对比当前帧与参考帧，统计每个块内亮度变化像素数
                 3. 变化像素数超过阈值的块标记为“变化块"
                 4. 当变化块数量和遮挡区域尺寸满足条件时，判定为遮挡
                 5. 通过清晰度(FV)对比判断遮挡类型
    History     :
******************************************************************************/
#include "g_log.h"
#include "jconfstruct.h"
#include "encode_Pre_od.h"

//#define PreOD_Mask_debug1
//#define PreOD_Mask_debug2

/* 模块级全局实例：保存遮挡检测的所有状态、参数和缓存 */
static TEncodePre_OD gtEncodePreOD;

/*
 * encode_Pre_OD_Get_FV - 计算图像清晰度(Focus Value)
 *
 * 通过计算图像水平和垂直方向梯度乘积的累加和来量化图像清晰度。
 * 清晰度越高，说明图像细节越丰富；遮挡后清晰度通常下降。
 *
 * 计算公式：
 *   FV = Σ (|dx| * |dy|)
 *   其中：
 *     dx = Y[w+1] - Y[w]        水平梯度（相邻右像素亮度差）
 *     dy = Y[RAW_W+w] - Y[w]    垂直梯度（相邻下像素亮度差）
 *   采样间隔为4像素（水平和垂直方向均隔4采样一次），兼顾速度与精度。
 *
 * 为什么用 dx*dy 而不是 dx+dy：
 *   乘积对边缘像素更敏感——只有同时存在水平和垂直变化的点（即角点和边缘交叉处）
 *   才会产生大值，能更好反映图像的纹理丰富程度。
 *
 * @param YBuffer 灰度图像数据指针
 * @return 图像清晰度值（保证 >= 1，避免除零）
 */
static long encode_Pre_OD_Get_FV(unsigned char *YBuffer)
{
    unsigned int w = 0, h = 0, dx = 0, dy = 0;
    long FV = 0;

    /*
     * 遍历图像，每隔4个像素采样一次：
     *   - 外层循环：垂直方向每隔4行采样（h += 4）
     *   - 内层循环：水平方向每隔4列采样（w += 4）
     *   - YBuffer += 4*RAW_W：每次外层循环后指针偏移4行，与h += 4对应
     */
    for (h = 0; h < RAW_H - 1; h += 4) {
        for (w = 0; w < RAW_W - 1; w += 4) {
            /* dx：水平梯度 = 右邻像素亮度 - 当前像素亮度 */
            dx = abs(YBuffer[w + 1] - YBuffer[w]);
            /* dy：垂直梯度 = 下邻像素亮度 - 当前像素亮度（RAW_W为一行字节数） */
            dy = abs(YBuffer[RAW_W + w] - YBuffer[w]);
            /* 梯度乘积累加：水平和垂直方向都有变化时值最大，反映图像纹理丰富度 */
            FV += dx * dy;
        }
        /* [BUG-1修复] 指针偏移4行，与外层h+=4对应，确保均匀采样整幅图像 */
        YBuffer += 4 * RAW_W;
    }

    /* 保证FV >= 1，避免后续用作除数时产生除零错误 */
    if (FV == 0) {
        FV = 1;
    }

    return FV;
}

/*
 * encode_Pre_OD_Check_Image - 图像变化检测与块分布生成
 *
 * 将当前帧与指定参考帧（pImageCopy）逐块对比，统计每个块内亮度变化
 * 像素数，生成块分布映射表(ODBlockMap)，并计算遮挡区域的边界和中心。
 *
 * 算法步骤：
 *   1. 遍历40x20网格，每个网格块大小为 block_w x block_h 像素
 *   2. 在每个块内每隔2个像素采样，对比当前帧与参考帧的亮度差
 *   3. 亮度差 > dotY 的像素计为有效变化点
 *   4. 块内有效变化点数 > Dot 的块标记为1（变化块）
 *   5. 遍历块映射表，统计变化块总数、边界、中心坐标
 *
 * @param frame       当前帧信息
 * @param pImageCopy  参考帧灰度图（用于对比）
 * @return S_OK 检测完成，S_FAIL 模块未初始化
 */
static int encode_Pre_OD_Check_Image(myIMPFrameInfo_t * frame,
                                     unsigned char *pImageCopy)
{
    int linestep, line;
    unsigned char *tpImage = (unsigned char *)frame->Frame->video_frame.virt_addr[0]; /* 当前帧灰度数据指针 */
    unsigned char *tpImageCopy = pImageCopy;  /* 参考帧灰度数据指针 */
    unsigned char *ODBlockMap = &gtEncodePreOD.ImageBuf.ODBlockMap[0][0]; /* 块映射表指针 */
    int Cnt, Dot, Bw, Bh, x, y, w, h, x0, y0, xy0, temp;
    unsigned char aData0, aData1, dotY;

    /* 未初始化则直接返回失败 */
    if (!gtEncodePreOD.param_InPut.init) {
        return S_FAIL;
    }

    /* 工作区初始化：清零并设置边界极值，方便后续 min/max 更新 */
    memset(&gtEncodePreOD.param_Work, 0, sizeof gtEncodePreOD.param_Work);
    gtEncodePreOD.param_Work.M_obj_x_max = 0;       /* 变化块最大列号初始为0 */
    gtEncodePreOD.param_Work.M_obj_x_min = 0xffff;   /* 变化块最小列号初始为最大值，方便取小 */
    gtEncodePreOD.param_Work.M_obj_y_max = 0;       /* 变化块最大行号初始为0 */
    gtEncodePreOD.param_Work.M_obj_y_min = 0xffff;   /* 变化块最小行号初始为最大值，方便取小 */

    Bw = gtEncodePreOD.param_InPut.block_w;   /* 单块像素宽度 */
    Bh = gtEncodePreOD.param_InPut.block_h;   /* 单块像素高度 */
    dotY = gtEncodePreOD.param_InPut.sensitivity_dotY; /* 像素亮度差阈值 */
    Dot = gtEncodePreOD.param_InPut.sensitivity_block_Dot; /* 块内最小有效变化点数 */
    line = RAW_W;          /* 一行的字节数 */
    linestep = Bh * RAW_W; /* 一个网格块组（Bh行）的总字节数，用于行间指针跳转 */

    /* ====== 第一阶段：逐块检测图像变化，生成块分布映射表 ====== */

    pri_od(LVL_LOOP, "check image start\n");

    /*
     * 遍历 40x20 网格：
     *   外层循环(y)：行方向，每行一个网格块带
     *   内层循环(x)：列方向，每列一个网格块
     */
    for (y = 0; y < VMask_DETECTION_GRID_ROW; y++) {
        x0 = 0; /* 当前行内块的起始水平偏移（像素） */
        for (x = 0; x < VMask_DETECTION_GRID_COLUMN; x++) {
            y0 = 0;  /* 当前块内行偏移（像素） */
            Cnt = 0;  /* 块内有效变化点计数器 */

            /*
             * 在当前块内每隔2像素采样对比：
             *   h循环：块内垂直方向每隔2行
             *   w循环：块内水平方向每隔2列
             *   隔2采样是为了减少计算量，同时保持足够的检测精度
             *   [OPT-10优化] 当Cnt已超过Dot时提前退出，避免不必要的采样计算
             */
            for (h = 0; h < Bh; h += 2) {
                xy0 = x0 + y0; /* 当前块内采样点的行起始偏移 */

                for (w = 0; w < Bw; w += 2) {
                    temp = xy0 + w;         /* 采样点在图像缓冲区中的绝对偏移 */
                    aData0 = tpImageCopy[temp]; /* 参考帧该点亮度值 */
                    aData1 = tpImage[temp];     /* 当前帧该点亮度值 */

                    /* 像素亮度差超过阈值则计为有效变化点 */
                    if (abs(aData0 - aData1) > dotY) {
                        Cnt++;
                    }
                }

                /* [OPT-10优化] 有效点已超过阈值，无需继续采样本块剩余像素 */
                if (Cnt > Dot) {
                    break;
                }

                y0 += line; /* 块内行偏移移动一行（line = RAW_W） */
            }

            /*
             * 判定当前块是否为变化块：
             *   有效变化点数 > Dot → 标记为1（有变化）
             *   否则 → 标记为0（无变化）
             */
            if (Cnt > Dot) {
                ODBlockMap[x] = 1;
            } else {
                ODBlockMap[x] = 0;
            }

            x0 += Bw; /* 移动到同一行的下一个块 */
            pri_od(LVL_LOOP, "block map[%d] %d\n", x, ODBlockMap[x]);
        }

        /* 移动到下一行网格块带 */
        tpImage += linestep;      /* 当前帧指针下移Bh行 */
        tpImageCopy += linestep;  /* 参考帧指针下移Bh行 */
        ODBlockMap += VMask_DETECTION_GRID_COLUMN; /* 映射表指针移到下一行 */
        pri_od(LVL_LOOP, "detect row %d\n", y);
    }

    pri_od(LVL_LOOP, "check image end\n");

    /* ====== 第二阶段：统计块映射表，计算遮挡区域特征 ====== */

    ODBlockMap = &gtEncodePreOD.ImageBuf.ODBlockMap[0][0]; /* 重置映射表指针到起始位置 */

    /*
     * 遍历块映射表，对所有标记为1（变化）的块：
     *   1. 判断是否为孤立块（左右邻居均为0），累计孤立块计数
     *   2. 累加变化块的坐标，用于计算遮挡中心
     *   3. 更新遮挡区域的边界（x_min/x_max/y_min/y_max）
     */
    for (y = 0; y < VMask_DETECTION_GRID_ROW; y++) {
        for (x = 0; x < VMask_DETECTION_GRID_COLUMN; x++) {
            /* 跳过无变化块 */
            if (ODBlockMap[x] <= 0) {
                continue;
            }

            /*
             * 孤立块判定：当前块有变化，但左右邻居均无变化
             * 孤立块可能代表噪声或小范围干扰，与大面积遮挡区分
             */
            if (x == 0) {
                /* 最左列：只检查右侧邻居 */
                if (!ODBlockMap[x + 1]) {
                    gtEncodePreOD.param_Work.M_obj_Acnt++;
                }
            } else if (x == VMask_DETECTION_GRID_COLUMN - 1) {
                /* 最右列：只检查左侧邻居 */
                if (!ODBlockMap[x - 1]) {
                    gtEncodePreOD.param_Work.M_obj_Acnt++;
                }
            } else {
                /* 中间列：左右邻居均为0才算孤立 */
                if (!ODBlockMap[x - 1] && !ODBlockMap[x + 1]) {
                    gtEncodePreOD.param_Work.M_obj_Acnt++;
                }
            }

            /* 累加变化块坐标，用于计算遮挡中心位置 */
            gtEncodePreOD.param_Work.M_obj_x += x;
            gtEncodePreOD.param_Work.M_obj_y += y,
            gtEncodePreOD.param_Work.M_obj_cnt += 1; /* 变化块总数+1 */

            /* 更新遮挡区域边界：取变化块坐标的极值 */
            if (x > gtEncodePreOD.param_Work.M_obj_x_max) {
                gtEncodePreOD.param_Work.M_obj_x_max = x; /* 右边界 */
            }
            if (x < gtEncodePreOD.param_Work.M_obj_x_min) {
                gtEncodePreOD.param_Work.M_obj_x_min = x; /* 左边界 */
            }
            if (y > gtEncodePreOD.param_Work.M_obj_y_max) {
                gtEncodePreOD.param_Work.M_obj_y_max = y; /* 下边界 */
            }
            if (y < gtEncodePreOD.param_Work.M_obj_y_min) {
                gtEncodePreOD.param_Work.M_obj_y_min = y; /* 上边界 */
            }
        }

        ODBlockMap += VMask_DETECTION_GRID_COLUMN; /* 映射表指针移到下一行 */
    }

    return S_OK;
}

/*
 * encode_Pre_OD_Sample_pImage - 参考帧采样管理
 *
 * 管理何时更新参考帧（用于遮挡检测对比的基准图像）。
 * 参考帧的更新策略分为三种情况：
 *   1. 图像持续变化（如摄像头晃动）：每300帧(≈60秒)强制更新
 *   2. 图像持续静止（正常监控场景）：每20帧(≈4秒)更新
 *   3. 报警后快速重建：每5帧(≈1秒)更新，使系统快速恢复检测能力
 *
 * 采样条件（满足任一即更新参考帧）：
 *   - 静止计时器 >= 静止采样周期（条件2或3，取决于当前模式）
 *   - 变化计时器 >= 变化采样周期上限（条件1）
 *
 * @param frame 当前帧信息
 * @return S_OK 已更新参考帧，S_FAIL 未到采样时机
 */
static int encode_Pre_OD_Sample_pImage(myIMPFrameInfo_t * frame)
{
    int temp;

    /* 变化环境计时器+1：记录图像持续有变化的帧数 */
    gtEncodePreOD.param_InPut.M_obj_Sample_T_Max++;

    /*
     * 静止环境计时器更新：
     *   变化块数 <= 静止判定阈值 → 图像静止，计时器+1
     *   变化块数 >  静止判定阈值 → 图像有变化，计时器清零
     */
    if (gtEncodePreOD.param_Work.M_obj_cnt <=
        gtEncodePreOD.param_InPut.M_obj_Static_Image_Limit) {
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Min++;
    } else {
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Min = 0;
    }

    /*
     * 选择静止采样周期：
     *   检测模式(Sample_En=1)：使用LimitA(20帧≈4秒)，正常周期采样
     *   报警恢复模式(Sample_En=0)：使用LimitB(5帧≈1秒)，快速重建参考帧
     */
    if (gtEncodePreOD.param_InPut.M_obj_Sample_En) {
        temp = gtEncodePreOD.param_InPut.M_obj_Sample_T_Min_LimitA;
    } else {
        temp = gtEncodePreOD.param_InPut.M_obj_Sample_T_Min_LimitB;
    }

    /*
     * 判断是否到达采样时机（满足任一条件）：
     *   条件1: 静止计时器 >= 采样周期（图像静止足够长时间）
     *   条件2: 变化计时器 > 最大采样周期（图像持续变化太久，强制更新）
     */
    if (gtEncodePreOD.param_InPut.M_obj_Sample_T_Min >= temp ||
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Max >
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Max_Limit) {

        /* 将当前帧复制为新的参考帧 */
        memcpy(&gtEncodePreOD.ImageBuf.Od_SamplegYBuf,
               (void *)frame->Frame->video_frame.virt_addr[0], RAW_W * RAW_H);

        /* 重置计时器 */
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Min = 0;
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Max = 0;

        /* 切换到检测模式 */
        gtEncodePreOD.param_InPut.M_obj_Sample_En = 1;
        gtEncodePreOD.param_InPut.md_result_cnt = 0; /* 清零疑似遮挡计数 */

        /* 计算参考帧的清晰度，作为后续遮挡判断的基准 */
        gtEncodePreOD.param_InPut.M_obj_FVstatic =
            encode_Pre_OD_Get_FV((unsigned char *)frame->Frame->video_frame.virt_addr[0]);

        pri_od(LVL_DBG, "save image, fv: %ld\n", gtEncodePreOD.param_InPut.M_obj_FVstatic);

        return S_OK; /* 参考帧已更新 */
    }

    return S_FAIL; /* 未到采样时机 */
}

/*
 * encode_Pre_OD_check_Vmask - 遮挡判定核心逻辑
 *
 * 综合图像变化检测结果，进行多层筛选判断是否发生遮挡：
 *   层1: 检查前置条件（采样使能、变化块数是否触发检测）
 *   层2: 与参考帧对比，检查变化块数是否达到遮挡阈值
 *   层3: 检查遮挡区域尺寸是否满足最小阈值
 *   层4: 连续疑似遮挡确认（防误报）
 *   层5: 通过清晰度(FV)变化方向判断遮挡类型
 *
 * FV方向放大差异指数计算公式：
 *   obj_FV_dir = 100 * (FV_current - FV_static) / (min(FV_current, FV_static))
 *   分母使用较小值+1的用意：放大FV差异，使结果可超过±100，
 *   便于用较大阈值(±1000)区分遮挡类型：
 *     近距离遮挡时FV接近0 → 分母≈1 → 结果绝对值极大
 *     镜头变位时FV小幅变化 → 结果绝对值较小
 *
 *   obj_FV_dir > +FV_DIR_THRESHOLD_CLOSE_OCCLUSION: 近距离遮挡撤销（当前图像比参考帧清晰很多）
 *   obj_FV_dir < -FV_DIR_THRESHOLD_CLOSE_OCCLUSION: 近距离遮挡（当前图像比参考帧模糊很多）
 *   0 < obj_FV_dir <= FV_DIR_THRESHOLD_CLOSE_OCCLUSION: 镜头变位或远距离遮挡撤销
 *   -FV_DIR_THRESHOLD_CLOSE_OCCLUSION <= obj_FV_dir < 0: 镜头变位或远距离遮挡
 *
 * @param frame     当前帧信息
 * @param ParamsOut 检测结果输出
 * @return 1=遮挡报警，0=无遮挡
 */
static int encode_Pre_OD_check_Vmask(myIMPFrameInfo_t * frame,
                                     PreODParamsOut_t * ParamsOut)
{
    long temp;

    /* 调试日志：图像有变化时打印状态信息 */
    if (gtEncodePreOD.param_Work.M_obj_cnt >
        gtEncodePreOD.param_InPut.M_obj_Static_Image_Limit) {
        pri_od(LVL_DBG, "check_motion, [en: %1d, cnt_l: %3d, cnt_s: %2d, bcnt(%d): %3d]\n",
               gtEncodePreOD.param_InPut.M_obj_Sample_En,
               gtEncodePreOD.param_InPut.M_obj_Sample_T_Max,
               gtEncodePreOD.param_InPut.M_obj_Sample_T_Min,
               gtEncodePreOD.param_InPut.sensitivity_blocks_check,
               gtEncodePreOD.param_Work.M_obj_cnt);
    }

    /*
     * 前置条件检查（满足任一则跳过遮挡检测）：
     *   1. Sample_En == 0：尚未采样参考帧，无法对比
     *   2. 变化块数 < blocks_check 且 无疑似遮挡累计：变化不够显著
     */
    if ((gtEncodePreOD.param_InPut.M_obj_Sample_En == 0) ||
        (gtEncodePreOD.param_Work.M_obj_cnt <
         gtEncodePreOD.param_InPut.sensitivity_blocks_check &&
         !gtEncodePreOD.param_InPut.md_result_cnt)) {
        return gtEncodePreOD.param_OutPut.md_result;
    }

    /* 与参考帧对比，重新生成块映射表（之前是与上一帧对比） */
    encode_Pre_OD_Check_Image(frame, &gtEncodePreOD.ImageBuf.Od_SamplegYBuf[0]);

    pri_od(LVL_DBG, "check_vmask, [bcnt(%d): %d, rcnt: %d]\n",
           gtEncodePreOD.param_InPut.sensitivity_blocks_mask,
           gtEncodePreOD.param_Work.M_obj_cnt,
           gtEncodePreOD.param_InPut.md_result_cnt);

    /*
     * 遮挡有效块数检查：
     *   与参考帧对比的变化块数 < 遮挡阈值 → 不满足遮挡条件
     */
    if (gtEncodePreOD.param_Work.M_obj_cnt <
        gtEncodePreOD.param_InPut.sensitivity_blocks_mask) {
        return gtEncodePreOD.param_OutPut.md_result;
    }

    /* 计算遮挡中心坐标：所有变化块坐标的算术平均值 */
    gtEncodePreOD.param_OutPut.obj_x =
        gtEncodePreOD.param_Work.M_obj_x / gtEncodePreOD.param_Work.M_obj_cnt;

    gtEncodePreOD.param_OutPut.obj_y =
        gtEncodePreOD.param_Work.M_obj_y / gtEncodePreOD.param_Work.M_obj_cnt;

    /* 变化块总数 */
    gtEncodePreOD.param_OutPut.obj_cnt = gtEncodePreOD.param_Work.M_obj_cnt;

    /* 遮挡区域高度 = 最大行号 - 最小行号，最小 1（网格行数） */
    gtEncodePreOD.param_OutPut.obj_h = MAX(gtEncodePreOD.param_Work.M_obj_y_max -
                                           gtEncodePreOD.param_Work.M_obj_y_min, 1);

    /* 遮挡区域宽度 = 最大列号 - 最小列号，最小 1（网格列数） */
    gtEncodePreOD.param_OutPut.obj_w = MAX(gtEncodePreOD.param_Work.M_obj_x_max -
                                           gtEncodePreOD.param_Work.M_obj_x_min, 1);

    /*
     * 遮挡区域尺寸检查：
     *   宽度 >= 最小宽度阈值 且 高度 >= 最小高度阈值 → 疑似遮挡
     */
    if (gtEncodePreOD.param_OutPut.obj_w >=
        gtEncodePreOD.param_InPut.sensitivity_Size_w &&
        gtEncodePreOD.param_OutPut.obj_h >=
        gtEncodePreOD.param_InPut.sensitivity_Size_h) {

        /* 连续疑似遮挡计数+1 */
        gtEncodePreOD.param_InPut.md_result_cnt++;

        pri_od(LVL_DBG, "check_vmask:[bcnt(%d): %d, rcnt: %d]\n",
               gtEncodePreOD.param_InPut.sensitivity_blocks_mask,
               gtEncodePreOD.param_Work.M_obj_cnt,
               gtEncodePreOD.param_InPut.md_result_cnt);

        /*
         * 连续疑似遮挡确认：
         *   累计次数 >= 最小确认次数 → 确认遮挡报警
         *   多次确认是为了防止瞬间干扰（如飞虫、光影闪变）导致误报
         */
        if (gtEncodePreOD.param_InPut.md_result_cnt >=
            gtEncodePreOD.param_InPut.M_obj_Mask_Min_Limit) {

            /* 重置计数器和采样状态，报警后进入快速重建模式 */
            gtEncodePreOD.param_InPut.md_result_cnt = 0;
            gtEncodePreOD.param_InPut.M_obj_Sample_En = 0; /* 切换到报警恢复模式，1秒后重新采样 */

            /* 计算当前帧的清晰度 */
            gtEncodePreOD.param_InPut.M_obj_FV =
                encode_Pre_OD_Get_FV((unsigned char *)frame->Frame->video_frame.virt_addr[0]);

            /*
             * 计算FV方向放大差异指数的分母（防止除零）：
             *   取FV_current和FV_static中较小的值，最小 1
             *   公式：temp = min(FV_current, FV_static)
             *   用较小值作分母可放大FV差异，使阈值区分更明显
             */
            if (gtEncodePreOD.param_InPut.M_obj_FV >
                gtEncodePreOD.param_InPut.M_obj_FVstatic) {
                temp = MAX(gtEncodePreOD.param_InPut.M_obj_FVstatic, 1);
            } else {
                temp = MAX(gtEncodePreOD.param_InPut.M_obj_FV, 1);
            }

            /*
             * FV方向放大差异指数计算：
             *   obj_FV_dir = 100 * (FV_current - FV_static) / temp
             *
             * 该值反映图像清晰度变化方向和程度：
             *   正值：当前帧比参考帧更清晰（遮挡可能撤销）
             *   负值：当前帧比参考帧更模糊（可能被遮挡）
             *   绝对值越大变化越剧烈（分母小时放大效应明显）
             *
             * [ISSUE-7修复] 利用孤立块占比辅助遮挡类型判断：
             *   孤立块占比高 → 变化分散，更可能是噪声或镜头变位，抑制报警
             *   孤立块占比低 → 变化集中，更可能是真实遮挡，正常报警
             *   当孤立块占比超过50%时，认为是分散变化而非遮挡，复位报警计数
             */
            if (gtEncodePreOD.param_Work.M_obj_cnt > 0 &&
                gtEncodePreOD.param_Work.M_obj_Acnt * 2 >
                gtEncodePreOD.param_Work.M_obj_cnt) {
                /* 孤立块占比 > 50%：变化分散，更可能是噪声/变位而非遮挡 */
                pri_od(LVL_DBG, "scattered changes (isolated: %d/%d), suppress alarm\n",
                       gtEncodePreOD.param_Work.M_obj_Acnt,
                       gtEncodePreOD.param_Work.M_obj_cnt);
                gtEncodePreOD.param_InPut.md_result_cnt = 0;
                gtEncodePreOD.param_InPut.M_obj_Sample_En = 1; /* 保持检测模式，不进入报警恢复 */
                memcpy(ParamsOut, &gtEncodePreOD.param_OutPut, sizeof(PreODParamsOut_t));
                return gtEncodePreOD.param_OutPut.md_result;
            }

            gtEncodePreOD.param_OutPut.obj_FV_dir =
                (100 * (gtEncodePreOD.param_InPut.M_obj_FV -
                        gtEncodePreOD.param_InPut.M_obj_FVstatic)) / temp;

            /* 保存当前帧和参考帧的FV值到输出 */
            gtEncodePreOD.param_OutPut.obj_FV = gtEncodePreOD.param_InPut.M_obj_FV;
            gtEncodePreOD.param_OutPut.obj_FV_static =
                                          gtEncodePreOD.param_InPut.M_obj_FVstatic;

            /* 标记遮挡报警结果 */
            gtEncodePreOD.param_OutPut.md_result = 1;

            pri_od(LVL_DBG, "result [w(%d): %d, h(%d): %d] [fv0: %ld, fv: %ld, fv_dx: %ld%%]\n",
                   gtEncodePreOD.param_InPut.sensitivity_Size_w,
                   gtEncodePreOD.param_OutPut.obj_w,
                   gtEncodePreOD.param_InPut.sensitivity_Size_h,
                   gtEncodePreOD.param_OutPut.obj_h,
                   gtEncodePreOD.param_OutPut.obj_FV_static,
                   gtEncodePreOD.param_OutPut.obj_FV,
                   gtEncodePreOD.param_OutPut.obj_FV_dir);

            /* 打印块分布映射表（调试用） */
            int x, y;
            for (y = 0; y < VMask_DETECTION_GRID_ROW; y++) {
                for (x = 0; x < VMask_DETECTION_GRID_COLUMN; x++) {
                    if (get_g_log(od) & LVL_LOOP) {
                        printf("%1d ", (int )gtEncodePreOD.ImageBuf.ODBlockMap[y][x]);
                    }
                }

                if (get_g_log(od) & LVL_LOOP) {
                    printf("\n");
                }
            }
            if (get_g_log(od) & LVL_LOOP) {
                printf("\n");
            }
        }
    }

    /* 将输出参数拷贝给调用方 */
    memcpy(ParamsOut, &gtEncodePreOD.param_OutPut, sizeof(PreODParamsOut_t));

    return gtEncodePreOD.param_OutPut.md_result;
}

/*********************************************************************/
/*                           视频遮挡介绍                            */
/* 1、功能                                                           */
/*        A) 图像被 遮挡报警；                                       */
/*        B) 镜头位置突变报警；                                      */
/*        C) 图像遮档撤销报警；                                      */
/* 2、性能                                                           */
/*        A) 具有较强抗震能力；                                      */
/*        B) 具有较低的误报率和漏报率；                              */
/*        C) 具有较高的实时性；                                      */
/*        D) 具有较低的敏感性和较高的稳定性；                        */
/*        E) 具有渐进式遮挡检测能力；                                */
/*        F) 能简单判断性质: 近距离遮挡、近距离遮挡撤销、变位、其他。*/
/* 3、定义                                                           */
/*        A) 遮挡:  当图像被挡超过一定比例，发出报警；               */
/*        B) 变位:  当镜头被突然改变方向超出一定范围，发出报警；     */
/*        C) 撤销: 当遮挡被撤销，发出报警；                          */
/* 4、参数                                                           */
/*        A) 剧烈晃动: 60 秒更新；                                   */
/*        B) 静止环境: 4 秒更新；                                    */
/*        B) 报警后，静止环境: 1 秒更新；                            */
/*        C) 灵敏度: 分三大段，各占1/3 ，每段有微小变化。不敏感。    */
/*        D) 具有较低的敏感性和较高的稳定性；                        */
/* 5、注意事项                                                       */
/*        A) 测试变位报警时，应保持设备静止4 秒；                    */
/*        B) 测试遮挡报警时，应保持设备静止4 秒；                    */
/*        B) 测试遮挡后撤销遮挡报警时，应保持稳定遮挡2 秒；          */
/*        C) 剧烈晃动设备，很快报警一次，随后 约60 秒再报警。        */
/*        D) 设备置于桌面，敲动桌子，不应出现报警，或很低误报率。    */
/*        E) 开关环境灯光可能引发报警。                              */
/* 6、代码生效                                                       */
/*      定义: #define OD_New    标记，代码生效；建议关闭调试打印。   */
/*    建议移动侦测和遮挡代码同时生效。不支持部分变更。               */
/*********************************************************************/
/*
 * encode_Pre_OD_init - 遮挡检测模块初始化
 *
 * 根据用户配置的灵敏度阈值(0~100)计算各项检测参数：
 *   1. 像素亮度差阈值(dotY)：灵敏度越高，阈值越低，越容易检测到微小变化
 *   2. 块内最小有效点数(block_Dot)：每个网格块内需要多少变化像素才标记为变化块
 *   3. 触发遮挡检测的变化块数下限(blocks_check)：与上一帧对比时变化块数阈值
 *   4. 遮挡有效块数下限(blocks_mask)：与参考帧对比时变化块数阈值
 *
 * 灵敏度阈值分段策略：
 *   thresh > 66 (高灵敏度): Block_check=20%, Block_mask=90%, Block_Dots=7%
 *   thresh > 33 (中灵敏度): Block_check=25%, Block_mask=95%, Block_Dots=8%
 *   thresh <=33 (低灵敏度): Block_check=30%, Block_mask=100%, Block_Dots=9%
 *   灵敏度越高→变化判定越容易（点数阈值低）→遮挡面积判定越宽松（比例低）
 *
 * @param pVMaskAlarm 遮挡报警配置（含enable使能和thresh灵敏度阈值0~100）
 * @return S_OK 初始化成功
 */
int encode_Pre_OD_init(VMaskAlarmS * pVMaskAlarm)
{
    int Temp, Block_check = 10, Block_mask = 80, Block_Dots = 5;
    int Block_Size_w = 40, Block_Size_h = 40;

    pri_od(LVL_DBG, "od_mem: %d\n", sizeof(TEncodePre_OD));

    /* 保存遮挡报警配置 */
    memcpy(&gtEncodePreOD.tVMaskAlarm, pVMaskAlarm, sizeof(VMaskAlarmS));

    /* 首次初始化各项基础参数（只执行一次） */
    if (!gtEncodePreOD.param_InPut.init) {
        gtEncodePreOD.param_InPut.init = 1;                         /* 标记已初始化 */
        gtEncodePreOD.param_InPut.md_result_cnt = 0;               /* 连续疑似遮挡计数清零 */
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Max = 0;          /* 变化环境计时器清零 */
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Min = 0;          /* 静止环境计时器清零 */
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Max_Limit = 300;  /* 变化环境采样周期上限: 300*200ms=60秒 */
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Min_LimitA = 20;  /* 正常静止采样周期: 20*200ms=4秒 */
        gtEncodePreOD.param_InPut.M_obj_Sample_T_Min_LimitB = 5;   /* 报警后快速采样周期: 5*200ms=1秒 */
        gtEncodePreOD.param_InPut.M_obj_Mask_Min_Limit = 2;        /* 连续确认2次才报警，防误报 */
        gtEncodePreOD.param_InPut.M_obj_Static_Image_Limit = 3;    /* 变化块数<=3视为静止 */

        /* 计算每个网格块的像素尺寸（分辨率固定，只需计算一次） */
        gtEncodePreOD.param_InPut.block_w = RAW_W / VMask_DETECTION_GRID_COLUMN; /* 块宽(像素) */
        gtEncodePreOD.param_InPut.block_h = RAW_H / VMask_DETECTION_GRID_ROW;    /* 块高(像素) */
    }

    /* 灵敏度阈值范围限制在0~100 */
    if (gtEncodePreOD.tVMaskAlarm.thresh > 100) {
        gtEncodePreOD.tVMaskAlarm.thresh = 100;
    }
    if (gtEncodePreOD.tVMaskAlarm.thresh < 0) {
        gtEncodePreOD.tVMaskAlarm.thresh = 0;
    }

    /*
     * 像素亮度差阈值计算：
     *   dotY = 16 + (21-16) * (100 - thresh) / 100
     *   dotY = 16 + 5 * (100 - thresh) / 100
     *
     *   灵敏度thresh=100时: dotY=16 (最敏感，亮度差>16即算变化)
     *   灵敏度thresh=0时:   dotY=21 (最不敏感，亮度差>21才算变化)
     *   阈值越高→越不敏感→误报少但可能漏报
     */
    gtEncodePreOD.param_InPut.sensitivity_dotY =
        16 + ((21 - 16) * (100 - gtEncodePreOD.tVMaskAlarm.thresh)) / 100;

    /*
     * 灵敏度分段策略：根据阈值将灵敏度分为高/中/低三档
     *   高灵敏度(thresh>66): 检测更灵敏，少量变化即可触发
     *   中灵敏度(thresh>33): 平衡模式
     *   低灵敏度(thresh<=33): 更稳定，减少误报
     *
     *   Block_check:  触发遮挡检测的变化块比例（与上一帧对比）
     *   Block_mask:   遮挡有效块比例（与参考帧对比）
     *   Block_Dots:   块内有效变化点比例
     *   Block_Size_w: 遮挡区域最小宽度比例 [ISSUE-5修复] 原固定100%，现为灵敏度关联
     *   Block_Size_h: 遮挡区域最小高度比例 [ISSUE-5修复] 原固定100%，现为灵敏度关联
     */
    if (gtEncodePreOD.tVMaskAlarm.thresh > 66) {
        Block_check = 10;   /* 高灵敏度: 10%块变化即开始遮挡检测 */
        Block_mask = 80;    /* 高灵敏度: 80%块变化即确认遮挡 */
        Block_Dots = 5;     /* 高灵敏度: 块内5%像素变化即标记该块 */
        Block_Size_w = 40;  /* 高灵敏度: 遮挡宽度>=40%即视为有效尺寸 */
        Block_Size_h = 40;  /* 高灵敏度: 遮挡高度>=40%即视为有效尺寸 */
    } else if (gtEncodePreOD.tVMaskAlarm.thresh > 33) {
        Block_check = 15;   /* 中灵敏度: 15%块变化触发检测 */
        Block_mask = 85;    /* 中灵敏度: 85%块变化确认遮挡 */
        Block_Dots = 6;     /* 中灵敏度: 块内6%像素变化标记 */
        Block_Size_w = 60;  /* 中灵敏度: 遮挡宽度>=60%即视为有效尺寸 */
        Block_Size_h = 60;  /* 中灵敏度: 遮挡高度>=60%即视为有效尺寸 */
    } else {
        Block_check = 20;   /* 低灵敏度: 20%块变化才触发检测 */
        Block_mask = 90;    /* 低灵敏度: 90%块变化才确认遮挡 */
        Block_Dots = 7;     /* 低灵敏度: 块内7%像素变化才标记 */
        Block_Size_w = 80;  /* 低灵敏度: 遮挡宽度>=80%（全画面） */
        Block_Size_h = 80;  /* 低灵敏度: 遮挡高度>=80%（全画面） */
    }

    /*
     * 块内最小有效点数计算：
     *   公式: block_Dot = (block_w * block_h * Block_Dots) / 100
     *   即块内总像素数 × 有效点比例
     */
    Temp = (gtEncodePreOD.param_InPut.block_w * gtEncodePreOD.param_InPut.block_h *
           Block_Dots) / 100;

    gtEncodePreOD.param_InPut.sensitivity_block_Dot = Temp; /* 块内最小有效点数 */

    /*
     * 触发遮挡检测的变化块数下限：
     *   公式: blocks_check = (40 * 20 * Block_check) / 100
     *   即总网格数(800) × 触发检测比例
     */
    gtEncodePreOD.param_InPut.sensitivity_blocks_check =
        (VMask_DETECTION_GRID_COLUMN * VMask_DETECTION_GRID_ROW * Block_check) / 100;

    /*
     * 遮挡有效块数下限：
     *   公式: blocks_mask = (40 * 20 * Block_mask) / 100
     *   即总网格数(800) × 遮挡确认比例
     */
    gtEncodePreOD.param_InPut.sensitivity_blocks_mask =
        (VMask_DETECTION_GRID_COLUMN * VMask_DETECTION_GRID_ROW * Block_mask) / 100;

    /*
     * [ISSUE-5+6修复] 遮挡区域最小尺寸阈值：
     *   公式: Size = (总网格数 * 百分比) / 100
     *   移出init守卫，使灵敏度变更时尺寸阈值同步更新
     *   高灵敏度: 60%即可检测部分遮挡
     *   中灵敏度: 80%覆盖才算有效
     *   低灵敏度: 100%全画面覆盖（保守策略）
     */
    gtEncodePreOD.param_InPut.sensitivity_Size_w =
        (VMask_DETECTION_GRID_COLUMN * Block_Size_w) / 100;
    gtEncodePreOD.param_InPut.sensitivity_Size_h =
        (VMask_DETECTION_GRID_ROW * Block_Size_h) / 100;

    pri_od(LVL_DBG, "vmaskcfg enable: %d, thresh: %d\n",
           gtEncodePreOD.tVMaskAlarm.enable, gtEncodePreOD.tVMaskAlarm.thresh);
    pri_od(LVL_DBG, "vmaskcfg sens doty: %2d, blocks start: %2d, mask: %2d, dots: %2d\n",
           gtEncodePreOD.param_InPut.sensitivity_dotY,
           gtEncodePreOD.param_InPut.sensitivity_blocks_check,
           gtEncodePreOD.param_InPut.sensitivity_blocks_mask,
           gtEncodePreOD.param_InPut.sensitivity_block_Dot);
    pri_od(LVL_DBG, "vmaskcfg block[w: %2d, h=%2d]\n",
           gtEncodePreOD.param_InPut.block_w, gtEncodePreOD.param_InPut.block_h);

    return S_OK;
}

/*
 * encode_Pre_OD_Print_Message - 打印遮挡检测结果诊断信息
 *
 * 输出遮挡报警状态和遮挡类型判断：
 *   obj_FV_dir > +1000:  近距离遮挡撤销（图像变清晰很多，遮挡物被移除）
 *   obj_FV_dir < -1000:  近距离遮挡（图像变模糊很多，镜头被近距离遮挡）
 *   0 < obj_FV_dir <= 1000:  镜头变位 或 远距离遮挡撤销
 *   -1000 <= obj_FV_dir < 0: 镜头变位 或 远距离遮挡
 *
 * 判断逻辑解释：
 *   近距离遮挡→FV大幅下降（模糊）→obj_FV_dir负值且很大
 *   远距离遮挡→FV小幅下降（轻微模糊）→obj_FV_dir小幅负值
 *   近距离遮挡撤销→FV大幅上升（恢复清晰）→obj_FV_dir正值且很大
 *   镜头变位→FV可能增减（场景完全不同）→obj_FV_dir小幅变化
 *
 * @return S_OK
 */
int encode_Pre_OD_Print_Message(void)
{
    pri_od(LVL_DBG, "vmask alarm: (result: %1d, cnt: %3d)\n",
           gtEncodePreOD.param_OutPut.md_result, gtEncodePreOD.param_OutPut.obj_cnt);

    pri_od(LVL_DBG, "[w: %2d, h: %2d], fv[x0: %ld, x: %ld, dx: %ld%%]\n",
           gtEncodePreOD.param_OutPut.obj_w, gtEncodePreOD.param_OutPut.obj_h,
           gtEncodePreOD.param_OutPut.obj_FV_static, gtEncodePreOD.param_OutPut.obj_FV,
           gtEncodePreOD.param_OutPut.obj_FV_dir);

    /* 根据FV方向放大差异指数判断遮挡类型（使用宏定义阈值，便于适配不同分辨率） */
    if (gtEncodePreOD.param_OutPut.obj_FV_dir > FV_DIR_THRESHOLD_CLOSE_OCCLUSION) {
        pri_od(LVL_DBG, "close up occlusion cancelled!\n");   /* 近距离遮挡撤销 */
    } else if (gtEncodePreOD.param_OutPut.obj_FV_dir < -FV_DIR_THRESHOLD_CLOSE_OCCLUSION) {
        pri_od(LVL_DBG, "blocked by close range!\n");         /* 近距离遮挡 */
    } else if (gtEncodePreOD.param_OutPut.obj_FV_dir > 0) {
        pri_od(LVL_DBG, "lens transposed! or remote occlusion cancelled!\n");  /* 镜头变位或远距离遮挡撤销 */
    } else {
        pri_od(LVL_DBG, "lens transposed! or blocked at a distance!\n");      /* 镜头变位或远距离遮挡 */
    }

    return S_OK;
}

/*
 * encode_Pre_OD_Copy_pImage - 拷贝灰度图作为参考帧
 *
 * 将外部提供的灰度图像数据复制到模块内部的参考帧缓冲区，
 * 并同步更新采样使能和参考帧清晰度，使后续遮挡检测可正常执行。
 * 通常在系统启动或需要强制更新参考帧时调用。
 *
 * [ISSUE-4修复] 补全了Sample_En和FVstatic的设置，
 * 确保调用后模块立即进入可检测状态。
 *
 * @param YBuf 源灰度图像数据（RAW_W * RAW_H字节）
 * @return S_OK 参考帧更新成功
 */
int encode_Pre_OD_Copy_pImage(unsigned char *YBuf)
{
    memcpy(&gtEncodePreOD.ImageBuf.Od_SamplegYBuf, YBuf, RAW_W * RAW_H);

    /* 标记采样使能：参考帧已就绪，后续帧可开始遮挡检测 */
    gtEncodePreOD.param_InPut.M_obj_Sample_En = 1;

    /* 计算参考帧清晰度，作为后续遮挡类型判定的基准 */
    gtEncodePreOD.param_InPut.M_obj_FVstatic =
        encode_Pre_OD_Get_FV(YBuf);

    /* 清零疑似遮挡计数，防止残留状态干扰 */
    gtEncodePreOD.param_InPut.md_result_cnt = 0;

    pri_od(LVL_DBG, "copy image as reference, fv: %ld\n",
           gtEncodePreOD.param_InPut.M_obj_FVstatic);

    return S_OK;
}

/*
 * encode_Pre_OD_Run - 遮挡检测主运行函数（每帧调用一次）
 *
 * 执行完整的遮挡检测三步流程：
 *   步骤1: 与上一帧对比检测图像变化（使用copygYBuf）
 *          生成块映射表，计算变化块统计信息
 *   步骤2: 判断是否需要更新参考帧（采样时机管理）
 *          如果到了采样时机，更新参考帧后返回FALSE（本帧不继续检测）
 *   步骤3: 与参考帧对比判断遮挡（使用Od_SamplegYBuf）
 *          通过多层筛选判断是否发生遮挡
 *
 * 流程图：
 *   输入帧 → Check_Image(与上一帧对比) → Sample_pImage(采样管理) → check_Vmask(遮挡判定) → 输出
 *
 * @param frame     当前帧信息（含视频帧指针和上一帧拷贝copygYBuf）
 * @param ParamsOut 检测结果输出
 * @return TRUE 检测到遮挡，FALSE 无遮挡
 */
BOOL encode_Pre_OD_Run(myIMPFrameInfo_t *frame, PreODParamsOut_t *ParamsOut)
{
    int ret = S_OK;

    /* 清零输出参数 */
    memset(&gtEncodePreOD.param_OutPut, 0, sizeof gtEncodePreOD.param_OutPut);

    /*
     * 步骤1: 与上一帧灰度图(copygYBuf)对比，检测图像变化
     * copygYBuf由调用方在每帧开始时拷贝上一帧数据
     * 如果模块未初始化，返回S_FAIL，本函数返回FALSE
     */
    ret = encode_Pre_OD_Check_Image(frame, (unsigned char *)&frame->copygYBuf[0]);
    if (ret == S_FAIL) {
        return FALSE;
    }

    /*
     * 步骤2: 参考帧采样管理
     * 如果到达采样时机，更新参考帧后返回S_OK
     * 此时本帧不继续遮挡判定（新参考帧刚建立，需要等待下一帧对比）
     */
    ret = encode_Pre_OD_Sample_pImage(frame);
    if (ret == S_OK) {
        return FALSE;
    }

    /*
     * 步骤3: 遮挡判定
     * 与参考帧对比，通过多层筛选判断是否遮挡
     * 返回1=遮挡报警，0=无遮挡
     */
    ret = encode_Pre_OD_check_Vmask(frame, ParamsOut);
    if (ret) {
        return TRUE;   /* 检测到遮挡 */
    } else {
        return FALSE;  /* 无遮挡 */
    }
}

void encode_Pre_OD_Clear(void)
{
    gtEncodePreOD.param_InPut.M_obj_Sample_T_Min = 0;
    gtEncodePreOD.param_InPut.M_obj_Sample_T_Max = 0;
    gtEncodePreOD.param_InPut.md_result_cnt = 0;
    gtEncodePreOD.param_InPut.M_obj_Sample_En = 0;
}
