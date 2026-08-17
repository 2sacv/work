/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : encode_freetype.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2016-03-15
    Description  : 
    History      : 
                        created by tianjun. 2016-03-15
******************************************************************************/

#ifndef __ENCODE_FREETYPE_H__
#define __ENCODE_FREETYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FREETYPE_CHECK_AND_SETVALUE(a) do{		\
	pdwPtrDes[a] = pdwPtrDes[a] == colorFont ? colorFont : colorBack;	\
}while(0);

#include "encode_common.h"
#include "linux_list.h"
#include "ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftglyph.h"
#include "freetype/ftimage.h"

typedef struct venc_freetype_t
{
    FT_Face             hFTFace;
    FT_Library          hFTLib; 
    struct list_head    LatticeHead; 
    DWORD               dwCountLattice;
} TVencFT;

typedef enum {
    OE_DRAW_OSD_SUCC,
    OE_WIDTH_BACKWARD_RANGE,
    OE_WIDTH_FORWARD_RANGE,
    OE_HEIGHT_BACKWARD_RANGE,
    OE_HEIGHT_FORWARD_RANGE,
    OE_OUT_OF_BUFFER,
    MAX_NUM_OE
} eOSDError;

int encode_freetype_init(void);
int encode_freetype_uninit(void);
int encode_freetype_get_lattice(TFont *ptFont,DWORD dwUnicode,TLattice *ptLattice);
int encode_freetype_show_lattice(void);

#ifdef __cplusplus
}
#endif

#endif//__ENCODE_FREETYPE_H__

