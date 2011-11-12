#ifndef _STDAFX_H
#define _STDAFX_H

#if defined(_WINDOWS)
#pragma once

#define _WIN32_LEAN_AND_MEAN

#ifndef STRICT
#define STRICT
#endif

#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <winsock2.h>
#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <strsafe.h>
#include <CommCtrl.h>
#include <shlobj.h>
#include <math.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#include <Ole2.h>
#include <OleIdl.h>
#include <ShObjIdl.h>
#include <assert.h>
#include <setjmp.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <stdio.h>
#include <sys/types.h>
#include <mmsystem.h>
#include <stddef.h>
#include <time.h>


#ifdef __cplusplus
#include <gdiplus.h>
using namespace Gdiplus;
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
using namespace ATL;
#include <map>
#include <list>
#include <vector>
#include <string>

namespace std
{
#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif
}
#endif

#elif defined(_LINUX)
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include <time.h>
#include <ctype.h>

#define TCHAR char
typedef void *LPVOID;
typedef const char *LPCTSTR;
#define ARRAYSIZE(z) (sizeof(z)/sizeof((z)[0]))

/* BEGIN MICROSOFT INCLUDE FILE CODE */
/*
 * The "tchar.h" include file (from Microsoft) has been truncated down
 * to the necessary defines for wxWabbitemu.
 * I'd never thought I'd say this... but thanks to Microsoft, most of
 * the original code compiles without modification! :P
 */

/*
 * tchar.h - definitions for generic international text functions
 *
 *       Copyright (c) 1991-1997, Microsoft Corporation. All rights reserved.
 *
 * Purpose:
 *       Definitions for generic international functions, mostly defines
 *       which map string/formatted-io/ctype functions to char, wchar_t, or
 *       MBCS versions.  To be used for compatibility between single-byte,
 *       multi-byte and Unicode text models.
 *
 *       [Public]
 */ 
#include <wchar.h>

#define _tprintf    wprintf
#define _tcsicmp    _wcsicmp
/* END MICROSOFT INCLUDE FILE CODE */
#include "calc.h"
int putst (const wchar_t * str);
int _wcsicmp(const char* cs,const wchar_t * ct);
#define _tcscpy_s _tcscpy
#define _tprintf_s _tprintf
#define strcpy_s strcpy
#define _putts putst
#define _strnicmp strncasecmp

#define ZeroMemory(dest, size) memset(dest, 0, size)

#elif defined(_MACVER)
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include <time.h>
#include <ctype.h>

/*
typedef char TCHAR;
typedef void *LPVOID;
typedef const char *LPCTSTR;
typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
typedef u_int8_t BYTE, *LPBYTE;
typedef u_int16_t WORD, *LPWORD;
typedef u_int32_t DWORD, *LPDWORD;
 */
/*
#ifndef TRUE
#define FALSE (0)
#define TRUE (!FALSE)
#ifdef WINVER
typedef int BOOL;
#else
typedef signed char BOOL;
#endif
#endif
*/
#define MAX_PATH 256
#define _T(z) z
#define _tprintf_s printf
#define ARRAYSIZE(z) (sizeof(z)/sizeof((z)[0]))
#define _strnicmp strncasecmp
#define _tcsicmp strcasecmp
#define _putts puts
#define _tcsrchr strrchr
#define _tcscpy_s strcpy
#define _tcslen strlen
#define _tcscmp strcmp

#endif

#endif
