#ifndef _MP4_DEMUX_H_XWPCOM_20091201_
#define _MP4_DEMUX_H_XWPCOM_20091201_

/*
libmp4用于mp4录像及回放
支持video格式:mpeg4/h.264/mjpg
支持audio格式:amr/ulaw
支持windows+linux平台
应用程序只需要使用CMP4Write和CMP4Read来完成mux和demux功能

录像使用CMP4Write
CMP4Write mp4Write;
mp4Write.Open(mp4OutputFileName);
	mp4OutputFileName以.mp4结尾
mp4Write.InitVideo(type,szVideo,h264_sprop_paramenter_sets);
	必须调用InitVideo
	type为video RTP的payload type,mjpg时为26,mpeg4和h.264时为97
	szVideo是视频尺寸,通常为320x240,720x576
	h264_sprop_paramenter_sets仅在h.264时才需要设置

mp4Write.InitAudio(audioType);//可选
    InitAudio是可选的,如果录像不带audio,则不需要调用.
	audioType必须是RTP_TYPE_ULAW,RTP_TYPE_MJPG,RTP_TYPE_AMR之一
	audio采样必须是8000hz

mp4Write.WriteFrame(bVideo,pFrame,cbFrame,dwRTPTimeStamp)
	当收到一个完整的audio/video frame后调用WriteFrame
	dwRTPTimeStamp为rtp包的timestamp

mp4Write.Close();
	录像完成后必须调用Close(),否则录像得到的.mp4可能无法正常播放

回放使用CMP4Read
CMP4Read mp4Read;
mp4Read.Open("xxx.mp4");
可以调用mp4Read.GetVideoInfo()和mp4Read.GetAudioInfo()得到video/audio信息
调用ReadVideoFrame和ReadAudioFrame得到下一帧AV数据
可以调用mp4Read.Seek来定位,seek的duration是以video trak为准的
回放完成后调用Close()

XiongWanPing 2009.12.01
//*/

#if __REC__

#ifdef _MSC_VER
#define snprintf	_snprintf
#define fstat		_fstat
#define stat		_stat

//使用BOOL32,DWORD32是为了兼容已有的linux代码,...
#define BOOL32 BOOL
#define DWORD32 DWORD

#else
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

//for linux platform
#ifndef ASSERT
#define ASSERT(x)	do{if(x){}else{DE("###ASSERT Fail:%s (File:[%s:%d])\n",#x,__FILE__,__LINE__);}}while(0)
#endif

typedef const char * LPCTSTR;
typedef unsigned char BOOL32;
typedef unsigned int DWORD32;
typedef unsigned int UINT;
typedef unsigned short WORD,*LPWORD;
typedef unsigned char BYTE,*LPBYTE;
typedef unsigned long *LPDWORD;
typedef unsigned long ULONG;
//typedef void VOID,*LPVOID;
typedef long long LONGLONG;

#define _strdup strdup

#define TRUE  1
#define FALSE 0

#define REC_REVERSE_SIZE 3000
#define PRE_STTS_SIZE (180*1000 + 16 + REC_REVERSE_SIZE)
#define PRE_STSC_SIZE (270*1000 + 16 + REC_REVERSE_SIZE)
#define PRE_STSZ_SIZE (90*1000 + 20 + REC_REVERSE_SIZE)
#define PRE_STCO_SIZE (90*1000 + 16 + REC_REVERSE_SIZE)
#define PRE_STSS_SIZE (3600 + 16 + REC_REVERSE_SIZE)

#define REC_ONCE_WRITE_SIZE (128*1024)  // 一次性写入大小

struct tagSize
{
	int cx;
	int cy;
};
class CSize:public tagSize
{
public:
	CSize()
	{
	}
	CSize(int x,int y)
	{
		cx=x;
		cy=y;
	}
	bool operator==(const CSize& sz)
	{
		return cx==sz.cx && cy==sz.cy;
	}
};

typedef int SOCKET;
#define INVALID_SOCKET	(-1)

#define _snprintf snprintf

#define DT	DebugTrace
#define DW	DebugTrace
#define DE	DebugTrace

void DebugTrace(LPCTSTR lpszFormat, ... );

#endif

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#include <vector>
using namespace std;

#ifndef Boolean
#define Boolean BOOL32
#endif

#ifndef COUNT_OF
#define COUNT_OF(x)	(sizeof(x)/sizeof((x)[0]))
#endif

enum eMpeg4FrameType
{
	eMpeg4_errFrame=0,
	eMpeg4_IFrame,
	eMpeg4_PFrame,
	eMpeg4_BFrame,
};

#define RTP_TYPE_ULAW	0
#define RTP_TYPE_ALAW	8
#define RTP_TYPE_MJPG	26
#define RTP_TYPE_AMR	97
#define RTP_TYPE_AAC	101



#define NAL_TYPE_SLICE      1
#define NAL_TYPE_IDR        5
#define NAL_TYPE_SEI        6
#define NAL_TYPE_SPS        7
#define NAL_TYPE_PPS        8
#define NAL_TYPE_SEQ_END    9
#define NAL_TYPE_STREAM_END 10

enum ePlaySpeed
{
	ePlaySpeed_Min=0,
	ePlaySpeed_Pause=0,	//pause
	ePlaySpeed_32,		//快放32倍
	ePlaySpeed_16,
	ePlaySpeed_8,
	ePlaySpeed_4,
	ePlaySpeed_2,
	ePlaySpeed_Normal,	//正常速度
	ePlaySpeed_1_2,
	ePlaySpeed_1_4,
	ePlaySpeed_1_8,
	ePlaySpeed_1_16,
	ePlaySpeed_1_32,	//慢放32倍
	//ePlaySpeed_SingleFramePerSecond,//帧进,每秒播放一帧
	ePlaySpeed_Max,
};

enum eDemuxState
{
	eDemuxState_READ_FRAME,		//需要读新帧
	eDemuxState_WAIT_PLAYING,	//已读取一帧,但正在等待playing的时机
};

#ifndef _MSC_VER
	#define MP4_AFX_EXT_CLASS
#elif defined _MP4_REC_TEST_
	#define MP4_AFX_EXT_CLASS
#else
	#define MP4_AFX_EXT_CLASS	AFX_EXT_CLASS	//用于支持windows dll,exe
#endif

enum eStblAtom
{
    eAtomStts = 0,
    eAtomStsc = 1,
    eAtomStsz = 2,
    eAtomStco = 3,
    eAtomStss = 4,
    eAtomNum  = 5,
};

//文件读取接口
class IReadFile
{
public:
	virtual ~IReadFile(){}
	virtual int		fseek(long offset,int origin)=0;
	virtual long	GetFileSize()=0;
	virtual long	ftell()=0;
	virtual size_t	fread( void *buffer, size_t size, size_t count)=0;
	virtual BOOL32	IsOpen()=0;
};

class CReadFile:public IReadFile
{
public:
	CReadFile()
	{
		m_hFile=NULL;
	}
	virtual ~CReadFile()
	{
		Close();
	}

	BOOL32 IsOpen()
	{
		return m_hFile != NULL;
	}

	int fseek(long offset,int origin)
	{
		ASSERT(m_hFile);
		int ret = ::fseek(m_hFile,offset,origin);
		return ret;
	}
    
    size_t fread( void *buffer, size_t size, size_t count);

    long GetFileSize()
	{
		ASSERT(m_hFile);
		
		struct stat fs;
		int ret = fstat(fileno(m_hFile), &fs);
		if(ret!=0 || fs.st_size<=0)
		{
			return 0;
		}

		return fs.st_size;
	}

	long ftell()
	{
		ASSERT(m_hFile);
		long pos = ::ftell(m_hFile);
		return pos;
	}

	void Close()
	{
		if(m_hFile)
		{
			fclose(m_hFile);
			m_hFile=NULL;
		}
	}

	static CReadFile * CreateInstance(const char *pszFile,const char *mode)
	{
		CReadFile *pFile = new CReadFile;
        pFile->m_hFile = fopen(pszFile,mode);
		if(!pFile->m_hFile)
		{
			delete pFile;
			pFile = NULL;
		}
		return pFile;
	}

protected:
	FILE *m_hFile;
};

//XiongWanPing 2009.11.05
class MP4_AFX_EXT_CLASS CMP4Base
{
public:
	static unsigned char* base64Decode(char* in, unsigned& resultSize,
			    Boolean trimTrailingZeros = TRUE);
    // returns a newly allocated array - of size "resultSize" - that
    // the caller is responsible for delete[]ing.

	static char* base64Encode(char const* orig, unsigned origLength);
    // returns a 0-terminated string that
    // the caller is responsible for delete[]ing.

	CMP4Base();
	virtual ~CMP4Base();
	static int gettimeofday(struct timeval* tp);
	static const char *GetMpeg4FrameTypeDesc(LPBYTE pData);
	static eMpeg4FrameType GetMpeg4FrameType(LPBYTE pData);
	static BOOL32 IsMpeg4IFrame(LPBYTE pData)
	{
		return GetMpeg4FrameType(pData)==eMpeg4_IFrame;
	}
	
	//pData指向一个完整的h.264帧
	static BOOL32 IsH264IFrame(LPBYTE pData)
	{
		BYTE nHeadType=(pData[0]&0x1F);
		if(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x01)
		{
			nHeadType =(pData[4] & 0x1F);
		}
		
		if(nHeadType == NAL_TYPE_IDR || nHeadType == NAL_TYPE_SPS || nHeadType == NAL_TYPE_PPS)
			return TRUE;
        else if(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x01 && (pData[5] == 0x88))
            return TRUE;
		return FALSE;
	}

	static BOOL32 IsH265IFrame(LPBYTE pData)
	{
		BYTE nHeadType=((pData[0]>>1)&0x3F);
		//WORD *pw =(WORD *)pData;
		if(pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x01)//以00000001开头时要跳过4个字节
		{
			nHeadType = ((pData[4] >> 1) & 0x3F);
		}
#define NAL_UNIT_VPS 32		
#define NAL_UNIT_SPS 33		
#define NAL_UNIT_PPS 34
#define NAL_UNIT_IDR_W_RADL 19
#define NAL_UNIT_IDR_N_LP   20
		if(nHeadType == NAL_UNIT_SPS || nHeadType == NAL_UNIT_PPS || nHeadType == NAL_UNIT_VPS)
			return TRUE;
		else if(nHeadType == NAL_UNIT_IDR_W_RADL || nHeadType == NAL_UNIT_IDR_N_LP)
			return TRUE;
		
		return FALSE;
	}

protected:
	unsigned addByte(unsigned char byte);
	long addAtomHeader(char const* atomName);
	unsigned addWord(unsigned word);
	unsigned addHalfWord(unsigned short halfWord);
	unsigned addZeroWords(unsigned numWords) ;
	unsigned add4ByteString(char const* str);
	unsigned addArbitraryString(char const* str,Boolean oneByteLength);
	void setWord(unsigned filePosn, unsigned size);
	void ModifyByte(unsigned filePosn,BYTE newValue);

	FILE	*m_hFile;

    BYTE     m_write_buf[REC_ONCE_WRITE_SIZE];
    DWORD32  m_write_size;
};


struct tagRTSPInterleavedFrameHeader
{
#ifdef _DEBUG
	tagRTSPInterleavedFrameHeader()
	{
		//make sure it's 4 bytes
		ASSERT(sizeof(*this)==4);
	}
#endif

	BYTE	Magic;
	BYTE	Channel;

	//raw length is net order,so protect it by Length()
	WORD	Length()
	{
		return ntohs(length);
	}
protected:
	WORD	length;
};

struct tagRTPFrameHeader
{
#ifdef _DEBUG
	tagRTPFrameHeader()
	{
		ASSERT(sizeof(*this)==12);
	}
#endif

	//只需要支持小端
    BYTE		cc				:4;				/* CSRC count */
    BYTE		x				:1;				/* header extension flag */
    BYTE		padding			:1;				/* padding flag */
    BYTE		version			:2;				/* protocol version */
	
    /* byte 1 */		
    BYTE		payload_type    :7;				/* payload type */
    BYTE		marker			:1;				/* marker bit */

	DWORD32 timestamp()
	{
		return ntohl(Timestamp);
	}

	DWORD32 sync_sid()
	{
		return ntohl(Sync_sid);
	}

protected:

	WORD	sequence_number;
	DWORD32	Timestamp;
	DWORD32 	Sync_sid;//sync source id
};

struct tagMP4Node_stts
{
	DWORD32 SampleCount;
	DWORD32 SampleDuration;
	tagMP4Node_stts()
	{
		//memset(this,0,sizeof(*this));
		SampleCount = 0;
		SampleDuration = 0;
	}
};

struct tagMP4Node_stsc
{
	DWORD32 FirstChunk;
	DWORD32 SamplesPerChunk;
	DWORD32 SampleDescriptionID;
	tagMP4Node_stsc()
	{
		//memset(this,0,sizeof(*this));
		FirstChunk = 0;
		SamplesPerChunk = 0;
		SampleDescriptionID = 0;
	}
};


class CMP4Write;
//XiongWanPing 2009.11.04
class CMP4Trak:public CMP4Base
{
	friend class CMP4Write;
	friend class CMP4Repair;
public:
	CMP4Trak();
	virtual ~CMP4Trak();

	BOOL32 IsAudioTrak()
	{
		return strncmp(m_handlerType,"soun",4)==0;
	}

	BOOL32 IsVideoTrak()
	{
		return strncmp(m_handlerType,"vide",4)==0;
	}

	BOOL32 IsIFrame(LPBYTE pData);
	DWORD32 GetFrameCount()
	{
		if(IsAudioTrak())
			return m_stsz_count;
		return m_lst_stsz.size();
	}

	BOOL32 IsH264()
	{
		return (strlen(m_fmtp_spropparametersets) != 0) && (!IsH265());
	}
	BOOL32 IsMjpg()
	{
		return m_trakid == 26;//26是标准MJPG rtp payload type
	}
	BOOL32 IsH265()
	{
		return m_bH265;//
	}
	DWORD32 GetTimeScale()
	{
		return m_timescale;
	}

protected:
	int  PreClose();
	void OnNewDuration(DWORD32 dwDurationLastFrame);

	long addAtom_trak();
	long addAtom_tkhd();
	long addAtom_mdia();
	long addAtom_mdhd();
	long addAtom_hdlr();
	long addAtom_minf();
	long addAtom_vmhd();
	long addAtom_mp4v();
	long addAtom_esds();
	long addAtom_smhd();
	long addAtom_dinf();
	long addAtom_dref();
	long addAtom_stts();
	long addAtom_stss();
	long addAtom_stsc();
	long addAtom_stsz();
	long addAtom_stco();
	long addAtom_stsd();
	long addAtom_samr();
	long addAtom_ulaw();
    long addAtom_stbl_stsd();
	long addAtom_stbl_other();
    long addAtom_header();
    unsigned addAtom_aac_esds();
    long addAtom_aac();
	long addAtom_avc1();
	long addAtom_avcC();
	long addAtom_hvc1();
	long addAtom_hvcC();
	long addAtom_mjpa();

	int WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader);
    int WriteToBufferedFile(LPBYTE pFrame, int cbFrame);

	int OnAmrFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnAmrFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnUlawFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int OnUlawFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
    int OnAacFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	tagMP4Node_stsc* GetLastStscNode()
	{
		if(m_lst_stsc.size()>0)
		{
			return m_lst_stsc[m_lst_stsc.size()-1];
		}
		return NULL;
	}

	tagMP4Node_stts* GetLastSttsNode()
	{
		if(m_lst_stts.size()>0)
		{
			return m_lst_stts[m_lst_stts.size()-1];
		}
		return NULL;
	}

	int InitAudioTrakInfo(DWORD32 type,const char *handlerType,DWORD32 timescale=8000);
	int InitVideoTrakInfo(DWORD32 type,const char *handlerType,CSize szVideo,const char *sprop_parameter_sets=NULL,DWORD32 timescale=90000);

	void Empty();
	vector<tagMP4Node_stts*>	m_lst_stts;
	vector<tagMP4Node_stsc*>	m_lst_stsc;
	vector<DWORD32>		m_lst_stco;
	vector<DWORD32>		m_lst_stss;//只在video时有用,audio不需要stss
	vector<DWORD32>		m_lst_stsz;//audio帧尺寸一般是固定的,所以m_lst_stszAudio只有一个item
	DWORD32				m_stsz_count;//仅在audio时用到	
	DWORD32	            m_stts_addr;
    DWORD32	            m_mdhd_addr;
    DWORD32	            m_tkhd_addr;
	DWORD32				m_st_size[eAtomNum];
    DWORD32				m_st_count[eAtomNum];
    DWORD32				m_pre_st_count[eAtomNum];

    DWORD32				m_trakid;
	unsigned			m_timescale;
	unsigned			m_duration;

	CSize				m_szVideo;//只对video track生效

	char				m_handlerType[4];
	CMP4Write			*m_pMP4Write;

	DWORD32				m_dwFrameIndex;
	DWORD32				m_nFirstChunk;

	DWORD32				m_dwTickRTPLast;//保存上一帧的RTP timestamp,已经是host order
	DWORD32				m_type;//trak type

	char				m_fmtp_spropparametersets[1024];//only for h.264 trak
	DWORD32				m_dwRTPTimeStamp;
	static WORD			m_amr_packed_size[8];
	BYTE				m_amr_ft;
	BOOL32				m_bH264IFrame;

	BOOL32				m_bH265;
	BOOL32				m_bSavedIFrame;
};


//XiongWanPing 2010.01.04 begin
//mp4 jco extend box,支持修复异常中止的mp4文件.
#define MP4_FLAGS_JCOK			0x0001		//支持修复mp4文件
#define MP4_FLAGS_AUTO_FFLUSH	0x0002		//仅在MP4_FLAGS_JCOK为真时有效,每次fwrite后都马上fflush
//当采用MP4_FLAGS_JCOK标志时,可以设置MP4_FLAGS_AUTO_FFLUSH来叫CMP4Write每次写文件后都自动fflush,
//但这样效率可能比较低(磁盘IO相对缓存操作是很慢的操作)
//为提高效率,程序可以不设置MP4_FLAGS_AUTO_FFLUSH,而是定时(比如每隔5秒或者每写10帧视频)自行调用CMP4Write::fflush
//注意:MP4_FLAGS_JCOK为假时设置MP4_FLAGS_AUTO_FFLUSH无意义,CMP4Write会自动启用MP4_FLAGS_JCOK
#ifdef _MSC_VER
#pragma pack(push,1)
#endif

struct tagMP4SampleHeader
{
#ifdef _DEBUG
	tagMP4SampleHeader()
	{
		int cbSize = sizeof(tagMP4SampleHeader);
		ASSERT(cbSize == 8);
	}
#endif
	
	DWORD32	trakid:8;
	DWORD32	size:24;
	DWORD32 flags:8;	//bit0:keyframe
	DWORD32 duration:24;
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif
//XiongWanPing 2010.01.04 end

class MP4_AFX_EXT_CLASS CMP4Write:public CMP4Base
{
	friend class CMP4Trak;
	friend class CMP4Repair;
public:
	int InitVideo(DWORD32 dwType,CSize szVideo,const char* sprop_parameter_sets=NULL);
	int InitAudio(DWORD32 dwType);

	CMP4Write();
	virtual ~CMP4Write();

	int Attach(FILE *hFile);//Attach和Detach仅供修复mp4使用
	void Detach();

	int Open(const char * pszFile,DWORD32 dwFlags=MP4_FLAGS_JCOK);//dwFlags为MP4_FLAGS_xxx
	int Close();
    int Pause();
    int IsOpen();
    int Sync();
    int ReInitTrak();
	//write audio amr时,有两种情况:写rtp封装的多个amr,或者独立的amr包
	//rtp封装包用WriteFrame
	//独立的amr包用WriteFrameEx
	//video没有此区别.
	int WriteFrame(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteFrameEx(BOOL32 bVideo,LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);

	//					获取当前文件尺寸
	BOOL32 GetFileSize(LONGLONG* lpFileSize);
	DWORD32 GetRecordTick();

	int fflush();
	DWORD32 GetMP4Flags() const
	{
		return m_dwMP4Flags;
	}

protected:
	int WriteAudioFrameHelper(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp,BOOL32 bRTPHeader);
	int WriteVideoFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrame(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);
	int WriteAudioFrameEx(LPBYTE pFrame,int cbFrame,DWORD32 dwRTPTimeStamp);

	int WriteHeader();
    long Write_mvhd_header();
	BOOL32 m_bHasWriteFtyp;

	CMP4Trak * FindVideoTrak();
	CMP4Trak * FindAudioTrak();

	void Empty();
	long addAtom_ftyp();
	long addAtom_JCOK();
	long addAtom_moov();
	long addAtom_mvhd();

	long				m_pos_mdat;
	long				m_pos_jcok;
	long				m_pos_moov;
    long                m_pos_mvhd;

	vector<CMP4Trak*>	m_arrTrak;
	CMP4Trak			*m_pTrakLastWrite;//上一次writefrmae的trak
	DWORD32				m_dwTickStartRecord;

#define MP4F_ULAW	0x0001	//通常使用的g.711是ulaw格式,又称pcmu,当audio为ulaw时,要做一些特殊处理
#define MP4F_ATTACH	0x0002	//修复mp4
	DWORD32				m_dwFlags;
	DWORD32				m_dwMP4Flags;

	int Repair();
	BOOL32 IsRepairMode()
	{
		return m_dwFlags & MP4F_ATTACH;
	}
};

struct tagMP4AudioInfo
{
	BOOL32 bValid;
	int type;//ulaw,amr
	DWORD32 duration;
	DWORD32 frame_count;

	tagMP4AudioInfo()
	{
		//memset(this,0,sizeof(*this));
		bValid = 0;
		type = 0;
		duration = 0;
		frame_count = 0;
	}
};

enum eVideoType
{
	eVideoType_None=0,
	eVideoType_Mpeg4=1,
	eVideoType_H264=2,
	eVideoType_Mjpg=3,
	eVideoType_H265=4,
};

struct tagMP4VideoInfo
{
	BOOL32 bValid;
	int type;//mpeg4,h.264
	int width;
	int height;
	double fps;
	DWORD32 timescale;
	DWORD32 duration;
	DWORD32 frame_count;
	eVideoType video_type;

	tagMP4VideoInfo()
	{
		//memset(this,0,sizeof(*this));
		bValid = 0;
		type = 0;
		width = 0;
		height = 0;
		fps = 0.0;
		timescale = 0;
		duration = 0;
		frame_count = 0;
		video_type = eVideoType_None;
	}
};

//XiongWanPing 2009.11.09
class MP4_AFX_EXT_CLASS CMP4Read_atom
{
	friend class CMP4Read;
public:
	CMP4Read_atom();
	virtual ~CMP4Read_atom();

	static CMP4Read_atom* CreateAtom(const char* type);
	void SetFile(IReadFile *hFile)
	{
		m_hFile=hFile;
	}
	void SetStart(DWORD32 start)
	{
		m_start=start;
	}
	void SetSize(DWORD32 size)
	{
		m_size=size;
	}
	void SetEnd(DWORD32 end)
	{
		m_end=end;
	}
	virtual void Read();
	void Dump();
	CMP4Read_atom * FindChildAtom(const char *type);
protected:
	CMP4Read_atom * FindChildAtomHelper(const char *type,int nIndex=0);

	DWORD32	m_start;
	DWORD32	m_end;
	DWORD32	m_size;
	char	m_type[5];
	IReadFile	*m_hFile;

	BOOL32	m_bHasChildAtom;
	vector<CMP4Read_atom*>	m_arrChildAtom;			//实际上读取到的child atom
	UINT	m_depth;
};

class CMP4Read_atom_tkhd:public CMP4Read_atom
{
public:
	void Read();
	int m_width;
	int m_height;
	DWORD32 m_duration;
};

class CMP4Read_atom_mdhd:public CMP4Read_atom
{
public:
	void Read();
	DWORD32 m_timescale;
	DWORD32 m_duration;
};

class CMP4Read_atom_hdlr:public CMP4Read_atom
{
public:
	void Read();

	BOOL32 IsVideo()
	{
		return strncmp(m_handlerType,"vide",4)==0;
	}
	char m_handlerType[5];
};

class CMP4Read_atom_stsd:public CMP4Read_atom
{
public:
	void Read();
};

class CMP4Read_atom_avc1:public CMP4Read_atom
{
public:
	void Read();
};

class CMP4Read_atom_avcC:public CMP4Read_atom
{
public:
	CMP4Read_atom_avcC()
	{
		m_pps = NULL;
		m_pps_count = 0;
		m_sps = NULL;
		m_sps_count = 0;
	}
	~CMP4Read_atom_avcC()
	{
		Empty();
	}
	void Empty();

	void Read();

	LPBYTE m_pps;
	int m_pps_count;
	LPBYTE m_sps;
	int m_sps_count;
};

class CMP4Read_atom_hvc1:public CMP4Read_atom
{
public:
	void Read();
};

class CMP4Read_atom_hvcC:public CMP4Read_atom
{
public:
	CMP4Read_atom_hvcC()
	{
		m_pps = NULL;
		m_pps_count = 0;
		m_sps = NULL;
		m_sps_count = 0;
		m_vps = NULL;
		m_vps_count = 0;
	}
	~CMP4Read_atom_hvcC()
	{
		Empty();
	}
	void Empty();

	void Read();

	LPBYTE m_pps;
	int m_pps_count;
	LPBYTE m_sps;
	int m_sps_count;
	LPBYTE m_vps;
	int m_vps_count;
};

class CMP4Read_atom_mp4v:public CMP4Read_atom
{
public:
	void Read();
	int m_width;
	int m_height;
};

class CMP4Read_atom_stts:public CMP4Read_atom
{
public:
	CMP4Read_atom_stts()
	{
		m_stts=NULL;
		m_stts_count=0;
	}

	~CMP4Read_atom_stts()
	{
		delete []m_stts;
		m_stts=NULL;
		m_stts_count=0;
	}

	void Read();
	
	tagMP4Node_stts *m_stts;
	UINT m_stts_count;
};

class CMP4Read_atom_stsc:public CMP4Read_atom
{
public:
	CMP4Read_atom_stsc()
	{
		m_stsc=NULL;
		m_stsc_count=0;
	}

	~CMP4Read_atom_stsc()
	{
		delete []m_stsc;
		m_stsc=NULL;
		m_stsc_count=0;
	}

	void Read();
	
	tagMP4Node_stsc *m_stsc;
	UINT m_stsc_count;
};

class CMP4Read_atom_stsz:public CMP4Read_atom
{
public:
	CMP4Read_atom_stsz()
	{
		m_stsz=NULL;
		m_stsz_count=0;
		m_size=0;
	}

	~CMP4Read_atom_stsz()
	{
		delete []m_stsz;
		m_stsz=NULL;
		m_stsz_count=0;
	}

	void Read();
	
	DWORD32 *m_stsz;
	DWORD32 m_size;
	UINT m_stsz_count;
};

class CMP4Read_atom_stco:public CMP4Read_atom
{
public:
	CMP4Read_atom_stco()
	{
		m_stco=NULL;
		m_stco_count=0;
		m_size=0;
	}

	~CMP4Read_atom_stco()
	{
		delete []m_stco;
		m_stco=NULL;
		m_stco_count=0;
	}

	void Read();
	
	DWORD32 *m_stco;
	UINT m_stco_count;
};

class CMP4Read_atom_stss:public CMP4Read_atom
{
public:
	CMP4Read_atom_stss()
	{
		m_stss=NULL;
		m_stss_count=0;
	}

	~CMP4Read_atom_stss()
	{
		delete []m_stss;
		m_stss=NULL;
		m_stss_count=0;
	}

	void Read();
	
	DWORD32 *m_stss;
	UINT m_stss_count;
};

class MP4_AFX_EXT_CLASS CMP4Read_atom_trak:public CMP4Read_atom
{
public:
	CMP4Read_atom_trak()
	{
		m_bInit=FALSE;
		m_bVideoTrak=FALSE;
		m_bAmr=FALSE;
		m_bMjpg=FALSE;
        // 初始化 AAC 相关成员
        m_bAAC = FALSE;
        m_aacSampleRate = 16000;
        m_aacChannels = 1;

		m_stts=NULL;
		m_stsc=NULL;
		m_stsz=NULL;
		m_stco=NULL;
		m_stss=NULL;

		m_dwFrameIndex = 0;
		m_curDuration=0;
	}

	BOOL32 IsIFrame(int nVideoFrameIndex);
	
	DWORD32 GetDurationSum(int nVideoFrame);
	DWORD32 GetCurDuration();
	int GetCurFrame();

	void Read();
	BOOL32 IsVideoTrak();
	BOOL32 IsUlaw()
	{
		return !IsVideoTrak() && !m_bAmr;
	}

	BOOL32 IsMjpgTrak();

	int ReadVideoFrame(LPBYTE pFrame,int cbFrame,int&cbRead,DWORD32& duration, int skip = 0);
	int ReadAudioFrame(LPBYTE pFrame,int cbFrame,int&cbRead,DWORD32& duration);
	int Test();

	int GetFrameCount();

	int Seek(DWORD32 durationSeek);

protected:
	int GetVideoFrameCount()
	{
		ASSERT(IsVideoTrak() && m_stsz);
		return m_stsz->m_stsz_count;
	}

	int GetAudioFrameCount();

	int		GetChunkIndex(int nSampleIndex,int& nChunkIndex,int& nChunkSampleIndex);
	DWORD32	GetSampleDuration(int nSampleIndex);

	BOOL32	m_bInit;
	BOOL32	m_bVideoTrak;//otherwise is audio trak(只在m_bInit为TRUE时有效)
	BOOL32	m_bAmr;//otherwise is ulaw(只在m_bInit为TRUE并且m_bVideoTrak为FALSE时有效)
	BOOL32	m_bMjpg;

    // ---------- 新增 AAC 支持 ----------
    BOOL32 m_bAAC;            // 是否是 AAC 音频
    int m_aacSampleRate;      // AAC 采样率（从 mdhd 获取）
    int m_aacChannels;        // AAC 声道数（默认 1）

	CMP4Read_atom_stts *m_stts;
	CMP4Read_atom_stsc *m_stsc;
	CMP4Read_atom_stsz *m_stsz;
	CMP4Read_atom_stco *m_stco;
	CMP4Read_atom_stss *m_stss;

	DWORD32	m_dwFrameIndex;
	DWORD32	m_curDuration;//当前位置的总共duration,用于demux
	int		m_nCurFrame;
};


enum eExtData
{
	eExtData_sps,
	eExtData_pps,
	eExtData_vps,
};

//XiongWanPing 2009.11.09
class MP4_AFX_EXT_CLASS CMP4Read
{
public:
	int GetExtData(eExtData type,LPBYTE pBuf,int cbBuf,int& cbDataBytes);
	BOOL32 IsOpen()
	{
		return m_iFile!=NULL;
	}

	BOOL32 IsIFrame(int nVideoFrameIndex);

	IReadFile	*m_iFile;

	CMP4Read();
	virtual ~CMP4Read();
	int Open(const char * pszFile);
	int Attach(IReadFile *iReadFile);
	int Close();
	int GetAudioInfo(tagMP4AudioInfo& ai);
	int GetVideoInfo(tagMP4VideoInfo& vi);
	int ReadVideoFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration, int skip = 0);
	int ReadAudioFrame(LPBYTE pFrame,int cbFrame,int& cbRead,DWORD32& duration);
	
	int GetCurVideoFrame();
	int Seek(DWORD32 durationVideoSeek);
	int SeekToFrame(int nVideoFrame);
    int SeekToDuration(unsigned int duration);
	int SeekToIFrame();
	int SeekToPrevIFrame();
	DWORD32 GetCurVideoDuration();
	DWORD32 GetCurAudioDuration();
	int GetIndexSplNum(char trakType, int& currMp4SampleIndex, int& currMp4SampleSum);
	
protected:
	int OpenHelper();
	int GetTrakCount();
	CMP4Read_atom_trak *FindAudioTrak();
	CMP4Read_atom_trak *FindVideoTrak();

	int  CheckValid();
	int  ParseAtom();
	void DumpAtom();

//.int get_audio_info(int&type,DWORD32& dwMaxTimeStamp);
//.int get_video_info(int& width,int &height,int& type,int& fps,DWORD32& dwMaxTimeStamp);
//.int read_audio_frame(LPBYTE pFrame,int cbFrame,DWORD32& dwTimeStamp);
//.int read_video_frame(LPBYTE pFrame,int cbFrame,DWORD32& dwTimeStamp);
//.int seek(DWORD32 dwTimeStamp);//同时seek audio&video到最接近dwTimeStamp的位置,以video dwTimeStamp为准(audio会根据video进行换算)
protected:
	DWORD32	m_cbFile;//文件长度

	//CMP4Read_moov	m_moov;
	CMP4Read_atom	*m_pRootAtom;

	tagMP4VideoInfo m_vi;
	tagMP4AudioInfo m_ai;

	CMP4Read_atom_trak *m_pVideoTrak;
	CMP4Read_atom_trak *m_pAudioTrak;
};

//XiongWanPing 2009.09.07
//for parse starvalley rtsp  stream config
class MP4_AFX_EXT_CLASS CBitStream  
{
public:
	CBitStream();
	virtual ~CBitStream();

	BOOL32	Init(LPBYTE pBitStream,int cbBit,int nBitPos);
	BOOL32	Init(const char *pszHex,int cbLen=-1);
	void	UnInit();

	UINT	Read(int nBit);
	UINT	Peek(int nBit);
	BOOL32	SkipBit(int nBit);

	BOOL32	SetBit(UINT nBitPos,BOOL32 bSet=TRUE);
	BOOL32	WriteBit(UINT nBitPos,UINT nValue,int cbBit);

	UINT	GetBitPos()
	{
		return m_nBitPos;
	}

protected:
	LPBYTE	m_pBitStream;
	int		m_cbBitStream;//in bits
	int		m_nBitPos;	  //bit pos

	LPBYTE	m_byBuf;

};

//XiongWanPing 2009.10.21
//for parse mp4 decode config
class CMpeg4BitStream:public CBitStream
{
public:
	int Parse();
protected:
	int Parse_VisualObject();
	int Parse_VisualObjectSequence();
	int Parse_video_signal_type();
	int VideoObjectLayer();

	
	int  next_start_code();
	bool bytealigned();
	UINT next_bits(int nBit);
};



//说明:只需要在Windows平台支持修复mp4文件

enum eMP4FileStatus
{
	eMP4File_Fail=-1,	//操作失败,可能是文件不存在,无法打开
	eMP4File_Unknown=0,	//无法识别的文件
	eMP4File_JCOK,		//ok,正常录像结束的文件
	eMP4File_JCOX,		//x=wrong,异常录像结束的文件,需要修复
	eMP4File_JCOR,		//r=repaired,异常录像结束的文件,已经修复
};

class MP4_AFX_EXT_CLASS CMP4Repair
{
public:
	CMP4Repair();
	virtual ~CMP4Repair();
	int Open(const char *pszMP4File);
	void Close();
	eMP4FileStatus GetMP4FileStatus();
	int Repair(unsigned int *timelen);
protected:
	FILE *m_hFile;
	char  *m_szFile;
	eMP4FileStatus m_fs;
	long	m_pos_jcox;
	long	m_pos_mdat;
	long	m_pos_moov;
	long	m_cbFile;
	long	m_jcox_size;
};



#endif  // DEV_TYPE

#endif  // _MP4_DEMUX_H_XWPCOM_20091201_

