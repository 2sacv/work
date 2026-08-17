/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_common.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2015-02-12
    Description  :
    History      :
                        created by tianjun. 2015-02-12
******************************************************************************/

#include "encode_common.h"
#include "system_ctrl.h"
#include "gpio.h"
#include "confapi.h"
#include "jconfstruct.h"
#include "shm_buf.h"
#include "g_log.h"

static BOOL gbThreadRun = TRUE;

int encode_thread_get_run(void)
{
    return gbThreadRun;
}

int encode_thread_set_quit(void)
{
    gbThreadRun = FALSE;
    return S_OK;
}
 
typedef struct {
    unsigned int r;
    unsigned int g;
    unsigned int b;
} tRgb;

static tRgb iRgbColorOSD[16] =
{
    {8,6,7},
    {32,34,33},
    {61,59,60},
    {88,87,86},
    {113,111,112},
    {139,137,138},
    {169,167,168},
    {195,193,194},

    {178,44,51},
    {24,170,0},
    {18,66,250},
    {33,175,221},
    {189,185,0},
    {180,37,163},
    
    {0,0,0},
    {255,255,255},
};

int encode_get_color(int index,unsigned int *r,unsigned int *g,unsigned int *b)
{
    *r = iRgbColorOSD[index].r;
    *g = iRgbColorOSD[index].g;
    *b = iRgbColorOSD[index].b;
    
    return 0;
}

void YCBCR_TO_RGB(int Y,int Cb,int Cr,int *R,int *G,int *B)
{
    *R = 1.164*(Y-16)+1.596*(Cr-128);
    *G = 1.164*(Y-16)-0.392*(Cb-128)-0.813*(Cr-128);
    *B = 1.164*(Y-16)+2.017*(Cb-128);  
}

void RGB_TO_YCBCR(int R,int G,int B,int *Y,int *Cb,int *Cr)
{
    *Y  = 0.257*R+0.564*G+0.098*B+16;
    *Cb = -0.148*R-0.291*G+0.439*B+128;
    *Cr = 0.439*R-0.368*G-0.071*B+128;
}

int encode_count_stat(int msec)
{
    int ret = S_OK;
    struct timeval time_now;
    static double time_new = 0;
    static double time_old = 0;
    static int count = 0;
    time_t tTime;
    struct tm time_tm;
    char timestr[24] = {0,};

    gettimeofday(&time_now,NULL);
    time_new = (double)(time_now.tv_sec*1000000.0+ time_now.tv_usec);
    count++;

    if(fabs(time_new-time_old) >= (msec*1000))
    {
        tTime = time(&tTime);
        localtime_r(&tTime, &time_tm);
        sprintf((char *)timestr, "%04d-%02d-%02d %02d:%02d:%02d",
                (int)(time_tm.tm_year + 1900), 
                (int)(time_tm.tm_mon + 1),
                (int)(time_tm.tm_mday),
                (int)(time_tm.tm_hour) % 24,
                (int)(time_tm.tm_min), 
                (int)(time_tm.tm_sec));

        time_old = time_new;
        dbg_venc("[%s] [msec:%d]  [count=%d] \n", timestr, msec, count);
        count = 0;
    } 
    return ret;
}

ETimeInterval encode_time_interval(struct timespec *ptime_pre,int interval_ms)
{
    ETimeInterval eTimeInterval = E_TIME_INTERVAL_ERR;
    struct timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    float time = 0;
    if (-1 == interval_ms)
    {
        ptime_pre->tv_sec = ts.tv_sec;
        ptime_pre->tv_nsec = ts.tv_nsec;
    }
    else
    {
        time = 	ts.tv_sec*1000.0+ts.tv_nsec/1000000-ptime_pre->tv_sec*1000.0-ptime_pre->tv_nsec/1000000;

        if (fabs(time) >= interval_ms)
        {
            eTimeInterval = E_TIME_INTERVAL_OK;
            ptime_pre->tv_sec = ts.tv_sec;
            ptime_pre->tv_nsec = ts.tv_nsec;
        }
    }
    return eTimeInterval;
}

#define MILLION 1000000
int MMPF_OsCounterGetMs(void)                                                            
{
    struct timespec t1;
    clock_gettime(CLOCK_BOOTTIME, &t1);
    //int T = (MILLION*(t1.tv_sec)+(t1.tv_nsec)/1000)/1000;
    int T = (1000*t1.tv_sec+t1.tv_nsec/1000000)%0x80000000;
    return T;
}
ECountCalc encode_count_calc(int *pcount_pre,int count_max)
{
    ECountCalc eCountCalc = E_COUNT_CALC_ERR;
    
    *pcount_pre = *pcount_pre + 1;
    if ((*pcount_pre) >= count_max)
    {
        *pcount_pre = 0;
        eCountCalc = E_COUNT_CALC_OK;
    }
    
    return eCountCalc;
}

int encode_memory_2d_copy(TRect *ptDesRect,TRect *ptSrcRect,DWORD dwSize)
{
    int ret = S_OK;
    int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0, w = 0, h = 0, q = 0;
    int w2 = ptSrcRect->dwWidth - ptSrcRect->dwOffX;
    int h2 = ptSrcRect->dwHeight - ptSrcRect->dwOffY;
    int w4 = ptDesRect->dwWidth - ptDesRect->dwOffX;
    int h4 = ptDesRect->dwHeight - ptDesRect->dwOffY;

    w = MIN(w2, w4);
    h = MIN(h2, h4);

    l = ptSrcRect->dwOffY;
    k = ptSrcRect->dwOffX;
    n = ptDesRect->dwOffY;
    m = ptDesRect->dwOffX;

    unsigned char *ptSrc1 = ptSrcRect->pcBuffer;
    unsigned char *ptDes1 = ptDesRect->pcBuffer;
    unsigned short *ptSrc2 = (unsigned short *)ptSrcRect->pcBuffer;
    unsigned short *ptDes2 = (unsigned short *)ptDesRect->pcBuffer;
    unsigned int *ptSrc4 = (unsigned int *)ptSrcRect->pcBuffer;
    unsigned int *ptDes4 = (unsigned int *)ptDesRect->pcBuffer;

    if ((0 == ptSrcRect->pcBuffer) || (0 == ptDesRect->pcBuffer))
    {
        ERR("SRC Buffer:%p DES Buffer:%p\n",ptSrcRect->pcBuffer,ptDesRect->pcBuffer);
        return ret;
    }

    switch(dwSize)
    {
    case 0: // 4-bit per pixel: two pixels per byte
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                // Source byte index and bit position
                int src_byte_idx = l * ptSrcRect->dwWidth + k + j;
                int src_is_high_nibble = (src_byte_idx & 1) == 0; // even index => high nibble
                unsigned char src_byte = ptSrc1[src_byte_idx >> 1];
                unsigned char src_nibble = src_is_high_nibble ? (src_byte >> 4) : (src_byte & 0x0F);

                // Destination byte index and bit position
                int des_byte_idx = n * ptDesRect->dwWidth + m + j;
                int des_is_high_nibble = (des_byte_idx & 1) == 0;
                int des_byte_addr = des_byte_idx >> 1;

                // Read current dest byte (to preserve other nibble)
                unsigned char des_byte = ptDes1[des_byte_addr];

                // Write the nibble
                if (des_is_high_nibble)
                {
                    ptDes1[des_byte_addr] = (des_byte & 0x0F) | (src_nibble << 4);
                }
                else
                {
                    ptDes1[des_byte_addr] = (des_byte & 0xF0) | src_nibble;
                }
            }
            l++;
            n++;
        }
        break;
    case 1:
        for (i=0; i<h; i++)
        {
            for (j=0; j<w; j++)
            {
                // Source pixel index and bit position
                int src_pixel_idx = l * ptSrcRect->dwWidth + k + j;
                int src_byte_idx = src_pixel_idx >> 3;
                int src_bit_pos = 7 - (src_pixel_idx & 0x7); // MSB first
                unsigned char src_bit = (ptSrc1[src_byte_idx] >> src_bit_pos) & 0x1;

                // Destination pixel index and bit position
                int des_pixel_idx = n * ptDesRect->dwWidth + m + j;
                int des_byte_idx = des_pixel_idx >> 3;
                int des_bit_pos = 7 - (des_pixel_idx & 0x7);

                // Read current dest byte (to preserve other bits)
                unsigned char des_byte = ptDes1[des_byte_idx];

                // Write the bit
                if (src_bit)
                    ptDes1[des_byte_idx] = des_byte | (1 << des_bit_pos);
                else
                    ptDes1[des_byte_idx] = des_byte & ~(1 << des_bit_pos);
            }
            l++;
            n++;
        }
        break;
    case 2:
        for (i=0; i<h; i++)
        {
            for (j=0;j<w;j++)
            {
                ptDes2[n*ptDesRect->dwWidth+j+m] = ptSrc2[l*ptSrcRect->dwWidth+j+k];
            }
            l++;
            n++;
        }
        break;
        
    case 3: // 2-bit per pixel: 4 pixels per byte
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                // Source pixel index and bit position (2 bits per pixel)
                int src_pixel_idx = l * ptSrcRect->dwWidth + k + j;
                int src_byte_idx = src_pixel_idx >> 2; // divide by 4
                int src_bit_shift = 6 - ((src_pixel_idx & 0x3) << 1); // MSB first: positions 6,4,2,0
                unsigned char src_2bit = (ptSrc1[src_byte_idx] >> src_bit_shift) & 0x3;

                // Destination pixel index and bit position
                int des_pixel_idx = n * ptDesRect->dwWidth + m + j;
                int des_byte_idx = des_pixel_idx >> 2;
                int des_bit_shift = 6 - ((des_pixel_idx & 0x3) << 1);

                // Read current dest byte (to preserve other 2-bit pixels)
                unsigned char des_byte = ptDes1[des_byte_idx];

                // Clear old 2-bit value and write new value
                ptDes1[des_byte_idx] = (des_byte & ~(0x3 << des_bit_shift)) | (src_2bit << des_bit_shift);
            }
            l++;
            n++;
        }
        break;
    case 4:
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                ptDes4[n*ptDesRect->dwWidth+j+m] = ptSrc4[l*ptSrcRect->dwWidth+j+k];
            }
            l++;
            n++;
        }
        break;

    default:
        for (i=0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                for (q = 0; q < dwSize; q++)
                {
                    ptDes1[(n*ptDesRect->dwWidth+j+m)*dwSize+q] = ptSrc1[(l*ptSrcRect->dwWidth+j+k)*dwSize+q];   
                }
            }
            l++;
            n++;
        }
        break;
    }
    
    return ret;
}

int encode_debug_show_lattice(TLattice *ptLattice)
{
    int ret = S_OK;
    int x = 0;
    int y = 0;
    
     for(x = 0;x < ptLattice->dwHeight;x++)
    {
        for(y = 0;y < ptLattice->dwWidth;y++)
        {
            if(ptLattice->pcBuffer[x*ptLattice->dwWidth + y] == 255)
            {
                printf("0");
            }
            else
            {  
                printf("_");
            }  

        }
        printf("\n");
    }
        
    return ret;
}


int encode_debug_show_rect(TRect *ptTRect)
{
    int ret = S_OK;
    int x = 0;
    int y = 0;
    int w = MIN(ptTRect->dwWidth,ptTRect->dwPitch-ptTRect->dwOffX);
    int h = ptTRect->dwHeight;

    DBG("TRect:[%d][%d][%d][%d][%d] \n",
        ptTRect->dwOffX,
        ptTRect->dwOffY,
        ptTRect->dwWidth,
        ptTRect->dwHeight,
        ptTRect->dwPitch
        );
    
    for(x = 0;x < h;x++)
    {
        for(y = 0;y < w;y++)
        {
            if(ptTRect->pcBuffer[x*ptTRect->dwPitch + y + ptTRect->dwOffX] == 255)
            {
                printf("0");
            }
            else
            {  
                printf("_");
            }  

        }
        printf("\n");
    }
        
    return ret;
}

int xslt_media_type(VideoEnc0 *p)
{
    if (p->codec == VENC_FORMAT_H264) {
        return SHM_MEDIA_VIDEO_H264;
    } else {
        return SHM_MEDIA_VIDEO_H265;
    }
}
