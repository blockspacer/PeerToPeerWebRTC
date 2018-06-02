# include "windows.h"
# include "clientSocket\Connection.h"
# include "clientSocket\socketErrors.h"
# include <string>


namespace Connection{

	namespace ConnectionUtils{

		// Constructor called from implementation class instances
		IConnection::IConnection():IReferenceCounter()
		{
			lastErrorCode = SOCKUTIL_ERROR_SUCCESS;

			lastErrorMsg = getErrorMessage(lastErrorCode);
		}

		IConnection::~IConnection()
		{
		}
		
		// Used to Set Custom error Codes.
		void IConnection::setLastError(int errorCode)
		{
			setLastError(errorCode, std::string(getErrorMessage(errorCode)));
		}
		
		// Used to Set POCO and Windows error codes.
		void IConnection::setLastError(int errorCode, std::string &errorMsg)
		{
			lastErrorCode = errorCode;
			lastErrorMsg = errorMsg;
		}

		void IConnection::setLastWinErrorCode(int errorCode)
		{
			LPSTR errorMsg = 0;

			if(FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				errorCode,
				0,
				(LPSTR)&errorMsg,
				0,
				NULL) != 0)
			{
				setLastError(errorCode, std::string(errorMsg));
			}
			else
			{
				setLastError(errorCode);
			}

			if(errorMsg != NULL)
			{
				LocalFree(errorMsg);
			}
		}

		int IConnection::getLastError()
		{
			return lastErrorCode;
		}

		std::string IConnection::getLastErrorMsg()
		{
			return lastErrorMsg;
		}

		// Server Details getters and setters

		void IConnection::setConnectionDetails(std::string hostName, int hostPort, bool isSecureMode,
				std::string proxyHostName, int proxyPort, 
				std::string proxyUser, std::string proxyPassword, long long timeout)
		{
			setServerHostName(hostName);
			setServerPort(hostPort);
			setConnectionMode(isSecureMode);
			setProxyDetails(proxyHostName, proxyPort, proxyUser, proxyPassword);
		}

		void IConnection::setServerHostName(std::string hostName)
		{
			serverHostName = hostName;
		}

		void IConnection::setServerPort(int port)
		{
			serverPort = port;
		}

		void IConnection::setConnectionMode(bool isSecure)
		{
			isSecureConnection = isSecure;
		}

		void IConnection::setProxyDetails(std::string proxyHostName, int proxyPort, std::string proxyUser, std::string proxyPassword)
		{
			setProxyHostName(proxyHostName);
			setProxyPort(proxyPort);
			setProxyUserName(proxyUser);
			setProxyPassword(proxyPassword);
		}

		void IConnection::setProxySwitch(bool isProxyEnabled)
		{
			_buseProxy = isProxyEnabled;
		}

		void IConnection::setProxyHostName(std::string hostName)
		{
			proxyHostName = hostName;
		}

		void IConnection::setProxyPort(int port)
		{
			proxyPort = port;
		}

		void IConnection::setProxyUserName(std::string userName)
		{
			proxyUserName = userName;
		}

		void IConnection::setProxyPassword(std::string password)
		{
			proxyPassword = password;
		}

		std::string IConnection::getServerHostName()
		{
			return serverHostName;
		}

		int IConnection::getServerPort()
		{
			return serverPort;
		}

		bool IConnection::isSecureMode()
		{
			return isSecureConnection;
		}

		bool IConnection::isProxyEnabled()
		{
			return _buseProxy;
		}

		std::string IConnection::getProxyHostName()
		{
			return proxyHostName;
		}
		
		std::string IConnection::getProxyUserName()
		{
			return proxyUserName;
		}
		
		std::string IConnection::getProxyPassword()
		{
			return proxyPassword;
		}

		int IConnection::getProxyPort()
		{
			return proxyPort;
		}

	}

}



