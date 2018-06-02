#include "stdafx.h"
#include "SocketStreamHandler.h"
#include "ICriticalSection.h"
#include "clientSocket\WebSocketImpl.h"
#include "clientSocket\TcpImpl.h"
#include "clientSocket\PeerConnectionImpl.h"
#include "SmartMutex.h"
#include <chrono>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

# define CONNECTION_LOG "\\log\\Connection.log"
# define PEER_CONNECTION_HANDLER_LOG "\\log\\PeerConnection.log"

// Gets the current time in milliseconds

inline  unsigned long long GetCurrentTimeMillis()
{

#if 0
	// This will to give the time accurate to nano seconds
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	unsigned long long time = ft.dwHighDateTime;
	time <<= 32;
	time |= ft.dwLowDateTime;
	time /= 10;
	time -= 11644473600000000ULL;

	return time / 1000;
#endif

	const unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return now;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Definitions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SocketStreamHandler::SocketStreamHandler(std::string serverHost, 
		int serverPort, 
		bool isSecureConnection, 
		int socketType, 
		std::string endPointUrl,
		std::string logPath,
		bool isSharedLog)
{
	this->serverHostName     = serverHost;
	this->serverPort         = serverPort;
	this->isSecureConnection = isSecureConnection;
	this->socketType         = socketType;
	this->serverEndpointUrl  = endPointUrl;
	this->loggerPath         = logPath;
	this->isSharedLog        = isSharedLog;
	this->b_isSocketOpen	 = false;

	this->socketHandle       = NULL;
	this->unreadIndex        = 0;
	this->bytesRemaining     = 0;
	this->bytesReceived      = 0;
	this->isConnected        = false;
	this->connectionMode     = DEFAULT_CONNECTION_MODE;

	socketShutdownLock		= new ICriticalSection(SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT);
	socketOperationLock		= new ICriticalSection(SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT);

	cbCommandProcessor		= NULL;
}



SocketStreamHandler::SocketStreamHandler()
{
	this->socketHandle       = NULL;
	this->unreadIndex        = 0;
	this->bytesRemaining     = 0;
	this->bytesReceived      = 0;
	this->isConnected        = false;
	this->b_isSocketOpen	 = false;
	this->isSharedLog		 = false;

	socketShutdownLock = new ICriticalSection(SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT);
	socketOperationLock = new ICriticalSection(SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT);
	cbCommandProcessor = NULL;
}




void SocketStreamHandler::Initialize(std::string serverHost, 
		int serverPort, 
		bool isSecureConnection, 
		int socketType, 
		std::string endPointUrl,
		std::string logPath,
		bool isSharedLog)
{
	this->serverHostName     = serverHost;
	this->serverPort         = serverPort;
	this->isSecureConnection = isSecureConnection;
	this->socketType         = socketType;
	this->serverEndpointUrl  = endPointUrl;
	this->loggerPath         = logPath;
	this->isSharedLog        = isSharedLog;
	b_isSocketOpen			 = false;

	// Other Initializations

	this->socketHandle       = NULL;
	this->unreadIndex        = 0;
	this->bytesRemaining     = 0;
	this->bytesReceived      = 0;
	this->isConnected        = false;
	this->connectionMode     = DEFAULT_CONNECTION_MODE;
	debug_print_level		 = 0;
	bEnableDebugMode		 = false;

	socketShutdownLock = new ICriticalSection(SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT);
	socketOperationLock = new ICriticalSection(SOCKET_CRITICAL_SECTION_SPIN_TIMEOUT);
	cbCommandProcessor = NULL;
}

void SocketStreamHandler::SetConnectionParams(std::string serverHost, 
		int serverPort, 
		bool isSecureConnection, 
		int socketType, 
		std::string endPointUrl,
		std::string logPath,
		bool isSharedLog
		)
{
	this->serverHostName     = serverHost;
	this->serverPort         = serverPort;
	this->isSecureConnection = isSecureConnection;
	this->socketType         = socketType;
	this->serverEndpointUrl  = endPointUrl;
	this->isSharedLog        = isSharedLog;

	this->loggerPath         = logPath;
	this->isSharedLog        = isSharedLog;
}


void SocketStreamHandler::Reset()
{
	__try
	{
		socketHandle->initialize();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		SET_ERROR(_T("ResetConnection: Exception occurred in ResetConnection"));
	}
}

void SocketStreamHandler::ResetConnection(std::string hostName, int hostPort, bool isSecureMode,
				std::string proxyHostName, int proxyPort, 
				std::string proxyUser, std::string proxyPassword)
{
	{
		LockGuard<ICriticalSection> operationLock(*(socketOperationLock.get()));

		{
			this->serverHostName = hostName;
			this->serverPort = hostPort;
			this->isSecureConnection = isSecureMode;

			socketHandle->setConnectionDetails(hostName, hostPort, isSecureMode, proxyHostName, proxyPort, proxyUser, proxyPassword);

			Reset();
		}
	}
}

void	SocketStreamHandler::SetConnectionMode(bool isSecure)
{
	if(socketHandle != NULL)
	{
		socketHandle->setConnectionMode(isSecure);
	}
	else
	{
		TRACE((_T("SetConnectionMode: Socket Handle not initialized"));
	}
}


void SocketStreamHandler::setServerHostName(std::string hostName)
{
	if(socketHandle != NULL)
	{
		socketHandle->setServerHostName(hostName);
	}
	else
	{
		TRACE(_T("setServerHostName: Socket Handle not initialized"));
	}
}

void SocketStreamHandler::setServerPort(int hostPort)
{
	if(socketHandle != NULL)
	{
		socketHandle->setServerPort(hostPort);
	}
	else
	{
		TRACE(_T("setServerPort: Socket Handle not initialized"));
	}
}

void SocketStreamHandler::SetDebugMode(unsigned int level)
{
	debug_print_level = level;

	if (debug_print_level > 0)
	{
		bEnableDebugMode = true;
	}
}

SocketStreamHandler::~SocketStreamHandler()
{
	if (socketHandle)
	{
		socketHandle->DestroySocket();
		socketHandle = NULL;
	}
}


void SocketStreamHandler::DestroyConnectionApp()
{
	__try
	{
		recv_time = INFINITE;

		this->unreadIndex = 0;
		this->bytesRemaining = 0;
		this->bytesReceived = 0;

		if (socketHandle)
		{
			this->ShutDown();
			socketHandle = NULL;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		SET_ERROR(_T("ConnectToPeer: Exception occurred in ConnectToPeer"));
	}
}

bool SocketStreamHandler::Restart()
{
	bool initializationResult = false;

	{
		LockGuard<ICriticalSection> operationLock(*(socketOperationLock.get()));

		{
			LockGuard<ICriticalSection> lock(*(socketShutdownLock.get())); //! brief pass the mutex to the smartMutex to create exception free one

			TRACE(_T("Restart::Initializing ..."));


			DestroyConnectionApp();

			if (socketType == SOCKET_TYPE_WS)
			{
				TRACE(_T("Restart: Creating WebSocket Connection %s:%d SSL - %d, ServerUrl - %s"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection, CString(serverEndpointUrl.c_str()).GetBuffer());

				{
					socketHandle = (CSocket *) new CWebSocketImpl(serverHostName, serverPort, isSecureConnection, serverEndpointUrl, loggerPath + std::string(CONNECTION_LOG) , isSharedLog);
				}
			}
			else if (socketType == SOCKET_TYPE_TCP)
			{
				TRACE(_T("Restart: Creating TCP Socket Connection %s:%d isSecureConnection - %d"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection);


				{
					socketHandle = (CSocket *) new CTCPImpl(serverHostName, serverPort, isSecureConnection, loggerPath + std::string(CONNECTION_LOG), isSharedLog);
				}
			}
			else if (socketType == SOCKET_TYPE_P2P)
			{
				TRACE(_T("Restart: Creating P2P Socket Connection %s:%d isSecureConnection - %d"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection);

				socketHandle = (CSocket *) new CPeerConnectionImpl(loggerPath + std::string(PEER_CONNECTION_HANDLER_LOG));
			}

			if (socketHandle != NULL)
			{
				initializationResult = true;

				TRACE(_T("Restart: Client Socket Initialized Successfully"));
			}
			else
			{
				SET_ERROR(_T("Restart: Error while initializing WebSocket Client"));
			}
		}
	}

	return initializationResult;
}


bool SocketStreamHandler::Restart(std::string host, int port, bool isSecure)
{
	bool initializationResult = false;

	{
		LockGuard<ICriticalSection> operationLock(*(socketOperationLock.get()));

		{
			LockGuard<ICriticalSection> lock(*(socketShutdownLock.get())); //! brief pass the mutex to the smartMutex to create exception free one

			{
				recv_time = INFINITE;

				TRACE(_T("Restart::Closing the socket ..."));

				DestroyConnectionApp();

				port ? serverPort = port : __noop;
				this->serverPort = isSecure;
				this->serverHostName = host;

				TRACE(_T("Restart::Initializing ..."));

				if (socketType == SOCKET_TYPE_WS)
				{
					TRACE(_T("Restart: Creating WebSocket Connection %s:%d SSL - %d, ServerUrl - %s"), CString(host.c_str()).GetBuffer(), serverPort, isSecureConnection, CString(serverEndpointUrl.c_str()).GetBuffer());


					{
						socketHandle = (CSocket *) new CWebSocketImpl(host, serverPort, isSecureConnection, serverEndpointUrl, loggerPath + std::string(CONNECTION_LOG), isSharedLog);
					}
				}
				else if (socketType == SOCKET_TYPE_TCP)
				{
					TRACE(_T("Restart: Creating TCP Socket Connection %s:%d isSecureConnection - %d"), CString(host.c_str()), serverPort, isSecureConnection);


					{
						socketHandle = (CSocket *) new CTCPImpl(host, serverPort, isSecureConnection, loggerPath + std::string(CONNECTION_LOG), isSharedLog);
					}
				}
				else if (socketType == SOCKET_TYPE_P2P)
				{
					TRACE(_T("Restart: Creating P2P Socket Connection %s:%d isSecureConnection - %d"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection);

					socketHandle = (CSocket *) new CPeerConnectionImpl(loggerPath + std::string(PEER_CONNECTION_HANDLER_LOG));
				}

				if (socketHandle != NULL)
				{
					initializationResult = true;

					TRACE(_T("Restart: Client Socket Initialized Successfully"));
				}
				else
				{
					SET_ERROR(_T("Restart: Error while initializing WebSocket Client"));
				}
			}
		}
	}

	return initializationResult;
}



bool SocketStreamHandler::initializeSocketHandler()
{
	bool initializationResult = false;


	socketHandle = NULL;

	recv_time = INFINITE;

	LockGuard<ICriticalSection> operationLock(*(socketOperationLock.get()));

	TRACE( _T("initializeSocketHandler::Initializing ..."));

	if(socketType == SOCKET_TYPE_WS)
	{
		TRACE( _T("initializeSocketHandler: Creating WebSocket Connection %s:%d SSL - %d, ServerUrl - %s"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection, CString(serverEndpointUrl.c_str()).GetBuffer());

		socketHandle = (CSocket *) new CWebSocketImpl(serverHostName, serverPort, isSecureConnection, serverEndpointUrl, loggerPath + std::string(CONNECTION_LOG), isSharedLog);
	}
	else if(socketType == SOCKET_TYPE_TCP)
	{
		TRACE(_T("initializeSocketHandler: Creating TCP Socket Connection %s:%d isSecureConnection - %d"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection);
		socketHandle = (CSocket *) new CTCPImpl(serverHostName, serverPort, isSecureConnection, loggerPath + std::string(CONNECTION_LOG), isSharedLog);
	}
	else if (socketType == SOCKET_TYPE_P2P)
	{
		TRACE(_T("Restart: Creating P2P Socket Connection %s:%d isSecureConnection - %d"), CString(serverHostName.c_str()).GetBuffer(), serverPort, isSecureConnection);

		socketHandle = (CSocket *) new CPeerConnectionImpl(loggerPath + std::string(PEER_CONNECTION_HANDLER_LOG));
	}

	if(socketHandle != NULL)
	{
		initializationResult = true;

		TRACE(_T("initializeSocketHandler: Client Socket Initialized Successfully"));
	}
	else
	{
		SET_ERROR(_T("initializeSocketHandler: Error while initializing WebSocket Client"));
	}

	return initializationResult;
}

bool SocketStreamHandler::ConnectDirectToPeer()
{
	bool connectResult = false;

	__try
	{
		b_isSocketOpen = true;

		if (socketHandle != NULL)
		{
			connectResult = socketHandle->connect();
		}
		else
		{
			SET_ERROR(_T("ConnectToPeer: Socket Handle is NULL"));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		SET_ERROR(_T("ConnectToPeer: Exception occurred in ConnectToPeer"));
	}

	return connectResult;
}

bool SocketStreamHandler::startCommunication()
{
	return ConnectToPeer();
}

bool SocketStreamHandler::ConnectToPeer()
{
	bool connectResult = false;

	{
		LockGuard<ICriticalSection> operationLock(*(socketOperationLock.get()));

		connectResult = ConnectDirectToPeer();

		if (connectResult == false)
		{
			closeSocket();

			TRACE(_T("ConnectToPeer: Socket Connect Failed Stream handler error message - %s, Socket Util Error Msg - %d:%s"),  GetErrorMessage(), socketHandle->getLastError(), CString(socketHandle->getLastErrorMsg().c_str()).GetBuffer());
		}
		else
		{
			isConnected = true;
			connectionMode = CONNECTION_MODE_DIRECT;

			TRACE(_T("ConnectToPeer: Successfully connected to Peer %s"), CString(socketHandle->getServerHostName().c_str()).GetBuffer());
		}
	}

	return connectResult;
}

void SocketStreamHandler::setProxyDetails(std::string proxyHost, int proxyPort, std::string proxyUser, std::string proxyPwd)
{
	if(socketHandle != NULL)
	{
		socketHandle->setProxyDetails(proxyHost, proxyPort, proxyUser, proxyPwd);
	}
	else
	{
		TRACE(_T("setProxyDetails: Socket Handle not initialized"));
	}
}


bool SocketStreamHandler::IsSecureMode()
{
	return socketHandle->isSecureMode();
}

bool SocketStreamHandler::ConnectToPeerViaProxy()
{
	bool connectResult = false;

	__try
	{
		b_isSocketOpen = true;

		if (socketHandle != NULL)
		{
			connectResult = socketHandle->connectViaProxy();
		}
		else
		{
			SET_ERROR(_T("connectThroughProxy: Socket Handle is NULL"));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		SET_ERROR(_T("ConnectToPeer: Exception occurred in connectThroughProxy"));
	}

	return connectResult;
}

bool SocketStreamHandler::connectThroughProxy()
{
	bool connectResult = false;

	{
		LockGuard<ICriticalSection> operationLock(*(socketOperationLock.get()));

		connectResult = ConnectToPeerViaProxy();

		if (connectResult == false)
		{
			closeSocket();

			TRACE(_T("connectThroughProxy: Socket Connect Failed - Stream handler error message - %s, Socket Util Error Msg - %s"), GetErrorMessage(), CString(socketHandle->getLastErrorMsg().c_str()).GetBuffer());
		}
		else
		{
			isConnected = true;
			connectionMode = CONNECTION_MODE_PROXY;

			TRACE(_T("connectThroughProxy: Successfully connected to Peer %s"), CString(socketHandle->getServerHostName().c_str()).GetBuffer());
		}
	}

	return connectResult;
}

bool SocketStreamHandler::checkAndRecoverSenderThread()
{
	if(isConnected)
	{
		return socketHandle->checkSenderThreadStatus();
	}

	return false;
}

int SocketStreamHandler::socketSend(char* buf, size_t len)
{
	int bytesSent = 0;

	if(isConnected)
	{
		bytesSent = socketHandle->sendStringAsync(buf, len);
	}
	else if (!isConnected)
	{
		TRACE( _T("socketSend: Socket Not ready for Send, Not connected to the server"));
	}
	else if (buf == NULL)
	{
		TRACE(_T("socketSend: Data is null"));
	}
	else
	{
		TRACE(_T("socketSend: Socket object is not available"));
	}

	return bytesSent;
}


int SocketStreamHandler::socketSend(char* buf)
{
	return socketSend( buf, strlen(buf) );
}


int SocketStreamHandler::sendVectorAsync(std::vector<char> &vecData)
{
	int bytesSent = 0;

	if(isConnected)
	{
		if ((vecData.size() > 0))
		{
			bytesSent = socketHandle->sendVectorAsync(vecData);
		}
		else
		{
			TRACE(_T("sendVectorAsync: Data is null"));
		}
	}
	else
	{
		if (!isConnected)
		{
			TRACE( _T("sendVectorAsync: Socket Not ready for Send, Not connected to the server"));
		}
		else
		{
			TRACE(_T("sendVectorAsync: Socket object is not available"));
		}
	}

	return bytesSent; 
}

int SocketStreamHandler::socketSendBytes(BYTE* buf, size_t len)
{
	int bytesSent = 0;


	if(isConnected)
	{

		bytesSent = socketHandle->sendBytesAsync(buf, len);

	}
	else if (!isConnected)
	{
		TRACE(_T("socketSendBytes: Socket Not ready for Send, Not connected to the server"));
	}
	else if (buf == NULL)
	{
		TRACE(_T("socketSendBytes: Data is null"));
	}
	else
	{
		TRACE(_T("socketSendBytes: Socket object is not available"));
	}

	return bytesSent;
}

size_t SocketStreamHandler::readLine ( unsigned long long &cur_time )
{
	int num_of_delimiters			= 0;
	int bytesProcessed				= 0 ;

	dataBuffer.clear();

	{
		if ( bytesRemaining > 0 )
		{
			int cur_index = unreadIndex;

			for ( bytesProcessed = 0 ; bytesProcessed < bytesRemaining ; bytesProcessed++, cur_index++ )
			{
				if ( ( '\n' == readBuffer[cur_index] ) || ( '\r' == readBuffer[cur_index]) )
				{
					num_of_delimiters++ ;

					if ( '\r' == readBuffer[cur_index] && (bytesProcessed + 1) < bytesRemaining && '\n' == readBuffer[cur_index + 1] )
					{
						num_of_delimiters++ ;
					}

					break ;

				 }
			 }

			dataBuffer += std::string(readBuffer + unreadIndex, bytesProcessed);

			if ( bytesProcessed < bytesRemaining ) 
			{
				bytesRemaining = bytesRemaining - (bytesProcessed + num_of_delimiters) ;
				unreadIndex = unreadIndex + bytesProcessed + num_of_delimiters;

				goto receive_complete;
			}
			else
			{	
				bytesRemaining = unreadIndex = 0 ;
			}
		 }

	}
	
    // 2. If newline/carriage return was not found above, then read more data
    // from client. Keep reading until a newline character is encountered.
	{

		bytesReceived				= 0 ;
		bytesProcessed				= 0 ;

		while ( isConnected )
		{
			bytesReceived = socketHandle->receiveString(readBuffer, READ_BUFFER_LENGTH);
    	
			if( bytesReceived == -1 )
			{
				TRACE( _T("readLine: ConnectionUtils readString returned -1"));

				std::string error = socketHandle->getLastErrorMsg();

				CString last_error = CString(error.c_str());
				TRACE( _T("readLine: Error Msg - %s"), last_error.GetBuffer());

				return -1;
			}
			else if( bytesReceived == 0 )
			{
				TRACE( _T("readLine: ConnectionUtils readString returned 0"));

				std::string error = socketHandle->getLastErrorMsg();

				CString last_error = CString(error.c_str());
				TRACE( _T("readLine: Error Msg - %s"), last_error.GetBuffer());

				return -1;
			}
		
			num_of_delimiters = 0;

			if ( bytesReceived > 0 )
			{
				for ( bytesProcessed = 0 ; bytesProcessed < bytesReceived ; bytesProcessed++ )
				{
					if ( ( '\n' == readBuffer[bytesProcessed] ) || ( '\r' == readBuffer[bytesProcessed]) )
					{
						num_of_delimiters++ ;

						if ( '\r' == readBuffer[bytesProcessed] && (bytesProcessed + 1) < bytesReceived && '\n' == readBuffer[bytesProcessed + 1] )
						{
							num_of_delimiters++ ;
						}

						goto received_line ;

					}
				}

				dataBuffer += std::string(readBuffer, bytesProcessed);
			}
		}

received_line:

		dataBuffer += std::string(readBuffer, bytesProcessed);

		// some data is unread in readBuffer
		if ( bytesProcessed < bytesReceived ) 
		{
			bytesRemaining = bytesReceived - (bytesProcessed + num_of_delimiters) ;
			unreadIndex = bytesProcessed + num_of_delimiters;
		}
		else
		{	
			bytesRemaining = unreadIndex = 0 ;
		}

	}

receive_complete:
	
	cur_time = recv_time = GetCurrentTimeMillis ( ) ;

	return dataBuffer.length ( ) ;
}

size_t SocketStreamHandler::ReadBytes(BYTE* buf, size_t bufferLength)
{
	if((socketType == SOCKET_TYPE_WS) || (isSecureConnection == true))
	{
		return readBytesWs(buf, bufferLength);
	}
	else if(socketType == SOCKET_TYPE_TCP)
	{
		return readBytesTcp(buf, bufferLength);
	}

	return 0;
}


size_t SocketStreamHandler::readBytesTcp(BYTE* buf, size_t bufferLength)
{
	size_t bufferOffset = 0;

	//1. Read any unread data from readBuffer
    if ((bytesRemaining != 0))
	{
		if ((bytesRemaining <= bufferLength))
		{
			bufferOffset = bytesRemaining;
			bytesRemaining = 0;
		}
		else
		{
			bufferOffset = bufferLength;
			bytesRemaining = bytesRemaining - bufferLength;
		}

		std::move( readBuffer + unreadIndex, readBuffer + unreadIndex + bufferOffset, buf );

		unreadIndex += bufferOffset;
	}

    while(isConnected && (bufferOffset < bufferLength))
    {
    	bytesReceived = socketHandle->receiveString(readBuffer, READ_BUFFER_LENGTH);
    
    	if( (bytesReceived > 0) && ((bufferOffset + bytesReceived) <= bufferLength) )
    	{
    		//memcpy ( buf + bufferOffset, readBuffer, bytesReceived );

			std::move( readBuffer, readBuffer + bytesReceived, buf + bufferOffset );

    		bytesRemaining = unreadIndex = 0 ;
    		bufferOffset += bytesReceived;

    		if(bufferOffset == bufferLength)
    		{
    			break;
    		}
    	}
    	else if( (bytesReceived > 0) && ((bufferOffset + bytesReceived) > bufferLength) )
    	{
    		int bytesToCopy = bufferLength - bufferOffset;

			//memcpy ( buf + bufferOffset, readBuffer, bytesToCopy );
			std::move( readBuffer, readBuffer + bytesToCopy, buf + bufferOffset );

    		bufferOffset = bufferOffset + bytesToCopy;
    		unreadIndex = bytesToCopy; 
    		bytesRemaining = bytesReceived - bytesToCopy;
    		break;
    	}
    	else if(bytesReceived <= 0)
    	{
			TRACE( _T("readBytes: ConnectionUtils readString returned lessthan 0. ErrMsg - %s"), CString(socketHandle->getLastErrorMsg().c_str()).GetBuffer());
    		

    		return -1;
    	}
    }

	recv_time = GetCurrentTimeMillis( ) ;

    return bufferOffset;
}




size_t SocketStreamHandler::readBytesWs(BYTE* buf, size_t bufferLength)
{
	size_t bytesRead = -1;

	if(isConnected)
	{
		bytesRead = socketHandle->receiveBytes(buf, bufferLength);

		if(bytesRead <= 0)
		{
			TRACE(_T("SocketStreamHandler::readBytes - receiveBytes returned - %d"), bytesRead);
			

			return -1;
		}
	}
	else
	{
		TRACE(_T("SocketStreamHandler::readBytes - Socket already closed"));
	}

	recv_time = GetCurrentTimeMillis ( ) ;

	return bytesRead;
}

size_t SocketStreamHandler::readBytesVector(std::vector<char> &buf, size_t bufferLength, unsigned long long &current_time)
{
	size_t bufferOffset = 0;
    
	if ((bytesRemaining != 0))
	{
		if ((bytesRemaining <= bufferLength))
		{
			bufferOffset = bytesRemaining;
			bytesRemaining = 0;
		}
		else
		{
			bufferOffset = bufferLength;
			bytesRemaining = bytesRemaining - bufferLength;
		}

		buf.assign(readBuffer + unreadIndex, readBuffer + unreadIndex + bufferOffset) ;

		unreadIndex += bufferOffset;
	}

    while( isConnected && (bufferOffset < bufferLength) )
    {
    	bytesReceived = socketHandle->receiveString(readBuffer, READ_BUFFER_LENGTH);
    
    	if( (bytesReceived > 0) && ((bufferOffset + bytesReceived) <= bufferLength) )
    	{
    		//memcpy ( buf + bufferOffset, readBuffer, bytesReceived );
			buf.insert(buf.end(), readBuffer,readBuffer + bytesReceived );
    		bytesRemaining = unreadIndex = 0 ;
    		bufferOffset += bytesReceived;

    		if(bufferOffset == bufferLength)
    		{
    			break;
    		}
    	}
    	else if( (bytesReceived > 0) && ((bufferOffset + bytesReceived) > bufferLength) )
    	{
    		int bytesToCopy = bufferLength - bufferOffset;
    		//memcpy ( buf + bufferOffset, readBuffer, bytesToCopy );
			buf.insert(buf.end(), readBuffer,readBuffer + bytesToCopy );
    		bufferOffset = bufferOffset + bytesToCopy;
    		unreadIndex = bytesToCopy; 
    		bytesRemaining = bytesReceived - bytesToCopy;
    		break;
    	}
    	else if(bytesReceived <= 0)
    	{
			TRACE( _T("readBytes: ConnectionUtils readString returned lessthan 0. ErrMsg - %s"), CString(socketHandle->getLastErrorMsg().c_str()).GetBuffer());
    		
			return -1;
    	}
    }

	current_time = recv_time = GetCurrentTimeMillis ( ) ;

    return bufferOffset;
}


size_t SocketStreamHandler::readBytes(BYTE* buf, size_t bufferLength)
{
	size_t bufferOffset = 0;

	//1. Read any unread data from readBuffer
    
	if ((bytesRemaining != 0))
	{
		if ((bytesRemaining <= bufferLength))
		{
			bufferOffset = bytesRemaining;
			bytesRemaining = 0;
		}
		else
		{
			bufferOffset = bufferLength;
			bytesRemaining = bytesRemaining - bufferLength;
		}

		std::move( readBuffer + unreadIndex, readBuffer + unreadIndex + bufferOffset, buf );

		unreadIndex += bufferOffset;
	}

    while( isConnected && (bufferOffset < bufferLength) )
    {
    	bytesReceived = socketHandle->receiveString(readBuffer, READ_BUFFER_LENGTH);
    
    	if( (bytesReceived > 0) && ((bufferOffset + bytesReceived) <= bufferLength) )
    	{
    		//memcpy ( buf + bufferOffset, readBuffer, bytesReceived );
			std::move( readBuffer, readBuffer + bytesReceived, buf + bufferOffset );
    		bytesRemaining = unreadIndex = 0 ;
    		bufferOffset += bytesReceived;

    		if(bufferOffset == bufferLength)
    		{
    			break;
    		}
    	}
    	else if( (bytesReceived > 0) && ((bufferOffset + bytesReceived) > bufferLength) )
    	{
    		int bytesToCopy = bufferLength - bufferOffset;
    		//memcpy ( buf + bufferOffset, readBuffer, bytesToCopy );
			std::move( readBuffer, readBuffer + bytesToCopy, buf + bufferOffset );
    		bufferOffset = bufferOffset + bytesToCopy;
    		unreadIndex = bytesToCopy; 
    		bytesRemaining = bytesReceived - bytesToCopy;
    		break;
    	}
    	else if(bytesReceived <= 0)
    	{
			TRACE( _T("readBytes: ConnectionUtils readString returned lessthan 0. ErrMsg - %s"), CString(socketHandle->getLastErrorMsg().c_str()).GetBuffer());
    		
			return -1;
    	}
    }

	recv_time = GetCurrentTimeMillis ( ) ;

    return bufferOffset;
}


void SocketStreamHandler::ShutDown()
{
	__try
	{
		if (b_isSocketOpen)
		{
			if (socketHandle != NULL)
			{
				socketHandle->DestroySocket();

				socketHandle->shutDownSocket();

				isConnected = b_isSocketOpen = false;
			}
			else
			{
				SET_ERROR(_T("closeSocket: socketHandle passed is NULL"));
			}
		}

		recv_time = INFINITE;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		SET_ERROR(_T("closeSocket: Exception occurred in closeSocket"));
	}
}


void SocketStreamHandler::closeSocket()
{
	{
		LockGuard<ICriticalSection> lock(*(socketShutdownLock.get())); //! brief pass the mutex to the smartMutex to create exception free one

		ShutDown();

		TRACE(_T("closeSocket: socket has been shutdown"));
	}
}


size_t SocketStreamHandler::getDataBufferContent(std::string &data)
{
	size_t len = dataBuffer.length( ) + 1 ;

	data = dataBuffer;

	return len;
}

bool SocketStreamHandler::IsDataDataAvailable()
{
	size_t len = dataBuffer.length( );

	return (len > 0);
}

int SocketStreamHandler::getServerPort()
{
	return serverPort;
}

std::string SocketStreamHandler::getServerHostName()
{
	return serverHostName;
}

bool SocketStreamHandler::getConnectionStatus()
{
	return isConnected;
}

int SocketStreamHandler::getConnectionMode()
{
	// Direct or Via Proxy
	return connectionMode;
}

int SocketStreamHandler::getSocketType()
{
	return socketType;
}

int SocketStreamHandler::RunStreamReaderLoop()
{
	int bytesRead = -1;

	if (!cbCommandProcessor)
	{
		return -1;
	}

	while (isConnected)
	{
		unsigned long long cur_time;

		bytesRead = readLine(cur_time);

		if (bytesRead > 0)
		{
			std::string command;

			size_t bytes_read = getDataBufferContent(command);

			cbCommandProcessor(command, cur_time, this);
		}
		else
		{
			if (bytesRead == -1)
			{
				break;
			}
		}
	}

	return isConnected ? bytesRead : 0;
}


// Reusing the non blocking Socket code from Old source
bool SocketStreamHandler::testConnectionWithTimeOut(char *hostName, int port, int timeOutSec)
{
	SOCKET client;
	ULONG NonBlk = 1;
	int Ret;
	DWORD Err;
	
	TRACE( _T("Connecting to %s:%d using non-blocking socket. Timeout is %d") ,
		hostName , port , timeOutSec ) ;
	
	// Get IP address of Gateway Server
    struct hostent * he ;
	if ( ( he = gethostbyname ( hostName ) ) == NULL )
    {
        int errorCode = WSAGetLastError ( ) ;
		TRACE( _T("Connection using non-blocking socket Err- Could not get IP for '%s': %d") ,
			hostName , errorCode ) ;
		return FALSE ;
	}
	
	// Open socket
	client = socket ( AF_INET , SOCK_STREAM , 0 ) ;
	
	// Set to Non-blocking mode
	ioctlsocket ( client , FIONBIO , &NonBlk ) ;
	
	// Some address and port.
	SOCKADDR_IN  sin ;
	DWORD        Addr ;
	Addr = inet_addr ( hostName ) ;
	
	sin.sin_family        = AF_INET ;
	sin.sin_port          = htons ( port ) ; //80);
	sin.sin_addr = *( reinterpret_cast<in_addr*> ( he->h_addr) ) ;
	
	// This call will return immediately, coz our socket is non-blocking
	Ret = connect ( client , ( const sockaddr * ) &sin , sizeof ( sin ) ) ;
	
	// If connected, it will return 0, or error
	if ( Ret == SOCKET_ERROR )
	{
		Err = WSAGetLastError ( ) ;
		
		// Check if the error was WSAEWOULDBLOCK, where we'll wait.
		if ( Err == WSAEWOULDBLOCK )
		{
			TRACE( _T("Connection using non-blocking socket Err - WSAEWOULDBLOCK. Need to Wait..") ) ;
			fd_set       Read , Write , Err ;
			TIMEVAL      Timeout ;
			
			FD_ZERO ( &Read ) ;
			FD_ZERO ( &Write ) ;
			FD_ZERO ( &Err ) ;
			
			if ( client )
			{
				FD_SET ( client , &Read ) ;
				FD_SET ( client , &Write ) ;
				FD_SET ( client , &Err ) ;
			}
			else
			{
				TRACE( _T("There is no socket connected") );
				//return FALSE;
			}
			
			Timeout.tv_sec  = timeOutSec ;
			Timeout.tv_usec = 0 ; // your timeout
			
			Ret = select ( 0 , &Read , &Write , &Err , &Timeout ) ;
			
			if ( Ret == 0 )
			{
				TRACE( _T("Connection using non-blocking socket Info - Timeout %d secs") ,
					timeOutSec ) ;
				closesocket ( client ) ;
			}
			else
			{
				if ( FD_ISSET ( client , &Write ) )
				{
					TRACE( _T("Connection using non-blocking socket succeeded") ) ;
					closesocket ( client ) ;
					return TRUE;
				}
				
				if ( FD_ISSET ( client , &Err ) )
				{
					TRACE( _T("Connection using non-blocking socket - Ret - %d -- %d %d %d %d %d %d %d"),
						Ret , WSAEINPROGRESS , WSAEINTR , WSAENOTSOCK , WSAENETDOWN , WSAEFAULT ,
						WSANOTINITIALISED , WSAEINVAL ) ;
					TRACE( _T("Connection using non-blocking socket Err - Select Error - %d") , Err ) ;
					// TODO - Changed this. Dunno if it's right ??
					closesocket ( client ) ;
					return FALSE ;
				}

				TRACE(_T("testConnectionWithTimeOut: Select returned but Write and Error descriptors not set"));
			}
		}
		else
		{
			TRACE( _T("Connection using non-blocking socket Err - %d") , WSAGetLastError ( ) ) ;
		}
	}
	else
	{
		TRACE( _T("Connection using non-blocking socket Info - Connected with NO Waiting!!") ) ;
		closesocket ( client ) ;

		return true ;
	}

	return false ;
}





