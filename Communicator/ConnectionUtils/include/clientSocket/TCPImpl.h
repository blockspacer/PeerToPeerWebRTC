/***********************************************************************************

CTCPImpl.h - Header file contains CTCPImpl class, which is a client socket to 
connect to TCP Servers.

@author: Ramesh Kumar K

*************************************************************************************/

# ifndef TCPIMPL_H
# define TCPIMPL_H

# include "clientSocket\Socket.h"
# include "Winsock2.h"

# include "openssl\ssl.h" 
# include "openssl\err.h" 
# include <string>
# include "SmartPtr.h"
# include <atomic>

namespace Connection{

	namespace ConnectionUtils{

		class CTCPImpl: public CSocket, public IReferenceCounter{

		private:
			SOCKET tcpSocket;
			SSL_CTX* sslContext;
			SSL* lpSSL;
			bool isTunnelHandShakeStarted;
			bool isShutdownInitiated;
			struct hostent *serverHost;

			void initializeSSLContext();
			void setKeepAlive(int aliveToggle);
			void detectErrorSetSocketOption(int* errCode,string& errMsg);
			bool doSSLConnect();
			bool doTunnelHandshake();
			bool getProxyServerResponse();
			std::string getAuthHeader();
			void setTcpErrorCode(int wsaErrorCode);

			void setTCPObjectReferences();


		public:

			CTCPImpl(std::string serverHostName, int serverPort, bool isSecureConnection, std::string loggerPath = "", bool isSharedLog = false); 

			CTCPImpl(std::string serverHostName, int serverPort, bool isSecureConnection);

			CTCPImpl(std::string serverHostName,
				int serverPort,
				bool isSecureConnection,
				std::string proxyHost,
				int proxyPort,
				std::string proxyUser,
				std::string proxyPassword);

			void initialize();

			// Parent Class' abstract methods - Implemented using POCO WebSocket APIs
			bool connect();
			bool connectViaProxy();
			int  receiveBytes(unsigned char* receiveBuf, int bufferSize);
			int  receiveString(char* receiveBuf, int bufferSize);
			int  sendString(char* sendBuffer);
			int  sendString(char* sendBuffer, int bufferLength);
			int  sendBytes(unsigned char* sendBuffer, int bufferLength);
			void close();
			void shutDownSocket();
			bool isSocketLive();


			virtual ~CTCPImpl();
		};
	}
}

# endif
