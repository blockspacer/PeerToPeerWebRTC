// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#if defined(_DEBUG) || defined (DEBUG)

#define _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC_NEW

#endif

#ifdef VLD_FORCE_ENABLE

# include <vld.h>

#include <stdlib.h>
#include <crtdbg.h>

#endif

#include "targetver.h"

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <string>
#include "MacroProcessor.h"

#ifdef WIN32

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstring;

typedef std::basic_stringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstringstream;

typedef std::basic_fstream<TCHAR, std::char_traits<TCHAR>> tfstream;

#endif

// TODO: reference additional headers your program requires here
