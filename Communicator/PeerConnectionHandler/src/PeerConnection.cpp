/**
	CPeerConnection.cpp
	Purpose: Utility for Peer to Peer communication using WEBRTC.

	@author Ramesh Kumar K - FEB 2018
*/

#include "stdafx.h"
#include "PeerConnection.h"
#include "JsonHandler.h"
#include "webrtc\base\ssladapter.h"
#include "webrtc\base\thread.h"
#include "webrtc\api\test\fakeconstraints.h"
#include "webrtc\media\base\mediaengine.h"
#include "webrtc\p2p\base\basicpacketsocketfactory.h"


namespace Connection {

	namespace ClientSocket {

		CPeerConnection* webrtc = NULL;
		CLogger* logger;

		CPeerConnection::CPeerConnection(std::string logPath)
		{
			loggerPath = logPath;

			initialize();
		}

		CPeerConnection::~CPeerConnection()
		{

		}

		/**
			Initializes class members.
		*/
		void CPeerConnection::initialize()
		{
			if (loggerPath.empty())
			{
				// default log path
				// TODO: Should be made configurable
				logger = CLogger::getInstance("..\\logs\\PeerConnection.log", false);
			}
			else
			{
				logger = CLogger::getInstance(loggerPath, false);
			}

			logger->log(CLogger::LOG_INFO, "CPeerConnection::initialize - Log opened");
			hConEvent = ::CreateEventA(NULL, true, false, NULL);
			peer_connection_factory = nullptr;
			thread = nullptr;
			webrtcFactoryThreadHandle = nullptr;
			isProxyEnabled = false;
			isRunning = false;
		}

		/**
			Thread to create peer connection factory.

			@param lParam The object with the event to be signalled upon peer connection factory creation.
			@return 1 on success and 0 on failure.
		*/
		DWORD WINAPI webrtcFactoryThread(LPVOID lParam) {
			DWORD threadRetVal = 0;
			CPeerConnection *obj = (CPeerConnection*)lParam;
			obj->peer_connection_factory = webrtc::CreateModularPeerConnectionFactory(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

			SetEvent(obj->hConEvent); 
			if (obj->peer_connection_factory.get() != nullptr)
			{
				obj->thread = rtc::Thread::Current();
				obj->thread->Run();
				threadRetVal = 1;
			}
			return threadRetVal;
		}

		bool CPeerConnection::InitSDPOffer()
		{
			if (connection.peer_connection)
			{
				connection.InitOffer();

				return true;
			}

			return false;
		}

		/**
			Creates peer connection instance and registers Peer Connection Observer.

			@return true on success and false on failure.
		*/
		bool CPeerConnection::connect()
		{
			bool connectResult = false;

			if (!isProxyEnabled)
			{
				try
				{
					configuration.type = webrtc::PeerConnectionInterface::IceTransportsType::kAll;

					rtc::InitializeSSL();

					DWORD webrtcThread;
					webrtcFactoryThreadHandle = ::CreateThread(NULL, 0, webrtcFactoryThread, this, 0, &webrtcThread);

					DWORD signal = WaitForSingleObject(hConEvent, waitTimeout);
					if (signal == WAIT_OBJECT_0)
					{
						if (peer_connection_factory.get() != nullptr)
						{
							/*
							webrtc::FakeConstraints constraints_;

							constraints_.SetMandatoryReceiveAudio(false);
							constraints_.SetMandatoryReceiveVideo(false);
							constraints_.SetAllowRtpDataChannels();

							rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
							rtc::scoped_refptr<webrtc::MediaStreamInterface> stream_;

							const char kAudioLabel[] = "audio_label";
							const char kStreamLabel[] = "stream_label";

							connection.peer_connection = peer_connection_factory->CreatePeerConnection(
							configuration, &constraints_, nullptr, nullptr, &connection.pco);
							*/

							connection.peer_connection = peer_connection_factory->CreatePeerConnection(
								configuration, nullptr, nullptr, &connection.pco);

							if (connection.peer_connection.get() != nullptr)
							{
								/*
								audio_track_ = peer_connection_factory->CreateAudioTrack(kAudioLabel,
									NULL);
								stream_ = peer_connection_factory->CreateLocalMediaStream(kStreamLabel);
								stream_->AddTrack(audio_track_);
								connection.peer_connection->AddStream(stream_);
								*/

								logger->log(CLogger::LOG_INFO, "CPeerConnection::connect - peer_connection created successfully");

								connectResult = true;
							}
							else
							{
								logger->log(CLogger::LOG_INFO, "CPeerConnection::connect - Error while creating peer_connection");
							}
						}
						else
						{
							logger->log(CLogger::LOG_ERROR, "CPeerConnection::connect - peer_connection_factory is not initialized properly");
						}
					}
					else
					{
						logger->log(CLogger::LOG_ERROR, "CPeerConnection::connect - Error - WaitForSingleObject not signalled");
					}
				}
				catch (std::exception &excep)
				{
					logger->log(CLogger::LOG_ERROR, "CPeerConnection::connect - Exception - %s", std::string(excep.what()));
				}
			}
			else
			{
				connectResult = connectThroughProxy();
			}

			isRunning = connectResult;

			return connectResult;
		}

		/**
		//TODO WEBRTC: Proxy implementation is not documented online. Should revisit. Code is not functional.
		Creates peer connection instance and registers Peer Connection Observer through a proxy.

		@return true on success and false on failure.
		*/
		bool CPeerConnection::connectThroughProxy()
		{
			bool connectResult = false;

			try
			{
				configuration.type = webrtc::PeerConnectionInterface::IceTransportsType::kAll;

				rtc::InitializeSSL();

				DWORD webrtcThread;
				::CreateThread(NULL, 0, webrtcFactoryThread, this, 0, &webrtcThread);

				DWORD signal = WaitForSingleObject(hConEvent, waitTimeout);
				if (signal == WAIT_OBJECT_0)
				{
					if (peer_connection_factory.get() != nullptr)
					{
						rtc::BasicPacketSocketFactory *packet_factory = new rtc::BasicPacketSocketFactory();
						rtc::BasicNetworkManager *network_manager = new rtc::BasicNetworkManager();

						rtc::SocketAddress local_socket_addr(INADDR_ANY, 0);
						rtc::SocketAddress remote_socket_addr("", 8443);
						rtc::ProxyInfo proxy_info;
						proxy_info.address.SetIP("");
						proxy_info.address.SetPort(9090);
						proxy_info.username = "";

						rtc::InsecureCryptStringImpl pass;
						pass.password() = "";
						proxy_info.password = rtc::CryptString(pass);
						proxy_info.type = rtc::ProxyType::PROXY_SOCKS5;
						rtc::AsyncPacketSocket *packet = packet_factory->CreateClientTcpSocket(local_socket_addr, remote_socket_addr, proxy_info, "", 0);
						std::unique_ptr<cricket::BasicPortAllocator> portalloc(new cricket::BasicPortAllocator(network_manager, packet_factory));
						cricket::RelayServerConfig relayconfig(std::string(""), 8443, turnUser, turnPwd, cricket::ProtocolType::PROTO_UDP);
						portalloc->AddTurnServer(relayconfig);
						portallocator = std::move(portalloc);
						portallocator->set_proxy("", proxy_info);
						portallocator->Initialize();
						
						connection.peer_connection = peer_connection_factory->CreatePeerConnection(
							configuration, std::move(portallocator), nullptr, &connection.pco);
						
						if (connection.peer_connection.get() != nullptr)
						{
							logger->log(CLogger::LOG_INFO, "CPeerConnection::connect - peer_connection created successfully");

							connectResult = true;
						}
						else
						{
							logger->log(CLogger::LOG_INFO, "CPeerConnection::connect - Error while creating peer_connection");
						}
					}
					else
					{
						logger->log(CLogger::LOG_ERROR, "CPeerConnection::connect - peer_connection_factory is not initialized properly");
					}
				}
				else
				{
					logger->log(CLogger::LOG_ERROR, "CPeerConnection::connect - Error - WaitForSingleObject not signalled");
				}
			}
			catch (std::exception &excep)
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::connect - Exception - %s", std::string(excep.what()));
			}

			return connectResult;
		}

		/**
			Sets the turn server name and credentials.

			@param server Turn server name
			@param uname Username for connecting to turn server
			@param pass Password for connecting to turn server
		*/
		void CPeerConnection::setTurnInfo(std::string &server, std::string &uname, std::string &pass)
		{
			turnUri = server;
			turnUser = uname;
			turnPwd = pass;

			webrtc::PeerConnectionInterface::IceServer ice_server;
			ice_server.uri = turnUri;
			ice_server.username = turnUser;
			ice_server.password = turnPwd;
			configuration.servers.push_back(ice_server);
			logger->log(CLogger::LOG_INFO, "CPeerConnection::setTurnInfo - %s - %s", server, uname);
		}

		/**
			Sets the SDP offer from the remote peer to Remote description.

			@param parameter Offer json string from the peer.
			@return true on success, false on failure.
		*/
		bool CPeerConnection::setSDPHandShakeMessage(const std::string& parameter)
		{
			bool retVal = false;
			JsonHandler json_obj;

			logger->log(CLogger::LOG_INFO, "CPeerConnection::setSDPHandShakeMessage - SDP SDPHandShakeMessage received %s", const_cast<char*>(parameter.c_str()));

			if (!json_obj.ParseJson(parameter))
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setSDPHandShakeMessage - Error while parsing offer json");
			}

			std::string sdp;

			if (!json_obj.get_string(sdp, "sdp"))
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setSDPHandShakeMessage - Error while getting sdp from offer json");
			}
			else
			{
				retVal = connection.onHandShakeMessage(sdp);
			}

			
			return retVal;
		}

		/**
			Sets the ICE candidate from the remote peer to Remote ICE candidates.

			@param parameter Offer json string from the peer.
			@return true on success, false on failure.
		*/
		bool CPeerConnection::setCandidate(const std::string& parameter)
		{
			std::string candidate;
			std::string sdpMid;
			int sdpMLineIndex = 0;
			bool retVal = false;

			JsonHandler json_obj;
			if (!json_obj.ParseJson(parameter))
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setCandidate - Error while parsing candidate json");
			}

			if (!json_obj.get_string(candidate, "candidate"))
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setCandidate - Error while getting candidate from json");
			}

			if (!json_obj.get_string(sdpMid, "sdpMid"))
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setCandidate - Error while getting sdpMid from json");
			}

			if (!json_obj.get_int(sdpMLineIndex, "sdpMLineIndex"))
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setCandidate - Error while getting sdpMLineIndex from json");
			}

			webrtc::SdpParseError err_sdp;
			webrtc::IceCandidateInterface* ice;
			ice = CreateIceCandidate(sdpMid,
				sdpMLineIndex,
				candidate,
				&err_sdp);

			if (!err_sdp.line.empty() && !err_sdp.description.empty())
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::setCandidate - Error - %s - %s", err_sdp.line, err_sdp.description);
			}
			else
			{
				if (connection.peer_connection)
				{
					connection.peer_connection->AddIceCandidate(ice);
					retVal = true;
				}
				else
				{
					logger->log(CLogger::LOG_ERROR, "CPeerConnection::setCandidate - peer_connection not initialized properly");
				}
			}
			return retVal;
		}


		/**
			Register the sdp-answer receiver.

			@param sdpReceiver Function to receive the SDP answer.
			@return true
		*/
		bool CPeerConnection::setSDPReceiver(bool(*sdpReceiver)(char*))
		{
			Connection::sendSDP = sdpReceiver;
			logger->log(CLogger::LOG_INFO, "CPeerConnection::setSDPReceiver - SDP receiver registered.");
			return true;
		}

		/**
			Register the candidate receiver.

			@param candidateReceiver Function to receive the ICE candidates.
			@return true
		*/
		bool CPeerConnection::setCandidateReceiver(bool(*candidateReceiver)(char*))
		{
			Connection::sendCandidate = candidateReceiver;
			logger->log(CLogger::LOG_INFO, "CPeerConnection::setCandidateReceiver - Candidate receiver registered.");
			return true;
		}

		/**
			Register the data channel state receiver.

			@param dataReceiver Function to receive the data channel state.
			@return true
		*/
		bool CPeerConnection::setDataChannelStateReceiver(bool(*dataStateReceiver)(char*))
		{
			Connection::dataChannelEvent = dataStateReceiver;
			logger->log(CLogger::LOG_INFO, "CPeerConnection::setDataChannelStateReceiver - Datachannel receiver registered.");
			return true;
		}

		/**
			Register the error receiver.

			@param dataReceiver Function to receive any error during connection.
			@return true
		*/
		bool CPeerConnection::setErrorReceiver(bool(*errorReceiver)(int, const char*))
		{
			Connection::errorHandler = errorReceiver;
			logger->log(CLogger::LOG_INFO, "CPeerConnection::setErrorReceiver - Error receiver registered.");
			return true;
		}

		/**
			Reads the number of characters specified from the input stream.

			@param receiveBuf Buffer to receive the characters.
			@param bufferSize Number of characters to be read from the buffer.
			@return Number of characters to read.
		*/
		int CPeerConnection::receiveString(char* receiveBuf, int bufferSize)
		{
			int receivedLength = 0;

			while (connection.stringData.size() <= 0)
			{
				WaitForSingleObject(connection.hQueueEvent, INFINITE);
			}

			EnterCriticalSection(&connection.m_SendQueueLock);
			{
				try
				{
					if (connection.stringData.size() > 0)
					{
						webrtc::DataBuffer & dataBuf = connection.stringData.front();
						int bufferLen = dataBuf.data.size();

						if (bufferLen <= bufferSize)
						{
							receivedLength = bufferLen;
							std::move(dataBuf.data.data<unsigned char>(), dataBuf.data.data<unsigned char>() + bufferLen, receiveBuf);
							connection.stringData.pop();
						}
						else
						{
							logger->log(CLogger::LOG_ERROR, "CPeerConnection::receiveString - Buffer length is small - %d", bufferLen);
							receivedLength = -1;
						}
					}
					else
					{
						logger->log(CLogger::LOG_ERROR, "CPeerConnection::receiveString - queue is empty");
					}
				}
				catch (std::exception &excep)
				{
					logger->log(CLogger::LOG_ERROR, "CPeerConnection::receiveString - Exception - %s", std::string(excep.what()));
					receivedLength = -1;
				}
			}
			
			LeaveCriticalSection(&connection.m_SendQueueLock);
			return receivedLength;
		}

		/**
			Reads the number of bytes specified from the input stream.

			@param receiveBuf Buffer to receive the bytes.
			@param bufferSize Number of bytes to be read from the buffer.
			@return Number of bytes read.
		*/
		int CPeerConnection::receiveBytes(unsigned char* receiveBuf, int bufferSize)
		{
			int receivedLength = 0;

			while (connection.stringData.size() <= 0)
			{
				WaitForSingleObject(connection.hQueueEvent, INFINITE);
			}

			EnterCriticalSection(&connection.m_SendQueueLock);

			try
			{
				webrtc::DataBuffer & dataBuf = connection.stringData.front();
				int bufferLen = dataBuf.data.size();

				if (bufferLen <= bufferSize)
				{
					receivedLength = bufferLen;
					std::move(dataBuf.data.data<unsigned char>(), dataBuf.data.data<unsigned char>() + bufferLen, receiveBuf);
					connection.stringData.pop();
				}
				else
				{
					logger->log(CLogger::LOG_ERROR, "CPeerConnection::receiveString - Buffer length is small - %d", bufferLen);
					receivedLength = -1;
				}
			}
			catch (std::exception &excep)
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::receiveBytes - Exception - %s", std::string(excep.what()));
				receivedLength = -1;
			}

			LeaveCriticalSection(&connection.m_SendQueueLock);
			return receivedLength;
		}



		/**
			Sends the string of characters to the peer.

			@param sendBuf Text to be sent.
			@return Number of characters sent.
		*/
		int CPeerConnection::sendString(char* sendBuf)
		{
			return sendString(sendBuf, strlen(sendBuf));
		}
		
		/**
			Sends the string of characters to the peer.

			@param sendBuf Text to be sent.
			@param bufferLength  Number of characters to be sent.
			@return Number of characters sent.
		*/
		int CPeerConnection::sendString(char* sendBuf, int bufferLength)
		{
			int bytesSent = 0;

			try
			{
				webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(sendBuf, bufferLength), false);
				if (isRunning && connection.data_channel && connection.data_channel->state() == 1)
				{ 
					int retryCount = 0;
					while (isRunning && connection.data_channel->state() == 1)
					{
						if (retryCount == datachannel_retry_count)
						{
							logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendString - Retry count reached. Error Handler DTCH_OVERFLOW");
							if (connection.errorHandler)
							{
								connection.errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_DTCH_OVERFLOW);
							}
							break;
						}

						if (connection.data_channel->buffered_amount() + bufferLength > datachannel_threshold) {
							std::string bufferedAmt = "CPeerConnection::sendString - bufferedamount - " + std::to_string(connection.data_channel->buffered_amount() + bufferLength);
							logger->log(CLogger::LOG_WARNING, (char*)bufferedAmt.c_str());
							Sleep(datachannel_wait_time);
							retryCount++;
						}
						else
						{
							if (connection.data_channel->Send(buffer))
							{
								bytesSent = bufferLength;

								break;
							}
							else
							{
								logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendString - failed, data length %d", bytesSent);
							}
						}
					}
				}
				else
				{
					if (!isRunning)
					{
						logger->log(CLogger::LOG_ERROR, "Connection has not been initialized");
					}
					else if (connection.data_channel)
					{
						if (connection.data_channel->state() != prevDataChannelState)
						{
							logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendBytes - Data Channel State - %d", (int)connection.data_channel->state());
							prevDataChannelState = connection.data_channel->state();
						}
					}
					else
					{
						logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendBytes - Data Channel not initialized");
					}
				}
			}
			catch (std::exception &excep)
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendString - Exception - %s", std::string(excep.what()));
				bytesSent = -1;
			}

			return bytesSent;
		}

		/**
			Sends the number of bytes specified to the peer.

			@param sendBuf Bytes to be sent.
			@param bufferLength  Number of bytes to be sent.
			@return Number of bytes sent.
		*/
		int CPeerConnection::sendBytes(unsigned char* sendBuf, int bufferLength)
		{
			int bytesSent = 0;

			try
			{
				webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(sendBuf, bufferLength), true);
				if (isRunning && connection.data_channel && connection.data_channel->state() == 1)
				{
					int retryCount = 0;
					while (isRunning && connection.data_channel->state() == 1)
					{
						if (retryCount == datachannel_retry_count)
						{
							logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendBytes - Retry count reached");
							if (connection.errorHandler)
							{
								connection.errorHandler(ERROR_SEVERITY::ERROR_SEVERE, ERROR_DTCH_OVERFLOW);
							}
							break;
						}

						if (connection.data_channel->buffered_amount() + bufferLength > datachannel_threshold)
						{
							std::string bufferedAmt = "CPeerConnection::sendBytes - bufferedamount - " + std::to_string(connection.data_channel->buffered_amount() + bufferLength);
							logger->log(CLogger::LOG_WARNING, (char*)bufferedAmt.c_str());
							Sleep(datachannel_wait_time);
							retryCount++;
						}
						else
						{
							if (connection.data_channel->Send(buffer))
							{
								bytesSent = bufferLength;

								logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendString - data sent, data length %d", bytesSent);

								break;
							}
							else
							{
								logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendString - failed, data length %d", bytesSent);
							}
						}
					}
				}
				else
				{
					if (!isRunning)
					{
						logger->log(CLogger::LOG_ERROR, "Connection has not been initialized");
					}
					else if (connection.data_channel)
					{
						if (connection.data_channel->state() != prevDataChannelState)
						{
							logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendBytes - Data Channel State - %d", (int)connection.data_channel->state());
							prevDataChannelState = connection.data_channel->state();
						}
					}
					else
					{
						logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendBytes - Data Channel not initialized");
					}
				}
			}
			catch (std::exception &excep)
			{
				logger->log(CLogger::LOG_ERROR, "CPeerConnection::sendBytes - Exception - %s", std::string(excep.what()));
				bytesSent = -1;
			}

			return bytesSent;
		}

		/** 
			Close the peer connection.
			TODO WEBRTC: Cleanup is not documented online. Should be revisited.
		*/
		void CPeerConnection::close()
		{
			logger->log(CLogger::LOG_INFO, "CPeerConnection::close - Cleanup task initiated.");
			connection.connectionCleanup();
			//TODO WEBRTC: Should be revisited. Commenting it out since exe is blocked here sometimes.
			/*if (peer_connection_factory)
			{
				peer_connection_factory = nullptr;
			}

			if (thread)
			{
				thread->Quit();
				thread = nullptr;
			}*/
			
			if (webrtcFactoryThreadHandle != nullptr)
			{
				CloseHandle(webrtcFactoryThreadHandle);
				webrtcFactoryThreadHandle = nullptr;
			}
			isRunning = false;
			rtc::CleanupSSL();
			logger->log(CLogger::LOG_INFO, "CPeerConnection::close - Cleanup task completed");
		}

		/**
			Close the peer connection.
			TODO WEBRTC: Cleanup is not documented online. Should be revisited.
		*/
		void CPeerConnection::shutDownSocket()
		{
			logger->log(CLogger::LOG_INFO, "CPeerConnection::shutDownSocket - Cleanup task initiated.");
			connection.connectionCleanup();

			//TODO WEBRTC: Should be revisited. Commenting it out since exe is blocked here sometimes.
			/*if (peer_connection_factory)
			{
				peer_connection_factory = nullptr;
			}
			if (thread)
			{
				thread->Quit();
				thread = nullptr;
			}*/

			if (webrtcFactoryThreadHandle != nullptr)
			{
				CloseHandle(webrtcFactoryThreadHandle);
				webrtcFactoryThreadHandle = nullptr;
			}
			isRunning = false;
			rtc::CleanupSSL();
			logger->log(CLogger::LOG_INFO, "CPeerConnection::close - Cleanup task completed");
		}

		/**
			Creates instance of CPeerConnection.

			@return Webrtc handler instance.
		*/
		CPeerConnection* createPeerConnectionHandlerInstance(std::string logger_path)
		{
			webrtc = new CPeerConnection(logger_path);
			return webrtc;
		}

		/**
			Deletes instance of CPeerConnection.
		*/
		void deleteWebrtcInstance()
		{
			//TODO WEBRTC: Commenting out since exe is blocked here
			/*if (webrtc != NULL)
			{
				delete webrtc;
				webrtc = NULL;
			}*/
			logger->log(CLogger::LOG_INFO, "CPeerConnection::initialize - Log closed");
		}
	}
}