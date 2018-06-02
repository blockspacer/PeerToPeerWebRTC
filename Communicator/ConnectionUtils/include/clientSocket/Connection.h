/***********************************************************************************

SocketWrapper.h - Header file representing a common Client Socket. WebSocket and TCP Socket 
implmentations are done extending this class.

Contains Only virtual methods and so this class could not be instantiated directly.

@author: Ramesh Kumar K

*************************************************************************************/

# ifndef CLIENTSOCKET_SOCKET_WRAPPER_H
# define CLIENTSOCKET_SOCKET_WRAPPER_H


#if defined(_DEBUG) || defined (DEBUG)

#define _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC_NEW


#define VLD_FORCE_ENABLE
# include <vld.h>

#include <stdlib.h>
#include <crtdbg.h>

#endif

# include <cstdio>
# include <string>
# include <vector>
# include "SmartPtr.h"
# include "ReferenceCounter.h"
# include "ConnectionParams.h"

using namespace Connection;


namespace Connection{

	namespace ConnectionUtils{

		class IConnection : public ClientConnection, public IReferenceCounter{

		public:

			IConnection();
			virtual ~IConnection();
			

			virtual void initialize() = 0;
			// Socket related methods - Protocol based implementations must be done on implementation classes
			virtual bool   connect() = 0;
			virtual bool   connectViaProxy() = 0;
			virtual int    receiveBytes(unsigned char* receiveBuf, int bufferSize) = 0;
			virtual int    receiveString(char* receiveBuf, int bufferSize) = 0;
			virtual int    sendString(char* sendBuffer) = 0;
			virtual int    sendString(char* sendBuffer, int bufferLength) = 0;
			virtual int    sendBytes(unsigned char* sendBuffer, int bufferLength) = 0;
			virtual void   close() = 0;
			virtual void   shutDownSocket() = 0;
			virtual void   DestroySocket() =0;

			// Methods for Asynchronous (Queue based) Sending
			virtual bool   sendStringAsync(char* sendBuffer) = 0;
			virtual bool   sendStringAsync(char* sendBuffer, int bufferLength) = 0;
			virtual bool   sendBytesAsync(unsigned char* sendBuffer, int bufferLength) = 0;
			virtual bool   sendVectorAsync(std::vector <char> &data) = 0;

			virtual bool   checkSenderThreadStatus() = 0;

			//override

		protected:

			// Setting error informations - Only allowed to call from implementation methods
			void   setLastError(int errorCode, std::string & errorMsg);
			void   setLastError(int errorCode);
			void   setLastWinErrorCode(int errorCode);


		public:

			// Error Handling
			int   getLastError();
			std::string getLastErrorMsg();
			
			// Getter Setters for Server Information

			void setServerHostName(std::string hostName);
			void setServerPort(int port);
			void setConnectionMode(bool isSecure);
			void setConnectionDetails(std::string hostName, int hostPort = 443, bool isSecureMode = true, 
				std::string proxyHostName = "", int proxyPort = 0, 
				std::string proxyUser = "", std::string proxyPassword = "", long long timeout = -1);
			void setProxyDetails(std::string proxyHostName, int proxyPort, 
				std::string proxyUser, std::string proxyPassword);
			void setProxySwitch(bool _buseProxy);
			void setProxyHostName(std::string hostName);
			void setProxyPort(int port);
			void setProxyUserName(std::string userName);
			void setProxyPassword(std::string password);
			std::string getServerHostName();
			int getServerPort();
			bool isSecureMode();
			bool isProxyEnabled();
			std::string getProxyHostName();
			std::string getProxyUserName();
			std::string getProxyPassword();
			int getProxyPort();

		};
	}
}

#endif


/*****

TODO:

1. Error Code handling
2. Logger
3. Configurable ssl settings
4. TCP implementation
5. Connect through proxy

*****/
		
		
		
		
		
		