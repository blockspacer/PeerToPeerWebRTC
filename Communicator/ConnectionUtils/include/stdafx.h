// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers



// TODO: reference additional headers your program requires here

#ifndef _STATIC_HTTP_LIBRARY_

	// CLIENSOCKET_DLL_EXPORTS must never be defined in applications using this dll.
	#if defined(CLIENTSOCKET_DLL_EXPORTS)
		// dllexport classes when compiled as dll project
		#define ClientSocket_API __declspec(dllexport)
	#else
		// dllimport class methods when used from application.
		#define ClientSocket_API __declspec(dllimport)	
	#endif

#else 

#define ClientSocket_API

#endif