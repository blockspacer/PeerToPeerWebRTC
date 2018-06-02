# include "clientSocket\socketErrors.h"

namespace Connection{
	
	// Customized Error codes with Error messages - must be assigned here.
	socketError stSockUtilErrors[] = {

		{ SOCKUTIL_ERROR_SUCCESS, "Socket Ready" },
		{ SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED, "Socket Not Intialized properly" },
		{ SOCKUTIL_ERROR_LOGGER_NOT_INITIALIZED, "Logger Not Initialized due to invalid arguments" },
		{ SOCKUTIL_ERROR_PROXY_NOT_CONFIGURED, "Proxy Details not configured" },
		{ SOCKUTIL_ERROR_HTTP_FORBIDDEN, "Http Error 403 - Forbidden" },
		{ SOCKUTIL_ERROR_HTTP_PROXY_AUTH_FAILED, "Http Error 407 - Proxy Authentication Failed" },
		{ SOCKUTIL_ERROR_HTTP_BAD_REQUEST, "Http Error 400 - Bad Request" },
		{ SOCKUTIL_ERROR_ALREADY_SHUTDOWN, "Socket shutdown already initiated" },
		{ SOCKUTIL_ERROR_EXCEPTION_OCCURED, "Critical exception occured" },
		{ SOCKUTIL_ERROR_MEMORY_FAILURE_OCCURED, "Memory error happened" },
		{ SOCKUTIL_ERROR_UNKNOWN, "Unknown Error" }
	};

	char* getErrorMessage( int errorCode )
	{
		for (int index = 0; stSockUtilErrors[index].errorMsg; ++index)
	    
		if (stSockUtilErrors[index].errorCode == errorCode)
		{
			return stSockUtilErrors[index].errorMsg;
		}

		return "Unknown Error Code";
	}

}