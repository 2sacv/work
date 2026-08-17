/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_typdedef.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2015-02-10
    Description  :
    History      :
                        created by tianjun. 2015-02-10
******************************************************************************/

#ifndef __ENCODE_TYPEDEF_H__
#define __ENCODE_TYPEDEF_H__

#ifdef __cplusplus
extern "C" {
#endif
	

#ifndef UCHAR	
typedef unsigned char   UCHAR;
#endif

#ifndef ULONG	
typedef unsigned long   ULONG;
#endif

#ifndef USHORT	
typedef unsigned short  USHORT;
#endif

#ifndef UINT	
typedef unsigned int    UINT;
#endif



#ifndef CHAR	
typedef char            CHAR;
#endif

#ifndef PCHAR	
typedef char            *PCHAR;
#endif

#ifndef PUCHAR	
typedef unsigned char   *PUCHAR;
#endif

#ifndef BYTE	
typedef unsigned char   BYTE;
#endif

#ifndef PBYTE	
typedef BYTE*           PBYTE;
#endif


#ifndef short	
typedef short           SHORT;
#endif

#ifndef PSHORT	
typedef short           *PSHORT;
#endif

#ifndef PUSHORT	
typedef unsigned short  *PUSHORT;
#endif

#ifndef WORD	
typedef unsigned short  WORD;
#endif

#ifndef PWORD	
typedef WORD*           PWORD;
#endif

#ifndef DWORD	
typedef unsigned int	DWORD;
#endif

#ifndef PDWORD	
typedef DWORD*          PDWORD;
#endif

#ifndef PUINT	
typedef UINT*           PUINT;
#endif

#ifndef long	
typedef long            LONG;
#endif

#ifndef PLONG	
typedef long            *PLONG;
#endif

#ifndef PULONG	
typedef unsigned long   *PULONG;
#endif


#ifndef BOOLEAN
typedef unsigned int    BOOLEAN;
#endif

#ifndef BOOL
#define BOOL            BOOLEAN
#endif


#ifndef PVOID	
typedef void *          PVOID;
#endif

#ifndef HANDLE	
typedef void *          HANDLE;
#endif

#ifndef SOCKET  
typedef int             SOCKET;
#endif

#ifndef FLOAT	
typedef float			FLOAT;
#endif

#ifndef SCHAR   
typedef signed char     SCHAR;
#endif

#ifndef SWORD	
typedef signed short    SWORD;
#endif

#ifndef SDWORD  
typedef signed int      SDWORD;
#endif

#ifndef TASK
typedef void            TASK;
#endif

#ifndef QWORD
typedef unsigned long long	QWORD;
#endif

#ifndef SQWORD
typedef long long		SQWORD;	
#endif

#ifndef FIX16
typedef signed short     FIX16;
#endif

#ifndef UFIX16
typedef unsigned short   UFIX16;
#endif

#ifndef FIX
typedef signed long      FIX;
#endif

#ifndef UFIX
typedef unsigned long    UFIX;
#endif

#ifndef TRUE
#define TRUE			1
#endif

#ifndef FALSE
#define FALSE			0
#endif

#ifndef NULL
#define NULL			0
#endif

#define ON				1
#define OFF				0

#ifndef S_OK
#define S_OK			0
#endif


#ifndef SCODE
typedef unsigned int    SCODE;
#endif

#ifndef S_FAIL
#define S_FAIL			(SCODE)(-1)
#endif

#ifndef S_INVALID_VERSION
#define S_INVALID_VERSION (SCODE)(-2)
#endif



#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0,ch1,ch2,ch3)  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif 


#ifdef __cplusplus
}
#endif

#endif//__ENCODE_TYPEDEF_H__