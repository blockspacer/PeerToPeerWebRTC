# include "stdafx.h"
# include "ErrorMessages.h"


/** thanks to: http://www.cplusplus.com/reference/string/string/find_last_of/
* Splits string at the last '/' or '\\' separator
* example: "/mnt/something/else.cpp" --> "else.cpp"
*          "c:\\windows\\hello.h" --> hello.h
*          "this.is.not-a-path.h" -->"this.is.not-a-path.h" */

tstring splitFileName(const tstring& str) {
   size_t found;
   found = str.find_last_of(_T("(/\\"));
   return str.substr(found + 1);
}

void CErrorMessage ::SetErrorMessageA( TCHAR *file, int Line, TCHAR* function, const char* printf_like_message, ...) 
{
	try	
	{
		char finished_message[4096];
		va_list arglist;

		va_start(arglist, printf_like_message);
			
		if(arglist)
		{
			const int nbrcharacters = _vsnprintf (finished_message, sizeof(finished_message), printf_like_message, arglist);

			if (nbrcharacters > 0)
			{
				m_str_error_msg.Format(_T("File %s, Line %d, Function %s message: "), file, Line, function);

				m_str_error_msg += CString((LPCSTR)finished_message);
			}
		}

		va_end(arglist);
	}
	catch(std::exception &e)
	{
		// do something
	}
	catch(...)
	{
		// do something
	}
}

void CErrorMessage ::SetErrorMessage( TCHAR *file, int Line, TCHAR* function, const TCHAR* printf_like_message, ...) 
{
	try	
	{
		TCHAR finished_message[4096];
		va_list arglist;

		va_start(arglist, printf_like_message);

		if (arglist)
		{
			const int nbrcharacters = _vsnwprintf (finished_message, sizeof(finished_message), printf_like_message, arglist);

			if (nbrcharacters > 0)
			{
				m_str_error_msg.Format(_T("File %s, Line %d, Function %s message: "), file, Line, function);

				m_str_error_msg += CString((LPCTSTR)finished_message);
			}
		}

		va_end(arglist);
	}
	catch(std::exception &e)
	{
		// do something
	}
	catch(...)
	{
		// do something
	}
}