/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_freetype.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2016-03-15
    Description  :
    History      :
                        created by tianjun. 2016-03-15
******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#include "encode_common.h"
#include "encode_freetype.h"

TVencFT gtVencFT;

static int encode_freetype_unregister_all_lattice(void)
{
    int ret = S_OK;
    struct list_head *pPos = NULL;
    struct list_head *pNn = NULL;
    TLattice *ptLattice = NULL;

    list_for_each_safe(pPos, pNn, &gtVencFT.LatticeHead)
    {
        ptLattice = list_entry(pPos, TLattice, list);
        if (ptLattice) {
            system_free(ptLattice->pcBuffer);
            ptLattice->pcBuffer = NULL;
            list_del(pPos);
            system_free(ptLattice);
            ptLattice = NULL;
        }

    }
    DBG("encode_freetype_unregister_all_lattice \n");
    return ret;
}

static int encode_freetype_check_count(void)
{
    int ret = S_OK;

    if (gtVencFT.dwCountLattice > MAX_LATTICE_NUMBER) {
        ret = encode_freetype_unregister_all_lattice();
        gtVencFT.dwCountLattice = 0;
        ENCODE_RET_JUDGE(ret);
    }
    return ret;
}

int encode_freetype_init(void)
{
    memset(&gtVencFT, 0, sizeof(gtVencFT));
    INIT_LIST_HEAD(&gtVencFT.LatticeHead);
    return S_OK;
}

int encode_freetype_uninit(void)
{
    int ret = S_OK;
    ret = encode_freetype_unregister_all_lattice();
    ENCODE_RET_JUDGE(ret);
    return ret;
}

static int BrushSideEx(unsigned char *pDataBuffer, short x, short y, unsigned char uSideColor, unsigned char uBgColorIdx,
                                                unsigned int u32OSDWidth, unsigned int u32OSDHeight, unsigned char nPixBit) {
    if ((x >= (short)(u32OSDWidth)) || ((x + 1) >= (short)(u32OSDWidth))) {
        return OE_WIDTH_BACKWARD_RANGE;
    }

    if (((x - 1) < 0) || x < 0) {
        return OE_WIDTH_FORWARD_RANGE;
    }

    if ((y >= (short)(u32OSDHeight)) || ((y + 1) >= (short)(u32OSDHeight))) {
        return OE_HEIGHT_BACKWARD_RANGE;
    }

    if (((y - 1) < 0) || y < 0) {
        return OE_HEIGHT_FORWARD_RANGE;
    }

    struct pix_posi_t {
        int x;
        int y;
    } pix_posi_arry[8] = {
        {x, y - 1}, {x, y + 1}, {x - 1, y}, {x + 1, y}, {x - 1, y - 1}, {x + 1, y - 1}, {x - 1, y + 1}, {x + 1, y + 1},
    };

    unsigned char pix_valid = (0xFF) >> (8 - nPixBit);
    unsigned char uPixCntInByte = 8 / nPixBit;
    unsigned int u32ByteW = u32OSDWidth * nPixBit / 8 ;

    for (int i = 0; i < 8; i++) {
        unsigned int pix_buf_pos_x = pix_posi_arry[i].x * nPixBit / 8;
        unsigned int pix_dot_pos = (pix_posi_arry[i].x % uPixCntInByte);
        unsigned char cur_color = (pDataBuffer[pix_buf_pos_x + pix_posi_arry[i].y * u32ByteW] >> (pix_dot_pos * nPixBit)) & pix_valid;

        if (!(uBgColorIdx ^ cur_color)) {
            pDataBuffer[pix_buf_pos_x + pix_posi_arry[i].y * u32ByteW] |= ((uSideColor & pix_valid) << (pix_dot_pos * nPixBit));
        }
    }

    return OE_DRAW_OSD_SUCC;
}

static int DrawPoint(unsigned char *pDataBuffer, SHORT x, SHORT y, WORD uFontColor,
                    DWORD u32OSDWidth, DWORD u32OSDHeight, int bpp)
{
    if (x >= u32OSDWidth) {
        return OE_WIDTH_BACKWARD_RANGE;
    }

    if (x < 0) {
        return OE_WIDTH_FORWARD_RANGE;
    }

    if (y >= u32OSDHeight) {
        return OE_HEIGHT_BACKWARD_RANGE;
    }

    if (y < 0) {
        return OE_HEIGHT_FORWARD_RANGE;
    }

    unsigned char  uPixCntInByte = 8 / bpp;
    unsigned int u32ByteW = u32OSDWidth * bpp / 8 ;

    if ((x + y * u32OSDWidth) < u32OSDWidth * u32OSDHeight) {
        unsigned int pix_buf_pos_x = x * bpp / 8;
        unsigned int pix_dot_pos = (x % (uPixCntInByte));
        unsigned char pix_valid = (0xFF) >> (8 - bpp);

        // clear old value
        pDataBuffer[pix_buf_pos_x + y * u32ByteW] &= ~((pix_valid) << (pix_dot_pos * bpp));

        // add new value
        pDataBuffer[pix_buf_pos_x + y * u32ByteW] |= ((uFontColor & pix_valid) << (pix_dot_pos * bpp));
        return OE_DRAW_OSD_SUCC;
    } else {
        return OE_OUT_OF_BUFFER;
    }
}

FT_Bitmap *FTGetGlpyhBitMap(WORD u16CharCode) {
    int s32Error = 0;

    s32Error = FT_Load_Char(gtVencFT.hFTFace, u16CharCode, FT_LOAD_RENDER | FT_LOAD_MONOCHROME | FT_LOAD_NO_AUTOHINT);
    if (0 != s32Error) {
        printf("FT_Load_Glyph failed!\r\n");
        return NULL;
    }

    return &gtVencFT.hFTFace->glyph->bitmap;
}

#if 0
static int dyn_save_latbitmap(void *buf, int len,int width)
{
    int i = 0;
    FILE *fp = NULL;
    char filepath[128] = {0};
    snprintf(filepath,127,"/opt/lattice-%d.map",width);

    if (!fp) {
        fp = fopen(filepath, "w");
    }

    int wr = fwrite(buf, len, 1, fp);
    DBG("___i[%d]_ len:%d wr:%d\n", i++, len, wr);

    fclose(fp);
    fp = NULL;

    return 0;
}
#endif

static int encode_freetype_lattice(TFont *ptFont,DWORD dwUnicode,TLattice *ptLattice)
{
    int ret = S_OK;
    DWORD dwSize = 0;

    unsigned int pointcolor = OSD_COLOR_IDX_WHITE;
    unsigned int brushsidecolor = OSD_COLOR_IDX_BLACK;
    FT_Bitmap *ftBitmap = NULL;
    int sBearingY = 0;
    BYTE dot8 = 0x0;

    if (1 == ptFont->dwFontColor) {
        pointcolor = OSD_COLOR_IDX_WHITE;
        brushsidecolor = OSD_COLOR_IDX_BLACK;
    } else if (2 == ptFont->dwFontColor) {
        pointcolor = OSD_COLOR_IDX_BLACK;
        brushsidecolor = OSD_COLOR_IDX_WHITE;
    } else {
        pointcolor = OSD_COLOR_IDX_WHITE;
        brushsidecolor = OSD_COLOR_IDX_BLACK;
    }

    ret = FT_Init_FreeType(&gtVencFT.hFTLib);
    ENCODE_RET_JUDGE(ret);

    ret = FT_New_Face(gtVencFT.hFTLib, ptFont->cFontFileName, 0,&gtVencFT.hFTFace);
    ENCODE_RET_JUDGE(ret);

    ret = FT_Select_Charmap(gtVencFT.hFTFace, FT_ENCODING_UNICODE);
    ENCODE_RET_JUDGE(ret);

    ret = FT_Set_Pixel_Sizes(gtVencFT.hFTFace, ptFont->dwFontWidth, ptFont->dwFontHeight);
    ENCODE_RET_JUDGE(ret);

    do {
        //生成位图
        ftBitmap = FTGetGlpyhBitMap(dwUnicode);
        if (NULL == ftBitmap){
            ERR("get glpy bitmap failed\n");
            break;
        }
        ptLattice->dwUnicode = dwUnicode;
        ptLattice->dwWidth = ALIGN_UP(gtVencFT.hFTFace->glyph->metrics.horiAdvance>>6, 4);
        ptLattice->dwHeight =ALIGN_UP(ptFont->dwFontHeight, 4);
        #if 0
        if ((dwUnicode >= 'A' && dwUnicode <= 'Z') ||   // 大写字母
            (dwUnicode >= 'a' && dwUnicode <= 'z') ||   // 小写字母
            (dwUnicode >= '0' && dwUnicode <= '9') ||   // 数字
            (dwUnicode == '-') ||                       // 连字符
            (dwUnicode == '_') ||                       // 下划线
            (dwUnicode == '.') ||                       // 点
            (dwUnicode == ':') ||                       // 冒号
            (dwUnicode == '/') ||                       // 斜杠
            (dwUnicode == '=') ||                       // 等于号
            (dwUnicode == ',') ||                       // 逗号
            (dwUnicode == ';')                          // 分号
        ) {
            if ((ptLattice->dwWidth - ftBitmap->width) < 4) {
                ptLattice->dwWidth += 4;
            }
        }
        #endif

        memcpy(&ptLattice->tFont,ptFont,sizeof(TFont));

        //所需字符空间为0则直接退出
        dwSize = (ptLattice->dwWidth * ptLattice->dwHeight) / 4;
        if (0 == dwSize)
        {
            ERR("dwSize is 0\n");
            ptLattice->pcBuffer = NULL;
            break;
        }
        //COLOR_Y("unicode:%d ptLattice.width:%d ptLattice.height:%d ptFont.width:%d ptFont.height:%d\n",dwUnicode,ptLattice->dwWidth,ptLattice->dwHeight,ptFont->dwFontWidth,ptFont->dwFontHeight);
        //COLOR_Y("ftBitmap width:%d rows:%d \n",ftBitmap->width, ftBitmap->rows);
        //COLOR_Y("glyph horiBearingX:%d horiBearingY:%d \n",gtVencFT.hFTFace->glyph->metrics.horiBearingX, gtVencFT.hFTFace->glyph->metrics.horiBearingY);

        //申请存储空间
        ptLattice->pcBuffer = system_malloc(dwSize);
        ENCODE_NULL_JUDGE(ptLattice->pcBuffer);
        memset(ptLattice->pcBuffer, 0x0, dwSize);

        //开始画字
        sBearingY = gtVencFT.hFTFace->glyph->metrics.horiBearingY;
        int x0 = (int)abs((ptLattice->dwWidth - ftBitmap->width) / 2);
        int y0 = (int)abs(ptLattice->dwHeight - (int)(8.0 * ptLattice->dwHeight / 64) - sBearingY / 64);

        //COLOR_Y("x0:%d y0:%d\n",x0, y0);
        for (WORD j = 0; j < ftBitmap->rows; j++) {
                for (SHORT k = 0; k < ftBitmap->pitch; k++) {
                    dot8 = ftBitmap->buffer[k + j * ftBitmap->pitch];
                    for (WORD dot = 0; dot < 8; dot++) {
                        if ((dot8 & 0x80) == 0x80) {
                            DrawPoint(ptLattice->pcBuffer, x0 + k * 8 + dot, y0 + j,
                                pointcolor, ptLattice->dwWidth, ptLattice->dwHeight, 2);
                            BrushSideEx(ptLattice->pcBuffer, x0 + k * 8 + dot, y0 + j,
                                brushsidecolor, 0, ptLattice->dwWidth, ptLattice->dwHeight, 2);
                        }
                        dot8 = dot8 << 1;
                    }
                }
        }
    }while(0);

    //dyn_save_latbitmap(ptLattice->pcBuffer,dwSize,ptLattice->dwHeight);

    // 开始退出
    ret = FT_Done_Face(gtVencFT.hFTFace);
    ENCODE_RET_JUDGE(ret);

    ret = FT_Done_FreeType(gtVencFT.hFTLib);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

static int encode_freetype_register_lattice(TLattice *ptLatticeExt)
{
    int ret = S_OK;
    struct list_head *pPos = NULL;
    struct list_head *pNn = NULL;
    TLattice *ptLattice = NULL;
    BOOL bFlagFind = FALSE;

    list_for_each_safe(pPos, pNn, &gtVencFT.LatticeHead)
    {
        ptLattice = list_entry(pPos, TLattice, list);
        if ((ptLattice->dwUnicode == ptLatticeExt->dwUnicode)
            && (ptLattice->tFont.dwFontWidth == ptLatticeExt->tFont.dwFontWidth)
            && (ptLattice->tFont.dwFontHeight == ptLatticeExt->tFont.dwFontHeight)
            && (ptLattice->tFont.dwFontColor == ptLatticeExt->tFont.dwFontColor)
            && (0 == strcmp(ptLattice->tFont.cFontFileName,ptLatticeExt->tFont.cFontFileName))
        ) {
            bFlagFind = TRUE;
            break;
        }
    }

    if (FALSE == bFlagFind) {
        ptLattice = system_malloc(sizeof(TLattice));
        ENCODE_NULL_JUDGE(ptLattice);

        memcpy(ptLattice,ptLatticeExt,sizeof(TLattice));
        INIT_LIST_HEAD(&ptLattice->list);

        list_add_tail(&ptLattice->list, &gtVencFT.LatticeHead);
        gtVencFT.dwCountLattice++;
    }

    return ret;
}

static BOOL encode_freetype_register_is_ok(TFont *ptFont,DWORD dwUnicode,TLattice *ptLatticeExt)
{
    struct list_head *pPos = NULL;
    struct list_head *pNn = NULL;
    TLattice *ptLattice = NULL;
    BOOL bFlagFind = FALSE;

    list_for_each_safe(pPos, pNn, &gtVencFT.LatticeHead)
    {
        ptLattice = list_entry(pPos, TLattice, list);
        if (ptLattice) {
            if ((ptLattice->dwUnicode == dwUnicode)
            && (ptLattice->tFont.dwFontWidth == ptFont->dwFontWidth)
            && (ptLattice->tFont.dwFontHeight == ptFont->dwFontHeight)
            && (ptLattice->tFont.dwFontColor == ptFont->dwFontColor)
            && (0 == strcmp(ptLattice->tFont.cFontFileName,ptFont->cFontFileName))
            ) {
                bFlagFind = TRUE;
                memcpy(ptLatticeExt,ptLattice,sizeof(TLattice));
                break;
            }
        }
    }

    return bFlagFind;
}

int encode_freetype_get_lattice(TFont *ptFont,DWORD dwUnicode,TLattice *ptLattice)
{
    int ret = S_OK;
    TLattice tLattic;

    if ((NULL == ptFont) || (NULL == ptLattice)){
        ERR("param is null \n");
        return S_FAIL;
    }

    ret = encode_freetype_check_count();
    ENCODE_RET_JUDGE(ret);

    // 寻找是否已经注册
    if (TRUE == encode_freetype_register_is_ok(ptFont,dwUnicode,&tLattic)) {
        memcpy(ptLattice,&tLattic,sizeof(TLattice));
        //DBG("Lattice is register before !\n");
    } else {
        ret = encode_freetype_lattice(ptFont,dwUnicode,ptLattice);
        ENCODE_RET_JUDGE(ret);

        ret = encode_freetype_register_lattice(ptLattice);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

int encode_freetype_show_lattice(void)
{
    int ret = S_OK;
    struct list_head *pPos = NULL;
    struct list_head *pNn = NULL;
    TLattice *ptLattice = NULL;

    list_for_each_safe(pPos, pNn, &gtVencFT.LatticeHead)
    {
        ptLattice = list_entry(pPos, TLattice, list);
        DBG("Lattic [%d] [%d][%d]\n",ptLattice->dwUnicode,ptLattice->dwWidth,ptLattice->dwHeight);
    }

    return ret;
}

