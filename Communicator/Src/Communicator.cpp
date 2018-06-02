/*
* @File : Communicator
* @Purpose : Handles peer to peer connection
* @Author : Ramesh Kumar K
*/

/**************************************************************************************************************************************************************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Headers
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include <list>
#include <winuser.h>
#include "Communicator.h"
#include "SmartPtr.h"
#include "ICriticalSection.h"
#include <Sensapi.h>
#include "SmartMutex.h"
#include <future>
#include "PeerConnectionHandler.h"
#include "EventHandler.h"

#pragma comment(lib, "Sensapi.lib")

Communicator::Communicator()
{
	__hook (&EventHandler::peer_connection_sdp_offer_handler, &event_handler, &Communicator::PeerConnectionSDPOffer);

	__hook (&EventHandler::peer_connection_candidate_info_handler, &event_handler, &Communicator::PeerConnectionCandidateInfo);

	__hook (&EventHandler::peer_connection_error_handler, &event_handler, &Communicator::PeerConnectionErrorHandler);

	__hook (&EventHandler::peer_connection_data_channel_state_handler, &event_handler, &Communicator::PeerConnectionDataChannelStateHandler);
}

Communicator::~Communicator()
{
	__unhook (&EventHandler::peer_connection_sdp_offer_handler, &event_handler, &Communicator::PeerConnectionSDPOffer);

	__unhook (&EventHandler::peer_connection_candidate_info_handler, &event_handler, &Communicator::PeerConnectionCandidateInfo);

	__unhook (&EventHandler::peer_connection_error_handler, &event_handler, &Communicator::PeerConnectionErrorHandler);

	__unhook (&EventHandler::peer_connection_data_channel_state_handler, &event_handler, &Communicator::PeerConnectionDataChannelStateHandler);
}

bool Communicator::PeerConnectionSDPOffer(std::string &sdp)
{
	// send the sdp offer in encoded format to other end through a signalling server. You can even copy and paste it on other client's input

	return false;
}

bool Communicator::PeerConnectionCandidateInfo(std::string &candidate)
{
	// send the ice candidates in encoded format to other end through a signalling server. You can even copy and paste it on other client's input

	return false;
}

bool Communicator::PeerConnectionErrorHandler(std::string &error)
{
	TRACE(_T("Peer connection error message received"));

	return true;
}

bool Communicator::PeerConnectionDataChannelStateHandler(std::string &state)
{
	TRACE(_T("Peer connection success message received"));

	return true;
}

bool Communicator::StartPeerConnection(const std::string cstrTurnServerURI, const std::pair<std::string, std::string> turnCredentials, \
										bool bInitOffer, bool(*cb_peer_connection_needed)(void))
{
	bool retVal = false;

	{
		TRACE(_T("Starting peer connection"));

		_pSocketStreamHandler = new PeerConnectionHandler(currentHost, currentPort, true, cstrTurnServerURI);

		if (_pSocketStreamHandler)
		{
			((PeerConnectionHandler *)(_pSocketStreamHandler.get()))->setTurnCredentials(turnCredentials);

			retVal = _pSocketStreamHandler->startCommunication();

			if (retVal && bInitOffer)
			{
				retVal = ((PeerConnectionHandler*)_pSocketStreamHandler.get())->InitOffer();
			}
		}
	}

	return retVal;
}

bool Communicator::setPeerConnectionHandShakeMessage(char* offer)
{

	bool result = false;

	if (_pSocketStreamHandler)
	{
		TRACE(_T("Setting SDP"));

		result = ((PeerConnectionHandler *)(_pSocketStreamHandler.get()))->setPeerConnectionHandShakeMessage(offer);
	}

	return result;
}

bool Communicator::setPeerConnectionCandidate(char* candidate)
{
	bool result = false;

	if (_pSocketStreamHandler)
	{
		TRACE (_T("Setting ICE candidates"));

		result = ((PeerConnectionHandler *)(_pSocketStreamHandler.get()))->setPeerConnectionCandidate(candidate);
	}

	return result;
}