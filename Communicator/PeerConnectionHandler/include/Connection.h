/**
	Connection.h
	Purpose: WEBRTC core connection handler - Header File.

	@author Ramesh Kumar K
*/

# ifndef CLIENTSOCKET_WEBRTCCONN_H
# define CLIENTSOCKET_WEBRTCCONN_H

# define ERROR_ICEFAILED "ICE_FAILED"
# define ERROR_ICECLOSED "ICE_CLOSED"
# define ERROR_ICEDISC "ICE_DISCONNECTED"
# define ERROR_OFFER "OFFER_FAILURE"
# define ERROR_ANSWER "ANSWER_FAILURE"
# define ERROR_DTCH_CLOSED "DTCH_FAILURE"
# define ERROR_DTCH_OVERFLOW "DTCH_OVERFLOW"
# define ERROR_SIGNAL_CLOSED "SIGNALLING_CLOSED"
# define RENEGOTIATION_NEEDED "RENEGOTIATION_NEEDED"

# define DTCH_BUFFER_MAX 3 * 1024 * 1024

enum ERROR_SEVERITY
{
	ERROR_SEVERE = 1,
	ERROR_NORMAL
};

#ifdef WEBRTC_EXPORTS
#include <webrtc\api\peerconnectioninterface.h>
#endif
#include "logger\logger.h"

#ifdef WEBRTC_EXPORTS
namespace Connection {

	namespace ConnectionUtils {

		extern CLogger* logger;

		///Handles WEBRTC core connection
		class Connection {

		public:
			rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
			rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
			
			static bool(*sendSDP)(char*); //SDP handler to be registered.
			static bool(*sendCandidate)(char*); //Candidate handler to be registered.
			static bool(*dataChannelEvent)(char*); //Data Channel notification handler to be registered.
			static bool(*errorHandler)(int, const char*);//Error handler to be registered.
			
			CRITICAL_SECTION m_SendQueueLock; //Lock to synchronize to the push and pop from Queue.
			HANDLE hQueueEvent; //Event to wait for front & pop from queue.
			std::queue<webrtc::DataBuffer> stringData; //Queue for String data.
			
			void connectionCleanup();

		public:

			void InitOffer();

			bool onHandShakeMessage(std::string &sdp);

		private:
			void onSDPCreationSuccess(webrtc::SessionDescriptionInterface* desc);
			void onAnswer(webrtc::SessionDescriptionInterface* desc); //Callback on Answer SDP.
			void onOffer(webrtc::SessionDescriptionInterface* desc); //Callback on Offer SDP.
			void onIceCandidate(const webrtc::IceCandidateInterface* candidate); //Callback on Candidate.							
			void handleAnswer(const char *answer); 
			void handleOffer(const char *answer);

			
			
			class PeerConnectionObserver : public webrtc::PeerConnectionObserver {
			private:
				Connection& parent;

			public:
				PeerConnectionObserver(Connection& parent);

				void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state);

				void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream);

				void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream);

				void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel);

				void OnRenegotiationNeeded();

				void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state);

				void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state);

				void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
			};

			class DataChannelObserver : public webrtc::DataChannelObserver {
			private:
				Connection& parent;

			public:
				DataChannelObserver(Connection& parent);

				void OnStateChange();
				void OnMessage(const webrtc::DataBuffer& buffer);
				void OnBufferedAmountChange(uint64_t previous_amount);
			};

			class CreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
			private:
				Connection& parent;

			public:
				CreateSessionDescriptionObserver(Connection& parent);

				void OnSuccess(webrtc::SessionDescriptionInterface* desc);

				void OnFailure(const std::string& error) override;

				int AddRef() const override;

				int Release() const override;
			};

			class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
			private:
				Connection& parent;

			public:
				SetSessionDescriptionObserver(Connection& parent);

				void OnSuccess() override;

				void OnFailure(const std::string& error) override;

				int AddRef() const override;

				int Release() const override;
			};

		public:
			PeerConnectionObserver  pco;
			DataChannelObserver  dco;
			CreateSessionDescriptionObserver csdo;
			SetSessionDescriptionObserver ssdo;

			Connection();
			~Connection();

		private:

			bool bCreator;
		};
	}
}
#endif
#endif