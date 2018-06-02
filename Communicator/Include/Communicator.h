/* $Id$ */

#pragma once

# include "stdafx.h"
# include "SocketStreamHandler.h"
# include <Windows.h>
# include <string>
# include <list>
#include <vector>
# include "ReferenceCounter.h"
# include <atomic>

using namespace Concurrency;
using namespace ppl_extras;

class ICriticalSection;

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstring;

class Communicator : public virtual IReferenceCounter
{
private:

	std::string										currentHost;

	int												currentPort;

public:

	Communicator();

	~Communicator();

	bool PeerConnectionSDPOffer						(std::string &);

	bool PeerConnectionCandidateInfo				(std::string &);
    
	bool PeerConnectionErrorHandler					(std::string &);

	bool PeerConnectionDataChannelStateHandler		(std::string &);

	bool setPeerConnectionHandShakeMessage			(char* offer);

	bool setPeerConnectionCandidate					(char* candidate);

	bool StartPeerConnection						(const std::string cstrTurnServerURI, const std::pair<std::string, std::string> turnCredentials, \
													bool bInitOffer, bool(*cb_peer_connection_needed)(void));

private:

	SmartPtr<SocketStreamHandler>	_pSocketStreamHandler;
};