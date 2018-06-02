/**
	PeerConnection.h
	Purpose: Utility for Peer to Peer communication using WEBRTC - Header File.

	@author Ramesh Kumar K
*/

# ifndef CLIENTSOCKET_WebrtcDLL_H
# define CLIENTSOCKET_WebrtcDLL_H

# include "iostream"
# include "string"
# include "queue"
# include "Connection.h"
# include "logger\Logger.h"

#ifdef WEBRTC_EXPORTS  
#include "webrtc\p2p\client\basicportallocator.h"
#define WEBRTC_API __declspec(dllexport)   
#else  
#define WEBRTC_API __declspec(dllimport)   
#endif  

namespace Connection {

	namespace ConnectionUtils {
		
		extern CLogger* logger;

		//Handles peer to peer communication using WEBRTC 
		class CPeerConnection {

		    public:
			
			#ifdef WEBRTC_EXPORTS
			/** Webrtc Library related members (Exposed only to Webrtc DLL) */
			Connection connection;

			rtc::Thread* thread;
			rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
			webrtc::PeerConnectionInterface::RTCConfiguration configuration;
			std::unique_ptr<cricket::BasicPortAllocator> portallocator;
			std::string turnUri,turnUser,turnPwd;
			HANDLE webrtcFactoryThreadHandle;
			#endif
			
			HANDLE hConEvent; //Handle to signal peer connection factory creation
			const int waitTimeout = 10000; //Wait timeout for WEBRTC main thread to complete
			const uint64_t datachannel_threshold = 1024 * 4096; //2048 KB threshold limit for datachannel buffer 
			const int datachannel_retry_count = 10; //Retry count when threshold limit for datachannel buffer is reached
			const int datachannel_wait_time = 100; //Time to wait before next retry when threshold limit for datachannel buffer is reached

			int prevDataChannelState; //Previous datachannel state maintained for logging purpose.
			bool isProxyEnabled; //Through proxy
			bool isRunning;

			public:

			CPeerConnection(std::string loggerPath = "");
			~CPeerConnection();

			/** Internal methods */
			void initialize();


			/** Methods to be called from other applications */

			//Register callback handlers.
			WEBRTC_API virtual bool setErrorReceiver(bool(*errorReceiver)(int, const char*));
			WEBRTC_API virtual bool setDataChannelStateReceiver(bool(*dataReceiver)(char*));
			WEBRTC_API virtual bool setSDPReceiver(bool(*sdpReceiver)(char*));
			WEBRTC_API virtual bool setCandidateReceiver(bool(*candidateReceiver)(char*));
			
			//Webrtc Signalling related functions.
			WEBRTC_API virtual bool setSDPHandShakeMessage(const std::string& parameter);
			WEBRTC_API virtual bool setCandidate(const std::string& parameter);
			WEBRTC_API virtual void setTurnInfo(std::string &server, std::string &uname, std::string &pass);

			//Webrtc connection and communication related functions.
			WEBRTC_API virtual bool connect();
			WEBRTC_API virtual bool InitSDPOffer();
			WEBRTC_API virtual bool connectThroughProxy();
			WEBRTC_API virtual int  receiveBytes(unsigned char* receiveBuf, int bufferSize);
			WEBRTC_API virtual int  receiveString(char* receiveBuf, int bufferSize);
			WEBRTC_API virtual int  sendString(char* sendBuffer);
			WEBRTC_API virtual int  sendString(char* sendBuffer, int bufferLength);
			WEBRTC_API virtual int  sendBytes(unsigned char* sendBuffer, int bufferLength);
			WEBRTC_API virtual void close();
			WEBRTC_API virtual void shutDownSocket();

		private:

			std::string loggerPath;

		};
		
		//Create and Delete Webrtc instance.
		WEBRTC_API CPeerConnection* createPeerConnectionHandlerInstance(std::string loggerpath = "");
		WEBRTC_API void deleteWebrtcInstance();
	}
}
#endif