#ifdef _MSC_VER
#include "stdafx.h"
#include <io.h>
#endif
#include <unistd.h>
#include <errno.h>

#include "libmp4.h"
#include "debug.h"
#include "../../utils/inc/utils.h"

#define UNUSED(x) (void)(x)  // 本文件中有些标志需要读取出来，但暂时未使用，使用该宏来规避警告

#if __REC__
CBitStream::CBitStream()
{
    m_pBitStream = NULL;
    m_cbBitStream=0;
    m_nBitPos=0;

    m_byBuf=NULL;
}

CBitStream::~CBitStream()
{
    UnInit();
}

BOOL32 CBitStream::Init(LPBYTE pBitStream,int cbBitStream,int nBitPos)
{
    UnInit();

    m_pBitStream = pBitStream;
    m_cbBitStream=cbBitStream;
    m_nBitPos=nBitPos;

    return nBitPos<cbBitStream;
}


size_t CReadFile::fread( void *buffer, size_t size, size_t count)
{
	ASSERT(m_hFile);
	//size_t ret = ::fread(buffer,size,count,m_hFile);
	size_t ret = system_fread(buffer,size,count,m_hFile);
	return ret;
}

static int GetHex(char ch)
{
    ch=tolower(ch);
    ASSERT((ch>='0' && ch<='9') || (ch>='a' && ch<='f'));
    if((ch>='0' && ch<='9'))
        return ch-'0';
    return ch-'a'+10;
}


BOOL32 CBitStream::Init(const char *pszHex,int cbLen)
{
    UnInit();

    char szBS[4096];
    ASSERT(cbLen < (int)sizeof(szBS));
    szBS[sizeof(szBS)-1]=0;
    strncpy(szBS,pszHex,sizeof(szBS)-1);
    ASSERT((strlen(szBS)%2)==0);

    if(cbLen==-1) {
        cbLen=strlen(szBS);
    }

    int bsLen=cbLen/2;
    m_byBuf = new BYTE[bsLen];
    memset(m_byBuf,0,bsLen);

    LPBYTE p = m_byBuf;
    for(int i=0; i<cbLen; i+=2) {
        BYTE value = (GetHex(szBS[i])<<4)|GetHex(szBS[i+1]);
        *p++=value;
    }

    m_pBitStream=m_byBuf;
    m_cbBitStream=bsLen*8;

    return TRUE;
}

void CBitStream::UnInit()
{
    if(m_byBuf) {
        delete []m_byBuf;
        m_byBuf=NULL;
    }

    m_pBitStream = NULL;
    m_cbBitStream=0;
    m_nBitPos=0;
}


//从当前位置读取nBit位数据
UINT CBitStream::Read(int nBit)
{
    ASSERT(nBit<=32);

    if(m_nBitPos + nBit>=m_cbBitStream) {
        ASSERT(FALSE);
        return 0;
    }

    UINT value=0;
    for(int i=0; i<nBit; i++) {
        int bytepos = (m_nBitPos +i)/8;
        int bitpos = 7 - ((m_nBitPos +i)%8);
        //DT("bitpos=%d",bitpos);
        value<<=1;
        BYTE by=m_pBitStream[bytepos];
        if( by & (1<<bitpos)) {
            value|=1;
        }
    }

    m_nBitPos += nBit;
    return value;
}

UINT CBitStream::Peek(int nBit)
{
    UINT ret = Read(nBit);
    m_nBitPos -= nBit;//回退
    return ret;
}

BOOL32 CBitStream::SkipBit(int nBit)
{
    if(m_nBitPos + nBit>=m_cbBitStream) {
        ASSERT(FALSE);
        return 0;
    }
    m_nBitPos += nBit;
    return TRUE;
}

BOOL32 CBitStream::SetBit(UINT nBitPos,BOOL32 bSet)
{
    if(nBitPos>=(UINT)m_cbBitStream) {
        ASSERT(FALSE);
        return FALSE;
    }

    UINT nBytePos=nBitPos/8;
    UINT nbpos=7-(nBitPos%8);

    if(bSet) {
        m_pBitStream[nBytePos]|=(1<<nbpos);
    } else {
        m_pBitStream[nBytePos]&=~(1<<nbpos);
    }

    return TRUE;
}

//以小端模式从nBitPos开始,写cbBit位,原数据会被覆盖
BOOL32 CBitStream::WriteBit(UINT nBitPos,UINT nValue,int cbBit)
{
    ASSERT(cbBit<=32);

    int nbpos = nBitPos;
    for(int i=0; i<cbBit; i++) {
        int bit = (cbBit-i-1);

        BOOL32 bset= (BOOL32)((nValue & (1<<bit)) == (UINT)(1<<bit));
        SetBit(nbpos,bset);
        nbpos++;
    }

    return TRUE;
}


#define visual_object_sequence_start_code   0x000001B0
#define visual_object_sequence_end_code     0x000001B1
#define user_data_start_code                0x000001B2
#define visual_object_start_code            0x000001B5

int CMpeg4BitStream::Parse()
{
    int ret = Parse_VisualObjectSequence();
    return ret;
}

int CMpeg4BitStream::Parse_VisualObject()
{
    DWORD32 dwCode = Read(32);
    ASSERT(dwCode == visual_object_start_code);
    BOOL32 is_visual_object_identifier=Read(1);
    if(is_visual_object_identifier) {
        ASSERT(FALSE);
        //visual_object_verid
        //visual_object_priority
    }

    BYTE visual_object_type = (BYTE)Read(4);
    switch(visual_object_type) {
        case 0x01: { //video ID
            Parse_video_signal_type();
            break;
        }
        default: {
            ASSERT(FALSE);
            break;
        }
    }

    next_start_code();

    dwCode = Peek(32);
    if(dwCode == user_data_start_code) {
        ASSERT(FALSE);
    }

    if(visual_object_type == 0x01) { //video ID
        BYTE video_object_start_code = Read(32);
        ASSERT(video_object_start_code<=0x1F);
        VideoObjectLayer();
        return -1;
    }

    return 0;
}

UINT CMpeg4BitStream::next_bits(int nBit)
{
    return Peek(nBit);
}

int CMpeg4BitStream::VideoObjectLayer()
{
    //video_object_layer_start_code:0x20~0x2F
    BYTE video_object_layer_start_code = next_bits(32);
    if(video_object_layer_start_code>=0x20 && video_object_layer_start_code<=0x2F) {
        BYTE short_video_header = 0;
        UNUSED(short_video_header);
    
        video_object_layer_start_code=Read(32);
        BOOL32 random_accessible_vol=Read(1);
        UNUSED(random_accessible_vol);
        
        BYTE video_object_type_indication=(BYTE)Read(8);
        UNUSED(video_object_type_indication);
        
        BOOL32 is_object_layer_identifier=Read(1);

        BYTE video_object_layer_verid =0;
        BYTE video_object_layer_priority=0;
        if(is_object_layer_identifier) {
            video_object_layer_verid = (BYTE)Read(4);
            video_object_layer_priority=(BYTE)Read(3);
            UNUSED(video_object_layer_verid);
            UNUSED(video_object_layer_priority);
        }

        BYTE aspect_ratio_info=(BYTE)Read(4);
#define EXTENDED_PAR    15
        if(aspect_ratio_info == EXTENDED_PAR) {
            BYTE par_width = Read(8);
            BYTE par_height = Read(8);
            UNUSED(par_width);
            UNUSED(par_height);
        }

        BOOL32 vol_control_parameters=Read(1);
        if(vol_control_parameters) {
            ASSERT(FALSE);
        }

        BYTE video_object_layer_shape=(BYTE)Read(2);
#define rectangular 0x00
#define binary      0x01
#define binary_only 0x02
#define grayscale   0x03    //ISO 14496-2_2001 pdf page 130
        ASSERT(video_object_layer_shape != grayscale);

        BYTE marker_bit=(BYTE)Read(1);
        ASSERT(marker_bit==1);
        WORD vop_time_increment_resolution=Read(16);
        UNUSED(vop_time_increment_resolution);
        
        marker_bit=(BYTE)Read(1);
        ASSERT(marker_bit==1);

        BYTE fixed_vop_rate=(BYTE)Read(1);
        ASSERT(!fixed_vop_rate);

        if(video_object_layer_shape != binary_only) {
            if(video_object_layer_shape == rectangular) {
                marker_bit=(BYTE)Read(1);
                ASSERT(marker_bit==1);
                {
                    //int nbpos = GetBitPos();
                    //DT("bit pos = %d[%d:%d]",nbpos,nbpos/8,nbpos%8);
                }
                WORD video_object_layer_width=Read(13);
                marker_bit=(BYTE)Read(1);
                ASSERT(marker_bit==1);
                WORD video_object_layer_height=Read(13);
                marker_bit=(BYTE)Read(1);
                ASSERT(marker_bit==1);

                UNUSED(video_object_layer_width);
                UNUSED(video_object_layer_height);
                //DT("size =%d,%d",video_object_layer_width,video_object_layer_height);
            }

            BYTE interlaced=(BYTE)Read(1);
            BYTE obmc_disable=(BYTE)Read(1);
            UNUSED(interlaced);
            UNUSED(obmc_disable);
            
            BYTE sprite_enable=0;
            if(video_object_layer_verid==0x01) {
                sprite_enable=Read(1);
            } else {
                sprite_enable=Read(2);
            }
#define SpriteCodingMode_static     0x01
#define SpriteCodingMode_GMC        0x02
#define SpriteCodingMode_Reserved   0x03

            ASSERT(sprite_enable==0);

            ASSERT(video_object_layer_verid==0x01);
            //if (video_object_layer_verid != ‘0001’ && video_object_layer_shape != ”rectangular”)

            BYTE not_8_bit=Read(1);
            if(not_8_bit) {
                BYTE quant_precision=(BYTE)Read(4);
                BYTE bits_per_pixel=(BYTE)Read(4);
                UNUSED(quant_precision);
                UNUSED(bits_per_pixel);
            }

            ASSERT(video_object_layer_shape != grayscale);

            BYTE quant_type=(BYTE)Read(1);
            ASSERT(!quant_type);

            if(video_object_layer_verid != 0x01) {
                BYTE quarter_sample=(BYTE)Read(1);
                UNUSED(quarter_sample);
            }

            BYTE complexity_estimation_disable=(BYTE)Read(1);
            if(!complexity_estimation_disable) {
                ASSERT(FALSE);
            }

            BYTE resync_marker_disable=(BYTE)Read(1);
            UNUSED(resync_marker_disable);
            
            BYTE data_partitioned=(BYTE)Read(1);
            if(data_partitioned) {
                BYTE reversible_vlc=(BYTE)Read(1);
                UNUSED(reversible_vlc);
            }

            if(video_object_layer_verid != 0x01) {
                ASSERT(FALSE);
            }
        }

        //下面的解析可能有错,14496-2 pdf中嵌套太深,block层次不容易看清楚
        BYTE scalability=(BYTE)Read(1);
        if(scalability) {
            ASSERT(FALSE);
        }

        //todo
        //int here=0;
    }

    return 0;
}

int CMpeg4BitStream::Parse_VisualObjectSequence()
{
    do {
        DWORD32 dwCode = Read(32);
        ASSERT(dwCode == visual_object_sequence_start_code);

        BYTE profile = Read(8);
        UNUSED(profile);

        while(Peek(32) == user_data_start_code) {
            ASSERT(FALSE);
            //user_data();
        }

        if(-1==Parse_VisualObject())
            return -1;

    } while(Peek(32) != visual_object_sequence_end_code);

    DWORD32 dwCode = Read(32);
    ASSERT(dwCode == visual_object_sequence_end_code);

    return 0;
}

int CMpeg4BitStream::Parse_video_signal_type()
{
    BOOL32 video_signal_type = Read(1);
    if(video_signal_type) {
        BYTE video_format=Read(3);
        UNUSED(video_format);
//video_format Meaning
//000 Component
//001 PAL
//010 NTSC
//011 SECAM
//100 MAC
//101 Unspecified video format
//110 Reserved
//111 Reserved

        BYTE video_range=Read(1);
        UNUSED(video_range);

        BYTE colour_description=Read(1);
        if(colour_description) {
            BYTE colour_primaries=Read(8);
            BYTE transfer_characteristics=Read(8);
            BYTE matrix_coefficients=Read(8);
            UNUSED(colour_primaries);
            UNUSED(transfer_characteristics);
            UNUSED(matrix_coefficients);
        }
    }

    return 0;
}

int CMpeg4BitStream::next_start_code()
{
    BOOL32 zero_bit=(BOOL32)Read(1);
    ASSERT(!zero_bit);
    while(!bytealigned()) {
        register BOOL32 one_bit = Read(1);
        ASSERT(one_bit);
    }
    return 0;
}

bool CMpeg4BitStream::bytealigned()
{
    if( (GetBitPos()%8)==0)
        return true;
    return false;
}

CMP4Base::CMP4Base()
{
    m_hFile=NULL;
}

CMP4Base::~CMP4Base()
{

}

unsigned CMP4Base::addByte(unsigned char byte)
{
    ASSERT(m_hFile);
    putc(byte, m_hFile);
    return 1;
}

long CMP4Base::addAtomHeader(char const* atomName)
{
    // Output a placeholder for the 4-byte size:
    addWord(0);
    // Output the 4-byte atom name:
    add4ByteString(atomName);
    return 8;
}

//注意:
//代码参考了live555中的openRTSP,它用word表示4个字节,用HalfWord表示两个字节
//不要和Windows下的DWORD32,WORD搞混
unsigned CMP4Base::addWord(unsigned word)
{
    addByte(word>>24);
    addByte(word>>16);
    addByte(word>>8);
    addByte(word);
    return 4;
}

unsigned CMP4Base::addHalfWord(unsigned short halfWord)
{
    addByte((unsigned char)(halfWord>>8));
    addByte((unsigned char)halfWord);
    return 2;
}

unsigned CMP4Base::addZeroWords(unsigned numWords)
{
    for (unsigned i = 0; i < numWords; ++i) {
        addWord(0);
    }

    return numWords*4;
}

unsigned CMP4Base::add4ByteString(char const* str)
{
    addByte(str[0]);
    addByte(str[1]);
    addByte(str[2]);
    addByte(str[3]);
    return 4;
}

unsigned CMP4Base::addArbitraryString(char const* str,Boolean oneByteLength)
{
    unsigned size = 0;
    if (oneByteLength) {
        // Begin with a byte containing the string length:
        unsigned strLength = strlen(str);
        if (strLength >= 256) {
            ASSERT(FALSE);
        }
        size += addByte((unsigned char)strLength);
    }

    while (*str != '\0') {
        size += addByte(*str++);
    }

    return size;
}

void CMP4Base::setWord(unsigned filePosn, unsigned size)
{
	int times = 3;
	do {
		times--;
		if (fseek(m_hFile, filePosn, SEEK_SET) < 0) {
			DBG("times:%d m_hFile:%p, setWord! error %d %s\n", times, m_hFile, errno, strerror(errno));
			if(EINTR == errno || errno == EAGAIN) {
				usleep(5000);
				continue;
			}
			goto out;
		}
		break;
	} while (times > 0);

	addWord(size);

	times = 3;
    do {
		times--;
        if (fseek(m_hFile, filePosn + size, SEEK_SET) < 0) {
			DBG("times:%d m_hFile:%p, setWord! error %d %s\n", times, m_hFile, errno, strerror(errno));
			if(EINTR == errno || errno == EAGAIN) {
				usleep(5000);
				continue;
			}
			goto out;
		}
		break;
    } while (times > 0);
	
	return;
	
out:
    // One of the fseek()s failed, probable because we're not a seekable file
    ASSERT(FALSE);
}

void CMP4Base::ModifyByte(unsigned filePosn,BYTE newValue)
{
    long pos = ftell(m_hFile);
    fseek(m_hFile,filePosn,SEEK_SET);
    putc(newValue, m_hFile);
    fseek(m_hFile,pos,SEEK_SET);
}

int CMP4Base::gettimeofday(struct timeval* tp)
{
#ifdef _MSC_VER

    static LARGE_INTEGER tickFrequency, epochOffset;

    // For our first call, use "ftime()", so that we get a time with a proper epoch.
    // For subsequent calls, use "QueryPerformanceCount()", because it's more fine-grain.
    static Boolean isFirstCall = TRUE;

    LARGE_INTEGER tickNow;
    QueryPerformanceCounter(&tickNow);

    if (isFirstCall) {
        struct timeb tb;
        ftime(&tb);
        tp->tv_sec = tb.time;
        tp->tv_usec = 1000*tb.millitm;

        // Also get our counter frequency:
        QueryPerformanceFrequency(&tickFrequency);
        // And compute an offset to add to subsequent counter times, so we get a proper epoch:
        epochOffset.QuadPart = tb.time*tickFrequency.QuadPart + (tb.millitm*tickFrequency.QuadPart)/1000 - tickNow.QuadPart;

        isFirstCall = FALSE; // for next time
    } else {
        // Adjust our counter time so that we get a proper epoch:
        tickNow.QuadPart += epochOffset.QuadPart;

        tp->tv_sec = (long) (tickNow.QuadPart / tickFrequency.QuadPart);
        tp->tv_usec = (long) (((tickNow.QuadPart % tickFrequency.QuadPart) * 1000000L) / tickFrequency.QuadPart);
    }
#else
    //linux

    ::gettimeofday(tp, NULL);
#endif

    return 0;
}

const char *CMP4Base::GetMpeg4FrameTypeDesc(LPBYTE pData)
{
    static const char *arr[]= {
        "invalid mpeg4 frame",
        "I Frame",
        "P Frame",
        "B Frame",
    };

    UINT idx = GetMpeg4FrameType(pData);
    if(idx>eMpeg4_BFrame)
        idx=eMpeg4_errFrame;
    return arr[idx];
}

// 判断当前MPEG4视频帧是否是I帧
eMpeg4FrameType CMP4Base::GetMpeg4FrameType(LPBYTE pData)
{
    /*
    vop_coding_type   is   encoded   in   the   first   2   bits   after   vop_start_code.
    vop_start_code   is   00   00   01   b6   (hex)

            vop_coding_type   means:
            0   0     I      VOP
            0   1     P      VOP
            1   0     B      VOP
            1   1     S      VOP

        */
    UINT nHead = *((UINT *)pData);
    if(nHead == 0xb3010000 || nHead == 0xb0010000) {
        return eMpeg4_IFrame;
    } else if(nHead == 0xb6010000) {
        unsigned char vt_byte = pData[4];

        vt_byte &= 0xC0;
        vt_byte  = vt_byte >> 6;

        switch(vt_byte) {
            case 0:                 // I Frame
                return eMpeg4_IFrame;
                break;
            case 1:
                return eMpeg4_PFrame;       // P Frame
                break;
            case 2:
                return eMpeg4_BFrame;       // B Frame
                break;
            default:
                break;
        }
    }

    return eMpeg4_errFrame;             // Unkown Frame
}

static char base64DecodeTable[256];

static void initBase64DecodeTable()
{
    int i;
    for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
    // default value: invalid

    for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
    for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
    for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
    base64DecodeTable[(unsigned char)'+'] = 62;
    base64DecodeTable[(unsigned char)'/'] = 63;
    base64DecodeTable[(unsigned char)'='] = 0;
}

unsigned char* CMP4Base::base64Decode(char* in, unsigned& resultSize,
                                      Boolean trimTrailingZeros)
{
    static Boolean haveInitedBase64DecodeTable = FALSE;
    if (!haveInitedBase64DecodeTable) {
        initBase64DecodeTable();
        haveInitedBase64DecodeTable = TRUE;
    }

#ifdef _MSC_VER
    unsigned char* out = (unsigned char*)_strdup(in); // ensures we have enough space
#else
    unsigned char* out = (unsigned char*)strdup(in); // ensures we have enough space
#endif

    int k = 0;
    int const jMax = strlen(in) - 3;
    // in case "in" is not a multiple of 4 bytes (although it should be)
    for (int j = 0; j < jMax; j += 4) {
        char inTmp[4], outTmp[4];
        for (int i = 0; i < 4; ++i) {
            inTmp[i] = in[i+j];
            outTmp[i] = base64DecodeTable[(unsigned char)inTmp[i]];
            if ((outTmp[i]&0x80) != 0) outTmp[i] = 0; // pretend the input was 'A'
        }

        out[k++] = (outTmp[0]<<2) | (outTmp[1]>>4);
        out[k++] = (outTmp[1]<<4) | (outTmp[2]>>2);
        out[k++] = (outTmp[2]<<6) | outTmp[3];
    }

    if (trimTrailingZeros) {
        while (k > 0 && out[k-1] == '\0') --k;
    }
    resultSize = k;
    unsigned char* result = new unsigned char[resultSize];
    memmove(result, out, resultSize);
    delete[] out;
    
    return result;
}

static const char base64Char[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* CMP4Base::base64Encode(char const* origSigned, unsigned origLength)
{
    unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
    if (orig == NULL) return NULL;

    unsigned const numOrig24BitValues = origLength/3;
    Boolean havePadding = origLength > numOrig24BitValues*3;
    Boolean havePadding2 = origLength == numOrig24BitValues*3 + 2;
    unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding);
    char* result = new char[numResultBytes+1]; // allow for trailing '\0'

    // Map each full group of 3 input bytes into 4 output base-64 characters:
    unsigned i;
    for (i = 0; i < numOrig24BitValues; ++i) {
        result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
        result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
        result[4*i+2] = base64Char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];
        result[4*i+3] = base64Char[orig[3*i+2]&0x3F];
    }

    // Now, take padding into account.  (Note: i == numOrig24BitValues)
    if (havePadding) {
        result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];
        if (havePadding2) {
            result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
            result[4*i+2] = base64Char[(orig[3*i+1]<<2)&0x3F];
        } else {
            result[4*i+1] = base64Char[((orig[3*i]&0x3)<<4)&0x3F];
            result[4*i+2] = '=';
        }
        result[4*i+3] = '=';
    }

    result[numResultBytes] = '\0';
    return result;
}

#ifndef _MSC_VER
void DebugTrace(LPCTSTR lpszFormat, ... )
{
    char szBuf[1024];//should be enough
    szBuf[0]=0;

    va_list argList;
    va_start(argList, lpszFormat);
    vsprintf(szBuf,lpszFormat, argList);
    va_end(argList);

    printf(szBuf);
    printf("\n");
}
#endif

CMP4Trak::CMP4Trak()
{
    m_pMP4Write = NULL;
    memset(m_st_size,0,sizeof(m_st_size));
    memset(m_st_count,0,sizeof(m_st_count));
    memset(m_pre_st_count,0,sizeof(m_st_count));
    
    m_stts_addr = 0;
    m_mdhd_addr = 0;
    m_tkhd_addr = 0;
    m_trakid=0;
    m_timescale=1;
    m_duration=0;
    memset(m_handlerType,0,sizeof(m_handlerType));
    m_szVideo=CSize(0,0);
    m_dwFrameIndex=0;
    m_nFirstChunk=0;

    m_dwTickRTPLast=0;
    m_type=0;
    m_stsz_count=0;
    memset(m_fmtp_spropparametersets,0,sizeof(m_fmtp_spropparametersets));
    //m_fmtp_spropparametersets=NULL;
    m_bH264IFrame=FALSE;
    m_amr_ft=0;
    m_dwRTPTimeStamp = 0;
    m_bH265 = FALSE;
    m_bSavedIFrame = FALSE;
}

CMP4Trak::~CMP4Trak()
{
    Empty();
}

void CMP4Trak::Empty()
{
    {
        for(vector<tagMP4Node_stts*>::iterator it=m_lst_stts.begin(); it!=m_lst_stts.end(); it++) {
            tagMP4Node_stts* ps = *it;
            if (ps != NULL) {
                delete ps;
                *it  = NULL;
            }
        }
        //m_lst_stts.clear();
    }

    {
        for(vector<tagMP4Node_stsc*>::iterator it=m_lst_stsc.begin(); it!=m_lst_stsc.end(); it++) {
            tagMP4Node_stsc* ps = *it;
            if (ps != NULL) {
                delete ps;
                *it  = NULL;
            }
        }

        //m_lst_stsc.clear();
    }

    if(m_fmtp_spropparametersets) {
        //free(m_fmtp_spropparametersets);
        //m_fmtp_spropparametersets=NULL;
        memset(m_fmtp_spropparametersets, 0, sizeof(m_fmtp_spropparametersets));
    }

    m_amr_ft=0;
    return ;
}

int CMP4Trak::InitAudioTrakInfo(DWORD32 type,const char *handlerType,DWORD32 timescale)
{
    m_type=type;
    ASSERT(m_pMP4Write);
    ASSERT(strlen(handlerType)==sizeof(m_handlerType));

    //采用amr的rtp payload type来设置audio track id,即使真正的audio是ulaw,这也不会引起问题
    //.mp4标准要求每个trak的id不同即可,除此之外,track id没有其他含义
    //ulaw的payload type为0,这是无效的trak id
    m_trakid = RTP_TYPE_AMR;
    ASSERT( (m_trakid & 0xFF) == m_trakid);//由于支持MP4_FLAGS_JCOK,所以要限制trakid为一个字节
    memcpy(m_handlerType,handlerType,sizeof(m_handlerType));;
    m_timescale=timescale;
    m_hFile=m_pMP4Write->m_hFile;

    return 0;
}

int CMP4Trak::InitVideoTrakInfo(DWORD32 type,const char *handlerType,CSize szVideo,const char *sprop_parameter_sets,DWORD32 timescale)
{
    if(szVideo==CSize(0,0)) {
        szVideo=CSize(720,576);
    }

    ASSERT(m_pMP4Write);
    ASSERT(type);
    ASSERT(strlen(handlerType)==sizeof(m_handlerType));
    m_type=type;
    m_trakid=type;//use rtp payload for .mp4 trak id
    ASSERT( (m_trakid & 0xFF) == m_trakid);//由于支持MP4_FLAGS_JCOK,所以要限制trakid为一个字节
    memcpy(m_handlerType,handlerType,sizeof(m_handlerType));
    m_szVideo = szVideo;
    m_timescale=timescale;
    m_hFile=m_pMP4Write->m_hFile;

    //ASSERT(!m_fmtp_spropparametersets);
    if(sprop_parameter_sets && strlen(sprop_parameter_sets)>0) {
        char tmp[1024] = {0};
        strcpy(tmp, sprop_parameter_sets);
        char * p = (char *)strstr(tmp, ";HEVC");//采用了一个取巧的标记，在sprop_parameter_sets后面加上 ";HEVC"时代表h.265
        if (p) {
            m_bH265 = TRUE;
            *p = 0x00;
        }

        memcpy(m_fmtp_spropparametersets, tmp, sizeof(m_fmtp_spropparametersets) - 1);
        //m_fmtp_spropparametersets=_strdup(tmp);
//      m_timescale=600;//h.264
        DT("m_fmtp_spropparametersets=[%s]",m_fmtp_spropparametersets);
    }

    return 0;
}

long CMP4Trak::addAtom_trak()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("trak");

    size += addAtom_tkhd();
    size += addAtom_mdia();

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_tkhd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("tkhd");

    struct timeval      m_fStartTime;
    gettimeofday(&m_fStartTime);
    unsigned   m_fAppleCreationTime = m_fStartTime.tv_sec - 0x83dac000;

    //audio为ulaw时,video trak和audio trak的Flags都要为0x0F.否则不兼容quicktimeplayer
    //xwpcom 2009.11.06
    DWORD32 dwVerFlags=(m_pMP4Write->m_dwFlags & MP4F_ULAW)?0x0000000F:0x00000001;
    size += addWord(dwVerFlags); // Version +  Flags(EnableTrack)

    size += addWord(m_fAppleCreationTime); // Creation time
    size += addWord(m_fAppleCreationTime); // Modification time

    m_tkhd_addr = ftell(m_hFile);
    size += addWord(m_trakid); // Track ID
    size += addWord(0x00000000); // Reserved

    //注意要先除m_timescale,再*600,否则录像时间稍长,m_duration * 600会大于0xFFFFFFFF引起越界错误.
    //DWORD32表示的duration最大支持13小时的单个录像时长(0xFFFFFFFF/90000).
    size += addWord((m_duration/m_timescale)*600); // Duration.
    size += addZeroWords(3); // Reserved+Layer+Alternate grp
    size += addWord(IsVideoTrak()?0x0:0x01000000); // Volume + Reserved
    size += addWord(0x00010000); // matrix top left corner
    size += addZeroWords(3); // matrix
    size += addWord(0x00010000); // matrix center
    size += addZeroWords(3); // matrix
    size += addWord(0x40000000); // matrix bottom right corner

    if (IsVideoTrak()) {
        size += addWord(m_szVideo.cx<<16); // Track width
        size += addWord(m_szVideo.cy<<16); // Track height
    } else {
        size += addZeroWords(2); // not video: leave width and height fields zero
    }
    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_mdia()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("mdia");

    size += addAtom_mdhd();
    size += addAtom_hdlr();
    size += addAtom_minf();

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_mdhd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("mdhd");
    size += addWord(0x00000000); // Version+flags

    struct timeval fStartTime;
    gettimeofday(&fStartTime);
    unsigned fCreationTime = fStartTime.tv_sec - 0x83dac000;

    size += addWord(fCreationTime); // create time
    size += addWord(fCreationTime); // modify time

    m_mdhd_addr = ftell(m_hFile);
    size += addWord(m_timescale);        // timescale
    size += addWord(m_duration);         // duration
    size += addHalfWord(0x0000); //language
    size += addHalfWord(0x0000); //reserved

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_hdlr()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("hdlr");
    size += addWord(0x00000000); // Version+flags
    if(m_type==0) {
        //ulaw
        size += add4ByteString("mhlr");
    } else {
        size += addWord(0x00000000); // reserved
    }
    size += add4ByteString((const char*)m_handlerType);//video,soun,...

    {
        if(m_type==0) {
            //ulaw
            size += add4ByteString("appl");
            for(int i=0; i<8; i++) {
                size += addByte(0x00);
            }
        } else {
            //12 bytes 0x00,reserved
            for(int i=0; i<12; i++) {
                size += addByte(0x00);
            }
        }
    }

    if(m_type==0) {
        const char *name="Apple Sound Media Handler";
        size += addArbitraryString(name,TRUE);
    } else {
        size += addByte(0x00);//null name
    }

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_minf()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("minf");

    if(IsVideoTrak()) {
        size +=addAtom_vmhd();
    } else {
        size +=addAtom_smhd();
    }

    size +=addAtom_dinf();
    size +=addAtom_stbl_stsd();

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_vmhd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("vmhd");
    size += addWord(0x00000001); // Version+flags

    if(IsH264()) {
        size += addWord(0x00408000); // Graphics mode + Opcolor[red]
        size += addWord(0x80008000); // Opcolor[green} + Opcolor[blue]
    } else {
        //8 bytes 0x00,reserved
        for(int i=0; i<8; i++) {
            size += addByte(0x00);
        }
    }

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_smhd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("smhd");
    size += addWord(0x00000000); // Version+flags
    size += addWord(0x00000000);
    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_dinf()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("dinf");
    size += addAtom_dref();

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_dref()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("dref");
    size += addWord(0x00000000); // Version+flags
    size += addWord(0x00000001); // entries
    {
        //url null
        size += addWord(0x0000000C);

        size+=add4ByteString("url ");
        size+=addWord(0x00000001);
    }

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_stts()
{
    //int index = 0;
    unsigned size = 0;
    long initFilePosn = ftell(m_hFile);

    size = addWord(PRE_STTS_SIZE); // 固定大小
    size += add4ByteString("stts");
    size += addWord(0x00000000); // Version+flags
    size += addWord(m_st_count[eAtomStts]);

    if (m_st_size[eAtomStts] != 0) {
        size = 0;    
        fseek(m_hFile, initFilePosn + m_st_size[eAtomStts], SEEK_SET);
    }

    if (IsAudioTrak()) {
        //index = 1;
    }

    for (DWORD32 i = m_pre_st_count[eAtomStts]; i < m_st_count[eAtomStts]; i++) {
        tagMP4Node_stts* ps = m_lst_stts[i];
        size += addWord(ps->SampleCount);
        size += addWord(ps->SampleDuration>0?ps->SampleDuration:1);
    }
    m_pre_st_count[eAtomStts] = m_st_count[eAtomStts];

    m_st_size[eAtomStts] += size;
    size = PRE_STTS_SIZE;
    
    return size;
}

long CMP4Trak::addAtom_stss()
{
    if(!IsVideoTrak() || IsMjpg()) {
        return 0;
    }

    unsigned size = 0;
    long initFilePosn = ftell(m_hFile);
    
    size = addWord(PRE_STSS_SIZE); // 固定大小
    size += add4ByteString("stss");
    size += addWord(0x00000000); // Version+flags
    size += addWord(m_st_count[eAtomStss]);
    
    if (m_st_size[eAtomStss] != 0) {
        size = 0;    
        fseek(m_hFile, initFilePosn + m_st_size[eAtomStss], SEEK_SET);
    }

    for (DWORD32 i = m_pre_st_count[eAtomStss]; i < m_st_count[eAtomStss]; i++) {
        DWORD32 idx = m_lst_stss[i];
        size += addWord(idx);
    }
    m_pre_st_count[eAtomStss] = m_st_count[eAtomStss];

    m_st_size[eAtomStss] += size;
    size = PRE_STSS_SIZE;
    
    return size;
}

long CMP4Trak::addAtom_stsc()
{
    //int index = 0;
    unsigned size = 0;
    long initFilePosn = ftell(m_hFile);

    size = addWord(PRE_STSC_SIZE); // 固定大小
    size += add4ByteString("stsc");
    size += addWord(0x00000000); // Version+flags
    size += addWord(m_st_count[eAtomStsc]);

    if (m_st_size[eAtomStsc] != 0) {
        size = 0;
        fseek(m_hFile, initFilePosn + m_st_size[eAtomStsc], SEEK_SET);
    }

    if (IsAudioTrak()) {
        //index = 1;
    }

    for (DWORD32 i = m_pre_st_count[eAtomStsc]; i < m_st_count[eAtomStsc]; i++) {
        tagMP4Node_stsc* ps = m_lst_stsc[i];
        size += addWord(ps->FirstChunk);
        size += addWord(ps->SamplesPerChunk);
        size += addWord(ps->SampleDescriptionID);
    }
    m_pre_st_count[eAtomStsc] = m_st_count[eAtomStsc];

    m_st_size[eAtomStsc] += size;
    size = PRE_STSC_SIZE;
    
    return size;
}

long CMP4Trak::addAtom_stsz()
{
    //int index = 0;
    unsigned size = 0;   
    long initFilePosn = ftell(m_hFile);

    size = addWord(PRE_STSZ_SIZE); // 固定大小
    size += add4ByteString("stsz");
    size += addWord(0x00000000); // Version+flags
    
    //判断所有sample是不是具有相同大小
    BOOL32 bHasDifferentSize = FALSE;
    DWORD32 dwSize = 0;
    {
        for(vector<DWORD32>::iterator it=m_lst_stsz.begin(); it!=m_lst_stsz.end(); it++) {
            DWORD32 sz = *it;
            if(dwSize && sz != dwSize) {
                bHasDifferentSize=TRUE;
                break;
            }

            dwSize = sz;
        }
    }

    //write 0 if the samples have different sizes
    //write single size if all the samples are the same size
    size += addWord(bHasDifferentSize?0x00000000:dwSize);

    size += addWord(IsAudioTrak()?m_stsz_count:m_st_count[eAtomStsz]);
    
    if (m_st_size[eAtomStsz] != 0) {
        size = 0;        
        fseek(m_hFile, initFilePosn + m_st_size[eAtomStsz], SEEK_SET);
    }

    if (IsAudioTrak()) {
        //index = 1;
    }

    for (DWORD32 i = m_pre_st_count[eAtomStsz]; i < m_st_count[eAtomStsz]; i++) {
        DWORD32 sz = m_lst_stsz[i];
        size += addWord(sz);
    }
    m_pre_st_count[eAtomStsz] = m_st_count[eAtomStsz];
    
    m_st_size[eAtomStsz] += size;
    size = PRE_STSZ_SIZE;
    
    return size;
}

long CMP4Trak::addAtom_stco()
{
    //int index = 0;
    unsigned size = 0;
    long initFilePosn = ftell(m_hFile);

    size = addWord(PRE_STCO_SIZE); // 固定大小
    size += add4ByteString("stco");
    size += addWord(0x00000000); // Version+flags
    size += addWord(m_st_count[eAtomStco]);
    
    if (m_st_size[eAtomStco] != 0) {
        size = 0;        
        fseek(m_hFile, initFilePosn + m_st_size[eAtomStco], SEEK_SET);
    }

    if (IsAudioTrak()) {
        //index = 1;
    }

    for (DWORD32 i = m_pre_st_count[eAtomStco]; i < m_st_count[eAtomStco]; i++) {
        DWORD32 sz = m_lst_stco[i];
        size += addWord(sz);
    }
    m_pre_st_count[eAtomStco] = m_st_count[eAtomStco];

    m_st_size[eAtomStco] += size;
    size = PRE_STCO_SIZE;

    return size;
}

long CMP4Trak::addAtom_stsd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("stsd");
    size += addWord(0x00000000); // Version+flags
    size += addWord(0x00000001); // entries
    if(IsVideoTrak()) {
        if(IsH264()) {
            size += addAtom_avc1();//h.264
        } else if(IsH265()) {
            size += addAtom_hvc1();//h.265
        } else if(IsMjpg()) {
            size += addAtom_mjpa();//mjpg-a
        } else {
            size += addAtom_mp4v();//mpeg4
        }
    } else {
        if(m_type == 0x00) {
            //ulaw
            size += addAtom_ulaw();
        } else if(m_type == RTP_TYPE_ALAW) {
            //alaw
            size += addAtom_ulaw();
        } else if (m_type == RTP_TYPE_AAC) {
            size += addAtom_aac();
        } else {
            size += addAtom_samr();
        }
    }

    setWord(initFilePosn, size);
    return size;
}

//AudioSampleEntry(ISO_IEC_14496_12.pdf)
long CMP4Trak::addAtom_samr()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("samr");
    for (int i=0; i<6; i++) {
        size += addByte(0);
    }

    size += addHalfWord(0x0001);//data-reference-index

    size += addWord(0x00000000);
    size += addWord(0x00000000);

    size += addHalfWord(0x0002);//reserved
    size += addHalfWord(0x0010);//reserved
    size += addWord(0x00000000);

    size += addHalfWord(8000);//timescale,这里为16bit
    size += addHalfWord(0000);

    //AMRSpecifiBox
    size += addWord(0x00000011);
    size += add4ByteString("damr");
    size += add4ByteString("JCO ");//vendor,can be other chars
    size += addByte(0x01);//decoder version
    WORD mode_set=0x0080;//0x0080;//0x81FF
    size += addHalfWord(mode_set);//mode_set
    size += addByte(0x00);//mode_change_period
    size += addByte(0x01);//frames_per_sample
    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_ulaw()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader(m_type == RTP_TYPE_ULAW ? "ulaw" : "alaw");

    // General sample description fields:
    size += addWord(0x00000000); // Reserved
    size += addWord(0x00000001); // Reserved+Data reference index

    // Sound sample description fields:
    unsigned short const version = 0;
    size += addWord(version<<16); // Version+Revision level
    size += addWord(0x00000000); // Vendor
    unsigned short numChannels=1;
    size += addHalfWord(numChannels); // Number of channels
    size += addHalfWord(0x0010); // Sample size
    size += addWord(0xfffe0000); // Compression ID+Packet size #####

    ASSERT(m_timescale==8000);

    DWORD32 timescale=m_timescale;
    DWORD32 sampleRateFixedPoint = timescale<<16;
    size += addWord(sampleRateFixedPoint); // Sample rate
    setWord(initFilePosn, size);
    return size;
}

unsigned CMP4Trak::addAtom_aac_esds()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("esds");

    size += addWord(0x00000000); /* version & flags */

    // write es_descriptor
    size += addByte(0x03); // 固定头
    size += addByte(0x19);  // size = 0x03 + DecoderConfigDescriptor(0x11 + 0x02) + SLConfigDescrTag(0x01 + 0x02)
    size += addHalfWord((unsigned short)m_trakid);  // track id
    size += addByte(0x00);

    // DecoderConfigDescriptor
    size += addByte(0x04);  // 固定头 0x04
    size += addByte(0x11); // size = 0x0d + DecSpecificInfotag(0x02 + 0x02)
    size += addByte(0x40); // 音频编码类型，根据 ISO/IEC 14496-3 AAC=0x40
    size += addByte( 0x01/*reserved*/ | (0x5 << 2)); // 流类型，根据 ISO/IEC 14496-1:2010 音频=0x5

    // 3 字节，建议解码器缓存大小
    size += addByte(0x00);
    size += addHalfWord(0x01f8);

    size += addWord(88360); // 音频数据最大码率
    size += addWord(88360); // 音频数据平均码率

    // DecSpecificInfotag
    size += addByte(0x05);  // 固定头 0x05
    size += addByte(0x02); // size = 2
    size += addHalfWord(0x1408); // 描述 aac 编码信息，这里配置的是 AAC LC，16K 单声道

    // SLConfigDescrTag 这个和 DecoderConfigDescriptor 是平行关系
    size += addByte(0x06);  // 固定头 0x06
    size += addByte(0x01);  // size = 1
    size += addByte(0x02);  // 02 reserved

    setWord(initFilePosn, size);

    return size;
}


long CMP4Trak::addAtom_aac()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("mp4a");

    // General sample description fields:
    size += addWord(0x00000000); // Reserved
    size += addWord(0x00000001); // Reserved+Data reference index

    // Sound sample description fields:
    unsigned short const version = 0;
    size += addWord(version<<16); // Version+Revision level
    size += addWord(0x00000000); // Vendor
    
    unsigned short numChannels=1;
    size += addHalfWord(numChannels); // Number of channels
    size += addHalfWord(0x0010); // Sample size
    
    size += addHalfWord(0x0000); // pre_defined
    size += addHalfWord(0x0000); // reserved / packet size (= 0)

    ASSERT(m_timescale==16000);

    DWORD32 timescale=m_timescale;
    DWORD32 sampleRateFixedPoint = timescale<<16;
    size += addWord(sampleRateFixedPoint); // Sample rate

    size += addAtom_aac_esds();
        
    setWord(initFilePosn, size);
    return size;
}


long CMP4Trak::addAtom_avc1()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("avc1");

    // General sample description fields:
    size += addWord(0x00000000); // Reserved
    size += addWord(0x00000001); // Reserved+Data       reference index
    // Video sample       description     fields:
    size += addWord(0x00000000); // Version+Revision level
    size += add4ByteString("appl"); // Vendor
    size += addWord(0x00000000); // Temporal quality
    size += addWord(0x00000000); // Spatial quality
    unsigned const widthAndHeight       = (m_szVideo.cx<<16)|m_szVideo.cy;
    size += addWord(widthAndHeight); // Width+height
    size += addWord(0x00480000); // Horizontal resolution
    size += addWord(0x00480000); // Vertical resolution
    size += addWord(0x00000000); // Data size
    size += addWord(0x00010548); // Frame       count+Compressor name (start)
    // "H.264"
    size += addWord(0x2e323634); // Compressor name (continued)
    size += addZeroWords(6); // Compressor name (continued - zero)
    size += addWord(0x00000018); // Compressor name (final)+Depth
    size += addHalfWord(0xffff); // Color       table id
    size += addAtom_avcC();

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_avcC()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("avcC");

    // Begin by Base-64 decoding the "sprop" parameter sets strings:
    //ASSERT(m_fmtp_spropparametersets);
    char* psets = _strdup(m_fmtp_spropparametersets);
    if (psets == NULL) return 0;

    size_t comma_pos = strcspn(psets, ",");
    psets[comma_pos] = '\0';
    char* sps_b64 = psets;
    char* pps_b64 = &psets[comma_pos+1];
    unsigned sps_count;
    unsigned char* sps_data = base64Decode(sps_b64, sps_count, false);
    unsigned pps_count;
    unsigned char* pps_data = base64Decode(pps_b64, pps_count, false);

    // Then add the decoded data:
    size += addByte(0x01); // configuration version
    size += addByte(sps_data[1]); // profile
    size += addByte(sps_data[2]); // profile compat
    size += addByte(sps_data[3]); // level
    size += addByte(0xff); /* 0b11111100 | lengthsize = 0x11 */
    size += addByte(0xe0 | (sps_count > 0 ? 1 : 0) );
    if (sps_count > 0) {
        size += addHalfWord(sps_count);
        for (unsigned i = 0; i < sps_count; i++) {
            size += addByte(sps_data[i]);
        }
    }
    size += addByte(pps_count > 0 ? 1 : 0);
    if (pps_count > 0) {
        size += addHalfWord(pps_count);
        for (unsigned i = 0; i < pps_count; i++) {
            size += addByte(pps_data[i]);
        }
    }

    // Finally, delete the data that we allocated:
    delete[] pps_data;
    pps_data=NULL;
    delete[] sps_data;
    sps_data=NULL;
    delete[] psets;
    psets=NULL;

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_hvc1()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("hvc1" );
    // General sample description fields:
    size += addWord(0x00000000); // Reserved
    size += addWord(0x00000001); // Reserved+Data       reference index
    // Video sample       description     fields:
    size += addWord(0x00000000); // Version+Revision level
    size += add4ByteString("appl"); // Vendor
    size += addWord(0x00000000); // Temporal quality
    size += addWord(0x00000000); // Spatial quality
    unsigned const widthAndHeight = (m_szVideo.cx << 16) | m_szVideo.cy;
    size += addWord(widthAndHeight); // Width+height
    size += addWord(0x00480000); // Horizontal resolution
    size += addWord(0x00480000); // Vertical resolution
    size += addWord(0x00000000); // Data size
    size += addWord(0x00010548); // Frame       count+Compressor name (start)
    // "H.265"
    size += addWord(0x2e323635); // Compressor name (continued)
    size += addZeroWords(6); // Compressor name (continued - zero)
    size += addWord(0x00000018); // Compressor name (final)+Depth
    size += addHalfWord(0xffff); // Color       table id
    size += addAtom_hvcC();

    setWord(initFilePosn, size);
    return size;
}

typedef struct HVCCNALUnitArray {
    unsigned char array_completeness;
    unsigned char  NAL_unit_type;
    unsigned short numNalus;
    unsigned short *nalUnitLength;
    unsigned char **nalUnit;
} HVCCNALUnitArray;

typedef struct HEVCDecoderConfigurationRecord {
    unsigned char configurationVersion;
    unsigned char general_profile_space;
    unsigned char general_tier_flag;
    unsigned char general_profile_idc;
    unsigned long general_profile_compatibility_flags;
    uint64_t general_constraint_indicator_flags;
    unsigned char general_level_idc;
    unsigned short min_spatial_segmentation_idc;
    unsigned char parallelismType;
    unsigned char chromaFormat;
    unsigned char bitDepthLumaMinus8;
    unsigned char bitDepthChromaMinus8;
    unsigned short avgFrameRate;
    unsigned char constantFrameRate;
    unsigned char numTemporalLayers;
    unsigned char temporalIdNested;
    unsigned char lengthSizeMinusOne;
    unsigned char numOfArrays;
    HVCCNALUnitArray *array;
} HEVCDecoderConfigurationRecord;

#define MAX_SPATIAL_SEGMENTATION 4096

static void hvcc_init(HEVCDecoderConfigurationRecord *hvcc)
{
    memset(hvcc, 0, sizeof(HEVCDecoderConfigurationRecord));
    hvcc->configurationVersion = 1;
    hvcc->lengthSizeMinusOne = 3; // 4 bytes
    /*
    * The following fields have all their valid bits set by default,
    * the ProfileTierLevel parsing code will unset them when needed.
    */
    hvcc->general_profile_compatibility_flags = 0xffffffff;
    hvcc->general_constraint_indicator_flags = 0xffffffffffff;
    /*
    * Initialize this field with an invalid value which can be used to detect
    * whether we didn't see any VUI (in which case it should be reset to zero).
    */
    hvcc->min_spatial_segmentation_idc = MAX_SPATIAL_SEGMENTATION + 1;
}

enum NALUnitType {
    NAL_TRAIL_N=0,
    NAL_TRAIL_R=1,
    NAL_TSA_N=2,
    NAL_TSA_R=3,
    NAL_STSA_N=4,
    NAL_STSA_R=5,
    NAL_RADL_N=6,
    NAL_RADL_R=7,
    NAL_RASL_N=8,
    NAL_RASL_R=9,
    NAL_BLA_W_LP=16,
    NAL_BLA_W_RADL=17,
    NAL_BLA_N_LP=18,
    NAL_IDR_W_RADL=19,
    NAL_IDR_N_LP=20,
    NAL_CRA_NUT=21,
    NAL_VPS=32,
    NAL_SPS=33,
    NAL_PPS=34,
    NAL_AUD=35,
    NAL_EOS_NUT=36,
    NAL_EOB_NUT=37,
    NAL_FD_NUT=38,
    NAL_SEI_PREFIX=39,
    NAL_SEI_SUFFIX=40,
};

long CMP4Trak::addAtom_hvcC()
{
    //参考ISO/IEC 14496-15文档中的 LHEVCDecoderConfigurationRecord 
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("hvcC" );

    HEVCDecoderConfigurationRecord atom;
    hvcc_init(&atom);
    HEVCDecoderConfigurationRecord *hvcc = &atom;

    /*
    * We only support writing HEVCDecoderConfigurationRecord version 1.
    */
    hvcc->configurationVersion = 1;

    /*
    * If min_spatial_segmentation_idc is invalid, reset to 0 (unspecified).
    */
    if (hvcc->min_spatial_segmentation_idc > MAX_SPATIAL_SEGMENTATION)
        hvcc->min_spatial_segmentation_idc = 0;

    /*
    * parallelismType indicates the type of parallelism that is used to meet
    * the restrictions imposed by min_spatial_segmentation_idc when the value
    * of min_spatial_segmentation_idc is greater than 0.
    */
    if (!hvcc->min_spatial_segmentation_idc)
        hvcc->parallelismType = 0;

    /*
    * It's unclear how to properly compute these fields, so
    * let's always set them to values meaning 'unspecified'.
    */
    hvcc->avgFrameRate = 0;
    hvcc->constantFrameRate = 0;

    /* unsigned int(8) configurationVersion = 1; */
    size += addByte(hvcc->configurationVersion);

    /*
    * unsigned int(2) general_profile_space;
    * unsigned int(1) general_tier_flag;
    * unsigned int(5) general_profile_idc;
    */
    size += addByte(hvcc->general_profile_space << 6 |
        hvcc->general_tier_flag << 5 |
        hvcc->general_profile_idc);

    /* unsigned int(32) general_profile_compatibility_flags; */
    size += addWord(hvcc->general_profile_compatibility_flags);

    /* unsigned int(48) general_constraint_indicator_flags; */
    size += addWord(hvcc->general_constraint_indicator_flags >> 16);
    size += addHalfWord(hvcc->general_constraint_indicator_flags);

    /* unsigned int(8) general_level_idc; */
    size += addByte(hvcc->general_level_idc);

    /*
    * bit(4) reserved = 鈥?111鈥檅;
    * unsigned int(12) min_spatial_segmentation_idc;
    */
    size += addHalfWord(hvcc->min_spatial_segmentation_idc | 0xf000);

    /*
    * bit(6) reserved = 鈥?11111鈥檅;
    * unsigned int(2) parallelismType;
    */
    size += addByte(hvcc->parallelismType | 0xfc);

    /*
    * bit(6) reserved = 鈥?11111鈥檅;
    * unsigned int(2) chromaFormat;
    */
    size += addByte(hvcc->chromaFormat | 0xfc);

    /*
    * bit(5) reserved = 鈥?1111鈥檅;
    * unsigned int(3) bitDepthLumaMinus8;
    */
    size += addByte(hvcc->bitDepthLumaMinus8 | 0xf8);

    /*
    * bit(5) reserved = 鈥?1111鈥檅;
    * unsigned int(3) bitDepthChromaMinus8;
    */
    size += addByte(hvcc->bitDepthChromaMinus8 | 0xf8);

    /* bit(16) avgFrameRate; */
    size += addHalfWord(hvcc->avgFrameRate);

    /*
    * bit(2) constantFrameRate;
    * bit(3) numTemporalLayers;
    * bit(1) temporalIdNested;
    * unsigned int(2) lengthSizeMinusOne;
    */
    size += addByte(hvcc->constantFrameRate << 6 |
        hvcc->numTemporalLayers << 3 |
        hvcc->temporalIdNested << 2 |
        hvcc->lengthSizeMinusOne);


    /* Add vps，sps，pps */
    size += addByte(3);	 //vps，sps，pps共3个

    //ASSERT(m_fmtp_spropparametersets);
    char* psets = _strdup(m_fmtp_spropparametersets);
    if (psets == NULL) return 0;

    size_t comma_pos = strcspn(psets, ",");
    psets[comma_pos] = '\0';
    char* vps_b64 = psets;
    char* sps_b64 = &psets[comma_pos + 1];

    size_t comma_pos1 = strcspn(sps_b64, ",");
    sps_b64[comma_pos1] = '\0';
    char* pps_b64 = &sps_b64[comma_pos1 + 1];

    unsigned vps_count;
    unsigned char* vps_data = base64Decode(vps_b64, vps_count, false);
    unsigned sps_count;
    unsigned char* sps_data = base64Decode(sps_b64, sps_count, false);
    unsigned pps_count;
    unsigned char* pps_data = base64Decode(pps_b64, pps_count, false);

    // Then add the decoded data:
    ASSERT(sps_count > 0 && pps_count > 0 && vps_count > 0);
    size += addByte(0xA0); // vps;
    size += addHalfWord(0x01); // numNalus;
    size += addHalfWord(vps_count);
    unsigned i = 0;
    for (i = 0; i < vps_count; i++) {
        size += addByte(vps_data[i]);
    }
    size += addByte(0xA1); // sps;
    size += addHalfWord(0x01); // numNalus;
    size += addHalfWord(sps_count);
    for (i = 0; i < sps_count; i++) {
        size += addByte(sps_data[i]);
    }
    size += addByte(0xA2); // pps;
    size += addHalfWord(0x01); // numNalus;
    size += addHalfWord(pps_count);
    for (i = 0; i < pps_count; i++) {
        size += addByte(pps_data[i]);
    }

    // Finally, delete the data that we allocated:
    delete[] vps_data;
    vps_data = NULL;
    delete[] pps_data;
    pps_data = NULL;
    delete[] sps_data;
    sps_data = NULL;
    delete[] psets;
    psets = NULL;

    setWord(initFilePosn, size);
    return size;
}

//VisualSampleEntry(ISO_IEC_14496_12.pdf)
long CMP4Trak::addAtom_mp4v()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("mp4v");
    for (int i=0; i<6; i++) {
        size += addByte(0);
    }

    size += addHalfWord(0x0001);//data-reference-index.todo

    size += addHalfWord(0x0000);//pre_defined
    size += addHalfWord(0x0000);//reserved

    size += addWord(0x00000000);//reserved
    size += addWord(0x00000000);//reserved
    size += addWord(0x00000000);//reserved

    //width,height
    size += addHalfWord((WORD)m_szVideo.cx);
    size += addHalfWord((WORD)m_szVideo.cy);

    size += addWord(0x00480000);//horizresolution = 0x00480000; // 72 dpi
    size += addWord(0x00480000);//vertresolution = 0x00480000; // 72 dpi

    size += addWord(0x00000000);//reserved
    size += addHalfWord(0x0001);//frame_count=1

    //string[32] compressorname
    {
        for(int i=0; i<32; i++) {
            size += addByte(0x00);
        }
    }

    size += addHalfWord(0x0018);//unsigned int(16) depth = 0x0018;
    size += addHalfWord(0xFFFF);//int(16) pre_defined = -1;

    size += addAtom_esds();

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_mjpa()
{
    unsigned initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("mjpa");
    for (int i=0; i<6; i++) {
        size += addByte(0);
    }

    size += addHalfWord(0x0001);//data-reference-index.todo

    size += addHalfWord(0x0000);//pre_defined
    size += addHalfWord(0x0000);//reserved

    size += addWord(0x00000000);//reserved
    size += addWord(0x00000000);//reserved
    size += addWord(0x00000000);//reserved

    //width,height
    size += addHalfWord((WORD)m_szVideo.cx);
    size += addHalfWord((WORD)m_szVideo.cy);

    size += addWord(0x00480000);//horizresolution = 0x00480000; // 72 dpi
    size += addWord(0x00480000);//vertresolution = 0x00480000; // 72 dpi

    size += addWord(0x00000000);//reserved
    size += addHalfWord(0x0001);//frame_count=1

    //string[32] compressorname
    {
        for(int i=0; i<32; i++) {
            size += addByte(0x00);
        }
    }

    size += addHalfWord(0x0018);//unsigned int(16) depth = 0x0018;
    size += addHalfWord(0xFFFF);//int(16) pre_defined = -1;

    {
        BYTE arr[]= {
            0x00,0x00,0x00,0x0A,'f','i','e','l',0x02,0x01,
            0x00,0x00,0x00,0x0C,'g','a','m','a',0x00,0x01,0xCC,0xCC,
            0x00,0x00,0x00,0x00,
        };

        for(UINT i=0; i<COUNT_OF(arr); i++) {
            size += addByte(arr[i]);
        }
    }

    setWord(initFilePosn, size);
    return size;
}

//esds:参考ffmpeg中movenc.c里的static int mov_write_esds_tag(ByteIOContext *pb, MOVTrack *track) // Basic
//32bit size
//32bit type('esds')
//32bit 8bit version+24bit flags)
//8bit  0x03表示ES descriptor
//8bit  ES descriptor长度)
//16bit track id(但经检查为0,但trak里的trackid并不为0
//8bit  flags=0
//8bit  0x04表示DecoderConfig descriptor
//8bit  DecoderConfig descriptor长度
//8bit codec tag,在ffmpeg中由ff_mp4_obj_type定义,0x20表示CODEC_ID_MPEG4,0x21表示CODEC_ID_H264
//8bit made of 6 bits to identify the streamtype (4 for video, 5 for audio)
// plus 1 bit to indicate upstream and 1 bit set to 1 (reserved)
//0x15 表示Audiostream
//0x11 表示Visualstream
//24bit Buffersize DB
//32bit maxBitrate
//32bit avgBitrate
//8bit  0x05表示DecoderSpecific info descriptor
//8bit  DecoderSpecific info descriptor长度decLen
//decLen个字节,mpeg4的话为0x000001b0xxx decoder config数据
//8bit 0x06表示SL descriptor
//8bit 0x01
//8bit 0x02
long CMP4Trak::addAtom_esds()
{
    int width=m_szVideo.cx;
    int height=m_szVideo.cy;
    BYTE mpeg4Config[]= {
        0x00,0x00,0x01,0xb0,0xf3,0x00,0x00,0x01,0xb5,0x0e,0xe0,0x40,0xc0,0xcf,0x00,0x00,
        0x01,0x00,0x00,0x00,0x01,0x20,0x00,0x84,0x40,0xfa,0x28,0x2f,0xa0,0xf0,0xa2,0x1f,
    };

    CMpeg4BitStream bs;
    bs.Init(mpeg4Config,sizeof(mpeg4Config)*8,0);
    bs.WriteBit(213,width,13);
    bs.WriteBit(213+13+1,height,13);

    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("esds");
    size += addWord(0x00000000); // Version+flags

    {
        size += addByte(0x03);//es descriptor
        size+=addByte(0x80);
        size+=addByte(0x80);
        size+=addByte(0x80);
        long pos_es_desc=ftell(m_hFile);//remember for update
        size += addByte(0x00);//placeholder for es descriptor len
        size += addHalfWord(0x0000);//track id
        size += addByte(0x00);//flags

        {
            size += addByte(0x04);//decoder config desc
            //下面的连加3个0x80是兼容quicktime,不加的话,能在replayer,vlc中播放,但不能在quicktime中播放
            size+=addByte(0x80);
            size+=addByte(0x80);
            size+=addByte(0x80);
            long pos_dec_config=ftell(m_hFile);//remember for update
            size += addByte(0x00);//placeholder for decoder config desc len
            size += addByte(0x20);//mpeg4
            size += addByte(0x11);//visualstream
            size += addByte(0x00);//24bit buf size
            size += addHalfWord(0xFFFF);
            size += addWord(0x000FFFFF);//32bit maxbitrate
            size += addWord(0x000FFFFF);//32bit avgbitrate

            size += addByte(0x05);//DecoderSpecific info descriptor
            size+=addByte(0x80);
            size+=addByte(0x80);
            size+=addByte(0x80);
            size += addByte(sizeof(mpeg4Config));//8bit  DecoderSpecific info descriptor长度decLen
            for(UINT i=0; i<sizeof(mpeg4Config); i++) {
                size += addByte(mpeg4Config[i]);
            }

            ModifyByte(pos_dec_config,(BYTE)(ftell(m_hFile)-pos_dec_config-1));
        }

        size += addByte(0x06);//SL descriptor
        size+=addByte(0x80);
        size+=addByte(0x80);
        size+=addByte(0x80);
        size += addByte(0x01);
        size += addByte(0x02);
        ModifyByte(pos_es_desc,(BYTE)(ftell(m_hFile)-pos_es_desc-1));
    }

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_stbl_stsd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("stbl");

    size += addAtom_stsd();

    m_stts_addr = initFilePosn + size;
    if (IsVideoTrak()) {
        size += PRE_STTS_SIZE + PRE_STSC_SIZE + PRE_STSZ_SIZE + 
            PRE_STCO_SIZE + PRE_STSS_SIZE;  // 预分配
    } else {
        size += PRE_STTS_SIZE + PRE_STSC_SIZE + PRE_STSZ_SIZE + PRE_STCO_SIZE;  // 预分配
    }    

    setWord(initFilePosn, size);
    return size;
}

long CMP4Trak::addAtom_stbl_other()
{
    unsigned size = m_stts_addr;

    fseek(m_hFile, size, SEEK_SET);
    size += addAtom_stts();
    fseek(m_hFile, size, SEEK_SET);
    size += addAtom_stsc();
    fseek(m_hFile, size, SEEK_SET);
    size += addAtom_stsz();
    fseek(m_hFile, size, SEEK_SET);
    size += addAtom_stco();
    fseek(m_hFile, size, SEEK_SET);
    size += addAtom_stss();

    return size;
}

long CMP4Trak::addAtom_header()
{
    fseek(m_hFile, m_tkhd_addr, SEEK_SET);
    addWord(m_trakid); // Track ID
    addWord(0x00000000); // Reserved

    //注意要先除m_timescale,再*600,否则录像时间稍长,m_duration * 600会大于0xFFFFFFFF引起越界错误.
    //DWORD32表示的duration最大支持13小时的单个录像时长(0xFFFFFFFF/90000).
    addWord((m_duration/m_timescale)*600); // Duration.

    fseek(m_hFile, m_mdhd_addr, SEEK_SET);
    addWord(m_timescale);        // timescale
    addWord(m_duration);         // duration
    return 0;
}

int CMP4Trak::WriteToBufferedFile(LPBYTE pFrame, int cbFrame)
{
    if (!pFrame || cbFrame <= 0) 
        return -1;

    int ret = 0;
    size_t offset = 0;

    BYTE *write_buf = m_pMP4Write->m_write_buf;
    DWORD32 write_size = m_pMP4Write->m_write_size;

    while (offset < cbFrame) {
        if (write_size < REC_ONCE_WRITE_SIZE) {
            size_t space_left = REC_ONCE_WRITE_SIZE - write_size;
            size_t data_left = cbFrame - offset;
            size_t copy_bytes = (data_left < space_left) ? data_left : space_left;

            memcpy(write_buf + write_size, pFrame + offset, copy_bytes);
            
            write_size += copy_bytes;
            offset += copy_bytes;
        }

        // 如果缓冲区填满，立即刷盘
        if (write_size >= REC_ONCE_WRITE_SIZE) {
            ret = system_fwrite(write_buf, 1, write_size, m_hFile);
            if (ret != write_size) {
                SYSLOG("Write error occurred! error=%d %s\n", errno, strerror(errno));
                m_pMP4Write->m_write_size = 0; // 3. 出错时必须重置状态
                return -1;
            }
            write_size = 0;
        }
    }

    m_pMP4Write->m_write_size = write_size;

    return 0;
}

int CMP4Trak::WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    if (m_type == RTP_TYPE_ULAW || m_type == RTP_TYPE_ALAW) {
        if (!m_dwRTPTimeStamp) {
            m_dwRTPTimeStamp = dwRTPTimeStamp;
        } else {
            m_dwRTPTimeStamp += cbFrame;
            dwRTPTimeStamp = m_dwRTPTimeStamp;
        }
    }
    return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,TRUE);
}

int CMP4Trak::WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,FALSE);
}

int CMP4Trak::WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader)
{
    int ret=-1;
    if(m_type == RTP_TYPE_ULAW) {
        ret = OnUlawFrame(pFrame, cbFrame, dwRTPTimeStamp);
    } else if(m_type == RTP_TYPE_ALAW ) {
        ret = OnUlawFrame(pFrame, cbFrame, dwRTPTimeStamp);
    } else if (m_type == RTP_TYPE_AAC) {
        ret = OnAacFrameHelper(pFrame, cbFrame, dwRTPTimeStamp);
    } else {
        if(bRTPHeader) {
            ret = OnAmrFrame(pFrame, cbFrame, dwRTPTimeStamp);
        } else {
            ret = OnAmrFrameHelper(pFrame, cbFrame, dwRTPTimeStamp);
        }
    }

    return ret;
}

//OnUlawFrameHelper接收由OnUlawFrame处理过的理想数据
int CMP4Trak::OnUlawFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    LPBYTE pUlawData = pFrame;
    const UINT cbUlawData = cbFrame;

    //bJCOK表示支持mp4修复
    const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

    BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
    m_pMP4Write->m_pTrakLastWrite=this;
    if(bJCOK) {
        //支持mp4修复时,chunk被禁用
        bAppendLastChunk = FALSE;
    }

    //stco
    {
        if(!bAppendLastChunk) {
            long pos = ftell(m_hFile);
            if(bJCOK) {
                //真正的audio/video sample位于tagMP4SampleHeader后面
                m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
            } else {
                m_lst_stco.push_back(pos);
            }
            m_st_count[eAtomStco]++;
        }
    }

    if(bJCOK) {
        tagMP4SampleHeader sh;
        ASSERT(sizeof(sh) == 8);
        sh.trakid=(BYTE)m_trakid;
        sh.duration = cbUlawData;//注意:这里保存的是下一帧的duration,修复时要赋给下一帧
        sh.flags = 1;//目前只有一个标志位,用于标识关键帧,audio都为关键帧
        //size不包括sh本身的大小
        sh.size = cbFrame;

        //把sh转成net顺序,假设host为小端
        DWORD32 sz = sh.size;
        DWORD32 dur = sh.duration;
        sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
        sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);
        system_fwrite(&sh,1,sizeof(sh),m_hFile);
    }

    system_fwrite(pUlawData,1,cbUlawData,m_hFile);

    //stsz
    {
        if(m_lst_stsz.size()==0) {
            m_lst_stsz.push_back(cbUlawData);
            m_st_count[eAtomStsz]++;
        } else {
            ASSERT(cbUlawData==m_lst_stsz[0]);//audio packet should be the sample size
        }

        m_stsz_count++;
    }

    //stsc
    {
        tagMP4Node_stsc* ps = NULL;
        if(bAppendLastChunk) {
            ps = GetLastStscNode();
        } else {
            ps = new tagMP4Node_stsc;
            ps->FirstChunk=++m_nFirstChunk;
            ps->SampleDescriptionID=1;
            m_lst_stsc.push_back(ps);
            m_st_count[eAtomStsc]++;
        }

        ps->SamplesPerChunk++;

        //todo:stsc还可以做点优化,把samplesPerChunk相同并相邻chuck合并成一个chunk entry
    }

    //stts
    {
        if(m_dwFrameIndex>0) {
            OnNewDuration(dwRTPTimeStamp);
        }
    }

    //audio没有stss

    m_dwFrameIndex++;

    return 0;
}

int CMP4Trak::OnAacFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	int ret = 0;
	
    int time_scale_per_sample=2048/2; // 每帧 2048(char) = 1024(short) 

    m_dwTickRTPLast = dwRTPTimeStamp;

    //bJCOK表示支持mp4修复
    const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

    BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
    m_pMP4Write->m_pTrakLastWrite=this;
    if(bJCOK) {
        //支持mp4修复时,chunk被禁用
        bAppendLastChunk = FALSE;
    }

    DWORD32 write_size = m_pMP4Write->m_write_size;

    //stco
    {
        if(!bAppendLastChunk) {
            long pos = ftell(m_hFile) + write_size;
            if(bJCOK) {
                //真正的audio/video sample位于tagMP4SampleHeader后面
                m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
            } else {
                m_lst_stco.push_back(pos);
            }
            m_st_count[eAtomStco]++;
        }
    }

    if(bJCOK) {
        tagMP4SampleHeader sh;
        ASSERT(sizeof(sh) == 8);

        sh.trakid=(BYTE)m_trakid;
        sh.duration = time_scale_per_sample;//注意:这里保存的是下一帧的duration,修复时要赋给下一帧
        sh.flags = 1;//目前只有一个标志位,用于标识关键帧,audio都为关键帧
        //size不包括sh本身的大小
        sh.size = cbFrame;
        
        //把sh转成net顺序,假设host为小端
        DWORD32 sz = sh.size;
        DWORD32 dur = sh.duration;
        sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
        sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);

        WriteToBufferedFile((unsigned char *)&sh, sizeof(sh));
    }
    
    WriteToBufferedFile(pFrame, cbFrame);

    //stsz
    {
        m_lst_stsz.push_back(cbFrame);  // aac 每帧尺寸不固定，需要一直添加
        m_st_count[eAtomStsz]++;
        m_stsz_count++;
    }

    //stsc
    {
        tagMP4Node_stsc* ps = NULL;
        if(bAppendLastChunk) {
            ps = GetLastStscNode();
        } else {
            ps = new tagMP4Node_stsc;
            ps->FirstChunk=++m_nFirstChunk;
            ps->SampleDescriptionID=1;
            m_lst_stsc.push_back(ps);
            m_st_count[eAtomStsc]++;
        }

        ps->SamplesPerChunk++;
    }

    //stts
    {
        if(m_dwFrameIndex>0) {
            OnNewDuration(time_scale_per_sample);//todo
        }
    }

    //audio没有stss
    m_dwFrameIndex++;
    return 0;
}


//OnAmrFrameHelper接收由OnAmrFrame处理过的理想数据
int CMP4Trak::OnAmrFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    BYTE amrHeader = pFrame[0];
    BYTE ft = (amrHeader >> 3) & 0x0F;
    ASSERT(m_amr_ft == 0 || m_amr_ft == ft);
    m_amr_ft = ft;
    ASSERT(cbFrame == m_amr_packed_size[m_amr_ft] + 1);
	int ret = 0;
	
    int time_scale_per_sample=160;

    m_dwTickRTPLast = dwRTPTimeStamp;

    //bJCOK表示支持mp4修复
    const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

    BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
    m_pMP4Write->m_pTrakLastWrite=this;
    if(bJCOK) {
        //支持mp4修复时,chunk被禁用
        bAppendLastChunk = FALSE;
    }

    //stco
    {
        if(!bAppendLastChunk) {
            long pos = ftell(m_hFile);
            if(bJCOK) {
                //真正的audio/video sample位于tagMP4SampleHeader后面
                m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
            } else {
                m_lst_stco.push_back(pos);
            }
        }
    }

    if(bJCOK) {
        tagMP4SampleHeader sh;
        ASSERT(sizeof(sh) == 8);

        sh.trakid=(BYTE)m_trakid;
        sh.duration = time_scale_per_sample;//注意:这里保存的是下一帧的duration,修复时要赋给下一帧
        sh.flags = 1;//目前只有一个标志位,用于标识关键帧,audio都为关键帧
        //size不包括sh本身的大小
        sh.size = cbFrame;

        //把sh转成net顺序,假设host为小端
        DWORD32 sz = sh.size;
        DWORD32 dur = sh.duration;
        sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
        sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);
        ret = system_fwrite(&sh,1,sizeof(sh),m_hFile);
		if (ret != sizeof(sh)) {
			SYSLOG("A write error has occured! error ret:%d, len:%d, error=%d %s\n", ret, sizeof(sh), errno, strerror(errno));
		    return -1;
		}
    }

    ret = system_fwrite(pFrame,1,cbFrame,m_hFile);
	if (ret != cbFrame) {
		SYSLOG("A write error has occured! error ret:%d, len:%d, error=%d %s\n", ret, cbFrame, errno, strerror(errno));
	    return -1;
	}

    //stsz
    {
        if(m_lst_stsz.size()==0) {
            m_lst_stsz.push_back(m_amr_packed_size[m_amr_ft]+1);
        }
        m_stsz_count++;
    }

    //stsc
    {
        tagMP4Node_stsc* ps = NULL;
        if(bAppendLastChunk) {
            ps = GetLastStscNode();
        } else {
            ps = new tagMP4Node_stsc;
            ps->FirstChunk=++m_nFirstChunk;
            ps->SampleDescriptionID=1;
            m_lst_stsc.push_back(ps);
            bAppendLastChunk=TRUE;//prepare for the second amr data frame
        }

        ps->SamplesPerChunk++;
    }

    //stts
    {
        if(m_dwFrameIndex>0) {
            OnNewDuration(time_scale_per_sample);//todo
        }
    }

    //audio没有stss
    m_dwFrameIndex++;
    return 0;
}

WORD CMP4Trak::m_amr_packed_size[8] = {12, 13, 15, 17, 19, 20, 26, 31,};
//todo:暂时采用静音包来处理丢帧,后续再改进
int CMP4Trak::OnAmrFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    int ret = 0;
    /*
    {
        static DWORD32 dwTimeStampLast = 0;
        if(dwTimeStampLast)
        {
            DWORD32 delta = dwRTPTimeStamp - dwTimeStampLast;
            DT("amr delta = %d",delta);
        }

        dwTimeStampLast = dwRTPTimeStamp;
    }
    //*/

    ASSERT(pFrame[0]==0xF0);

    //分析amr包个数
    int nPack=1;
    {
        for(int i=1; i<cbFrame; i++) {
            if((pFrame[i]&0x80) == 0x00) {
                break;
            }
            nPack++;
        }
    }

    BYTE amrHeader = pFrame[nPack];
    BYTE ft = (amrHeader >> 3) & 0x0F;
    if(ft>=COUNT_OF(m_amr_packed_size)) {
        //DW("invalid amr");
        return -1;
    }

    m_amr_ft = ft;

    if(cbFrame!= (1+m_amr_packed_size[m_amr_ft])*nPack + 1) {
        ASSERT(FALSE);
        return -1;
    }

    DWORD32 dwDurationLastFrame = 0;
    UNUSED(dwDurationLastFrame);
    if(m_dwFrameIndex>0) {
        //计算上一帧的duration
        if(dwRTPTimeStamp > m_dwTickRTPLast) {
            dwDurationLastFrame  = dwRTPTimeStamp - m_dwTickRTPLast;
        } else {
#ifdef _MSC_VER
            if(abs(0xFFFFFFFF - dwRTPTimeStamp)<8000*10) { //阀值:10秒
                dwDurationLastFrame  = (0xFFFFFFFF - m_dwTickRTPLast) + dwRTPTimeStamp;
                DW("RTP timestamp loopback,m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x,dwDurationLastFrame=0x%x",
                   m_dwTickRTPLast,dwRTPTimeStamp,dwDurationLastFrame);
            } else
#endif
            {
                DW("[Error] ulaw rtp timestamp loopback:m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x",m_dwTickRTPLast,dwRTPTimeStamp);
                ASSERT(FALSE);
                //采用默认值
                dwDurationLastFrame=160;//amr
            }
        }
    }

    /*
    dwDurationLastFrame = ((dwDurationLastFrame+10)/(160*nPack))*(160*nPack);//忽略微小误差,amr

    int nBlock = dwDurationLastFrame/(160*nPack);//amr
    if(nBlock>1)
    {
        //说明有音频丢帧,需要填充丢失的静音数据
        BYTE mute[65];
        memset(mute,0x11,sizeof(mute));
        memcpy(mute,pFrame,3);//copy amr header
        for(int i=0;i<nBlock-1;i++)
        {
            static int nTimes=0;
            DT("insert mute audio frame,nTimes=%d",nTimes++);
            OnAmrFrameHelper(pFrame,cbFrame,m_dwTickRTPLast+(i+1)*320);//amr
        }
    }
    //*/

    {
        LPBYTE pamr = pFrame + 1 + nPack;//skip header
        for(int i=0; i<nPack; i++) {
            BYTE amr[32];//最大为32
            amr[0]=amrHeader;
            memcpy(amr+1,pamr,m_amr_packed_size[m_amr_ft]);

            ret = OnAmrFrameHelper(amr,m_amr_packed_size[m_amr_ft]+1,m_dwTickRTPLast+160);
            pamr+=m_amr_packed_size[m_amr_ft];
        }
    }

    return ret;
}

//XiongWanPign 2009.11.11
//已测试,采用像video那样增加duration的方法来处理音频丢帧只对VLC有效,但对QuickTimePlayer无效
//下面采用插入静音帧来处理音频丢帧
int CMP4Trak::OnUlawFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    //LPBYTE pUlawData = pFrame;
    //UINT cbUlawData = cbFrame;
    DWORD32 dwDurationLastFrame = 0;
    if(m_dwFrameIndex>0) {
        //计算上一帧的duration
        if(dwRTPTimeStamp > m_dwTickRTPLast) {
            dwDurationLastFrame  = dwRTPTimeStamp - m_dwTickRTPLast;
        } else {
#ifdef _MSC_VER
            if(abs(0xFFFFFFFF - dwRTPTimeStamp)<8000*10) { //阀值:10秒
                dwDurationLastFrame  = (0xFFFFFFFF - m_dwTickRTPLast) + dwRTPTimeStamp;
                DW("RTP timestamp loopback,m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x,dwDurationLastFrame=0x%x",
                   m_dwTickRTPLast,dwRTPTimeStamp,dwDurationLastFrame);
            } else
#endif
            {
                DW("[Error]ulaw rtp timestamp loopback:m_dwTickRTPLast=0x%x,dwRTPTimeStamp=0x%x",m_dwTickRTPLast, dwRTPTimeStamp);
                ASSERT(FALSE);

                //采用默认值
                dwDurationLastFrame=cbFrame;
            }
        }
    }

    //dwDurationLastFrame有时并不是cbFrame的整数倍,这里忽略微小误差
    dwDurationLastFrame = ((dwDurationLastFrame+10)/cbFrame)*cbFrame;

    /*int nBlock = dwDurationLastFrame/cbFrame;
    if(nBlock>1) {
        //说明有音频丢帧,用静音填充丢失的数据
        BYTE mute[4096];
        memset(mute,0xEF,sizeof(mute));
        LPBYTE pMute = mute;
        if(cbFrame>(int)sizeof(mute)) {
            pMute = new BYTE[cbFrame];//remember to delete
            memset(pMute,0xEF,cbFrame);
        }

        for(int i=0; i<nBlock-1; i++) {
            static int nTimes=0;
            DT("insert mute audio frame,nTimes=%d",nTimes++);
            OnUlawFrameHelper(pFrame,cbFrame,m_dwTickRTPLast+(i+1)*cbFrame);
        }

        if(pMute != mute) {
            delete []pMute;
            pMute=NULL;
        }
    }*/

    m_dwTickRTPLast = dwRTPTimeStamp;

    OnUlawFrameHelper(pFrame,cbFrame,dwDurationLastFrame);

    return 0;
}

void CMP4Trak::OnNewDuration(DWORD32 dwDurationLastFrame)
{
//#ifdef _DEBUG
    if(IsVideoTrak()) {
        if(dwDurationLastFrame>90000*10) {
            DW("######### dwDurationLastFrame=%d",dwDurationLastFrame);
            //set as default
            dwDurationLastFrame = 90000/25;
        }
    }
//#endif

    m_duration += dwDurationLastFrame;

    //tagMP4Node_stts *ps=GetLastSttsNode();
    //if(ps && ps->SampleDuration == dwDurationLastFrame) {
        //ps->SampleCount++;
    //} else {
        tagMP4Node_stts *ps = new tagMP4Node_stts;
        ps->SampleDuration=dwDurationLastFrame;
        ps->SampleCount++;
        m_lst_stts.push_back(ps);
        m_st_count[eAtomStts]++;
    //}
}

int CMP4Trak::WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
	int ret = 0;
    if(!m_hFile) {
        return -1;
    }

    const BOOL32 bH264=IsH264() || IsH265();

    if(bH264) {
        const BOOL32 bHasH264Prefix= (bH264 && (pFrame[0] == 0x00 && pFrame[1] == 0x00 && pFrame[2] == 0x00 && pFrame[3] == 0x01));
        //董超库中DecDraw提供的h.264增加了0x00000001头
        if(bHasH264Prefix) {
            //去掉0x00000001头
            pFrame+=4;
            cbFrame-=4;
        }

        /*
        BYTE naluType = (pFrame[0]&0x1F);
        if(
            naluType==7
            || naluType==8
            )
        {
            if(bHasWrite_sps_pps)
                return 0;

            //录像不需要保存sps,pps
            //如果保存了sps,pps,quicktimeplayer播放时会有闪屏,vlc则播放正常.
            m_bH264IFrame=TRUE;
            return 0;
        }
        //*/
    } else if(IsMjpg()) {

    } else {
        //限制mpeg4第1帧必须是I帧
        if(m_dwFrameIndex==0 && !IsIFrame(pFrame)) {
            return 0;
        }
    }

    DWORD32 dwDurationLastFrame = 0;
    if(m_dwFrameIndex>0) {
        //计算上一帧的duration
        if(dwRTPTimeStamp >= m_dwTickRTPLast) {
            dwDurationLastFrame  = dwRTPTimeStamp - m_dwTickRTPLast;
        } else {
            dwDurationLastFrame  = (0xFFFFFFFF - m_dwTickRTPLast) + dwRTPTimeStamp;
            DW("RTP timestamp loop back,m_dwTickRTPLast=%u,dwRTPTimeStamp=%u,dwDurationLastFrame=%u",
               m_dwTickRTPLast,dwRTPTimeStamp,dwDurationLastFrame);
        }
    }

    //DT("rtp ts=%d,last ts=%d",dwRTPTimeStamp,dwDurationLastFrame);

    m_dwTickRTPLast = dwRTPTimeStamp;

    //bJCOK表示支持mp4修复
    const BOOL32 bJCOK = (m_pMP4Write->GetMP4Flags() & MP4_FLAGS_JCOK);

    //判断是否需要新建chunk
    //bAppendLastChunk为TRUE时,表示上一次write sample也是此trak的数据,因此不需要重新分chunk
    BOOL32 bAppendLastChunk = (m_pMP4Write->m_pTrakLastWrite==this);
    m_pMP4Write->m_pTrakLastWrite=this;
    if(bJCOK) {
        //支持mp4修复时,chunk被禁用
        bAppendLastChunk = FALSE;
    }

    DWORD32 write_size = m_pMP4Write->m_write_size;
    
    //stco
    {
        if(!bAppendLastChunk) {
            long pos = ftell(m_hFile) + write_size;
            if(bJCOK) {
                //真正的audio/video sample位于tagMP4SampleHeader后面
                m_lst_stco.push_back(pos+sizeof(tagMP4SampleHeader));
            } else {
                m_lst_stco.push_back(pos);
            }
            m_st_count[eAtomStco]++;
        }
    }

    BOOL32 bIFrame = IsIFrame(pFrame);
    if(m_bH264IFrame) {
        bIFrame =TRUE;
        m_bH264IFrame=FALSE;
    } else if(IsMjpg()) {
        bIFrame =TRUE;
    }

    if(bJCOK) {
        tagMP4SampleHeader sh;
        ASSERT(sizeof(sh) == 8);

        sh.trakid=(BYTE)m_trakid;
        sh.duration = dwDurationLastFrame;//注意:这里保存的是下一帧的duration,修复时要赋给下一帧
        sh.flags = bIFrame?1:0;//目前只有一个标志位,用于标识关键帧
        //size不包括sh本身的大小
        sh.size = cbFrame + (bH264?sizeof(cbFrame):0);//h.264时数据前面会写上dwFrameLen,所以要加上4字节

        //把sh转成net顺序,假设host为小端
        DWORD32 sz = sh.size;
        DWORD32 dur = sh.duration;
        sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
        sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);

        WriteToBufferedFile((unsigned char *)&sh, sizeof(sh));
    }

    if(bH264) {
        DWORD32 dwLen=htonl(cbFrame);
        WriteToBufferedFile((unsigned char *)&dwLen, sizeof(dwLen));
    }

    WriteToBufferedFile(pFrame,cbFrame);

    //stsz
    {
        //认为每个sample size都不相同,所以要记录每个sample size
        m_lst_stsz.push_back(cbFrame+(bH264?sizeof(cbFrame):0));//h.264时数据前面会写上dwFrameLen,所以要加上4字节
        m_st_count[eAtomStsz]++;
    }

    //stsc
    {
        tagMP4Node_stsc* ps = NULL;
        if(bAppendLastChunk) {
            ps = GetLastStscNode();
        } else {
			ps = new tagMP4Node_stsc;
            ps->FirstChunk=++m_nFirstChunk;
            ps->SampleDescriptionID=1;
            m_lst_stsc.push_back(ps);
            m_st_count[eAtomStsc]++;
        }

        ps->SamplesPerChunk++;
    }

    //stts
    if(m_dwFrameIndex>0) {
        //视频帧的duration每个都不同,并且当前帧的duration只能等到已经收到下一帧时才能算出
        OnNewDuration(dwDurationLastFrame);
    }

    //realplayer要求至少有两帧,否则会显示不出来
    m_dwFrameIndex++;

    //stss
    {
        if(bIFrame) {
            m_lst_stss.push_back(m_dwFrameIndex);
            m_st_count[eAtomStss]++;
        }
    }

    return 0;
}

//CMP4Write是停止录像时会调用所有trak的PreClose
//trak可以在PreClose()中做收尾操作
//现在的收尾操作包括:把最后一帧的duration写进去
int CMP4Trak::PreClose()
{
    if(m_dwFrameIndex<=0) {
        return 0;
    }

    ASSERT(m_lst_stsz.size()>0);

    //假定最后一帧为理想状态
    if(IsVideoTrak()) {
        OnNewDuration(m_timescale/25);
    } else {
        if(m_type==0) {
            //ulaw
            OnNewDuration(m_lst_stsz[0]);
        } else if (m_type == RTP_TYPE_AAC) {
            OnNewDuration(1024);//aac
        } else {
            OnNewDuration(160);//amr
        }
    }
    return 0;
}

BOOL32 CMP4Trak::IsIFrame(LPBYTE pData)
{
    ASSERT(IsVideoTrak());

    if (IsH265()) {
        return IsH265IFrame(pData);
    }
    else if(IsH264()) {
        return IsH264IFrame(pData);
    }

    return IsMpeg4IFrame(pData);
}

CMP4Write::CMP4Write()
{
    m_hFile =  NULL;
    m_write_size = 0;
    m_dwFlags=0;
    m_bHasWriteFtyp=FALSE;

    m_pTrakLastWrite=0;
    m_dwTickStartRecord=0;
    m_pos_mdat=0;
    m_pos_jcok=0;
    m_pos_moov=0;

    m_dwMP4Flags = 0;
}

CMP4Write::~CMP4Write()
{
    if(m_hFile) {
        fclose(m_hFile);
        m_hFile=0;
    }

    Empty();
}

int CMP4Write::WriteHeader()
{
    if(!m_bHasWriteFtyp) {
        // Use the current time as the file's creation and modification
        // time.  Use Apple's time format: seconds since January 1, 1904
        struct timeval      m_fStartTime;
        unsigned            m_fAppleCreationTime;
        gettimeofday(&m_fStartTime);
        m_fAppleCreationTime = m_fStartTime.tv_sec - 0x83dac000;
        UNUSED(m_fAppleCreationTime);
        {
            //注意ftyp要在moov前面,否则quicktime不能播放
            addAtom_ftyp();

            //bJCOK表示支持mp4修复
            const BOOL32 bJCOK = (GetMP4Flags() & MP4_FLAGS_JCOK);
            if(bJCOK) {
                //添加jabsco扩展JCOK box
                m_pos_jcok=ftell(m_hFile);
                addAtom_JCOK();
            }

            m_pos_moov = ftell(m_hFile);
            unsigned size = addAtom_moov();
            UNUSED(size);
            
            m_pos_mdat = ftell(m_hFile);
            addAtomHeader("mdat");
        }

        m_bHasWriteFtyp = TRUE;

        fflush();
    }

    return 0;
}

long CMP4Write::Write_mvhd_header()
{
    fseek(m_hFile, 0, SEEK_END);
    long pos = ftell(m_hFile) - m_pos_mdat;
    setWord(m_pos_mdat,pos); 

    fseek(m_hFile, m_pos_mvhd, SEEK_SET);
    
    DWORD32 timescale = 1;
    DWORD32 duration =  1;
    CMP4Trak *pTrakA = FindAudioTrak();
    CMP4Trak *pTrakV = FindVideoTrak();

    if(pTrakV && pTrakV->GetFrameCount()>0) {
        timescale = pTrakV->m_timescale;
        duration = pTrakV->m_duration;
    } else if(pTrakA && pTrakA->GetFrameCount()>0) {
        timescale = pTrakA->m_timescale;
        duration = pTrakA->m_duration;
    }

    {
        double rate = 1.0*timescale/600;
        timescale=600;
        duration /= rate;
    }

    addWord(timescale);
    addWord(duration); // Duration

    return 0;
}

//直接调底层api fflush来确保文件被真正写入存储设备
int CMP4Write::fflush()
{
    int ret = EOF;

    if(m_hFile) {
        ret = ::fflush(m_hFile);
        fsync(fileno(m_hFile));
    }

    return ret;
}

//Attach和Detach仅供修复mp4使用
int CMP4Write::Attach(FILE *hFile)
{
    if(m_hFile) {
        return -1;
    }

    m_hFile = hFile;
    m_dwFlags |= MP4F_ATTACH;

    return 0;
}

void CMP4Write::Detach()
{
    m_hFile = NULL;
    m_dwFlags = 0;
}

//默认不支持修复录像
//要启用修复录像,dwFlags请设置为MP4_FLAGS_JCOK,由操作系统自行fflush
//如果设置dwFlags包括MP4_FLAGS_AUTO_FFLUSH,则libmp4自动fflush每一帧数据,效率可能稍低
int CMP4Write::Open(const char * pszFile,DWORD32 dwFlags)
{
    ASSERT(!m_hFile);
    if(m_hFile || strlen(pszFile) ==0) {
        return -1;
    } else {
        if (access(pszFile, F_OK) == 0) {
            DBG("pszFile is exit\n");
            m_hFile = fopen(pszFile,"rb+");            
            fseek(m_hFile, 0, SEEK_END);
        } else {
            DBG("pszFile isnt exit\n");
            m_hFile = fopen(pszFile,"wb");
        }
    }

    if (NULL == m_hFile) {
        ERR("MP4 fopen fail\n");
        return -1;
    }
    
    //注意:MP4_FLAGS_JCOK为假时设置MP4_FLAGS_AUTO_FFLUSH无意义,CMP4Write会自动启用MP4_FLAGS_JCOK
    if(dwFlags & MP4_FLAGS_AUTO_FFLUSH) {
        dwFlags |= MP4_FLAGS_JCOK;
    }

    m_dwMP4Flags = dwFlags;
	DBG("~m_hFile:%p, fd:%d\n", m_hFile, fileno(m_hFile));
    return m_hFile?0:-1;
}

//Repair()的操作与CMP4Write::Close()相近
int CMP4Write::Repair()
{
//#ifdef _MSC_VER
#if 1
    if(!m_hFile || !IsRepairMode()) {
        ASSERT(FALSE);
        return -1;
    }

    if(m_pos_mdat) {
        long pos = ftell(m_hFile) - m_pos_mdat;
        setWord(m_pos_mdat,pos);

        if(m_pos_moov) {
            fseek(m_hFile,m_pos_moov,SEEK_SET);
        }

        addAtom_moov();
        for (vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
            CMP4Trak *ptrak = *it;
            ptrak->addAtom_stbl_other();
        }

        if(m_pos_jcok) {
            fseek(m_hFile,m_pos_jcok+4,SEEK_SET);
            system_fwrite("JCOR",1,4,m_hFile);//标记为jabsco Repair
            fflush();
        }
    }

    return 0;
#else
    return -1;
#endif
}

int CMP4Write::Close()
{
    ASSERT(!IsRepairMode());//修复模式时请调用Repair()

    if(m_hFile) {
        /* Flush remaining data in application write buffer */
        if(m_write_size > 0) {
            system_fwrite(m_write_buf, 1, m_write_size, m_hFile);
            m_write_size = 0;
        }
        fflush();
        if(m_pos_mdat) {
            Write_mvhd_header();

            {
                //notify all trak to preclose
                for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
                    CMP4Trak* pTrak = *it;
                    pTrak->PreClose();
                    pTrak->addAtom_header();
                    pTrak->addAtom_stbl_other();
                }
            }

            const BOOL32 bJCOK = (GetMP4Flags() & MP4_FLAGS_JCOK);
            if(bJCOK) {
                if(m_pos_jcok) {
                    long pos = ftell(m_hFile);
                    fseek(m_hFile,m_pos_jcok+4,SEEK_SET);
                    system_fwrite("JCOK",1,4,m_hFile);
                    fseek(m_hFile,pos,SEEK_SET);
                    fflush();
                }
            }
        }
		
        fclose(m_hFile);
        m_hFile = NULL;

        m_dwFlags=0;
        m_bHasWriteFtyp=FALSE;

        m_pTrakLastWrite=0;
        m_dwTickStartRecord=0;
        m_pos_mdat=0;
        m_pos_jcok=0;
    }
    //Empty();

    return 0;
}

int CMP4Write::Pause()
{
    if (m_hFile) {
        fflush();
        fclose(m_hFile);
        m_hFile = NULL;
    }
    
    return 0;
}

int CMP4Write::IsOpen()
{
    if (m_hFile) {
        return TRUE;
    } else {
        return FALSE;
    }
}

int CMP4Write::Sync()
{
    if (!m_hFile) {
        return FALSE;
    }

    Write_mvhd_header();
    for (vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
        CMP4Trak *ptrak = *it;
        ptrak->addAtom_header();
        ptrak->addAtom_stbl_other();
    }    
    fseek(m_hFile, 0, SEEK_END);
    fflush();

    return TRUE;
}

void CMP4Write::Empty()
{
    {
        for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
            CMP4Trak* pTrak = *it;
            delete pTrak;
        }
        //m_arrTrak.clear();
    }
    return ;
}

long CMP4Write::addAtom_ftyp()
{
    BOOL32 bEnableQuickTime = TRUE;//为TRUE时,录下来的amr.mp4能在quicktime中播放,

    if(m_dwFlags & MP4F_ULAW) {
        bEnableQuickTime = FALSE;
    }

    //bEnableQuickTime=FALSE;//xwpcom 2009.11.30 test mjpg

    if(bEnableQuickTime) {
        //quicktime和realplayer都能正常播放amr

        long initFilePosn = ftell(m_hFile);
        unsigned size = addAtomHeader("ftyp");
        size += add4ByteString("3gp5");//mp42,3gp5
        size += addWord(0x00000000);
        size += add4ByteString("3gp5");//mp42
        size += add4ByteString("isom");
        //size += add4ByteString("qt  ");//xwpcom 2009.10.28 for quicktime
        setWord(initFilePosn, size);
        return size;
    } else {
        //realplayer能正常播放amr,quicktime为静音

        long initFilePosn = ftell(m_hFile);
        unsigned size = addAtomHeader("ftyp");
        size += add4ByteString("mp42");//mp42,3gp5
        size += addWord(0x00000000);
        size += add4ByteString("mp42");//mp42
        size += add4ByteString("isom");
        size += add4ByteString("qt  ");//xwpcom 2009.10.28 for quicktime
        setWord(initFilePosn, size);
        return size;
    }
}

CMP4Trak * CMP4Write::FindVideoTrak()
{
    for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
        CMP4Trak* pTrak = *it;
        if(pTrak->IsVideoTrak()) {
            return pTrak;
        }
    }

    return NULL;
}

CMP4Trak * CMP4Write::FindAudioTrak()
{
    for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
        CMP4Trak* pTrak = *it;
        if(pTrak->IsAudioTrak()) {
            return pTrak;
        }
    }

    return NULL;
}

int CMP4Write::ReInitTrak()
{
    for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
        CMP4Trak* pTrak = *it;
        pTrak->m_pMP4Write=this;
        pTrak->m_hFile = m_hFile;
    }

    return 0;
}

//mpeg4时trackid用96
int CMP4Write::InitVideo(DWORD32 type,CSize szVideo,const char* sprop_parameter_sets)
{
    //为了兼容SDP的动态payload_type
    if (type > 96) {
        type = 96;
    }
    //only support a single video trak
    CMP4Trak *pt = FindVideoTrak();

    if(pt) {
        ASSERT(FALSE);

        return -1;
    }

    ASSERT(m_hFile);
	
    pt = new CMP4Trak;
    pt->m_pMP4Write=this;
    pt->m_hFile=m_hFile;
    pt->InitVideoTrakInfo(type,"vide",szVideo,sprop_parameter_sets);
    m_arrTrak.push_back(pt);
    return 0;
}

//audio为ulaw时dwType采用0
//audio为amrdwType采用97
int CMP4Write::InitAudio(DWORD32 dwType)
{
    //only support a single audio trak
    CMP4Trak *pt = FindAudioTrak();

    if(pt) {
        ASSERT(FALSE);
        return -1;
    }

    ASSERT(m_hFile);

    pt = new CMP4Trak;
    pt->m_pMP4Write=this;
    pt->m_hFile=m_hFile;
    if (dwType == RTP_TYPE_AAC) {
        pt->InitAudioTrakInfo(dwType, "soun", 16000);
    } else {
        pt->InitAudioTrakInfo(dwType, "soun");
    }
    
    m_arrTrak.push_back(pt);
    if(dwType == 0 || dwType == RTP_TYPE_AAC) {
        m_dwFlags |= MP4F_ULAW;
    }

    return 0;
}

struct tag_atom_jcox_trak_v1 {
    DWORD32 trakid;
    BYTE    handlerType[4];
    BYTE	mediaType[4];	// 音视频格式
    DWORD32 width;
    DWORD32 height;
    DWORD32 timescale;
    DWORD32 fmtp_spropparametersets_len;
    //...后面跟fmtp_spropparametersets_len个字节的h.264参数
};

struct tag_atom_jcox_trak_v0 {
    DWORD32 trakid;
    BYTE    handlerType[4];
    DWORD32 width;
    DWORD32 height;
    DWORD32 timescale;
    DWORD32 fmtp_spropparametersets_len;
    //...后面跟fmtp_spropparametersets_len个字节的h.264参数
};

//addAtom_JCOK()内容同addAtom_moov()
long CMP4Write::addAtom_JCOK()
{
    /*
    jcox需要如下信息才能重建moov:
    trakcount:4bytes
        //下面是trakcount个trakinfo
        trakid:4bytes
        handleType:4 bytes char("vide" or "soun")
        width
        height
        timescale
        sprop_parameter_sets_len;//表示后面跟着的sprop_parameter_sets的字节数,一般为0,只在h.264时有效
        BYTE sprop_parameter_sets[sprop_parameter_sets_len];
    //*/

    long initFilePosn = ftell(m_hFile);
    //JCOK实际上是一个空的moov box,所以调试时可以将JCOX和CJOK改为moov,方便解析检查内容.
    unsigned size = addAtomHeader("JCOX");//正常关闭mp4录像文件时会改写为JCOK
    DWORD32 dwVerFlags=1;//版本号,目前定为1 for tag_atom_jcox_trak_v1
    size += addWord(dwVerFlags);

    DWORD32 dwTrackCount = m_arrTrak.size();
    size += addWord(dwTrackCount);
    for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
        CMP4Trak *pt = *it;
        size += addWord(pt->m_trakid);
        size += add4ByteString(pt->m_handlerType);
        if (pt->IsVideoTrak()) { // 记录视频格式
            size += add4ByteString(pt->IsH265() ? "H265" : "H264");
        } else { // 记录音频格式
            if (pt->m_type == RTP_TYPE_AAC) {
                size += add4ByteString("AAC");
            } else {
                size += add4ByteString("ULAW");
            }
        }
		
        size += addWord(pt->m_szVideo.cx);
        size += addWord(pt->m_szVideo.cy);
        size += addWord(pt->m_timescale);

        if(pt->m_fmtp_spropparametersets) {
            size += addWord(strlen(pt->m_fmtp_spropparametersets));
            size += addArbitraryString(pt->m_fmtp_spropparametersets,FALSE);
        } else {
            size += addWord(0);
        }
    }

    setWord(initFilePosn, size);
    return size;
}

long CMP4Write::addAtom_moov()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("moov");

    size += addAtom_mvhd();
    
    {
        for(vector<CMP4Trak*>::iterator it=m_arrTrak.begin(); it!=m_arrTrak.end(); it++) {
            CMP4Trak *pt = *it;
            //if(pt->GetFrameCount()>0) { //屏蔽没有任何数据的trak
                size += pt->addAtom_trak();
            //}
        }
    }

    setWord(initFilePosn, size);
    
    return size;
}

long CMP4Write::addAtom_mvhd()
{
    long initFilePosn = ftell(m_hFile);
    unsigned size = addAtomHeader("mvhd");

    struct timeval      m_fStartTime;
    unsigned            m_fAppleCreationTime;;
    gettimeofday(&m_fStartTime);
    m_fAppleCreationTime = m_fStartTime.tv_sec - 0x83dac000;

    size += addWord(0x00000000); // Version + Flags
    size += addWord(m_fAppleCreationTime); // Creation time
    size += addWord(m_fAppleCreationTime); // Modification time

    // For the "Time scale" field, use the largest RTP timestamp frequency
    // that we saw in any of the subsessions.
    DWORD32 timescale = 1;
    DWORD32 duration =  1;
    CMP4Trak *pTrakA = FindAudioTrak();
    CMP4Trak *pTrakV = FindVideoTrak();

    if(pTrakV && pTrakV->GetFrameCount()>0) {
        timescale = pTrakV->m_timescale;
        duration = pTrakV->m_duration;
    } else if(pTrakA && pTrakA->GetFrameCount()>0) {
        timescale = pTrakA->m_timescale;
        duration = pTrakA->m_duration;
    }

    {
        double rate = 1.0*timescale/600;
        timescale=600;
        duration /= rate;
    }

    m_pos_mvhd = ftell(m_hFile);
    size += addWord(timescale);

    size += addWord(duration); // Duration

    size += addWord(0x00010000); // Preferred rate
    size += addWord(0x01000000); // Preferred volume + Reserved[0]
    size += addZeroWords(2); // Reserved[1-2]
    size += addWord(0x00010000); // matrix top left corner
    size += addZeroWords(3); // matrix
    size += addWord(0x00010000); // matrix center
    size += addZeroWords(3); // matrix
    size += addWord(0x40000000); // matrix bottom right corner
    size += addZeroWords(6); // various time fields
    size += addWord(2009);// Next track ID,可以为其它值,不和当前的video/audio trackid相同就可以了

    setWord(initFilePosn, size);
    return size;
}

int CMP4Write::WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader)
{
    if(!m_hFile) {
        //ASSERT(FALSE);
        return -1;
    }

    if(!m_bHasWriteFtyp) {
        WriteHeader();
    }

    if(m_dwTickStartRecord==0) {
        if(FindVideoTrak())//如果同时录视频,则等待第一个I帧后才开始录audio
            return 0;
#ifdef _MSC_VER
        m_dwTickStartRecord = GetTickCount();
#else
        time_t tm;
        m_dwTickStartRecord = time(&tm);
#endif
    }

    int ret =0;
    CMP4Trak *pt = FindAudioTrak();
    if(pt) {
        fseek(m_hFile, 0, SEEK_END);
        if(bRTPHeader) {
            ret = pt->WriteAudioFrame(pFrame,cbFrame,dwRTPTimeStamp);
        } else {
            ret = pt->WriteAudioFrameEx(pFrame,cbFrame,dwRTPTimeStamp);
        }
    }

    if(ret) {
        //Close();
    }

    return ret;
}


//pFrame指向一个完整的audio frame
//cbFrame是audio frame字节数
//dwRTPTimeStamp为rtp timestamp
int CMP4Write::WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,TRUE);
}

int CMP4Write::WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    return WriteAudioFrameHelper(pFrame,cbFrame,dwRTPTimeStamp,FALSE);
}

//pFrame指向一个完整的video frame
//cbFrame是video frame字节数
//dwRTPTimeStamp为rtp timestamp
int CMP4Write::WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    if(!m_hFile) {
        //ASSERT(FALSE);
        return -1;
    }

    if(!m_bHasWriteFtyp) {
        WriteHeader();
    }

    CMP4Trak *pt = FindVideoTrak();
    ASSERT(pt);

    if(m_dwTickStartRecord==0) {
        if(!pt->IsH265() && !pt->IsH264() && !pt->IsMjpg() && !pt->IsIFrame(pFrame)) {
            return 0;
        }

#ifdef _MSC_VER
        m_dwTickStartRecord = GetTickCount();
#else
        time_t tm;
        m_dwTickStartRecord = time(&tm);
#endif
    }

    int ret =0;
    if(pt) {
        ret = pt->WriteVideoFrame(pFrame,cbFrame,dwRTPTimeStamp);
    }

    if(ret) {
        //Close();
    }

    return ret;
}

int CMP4Write::WriteFrame(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
#ifdef _DEBUG
   /* 
    {
        DT("WriteFrame(bVideo=%d,pFrame=0x%02x%02x%02x%02x,0x%02x%02x%02x%02x,cbFrame=%d",
            bVideo,
            pFrame[0],pFrame[1],pFrame[2],pFrame[3],
            pFrame[4],pFrame[5],pFrame[6],pFrame[7],
            cbFrame
            );
    }
    //*/
#endif

    int ret = bVideo?WriteVideoFrame(pFrame,cbFrame,dwRTPTimeStamp):WriteAudioFrame(pFrame,cbFrame,dwRTPTimeStamp);

    if(m_dwMP4Flags & MP4_FLAGS_AUTO_FFLUSH) {
        fflush();
    }

    return ret;
}

int CMP4Write::WriteFrameEx(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp)
{
    int ret = bVideo?WriteVideoFrame(pFrame,cbFrame,dwRTPTimeStamp):WriteAudioFrameEx(pFrame,cbFrame,dwRTPTimeStamp);

    if(m_dwMP4Flags & MP4_FLAGS_AUTO_FFLUSH) {
        fflush();
    }

    return ret;
}

BOOL32 CMP4Write::GetFileSize(LONGLONG* lpFileSize)
{
#ifdef _MSC_VER
    struct _stati64 buffer;
    int ret = _fstati64(m_hFile->_file,&buffer);
    if(ret==0 && lpFileSize) {
        *lpFileSize=buffer.st_size;
    }
    return ret==0;
#else
    long pos = ftell(m_hFile);
    fseek(m_hFile,0,SEEK_END);
    long size = ftell(m_hFile);
    fseek(m_hFile,pos,SEEK_SET);
    *lpFileSize=size;
    return TRUE;
#endif
}

DWORD32 CMP4Write::GetRecordTick()
{
    if(!m_dwTickStartRecord)
        return 0;
#ifdef _MSC_VER
    return GetTickCount() - m_dwTickStartRecord;
#else
    time_t tm;
    time(&tm);
    return tm - m_dwTickStartRecord;
#endif


}

#ifdef _LINUX_APP
#include "test_record.h"
int main()
{
    {
        CMP4Record rec;
        rec.Test();
        return 0;
    }


    /*
    //parse frame
    {
        CMP4Read mp4Read;
        int ret = mp4Read.Open("./t.mp4");
        DT("ret = %d",ret);
        mp4Read.Seek(0);

        int cbVideoBuf=512*1024;
        LPBYTE pVideoBuf = new BYTE[cbVideoBuf];
        int nFrame=0;
        while(1)
        {
            int cbRead=0;
            DWORD32 duration =0;
            int ret = mp4Read.ReadVideoFrame(pVideoBuf,cbVideoBuf,cbRead,duration);

            if(cbRead==0)
            {
                break;
            }

            DT("frame[%06d]=%d,hex=0x%02x%02x%02x%02x",nFrame,cbRead,
                pVideoBuf[0],pVideoBuf[1],pVideoBuf[2],pVideoBuf[3]
                );

            nFrame++;
        }
    }
    //*/

    return 0;
}
#endif

CMP4Read::CMP4Read()
{
    m_iFile=NULL;

    m_cbFile=0;
    m_pRootAtom=NULL;

    m_pVideoTrak = NULL;
    m_pAudioTrak = NULL;
}

CMP4Read::~CMP4Read()
{
    Close();
}

//说明:CMP4Read会自动delete iReadFile
int CMP4Read::Attach(IReadFile *iReadFile)
{
    if(m_iFile || !iReadFile || !iReadFile->IsOpen()) {
        ASSERT(FALSE);

        return -1;
    }

    m_iFile = iReadFile;
    return OpenHelper();
}

int CMP4Read::Open(const char * pszFile)
{
    ASSERT(!m_iFile);
    if (strlen(pszFile) == 0) {
        return -1;
    }
    IReadFile * iReadFile = CReadFile::CreateInstance(pszFile,"rb");
    if(!iReadFile) {
        return -1;
    }

    m_iFile = iReadFile;
    return OpenHelper();
}

int CMP4Read::OpenHelper()
{
    int ret = CheckValid();
    if(ret) {
        DW("invalid mp4 file");
        Close();
        return ret;
    }

    ParseAtom();

    {
        int ncTrak = GetTrakCount();
        if(ncTrak<=0 || ncTrak>2) {
            DW("ncTrak=%d",ncTrak);
            Close();
            return -1;
        }
    }

    //收集audio/video info
    {
        m_pVideoTrak = FindVideoTrak();
        m_pAudioTrak = FindAudioTrak();

        m_vi.video_type=eVideoType_None;

        if(m_pVideoTrak) {
            CMP4Read_atom_tkhd *pAtom_tkhd = (CMP4Read_atom_tkhd *)m_pVideoTrak->FindChildAtom("tkhd");
            if(pAtom_tkhd) {
                m_vi.bValid=TRUE;
                m_vi.type=96;//todo
                m_vi.width=pAtom_tkhd->m_width;
                m_vi.height=pAtom_tkhd->m_height;
                m_vi.duration=pAtom_tkhd->m_duration;
                m_vi.frame_count=m_pVideoTrak->GetFrameCount();
                if(m_vi.frame_count == -1) {
                    DW("bad file,m_stsz is NULL,return\n");
                    return -1;
                }

                //先设置理想值
                m_vi.fps=25;
                m_vi.timescale=90000;

                //再计算实际帧率
                CMP4Read_atom_mdhd *pAtom_mdhd = (CMP4Read_atom_mdhd *)m_pVideoTrak->FindChildAtom("mdia.mdhd");
                if(pAtom_mdhd && pAtom_mdhd->m_timescale!=0) {
                    m_vi.timescale=pAtom_mdhd->m_timescale;
                    m_vi.duration=pAtom_mdhd->m_duration;
                    double second = 1.0 * pAtom_mdhd->m_duration/pAtom_mdhd->m_timescale;
                    m_vi.fps = m_vi.frame_count/second;
                }

                //DT("m_vi.fps =%.1f fps",m_vi.fps );

                if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.mp4v")) {
                    m_vi.video_type=eVideoType_Mpeg4;
                } else if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.avc1")) {
                    m_vi.video_type=eVideoType_H264;
                } else if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.hvc1")) {
                    m_vi.video_type=eVideoType_H265;
                } else if(m_pVideoTrak->FindChildAtom("mdia.minf.stbl.stsd.mjpa")) {
                    m_vi.video_type=eVideoType_Mjpg;
                }
            }
        }

        m_ai.bValid = FALSE;
        if(m_pAudioTrak) {
            CMP4Read_atom *pAtom_ulaw = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.ulaw");
            CMP4Read_atom *pAtom_alaw = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.alaw");
            CMP4Read_atom *pAtom_samr = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.samr");
            CMP4Read_atom *pAtom_mp4a = m_pAudioTrak->FindChildAtom("mdia.minf.stbl.stsd.mp4a");
            if (pAtom_ulaw) {
                m_ai.type = RTP_TYPE_ULAW;
            } else if(pAtom_alaw) {
                m_ai.type = RTP_TYPE_ALAW;
            } else if (pAtom_mp4a) {
                m_ai.type = RTP_TYPE_AAC;
            } else {
                m_ai.type = RTP_TYPE_AMR;
            }
            CMP4Read_atom_tkhd *pAtom_tkhd = (CMP4Read_atom_tkhd *)m_pAudioTrak->FindChildAtom("tkhd");
            if(pAtom_tkhd && (pAtom_ulaw || pAtom_alaw || pAtom_mp4a || pAtom_samr)) {
                m_ai.bValid = TRUE;
                m_ai.duration=pAtom_tkhd->m_duration;
                m_ai.frame_count=m_pAudioTrak->GetFrameCount();
            }

            CMP4Read_atom_mdhd *pAtom_mdhd = (CMP4Read_atom_mdhd *)m_pAudioTrak->FindChildAtom("mdia.mdhd");
            if(pAtom_mdhd && pAtom_mdhd->m_timescale!=0) {
                //m_ai.timescale=pAtom_mdhd->m_timescale;
                m_ai.duration=pAtom_mdhd->m_duration;
            }
        }

        if(!m_ai.bValid) {
            m_pAudioTrak=NULL;
        }
    }

    m_iFile->fseek(0,SEEK_SET);

    if(m_vi.video_type == eVideoType_None) {
        Close();
        return -1;
    }

    return 0;
}

int CMP4Read::Close()
{
    if(m_pRootAtom) {
        delete m_pRootAtom;
        m_pRootAtom=NULL;
    }

    if(m_iFile) {
        //DT("delete m_iFile,m_iFile=0x%x",m_iFile);
        delete m_iFile;
        m_iFile=NULL;
    }

    memset(&m_vi,0,sizeof(m_vi));
    memset(&m_ai,0,sizeof(m_ai));
    m_cbFile=0;
    m_pVideoTrak=NULL;
    m_pAudioTrak=NULL;

    return 0;
}

//return 0 on success,cbDataBytes is actual bytes return
//return -1 on fail
int CMP4Read::GetExtData(eExtData type,LPBYTE pBuf,int cbBuf,int& cbDataBytes)
{
    cbDataBytes = 0;
    if (!m_vi.bValid) {
        return -1;
    } else if (eVideoType_H264 != m_vi.video_type && eVideoType_H265 != m_vi.video_type) {
        return -1;
    }

    LPBYTE psrc = NULL;
    int cbsrc = 0;
    CMP4Read_atom_trak *pv = FindVideoTrak();
    if (!pv) {
        return -1;
    } else if (eVideoType_H264 == m_vi.video_type) {
        CMP4Read_atom_avcC *pAtom = (CMP4Read_atom_avcC*)pv->FindChildAtom("mdia.minf.stbl.stsd.avc1.avcC");
        if(!pAtom) {
            return -1;
        } else if(eExtData_sps == type) {
            psrc = pAtom->m_sps;
            cbsrc = pAtom->m_sps_count;
        } else if(eExtData_pps == type) {
            psrc = pAtom->m_pps;
            cbsrc = pAtom->m_pps_count;
        } else {
            return -1;
        }
    } else if (eVideoType_H265 == m_vi.video_type) {
        CMP4Read_atom_hvcC *pAtom = (CMP4Read_atom_hvcC*)pv->FindChildAtom("mdia.minf.stbl.stsd.hvc1.hvcC");
        if(!pAtom) {
            return -1;
        } else if(eExtData_sps == type) {
            psrc = pAtom->m_sps;
            cbsrc = pAtom->m_sps_count;
        } else if(eExtData_pps == type) {
            psrc = pAtom->m_pps;
            cbsrc = pAtom->m_pps_count;
        } else if(eExtData_vps == type) {
            psrc = pAtom->m_vps;
            cbsrc = pAtom->m_vps_count;
        }
    }

    cbDataBytes = cbsrc;
    if (psrc && cbsrc) {
        if (cbBuf >= cbsrc) {
            memcpy(pBuf,psrc,cbsrc);
            return 0;
        } else {
            cbBuf = cbsrc;
            return -1;
        }
    }

    return -1;
}

//检查是否为有效的mp4文件
int CMP4Read::CheckValid()
{
    IReadFile *hFile = m_iFile;
    if(!hFile) {
        return -1;
    }

    m_cbFile = hFile->GetFileSize();
    if(m_cbFile<=0) {
        return -1;
    }

    //下面通过atom size粗略判断mp4是否有效
    long pos_moov=-1;
    //int  cb_moov_next=0;
    int ret = -1;
    hFile->fseek(0,SEEK_SET);
    do {
        long start = hFile->ftell();
        BYTE buf[8] = {0};
        ret = hFile->fread(buf,1,sizeof(buf));
        if(ret!=sizeof(buf)) {
            break;
        }

        long *pAtom = (long*)buf;
        DWORD32 dwAtomSize = ntohl(*pAtom);
        long next = start + dwAtomSize;
        const char *pszAtomType = (char*)pAtom+4;
        if (dwAtomSize < 8 || (dwAtomSize == 0xFFFFFFFF)) {
#ifdef _MSC_VER
            DT("[%.4s] start=%d(0x%x),dwAtomSize=%d(0x%x),next=%d(0x%x)",
               pszAtomType,
               start,start,
               dwAtomSize,dwAtomSize,
               next,next
              );
            DW("invalid atom size");
#endif
            return -1;
        }

        if(strncmp(pszAtomType,"moov",4)==0) {
            pos_moov=start;
            //cb_moov_next=next;
        }

        if(next>(long)m_cbFile) {
            break;
        } else if(next==(long)m_cbFile) {
            ret=0;
            break;
        }

        if(strncmp(pszAtomType,"mdat",4)==0) {
            ret=0;
            break;
        }
        //DT("next=%d",next);
        hFile->fseek(next,SEEK_SET);
    } while(1);

    if(ret || pos_moov==-1) {
        return -1;
    }

    long pos = hFile->ftell();
    hFile->fseek(pos,SEEK_SET);
    return ret;
}

int CMP4Read::ParseAtom()
{
    //DT("CMP4Read::ParseAtom()");

    m_iFile->fseek(0,SEEK_SET);
    ASSERT(!m_pRootAtom);

    m_pRootAtom = CMP4Read_atom::CreateAtom(NULL);
    m_pRootAtom->SetFile(m_iFile);
    m_pRootAtom->SetStart(0);
    m_pRootAtom->SetSize(m_cbFile);
    m_pRootAtom->SetEnd(m_cbFile);

    m_pRootAtom->Read();
    return 0;
}

void CMP4Read::DumpAtom()
{
    if(m_pRootAtom) {
        m_pRootAtom->Dump();
    }
}

int CMP4Read::GetTrakCount()
{
    if(!m_pRootAtom) {
        ASSERT(FALSE);
        return 0;
    }

    int nc=0;
    for(int i=0; i<100; i++) { //should be enough
        char szItem[64];
        snprintf(szItem,sizeof(szItem)-1,"moov.trak[%d]",i);
        CMP4Read_atom_trak * pAtom = (CMP4Read_atom_trak*)m_pRootAtom->FindChildAtom(szItem);
        if(!pAtom) {
            break;
        }

        nc++;
    }

    return nc;
}

CMP4Read_atom_trak *CMP4Read::FindVideoTrak()
{
    if(!m_pRootAtom) {
        ASSERT(FALSE);
        return 0;
    }

    for(int i=0; i<2; i++) {
        char szItem[64];
        snprintf(szItem,sizeof(szItem)-1,"moov.trak[%d]",i);
        CMP4Read_atom_trak * pAtom = (CMP4Read_atom_trak*)m_pRootAtom->FindChildAtom(szItem);
        if(pAtom && pAtom->IsVideoTrak()) {
            return pAtom;
        }
    }

    return NULL;
}

CMP4Read_atom_trak *CMP4Read::FindAudioTrak()
{
    if(!m_pRootAtom) {
        ASSERT(FALSE);
        return 0;
    }

    for(int i=0; i<2; i++) {
        char szItem[64];
        snprintf(szItem,sizeof(szItem)-1,"moov.trak[%d]",i);

        CMP4Read_atom_trak * pAtom = (CMP4Read_atom_trak*)m_pRootAtom->FindChildAtom(szItem);
        if(pAtom && !pAtom->IsVideoTrak()) {
            return pAtom;
        }
    }

    return NULL;
}

int CMP4Read::GetAudioInfo(tagMP4AudioInfo& ai)
{
    memset(&ai,0,sizeof(ai));
    if(m_ai.bValid) {
        memcpy(&ai,&m_ai,sizeof(ai));
    }

    return m_ai.bValid?0:-1;
}

int CMP4Read::GetVideoInfo(tagMP4VideoInfo& vi)
{
    memset(&vi,0,sizeof(vi));
    if(m_vi.bValid) {
        memcpy(&vi,&m_vi,sizeof(vi));
    }

    return m_vi.bValid?0:-1;
}

int CMP4Read::ReadAudioFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration)
{
    if(m_pAudioTrak) {
        int ret = m_pAudioTrak->ReadAudioFrame(pFrame,cbFrame,cbRead,duration);
        return ret;
    }

    return -1;
}

int CMP4Read::ReadVideoFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration, int skip)
{
    if(m_pVideoTrak) {
        int ret = m_pVideoTrak->ReadVideoFrame(pFrame,cbFrame,cbRead,duration,skip);
        if(ret==0) {
#ifdef _DEBUG
            /*
            {
                static idx=0;
                DT("video frame [%d],size=%d",idx++,cbRead);
            }

            DT("ReadVideoFrame cbRead=%d(0x%x),hex=0x%02x%02x%02x%02x,0x%02x%02x%02x%02x",
                cbRead,
                cbRead,
                pFrame[0],pFrame[1],pFrame[2],pFrame[3],
                pFrame[4],pFrame[5],pFrame[6],pFrame[7]
                );
            if(0 && cbRead>=10*1024)
            {
                static int idx=0;
                CString szFile;
                szFile.Format("c:\\save1\\xwp\\%03d.bin",idx++);
                CFile file;
                file.Open(szFile,CFile::modeCreate | CFile::modeWrite);
                file.Write(pFrame,cbRead);
            }
            //*/
#endif
        }
        return ret;
    }

    ASSERT(FALSE);
    return -1;
}

//返回上一次ReadVideoFrame读取的帧下标,此帧称为当前帧
int CMP4Read::GetCurVideoFrame()
{
    if(m_pVideoTrak) {
        return m_pVideoTrak->GetCurFrame();
    }

    ASSERT(FALSE);
    return -1;
}

//seek到指定的video帧下标
int CMP4Read::SeekToFrame(int nVideoFrame)
{
    if(m_pVideoTrak) {
        if(nVideoFrame<0 || nVideoFrame >= m_pVideoTrak->GetFrameCount()) {
            return -1;
        }

        //得到第nVideoFrame帧的duration,然后seek
        DWORD32 duration = m_pVideoTrak->GetDurationSum(nVideoFrame);
        return Seek(duration);
    }

    return -1;
}

//seek到指定的video帧下标
int CMP4Read::SeekToDuration(unsigned int duration)
{
    if(m_pVideoTrak) {
        if (duration > m_vi.duration) {
            // 超过视频 duration 范围，默认为最大 duration
            duration = m_vi.duration-1;
        }
        return Seek(duration);
    }

    return -1;
}


//定位到从当前帧开始(包括当前帧)的遇到的第一个I帧
int CMP4Read::SeekToIFrame()
{
    if(!m_pVideoTrak) {
        return -1;
    }

    int i = GetCurVideoFrame();
    int nc = m_vi.frame_count;
    while(i<nc) {
        if(IsIFrame(i)) {
            return SeekToFrame(i);
        }

        i++;
    }

    return -1;
}

//从当前帧开始(不包括当前帧)向前搜索,定位到遇到的第一个I帧
//用于做帧退播放(倒退时只播放I帧)
int CMP4Read::SeekToPrevIFrame()
{
    int nCurFrame = GetCurVideoFrame();
    for(int i=nCurFrame-1; i>=0; i--) {
        if(IsIFrame(i)) {
            SeekToFrame(i);
            return 0;
        }
    }

    return -1;
}

BOOL32 CMP4Read::IsIFrame(int nVideoFrameIndex)
{
    if(!m_pVideoTrak) {
        ASSERT(FALSE);
        return FALSE;
    }

    if(!m_vi.bValid || nVideoFrameIndex < 0 || nVideoFrameIndex >= (int)m_vi.frame_count) {
        return FALSE;
    }

    return m_pVideoTrak->IsIFrame(nVideoFrameIndex);
}

//函数参数:
//      trakType            要处理的trak的类型
//      currMp4SampleNo     当前的mp4中的处理的sample对应的索引号
//      currMp4SampleSum    当前的mp4中的总的帧数
//
int CMP4Read::GetIndexSplNum(char trakType, int& currMp4SampleIndex, int& currMp4SampleSum)
{
    //DBG_INFO("\tCall %s.\n", __FUNCTION__);

    //index     = m_pVideoTrak->m_index;
    //printf("\t(%d):     index = %d.\n", __LINE__, index);
    //sampleNum = m_pVideoTrak->m_sampleCnt;
    //printf("\t(%d):     sampleNum = %d.\n", __LINE__, sampleNum);

    if (0 == trakType) {
        //处理视频trak
        currMp4SampleIndex          = m_pVideoTrak->GetCurFrame();
        currMp4SampleSum            = m_pVideoTrak->GetFrameCount();
    } else if (1 == trakType) {
        //处理音频trak
        currMp4SampleIndex          = m_pAudioTrak->GetCurFrame();
        currMp4SampleSum            = m_pAudioTrak->GetFrameCount();
    } else {
        printf("\t(%s-%d):	Find a exception, trakType = %d!!!\n", __FILE__,__LINE__,trakType);
        return -1;
    }

    //DBG_INFO("\tOut %s.\n", __FUNCTION__);
    return 0;
}

//seek时以video duration为准
int CMP4Read::Seek(DWORD32 durationVideo)
{
    int ret=0;

    if(m_pVideoTrak) {
        ret |= m_pVideoTrak->Seek(durationVideo);
        if(m_pAudioTrak) {
            //转换成audio trak中的duration
            DWORD32 durationAudio = 0;
            if (m_ai.type == RTP_TYPE_AAC) {
                durationAudio = (DWORD32)(16000.0*durationVideo/90000);
            } else {
                durationAudio = (DWORD32)(8000.0*durationVideo/90000);
            }
            ret |= m_pAudioTrak->Seek(durationAudio);
        }
    }

    return ret;
}

DWORD32 CMP4Read::GetCurVideoDuration()
{
    ASSERT(m_pVideoTrak);
    return m_pVideoTrak->GetCurDuration();
}

DWORD32 CMP4Read::GetCurAudioDuration()
{
    ASSERT(m_pAudioTrak);
    return m_pAudioTrak->GetCurDuration();
}

CMP4Read_atom::CMP4Read_atom()
{
    m_start=0;
    m_size=0;
    m_end=0;
    m_hFile=0;
    memset(m_type,0,sizeof(m_type));
    m_bHasChildAtom=FALSE;
    m_depth=0;
}

CMP4Read_atom::~CMP4Read_atom()
{
    {
        for(UINT i=0; i<m_arrChildAtom.size(); i++) {
            CMP4Read_atom *pAtom = m_arrChildAtom[i];
            if (NULL != pAtom){
                delete pAtom;
                m_arrChildAtom[i] = NULL;
            }
        }
        m_arrChildAtom.clear();
    }
}

CMP4Read_atom* CMP4Read_atom::CreateAtom(const char* type)
{
    CMP4Read_atom* pAtom = NULL;

    if(type) {
        if(0) {
        } else if(!strcmp(type,"trak"))
            pAtom = new CMP4Read_atom_trak;
        else if(!strcmp(type,"tkhd"))
            pAtom = new CMP4Read_atom_tkhd;
        else if(!strcmp(type,"hdlr"))
            pAtom = new CMP4Read_atom_hdlr;
        else if(!strcmp(type,"mdhd"))
            pAtom = new CMP4Read_atom_mdhd;
        else if(!strcmp(type,"stsd"))
            pAtom = new CMP4Read_atom_stsd;
        else if(!strcmp(type,"avc1"))
            pAtom = new CMP4Read_atom_avc1;
        else if(!strcmp(type,"avcC"))
            pAtom = new CMP4Read_atom_avcC;
        else if(!strcmp(type,"hvc1"))
            pAtom = new CMP4Read_atom_hvc1;
        else if(!strcmp(type,"hvcC"))
            pAtom = new CMP4Read_atom_hvcC;
        else if(!strcmp(type,"mp4v"))
            pAtom = new CMP4Read_atom_mp4v;
        else if(!strcmp(type,"stsc"))
            pAtom = new CMP4Read_atom_stsc;
        else if(!strcmp(type,"stsz"))
            pAtom = new CMP4Read_atom_stsz;
        else if(!strcmp(type,"stco"))
            pAtom = new CMP4Read_atom_stco;
        else if(!strcmp(type,"stss"))
            pAtom = new CMP4Read_atom_stss;
        else if(!strcmp(type,"stts"))
            pAtom = new CMP4Read_atom_stts;
    }

    if(!pAtom) {
        pAtom = new CMP4Read_atom;
    }

	
    if(type) {
        strncpy(pAtom->m_type,type,sizeof(pAtom->m_type)-1);
    }

    if( !type
        || !strcmp(type,"moov")
        || !strcmp(type,"trak")
        || !strcmp(type,"mdia")
        || !strcmp(type,"minf")
        || !strcmp(type,"stbl")
        || !strcmp(type,"stsd")
        || !strcmp(type,"mp4v")
        || !strcmp(type,"avc1")
        || !strcmp(type,"hvc1")
      ) {
        pAtom->m_bHasChildAtom=TRUE;
    }

    return pAtom;
}

int is_none_type(const char* type)
{
    int ret = 0;
    if(type) {
        if((!strcmp(type,"ftyp")) || (!strcmp(type,"JCOX")) || (!strcmp(type,"moov")) || (!strcmp(type,"mvhd")) || 
            (!strcmp(type,"trak")) || (!strcmp(type,"tkhd")) || (!strcmp(type,"mdia")) || (!strcmp(type,"mdhd")) || 
            (!strcmp(type,"hdlr")) || (!strcmp(type,"minf")) || (!strcmp(type,"vmhd")) || (!strcmp(type,"dinf")) || 
            (!strcmp(type,"stbl")) || (!strcmp(type,"stsd")) || (!strcmp(type,"hvc1")) || (!strcmp(type,"hvcC")) || 
            (!strcmp(type,"stts")) || (!strcmp(type,"stsc")) || (!strcmp(type,"stsz")) || (!strcmp(type,"stco")) || 
            (!strcmp(type,"stss")) || (!strcmp(type,"smhd")) || (!strcmp(type,"ulaw")) || (!strcmp(type,"mdat")) ||
            (!strcmp(type,"alaw")) || (!strcmp(type,"samr")) || (!strcmp(type,"mp4v")) || (!strcmp(type,"avc1")) ||
            (!strcmp(type,"mjpa")) || (!strcmp(type,"dref")) || (!strcmp(type,"avcC")) || (!strcmp(type,"esds")) ||
            (!strcmp(type,"JCOK")) || (!strcmp(type,"JCOR")) || (!strcmp(type,"mp4a"))) {
                ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = 1;
    }
    return ret;
}

void CMP4Read_atom::Read()
{
    BYTE buf[8];
    char type[8];
    if(!m_bHasChildAtom) {
        return;
    }

    while(1) {
        DWORD32 start = m_hFile->ftell();
        if(start>=m_end) {
            //DW("reach end,start=%d,end=%d",start,m_end);
            break;
        }

        int ret = m_hFile->fread(buf,1,sizeof(buf));
        if(ret!=sizeof(buf)) {
            //DW("fread fail,ret=%d",ret);
            break;
        }

        LPDWORD pSize = (LPDWORD)buf;
        DWORD32 size = ntohl(*pSize);
        DWORD32 end  = start + size;

        type[4]=0;
        strncpy(type,(char*)buf+4,4);

        CMP4Read_atom *pAtom = CMP4Read_atom::CreateAtom(type);

        ASSERT(pAtom);
        //DT("[%s]",type);

        pAtom->m_depth=m_depth+1;
        pAtom->SetFile(m_hFile);
        pAtom->SetStart(start);
        pAtom->SetSize(size);
        pAtom->SetEnd(end);
        pAtom->Read();

        m_hFile->fseek(end,SEEK_SET);
        m_arrChildAtom.push_back(pAtom);

        if(is_none_type(type)) {
            //DW("is_none_type break\n");
            break;
        }
        
        if(size == 0) {
            break;
        }
    }
}

void CMP4Read_atom::Dump()
{
#ifdef _MSC_VER
    if(m_type[0]==0) {
        DT("root");
    } else {
        char szText[1024];
        szText[0]=0;

        for(UINT i=0; i<m_depth; i++) {
            strcat(szText,"|   ");
        }

        char szType[1024];
        strcpy(szType,szText);
        strcat(szType,(char*)m_type);
        //DT("%s",szType);
    }

    for(int i=0; i<m_arrChildAtom.size(); i++) {
        m_arrChildAtom[i]->Dump();
    }
#endif
}

void CMP4Read_atom_stsd::Read()
{
    BYTE buf[8];
    m_hFile->fread(buf,1,sizeof(buf));//skip version and flags
    CMP4Read_atom::Read();
}

void CMP4Read_atom_avc1::Read()
{
    BYTE buf[8];
    m_hFile->fread(buf,1,sizeof(buf));//skip version and flags
    {
        //skip un-need data
        int size=0;
        size += 4;//addWord(0x00000000); // Version+Revision level
        size += 4;//add4ByteString("appl"); // Vendor
        size += 4;//addWord(0x00000000); // Temporal quality
        size += 4;//addWord(0x00000000); // Spatial quality
        //unsigned const widthAndHeight       = (m_szVideo.cx<<16)|m_szVideo.cy;
        size += 4;//addWord(widthAndHeight); // Width+height
        size += 4;//addWord(0x00480000); // Horizontal resolution
        size += 4;//addWord(0x00480000); // Vertical resolution
        size += 4;//addWord(0x00000000); // Data size
        size += 4;//addWord(0x00010548); // Frame       count+Compressor name (start)
        // "H.264"
        size += 4;//addWord(0x2e323634); // Compressor name (continued)
        size += 4*6;//addZeroWords(6); // Compressor name (continued - zero)
        size += 4;//addWord(0x00000018); // Compressor name (final)+Depth
        size += 2;//addHalfWord(0xffff); // Color       table id
        for(int i=0; i<size; i++) {
            m_hFile->fread(buf,1,1);//skip
        }
    }
    CMP4Read_atom::Read();
}

void CMP4Read_atom_avcC::Empty()
{
    if(m_pps) {
        delete []m_pps;
        m_pps=NULL;
        m_pps_count=0;
    }
    if(m_sps) {
        delete []m_sps;
        m_sps=NULL;
        m_sps_count=0;
    }
}

void CMP4Read_atom_avcC::Read()
{
    Empty();

    BYTE buf[8];
    {
        //skip un-need data
        int size=0;
        size += 1;//addByte(0x01); // configuration version
        size += 1;//addByte(sps_data[1]); // profile
        size += 1;//addByte(sps_data[2]); // profile compat
        size += 1;//addByte(sps_data[3]); // level
        size += 1;//addByte(0xff); /* 0b11111100 | lengthsize = 0x11 */
        m_hFile->fseek(size,SEEK_CUR);
        m_hFile->fread(buf,1,1);
        ASSERT(buf[0]==0xe0 || buf[0]==0xe1);
        if(buf[0]&0xe0) { //has sps
            m_hFile->fread(buf,1,2);
            int sps_count=((buf[0]<<8)| buf[1]);
            if(sps_count) {
                m_sps_count=sps_count;
                m_sps = new BYTE[m_sps_count];
                m_hFile->fread(m_sps,1,m_sps_count);
            }
        }
        m_hFile->fread(buf,1,1);
        if(buf[0]==0x01) {
            m_hFile->fread(buf,1,2);
            int pps_count=((buf[0]<<8)| buf[1]);
            if(pps_count>0) {
                m_pps_count=pps_count;
                m_pps = new BYTE[m_pps_count];
                m_hFile->fread(m_pps,1,m_pps_count);
            }
        }
    }

    CMP4Read_atom::Read();
}

void CMP4Read_atom_hvc1::Read()
{
    BYTE buf[8];
    m_hFile->fread(buf,1,sizeof(buf));//skip version and flags
    {
        //skip un-need data
        int size=0;
        size += 4;//addWord(0x00000000); // Version+Revision level
        size += 4;//add4ByteString("appl"); // Vendor
        size += 4;//addWord(0x00000000); // Temporal quality
        size += 4;//addWord(0x00000000); // Spatial quality
        //unsigned const widthAndHeight       = (m_szVideo.cx<<16)|m_szVideo.cy;
        size += 4;//addWord(widthAndHeight); // Width+height
        size += 4;//addWord(0x00480000); // Horizontal resolution
        size += 4;//addWord(0x00480000); // Vertical resolution
        size += 4;//addWord(0x00000000); // Data size
        size += 4;//addWord(0x00010548); // Frame       count+Compressor name (start)
        // "H.265"
        size += 4;//addWord(0x2e323635); // Compressor name (continued)
        size += 4*6;//addZeroWords(6); // Compressor name (continued - zero)
        size += 4;//addWord(0x00000018); // Compressor name (final)+Depth
        size += 2;//addHalfWord(0xffff); // Color       table id
        for(int i=0; i<size; i++) {
            m_hFile->fread(buf,1,1);//skip
        }
    }
    CMP4Read_atom::Read();
}

void CMP4Read_atom_hvcC::Empty()
{
    if(m_pps) {
        delete []m_pps;
        m_pps=NULL;
        m_pps_count=0;
    }
    if(m_sps) {
        delete []m_sps;
        m_sps=NULL;
        m_sps_count=0;
    }
    if(m_vps) {
        delete []m_vps;
        m_vps=NULL;
        m_vps_count=0;
    }
}

void CMP4Read_atom_hvcC::Read()
{
    Empty();

    BYTE buf[8];
    //skip un-need data
    int size=0;
    size += 1;  // addByte(hvcc->configurationVersion); // 1
    size += 1;  // addByte(hvcc->general_profile_space << 6 | hvcc->general_tier_flag << 5 | hvcc->general_profile_idc)
    size += 4;  // addWord(hvcc->general_profile_compatibility_flags)  // 0xffffffff
    size += 4;  // addWord(hvcc->general_constraint_indicator_flags >> 16)  // 0xffffffffffff
    size += 2;  // addHalfWord(hvcc->general_constraint_indicator_flags)    // 0xffffffffffff
    size += 1;  // addByte(hvcc->general_level_idc);
    size += 2;  // addHalfWord(hvcc->min_spatial_segmentation_idc | 0xf000);
    size += 1;  // addByte(hvcc->parallelismType | 0xfc);
    size += 1;  // addByte(hvcc->chromaFormat | 0xfc);
    size += 1;  // addByte(hvcc->bitDepthLumaMinus8 | 0xf8);
    size += 1;  // addByte(hvcc->bitDepthChromaMinus8 | 0xf8);
    size += 2;  // addHalfWord(hvcc->avgFrameRate);
    size += 1;  // addByte(hvcc->constantFrameRate << 6 | hvcc->numTemporalLayers << 3 | hvcc->temporalIdNested << 2 | hvcc->lengthSizeMinusOne);
    size += 1;  // addByte(3);  // vps，sps，pps共3个
    m_hFile->fseek(size, SEEK_CUR);

    // VPS
    m_hFile->fread(buf, 1, 1);
    ASSERT(buf[0] == 0xA0);
    m_hFile->fread(buf, 1, 2);  // numNalus;
    m_hFile->fread(buf, 1, 2);  // vps_count
    int vps_count =((buf[0]<<8)| buf[1]);
    if (vps_count) {
        m_vps_count = vps_count;
        m_vps = new BYTE[m_vps_count];
        m_hFile->fread(m_vps, 1, m_vps_count);
    }

    // SPS
    m_hFile->fread(buf, 1, 1);
    ASSERT(buf[0] == 0xA1);
    m_hFile->fread(buf, 1, 2);  // numNalus;
    m_hFile->fread(buf, 1, 2);  // sps_count
    int sps_count =((buf[0]<<8)| buf[1]);
    if (sps_count) {
        m_sps_count = sps_count;
        m_sps = new BYTE[m_sps_count];
        m_hFile->fread(m_sps, 1, m_sps_count);
    }

    // PPS
    m_hFile->fread(buf, 1, 1);
    ASSERT(buf[0] == 0xA2);
    m_hFile->fread(buf, 1, 2);  // numNalus;
    m_hFile->fread(buf, 1, 2);  // pps_count
    int pps_count =((buf[0]<<8)| buf[1]);
    if (pps_count) {
        m_pps_count = pps_count;
        m_pps = new BYTE[m_pps_count];
        m_hFile->fread(m_pps, 1, m_pps_count);
    }
    
    CMP4Read_atom::Read();
}

//参考long CMP4Trak::addAtom_mp4v()
void CMP4Read_atom_mp4v::Read()
{
    long pos = m_hFile->ftell();
    m_hFile->fseek(pos
                   +6  //reserved
                   +2  //data-reference-index,must be 0x0001
                   +16 //reserved
                   ,
                   SEEK_SET
                  );

    WORD width,height;
    m_hFile->fread(&width,1,sizeof(width));
    m_hFile->fread(&height,1,sizeof(width));
    m_width=ntohs(width);
    m_height=ntohs(height);

    pos = m_hFile->ftell();
    m_hFile->fseek(pos
                   +4  //horizresolution
                   +4  //vertresolution
                   +4  //reserved
                   +2  //frame_count
                   +32 //string[32] compressorname
                   +2  //unsigned int(16) depth = 0x0018;
                   +2  //int(16) pre_defined = -1;
                   ,
                   SEEK_SET
                  );

    CMP4Read_atom::Read();
}

void CMP4Read_atom_stts::Read()
{
    ASSERT(!m_stts);

    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

    DWORD32 nc;
    m_hFile->fread(&nc,1,sizeof(nc));
    m_stts_count=ntohl(nc);
    if(m_stts_count>0) {
        m_stts = new tagMP4Node_stts[m_stts_count];
        ASSERT(m_stts);
    }

    m_hFile->fread(m_stts,1,sizeof(tagMP4Node_stts)*m_stts_count);

    //ntohl
    {
        for(UINT i=0; i<m_stts_count; i++) {
            m_stts[i].SampleCount=ntohl(m_stts[i].SampleCount);
            m_stts[i].SampleDuration=ntohl(m_stts[i].SampleDuration);
        }
    }

    /*
    //dump,for test
    {
        for(UINT i=0;i<m_stts_count;i++)
        {
            DT("SampleCount,SampleDuration=[%d,%d]",m_stts[i].SampleCount,m_stts[i].SampleDuration);
        }
    }
    //*/
}

//参考long CMP4Trak::addAtom_stsc()
void CMP4Read_atom_stsc::Read()
{
    ASSERT(!m_stsc);

    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

    DWORD32 nc;
    m_hFile->fread(&nc,1,sizeof(nc));
    m_stsc_count=ntohl(nc);
    if(m_stsc_count>0) {
        m_stsc = new tagMP4Node_stsc[m_stsc_count];
        ASSERT(m_stsc);
    }

    m_hFile->fread(m_stsc,1,sizeof(tagMP4Node_stsc)*m_stsc_count);

    //ntohl
    {
        for(UINT i=0; i<m_stsc_count; i++) {
            m_stsc[i].FirstChunk=ntohl(m_stsc[i].FirstChunk);
            m_stsc[i].SamplesPerChunk=ntohl(m_stsc[i].SamplesPerChunk);
            m_stsc[i].SampleDescriptionID=ntohl(m_stsc[i].SampleDescriptionID);
        }
    }

    /*
    //dump,for test
    {
        for(UINT i=0;i<m_stsc_count;i++)
        {
            DT("FirstChunk,SamplesPerChunk=[%d,%d]",m_stsc[i].FirstChunk,m_stsc[i].SamplesPerChunk);
        }
    }
    //*/
}

//参考long CMP4Trak::addAtom_stsz()
void CMP4Read_atom_stsz::Read()
{
    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

    DWORD32 dwSize,dwCount;
    m_hFile->fread(&dwSize,1,sizeof(dwSize));
    m_hFile->fread(&dwCount,1,sizeof(dwCount));
    dwSize = ntohl(dwSize);
    dwCount= ntohl(dwCount);

    m_size=dwSize;
    m_stsz_count = dwCount;

    if(m_size == 0 && m_stsz_count>0) { //every sample has different size
        m_stsz = new DWORD32[m_stsz_count];
        ASSERT(m_stsz);
        m_hFile->fread(m_stsz,1,sizeof(DWORD32)*m_stsz_count);

        //ntohl
        {
            for(UINT i=0; i<m_stsz_count; i++) {
                m_stsz[i]=ntohl(m_stsz[i]);
                //DT("stsz[%d]=%d",i,m_stsz[i]);
            }
        }
    }
}

void CMP4Read_atom_stco::Read()
{
    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

    DWORD32 dwCount;
    m_hFile->fread(&dwCount,1,sizeof(dwCount));
    dwCount= ntohl(dwCount);

    m_stco_count = dwCount;
    if(m_stco_count>0) {
        m_stco = new DWORD32[m_stco_count];
        ASSERT(m_stco);
        m_hFile->fread(m_stco,1,sizeof(DWORD32)*m_stco_count);

        //ntohl
        {
            for(UINT i=0; i<m_stco_count; i++) {
                m_stco[i]=ntohl(m_stco[i]);
                //DT("stco[%d]=%d",i,m_stco[i]);
            }
        }
    }
}

void CMP4Read_atom_stss::Read()
{
    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos+4,SEEK_SET);//skip version + flags

    DWORD32 dwCount;
    m_hFile->fread(&dwCount,1,sizeof(dwCount));
    dwCount= ntohl(dwCount);

    m_stss_count = dwCount;
    if(m_stss_count>0) {
        m_stss = new DWORD32[m_stss_count];
        ASSERT(m_stss);
        m_hFile->fread(m_stss,1,sizeof(DWORD32)*m_stss_count);

        //ntohl
        {
            for(UINT i=0; i<m_stss_count; i++) {
                m_stss[i]=ntohl(m_stss[i]);
                //DT("stss[%d]=%d",i,m_stss[i]);
            }
        }
    }
}

void CMP4Read_atom_mdhd::Read()
{
    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos
                   +4      //version + flags
                   +4      //create time
                   +4      //modify time
                   ,SEEK_SET);
    DWORD32 timescale,duration;
    m_hFile->fread(&timescale,1,sizeof(timescale));
    m_hFile->fread(&duration,1,sizeof(duration));
    m_timescale=ntohl(timescale);
    m_duration=ntohl(duration);
}

void CMP4Read_atom_hdlr::Read()
{
    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos
                   +4      //version + flags
                   +4      //reserved
                   ,SEEK_SET);

    m_hFile->fread(m_handlerType,1,sizeof(m_handlerType));
    m_handlerType[4]=0;
}

void CMP4Read_atom_tkhd::Read()
{
    DWORD32 pos = m_hFile->ftell();
    m_hFile->fseek(pos
                   +4      //version + flags
                   +4      //Creation time
                   +4      //Modification time
                   +4      //trak id
                   +4      //reserved
                   ,SEEK_SET);

    DWORD32 duration;//+4       //duration
    m_hFile->fread(&duration,1,sizeof(duration));
    m_duration=ntohl(duration);

    pos = m_hFile->ftell();
    m_hFile->fseek(pos
                   +3*4    //Reserved+Layer+Alternate grp
                   +4      // Volume + Reserved
                   +4      // matrix top left corner
                   +3*4    // matrix
                   +4      // matrix center
                   +3*4    // matrix
                   +4      // matrix bottom right corner
                   ,SEEK_SET);

    DWORD32 w,h;
    m_hFile->fread(&w,1,sizeof(w));
    m_hFile->fread(&h,1,sizeof(h));

    m_width=(ntohl(w)>>16);
    m_height=(ntohl(h)>>16);
}

//搜索子atom,支持多层,多层时用.隔开,比如type为moov.trak.mdia.minf.stbl
//当子atom中有多个相同的type时,可以用[]表示下标
//moov.trak[0].mdia.hdlr
//moov.trak[1].mdia.hdlr
CMP4Read_atom * CMP4Read_atom::FindChildAtom(const char *type)
{
    CMP4Read_atom *pAtom = NULL;

    char szBuf[1024];//should be enough
    memset(szBuf,0,sizeof(szBuf));
    strncpy(szBuf,type,sizeof(szBuf)-1);
    char *pszSave=szBuf;
    char *psz = pszSave;
    while(psz && *psz) {
        int nIndex=0;
        char* period = strstr(psz,".");
        if(period) {
            *period=0;
        }

        char *pszIndex=strstr(psz,"[");
        if(pszIndex) {
            nIndex=atoi(pszIndex+1);
            *pszIndex=0;
        }

        if(pAtom)
            pAtom = pAtom->FindChildAtomHelper(psz,nIndex);
        else
            pAtom = FindChildAtomHelper(psz,nIndex);

        if(period) {
            psz=period+1;
        } else {
            break;
        }
    }

    return pAtom;
}

//搜索直接子atom,不支持多层
//nIndex表示第nIndex个类型为type的atom,nIndex从0开始
CMP4Read_atom * CMP4Read_atom::FindChildAtomHelper(const char *type,int nIndex)
{
    ASSERT(strlen(type)==4);

    int nMatchIndex=0;
    for(UINT i=0; i<m_arrChildAtom.size(); i++) {
        CMP4Read_atom *pAtom = m_arrChildAtom[i];
        if(pAtom->m_type[0] && strcmp(pAtom->m_type,type)==0) {
            if(nIndex == nMatchIndex) {
                return pAtom;
            }

            nMatchIndex++;
        }
    }

    return NULL;
}

BOOL32 CMP4Read_atom_trak::IsVideoTrak()
{
    ASSERT(m_bInit);
    return m_bVideoTrak;
}

BOOL32 CMP4Read_atom_trak::IsMjpgTrak()
{
    return m_bMjpg;
}

BOOL32 CMP4Read_atom_trak::IsIFrame(int nVideoFrameIndex)
{
    if(!IsVideoTrak()) {
        ASSERT(FALSE);
        return TRUE;
    }

    if(nVideoFrameIndex < 0 || nVideoFrameIndex >= GetFrameCount()) {
        return FALSE;
    }

    if(!m_stss || !m_stss->m_stss || IsMjpgTrak()) {
        return TRUE;
    }

    int nc = m_stss->m_stss_count;
    DWORD32 *pidx = m_stss->m_stss;
    for(int i=0; i<nc; i++) {
        DWORD32 idx = pidx[i];
        if((int)idx == (nVideoFrameIndex+1)) {  //注意:stss里的index从1开始,nVideoFrameIndex则是从0开始的
            return TRUE;
        } else  if((int)idx > nVideoFrameIndex) {
            return FALSE;
        }
    }

    return FALSE;
}

void CMP4Read_atom_trak::Read()
{
    // 初始化新增的AAC相关成员
    m_bAAC = FALSE;
    m_aacChannels = 1;
    CMP4Read_atom::Read();

    m_bInit = TRUE;
    CMP4Read_atom_hdlr * pAtom = (CMP4Read_atom_hdlr*)FindChildAtom("mdia.hdlr");
    m_bVideoTrak  = (pAtom && pAtom->IsVideo());
    if(m_bVideoTrak) {
        CMP4Read_atom *pAtom_mjpa = FindChildAtom("mdia.minf.stbl.stsd.mjpa");
        m_bMjpg = (pAtom_mjpa != NULL);
    } else {
        CMP4Read_atom *pAtom_samr = FindChildAtom("mdia.minf.stbl.stsd.samr");
        m_bAmr = (pAtom_samr != NULL);
        
        // 添加AAC识别
        CMP4Read_atom *pAtom_mp4a = FindChildAtom("mdia.minf.stbl.stsd.mp4a");
        m_bAAC = (pAtom_mp4a != NULL);
        
        // 获取AAC的采样率和声道数
        if (m_bAAC) {
            CMP4Read_atom_mdhd *pMdhd = (CMP4Read_atom_mdhd*)FindChildAtom("mdia.mdhd");
            if (pMdhd) {
                m_aacSampleRate = pMdhd->m_timescale;
            } else {
                m_aacSampleRate = 16000; // 默认值
            }
            m_aacChannels = 1; // 默认单声道，可从mp4a atom中获取
        }
    }


    //cache items
    m_stts=(CMP4Read_atom_stts *)FindChildAtom("mdia.minf.stbl.stts");
    m_stsc=(CMP4Read_atom_stsc *)FindChildAtom("mdia.minf.stbl.stsc");
    m_stsz=(CMP4Read_atom_stsz *)FindChildAtom("mdia.minf.stbl.stsz");
    m_stco=(CMP4Read_atom_stco *)FindChildAtom("mdia.minf.stbl.stco");
    m_stss=(CMP4Read_atom_stss *)FindChildAtom("mdia.minf.stbl.stss");

    ASSERT(m_stts && m_stsc && m_stsz && m_stco);
    if(IsVideoTrak()) {
        if(!IsMjpgTrak()) {
            ASSERT(m_stss);
        }
    }

    m_dwFrameIndex=0;
}

//成功时返回0
//失败时返回-1或者当前帧字节数
//返回值ret大于0时,表示当前cbFrame buffer不够大,当前帧为ret字节
//duration为当前帧的持续时间
int CMP4Read_atom_trak::ReadVideoFrame(LPBYTE pFrame,int cbFrame,int &cbRead,DWORD32& duration,int skip)
{
    cbRead=0;
    duration=0;
    //int skipbytes = 0;

    if(!IsVideoTrak()) {
        ASSERT(FALSE);
        return -1;
    }

    if(m_dwFrameIndex<0 || m_dwFrameIndex>=m_stsz->m_stsz_count || (m_stsz->m_stsz == NULL)) {
        return 0;
    }

    int nSampleSize = m_stsz->m_stsz[m_dwFrameIndex];
    if(nSampleSize > cbFrame) {
        DW("ReadVideoFrame buf too small,nSampleSize = %d > cbFrame=%d",
           nSampleSize,
           cbFrame
          );

        return nSampleSize;
    }

    duration = GetSampleDuration(m_dwFrameIndex);

    int nChunkIndex=-1;
    int nChunkSampleIndex=-1;
    int i=m_dwFrameIndex;
    {
        int ret = GetChunkIndex(i,nChunkIndex,nChunkSampleIndex);
        ASSERT(ret==0);

        ASSERT(nChunkIndex>=0 && nChunkIndex<(int)m_stco->m_stco_count);

        long posChuck = m_stco->m_stco[nChunkIndex];
        long posSample = posChuck;
        if(nChunkSampleIndex > 0) {
            //要找的sample不是所在chuck的第一个sample
            //跳过前面的nChunkSampleIndex个sample
            if (i >= nChunkSampleIndex) {
                DW("video data is wrong, i = %d, nChunkSampleIndex = %d\n", i, nChunkSampleIndex);
                int nChuckFirstSample = i - nChunkSampleIndex;//本chuck的第一个sample index
                for(int s=0; s<nChunkSampleIndex; s++) {
                    posSample += m_stsz->m_stsz[nChuckFirstSample++];
                }
            }
        }

        /*if(skip)
            skipbytes = 4;
        else
            skipbytes = 0;*/

        m_hFile->fseek(posSample+4,SEEK_SET);
        nSampleSize -= 4;
        ret = m_hFile->fread(pFrame,1,nSampleSize);
        if(ret != nSampleSize) {
            return -1;
        }
        cbRead=nSampleSize;
    }

    m_curDuration += duration;
    m_dwFrameIndex++;

    return 0;
}

int CMP4Read_atom_trak::ReadAudioFrame(LPBYTE pFrame,int cbFrame,int &cbRead,DWORD32& duration)
{
    int nSampleSize = 0;
    
    cbRead=0;
    duration=0;

    if(IsVideoTrak()) {
        ASSERT(FALSE);
        return -1;
    }

    if(m_dwFrameIndex<0 || (m_stsz == NULL) || (m_stsz->m_size == 0 && m_stsz->m_stsz == NULL) || \
        (m_stco == NULL) || (m_stco->m_stco == NULL) || m_dwFrameIndex>=(UINT)GetAudioFrameCount()) {
        return 0;
    }

    // 获取原始帧大小
    if (m_stsz->m_size) {
        nSampleSize = m_stsz->m_size;
    } else {
        nSampleSize = m_stsz->m_stsz[m_dwFrameIndex];
    }
    
    // 计算需要的缓冲区大小（加上7字节ADTS头）
    int frameLength = nSampleSize;
    if (m_bAAC) {
        frameLength += 7;  // ADTS header
    }
    
    if(frameLength > cbFrame) {
        DW("ReadAudioFrame buf too small, requiredSize = %d > cbFrame=%d", frameLength, cbFrame);
        return frameLength;
    }

    duration = GetSampleDuration(m_dwFrameIndex);

    int nChunkIndex=-1;
    int nChunkSampleIndex=-1;
    int i=m_dwFrameIndex;

    {
        int ret = GetChunkIndex(i,nChunkIndex,nChunkSampleIndex);
        ASSERT(ret==0);

        ASSERT(nChunkIndex>=0 && nChunkIndex<(int)m_stco->m_stco_count);

        long posChuck = m_stco->m_stco[nChunkIndex];
        long posSample = posChuck;
        if(nChunkSampleIndex > 0) {
            // 要找的sample不是所在chuck的第一个sample
            // 跳过前面的nChunkSampleIndex个sample
            int nChuckFirstSample = i - nChunkSampleIndex;//本chuck的第一个sample index
            for(int s=0; s<nChunkSampleIndex; s++) {
                if(m_stsz->m_size) {
                    if(m_bAmr)
                        posSample += m_stsz->m_size;
                    else
                        posSample += (m_stsz->m_size);//ulaw
                } else {
                    posSample += m_stsz->m_stsz[nChuckFirstSample++];
                }
            }
        }

        // 如果是AAC，先写入ADTS头
        if (m_bAAC) {
            // 计算采样率索引
            int sampling_frequency_index = 0;
            switch(m_aacSampleRate) {
                case 96000: sampling_frequency_index = 0; break;
                case 88200: sampling_frequency_index = 1; break;
                case 64000: sampling_frequency_index = 2; break;
                case 48000: sampling_frequency_index = 3; break;
                case 44100: sampling_frequency_index = 4; break;
                case 32000: sampling_frequency_index = 5; break;
                case 24000: sampling_frequency_index = 6; break;
                case 22050: sampling_frequency_index = 7; break;
                case 16000: sampling_frequency_index = 8; break;
                case 12000: sampling_frequency_index = 9; break;
                case 11025: sampling_frequency_index = 10; break;
                case 8000:  sampling_frequency_index = 11; break;
                case 7350:  sampling_frequency_index = 12; break;
                default:    sampling_frequency_index = 8; break; // 默认16000
            }
            
            int channel_config = m_aacChannels;
            // 填充7字节ADTS头
            pFrame[0] = 0xFF;
            pFrame[1] = 0xF1;
            pFrame[2] = ((sampling_frequency_index << 2) & 0xFC) | ((channel_config >> 2) & 0x03);
            pFrame[3] = ((channel_config & 0x03) << 6) | ((frameLength >> 11) & 0x03);
            pFrame[4] = (frameLength >> 3) & 0xFF;
            pFrame[5] = ((frameLength & 0x07) << 5) | 0x1F;
            pFrame[6] = 0xFC;
            
            // 读取AAC原始数据到ADTS头之后
            m_hFile->fseek(posSample, SEEK_SET);
            ret = m_hFile->fread(pFrame + 7, 1, nSampleSize);
            if(ret != nSampleSize) {
                return -1;
            }
            cbRead = frameLength;
        } else {
            // 非AAC：直接读取原始数据
            m_hFile->fseek(posSample, SEEK_SET);
            ret = m_hFile->fread(pFrame, 1, nSampleSize);
            if(ret != nSampleSize) {
                return -1;
            }
            cbRead = nSampleSize;
        }
    }

    m_curDuration += duration;
    m_dwFrameIndex++;

    return 0;
}

DWORD32 CMP4Read_atom_trak::GetSampleDuration(int nSampleIndex)
{
    ASSERT(m_stts);

    int nFristSampleIndex = 0;
    for(UINT i=0; i<m_stts->m_stts_count; i++) {
        int sampleCount = m_stts->m_stts[i].SampleCount;
        if(nFristSampleIndex <= nSampleIndex && nSampleIndex < nFristSampleIndex + sampleCount) {
            return m_stts->m_stts[i].SampleDuration;
        }

        nFristSampleIndex += sampleCount;
    }

    ASSERT(FALSE);
    return 0;
}

int CMP4Read_atom_trak::Test()
{
    ASSERT(IsVideoTrak());


    return 0;
}

//从sample index得到chunk index
//nSampleIndex从0开始,必须是有效的
//返回值:
//sample所在的chunk下标:nChunkIndex,
//sample是在此chunk中的第nChunkSampleIndex个sample,需要在此chunk中跳过nChunkSampleIndex个sample后才是所找的sample数据
int CMP4Read_atom_trak::GetChunkIndex(int nSampleIndex,int& nChunkIndex,int& nChunkSampleIndex)
{
    nChunkIndex=-1;
    nChunkSampleIndex=-1;

    int nSampleSum = 0;
    for(UINT i=0; i<m_stsc->m_stsc_count; i++) {
        int nSample = m_stsc->m_stsc[i].SamplesPerChunk;
        int nSampleEntry = nSample;//本entry包含的sample个数

        if(i < m_stsc->m_stsc_count-1) {
            int nChunk = m_stsc->m_stsc[i+1].FirstChunk - m_stsc->m_stsc[i].FirstChunk;
            if(nChunk > 1) {
                //本entry包含多个chunk,其sample个数相同
                nSampleEntry = nSample * nChunk;
            }
        }

        if(nSampleIndex >= nSampleSum && nSampleIndex < nSampleSum + nSampleEntry) {
            //ok,找到了
            nChunkIndex = i + (nSampleIndex - nSampleSum)/nSample;
            nChunkSampleIndex = ((nSampleIndex - nSampleSum) % nSample);
            return 0;
        }

        nSampleSum += nSampleEntry;
    }

    DW("invalid sample index for mp4 file @%u!", m_stsc->m_stsc_count);
    ASSERT(FALSE);
    return -1;
}

int CMP4Read_atom_trak::GetFrameCount()
{
    if (m_stsz) {
        return IsVideoTrak()?GetVideoFrameCount():GetAudioFrameCount();
    } else {
        DW("m_stsz is NULL\n");
        return -1;
    }
}

//amr:每帧32byte,duration为160
//ulaw:每帧320byte,每个sample的duration为1
int CMP4Read_atom_trak::GetAudioFrameCount()
{
    ASSERT(!IsVideoTrak());

    //ASSERT(m_stsz->m_size);
    return m_bAmr?(m_stsz->m_stsz_count):m_stsz->m_stsz_count;
}

//durationSeek是要定位的sample duration之和
//在seet video trak时,CMP4Read_atom_trak不保证seek之后的第一个ReadVideoFrame得到的是I帧
int CMP4Read_atom_trak::Seek(DWORD32 durationSeek)
{
    BOOL32 bOK = FALSE;

    DWORD32 durationPrev=0;
    int nSampleIndex=0;
    for(UINT i=0; i<m_stts->m_stts_count; i++) {
        DWORD32 dwCurrent = durationPrev + m_stts->m_stts[i].SampleDuration * m_stts->m_stts[i].SampleCount;

        if(durationPrev <= durationSeek && durationSeek < dwCurrent) {
            //ok,已定位在当前entry中,如果当前entry包含多个sample,要找到最接近的sample
            int nIndex=0;
            if(m_stts->m_stts[i].SampleCount>1) {
                DWORD32 delta = durationSeek - durationPrev;
                nIndex = (delta+m_stts->m_stts[i].SampleDuration/2)/m_stts->m_stts[i].SampleDuration;//四舍五入
                nSampleIndex += nIndex;
            }

            if(IsVideoTrak()) {
                m_curDuration = durationPrev;
                m_dwFrameIndex=(DWORD32)nSampleIndex;
            } else {
                m_curDuration = durationPrev + m_stts->m_stts[i].SampleDuration;
                m_dwFrameIndex=(DWORD32)nSampleIndex + m_stts->m_stts[i].SampleCount;
            }
            if(m_dwFrameIndex>=(UINT)GetFrameCount()) {
                m_dwFrameIndex = GetFrameCount()-1;
            }

            bOK = TRUE;
            break;
        }

        durationPrev = dwCurrent;
        nSampleIndex += m_stts->m_stts[i].SampleCount;
    }

    return bOK?0:-1;
}

DWORD32 CMP4Read_atom_trak::GetCurDuration()
{
    return m_curDuration;
}

//返回上一次ReadVideoFrame读取的帧下标
int CMP4Read_atom_trak::GetCurFrame()
{
    return m_dwFrameIndex-1;
}

//返回到第nVideoFrame帧之前的总共duration
DWORD32 CMP4Read_atom_trak::GetDurationSum(int nVideoFrame)
{
    if(!IsVideoTrak()) {
        return 0;
    }

    int nSampleIndex=nVideoFrame;
    DWORD32 durationSum=0;

    ASSERT(m_stts);

    int nFristSampleIndex = 0;
    for(UINT i=0; i<m_stts->m_stts_count; i++) {
        int sampleCount = m_stts->m_stts[i].SampleCount;
        if(nFristSampleIndex <= nSampleIndex && nSampleIndex < nFristSampleIndex + sampleCount) {
            int idx = nVideoFrame - nFristSampleIndex;
            durationSum += idx * m_stts->m_stts[i].SampleDuration;
            return durationSum;
        }

        nFristSampleIndex += sampleCount;
        durationSum += sampleCount * m_stts->m_stts[i].SampleDuration;
    }

    return durationSum;
}



#ifndef MIN
#define MIN(x,y)    ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y)    ((x)>(y)?(x):(y))
#endif

int CMP4Repair::Open(const char *pszMP4File)
{
    if(m_hFile) {
        ASSERT(FALSE);
        return -1;
    }

    m_szFile = (char*)pszMP4File;
    m_hFile = fopen(pszMP4File,"r+b");
    if(!m_hFile) {
        DW("fail to open mp4 file:[%s]",pszMP4File);
        return -1;
    }

    //检查是否为异常中止的mp4录像文件
    //通过搜索JCOx来判断
    m_fs = eMP4File_Unknown;

    long cbFileSize = 0;
    {
        struct stat buffer;
        int ret = stat(pszMP4File,&buffer);
        if(ret==0) {
            cbFileSize = (long)buffer.st_size;
            m_cbFile = cbFileSize;

            //BYTE arr[1024];
            //size_t ret = fread(arr,1,sizeof(arr),m_hFile);
            //ret=0;
        }
    }

    BOOL32 bFind_mdat_atom = FALSE;
    while(1) {
        long pos = ftell(m_hFile);
        if(pos+(long)sizeof(DWORD32)>cbFileSize) {
            break;
        }

        DWORD32 dwAtomSize = 0;
        int ret = system_fread(&dwAtomSize,1,sizeof(DWORD32),m_hFile);
        ASSERT(ret == sizeof(DWORD32));

        dwAtomSize = ntohl(dwAtomSize);

        if(pos + (long)MAX(dwAtomSize,8) > cbFileSize) {
            DW("atom out of mp4 file end!");
            //m_fs = eMP4File_Unknown;
            break;
        }

        char szAtom[5];
        szAtom[4]=0;
        system_fread(szAtom,1,4,m_hFile);
        //DT("atom=[%s]",szAtom);

        if(strcmp(szAtom,"mdat")==0) {
            bFind_mdat_atom=TRUE;
            m_pos_mdat = pos;
        }
        if(strcmp(szAtom,"moov")==0) {
            m_pos_moov = pos;
        }
        if(strcmp(szAtom,"JCOK")==0) {
            m_fs = eMP4File_JCOK;
            m_pos_jcox = pos;
            m_jcox_size = dwAtomSize;
        } else if(strcmp(szAtom,"JCOX")==0) {
            m_fs = eMP4File_JCOX;
            m_pos_jcox = pos;
            m_jcox_size = dwAtomSize;
        } else if(strcmp(szAtom,"JCOR")==0) {
            m_fs = eMP4File_JCOR;
            m_pos_jcox = pos;
            m_jcox_size = dwAtomSize;
        }

        if(dwAtomSize<=8) {
            break;//invalid atom,异常中止录像时,这里一般是mdat atom
        }

        DWORD32 posNextAtom = pos+dwAtomSize;
        fseek(m_hFile,posNextAtom,SEEK_SET);
    }

    if(!bFind_mdat_atom || m_pos_mdat == 0) {
        m_fs = eMP4File_Unknown;//没有找到mdat时,无法修复此文件
    }

    return 0;
}


//CMP4Repair处理的文件极可能是非法mp4文件,所以要特别注意各种错误的处理
//开始录像时会标记为JCOX,表示x,正在录像中...,JCO表示Jabsco logo
//正常结束录像后标记改为JCOK,表示OK
//修复后标记改为JCOR,表示Repaired
CMP4Repair::CMP4Repair()
{
    m_hFile = NULL;
    Close();
}

CMP4Repair::~CMP4Repair()
{
    Close();
}

void CMP4Repair::Close()
{
    if(m_hFile) {
        fclose(m_hFile);
        m_hFile=0;
    }

    m_fs = eMP4File_Fail;
    m_pos_jcox=0;
    m_pos_mdat = 0;
    m_pos_moov=0;
    m_jcox_size=0;
    m_cbFile = 0;
    m_szFile=NULL;
}

//判断mp4文件状态
eMP4FileStatus CMP4Repair::GetMP4FileStatus()
{
    return m_fs;
}

int CMP4Repair::Repair(unsigned int *timelen)
{
    int count = 0;
    //确保文件是有效的JCOx
    if(m_hFile == NULL || (m_fs != eMP4File_JCOX && m_fs != eMP4File_JCOK  && m_fs != eMP4File_JCOR) 
//#ifdef _DEBUG
       //测试重复修复
       //&& m_fs != eMP4File_JCOK
       //&& m_fs != eMP4File_JCOR
//#endif
      ) {
        return -1;
    }

    if(!m_pos_mdat) {
        ASSERT(m_pos_mdat);
        return -1;
    }

    fseek(m_hFile,m_pos_jcox+8,SEEK_SET);//seek to the first child atom
    //提取jcox信息
    DWORD32 dwVerFlags = 0;
    system_fread(&dwVerFlags,1,sizeof(dwVerFlags),m_hFile);
    dwVerFlags=ntohl(dwVerFlags);
    
    DWORD32 dwTrakCount = 0;
    system_fread(&dwTrakCount,1,sizeof(dwTrakCount),m_hFile);
    dwTrakCount=ntohl(dwTrakCount);

    CMP4Write mp4Write;
    mp4Write.Attach(m_hFile);

    DWORD32 videoTrakId=0;
    DWORD32 audioTrakId=0;
    for(DWORD32 i=0; i<dwTrakCount; i++) {
        tag_atom_jcox_trak_v1 trak;
        tag_atom_jcox_trak_v0 trak_v0;
        if (0 == dwVerFlags) {
            system_fread(&trak_v0,1,sizeof(trak_v0),m_hFile);
            trak.trakid = trak_v0.trakid;
            memcpy(trak.handlerType, trak_v0.handlerType, sizeof(trak_v0.handlerType));
            memcpy(trak.mediaType, "H264", 4);
            trak.width = trak_v0.width;
            trak.height = trak_v0.height;
            trak.timescale = trak_v0.timescale;
            trak.fmtp_spropparametersets_len = trak_v0.fmtp_spropparametersets_len;
        } else {
            system_fread(&trak,1,sizeof(trak),m_hFile);
        }
        
        int ret = 0;
        trak.trakid=ntohl(trak.trakid);
        trak.width=ntohl(trak.width);
        trak.height=ntohl(trak.height);
        trak.timescale=ntohl(trak.timescale);
        trak.fmtp_spropparametersets_len=ntohl(trak.fmtp_spropparametersets_len);
        char *sprop_parameter_sets=NULL;
        if(trak.fmtp_spropparametersets_len>0) {
            int nPost = !memcmp(trak.mediaType, "H265", 4) ? strlen(";HEVC") : 0;
            sprop_parameter_sets = new char[trak.fmtp_spropparametersets_len + nPost + 1];
            fread(sprop_parameter_sets,1,trak.fmtp_spropparametersets_len,m_hFile);
            if (nPost) {
                memcpy(sprop_parameter_sets + trak.fmtp_spropparametersets_len, ";HEVC", nPost);
            }
            sprop_parameter_sets[trak.fmtp_spropparametersets_len+nPost]=0;
        }

        if(strncmp((char*)trak.handlerType,"vide",4)==0) {
            videoTrakId = trak.trakid;
            ret = mp4Write.InitVideo(trak.trakid,CSize(trak.width,trak.height),sprop_parameter_sets);
            ASSERT(ret==0);

        } else {
            audioTrakId = trak.trakid;
            if (strncmp((const char*)trak.mediaType, "AAC", sizeof(trak.mediaType)) == 0) {
               trak.trakid = RTP_TYPE_AAC; 
            } else {
                trak.trakid = 0; // ulaw
            }
            ret = mp4Write.InitAudio(trak.trakid);
            ASSERT(ret==0);
        }

        if(sprop_parameter_sets) {
            delete []sprop_parameter_sets;
            sprop_parameter_sets = NULL;
        }
    }

    CMP4Trak * pVideoTrak = mp4Write.FindVideoTrak();
    CMP4Trak * pAudioTrak = mp4Write.FindAudioTrak();

    {
        fseek(m_hFile,m_pos_mdat+8,SEEK_SET);
        tagMP4SampleHeader sh;
        while(1) {
            long pos = ftell(m_hFile);
            if(pos+(long)sizeof(tagMP4SampleHeader)>=m_cbFile) {
                break;
            }

            int ret = system_fread(&sh,1,sizeof(sh),m_hFile);
            ASSERT(ret == sizeof(sh));

            DWORD32 sz=sh.size;
            DWORD32 dur=sh.duration;
            sh.size = ((sz&0xFF0000)>>16) | (sz&0x00FF00) | ((sz&0xFF)<<16);
            sh.duration= ((dur&0xFF0000)>>16) | (dur&0x00FF00) | ((dur&0xFF)<<16);

            if(pos+(long)sizeof(tagMP4SampleHeader)+sh.size>=m_cbFile) {
                break;
            }

            if (sh.trakid == videoTrakId && pVideoTrak) {
                // STTS: 记录每帧时长
                pVideoTrak->OnNewDuration(sh.duration);

                // STSC: 关键修改！每个sample独立为一个chunk
                {
                    tagMP4Node_stsc* ps = new tagMP4Node_stsc;
                    ps->FirstChunk = pVideoTrak->m_lst_stco.size() + 1;  // 动态递增
                    ps->SamplesPerChunk = 1;
                    ps->SampleDescriptionID = 1;
                    pVideoTrak->m_lst_stsc.push_back(ps);
                    pVideoTrak->m_st_count[eAtomStsc]++;
                }

                // STSZ: 记录帧大小
                pVideoTrak->m_lst_stsz.push_back(sh.size);
                pVideoTrak->m_st_count[eAtomStsz]++;

                // STCO: 记录帧偏移
                pVideoTrak->m_lst_stco.push_back(pos + sizeof(tagMP4SampleHeader));
                pVideoTrak->m_st_count[eAtomStco]++;

                // STSS: 关键帧标记
                if (sh.flags & 0x01) {
                    DWORD32 idx = pVideoTrak->m_lst_stsz.size();
                    pVideoTrak->m_lst_stss.push_back(idx);
                    pVideoTrak->m_st_count[eAtomStss]++;
                }
            }
            else if (sh.trakid == audioTrakId && pAudioTrak) {
                // STTS: 记录每帧时长
                pAudioTrak->OnNewDuration(sh.duration);

                // STSC: 每个sample独立为一个chunk（关键修改），并创建独立 entry
                {
                    tagMP4Node_stsc* ps = new tagMP4Node_stsc;
                    ps->FirstChunk = pAudioTrak->m_lst_stco.size() + 1;  // 动态计算当前chunk序号
                    ps->SamplesPerChunk = 1;  // 每个chunk仅含1个sample
                    ps->SampleDescriptionID = 1;
                    pAudioTrak->m_lst_stsc.push_back(ps);
                    pAudioTrak->m_st_count[eAtomStsc]++;
                }

                // STSZ: 区分定长/变长音频
                {
                    if (pAudioTrak->m_type == RTP_TYPE_AAC) {
                        pAudioTrak->m_lst_stsz.push_back(sh.size);  // 记录变长帧大小
                        pAudioTrak->m_st_count[eAtomStsz]++;
                    } else {
                        if (pAudioTrak->m_lst_stsz.empty()) {
                            pAudioTrak->m_lst_stsz.push_back(sh.size);
                            pAudioTrak->m_st_count[eAtomStsz]++;
                        } else {
                            ASSERT(sh.size == pAudioTrak->m_lst_stsz[0]);
                        }
                    }
                    pAudioTrak->m_stsz_count++;  // 维护帧计数器
                }

                // STCO: 记录帧偏移
                pAudioTrak->m_lst_stco.push_back(pos + sizeof(tagMP4SampleHeader));
                pAudioTrak->m_st_count[eAtomStco]++;
            } else {
                //无效数据!
                break;
            }

            long posNextSample = pos + sizeof(sh)+sh.size;
            fseek(m_hFile,posNextSample,SEEK_SET);
			count ++;
			if(0 == (count % 100))
				usleep(1000);	//防止cpu过高
        }
    }

    fseek(m_hFile,0,SEEK_END);
    mp4Write.m_pos_jcok=m_pos_jcox;
    mp4Write.m_pos_mdat=m_pos_mdat;
    mp4Write.m_pos_moov=m_pos_moov;

	CMP4Trak *pTrakA = mp4Write.FindAudioTrak();
    CMP4Trak *pTrakV = mp4Write.FindVideoTrak();
    if(pTrakV && pTrakV->GetFrameCount()>0) {
		*timelen = pTrakV->m_duration / pTrakV->m_timescale;
    } else if(pTrakA && pTrakA->GetFrameCount()>0) {
		*timelen = pTrakA->m_duration / pTrakA->m_timescale;
    }
	
    mp4Write.Repair();
    mp4Write.Detach();
    return 0;
}

#endif  // DEV_TYPE

