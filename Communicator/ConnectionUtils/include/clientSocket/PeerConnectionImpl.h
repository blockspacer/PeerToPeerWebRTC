# ifndef PEER_CONNECTION_IMPL_H
# define PEER_CONNECTION_IMPL_H

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "clientSocket\Socket.h"
#include "logger\Logger.h"
#include "clientSocket\socketErrors.h"
#include "clientSocket\socketConfHandler.h"

#include "PeerConnection.h"

namespace Connection {

	namespace ConnectionUtils {
		
		class CPeerConnectionImpl : public CSocket, public IReferenceCounter {
		
			CPeerConnection* _pPeerConnection;
		
		public:
			CPeerConnectionImpl(std::string logger_path = "");

			// Sets up Session, request and response members based on arguments.
			// Called from Constructor

			void initialize();

			//Webrtc related function calls.
			
			bool setSDPReceiver(bool(*sdpReceiver)(char*));
			bool setCandidateReceiver(bool(*candidateReceiver)(char*));
			bool setDataChannelStateReceiver(bool(*dataReceiver)(char*));
			bool setErrorReceiver(bool(*dataReceiver)(int, const char*));

			bool setSDPHandShakeMessage(const std::string& parameter);
			bool setCandidate(const std::string& parameter);
			void setTurnInfo(std::string &server, std::string &uname, std::string &pass);

			// Parent Class' abstract methods - Implemented using Chromium WEBRTC native APIs
			bool connect();
			bool InitSDPOffer();
			bool connectViaProxy();
			int  receiveBytes(unsigned char* receiveBuf, int bufferSize);
			int  receiveString(char* receiveBuf, int bufferSize);
			int  sendString(char* sendBuffer);
			int  sendString(char* sendBuffer, int bufferLength);
			int  sendBytes(unsigned char* sendBuffer, int bufferLength);
			void close();
			void shutDownSocket();
			bool isSocketLive();

			~CPeerConnectionImpl();

		private:

			std::string logger_path;
		};
	}
}
#endif