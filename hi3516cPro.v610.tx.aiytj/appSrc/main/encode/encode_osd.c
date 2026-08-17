/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_osd_video.c
 * @Created Time : 2021-04-13
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ot_type.h"
#include "ot_common.h"
#include "ot_common_isp.h"
#include "ot_common_region.h"
#include "ss_mpi_region.h"
#include "ot_common_video.h"

#include "jconfstruct.h"
#include "debug.h"
#include "g_log.h"

#include "js_scheduler.h"
#include "conf_list.h"
#include "jconfig.h"
#include "system_ctrl.h"
#include "encode_common.h"
#include "encode_osd.h"
#include "encode_freetype.h"
#include "confapi.h"
#include "encode_region.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_awb.h"


#define ALIGN_OSD(x) ((x%2 == 0)?x:x+1)
#define OSD_FONT_PATH    "/algo/simhei.ttf"
#define CLUT2_BYTES(w)   (((w) + 3) >> 2)

#define OSD_ZOOM_STARTX        (912)
#define OSD_ZOOM_STARTY        (1016)

#define OSD_CANVAS_NUM        (2)
#define TIME_UPDATE_COUNT     (5)
#define STREAM_UPDATE_COUNT   (4)
#define ZOOM_UPDATE_COUNT     (10)

#define OSD_OVERLAY_LAYER   (5)
#define OSD_MERGE_INTERVAL   (30)

typedef enum {
    E_OSD_DATE_FORMAT0  = 0,   //2020年1月1日
    E_OSD_DATE_FORMAT1,        //2020年1月1日
    E_OSD_DATE_FORMAT2,        //2020-1-1
    E_OSD_DATE_FORMAT3,        //1-1-2020
    E_OSD_DATE_FORMAT4,        //2020/1/1
    E_OSD_DATE_FORMAT5,        //1/1/2020
    E_OSD_DATE_FORMAT6,        //12:12 2020年1月1日
    E_OSD_DATE_FORMAT7,        //12:12 2020-1-1
    E_OSD_DATE_FORMAT8         //12:12 2020/1/1
} OsdDateFormatE;


typedef struct {
    OsdGroupE     encGroup;
    DWORD         dwAddFps;
    DWORD         dwAddBits;
} StreamInfoS;

typedef struct {
    OsdTypeE       osdType;
    int            language;
    int            showWeek;
    OsdDateFormatE dateFmt;
    DWORD          dwUncLen;
    DWORD          dwUncBuf[MAX_LATTICE_NUMBER];
    int            startX;
    int            startY;
    DWORD          dwWidth;
    DWORD          dwHeight;
    UCHAR          *pcBuffer;
    TFont          *ptFont;
    ot_rgn_handle  rgnHandle;
    ot_rgn_type    rgnType;
    ot_mpp_chn     rgnChn;
    BOOL           bNeedSet;
    BOOL           bShowFlag;
    OsdGroupE      osdGroup;
    int            bpsenable;
    int            mergeNum;
    BOOL           osdregion;
} OsdWindowS;

typedef struct {
    DWORD               dwFps;
    DWORD               dwBits;
    DWORD               dwFpsMax;
    struct timespec     lastFrameTs;
} OsdMediaS;

typedef struct {
    TFont         tFont;
    OsdWindowS    win[E_OSD_TEXT_MAX];
} OsdTextS;


typedef struct {
    BOOL          bAvail;   // OSD功能
    OsdMediaS     media;    // 统计媒体信息
    VideoEnc0     venc;     // 编码参数

    OsdTextS      text;     // 画文本
} EncodeOsdS;

struct osd_cfg{
    OsdInfoS     osdinfo;
    OsdStyleS    osdstyle;
    OsdExpandS   osdexpand;
    osd_zoom_t   osdzoom;
};

struct osd_run {
    JSScheduler     sch;
    JSTCHandle      hdl_loop;

    unsigned int expand_chg_flag;
    struct cmdstat *p_ctx;
};

static struct osd_cfg cfg = {{0}};
static struct osd_cfg raw = {{0}};
static struct osd_run run = {NULL};
static struct osd_cfg *g_osd_cfg = &cfg;
static struct osd_cfg *g_osd_raw = &raw;
static struct osd_run *g_osd_run = &run;

static EncodeOsdS g_osd[E_OSD_GROUP_MAX] = {{0,}};

static int g_blink_cnt = -1;

void encode_osd_process_zoom(void);

float encode_osd_video_get_x_ratio(int group)
{
    return encode_get_ve_x_ratio(encode_vencsize_to_idx(g_osd[group].venc.vencsize));
}

float encode_osd_video_get_y_ratio(int group)
{
    return encode_get_ve_y_ratio(encode_vencsize_to_idx(g_osd[group].venc.vencsize));
}

float encode_osd_draw_get_x_ratio(int group)
{
    return encode_get_x_ratio(encode_vencsize_to_idx(g_osd[group].venc.vencsize), RAW_W);
}

float encode_osd_draw_get_y_ratio(int group)
{
    return encode_get_y_ratio(encode_vencsize_to_idx(g_osd[group].venc.vencsize), RAW_H);
}

static void encode_osd_change_font_size(int venc, int size, int *w, int *h)
{
    int tWidth = *w;
    int tHeight = *h;

    if (venc > VideoIdxE_VGA) {
        switch(size) {
            case OSD_SIZE_S:
                tWidth  *= 0.8;
                tHeight *= 0.8;
                break;
            case OSD_SIZE_M:
                tWidth  *= 1.0;
                tHeight *= 1.0;
                break;
            case OSD_SIZE_L:
                tWidth  *= 1.08;
                tHeight *= 1.08;
                break;
            default:
                break;
        }
    } else {
        tWidth  = 10;
        tHeight = 10;
    }

    *w = ENC_GET2MULTIPLE(tWidth);
    *h = ENC_GET2MULTIPLE(tHeight);
}

int encode_osd_set_style(int group, OsdStyleS *ptOsdStyle, int size)
{
    TFont *ptFont = NULL;
    int width = 0, height = 0;

    ptFont = &g_osd[group].text.tFont;
    strcpy(ptFont->cFontFileName, OSD_FONT_PATH);
    height = ptOsdStyle->height * encode_osd_video_get_y_ratio(group);
    switch (group) {
        case E_OSD_GROUP_MAIN:
            width = ptOsdStyle->width * encode_osd_video_get_x_ratio(group);
            break;
        case E_OSD_GROUP_SUB:
            width = ptFont->dwFontWidth;
            break;
        default:
            break;
    }

    encode_osd_change_font_size(encode_vencsize_to_idx(g_osd[group].venc.vencsize), size, &width, &height);

    ptFont->dwFontWidth = width;
    ptFont->dwFontHeight = height;
    ptFont->dwFontColor = ptOsdStyle->colormode;
    dbg_osd("Font W:%d H:%d Color:%d \n", ptFont->dwFontWidth, ptFont->dwFontHeight, ptFont->dwFontColor);

    return S_OK;
}

static int osd_string_to_unicode(char *pString, USHORT *pUnicode, DWORD *pdwLen)
{
    unsigned char *p = NULL;
    unsigned short stmp;
    int i, j, valid_len;

    if (NULL == pString || NULL == pUnicode) {
        ERR("invalid parameters.\n");
        return -1;
    }

    valid_len = strlen(pString);

    p = (unsigned char *)pString;
    i = j = 0;
    /*utf8 规则
     *首字节:
     *      单字节字: < 0xC0            0000 0000 ~ 1100 0000
     *      双字节字: > 0xC0 && < 0xE0  1100 0000 ~ 1110 0000
     *      三字节字: > 0xE0            1110 0000 ~ 1111 0000
     */
    while(i < valid_len) {
        if (p[i] < 0x80) {  //ascll
            stmp = p[i];
            i++;
        } else if (p[i] > 0xC0 && p[i] < 0xE0) { //双字节有效，俄文用双字节编码
            /* utf8 to unicode */
            stmp = 0x07ff;
            stmp &= (((p[i] | 0xe0) << 6) | 0xe03f);
            stmp &= (p[i + 1] | 0xffc0);

            i += 2;
        } else {
            /* utf8 to unicode */
            stmp = 0xffff;
            stmp &= (((p[i] | 0xf0) << 12) | 0x0fff);
            stmp &= (((p[i + 1] | 0xc0) << 6) | 0xf03f);
            stmp &= (p[i + 2] | 0xffc0);

            i += 3;
        }

        pUnicode[j] = stmp;
        j += 1;
    }
    *pdwLen = j;
    return 0;
}

static int osd_string_get_lattice(OsdWindowS *pOsdWin, char *pString)
{
    int i = 0;
    int ret = S_OK;
    USHORT Unicode[MAX_LATTICE_NUMBER] = {0};
    DWORD dwUnicodeLen = 0;
    TLattice tLattice = {{NULL},};
    TFont *ptFont = pOsdWin->ptFont;
    DWORD dwWidth = 0;
    UCHAR *ptBufDel = NULL;
    TRect tSrcRect = {0};
    TRect tDesRect = {0};
    int max_x = encode_osd_video_get_x_ratio(pOsdWin->osdGroup)*P1080_WIDTH;
    int max_y = encode_osd_video_get_y_ratio(pOsdWin->osdGroup)*P1080_HEIGHT;

    ret = osd_string_to_unicode(pString, Unicode, &dwUnicodeLen);
    ENCODE_RET_JUDGE(ret);

    /* 第一次转化为unicode不做memcmp */
    if (0 != pOsdWin->dwUncLen && 0 == memcmp(pOsdWin->dwUncBuf,Unicode, pOsdWin->dwUncLen*sizeof(DWORD))) {
        return 1;
    }

    memcpy(pOsdWin->dwUncBuf, Unicode, MAX_LATTICE_NUMBER);
    pOsdWin->dwUncLen = dwUnicodeLen;

    dwWidth = 0;
    for (i=0; i<dwUnicodeLen; i++) {
        ret = encode_freetype_get_lattice(ptFont, Unicode[i], &tLattice);
        ENCODE_RET_JUDGE(ret);
        dwWidth += tLattice.dwWidth;
    }

    if (dwWidth > pOsdWin->dwWidth) {
        ptBufDel = pOsdWin->pcBuffer;
        pOsdWin->dwWidth = dwWidth;

        if ((pOsdWin->dwWidth+pOsdWin->startX) > max_x) {
            pOsdWin->startX = max_x - pOsdWin->dwWidth;
            if(pOsdWin->startX % 2 != 0) pOsdWin->startX -= 1;
        }
        if ((pOsdWin->dwHeight+pOsdWin->startY) > max_y) {
            pOsdWin->startY = max_y - pOsdWin->dwHeight;
        }
        if (pOsdWin->startX < 0) {
            pOsdWin->dwWidth = max_x / ptFont->dwFontWidth * ptFont->dwFontWidth;
            pOsdWin->startX = 0;
        }
        if (pOsdWin->startY < 0) {
            pOsdWin->startY = 0;
        }

        pOsdWin->startX = ALIGN_OSD(pOsdWin->startX);
        pOsdWin->startY = ALIGN_OSD(pOsdWin->startY);
        //DBG("OSD dwWidth:%d dwHeight:%d maxX,Y:%d,%d\n",ptOSDWin->dwWidth,ptOSDWin->dwHeight,max_x,max_y);
        //DBG("OSD name:%s dwUnicodeLen:%d \n",pString,dwUnicodeLen);
        //DBG("Font W:%d H:%d dwX:%d dwY:%d \n",ptFont->dwFontWidth,ptFont->dwFontHeight,ptOSDWin->dwX,ptOSDWin->dwY);
        //DBG("OSD len:%d\n", pOsdWin->dwWidth*pOsdWin->dwHeight*2);
        //DBG("OSD dwWidth:%d dwHeight:%d maxX,Y:%d,%d\n",pOsdWin->dwWidth,pOsdWin->dwHeight,max_x,max_y);
        pOsdWin->pcBuffer = system_malloc(pOsdWin->dwWidth * pOsdWin->dwHeight / 4);

        ENCODE_NULL_JUDGE(pOsdWin->pcBuffer);
        if (NULL == pOsdWin->pcBuffer) {
            pOsdWin->pcBuffer = ptBufDel;
            return S_FAIL;
        }

        if (NULL != ptBufDel) {
            pOsdWin->bNeedSet = TRUE;
            system_free(ptBufDel);
            ptBufDel = NULL;
        }
    }

    if (NULL != pOsdWin->pcBuffer) {
        memset(pOsdWin->pcBuffer, 0, (pOsdWin->dwWidth * pOsdWin->dwHeight / 4));
    }

    dwWidth = 0;
    for (i = 0; i < dwUnicodeLen; i++) {
        ret = encode_freetype_get_lattice(ptFont,Unicode[i],&tLattice);
        ENCODE_RET_JUDGE(ret);

        if (tLattice.pcBuffer) {
            tSrcRect.dwWidth = tLattice.dwWidth;
            tSrcRect.dwHeight = tLattice.dwHeight;
            tSrcRect.dwPitch = tLattice.dwWidth / 4;
            tSrcRect.dwOffX = 0;
            tSrcRect.dwOffY = 0;
            tSrcRect.pcBuffer = tLattice.pcBuffer;

            tDesRect.dwWidth = pOsdWin->dwWidth;
            tDesRect.dwHeight = pOsdWin->dwHeight;
            tDesRect.dwPitch = pOsdWin->dwWidth / 4;
            tDesRect.dwOffX = dwWidth;
            tDesRect.dwOffY = 0;
            tDesRect.pcBuffer = pOsdWin->pcBuffer;

            encode_memory_2d_copy(&tDesRect,&tSrcRect, 3);
        }

        dwWidth += tLattice.dwWidth;
    }

    return ret;
}

static int encode_osd_create_text(OsdGroupE osdGroup, OsdWindowS *pOsdWin)
{
    int ret = S_OK;
    ot_rgn_attr rgn_attr = {0};
    ot_rgn_chn_attr chn_attr = {0};
    chn_attr.is_show = TD_TRUE;

    switch (pOsdWin->rgnType) {
        case OT_RGN_OVERLAY:
            rgn_attr.type = OT_RGN_OVERLAY;
            rgn_attr.attr.overlay.canvas_num = OSD_CANVAS_NUM;
            rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_CLUT2;
            rgn_attr.attr.overlay.bg_color = 0;
            rgn_attr.attr.overlay.clut[0] = COLOR_ARGB_TRANSPARENT;
            rgn_attr.attr.overlay.clut[1] = COLOR_ARGB_WHITE;
            rgn_attr.attr.overlay.clut[2] = COLOR_ARGB_BLACK;
            rgn_attr.attr.overlay.clut[3] = COLOR_ARGB_RED;
            if(pOsdWin->mergeNum) {
                int osd_interval = OSD_MERGE_INTERVAL;
                if(E_OSD_GROUP_SUB == pOsdWin->osdGroup) {
                    osd_interval = OSD_MERGE_INTERVAL/3;
                }
                rgn_attr.attr.overlay.size.height = ALIGN_UP((pOsdWin->dwHeight + osd_interval) * pOsdWin->mergeNum, 2);
            } else {
                rgn_attr.attr.overlay.size.height = ALIGN_UP(pOsdWin->dwHeight, 2);
            }

            rgn_attr.attr.overlay.size.width = ALIGN_UP(pOsdWin->dwWidth, 2);

            chn_attr.type = OT_RGN_OVERLAY;
            chn_attr.attr.overlay_chn.bg_alpha = 0;
            chn_attr.attr.overlay_chn.fg_alpha = 255;
            chn_attr.attr.overlay_chn.qp_info.enable = TD_TRUE;
            chn_attr.attr.overlay_chn.qp_info.is_abs_qp = TD_TRUE;
            chn_attr.attr.overlay_chn.qp_info.qp_val = 30;
            chn_attr.attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;
            chn_attr.attr.overlay_chn.qp_info.enable = TD_FALSE;
            chn_attr.attr.overlay_chn.point.x = ALIGN_UP(pOsdWin->startX, 2);
            chn_attr.attr.overlay_chn.point.y = ALIGN_UP(pOsdWin->startY, 2);
            if(E_OSD_TIME == pOsdWin->osdType) {
                chn_attr.attr.overlay_chn.layer = osdGroup+OSD_OVERLAY_LAYER+1;
            } else {
                chn_attr.attr.overlay_chn.layer = osdGroup+OSD_OVERLAY_LAYER;
            }
            break;
        default:
            ERR("unknown type:%d\n", pOsdWin->rgnType);
            break;
    }

    ret = ss_mpi_rgn_create(pOsdWin->rgnHandle, &rgn_attr);
    ENCODE_RET_JUDGE(ret);

    ret = ss_mpi_rgn_attach_to_chn(pOsdWin->rgnHandle, &pOsdWin->rgnChn, &chn_attr);
    ENCODE_RET_JUDGE(ret);
    pOsdWin->osdregion = TRUE;

    return ret;
}

static int encode_osd_destroy_text(OsdGroupE osdGroup, OsdTypeE osdType)
{
    int ret = S_OK;
    OsdWindowS *pOsdWin = &g_osd[osdGroup].text.win[osdType];

    do {
        if( NULL == pOsdWin) {
            ERR("pOsdWin is null\n");
            break;
        }

        if (pOsdWin->osdType < 0) {
            //DBG("osd group:%d type:%d not create\n", osdGroup, osdType);
            break;
        }

        if(pOsdWin->osdregion) {
            ret = ss_mpi_rgn_detach_from_chn(pOsdWin->rgnHandle, &pOsdWin->rgnChn);
            ENCODE_RET_JUDGE(ret);

            ret = ss_mpi_rgn_destroy(pOsdWin->rgnHandle);
            ENCODE_RET_JUDGE(ret);
            pOsdWin->osdregion = FALSE;
        }

        if (NULL != pOsdWin->pcBuffer) {
            system_free(pOsdWin->pcBuffer);
            pOsdWin->pcBuffer = NULL;
        }

        memset(pOsdWin, 0, sizeof(OsdWindowS));
        pOsdWin->osdType   = E_OSD_BEGIN;
        pOsdWin->rgnHandle = -1;
    } while(0);
    return ret;
}

static int osd_text_win_merge_init(OsdWindowS *pOsdWin, OsdGroupE osdGroup, OsdTypeE osdType, TFont *ptFont, DWORD startX, DWORD startY)
{
    int ret = 0;
    pOsdWin->startX    = startX * encode_osd_video_get_x_ratio(osdGroup);
    pOsdWin->startY    = startY * encode_osd_video_get_y_ratio(osdGroup);
    pOsdWin->osdType   = osdType;
    pOsdWin->dwHeight  = ptFont->dwFontHeight;
    pOsdWin->osdGroup  = osdGroup;
    pOsdWin->ptFont    = ptFont;
    pOsdWin->rgnType   = OT_RGN_OVERLAY;
    pOsdWin->rgnChn.mod_id  = OT_ID_VENC;
    pOsdWin->rgnChn.dev_id  = 0;
    pOsdWin->rgnChn.chn_id  = osdGroup;

    switch (osdType) {
    case E_OSD_NAME:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_NAME_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_NAME_HANDLE;
        }
    break;

    case E_OSD_TIME:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_TIME_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_TIME_HANDLE;
        }
    break;

    case E_OSD_TEXT:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_TEXT_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_TEXT_HANDLE;
        }
    break;

    case E_OSD_ZOOM:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_ZOOM_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_ZOOM_HANDLE;
        }
    break;

    case E_OSD_STREAM:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_STREAM_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_STREAM_HANDLE;
        }
    break;

    case E_OSD_EXPAND1:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_EXPAND1_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_EXPAND1_HANDLE;
        }
    break;

    case E_OSD_EXPAND2:
        if(E_OSD_GROUP_MAIN == osdGroup) {
            pOsdWin->rgnHandle = E_RGN_MAIN_EXPAND2_HANDLE;
        } else {
            pOsdWin->rgnHandle = E_RGN_SUB_EXPAND2_HANDLE;
        }
    break;

        default:
        break;
    }

    pOsdWin->bShowFlag = true;

    return ret;
}

static int encode_osd_text_update(OsdWindowS *pOsdWin, char *pString)
{
    ot_bmp text_bmp = {0};
    int ret = S_OK;

    if(NULL != pString) {
        int ret = osd_string_get_lattice(pOsdWin, pString);
        if (ret < 0) {
            ERR("osd_string_get_lattice failed\n");
            return ret;
        }
    }

    if (S_OK == ret) {
        text_bmp.data = pOsdWin->pcBuffer;
        text_bmp.pixel_format  = OT_PIXEL_FORMAT_ARGB_CLUT2;
        if(pOsdWin->mergeNum) {
            int osd_interval = OSD_MERGE_INTERVAL;
            if(E_OSD_GROUP_SUB == pOsdWin->osdGroup) {
                osd_interval = OSD_MERGE_INTERVAL/3;
            }
            text_bmp.height = (pOsdWin->dwHeight + osd_interval) * pOsdWin->mergeNum;
        } else {
            text_bmp.height = pOsdWin->dwHeight;
        }

        text_bmp.width = pOsdWin->dwWidth;
        ret = ss_mpi_rgn_set_bmp(pOsdWin->rgnHandle, &text_bmp);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

static int encode_osd_get_stream_info(OsdWindowS *pOsdWin, char *string, int str_len)
{
    int ret = S_OK;
    float fr = 0;
    float br = 0;
    int vi_pipe = 0;
    ot_isp_wb_info isp_wb_info = {0};
    ot_isp_exp_info exp_info = {0};
    OsdMediaS *pMedia = &g_osd[pOsdWin->osdGroup].media;

    int fInterMs = ms_since_previous2(&pMedia->lastFrameTs);
    fr = pMedia->dwFps * 1000.0 / fInterMs;
    fr = MIN(fr, pMedia->dwFpsMax);
    br = pMedia->dwBits * 8.0 / fInterMs;

    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &exp_info);
    ENCODE_RET_CHECK(ret, "ss_mpi_isp_query_exposure_info fail\n");
    ret = ss_mpi_isp_query_wb_info(vi_pipe, &isp_wb_info);
    ENCODE_RET_CHECK(ret, "ss_mpi_isp_query_wb_info fail\n");

    if (g_osd_cfg->osdinfo.bpsen) {
        if (is_test_ver()) {
            if(g_osd[pOsdWin->osdGroup].venc.codec == VENC_FORMAT_H265) {
                snprintf(string, str_len-1, (char *)"H265+ FR=%.1f BR=%.0fK ISO=%d ct=%d",
                    fr, br / 10, exp_info.iso, isp_wb_info.color_temp);
            } else {
                snprintf(string, str_len-1, (char *)"H264+ FR=%.1f BR=%.0fK ISO=%d ct=%d",
                    fr, br / 10, exp_info.iso, isp_wb_info.color_temp);
            }
        } else {
            if(g_osd[pOsdWin->osdGroup].venc.codec == VENC_FORMAT_H265) {
                snprintf(string, str_len-1, (char *)"H265+ BR=%.0fK", br / 10);
            } else {
                snprintf(string, str_len-1, (char *)"H264+ BR=%.0fK", br / 10);
            }
        }
    } else {
        return 0;
    }

    pMedia->dwFps = 0;
    pMedia->dwBits = 0;

    return ret;
}

static int encode_osd_update_time(OsdWindowS *p_osdwin)
{
    int ret = 0;
    char str[MAX_LATTICE_NUMBER] = {0};

    size_t len = sizeof(str) - 1;
    time_t time_now = {0,};
    int show_wk = p_osdwin->showWeek, lang = p_osdwin->language;
    size_t type = p_osdwin->dateFmt;

    struct tm tm = {0,};
    time_now = time(&time_now);
    localtime_r(&time_now, &tm);
    tm.tm_year += 1900;
    tm.tm_mon  += 1;
    tm.tm_hour %= 24;

    const char *weekday[7][2] = {
        {"星期日", "Sun"},
        {"星期一", "Mon"},
        {"星期二", "Tue"},
        {"星期三", "Wed"},
        {"星期四", "Thu"},
        {"星期五", "Fri"},
        {"星期六", "Sat"},
    };

    struct datemap {
        const char *fmt;
        int wk_mid;    //0:不显示 1:中间 2:末尾
        int d[6];
    } wk_maps[] = {
        {"%04d年%02d月%02d日 %s %02d:%02d:%02d", 1, {tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec}},
        {"%02d日%02d月%04d年 %s %02d:%02d:%02d", 1, {tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec}},
        {"%04d-%02d-%02d %s %02d:%02d:%02d"    , 1, {tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec}},
        {"%02d-%02d-%04d %s %02d:%02d:%02d"    , 1, {tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec}},
        {"%04d/%02d/%02d %s %02d:%02d:%02d"    , 1, {tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec}},
        {"%02d/%02d/%04d %s %02d:%02d:%02d"    , 1, {tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec}},
        {"%02d:%02d:%02d %04d年%02d月%02d日 %s", 2, {tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year, tm.tm_mon, tm.tm_mday}},
        {"%02d:%02d:%02d %04d-%02d-%02d %s"    , 2, {tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year, tm.tm_mon, tm.tm_mday}},
        {"%02d:%02d:%02d %04d/%02d/%02d %s"    , 2, {tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year, tm.tm_mon, tm.tm_mday}},
    };

    struct datemap nowk_maps[] = {
        {"%04d年%02d月%02d日 %02d:%02d:%02d",},
        {"%02d日%02d月%04d年 %02d:%02d:%02d",},
        {"%04d-%02d-%02d %02d:%02d:%02d"    ,},
        {"%02d-%02d-%04d %02d:%02d:%02d"    ,},
        {"%04d/%02d/%02d %02d:%02d:%02d"    ,},
        {"%02d/%02d/%04d %02d:%02d:%02d"    ,},
        {"%02d:%02d:%02d %04d年%02d月%02d日",},
        {"%02d:%02d:%02d %04d-%02d-%02d"    ,},
        {"%02d:%02d:%02d %04d/%02d/%02d"    ,},
    };

    if (type >= ARRAY_SIZE(wk_maps)) {
        DBG("invalid type: %d\n", type);
        type = 2; //默认使用fmt2
    }

    const char *wk_str = weekday[MIN(6, (int)tm.tm_wday)][lang];
    const char *fmt = (show_wk) ? wk_maps[type].fmt : nowk_maps[type].fmt;
    struct datemap *map = &wk_maps[type];

    if (g_osd_cfg->osdinfo.timeen) {
        if (show_wk == 0) {
            snprintf(str, len, fmt, map->d[0], map->d[1], map->d[2],         map->d[3], map->d[4], map->d[5]);
        } else if (map->wk_mid == 1) {
            snprintf(str, len, fmt, map->d[0], map->d[1], map->d[2], wk_str, map->d[3], map->d[4], map->d[5]);
        } else {
            snprintf(str, len, fmt, map->d[0], map->d[1], map->d[2],         map->d[3], map->d[4], map->d[5], wk_str);
        }
    }

    ret = encode_osd_text_update(p_osdwin, str);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_update_zoom(OsdWindowS *pOsdWin)
{
    int ret = 0;
    static char zoom_str[16] = {0};

    snprintf(zoom_str, sizeof(zoom_str), "%s", g_osd_cfg->osdzoom.zoomstr);

    ret = encode_osd_text_update(pOsdWin, zoom_str);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_update_stream_info(OsdWindowS *pOsdWin)
{
    int ret = 0;
    char str[MAX_LATTICE_NUMBER] = {0};

    encode_osd_get_stream_info(pOsdWin, str, MAX_LATTICE_NUMBER);

    ret = encode_osd_text_update(pOsdWin, str);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

static int encode_osd_video_select_time_dateformat(char *deststr, OsdInfoS *ptOsdInfo)
{
    if (deststr == NULL || ptOsdInfo == NULL) {
        ERR("error deststr\n");
        return -1;
    }

    if (0 == ptOsdInfo->osdlanguage) {
        if (ptOsdInfo->osdweek) {
            switch (ptOsdInfo->dateformat) {
            case 0:
                memcpy(deststr, "2016年03月21日 星期一 15:34:12", strlen("2016年03月21日 星期一 15:34:12"));
                break;
            case 1:
                memcpy(deststr, "21日03月2016年 星期一 15:34:12", strlen("21日03月2016年 星期一 15:34:12"));
                break;
            case 2:
                memcpy(deststr, "2016-03-21 星期一 15:34:12", strlen("2016-03-21 星期一 15:34:12"));
                break;
            case 3:
                memcpy(deststr, "21-03-2016 星期一 15:34:12", strlen("21-03-2016 星期一 15:34:12"));
                break;
            case 4:
                memcpy(deststr, "2016/03/21 星期一 15:34:12", strlen("2016/03/21 星期一 15:34:12"));
                break;
            case 5:
                memcpy(deststr, "21/03/2016 星期一 15:34:12", strlen("21/03/2016 星期一 15:34:12"));
                break;
            case 6:
                memcpy(deststr, "15:34:12 2016年03月21日 星期一", strlen("15:34:12 2016年03月21日 星期一"));
                break;
            case 7:
                memcpy(deststr, "15:34:12 2016-03-21 星期一", strlen("15:34:12 2016-03-21 星期一"));
                break;
            case 8:
                memcpy(deststr, "15:34:12 2016/03/21 星期一", strlen("15:34:12 2016/03/21 星期一"));
                break;
            default:
                memcpy(deststr, "2016-03-21 星期一 15:34:12", strlen("2016-03-21 星期一 15:34:12"));
                break;
            }
        } else {
            switch (ptOsdInfo->dateformat) {
            case 0:
                memcpy(deststr, "2016年03月21日 15:34:12", strlen("2016年03月21日 15:34:12"));
                break;
            case 1:
                memcpy(deststr, "21日03月2016年 15:34:12", strlen("21日03月2016年 15:34:12"));
                break;
            case 2:
                memcpy(deststr, "2016-03-21 15:34:12", strlen("2016-03-21 15:34:12"));
                break;
            case 3:
                memcpy(deststr, "21-03-2016 15:34:12", strlen("21-03-2016 15:34:12"));
                break;
            case 4:
                memcpy(deststr, "2016/03/21 15:34:12", strlen("2016/03/21 15:34:12"));
                break;
            case 5:
                memcpy(deststr, "21/03/2016 15:34:12", strlen("21/03/2016 15:34:12"));
                break;
            case 6:
                memcpy(deststr, "15:34:12 2016年03月21日", strlen("15:34:12 2016年03月21日"));
                break;
            case 7:
                memcpy(deststr, "15:34:12 2016-03-21", strlen("15:34:12 2016-03-21"));
                break;
            case 8:
                memcpy(deststr, "15:34:12 2016/03/21", strlen("15:34:12 2016/03/21"));
                break;
            default:
                memcpy(deststr, "2016-03-21 15:34:12", strlen("2016-03-21 15:34:12"));
                break;
            }
        }
    } else {
        if (ptOsdInfo->osdweek) {
            switch (ptOsdInfo->dateformat) {
            case 0:
                memcpy(deststr, "2016年03月21日 Mon 15:34:12", strlen("2016年03月21日 Mon 15:34:12"));
                break;
            case 1:
                memcpy(deststr, "21日03月2016年 Mon 15:34:12", strlen("21日03月2016年 Mon 15:34:12"));
                break;
            case 2:
                memcpy(deststr, "2016-03-21 Mon 15:34:12", strlen("2016-03-21 Mon 15:34:12"));
                break;
            case 3:
                memcpy(deststr, "21-03-2016 Mon 15:34:12", strlen("21-03-2016 Mon 15:34:12"));
                break;
            case 4:
                memcpy(deststr, "2016/03/21 Mon 15:34:12", strlen("2016/03/21 Mon 15:34:12"));
                break;
            case 5:
                memcpy(deststr, "21/03/2016 Mon 15:34:12", strlen("21/03/2016 Mon 15:34:12"));
                break;
            case 6:
                memcpy(deststr, "15:34:12 2016年03月21日 Mon", strlen("15:34:12 2016年03月21日 Mon"));
                break;
            case 7:
                memcpy(deststr, "15:34:12 2016-03-21 Mon", strlen("15:34:12 2016-03-21 Mon"));
                break;
            case 8:
                memcpy(deststr, "15:34:12 2016/03/21 Mon", strlen("15:34:12 2016/03/21 Mon"));
                break;
            default:
                memcpy(deststr, "2016-03-21 Mon 15:34:12", strlen("2016-03-21 Mon 15:34:12"));
                break;
        }
        } else {
            switch (ptOsdInfo->dateformat) {
            case 0:
                memcpy(deststr, "2016年03月21日 15:34:12", strlen("2016年03月21日 15:34:12"));
                break;
            case 1:
                memcpy(deststr, "21日03月2016年 15:34:12", strlen("21日03月2016年 15:34:12"));
                break;
            case 2:
                memcpy(deststr, "2016-03-21 15:34:12", strlen("2016-03-21 15:34:12"));
                break;
            case 3:
                memcpy(deststr, "21-03-2016 15:34:12", strlen("21-03-2016 15:34:12"));
                break;
            case 4:
                memcpy(deststr, "2016/03/21 15:34:12", strlen("2016/03/21 15:34:12"));
                break;
            case 5:
                memcpy(deststr, "21/03/2016 15:34:12", strlen("21/03/2016 15:34:12"));
                break;
            case 6:
                memcpy(deststr, "15:34:12 2016年03月21日", strlen("15:34:12 2016年03月21日"));
                break;
            case 7:
                memcpy(deststr, "15:34:12 2016-03-21", strlen("15:34:12 2016-03-21"));
                break;
            case 8:
                memcpy(deststr, "15:34:12 2016/03/21", strlen("15:34:12 2016/03/21"));
                break;
            default:
                memcpy(deststr, "2016-03-21 15:34:12", strlen("2016-03-21 15:34:12"));
                break;
            }
        }
    }

    return 0;
}

int encode_osd_set_time(int group, OsdInfoS *ptOsdInfo)
{
    int ret = 0;
    TFont *ptFont = NULL;
    OsdWindowS* pOsdTimeWin = NULL;

    char timestr[MAX_LATTICE_NUMBER] = {0};

    ret = encode_osd_destroy_text(group, E_OSD_TIME);
    ENCODE_RET_JUDGE(ret);

    // SysCustomS capability = {0,};
    // conf_get_capability(&capability);
    // g_osd_cfg->osdzoom.enable = capability.zoom;

    ptFont = &g_osd[group].text.tFont;
    pOsdTimeWin = &g_osd[group].text.win[E_OSD_TIME];
    pOsdTimeWin->showWeek = ptOsdInfo->osdweek;
    pOsdTimeWin->dateFmt  = ptOsdInfo->dateformat;
    pOsdTimeWin->language = ptOsdInfo->osdlanguage;
    pOsdTimeWin->bpsenable = ptOsdInfo->bpsen;

    if (!g_osd_cfg->osdinfo.timeen) {
        return 0;
    }

    encode_osd_video_select_time_dateformat(timestr, ptOsdInfo);

    ret = osd_text_win_merge_init(pOsdTimeWin,
            group,
            E_OSD_TIME,
            ptFont,
            ptOsdInfo->timeleft,
            ptOsdInfo->timetop);
    ENCODE_RET_JUDGE(ret);

    ret = osd_string_get_lattice(pOsdTimeWin, timestr);
    ENCODE_RET_JUDGE(ret);

    ret = encode_osd_create_text(group, pOsdTimeWin);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_set_zoom(int group, OsdInfoS *ptOsdInfo)
{
    int ret = 0;
    TFont *ptFont = NULL;
    OsdWindowS* pOsdZoom = NULL;

    char zoomstr[MAX_LATTICE_NUMBER] = {0};
    strcpy(zoomstr, "12.0X");

    ret = encode_osd_destroy_text(group, E_OSD_ZOOM);
    ENCODE_RET_JUDGE(ret);

    ptFont = &g_osd[group].text.tFont;
    pOsdZoom = &g_osd[group].text.win[E_OSD_ZOOM];

    ret = osd_text_win_merge_init(pOsdZoom,
            group,
            E_OSD_ZOOM,
            ptFont,
            OSD_ZOOM_STARTX,
            OSD_ZOOM_STARTY);
    ENCODE_RET_JUDGE(ret);

    ret = osd_string_get_lattice(pOsdZoom, zoomstr);
    ENCODE_RET_JUDGE(ret);

    ret = encode_osd_create_text(group, pOsdZoom);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_set_stream_info(int group, OsdInfoS *ptOsdInfo)
{
    int ret = 0;
    TFont *ptFont = NULL;
    OsdWindowS* ptOsdStream = NULL;

    char stream_str[MAX_LATTICE_NUMBER] = {0};

    ret = encode_osd_destroy_text(group, E_OSD_STREAM);
    ENCODE_RET_JUDGE(ret);

    ptFont = &g_osd[group].text.tFont;
    ptOsdStream = &g_osd[group].text.win[E_OSD_STREAM];
    if (ptOsdInfo->bpsen) {
        if (is_test_ver()) {
            strcpy(stream_str, "H265+ FR=15.0 BR=1234K ISO=100 ct=1064");
        } else {
            strcpy(stream_str, "H265+ BR=1234K");
        }
    } else {
        return 0;
    }

    ret = osd_text_win_merge_init(ptOsdStream,
            group,
            E_OSD_STREAM,
            ptFont,
            ptOsdInfo->bpsleft,
            ptOsdInfo->bpstop);
    ENCODE_RET_JUDGE(ret);

    ret = osd_string_get_lattice(ptOsdStream, stream_str);
    ENCODE_RET_JUDGE(ret);

    ret = encode_osd_create_text(group, ptOsdStream);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_get_expand(OsdWindowS* osdWin, OsdExpandS *ptOsdExpand)
{
    int ret = 0;
    int src_w = 0, src_h = 0;
    int src_stride = 0, copy_pixels = 0, copy_bytes  = 0;
    int i = 0, height_num = 0, max_width = 0;
    int current_height = 0, dst_stride = 0;
    OsdExpand0 *ptOsdExpand0 = NULL;
    OsdWindowS pExpandOsdWin[OSD_EXPAND_MAX_CHN] = {{0},};
    int max_x = encode_osd_video_get_x_ratio(osdWin->osdGroup)*P1080_WIDTH;
    int max_y = encode_osd_video_get_y_ratio(osdWin->osdGroup)*P1080_HEIGHT;
    int osd_interval = OSD_MERGE_INTERVAL;
    if(E_OSD_GROUP_SUB == osdWin->osdGroup) {
        osd_interval = OSD_MERGE_INTERVAL/3;
    }

    for (i = 0; i < OSD_EXPAND_MAX_CHN; i++) {
        ptOsdExpand0 = &ptOsdExpand->cusosd[i];
        if (ptOsdExpand0->enable) {
            pExpandOsdWin[i].ptFont = osdWin->ptFont;
            pExpandOsdWin[i].startX = osdWin->startX;
            pExpandOsdWin[i].startY = osdWin->startY;
            pExpandOsdWin[i].dwHeight = osdWin->dwHeight;
            pExpandOsdWin[i].osdGroup = osdWin->osdGroup;
            ret = osd_string_get_lattice(&pExpandOsdWin[i], ptOsdExpand0->content);
            ENCODE_RET_JUDGE(ret);
            max_width = MAX(max_width, pExpandOsdWin[i].dwWidth);
            height_num++;
        }
    }

    if ((osdWin->startX+max_width) > max_x) {
        osdWin->startX = max_x - max_width;
        if(osdWin->startX % 2 != 0) osdWin->startX -= 1;
    }

    if ((osdWin->startY+(osdWin->dwHeight + osd_interval)*height_num) > max_y) {
        osdWin->startY = max_y - (osdWin->dwHeight + osd_interval)*height_num;
    }

    if (osdWin->startX < 0) {
        osdWin->startX = 0;
    }

    if (osdWin->startY < 0) {
        osdWin->startY = 0;
    }

    if((max_width * (osdWin->dwHeight + osd_interval) * height_num) > osdWin->dwWidth * osdWin->dwHeight) {
        osdWin->dwWidth = max_width;
        osdWin->dwHeight = (osdWin->dwHeight + osd_interval) * height_num;
        osdWin->pcBuffer = system_malloc(osdWin->dwWidth * osdWin->dwHeight / 4);
    }

    memset(osdWin->pcBuffer, 0, (osdWin->dwWidth * osdWin->dwHeight / 4));

    dst_stride = CLUT2_BYTES(max_width);
    for (i = 0; i < OSD_EXPAND_MAX_CHN; i++) {
        ptOsdExpand0 = &ptOsdExpand->cusosd[i];
        if (!ptOsdExpand0->enable) {
            continue;
        }
        src_w = pExpandOsdWin[i].dwWidth;
        src_h = pExpandOsdWin[i].dwHeight;
        src_stride = CLUT2_BYTES(src_w);
        copy_pixels = (src_w < max_width) ? src_w : max_width;
        copy_bytes  = CLUT2_BYTES(copy_pixels);
        for (int row = 0; row < src_h; row++) {
            unsigned char *dst = osdWin->pcBuffer + (current_height + row) * dst_stride;
            const unsigned char *src = pExpandOsdWin[i].pcBuffer + row * src_stride;
            memcpy(dst, src, copy_bytes);
        }
        current_height += (src_h + osd_interval);
        if (pExpandOsdWin[i].pcBuffer) {
            system_free(pExpandOsdWin[i].pcBuffer);
            pExpandOsdWin[i].pcBuffer = NULL;
        }
    }

    return ret;
}

int encode_osd_set_expand(int group, OsdExpandS *ptOsdExpand)
{
    int ret = 0;
    int expand_count = 0;
    TFont *ptFont = NULL;
    OsdWindowS* pOsdWin = NULL;
    OsdExpand0 *ptOsdExpand0 = NULL;
    ret = encode_osd_destroy_text(group, E_OSD_TEXT);
    ENCODE_RET_JUDGE(ret);

    for (int i = 0; i < OSD_EXPAND_MAX_CHN; i++) {
        ptOsdExpand0 = &ptOsdExpand->cusosd[i];
        if (ptOsdExpand0->enable) {
            expand_count++;
        }
    }

    if(expand_count) {
        ptFont = &g_osd[group].text.tFont;
        pOsdWin = &g_osd[group].text.win[E_OSD_TEXT];
        ret = osd_text_win_merge_init(pOsdWin,
                group,
                E_OSD_TEXT,
                ptFont,
                ptOsdExpand->cusosd[0].x,
                ptOsdExpand->cusosd[0].y);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_get_expand(pOsdWin, ptOsdExpand);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_create_text(group, pOsdWin);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_text_update(pOsdWin, NULL);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

// 每个扩展字幕 OSD 区域均单开一行用此函数, 网页扩展字幕改成坐标设置
/*
int encode_osd_set_expand(int group, OsdExpandS *ptOsdExpand, int filter)
{
    int ret = S_OK;
    int i = 0;
    TFont *ptFont = NULL;
    OsdWindowS* pOsdWin = NULL;
    OsdExpand0 *ptOsdExpand0 = NULL;

    ptFont = &g_osd[group].text.tFont;

    if (!g_osd[group].venc.enable) {
        return ret;
    }

    int index = E_OSD_TEXT;
    for (i = 0; i < OSD_EXPAND_MAX_CHN; i++) {
        ptOsdExpand0 = &ptOsdExpand->cusosd[i];

        if (filter && ((g_osd_run->expand_chg_flag & (0x01 << i)) == 0)) {
            index++;
            continue;
        }

        if (index >= E_OSD_TEXT_MAX) {
            break;
        }

        ret = encode_osd_destroy_text(group, index);
        ENCODE_RET_JUDGE(ret);

        pOsdWin = &g_osd[group].text.win[index];
        if (ptOsdExpand0->enable) {
            ret = osd_text_win_init(pOsdWin,
                group,
                index,
                ptFont,
                ptOsdExpand0->content,
                ptOsdExpand0->x,
                ptOsdExpand0->y);
            ENCODE_RET_JUDGE(ret);

            if (S_OK == ret) {
                ret = encode_osd_create_text(group, pOsdWin);
                ENCODE_RET_JUDGE(ret);
            }

            ret = encode_osd_text_update(pOsdWin, ptOsdExpand0->content);
            ENCODE_RET_JUDGE(ret);
        }

        index++;
    }

    dbg_osd("encode_osd_set_expand is OK \n");
    return ret;
}
*/

int encode_osd_set_name(int group, OsdInfoS *ptOsdInfo)
{
    int ret = S_OK;
    OsdMediaS *pMedia = NULL;
    TFont *ptFont = NULL;
    OsdWindowS* pOsdWin = NULL;

    ret = encode_osd_destroy_text(group, E_OSD_NAME);
    ENCODE_RET_JUDGE(ret);

    ptFont = &g_osd[group].text.tFont;
    pMedia = &g_osd[group].media;

    // name
    pMedia->dwFpsMax = g_osd[group].venc.fps;
    if (ptOsdInfo->nameen) {
        pOsdWin = &g_osd[group].text.win[E_OSD_NAME];

        ret = osd_text_win_merge_init(pOsdWin,
                group,
                E_OSD_NAME,
                ptFont,
                ptOsdInfo->nameleft,
                ptOsdInfo->nametop);
        ENCODE_RET_JUDGE(ret);

        ret = osd_string_get_lattice(pOsdWin, ptOsdInfo->name);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_create_text(group, pOsdWin);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_text_update(pOsdWin, NULL);
        ENCODE_RET_JUDGE(ret);
    }

    dbg_osd("encode_osd_set_name is OK \n");
    return ret;
}

void encode_osd_process_time_stream(void)
{
    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        OsdWindowS *pOsdWin = &g_osd[group].text.win[E_OSD_TIME];
        if (pOsdWin->osdType < 0) {
            continue;
        }

        int ret = encode_osd_update_time(pOsdWin);
        ENCODE_RET_JUDGE(ret);
    }
}

void encode_osd_process_zoom(void)
{
    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        OsdWindowS *pOsdZoom = &g_osd[group].text.win[E_OSD_ZOOM];
        if (pOsdZoom->osdType < 0) {
            continue;
        }

        int ret = encode_osd_update_zoom(pOsdZoom);
        ENCODE_RET_JUDGE(ret);
    }
}

void encode_osd_process_stream_info(void)
{
    if (!g_osd_cfg->osdinfo.bpsen) {
        return;
    }

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        OsdWindowS *pOsdStream = &g_osd[group].text.win[E_OSD_STREAM];
        if (pOsdStream->osdType < 0) {
            continue;
        }

        int ret = encode_osd_update_stream_info(pOsdStream);
        ENCODE_RET_JUDGE(ret);
    }
}

static void osd_add_stream_cb(void* data)
{
    StreamInfoS* info = (StreamInfoS*)data;
    OsdMediaS* pMedia = &g_osd[info->encGroup].media;

    pMedia->dwFps  += info->dwAddFps;
    pMedia->dwBits += info->dwAddBits;

    free(info);
    info = NULL;
}

int encode_osd_add_stream(OsdGroupE group, DWORD dwAddFps, DWORD dwAddBits)
{
    if (NULL == g_osd_run->sch) {
        return -1;
    }

    StreamInfoS* info = (StreamInfoS*)malloc(sizeof(StreamInfoS));
    if (NULL == info) {
        return -1;
    }

    info->encGroup  = group;
    info->dwAddFps  = dwAddFps;
    info->dwAddBits = dwAddBits;

    js_run_function(g_osd_run->sch, osd_add_stream_cb, (void *)info, 0);
    return 0;
}

static int encode_osd_text_init(int group)
{
    int i = 0;
    VideoEncS video = {0};
    get_config(handleRealVideoCfg, video);
    memcpy(&g_osd[group].venc, &video.enc[group], sizeof(VideoEnc0));
    encode_region_get_venc(group, &video.enc[group]);

    for (i = 0; i < E_OSD_TEXT_MAX; i++) {
        OsdWindowS *pOsdWin = &g_osd[group].text.win[i];
        pOsdWin->osdType = E_OSD_BEGIN;
        pOsdWin->rgnHandle = -1;
    }

    encode_osd_set_style(group, &g_osd_cfg->osdstyle, g_osd_cfg->osdexpand.size);
    encode_osd_set_name(group, &g_osd_cfg->osdinfo);
    encode_osd_set_time(group, &g_osd_cfg->osdinfo);
    encode_osd_set_zoom(group, &g_osd_cfg->osdinfo);
    encode_osd_set_stream_info(group, &g_osd_cfg->osdinfo);
    encode_osd_set_expand(group, &g_osd_cfg->osdexpand);

    return 0;
}

static int encode_osd_text_uninit(int group)
{
    for (int type = 0; type < E_OSD_TEXT_MAX; type++) {
        encode_osd_destroy_text(group, type);
    }

    return 0;
}

static void osd_stop_cb(void* data)
{
    int ret = S_OK;
    int group = (int)data;
    g_osd[group].bAvail = false;

    ret = encode_osd_vgrect_group_uninit(group);
    ENCODE_RET_JUDGE(ret);

    ret = encode_osd_vgline_group_uninit(group);
    ENCODE_RET_JUDGE(ret);

    ret = encode_osd_aidet_group_uninit(group);
    ENCODE_RET_JUDGE(ret);

    ret = encode_osd_text_uninit(group);
    ENCODE_RET_JUDGE(ret);

    dbg_osd("encode osd stoped\n");
}

int encode_osd_group_stop(int group)
{
    if (NULL == g_osd_run->sch) {
        return -1;
    }

    js_run_function(g_osd_run->sch, osd_stop_cb, (void *)group, 1);
    return 0;
}

static void osd_start_cb(void* data)
{
    int group = (int)data;
    int ret = 0;

    VideoEncS videoCfg = {0};
    get_config(handleRealVideoCfg, videoCfg);
    memcpy(&g_osd[group].venc, &videoCfg.enc[group], sizeof(VideoEnc0));

    VglineS vgLinecfg = {0};
    VgrectS vgRectcfg = {0};
    HumanDetectionS humancfg = {0};
    CarDetectionS carcfg = {0};
    follow_info_t followcfg = {0};
    get_config(handleVglineCfg, vgLinecfg);
    get_config(handleVgrectCfg, vgRectcfg);
    get_config(handleHumanDetectCfg, humancfg);
    get_config(handleCarDetectCfg, carcfg);
    get_config(handleFollowCfg, followcfg);

    ret = encode_osd_text_init(group);
    ENCODE_RET_JUDGE(ret);

    if (TD_TRUE == vgLinecfg.enable) {
        ret = encode_osd_vgline_group_init(group, &vgLinecfg);
        ENCODE_RET_JUDGE(ret);

        if (vgLinecfg.blink) {
            ret = encode_osd_vgline_update(group, &vgLinecfg, -1, TD_TRUE);
            ENCODE_RET_JUDGE(ret);
        }
    }

    if (TD_TRUE == vgRectcfg.enable) {
        ret = encode_osd_vgrect_group_init(group, &vgRectcfg);
        ENCODE_RET_JUDGE(ret);

        if (vgRectcfg.blink) {
            ret = encode_osd_vgrect_update(group, &vgRectcfg, -1, TD_TRUE);
            ENCODE_RET_JUDGE(ret);
        }
    }

    if(TD_TRUE == humancfg.enable || TD_TRUE == carcfg.enable
        || TD_TRUE == followcfg.enable || TD_TRUE == humancfg.person_center) {
        ret = encode_osd_aidet_group_init(group);
        ENCODE_RET_JUDGE(ret);
    }

    g_osd[group].bAvail = true;
    dbg_osd("encode osd started\n");
}

int encode_osd_group_start(int group)
{
    if (NULL == g_osd_run->sch) {
        return -1;
    }

    js_run_function(g_osd_run->sch, osd_start_cb, (void *)group, 1);
    return 0;
}

static void osd_zoom_change_cb(void* data)
{
    do {
        if(NULL == data) {
            break;
        }

        snprintf(g_osd_cfg->osdzoom.zoomstr, sizeof(g_osd_cfg->osdzoom.zoomstr), "%s", (char *)data);

        encode_osd_process_zoom();

    } while(0);
}

int encode_osd_zoom_change(int enable, char *zoomstr)
{
    if (NULL == g_osd_run->sch) {
        return -1;
    }

    js_run_function(g_osd_run->sch, osd_zoom_change_cb, (void *)zoomstr, 1);

    return 0;
}

static void osd_vgrect_cb(void *data)
{
    int ret = 0;
    VgrectS vgRectcfg = {0};
    get_config(handleVgrectCfg, vgRectcfg);

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        encode_osd_vgrect_update(group, &vgRectcfg, g_blink_cnt, vgRectcfg.blink);
        ENCODE_RET_JUDGE(ret);
    }

}

int encode_osd_vgrect_blink(int blink_cnt)
{
    if (NULL == g_osd_run->sch) {
        return -1;
    }

    g_blink_cnt = blink_cnt;

    return js_run_function(g_osd_run->sch, osd_vgrect_cb, NULL, 0);
}

static void osd_vgline_cb(void *data)
{
    int ret = 0;
    VglineS vgLinecfg = {0};
    get_config(handleVglineCfg, vgLinecfg);

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        encode_osd_vgline_update(group, &vgLinecfg, g_blink_cnt, vgLinecfg.blink);
        ENCODE_RET_JUDGE(ret);
    }

}

int encode_osd_vgline_blink(int blink_cnt)
{
    if (NULL == g_osd_run->sch) {
        return -1;
    }

    g_blink_cnt = blink_cnt;

    return js_run_function(g_osd_run->sch, osd_vgline_cb, NULL, 0);
}


static void cb_osd_info_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_OSD_INFO, &g_osd_raw->osdinfo, p_src, size);
}

static void cb_osd_style_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_OSD_STYLE, &g_osd_raw->osdstyle, p_src, size);
}

static void cb_osd_expand_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_OSD_EXPAND, &g_osd_raw->osdexpand, p_src, size);
}

static void cb_osd_video_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_VIDEO_CHAGE);
}

static int exec_cmd_osd_info()
{
    int group = 0;

    for (group = 0; group < E_OSD_GROUP_MAX; group++) {
        if (true == g_osd[group].bAvail) {
            encode_osd_set_name(group, &g_osd_cfg->osdinfo);
            encode_osd_set_time(group, &g_osd_cfg->osdinfo);
            encode_osd_set_zoom(group, &g_osd_cfg->osdinfo);
            encode_osd_set_stream_info(group, &g_osd_cfg->osdinfo);
        }
    }

    encode_osd_process_zoom();

    return 0;
}

static int exec_cmd_osd_style()
{
    int group = 0;

    if (0 >= g_osd_cfg->osdstyle.width || 0 >= g_osd_cfg->osdstyle.height) {
        g_osd_cfg->osdstyle.width  = 48;
        g_osd_cfg->osdstyle.height = 48;
    }

    for (group = 0; group < E_OSD_GROUP_MAX; group++) {
        if (true == g_osd[group].bAvail) {
            encode_osd_set_style(group, &g_osd_cfg->osdstyle, g_osd_cfg->osdexpand.size);
            encode_osd_set_name(group, &g_osd_cfg->osdinfo);
            encode_osd_set_time(group, &g_osd_cfg->osdinfo);
            encode_osd_set_zoom(group, &g_osd_cfg->osdinfo);
            encode_osd_set_stream_info(group, &g_osd_cfg->osdinfo);
            encode_osd_set_expand(group, &g_osd_cfg->osdexpand);
        }
    }

    encode_osd_process_zoom();

    return 0;
}

static int exec_cmd_osd_expand(int cmd)
{
    int group = 0;

    for (group = 0; group < E_OSD_GROUP_MAX; group++) {
        if (g_osd[group].bAvail) {
            if (cmd & CMD_OSD_EXPAND_FONT) {
                encode_osd_set_style(group, &g_osd_cfg->osdstyle, g_osd_cfg->osdexpand.size);
                encode_osd_set_name(group, &g_osd_cfg->osdinfo);
                encode_osd_set_time(group, &g_osd_cfg->osdinfo);
                encode_osd_set_zoom(group, &g_osd_cfg->osdinfo);
                encode_osd_set_stream_info(group, &g_osd_cfg->osdinfo);
            }

            encode_osd_set_expand(group, &g_osd_cfg->osdexpand);
        }
    }

    encode_osd_process_zoom();
    g_osd_run->expand_chg_flag = 0;

    return 0;
}

static int exec_cmd_video_change()
{
    VideoEncS video = {0};
    get_config(handleRealVideoCfg, video);

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        memcpy(&g_osd[group].venc, &video.enc[group], sizeof(VideoEnc0));
        encode_region_get_venc(group, &video.enc[group]);
        g_osd[group].media.dwFpsMax = video.enc[group].fps;
        dbg_osd("video dwFpsMax:%d\n", video.enc[group].fps);
    }

    return 0;
}

static int init_osd_resource(void)
{
    int ret = encode_freetype_init();
    ENCODE_RET_JUDGE(ret);

    VglineS vgLinecfg = {0};
    VgrectS vgRectcfg = {0};
    HumanDetectionS humancfg = {0};
    CarDetectionS carcfg = {0};
    follow_info_t followcfg = {0};
    get_config(handleVglineCfg, vgLinecfg);
    get_config(handleVgrectCfg, vgRectcfg);
    get_config(handleHumanDetectCfg, humancfg);
    get_config(handleCarDetectCfg, carcfg);
    get_config(handleFollowCfg, followcfg);

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        ret = encode_osd_text_init(group);
        ENCODE_RET_JUDGE(ret);

        if (TD_TRUE == vgLinecfg.enable) {
            ret = encode_osd_vgline_group_init(group, &vgLinecfg);
            ENCODE_RET_JUDGE(ret);

            if (vgLinecfg.blink) {
                ret = encode_osd_vgline_update(group, &vgLinecfg, -1, TD_TRUE);
                ENCODE_RET_JUDGE(ret);
            }
        }

        if (TD_TRUE == vgRectcfg.enable) {
            ret = encode_osd_vgrect_group_init(group, &vgRectcfg);
            ENCODE_RET_JUDGE(ret);

            if (vgRectcfg.blink) {
                ret = encode_osd_vgrect_update(group, &vgRectcfg, -1, TD_TRUE);
                ENCODE_RET_JUDGE(ret);
            }
        }

        if(TD_TRUE == humancfg.enable ||  TD_TRUE == carcfg.enable
            || TD_TRUE == followcfg.enable || TD_TRUE == humancfg.person_center)  {
            ret = encode_osd_aidet_group_init(group);
            ENCODE_RET_JUDGE(ret);
        }

        g_osd[group].bAvail = true;
    }

    DBG("osd resource init\n");
    return ret;
}

static int uninit_osd_resource(void)
{
    int ret = TD_SUCCESS;

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        g_osd[group].bAvail = false;

        ret = encode_osd_vgrect_group_uninit(group);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_vgline_group_uninit(group);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_aidet_group_uninit(group);
        ENCODE_RET_JUDGE(ret);

        ret = encode_osd_text_uninit(group);
        ENCODE_RET_JUDGE(ret);

    }

    ret = encode_freetype_uninit();
    ENCODE_RET_JUDGE(ret);

    DBG("osd resource uninit\n");
    return ret;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_OSD_STYLE) {
            memcpy(&g_osd_cfg->osdstyle, &g_osd_raw->osdstyle, sizeof(g_osd_cfg->osdstyle));
        }

        if (p_cmd->cmd_stage & CMD_OSD_INFO) {
            memcpy(&g_osd_cfg->osdinfo, &g_osd_raw->osdinfo, sizeof(g_osd_cfg->osdinfo));
        }

        if (p_cmd->cmd_stage & CMD_OSD_EXPAND) {
            if (g_osd_cfg->osdexpand.size != g_osd_raw->osdexpand.size || g_osd_cfg->osdexpand.font != g_osd_raw->osdexpand.font) {
                cmd_set_command(p_cmd, CMD_OSD_EXPAND_FONT);
                g_osd_run->expand_chg_flag |= 0xff;
            }

            for (int i = 0; i < ARRAY_SIZE(g_osd_cfg->osdexpand.cusosd); i++) {
                if (memcmp(&g_osd_cfg->osdexpand.cusosd[i], &g_osd_raw->osdexpand.cusosd[i],
                    sizeof(g_osd_raw->osdexpand.cusosd[i]))) {
                    g_osd_run->expand_chg_flag |= (0x01 << i);
                }
            }
            memcpy(&g_osd_cfg->osdexpand, &g_osd_raw->osdexpand, sizeof(g_osd_cfg->osdexpand));
        }
    }
}

static void loop_osd(void *ctx)
{
    static int s_loop_count = 0;
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd) {
        if (cmd & CMD_VIDEO_CHAGE) {
            exec_cmd_video_change();
        }

        if (cmd & CMD_OSD_STYLE) {
            exec_cmd_osd_style();
        }

        if (cmd & CMD_OSD_INFO) {
            exec_cmd_osd_info();
        }

        if (cmd & CMD_OSD_EXPAND) {
            exec_cmd_osd_expand(cmd);
        }
    }

    s_loop_count++;
    if (s_loop_count%TIME_UPDATE_COUNT == 0) {  // 500ms
        encode_osd_process_time_stream();
        if (s_loop_count%STREAM_UPDATE_COUNT == 0) {    // 2s
            encode_osd_process_stream_info();
        }
    }

}

int encode_osd_init(void)
{
    static struct cmdstat cmdstat_osd;
    struct cmdstat *ctx = &cmdstat_osd;
    cmdstat_osd.diff_cfg2cmd = diff_cfg2cmd;
    g_osd_run->p_ctx = ctx;

    /* STEP 1 */
    g_osd_run->sch = js_create_scheduler("sch_osd");
    get_config(handleOsdinfoCfg,   g_osd_cfg->osdinfo);
    get_config(handleOsdStyleCfg,  g_osd_cfg->osdstyle);
    get_config(handleOsdExpandCfg, g_osd_cfg->osdexpand);

    init_osd_resource();

    /* STEP 2 */
    attach_config(JEvent_OsdCfgChg,       cb_osd_info_cfg,   (void *)ctx);
    attach_config(JEvent_OsdStyleCfgChg,  cb_osd_style_cfg,  (void *)ctx);
    attach_config(JEvent_OsdExpandCfgChg, cb_osd_expand_cfg, (void *)ctx);
    attach_config(JEvent_VideoCfgChg    , cb_osd_video_cfg,  (void *)ctx);

    /* STEP 3 */
    js_create_timer_r(g_osd_run->sch, 2000, 100, loop_osd, ctx, &g_osd_run->hdl_loop);

    DBG("encode osd init\n");
    return 0;
}

int encode_osd_uninit(void)
{
    int ret = TD_SUCCESS;
    if (g_osd_run->hdl_loop != NULL) {
        js_delete_timer_r(&g_osd_run->hdl_loop);
    }

    detach_config(JEvent_OsdCfgChg,       cb_osd_info_cfg,   g_osd_run->p_ctx);
    detach_config(JEvent_OsdStyleCfgChg,  cb_osd_style_cfg,  g_osd_run->p_ctx);
    detach_config(JEvent_OsdExpandCfgChg, cb_osd_expand_cfg, g_osd_run->p_ctx);
    detach_config(JEvent_VideoCfgChg    , cb_osd_video_cfg,  g_osd_run->p_ctx);

    uninit_osd_resource();
    DBG("encode osd uninit\n");
    return ret;
}

