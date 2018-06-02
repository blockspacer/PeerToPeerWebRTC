#pragma once
# include <string>
# include "stdint.h"


# define DEFAULT_SSL_PORT						443

# define DEFAULT_NON_SSL_PORT					80

namespace Connection
{

	class ClientConnection
	{
		protected:

			// Host Name of the Server to be connected
			std::string serverHostName;

			// Proxy Host Name in case proxy is to be used.
			// Proxy details will be used when direct communication fails and use Proxy is enabled.
			std::string proxyHostName;

			// Proxy User Name (Credential)
			std::string proxyUserName;

			// Proxy Sever password (Credential)
			std::string proxyPassword;

			// Server port.
			int serverPort;

			// Proxy Server's port.
			int proxyPort;
			
			// Specifies whether to connect to server using SSL or non secure connection. (http/https)
			bool isSecureConnection;

			// Switch to allow or disable using proxy server.
			bool _buseProxy;


			// Error Informations - Every error code must be set with error message.
			std::string			lastErrorMsg;
			int					lastErrorCode;	

			unsigned int		log_level;
			

		protected:

			// Setting error informations - Only allowed to call from implementation methods
			virtual void   setLastError(int errorCode, std::string& errorMsg)=0;
			virtual void   setLastError(int errorCode)=0;
			virtual void   setLastWinErrorCode(int errorCode)=0;


		public:

			// Error Handling
			virtual int   getLastError()=0;
			virtual std::string getLastErrorMsg()=0;
			
			// Getter Setters for Server Information

			virtual void setServerHostName(std::string hostName)=0;
			virtual void setServerPort(int port)=0;
			virtual void setConnectionMode(bool isSecure)=0;
			virtual void setConnectionDetails(std::string hostName, int hostPort = 443, bool isSecureMode = true,
				std::string proxyHostName = "", int proxyPort = 0, 
				std::string proxyUser = "", std::string proxyPassword = "", long long timeout = -1)=0;
			virtual void setProxyDetails(std::string proxyHostName, int proxyPort, 
				std::string proxyUser, std::string proxyPassword)=0;
			virtual void setProxySwitch(bool _buseProxy)=0;
			virtual void setProxyHostName(std::string hostName)=0;
			virtual void setProxyPort(int port)=0;
			virtual void setProxyUserName(std::string userName)=0;
			virtual void setProxyPassword(std::string password)=0;
			virtual std::string getServerHostName()=0;
			virtual int getServerPort()=0;
			virtual bool isSecureMode()=0;
			virtual bool isProxyEnabled()=0;
			virtual std::string getProxyHostName()=0;
			virtual std::string getProxyUserName()=0;
			virtual std::string getProxyPassword()=0;
			virtual int getProxyPort()=0;
	};

};