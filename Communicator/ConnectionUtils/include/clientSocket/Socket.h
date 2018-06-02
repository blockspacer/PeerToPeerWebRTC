/***********************************************************************************

CSocket.h - CSocket inherits Socket Wrapper. Bridge betwen IConnection and protocol
			 based socket implementation and adds Poco::Logger element to Socket class.

		   - Including CLogger directly will include _WINSOCK_API macro, which disables 
		     few win APIs. (Ex: CreateProcess, ...)

		   - Abstracts Poco::Logger from implementation.

Contains virtual methods and so this class could not be instantiated directly.

@author: Ramesh Kumar K

*************************************************************************************/

# ifndef CLIENTSOCKET_SOCKETS_H
# define CLIENTSOCKET_SOCKETS_H

# include <cstdio>
# include <string>
# include "logger\Logger.h"
# include "Connection.h"
# include "concurrent_queue.h"
# include "QueueHandler.h"
# include "SmartPtr.h"

#define SEND_BUFFER_LENGTH	500000

namespace Connection{

	namespace ConnectionUtils{

		class CSocket : public IConnection{

		protected:

			HANDLE hSenderThread;
			bool   checkSenderThreadStatus();
			
		protected:
			
			// Setting error informations - Only allowed to call from implementation methods
			void   setLastError(int errorCode, std::string & errorMsg);
			void   setLastError(int errorCode);

		public:
			
			bool   bRunSenderThread;
			bool	bSenderThreadRunning;

			BinaryDataPairQueue < char , int  > sendQueue;
			
			// Default constructor must be called from implementation with reference for Logger Object
			CSocket(std::string logPath = "", bool isSharedLog = false);

			~CSocket()
			{
				sockUtilLogger = NULL;
			}

			// Logger Object
			SmartPtr<CLogger> sockUtilLogger;

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
			virtual bool   isSocketLive() = 0;
			virtual void   shutDownSocket() = 0;

			// Methods for Asynchronous (Queue based) Sending
			bool   sendStringAsync(char* sendBuffer);
			bool   sendStringAsync(char* sendBuffer, int bufferLength);
			bool   sendBytesAsync(unsigned char* sendBuffer, int bufferLength);
			bool   sendVectorAsync(std::vector <char>& data);
			void   DestroySocket();

		protected:

			atomic<bool> is_connected_to_peer;

		public:

			std::recursive_mutex mtxSenderThread;

		};
	}
}

#endif


/*****

TODO:

1. Error Code handling
2. Logger
3. Configurable ssl settings

*****/
		
		
		
		
		
		