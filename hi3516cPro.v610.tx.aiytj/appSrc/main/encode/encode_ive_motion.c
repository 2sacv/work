#include "ot_type.h"
#include "ot_common_svp.h"
#include "ot_common_video.h"
#include "ot_common_md.h"
#include "ot_ivs_md.h"

#include "ss_mpi_vpss.h"
#include "ss_mpi_ive.h"
#include "ss_mpi_sys_mem.h"
#include "securec.h"

#include "debug.h"
#include "js_scheduler.h"
#include "encode_common.h"
#include "encode_ive_motion.h"
#include "alarmapi.h"

#define MOTION_DETECTION_GRID_COLUMN        22
#define MOTION_DETECTION_GRID_ROW           18

#define OT_IVE_MD_CHN          0

#define OT_IVE_MD_IMAGE_NUM          2
#define OT_IVE_MD_MILLIC_SEC         100

#define OT_IVE_IMAGE_CHN_TWO         2
#define OT_IVE_IMAGE_CHN_THREE       3

#define OT_IVE_MD_ADD_X_VAL          32768
#define OT_IVE_MD_ADD_Y_VAL          32768

#define OT_IVE_MD_AREA_THR_STEP      8
#define OT_IVE_MD_VPSS_CHN           2
#define OT_IVE_MD_NUM_TWO            2
#define OT_IVE_SAD_THRESHOLD         100

#define OT_MD_RECT_NUM               64
#define OT_POINT_NUM                 4
#define OT_MAX_LOOP_IMG_H            3
#define OT_IVE_DIV_TWO               2
#define OT_POINT_IDX_ZERO            0
#define OT_POINT_IDX_ONE             1
#define OT_POINT_IDX_TWO             2
#define OT_POINT_IDX_THREE           3
#define OT_ADDR_IDX_ZERO             0
#define OT_ADDR_IDX_ONE              1
#define OT_ADDR_IDX_TWO              2

#define OT_IVE_QUERY_SLEEP   100
#define OT_IVE_MAX_WIDTH     4096
#define OT_IVE_MAX_HEIGHT    4096

#define OT_IVE_ALIGN         16

#define encode_ive_convert_ptr_to_addr(type, addr) ((type)(td_uintptr_t)(addr))
#define encode_ive_convert_addr_to_ptr(type, addr) ((type *)(td_uintptr_t)(addr))

#define md_mmz_free(phys, virt)                                                 \
do {                                                                                    \
    if (((phys) != 0) && ((virt) != 0)) {                                               \
        ss_mpi_sys_mmz_free((td_phys_addr_t)(phys), (td_void*)(td_uintptr_t)(virt));    \
        (phys) = 0;                                                                     \
        (virt) = 0;                                                                     \
    }                                                                                   \
} while (0)

typedef struct {
    ot_point point[OT_POINT_NUM];
} ot_md_rect;

typedef struct {
    td_u16 num;
    ot_md_rect rect[OT_MD_RECT_NUM];
    td_u32 ids[OT_MD_RECT_NUM];
} ot_md_rect_info;

typedef struct {
    int cur_idx;
    BOOL is_first_frm;
    ot_rect md_border;
    ot_svp_src_img img[OT_IVE_MD_IMAGE_NUM];
    ot_svp_dst_mem_info blob;
    ot_md_attr md_attr;
    ot_md_rect_info region;
    struct timespec time;
} ot_ive_md_info;

typedef struct {
    td_u32 ele_size; /* element size */
    td_u32 loop_c;  /* loop times of c dimension */
    td_u32 loop_h[OT_SVP_IMG_ADDR_NUM]; /* loop times of h dimension */
} ot_rw_image_loop_info;

typedef struct {
    ot_size src;
    ot_size dst;
} ot_src_dst_size;


static ot_ive_md_info g_md_info = {0};

int change_motion_time_interval(struct timespec time)
{
    g_md_info.time.tv_nsec = time.tv_nsec;
    g_md_info.time.tv_sec  = time.tv_sec;

    return 0;
}

static int encode_ive_md_set_area(char *mbdesc, ot_rect *md_param)
{
    if (mbdesc == NULL || md_param == NULL) {
        return -1;
    }

    int w = 0, h = 0;
    int mbdesc_x1 = 100, mbdesc_y1 = 100, mbdesc_x2 = -1, mbdesc_y2 = -1;

    for (h = 0; h < MOTION_DETECTION_GRID_ROW; h ++) {	//18
        for (w = 0; w < MOTION_DETECTION_GRID_COLUMN; w ++) {	//22
            if ('1' == mbdesc[ h * (MOTION_DETECTION_GRID_COLUMN + 1) + w]) {	//mbdesc[h*(22+1) +w]
                if (mbdesc_x1 > w)
                    mbdesc_x1 = w;
                if (mbdesc_x2 < w)
                    mbdesc_x2 = w;

                if (mbdesc_y1 > h)
                    mbdesc_y1 = h;
                if (mbdesc_y2 < h)
                    mbdesc_y2 = h;
            }
        }
    }

    if (100 == mbdesc_x1)
        mbdesc_x1 = 0;
    if (100 == mbdesc_y1)
        mbdesc_y1 = 0;

    if(mbdesc_x1 < 0) mbdesc_x1 = 0;
    if(mbdesc_y1 < 0) mbdesc_y1 = 0;
    if(mbdesc_x2 < 0) mbdesc_x2 = 0;
    if(mbdesc_y2 < 0) mbdesc_y2 = 0;

    DBG("[mbdesc_x1:%d, mbdesc_y1:%d]; [mbdesc_x2:%d, mbdesc_y2:%d]\n", mbdesc_x1, mbdesc_y1, mbdesc_x2, mbdesc_y2);

    td_u32  u32X1 = 0, u32Y1 =0, u32X2 = 0, u32Y2 = 0; 
    u32X1 = ALIGN_DOWN(mbdesc_x1*RAW_W/MOTION_DETECTION_GRID_COLUMN,32);
    u32Y1 = ALIGN_DOWN(mbdesc_y1*RAW_H/MOTION_DETECTION_GRID_ROW,32);
    u32X2 = ALIGN_DOWN(mbdesc_x2*RAW_W/MOTION_DETECTION_GRID_COLUMN,32);
    u32Y2 = ALIGN_DOWN(mbdesc_y2*RAW_H/MOTION_DETECTION_GRID_ROW,32);

    md_param->x = u32X1;
    md_param->y = u32Y1;
    md_param->width = ALIGN_DOWN(abs(u32X2 - u32X1), 2);
    md_param->height = ALIGN_DOWN(abs(u32Y2 - u32Y1), 2);

    DBG("[X:%d, Y:%d] [W:%d, H:%d]\n", md_param->x,md_param->y,md_param->width,md_param->height);

    return 0;
}

/*
 * function : judge if rect is valid
 */
static td_void encode_ive_md_is_rect_valid(ot_ive_md_info *md_info, td_u32 num, td_bool *valid)
{
    td_u32 j = 0, k = 0;
    for (j = 0; j < (OT_POINT_NUM - 1); j++) {
        for (k = j + 1; k < OT_POINT_NUM; k++) {
            if ((md_info->region.rect[num].point[j].x == md_info->region.rect[num].point[k].x) &&
                (md_info->region.rect[num].point[j].y == md_info->region.rect[num].point[k].y)) {
                *valid = TD_FALSE;
                break;
            }
        }
    }

    if(TD_TRUE == *valid) {
        for (k = 0; k < OT_POINT_NUM; k++) {
            if ((md_info->region.rect[num].point[k].x < md_info->md_border.x) ||
            (md_info->region.rect[num].point[k].x > (md_info->md_border.x + md_info->md_border.width)) ||
            (md_info->region.rect[num].point[k].y < md_info->md_border.y) ||
            (md_info->region.rect[num].point[k].y > (md_info->md_border.y + md_info->md_border.height))) {
                *valid = TD_FALSE;
                break;
            }
        }
    }
}

static td_void encode_ive_md_get_thresh(ot_ive_ccblob *blob, td_u16 area_thr_step, td_u16 rect_max_num,
    td_u32 *thresh)
{
    td_u32 i = 0;
    td_u16 num = 0;
    td_u32 thr = blob->info.bits.cur_area_threshold;
    do {
        num = 0;
        thr += area_thr_step;
        for (i = 0; i < blob->info.bits.rgn_num; i++) {
            if (blob->rgn[i].area > thr) {
                num++;
            }
        }
    } while (num > rect_max_num);

    *thresh = thr;
}

static td_s32 encode_ive_md_check_src_dst(ot_src_dst_size *src_dst_size)
{
    if(src_dst_size->src.width == 0){
        ERR("src width can't be 0\n");
        return OT_ERR_IVE_ILLEGAL_PARAM;
    }

    if(src_dst_size->src.height == 0){
        ERR("src height can't be 0\n");
        return OT_ERR_IVE_ILLEGAL_PARAM;
    }

    if(src_dst_size->dst.width == 0){
        ERR("dst width can't be 0\n");
        return OT_ERR_IVE_ILLEGAL_PARAM;
    }

    if(src_dst_size->dst.height == 0){
        ERR("dst height can't be 0\n");
        return OT_ERR_IVE_ILLEGAL_PARAM;
    }

    return TD_SUCCESS;
}

/*
 * function : Copy blob to rect
 */
 
td_s32 encode_ive_md_blob_to_rect(ot_ive_ccblob *blob, ot_ive_md_info *ive_md,
    td_u16 rect_max_num, td_u16 area_thr_step, ot_src_dst_size *src_dst_size)
{
    td_u16 num = 0, i = 0;
    td_s32 ret = 0;
    td_u32 thr = 0;
    td_bool valid = 0;

    if(blob == TD_NULL){
        ERR("blob can't be null\n");
        return OT_ERR_IVE_NULL_PTR;
    }

    if(ive_md == TD_NULL){
        ERR("ive_md can't be null\n");
        return OT_ERR_IVE_NULL_PTR;
    }

    ret = encode_ive_md_check_src_dst(src_dst_size);
    if(ret != TD_SUCCESS){
        ERR("sample_common_ive_check_src_dst failed\n");
        return OT_ERR_IVE_ILLEGAL_PARAM;
    }

    if (blob->info.bits.rgn_num > rect_max_num) {
        encode_ive_md_get_thresh(blob, area_thr_step, rect_max_num, &thr);
    }

    for (i = 0; i < blob->info.bits.rgn_num; i++) {
        if (blob->rgn[i].area <= thr) {
            continue;
        }
        
        if(num > (OT_MD_RECT_NUM - 1)){
            ERR("num is larger than %u\n", OT_MD_RECT_NUM - 1);
            return TD_FAILURE;
        }
   
        ive_md->region.rect[num].point[OT_POINT_IDX_ZERO].x = (td_u32)((td_float)blob->rgn[i].left /
            (td_float)src_dst_size->src.width * (td_float)src_dst_size->dst.width) & (~1);
        ive_md->region.rect[num].point[OT_POINT_IDX_ZERO].y = (td_u32)((td_float)blob->rgn[i].top /
            (td_float)src_dst_size->src.height * (td_float)src_dst_size->dst.height) & (~1);

        ive_md->region.rect[num].point[OT_POINT_IDX_ONE].x = (td_u32)((td_float)blob->rgn[i].right /
            (td_float)src_dst_size->src.width * (td_float)src_dst_size->dst.width) & (~1);
        ive_md->region.rect[num].point[OT_POINT_IDX_ONE].y = (td_u32)((td_float)blob->rgn[i].top /
            (td_float)src_dst_size->src.height * (td_float)src_dst_size->dst.height) & (~1);

        ive_md->region.rect[num].point[OT_POINT_IDX_TWO].x = (td_u32)((td_float)blob->rgn[i].right /
            (td_float)src_dst_size->src.width * (td_float)src_dst_size->dst.width) & (~1);
        ive_md->region.rect[num].point[OT_POINT_IDX_TWO].y = (td_u32)((td_float)blob->rgn[i].bottom /
            (td_float)src_dst_size->src.height * (td_float)src_dst_size->dst.height) & (~1);

        ive_md->region.rect[num].point[OT_POINT_IDX_THREE].x = (td_u32)((td_float)blob->rgn[i].left /
            (td_float)src_dst_size->src.width * (td_float)src_dst_size->dst.width) & (~1);
        ive_md->region.rect[num].point[OT_POINT_IDX_THREE].y = (td_u32)((td_float)blob->rgn[i].bottom /
            (td_float)src_dst_size->src.height * (td_float)src_dst_size->dst.height) & (~1);

        valid = TD_TRUE;
        encode_ive_md_is_rect_valid(ive_md, num, &valid);
        if (valid == TD_TRUE) {
            num++;
        }
    }

    ive_md->region.num = num;
#if 0
    DBG("ive_md->region.num:%d\n", ive_md->region.num);
    for(i = 0; i < ive_md->region.num; i++) {
        for(int j = 0; j < OT_POINT_NUM; j++) {
            DBG("rect->rect[%d].point[%d].x:%d\n", i, j, ive_md->region.rect[i].point[j].x);
            DBG("rect->rect[%d].point[%d].y:%d\n", i, j, ive_md->region.rect[i].point[j].y);
        }
    }
#endif
    return TD_SUCCESS;
}

static td_void encode_ive_md_set_src_dst_size(ot_src_dst_size *src_dst, td_u32 src_width,
    td_u32 src_height, td_u32 dst_width, td_u32 dst_height)
{
    src_dst->src.width = src_width;
    src_dst->src.height = src_height;
    src_dst->dst.width = dst_width;
    src_dst->dst.height = dst_height;
}

/*
 * function : Dma frame info to ive image
 */
td_s32 encode_ive_md_dma_image(ot_video_frame_info *frame_info, ot_svp_dst_img *dst,
    td_bool is_instant)
{
    td_s32 ret = OT_ERR_IVE_NULL_PTR;
    ot_ive_handle handle;
    ot_svp_src_data src_data;
    ot_svp_dst_data dst_data;
    ot_ive_dma_ctrl ctrl = { OT_IVE_DMA_MODE_DIRECT_COPY, 0, 0, 0, 0 };
    td_bool is_finish = TD_FALSE;
    td_bool is_block = TD_TRUE;

    do {
        if(frame_info == TD_NULL){
            ERR("frame_info can't be null\n");
            return TD_FAILURE;
        }

        if(dst == TD_NULL){
            ERR("dst can't be null\n");
            return TD_FAILURE;
        }

        if(frame_info->video_frame.virt_addr == TD_NULL){
            ERR("frame_info->video_frame.virt_addr can't be null\n");
            return TD_FAILURE;
        }

        ret = OT_ERR_IVE_ILLEGAL_PARAM;

        if(frame_info->video_frame.phys_addr == 0){
            ERR("frame_info->video_frame.virt_addr can't be 0\n");
            return TD_FAILURE;
        }

        if(dst->virt_addr == 0){
            ERR("dst->virt_addr can't be 0\n");
            return TD_FAILURE;
        }

        if(dst->phys_addr == 0){
            ERR("dst->phys_addr can't be 0\n");
            return TD_FAILURE;
        }
   
        /* fill src */
        src_data.virt_addr = encode_ive_convert_ptr_to_addr(td_u64, frame_info->video_frame.virt_addr[0]);
        src_data.phys_addr = frame_info->video_frame.phys_addr[0];
        src_data.width = frame_info->video_frame.width;
        src_data.height = frame_info->video_frame.height;
        src_data.stride = frame_info->video_frame.stride[0];

        /* fill dst */
        dst_data.virt_addr = dst->virt_addr[0];
        dst_data.phys_addr = dst->phys_addr[0];
        dst_data.width = dst->width;
        dst_data.height = dst->height;
        dst_data.stride = dst->stride[0];

        ret = ss_mpi_ive_dma(&handle, &src_data, &dst_data, &ctrl, is_instant);
        ENCODE_RET_BREAK(ret, "ss_mpi_ive_dma failed\n");
        
        if (is_instant == TD_TRUE) {
            ret = ss_mpi_ive_query(handle, &is_finish, is_block);
            while (ret == OT_ERR_IVE_QUERY_TIMEOUT) {
                usleep(OT_IVE_QUERY_SLEEP);
                ret = ss_mpi_ive_query(handle, &is_finish, is_block);
            }
            ENCODE_RET_BREAK(ret, "ss_mpi_ive_query failed\n");
        }
    } while(0);
    return TD_SUCCESS;
}

/* first frame just init reference frame, if not, change the frame idx */
static td_s32 encode_ive_md_dma_data(td_u32 cur_idx, ot_video_frame_info *frm,
    ot_ive_md_info *md_ptr, td_bool *is_first_frm)
{
    td_s32 ret = 0;
    td_bool is_instant = TD_TRUE;
    
    do {
        if (*is_first_frm != TD_TRUE) {
            ret = encode_ive_md_dma_image(frm, &md_ptr->img[cur_idx], is_instant);
            ENCODE_RET_BREAK(ret, "encode_ive_md_dma_image failed\n");
        } else {
            ret = encode_ive_md_dma_image(frm, &md_ptr->img[1 - cur_idx], is_instant);
            ENCODE_RET_BREAK(ret, "encode_ive_md_dma_image failed\n");
            *is_first_frm = TD_FALSE;
        }
    } while(0);

    return ret;
}

void encode_ive_md_process(MotionDetectS *mdinfo)
{
    td_s32 ret = 0;
    int vpss_grp = 0;
    td_s32 vpss_chn = OT_VPSS_CHN2;
    td_bool idx_chnage = TD_FALSE;
    td_bool frames[OT_IVE_MD_VPSS_CHN] = {TD_FALSE, TD_FALSE};
    ot_ive_md_info *md_ptr = (ot_ive_md_info *)(&g_md_info);
    ot_video_frame_info frm[OT_IVE_MD_VPSS_CHN];
    ot_src_dst_size src_dst = {0};

    static int md_counter = 0;
    static BOOL md_time   = FALSE;
    static struct timespec md_time_pre = {0};

    do {
        md_counter++;
        if(md_counter > 5) {
            md_counter = 0;
            md_time = TimeJudge(mdinfo->times);
        }

        if(FALSE == md_time) {
            md_ptr->is_first_frm = TRUE;
            ret = -1;
            break;
        }

        ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frm[1], OT_IVE_MD_MILLIC_SEC);
        if(TD_SUCCESS != ret) {
            break;
        }
        //ENCODE_RET_BREAK(ret, "ss_mpi_vpss_get_chn_frame failed\n");
        frames[OT_VPSS_CHN1] = TD_TRUE;

        ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frm[0], OT_IVE_MD_MILLIC_SEC);
        if(TD_SUCCESS != ret ) {
            break;
        }
        //ENCODE_RET_BREAK(ret, "ss_mpi_vpss_get_chn_frame failed\n");
        frames[OT_VPSS_CHN0] = TD_TRUE;

        ret = encode_ive_md_dma_data(md_ptr->cur_idx, &frm[1], md_ptr, &md_ptr->is_first_frm);
        ENCODE_RET_BREAK(ret, "encode_ive_md_dma_data failed\n");
        
        /* change idx */
        if (md_ptr->is_first_frm == TD_TRUE) {
            idx_chnage = TD_TRUE;
            break;
        }

        ret = ot_ivs_md_proc(0, &md_ptr->img[md_ptr->cur_idx], &md_ptr->img[1 - md_ptr->cur_idx], TD_NULL, &md_ptr->blob);
        if(ret != TD_SUCCESS) {
            ERR("ivs_md_proc fail,Err(%#x)\n", ret);
   
         
            idx_chnage = TD_TRUE;
            break;
        }

        encode_ive_md_set_src_dst_size(&src_dst, md_ptr->md_attr.width, md_ptr->md_attr.height,
        frm[0].video_frame.width, frm[0].video_frame.height);

        ret = encode_ive_md_blob_to_rect(encode_ive_convert_addr_to_ptr(ot_ive_ccblob, md_ptr->blob.virt_addr),
                                        md_ptr, OT_MD_RECT_NUM, OT_IVE_MD_AREA_THR_STEP, &src_dst);

        ENCODE_RET_BREAK(ret, "encode_ive_md_blob_to_rect failed\n");
        idx_chnage = TD_TRUE;
    } while(0);

    if(TD_TRUE == idx_chnage) {
        md_ptr->cur_idx = 1 - md_ptr->cur_idx;
    }

    if(TD_TRUE == frames[OT_VPSS_CHN1]) {
        ret = ss_mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frm[1]);
        ENCODE_RET_CHECK(ret, "ss_mpi_vpss_release_chn_frame failed\n");
    }

    if(TD_TRUE == frames[OT_VPSS_CHN0]) {
        ret = ss_mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frm[0]);
        ENCODE_RET_CHECK(ret, "ss_mpi_vpss_release_chn_frame failed\n");
    }

    if (md_ptr->region.num) {
        md_time_pre.tv_nsec = MAX(md_time_pre.tv_nsec, g_md_info.time.tv_nsec);
        md_time_pre.tv_sec = MAX(md_time_pre.tv_sec, g_md_info.time.tv_sec);
        if (ms_clock_is_timeup(&md_time_pre, 2000)) {
            ENCODE_RET_JUDGE(alarm_report(JALARM_TYPE_MD, 0, NEED_TIME_CHECK, "motion detect"));
        }
    }
}

td_s32 encode_ive_md_create_mem_info(ot_svp_mem_info *mem_info, td_u32 size)
{
    td_s32 ret = 0;
    td_void *virt_addr = TD_NULL;

    do {
        if(mem_info == TD_NULL) {
            ret = TD_FAILURE;
            ERR("mem_info can't be null\n");
            break;
        }

        mem_info->size = size;
        ret = ss_mpi_sys_mmz_alloc((td_phys_addr_t *)&mem_info->phys_addr, (td_void **)&virt_addr, TD_NULL, TD_NULL, size);
        ENCODE_RET_BREAK(ret, "ss_mpi_sys_mmz_alloc failed\n");
        mem_info->virt_addr = encode_ive_convert_ptr_to_addr(td_u64, virt_addr);
    } while(0);

    return ret;
}

td_u32 encode_ive_md_calc_stride(td_u32 width, td_u8 align)
{
    td_u32 ret = OT_ERR_IVE_ILLEGAL_PARAM;

    do {
        if(align == 0) {
            ERR("align can't be 0\n");
            ret = OT_ERR_IVE_ILLEGAL_PARAM;
            break;
        }

        if(width > OT_IVE_MAX_WIDTH || width < 1) {
            ERR("width(%u) must be in [1, %u]\n", width, OT_IVE_MAX_WIDTH);
            ret = OT_ERR_IVE_ILLEGAL_PARAM;
            break;
        }
        ret = (width + (align - width % align) % align);
    } while(0);
   
    return ret;
}

static td_void encode_ive_md_get_loop_info(const ot_svp_img *img, ot_rw_image_loop_info *loop_info)
{
    loop_info->ele_size = 1;
    loop_info->loop_c = 1;
    loop_info->loop_h[0] = img->height;
    switch (img->type) {
        case OT_SVP_IMG_TYPE_U8C1:
        case OT_SVP_IMG_TYPE_S8C1:
            break;
        case OT_SVP_IMG_TYPE_YUV420SP:
            loop_info->ele_size = 1;
            loop_info->loop_c = OT_IVE_IMAGE_CHN_TWO;
            loop_info->loop_h[1] = img->height / OT_IVE_DIV_TWO;
            break;
        case OT_SVP_IMG_TYPE_YUV422SP:
            loop_info->loop_c = OT_IVE_IMAGE_CHN_TWO;
            loop_info->loop_h[1] = img->height;
            break;
        case OT_SVP_IMG_TYPE_U8C3_PACKAGE:
            loop_info->ele_size = (td_u32)(sizeof(td_u8) + sizeof(td_u16));
            break;
        case OT_SVP_IMG_TYPE_U8C3_PLANAR:
            loop_info->loop_c = OT_IVE_IMAGE_CHN_THREE;
            loop_info->loop_h[1] = img->height;
            loop_info->loop_h[OT_IVE_IMAGE_CHN_TWO] = img->height;
            break;
        case OT_SVP_IMG_TYPE_S16C1:
        case OT_SVP_IMG_TYPE_U16C1:
            loop_info->ele_size = (td_u32)sizeof(td_u16);
            break;
        case OT_SVP_IMG_TYPE_U32C1:
        case OT_SVP_IMG_TYPE_S32C1:
            loop_info->ele_size = (td_u32)sizeof(td_u32);
            break;
        case OT_SVP_IMG_TYPE_S64C1:
        case OT_SVP_IMG_TYPE_U64C1:
            loop_info->ele_size = (td_u32)sizeof(td_u64);
            break;
        default:
            break;
    }
}

static td_s32 encode_ive_md_set_image_addr(ot_svp_img *img, const ot_rw_image_loop_info *loop_info,
    td_bool is_mmz_cached)
{
    td_u32 c = 0;
    td_u32 size = 0;
    td_s32 ret = 0;
    td_void *virt_addr = TD_NULL;
    do {
        for (c = 0; (c < loop_info->loop_c) && (c < OT_MAX_LOOP_IMG_H) && (c < OT_SVP_IMG_STRIDE_NUM); c++) {
            size += img->stride[0] * loop_info->loop_h[c] * loop_info->ele_size;
            img->stride[c] = img->stride[0];
        }

        if (is_mmz_cached == TD_FALSE) {
            ret = ss_mpi_sys_mmz_alloc((td_phys_addr_t *)&img->phys_addr[0], (td_void **)&virt_addr,
                                        TD_NULL, TD_NULL, size);
        } else {
            ret = ss_mpi_sys_mmz_alloc_cached((td_phys_addr_t *)&img->phys_addr[0], (td_void **)&virt_addr,
                                               TD_NULL, TD_NULL, size);
        }
        
        if(ret != TD_SUCCESS) {
            ERR("mmz malloc fail\n");
            break;
        }

        img->virt_addr[OT_ADDR_IDX_ZERO] = encode_ive_convert_ptr_to_addr(td_u64, virt_addr);

        if (img->type != OT_SVP_IMG_TYPE_U8C3_PACKAGE) {
            for (c = 1; (c < loop_info->loop_c) && (c < OT_MAX_LOOP_IMG_H) && (c < OT_SVP_IMG_STRIDE_NUM); c++) {
                img->phys_addr[c] = img->phys_addr[c - 1] + img->stride[c - 1] * img->height;
                img->virt_addr[c] = img->virt_addr[c - 1] + img->stride[c - 1] * img->height;
            }
        } else {
            img->virt_addr[OT_ADDR_IDX_ONE] = img->virt_addr[OT_ADDR_IDX_ZERO] + 1;
            img->virt_addr[OT_ADDR_IDX_TWO] = img->virt_addr[OT_ADDR_IDX_ONE] + 1;
            img->phys_addr[OT_ADDR_IDX_ONE] = img->phys_addr[OT_ADDR_IDX_ZERO] + 1;
            img->phys_addr[OT_ADDR_IDX_TWO] = img->phys_addr[OT_ADDR_IDX_ONE] + 1;
        }
    }while(0);

    return ret;
}

td_s32 encode_ive_md_create_image(ot_svp_img *img, ot_svp_img_type type, td_u32 width, td_u32 height)
{
    td_s32 ret = OT_ERR_IVE_ILLEGAL_PARAM;
    ot_rw_image_loop_info loop_info = {0};

    do {
        if(img == TD_NULL) {
            ERR("img can't be null\n");
            break;
        }

        if(type < 0 || type >= OT_SVP_IMG_TYPE_BUTT) {
            ERR("type(%u) must be in [0, %u)!\n", type, OT_SVP_IMG_TYPE_BUTT);
            break;
        }

        if(width > OT_IVE_MAX_WIDTH) {
            ERR("width(%u) must be in [1, %u]!\n", width, OT_IVE_MAX_WIDTH);
            break;
        }

        if(height > OT_IVE_MAX_HEIGHT) {
            ERR("height(%u) must be in [1, %u]!\n", height, OT_IVE_MAX_HEIGHT);
            break;
        }

        img->type = type;
        img->width = width;
        img->height = height;
        img->stride[0] = encode_ive_md_calc_stride(img->width, OT_IVE_ALIGN);

        switch (type) {
        case OT_SVP_IMG_TYPE_U8C1:
        case OT_SVP_IMG_TYPE_S8C1:
        case OT_SVP_IMG_TYPE_YUV420SP:
        case OT_SVP_IMG_TYPE_YUV422SP:
        case OT_SVP_IMG_TYPE_S16C1:
        case OT_SVP_IMG_TYPE_U16C1:
        case OT_SVP_IMG_TYPE_U8C3_PACKAGE:
        case OT_SVP_IMG_TYPE_S32C1:
        case OT_SVP_IMG_TYPE_U32C1:
        case OT_SVP_IMG_TYPE_S64C1:
        case OT_SVP_IMG_TYPE_U64C1: {
            encode_ive_md_get_loop_info(img, &loop_info);
            ret = encode_ive_md_set_image_addr(img, &loop_info, TD_FALSE);
            ENCODE_RET_CHECK(ret, "sample_comm_ive_set_image_addr failed\n");
            break;
        }
        case OT_SVP_IMG_TYPE_YUV420P:
            break;
        case OT_SVP_IMG_TYPE_YUV422P:
            break;
        case OT_SVP_IMG_TYPE_S8C2_PACKAGE:
            break;
        case OT_SVP_IMG_TYPE_S8C2_PLANAR:
            break;
        case OT_SVP_IMG_TYPE_U8C3_PLANAR:
            break;
        default:
            break;
        }
    } while(0);

    return ret;
}

static td_s32 encode_ive_motion_param_init(MotionDetectS *mdinfo, ot_ive_md_info *md_inf_ptr)
{
    td_s32 ret = OT_ERR_IVE_NULL_PTR;
    td_u32 size = 0, sad_mode = 0;
    td_u8 wnd_size = 0;

    do {
        if(NULL == mdinfo ||NULL == md_inf_ptr) {
            ERR("md_inf_ptr is null\n");
            break;
        }

        md_inf_ptr->cur_idx = 0;
        md_inf_ptr->is_first_frm = TRUE;
        int thresh = (int)((100 - mdinfo->thresh)/100.0*6000);
        encode_ive_md_set_area(mdinfo->mbdesc, &md_inf_ptr->md_border);

        for (int i = 0; i < OT_IVE_MD_IMAGE_NUM; i++) {
            ret = encode_ive_md_create_image(&md_inf_ptr->img[i], OT_SVP_IMG_TYPE_U8C1, RAW_W, RAW_H);
            ENCODE_RET_BREAK(ret, "Create blob mem info failed %d\n", i);
        }

        ENCODE_RET_BREAK(ret, "Create blob mem info failed\n");
        
        size = (td_u32)sizeof(ot_ive_ccblob);
        ret = encode_ive_md_create_mem_info(&md_inf_ptr->blob, size);
        ENCODE_RET_BREAK(ret, "encode_ive_md_create_mem_info failed\n");
        
        /* Set attr info */
        md_inf_ptr->md_attr.alg_mode = OT_MD_ALG_MODE_BG;
        md_inf_ptr->md_attr.sad_mode = OT_IVE_SAD_MODE_MB_4X4;
        md_inf_ptr->md_attr.sad_out_ctrl = OT_IVE_SAD_OUT_CTRL_THRESHOLD;
        
        md_inf_ptr->md_attr.sad_threshold = (thresh == 0) ? 50 : thresh;
        md_inf_ptr->md_attr.width = RAW_W;
        md_inf_ptr->md_attr.height = RAW_H;
        md_inf_ptr->md_attr.add_ctrl.x = OT_IVE_MD_ADD_X_VAL;
        md_inf_ptr->md_attr.add_ctrl.y = OT_IVE_MD_ADD_Y_VAL;
        md_inf_ptr->md_attr.ccl_ctrl.mode = OT_IVE_CCL_MODE_4C;
        sad_mode = (td_u32)md_inf_ptr->md_attr.sad_mode;
        wnd_size = (1 << (OT_IVE_MD_NUM_TWO + sad_mode));
        md_inf_ptr->md_attr.ccl_ctrl.init_area_threshold = wnd_size * wnd_size;
        md_inf_ptr->md_attr.ccl_ctrl.step = wnd_size;
    } while(0);

    return ret;
}

int encode_ive_md_set_param(MotionDetectS *mdinfo)
{
    int ret = 0;
    ot_md_chn md_chn = OT_IVE_MD_CHN; 
    ot_md_attr md_attr = {0};

    do {
        ret = ot_ivs_md_get_chn_attr(md_chn, &md_attr);
        ENCODE_RET_BREAK(ret, "ot_ivs_md_get_chn_attr failed\n");
        int thresh = (int)((100 - mdinfo->thresh)/100.0*6000);

        md_attr.sad_threshold = (thresh == 0) ? 50 : thresh;
        encode_ive_md_set_area(mdinfo->mbdesc, &g_md_info.md_border);

        ret = ot_ivs_md_set_chn_attr(md_chn, &md_attr);
        ENCODE_RET_BREAK(ret, "ot_ivs_md_set_chn_attr failed\n");
    } while(0);

    return ret;
}

int encode_ive_md_init(MotionDetectS *mdinfo)
{
    DBG("%s\n", __func__);

    int ret = 0;

    do {
        ret = ot_ivs_md_init();
        ENCODE_RET_BREAK(ret, "ot_ivs_md_init failed\n");

        ret = encode_ive_motion_param_init(mdinfo, &g_md_info);
        ENCODE_RET_BREAK(ret, "encode_ive_motion_param_init failed\n");

        ret = ot_ivs_md_create_chn(OT_IVE_MD_CHN, &g_md_info.md_attr);
        ENCODE_RET_BREAK(ret, "ot_ivs_md_create_chn failed\n");
    } while(0);

    return ret;
}

int encode_ive_md_uninit(void)
{
    int ret = 0;

    ret = ot_ivs_md_destroy_chn(OT_IVE_MD_CHN);
    ENCODE_RET_CHECK(ret, "ot_ivs_md_destroy_chn failed\n");
  
    for (int i = 0; i < OT_IVE_MD_IMAGE_NUM; i++) {
        md_mmz_free(g_md_info.img[i].phys_addr[0], g_md_info.img[i].virt_addr[0]);
    }

    md_mmz_free(g_md_info.blob.phys_addr, g_md_info.blob.virt_addr);

    memset_s(&g_md_info, sizeof(g_md_info), 0, sizeof(g_md_info));

    ret = ot_ivs_md_exit();
    ENCODE_RET_CHECK(ret, "ot_ivs_md_exit failed\n");

    return ret;
}
