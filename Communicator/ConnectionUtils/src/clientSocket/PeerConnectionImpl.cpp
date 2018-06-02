# include "clientSocket\PeerConnectionImpl.h"

namespace Connection {

	namespace ConnectionUtils {
		
		CPeerConnectionImpl::CPeerConnectionImpl(std::string log_path) :CSocket(), IReferenceCounter()
		{
			logger_path = log_path;

			initialize();
		}

		CPeerConnectionImpl::~CPeerConnectionImpl()
		{

		}

		/**
			Initializes webrtc instance.
		*/
		void CPeerConnectionImpl::initialize()
		{
			is_connected_to_peer = false;

			_pPeerConnection = createPeerConnectionHandlerInstance(logger_path);

			checkSenderThreadStatus();
		}

		/**
			Initates connection using webrtc.

			@return true on success and false on failure.
		*/
		bool CPeerConnectionImpl::connect()
		{
			is_connected_to_peer = false;

			try
			{
				if (_pPeerConnection)
				{
					is_connected_to_peer = _pPeerConnection->connect();
				}
				else
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::connect - Webrtcdll not initialized");
				}

				if (is_connected_to_peer == false)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CPeerConnectionImpl::connect - CreatePeerConnection failure");
				}
			}
			catch (std::exception &excep)
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::connect - Exception - %s", std::string(excep.what()));

				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			if (is_connected_to_peer)
			{
				is_connected_to_peer = checkSenderThreadStatus();
			}

			return is_connected_to_peer;
		}

		bool CPeerConnectionImpl::InitSDPOffer()
		{
			if (_pPeerConnection)
			{
				return _pPeerConnection->InitSDPOffer();
			}

			return false;
		}

		/**
			Initates connection using webrtc.

			@return true on success and false on failure.
		*/
		bool CPeerConnectionImpl::connectViaProxy()
		{
			bool is_connected_to_peer = false;

			try
			{
				is_connected_to_peer = connect();
			}
			catch (std::exception &excep)
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::connectViaProxy - Exception - %s", std::string(excep.what()));

				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			if (is_connected_to_peer)
			{
				is_connected_to_peer = checkSenderThreadStatus();
			}

			return is_connected_to_peer;
		}

		/**
			Register the sdp-answer receiver.

			@param sdpReceiver Function to receive the SDP answer.
			@return true
		*/
		bool CPeerConnectionImpl::setSDPReceiver(bool(*sdpReceiver)(char*))
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->setSDPReceiver(sdpReceiver);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setSDPReceiver - Webrtcdll not initialized");
			}

			return retVal;
		}

		/**
			Register the candidate receiver.

			@param candidateReceiver Function to receive the ICE candidates.
			@return true
		*/
		bool CPeerConnectionImpl::setCandidateReceiver(bool(*candidateReceiver)(char*))
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->setCandidateReceiver(candidateReceiver);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setCandidateReceiver - Webrtcdll not initialized");
			}

			return retVal;
		}

		/**
			Register the data channel state receiver.

			@param dataReceiver Function to receive the data channel state.
			@return true
		*/
		bool CPeerConnectionImpl::setDataChannelStateReceiver(bool(*dataReceiver)(char*))
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->setDataChannelStateReceiver(dataReceiver);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setDataChannelStateReceiver - Webrtcdll not initialized");
			}

			return retVal;
		}

		/**
			Register the error receiver.

			@param dataReceiver Function to receive any error during connection.
			@return true
		*/
		bool CPeerConnectionImpl::setErrorReceiver(bool(*errorReceiver)(int, const char*))
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->setErrorReceiver(errorReceiver);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setErrorReceiver - Webrtcdll not initialized");
			}

			return retVal;
		}

		/**
			Sets the turn server name and credentials.

			@param server Turn server name
			@param uname Username for connecting to turn server
			@param pass Password for connecting to turn server
		*/
		void CPeerConnectionImpl::setTurnInfo(std::string &server, std::string &uname, std::string &pass)
		{
			if (_pPeerConnection)
			{
				_pPeerConnection->setTurnInfo(server, uname, pass);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setTurnInfo - Webrtcdll not initialized");
			}
		}

		/**
			Sets the SDP offer from the remote peer to Remote description.

			@param parameter Offer json string from the peer.
			@return true on success, false on failure.
		*/
		bool CPeerConnectionImpl::setSDPHandShakeMessage(const std::string& parameter)
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->setSDPHandShakeMessage(parameter);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setSDPHandShakeMessage - Webrtcdll not initialized");
			}

			return retVal;
		}

		/**
			Sets the ICE candidate from the remote peer to Remote ICE candidates.

			@param parameter Offer json string from the peer.
			@return true on success, false on failure.
		*/
		bool CPeerConnectionImpl::setCandidate(const std::string& parameter)
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->setCandidate(parameter);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::setCandidate - Webrtcdll not initialized");
			}

			return retVal;
		}

		int CPeerConnectionImpl::receiveString(char* receiveBuf, int bufferSize)
		{
			int receivedLength = 0;

			if (_pPeerConnection)
			{
				receivedLength = _pPeerConnection->receiveString(receiveBuf, bufferSize);
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::receiveString - Webrtcdll not initialized");
			}

			return receivedLength;
		}

		int CPeerConnectionImpl::receiveBytes(unsigned char* receiveBuf, int bufferSize)
		{
			int receivedLength = -1;

			try
			{
				if (_pPeerConnection)
				{
					receivedLength = _pPeerConnection->receiveBytes(receiveBuf, bufferSize);
				}
				else
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::receiveBytes - Webrtcdll not initialized");
				}
			}
			catch (std::exception &excep)
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::receiveBytes - Exception - %s", std::string(excep.what()));
				receivedLength = -1;

				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}
			return receivedLength;
		}

		int CPeerConnectionImpl::sendString(char* sendBuf)
		{
			bool retVal = false;

			if (_pPeerConnection)
			{
				retVal = _pPeerConnection->sendString(sendBuf, strlen(sendBuf));
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::sendString - Webrtcdll not initialized");
			}

			return retVal;
		}

		int CPeerConnectionImpl::sendString(char* sendBuf, int bufferLength)
		{
			int bytesSent = 0;

			try
			{
				if (_pPeerConnection)
				{
					bytesSent = _pPeerConnection->sendString(sendBuf, bufferLength);
				}
				else
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::sendString - Webrtcdll not initialized");
				}
			}
			catch (std::exception &excep)
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::sendString - Exception - %s", std::string(excep.what()));
				bytesSent = -1;

				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return bytesSent;
		}

		int CPeerConnectionImpl::sendBytes(unsigned char* sendBuf, int bufferLength)
		{
			int bytesSent = 0;

			try
			{
				if (_pPeerConnection)
				{
					bytesSent = _pPeerConnection->sendBytes(sendBuf, bufferLength);
				}
				else
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::sendBytes - Webrtcdll not initialized");
				}
			}
			catch (std::exception &excep)
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::sendBytes - Exception - %s", std::string(excep.what()));
				bytesSent = -1;

				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return bytesSent;
		}

		/**
			Closes and deletes webrtc instance.
		*/
		void CPeerConnectionImpl::close()
		{
			if (_pPeerConnection)
			{
				_pPeerConnection->close();

				deleteWebrtcInstance();

				_pPeerConnection = NULL;
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::close - Webrtcdll not initialized");
			}
		}

		/**
			Closes and deletes webrtc instance.
		*/
		void CPeerConnectionImpl::shutDownSocket()
		{
			if (_pPeerConnection)
			{
				_pPeerConnection->close();

				deleteWebrtcInstance();

				_pPeerConnection = NULL;
			}
			else
			{
				sockUtilLogger->log(CLogger::LOG_ERROR, "CPeerConnectionImpl::shutDownSocket - Webrtcdll not initialized");
			}
		}

		bool CPeerConnectionImpl::isSocketLive()
		{
			return (_pPeerConnection != NULL);
		}
	}
}