/***********************************************************************************

CWebSocketImpl.h - Header file contains CWebSocketImpl class, which is a client socket to 
connect to WebSocket Servers.

@author: Ramesh Kumar K

*************************************************************************************/


# ifndef WEBSOCKET_IMPL_H
# define WEBSOCKET_IMPL_H


#include "clientSocket\Socket.h"

// Poco Library Headers
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/Context.h"
# include <string>

// Poco Namespaces
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::WebSocket;
using Poco::Net::Context;



namespace Connection{

	namespace ConnectionUtils{

		class CWebSocketImpl: public CSocket, public IReferenceCounter{

		private:
			
			// WebSocket Specific variables (POCO Objects)

			// HTTP Session object. HTTPSClientSession object will be used for SSL connection. (casted to HTTPClientSession)
			HTTPClientSession*   lpClientSession;
			HTTPRequest*         lpHttpRequest;
			HTTPResponse         httpResponse;

			// WebSocket pointer
			WebSocket*           lpWebSocket;
			std::string          serverEndpointUrl;
			Poco::Net::Context::Ptr         lpWssContext;

			// Sets up Socket configurations parameters
			// TODO: Make it public and move it to base class when making Socket Options configurable
			bool initSocketConfigurations();
			
			void setPocoObjectReferences();

		public:

			CWebSocketImpl(std::string serverHostName,
				int serverPort,
				bool isSecureConnection,
				std::string serverEndPointUrl = "");

			CWebSocketImpl(std::string serverHostName, 
				int serverPort, 
				bool isSecureConnection, 
				std::string serverEndPointUrl = "", 
				std::string loggerPath = "", bool isSharedLog = false);

			CWebSocketImpl(std::string serverHostName,
				int serverPort,
				bool isSecureConnection,
				std::string proxyHost,
				int proxyPort,
				std::string proxyUser,
				std::string proxyPassword,
				std::string serverEndPointUrl = "");
			
			// Sets up Session, request and response members based on arguments.
			// Called from Constructor
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

			virtual ~CWebSocketImpl();
		};
	}
}

#endif



