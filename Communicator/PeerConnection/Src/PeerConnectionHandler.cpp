#include "stdafx.h"
#include "PeerConnectionHandler.h"
#include "logging.h"
#include "ICriticalSection.h"
#include "clientSocket\WebSocketImpl.h"
#include "clientSocket\TcpImpl.h"
#include "clientSocket\PeerConnectionImpl.h"
#include "SmartMutex.h"
#include "EventHandler.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Definitions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PeerConnectionHandler::PeerConnectionHandler(std::string serverHost,
											int serverPort,
											bool isSecureConnection,
											std::string strTurnserverUri,
											std::string logPath) :
	SocketStreamHandler(serverHost, serverPort, isSecureConnection, SOCKET_TYPE_P2P, "", logPath)
{
	cstrTurnServerURI = strTurnserverUri;

	initializeSocketHandler();

	setReceivers(PeerConnectionHandler::sdpReceiver, PeerConnectionHandler::candidateReceiver, PeerConnectionHandler::dataChannelStateReceiver, PeerConnectionHandler::PeerConnectionErrorReceiver);
}

PeerConnectionHandler::~PeerConnectionHandler()
{
	stop();
}

bool PeerConnectionHandler::setReceivers(bool(sdpReceiver)(char*), bool(candidateReceiver)(char*), bool(dataChannelStateReceiver)(char*), bool(errorReceiver)(int, const char*))
{
	((CPeerConnectionImpl*)(socketHandle.get()))->setSDPReceiver(sdpReceiver);
	((CPeerConnectionImpl*)(socketHandle.get()))->setCandidateReceiver(candidateReceiver);
	((CPeerConnectionImpl*)(socketHandle.get()))->setDataChannelStateReceiver(dataChannelStateReceiver);
	((CPeerConnectionImpl*)(socketHandle.get()))->setErrorReceiver(errorReceiver);

	return true;
}

bool PeerConnectionHandler::setTurnInfo(std::string server, std::string uname, std::string pass)
{
	((CPeerConnectionImpl*)(socketHandle.get()))->setTurnInfo(server, uname, pass);

	return true;
}

bool PeerConnectionHandler::setPeerConnectionHandShakeMessage(char *offer)
{
	((CPeerConnectionImpl*)(socketHandle.get()))->setSDPHandShakeMessage(offer);

	return true;
}

bool PeerConnectionHandler::setPeerConnectionCandidate(char *candidate)
{
	((CPeerConnectionImpl*)(socketHandle.get()))->setCandidate(candidate);

	return true;
}

bool PeerConnectionHandler::PeerConnectionErrorReceiver(int errorSeverity, const char* errorString)
{
	std::string errorMsg(errorString);

	LogCritical(_T("PeerConnectionErrorReceiver: Error Severity - %d, Error string - %s"), errorSeverity, CString(errorMsg.c_str()).GetBuffer());

	if (errorSeverity == 1)
	{
		event_handler.peer_connection_error_handler(errorMsg);
	}

	return true;
}

bool PeerConnectionHandler::InitOffer()
{
	if (socketHandle)
	{
		return ((CPeerConnectionImpl*)(socketHandle.get()))->InitSDPOffer();
	}

	return false;
}

bool PeerConnectionHandler::dataChannelStateReceiver(char *command)
{
	LogCritical(_T("Data receiver acknowledged...!"));
	
	event_handler.peer_connection_data_channel_state_handler(std::string(""));

	return true;
}

bool PeerConnectionHandler::sdpReceiver(char *sdp)
{
	std::string sdpStr;
	sdpStr = std::string(sdp);

	return event_handler.peer_connection_sdp_offer_handler(sdpStr);
}

bool PeerConnectionHandler::candidateReceiver(char *candidate)
{
	std::string candidateStr;
	candidateStr =  candidate;

	return event_handler.peer_connection_candidate_info_handler(candidateStr);
}

void PeerConnectionHandler::setTurnCredentials(const std::pair<std::string, std::string> turnCredentials)
{
	std::string turnServerUri = getTurnServerUri();

	setTurnInfo(turnServerUri.c_str(), turnCredentials.first, turnCredentials.second);
}

bool PeerConnectionHandler::stopIncomingListener()
{
	isConnected = false;
	return true;
}

bool PeerConnectionHandler::isRunning()
{
	return isConnected;
}

bool PeerConnectionHandler::stop()
{
	closeSocket();

	return true;
}





