#pragma once

#include "stdafx.h"
#include <cstdarg>
#include "MacroProcessor.h"

class CErrorMessage
{

protected:

	static void SetErrorMessageA											( TCHAR *file, int Line, TCHAR* function, const char* printf_like_message, ... ) ;


	static void SetErrorMessage												( TCHAR *file, int Line, TCHAR* function, const wchar_t* printf_like_message, ... ) ;

public:

	static TCHAR* GetErrorMessage()
	{
		return m_str_error_msg.GetBuffer();
	}

protected:

	static CString m_str_error_msg;
};

#define SET_ERROR(printf_like_message, ...)									SetErrorMessage($File, __LINE__, $Function, printf_like_message, ##__VA_ARGS__);
#define SET_ERROR_A(printf_like_message, ...)								SetErrorMessageA($File, __LINE__, $Function, printf_like_message, ##__VA_ARGS__);