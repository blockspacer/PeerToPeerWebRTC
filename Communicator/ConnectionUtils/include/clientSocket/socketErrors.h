/***********************************************************************************

CTCPImpl.h - Header file contains common error codes used by Connection project.
              - defines SocketError struct

@author: Ramesh Kumar K

*************************************************************************************/

# ifndef CLIENTSOCKET_SOCKUTIL_ERRORS_H
# define CLIENTSOCKET_SOCKUTIL_ERRORS_H

namespace Connection{

# define SOCKUTIL_STD_EXCEPTION      999
# define MAX_ERROR_MSG_LEN           500

	// Enumerate Custom error messages.
	enum{
		SOCKUTIL_ERROR_SUCCESS = 1000,
		SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED,
		SOCKUTIL_ERROR_LOGGER_NOT_INITIALIZED,
		SOCKUTIL_ERROR_PROXY_NOT_CONFIGURED,
		SOCKUTIL_ERROR_HTTP_FORBIDDEN,
		SOCKUTIL_ERROR_HTTP_PROXY_AUTH_FAILED,
		SOCKUTIL_ERROR_HTTP_BAD_REQUEST,
		SOCKUTIL_ERROR_ALREADY_SHUTDOWN,
		SOCKUTIL_ERROR_EXCEPTION_OCCURED,
		SOCKUTIL_ERROR_MEMORY_FAILURE_OCCURED,
		SOCKUTIL_ERROR_UNKNOWN
	};
	
	// Structure to represent ErrorCode with Error Message
	struct socketError{
		int errorCode;
		char* errorMsg;
	} ;
	
	// Method returns Error message for given error code. Should be used only while setting errors.
	// Returns error message only for custom error codes. (enumerated above)
	char* getErrorMessage( int errorCode );

}

# endif


