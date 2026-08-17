#ifndef __basetype_h__
#define __basetype_h__

#include <stdint.h>

#ifndef WINAPI

typedef unsigned BOOL;

typedef unsigned char   BYTE;
typedef BYTE*           PBYTE;
typedef void*           HANDLE;

typedef char            CHAR;
typedef char            *PCHAR;
typedef unsigned char   *PUCHAR;

typedef short           SHORT;
typedef short           *PSHORT;
typedef unsigned short  *PUSHORT;

typedef uint16_t        WORD;
typedef WORD*           PWORD;
typedef uint32_t        DWORD;
typedef DWORD*          PDWORD;
typedef unsigned long *LPDWORD;

typedef unsigned char   UCHAR;
typedef uint16_t        USHORT;

typedef uint32_t        UINT;
typedef UINT*           PUINT;

typedef long            LONG;
typedef long            *PLONG;
typedef unsigned long   ULONG;
typedef unsigned long   *PULONG;
typedef long long	    LONGLONG; //64 bit

typedef unsigned int    BOOLEAN;

#endif

// macro
#ifndef S_OK
#define S_OK                           ( 0)
#endif
#ifndef S_FAIL
#define S_FAIL                         (-1)
#endif

#endif
