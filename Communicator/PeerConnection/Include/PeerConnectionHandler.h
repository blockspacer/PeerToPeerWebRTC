# ifndef PEERCONNECTION_HANDLER_H
# define PEERCONNECTION_HANDLER_H


# include "clientSocket\Connection.h"
# include <vector>
# include "ErrorMessages.h"
# include "ReferenceCounter.h"
# include "SocketStreamHandler.h"

using namespace Connection::ConnectionUtils;

class ICriticalSection;

class PeerConnectionHandler : public CErrorMessage,  
							  public SocketStreamHandler {

public:

	PeerConnectionHandler									(std::string serverHost,
															int serverPort,
															bool isSecureConnection, std::string cstrTrunServer, std::string log_path = "./peerconnection.log");

	~PeerConnectionHandler									();

	bool stop												();

	bool stopIncomingListener								();

	bool isRunning											();

	bool InitOffer											();

	static bool dataChannelStateReceiver					(char *command);

	static bool candidateReceiver							(char *command);

	static bool sdpReceiver									(char *command);

	static bool PeerConnectionErrorReceiver							(int severity, const char *errorString);

	void   setTurnCredentials								(const std::pair<std::string, std::string> turnCredentials);

	bool	setTurnInfo										(std::string server, std::string uname, std::string pass);

	bool	setPeerConnectionHandShakeMessage							(char *offer);

	bool	setPeerConnectionCandidate						(char *candidate);

	bool	setReceivers									(bool(sdpReceiver)(char*), bool(candidateReceiver)(char*), bool(dataChannelStateReceiver)(char*), bool(errorReceiver)(int, const char*));

	std::string getTurnServerUri()
	{
		return cstrTurnServerURI;
	}

	void setTurnServerUri(std::string uri)
	{
		cstrTurnServerURI = uri;
	}

private:

	std::string cstrTurnServerURI;

};

#endif // PEERCONNECTION_HANDLER_H