/**
	Connection.cpp
	Purpose: WEBRTC core connection handler.

	@author  Ramesh Kumar K - FEB 2018
*/

#include "stdafx.h"
#include "Connection.h"
#include "JsonHandler.h"

namespace Connection {

	namespace ClientSocket {

		//Callbacks to be registered.
		bool(*Connection::sendSDP)(char*);
		bool(*Connection::sendCandidate)(char*);
		bool(*Connection::dataChannelEvent)(char*);
		bool(*Connection::errorHandler)(int, const char*);

		Connection::Connection() :
			pco(*this),
			dco(*this),
			csdo(*this),
			ssdo(*this) {
			InitializeCriticalSectionAndSpinCount(&m_SendQueueLock, 4000);
			hQueueEvent = ::CreateEventA(NULL, false, false, NULL);
			data_channel = nullptr;
			peer_connection = nullptr;
			bCreator = false;
		}

		Connection::~Connection()
		{
			connectionCleanup();
			logger->log(CLogger::LOG_INFO, "Connection::~Connection - Data channel and peer connection handlers closed.");
		}

		void Connection::connectionCleanup()
		{
			//TODO WEBRTC: Should be revisited. Commenting it out since exe is blocked here sometimes.
			/*if (data_channel != nullptr)
			{
				data_channel->Close();
				data_channel = nullptr;
			}
			if (peer_connection != nullptr)
			{
				peer_connection->Close();
				peer_connection = nullptr;
			}*/
			logger->log(CLogger::LOG_INFO, "Connection::~Connection - Data channel and peer connection handlers closed.");
		}

		/**
		Callback to receive SDP answer description.

		@param desc Session description with answer.
		*/
			void Connection::onSDPCreationSuccess(webrtc::SessionDescriptionInterface* desc) {
			
				if (bCreator)
				{
					onOffer(desc);
				}
				else
				{
					onAnswer(desc);
				}
		}

		/**
		handle to set SDP offer/answer description.

		@param NIL.
		*/
			bool Connection::onHandShakeMessage(std::string &sdp) {

				bool retVal = false;

				webrtc::SdpParseError error;
				webrtc::SessionDescriptionInterface* session_description = nullptr;

				if (bCreator)
				{
					session_description = (webrtc::CreateSessionDescription("answer", sdp, &error));
				}
				else
				{
					session_description = (webrtc::CreateSessionDescription("offer", sdp, &error));
				}

				if (session_description == nullptr)
				{
					logger->log(CLogger::LOG_ERROR, "CPeerConnection::onHandShakeMessage - Session Description not initialized properly");
				}
				else
				{
					if (peer_connection)
					{
						peer_connection->SetRemoteDescription(&ssdo, session_description);

						if (!bCreator)
						{
							peer_connection->CreateAnswer(&csdo, nullptr);
						}

						retVal = true;
					}
					else
					{
						logger->log(CLogger::LOG_ERROR, "CPeerConnection::onHandShakeMessage - peer_connection not initialized properly");
					}
				}

				return retVal;
			}

		void Connection::InitOffer() {

				bCreator = true;

				if (peer_connection.get() != nullptr)
				{
					/*
					webrtc::DataChannelInit data_channel_config;

					data_channel_config.ordered = false;
					data_channel_config.maxRetransmits = 0;
					*/

					// Create the RTCDataChannel with an observer.
					data_channel = peer_connection->CreateDataChannel("dc", nullptr);

					data_channel->RegisterObserver(&dco);

					logger->log(CLogger::LOG_INFO, "CPeerConnection::connect - peer_connection offer created successfully");

					peer_connection->CreateOffer(&csdo, NULL);
				}
				else
				{
					logger->log(CLogger::LOG_INFO, "CPeerConnection::connect - failed to create peer_connection offer");
				}
			}

		/**
			Callback to receive SDP answer description.

			@param desc Session description with answer.
		*/
		void Connection::onAnswer(webrtc::SessionDescriptionInterface* desc) { 
			peer_connection->SetLocalDescription(&ssdo, desc);
			std::string sdp;
			if(!desc->ToString(&sdp))
			{ 
				logger->log(CLogger::LOG_ERROR, "Connection::onAnswer - Unable to serialize answer sdp");
			}
			else
			{
				handleAnswer(sdp.c_str());
			}
		}

		/**
		Callback to receive SDP offer description.

		@param desc Session description with answer.
		*/
		void Connection::onOffer(webrtc::SessionDescriptionInterface* desc) {
			peer_connection->SetLocalDescription(&ssdo, desc);
			std::string sdp;
			if (!desc->ToString(&sdp))
			{
				logger->log(CLogger::LOG_ERROR, "Connection::onAnswer - Unable to serialize answer sdp");
			}
			else
			{
				handleOffer(sdp.c_str());
			}
		}

		/**
			Callback to receive Local ICE candidate.

			@param candidate Local ICE candidate.
		*/
		void Connection::onIceCandidate(const webrtc::IceCandidateInterface* candidate) {
			std::string sdp;

			if (!candidate->ToString(&sdp))
			{
				logger->log(CLogger::LOG_ERROR, "Connection::onIceCandidate - Unable to serialize candidate");
			}
			else
			{
				if (sdp.find("relay") == std::string::npos)
				{
					JsonHandler actionJson;
					actionJson.create_json_object();
					actionJson.add_string("candidate", sdp);
					actionJson.add_string("sdpMid", candidate->sdp_mid());
					actionJson.add_int("sdpMLineIndex", candidate->sdp_mline_index());
					std::string candidateStr =  actionJson.toString();
					logger->log(CLogger::LOG_INFO, "Connection::onIceCandidate - Candidate - %s", candidateStr);
					if (sendCandidate)
					{
						sendCandidate(const_cast<char*>(candidateStr.c_str()));
					}
				}
				else
				{
					logger->log(CLogger::LOG_ERROR, "Connection::onIceCandidate - Relay candidate neglected - %s", sdp);
				}
			}
		}

		/**
			SDP answer handler.

			@param answer SDP Answer.
		*/
		void Connection::handleAnswer(const char *answer)
		{
			JsonHandler actionJson;
			actionJson.create_json_object();
			actionJson.add_string("type", "answer");
			actionJson.add_string("sdp", answer);
			std::string sdpAns = actionJson.toString();
			logger->log(CLogger::LOG_INFO, "Connection::handleAnswer - Answer SDP - %s", sdpAns);
			if (sendSDP)
			{
				sendSDP(const_cast<char*>(sdpAns.c_str()));
			}
		}

		/**
		SDP answer handler.

		@param answer SDP Answer.
		*/
		void Connection::handleOffer(const char *offer)
		{
			JsonHandler actionJson;
			actionJson.create_json_object();
			actionJson.add_string("type", "offer");
			actionJson.add_string("sdp", offer);
			std::string sdpOffer = actionJson.toString();
			logger->log(CLogger::LOG_INFO, "Connection::handleOffer - Offer SDP - %s", sdpOffer);
			if (sendSDP)
			{
				sendSDP(const_cast<char*>(sdpOffer.c_str()));
			}
		}

		/* PeerConnectionObserver */
		Connection::PeerConnectionObserver::PeerConnectionObserver(Connection& parent) : parent(parent) {

		}

		/**
			Callback to handle Signalling state Change.

			@param new_state Signalling state.
		*/
		void Connection::PeerConnectionObserver::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
			logger->log(CLogger::LOG_INFO, "PeerConnectionObserver::OnSignalingChange - State - %d",(int)new_state);
			if (new_state == webrtc::PeerConnectionInterface::SignalingState::kClosed)
			{
				if (errorHandler)
				{
					errorHandler(ERROR_SEVERITY::ERROR_NORMAL, ERROR_SIGNAL_CLOSED);
				}
			}
		};

		void Connection::PeerConnectionObserver::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
			
		};

		void Connection::PeerConnectionObserver::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

		};

		/**
			Callback to receive Data channel created on remote peer.

			@param data_channel Data channel used to communicate remote peer.
		*/
		void Connection::PeerConnectionObserver::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
			parent.data_channel = data_channel;
			parent.data_channel->RegisterObserver(&parent.dco);
			logger->log(CLogger::LOG_INFO, "PeerConnectionObserver::OnDataChannel - Data channel received");
		};

		void Connection::PeerConnectionObserver::OnRenegotiationNeeded() {
			logger->log(CLogger::LOG_INFO, "PeerConnectionObserver::OnRenegotiationNeeded - Renegotiating");
			if (errorHandler)
			{
				errorHandler(ERROR_SEVERITY::ERROR_NORMAL, RENEGOTIATION_NEEDED);
			}
		};

		/**
			Callback to handle ICE connection state Change.

			@param new_state ICE connection state.
		*/
		void Connection::PeerConnectionObserver::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
			logger->log(CLogger::LOG_INFO, "PeerConnectionObserver::OnIceConnectionChange - State - %d",(int)new_state);
			if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionFailed)
			{
				if (errorHandler)
				{
					errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_ICEFAILED);
				}
			}
			else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed)
			{
				if (errorHandler)
				{
					errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_ICECLOSED);
				}
			}
			else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionDisconnected)
			{
				if (errorHandler)
				{
					errorHandler(ERROR_SEVERITY::ERROR_NORMAL, ERROR_ICEDISC);
				}
			}
		};

		/**
			Callback to handle ICE gathering state Change.

			@param new_state ICE gathering state.
		*/
		void Connection::PeerConnectionObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
			logger->log(CLogger::LOG_INFO, "PeerConnectionObserver::OnIceGatheringChange - State - %d", (int)new_state);
		
		};

		void Connection::PeerConnectionObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
			logger->log(CLogger::LOG_INFO, "PeerConnectionObserver::OnIceCandidate - candidate received");
			parent.onIceCandidate(candidate);
		};


		/* DataChannelObserver */
		Connection::DataChannelObserver::DataChannelObserver(Connection& parent) : parent(parent) {

		}

		/**
			Callback to handle Data channel state Change.

			@param new_state Data channel state.
		*/
		void Connection::DataChannelObserver::OnStateChange() {
			logger->log(CLogger::LOG_INFO, "DataChannelObserver::OnStateChange - State - %d", (int)parent.data_channel->state());
			if (parent.data_channel->state() == webrtc::DataChannelInterface::DataState::kOpen)
			{
				if (dataChannelEvent)
				{
					dataChannelEvent(const_cast<char*>(parent.data_channel->DataStateString(parent.data_channel->state())));
				}
			}
			else if (parent.data_channel->state() == webrtc::DataChannelInterface::DataState::kClosed)
			{
				if (errorHandler)
				{
					errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_DTCH_CLOSED);
				}
			}
		};

		/**
			Callback to handle data from remote peer.

			@param buffer Data from remote peer.
		*/
		void Connection::DataChannelObserver::OnMessage(const webrtc::DataBuffer& buffer) {
			EnterCriticalSection(&parent.m_SendQueueLock);
			parent.stringData.push(buffer);

			SetEvent(parent.hQueueEvent);
			LeaveCriticalSection(&parent.m_SendQueueLock);
		};

		/**
			Callback to handle data channel buffer amount change.

			@param previous_amount Previous data channel buffer amount.
		*/
		void Connection::DataChannelObserver::OnBufferedAmountChange(uint64_t previous_amount) {
			if (parent.data_channel->buffered_amount() > DTCH_BUFFER_MAX)
			{
				if (errorHandler)
				{
					errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_DTCH_OVERFLOW);
				}
			}
			std::string bufferedAmt = "DataChannelObserver::OnBufferedAmountChange - Prev amt - " + std::to_string(previous_amount) + " - Cur amt - " + std::to_string(parent.data_channel->buffered_amount());
			logger->log(CLogger::LOG_INFO, (char*)bufferedAmt.c_str());
		};


		/* CreateSessionDescriptionObserver */
		Connection::CreateSessionDescriptionObserver::CreateSessionDescriptionObserver(Connection& parent) : parent(parent) {
		
		}

		/**
			Callback to receive Local session description.

			@param desc Local session description.
		*/
		void Connection::CreateSessionDescriptionObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
			logger->log(CLogger::LOG_INFO, "CreateSessionDescriptionObserver::OnSuccess - sdp received");

			parent.onSDPCreationSuccess(desc);
		};

		/**
			Callback to receive error while session description creation.

			@param error Error string.
		*/
		void Connection::CreateSessionDescriptionObserver::OnFailure(const std::string& error) {
			logger->log(CLogger::LOG_ERROR, "CreateSessionDescriptionObserver::OnFailure - sdp error - %s", error.c_str());
			if (errorHandler)
			{
				errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_ANSWER);
			}
		};

		int Connection::CreateSessionDescriptionObserver::AddRef() const {
			return 0;
		};

		int Connection::CreateSessionDescriptionObserver::Release() const {
			return 0;
		};

		/* SetSessionDescriptionObserver */
		Connection::SetSessionDescriptionObserver::SetSessionDescriptionObserver(Connection& parent) : parent(parent) {

		}

		/**
			Callback to receive success when session description is set.
		*/
		void Connection::SetSessionDescriptionObserver::OnSuccess() {
			logger->log(CLogger::LOG_INFO, "SetSessionDescriptionObserver::OnSuccess - SSDO success");
		};

		/**
			Callback to receive error when session description is set.

			@param error Error string.
		*/
		void Connection::SetSessionDescriptionObserver::OnFailure(const std::string& error) {
			logger->log(CLogger::LOG_ERROR, "SetSessionDescriptionObserver::OnFailure - SSDO failure - %s", const_cast<char*>(error.c_str()));
			if (errorHandler)
			{
				errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_OFFER);
			}
		};

		int Connection::SetSessionDescriptionObserver::AddRef() const {

			return 0;
		};

		int Connection::SetSessionDescriptionObserver::Release() const {

			return 0;
		};
	}
}