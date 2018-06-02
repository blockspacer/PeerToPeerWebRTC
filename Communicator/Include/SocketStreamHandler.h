# ifndef STREAM_SOCKET_HANDLER
# define STREAM_SOCKET_HANDLER


# include "clientSocket\Connection.h"
# include <vector>
# include "ErrorMessages.h"
# include "ReferenceCounter.h"

using namespace Connection::ConnectionUtils;


enum{
	SOCKET_TYPE_TCP,
	SOCKET_TYPE_WS,
	SOCKET_TYPE_P2P
};

enum{
	CONNECTION_MODE_DIRECT,
	CONNECTION_MODE_PROXY
};

# define DEFAULT_CONNECTION_MODE				CONNECTION_MODE_DIRECT
# define READ_BUFFER_LENGTH						1024 * 4096
# define SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT	4000

# define MIN_RECV_TIME							45
# define MAX_RECV_TIME							65
# define MAX_RECV_TIME_ON_P2P_ALIVE				120

class ICriticalSection;

class SocketStreamHandler : public CErrorMessage, public IReferenceCounter {

private:
	std::string loggerPath;
	std::string serverHostName;
	std::string serverEndpointUrl;

	int         serverPort;
	bool        isSecureConnection;
	bool 		isSharedLog;
	int         connectionMode;
	int         socketType;
	unsigned int	debug_print_level;
	bool		bEnableDebugMode;

	//Protocol based handling
	size_t readBytesTcp(BYTE* buf, size_t bufferLength);
	size_t readBytesWs(BYTE* buf, size_t bufferLength);


protected:

	bool        isConnected;
	bool		b_isSocketOpen;
	int         unreadIndex;
	int         bytesRemaining;
	int         bytesReceived;
	char        readBuffer[READ_BUFFER_LENGTH + 1];
	std::string dataBuffer;

public:

	SmartPtr<ICriticalSection> socketOperationLock;
	SmartPtr<ICriticalSection> socketShutdownLock;

public:

	SmartPtr<socketWrapper>   socketHandle;

	unsigned long long recv_time;

	//Constructors and Destructors
	SocketStreamHandler();

	SocketStreamHandler(std::string serverHost,
		int serverPort,
		bool isSecureConnection,
		int socketType,
		std::string endPointUrl = "",
		std::string logPath = "",
		bool isSharedLog = false);

	~SocketStreamHandler();


	bool        initializeSocketHandler();
	void        setProxyDetails(std::string proxyHost, int proxyPort, std::string proxyUser, std::string proxyPwd);
	void		ResetConnection(std::string hostName = "", int hostPort = 0, bool isSecureMode = true,
		std::string proxyHostName = "", int proxyPort = 0,
		std::string proxyUser = "", std::string proxyPassword = "");

	void		Reset();

	void		SetConnectionMode(bool isSecure);

	void		SetConnectionStatus(bool bConnected)
	{
		isConnected = bConnected;
	}

	void		setServerHostName(std::string);

	void		setServerPort(int);

	bool		startCommunication();

	bool        testConnectionWithTimeOut(char *hostName, int port, int timeOutSec);
	bool        ConnectToPeer();
	bool		ConnectDirectToPeer();
	bool        connectThroughProxy();
	bool		ConnectToPeerViaProxy();
	int         socketSend(char* buf, size_t len);
	int         socketSend(char* buf);
	int         socketSendBytes(BYTE* buf, size_t len);
	int			sendVectorAsync(std::vector<char> &vecData);
	size_t	    readLine(unsigned long long &cur_time);
	size_t      getDataBufferContent(std::string &);
	bool		IsDataDataAvailable();
	size_t      readBytes(BYTE* buf, size_t bufferLength);
	size_t      readBytesVector(std::vector<char>& buf, size_t bufferLength, unsigned long long &current_time);
	size_t      ReadBytes(BYTE* buf, size_t bufferLength);
	void        closeSocket();
	void		ShutDown();
	bool		checkAndRecoverSenderThread();

	unsigned long long GetLastReceivedTime()
	{
		return recv_time;
	}

	//void		setConnectionMode(bool isSSL);

	// Getter Setters
	std::string getServerHostName();
	int         getServerPort();
	bool        getConnectionStatus();
	int         getConnectionMode();
	int         getSocketType();

	bool		Restart();

	void		DestroyConnectionApp();

	void		SetDebugMode(unsigned int debug) ;
	bool		IsDebugPrintEnabled()     {return bEnableDebugMode;}

	bool		Restart(std::string host, int port = -1, bool isSecure = true);

	int			RunStreamReaderLoop();

	void SetConnectionParams(std::string serverHost, 
		int serverPort, 
		bool isSecureConnection, 
		int socketType, 
		std::string endPointUrl,
		std::string logPath = "",
		bool isSharedLog = false
		);

	void Initialize(std::string serverHost, 
		int serverPort, 
		bool isSecureConnection, 
		int socketType, 
		std::string endPointUrl,
		std::string logPath = "",
		bool isSharedLog = false
		);

	void RegisterForDataBufferCallback(void(*cbProcessor)(std::string &, unsigned long long &, SocketStreamHandler *))
	{
		cbCommandProcessor = cbProcessor;
	}

	bool		IsSecureMode();

protected:

	void(*cbCommandProcessor)(std::string &, unsigned long long &, SocketStreamHandler *); // command processor callback
};

#endif