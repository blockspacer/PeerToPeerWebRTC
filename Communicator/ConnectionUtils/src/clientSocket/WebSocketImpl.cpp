
# include "clientSocket\WebSocketImpl.h"
# include "logger\Logger.h"
# include "clientSocket\socketErrors.h"
# include "clientSocket\socketConfHandler.h"

// Poco Includes
# include "Poco/Exception.h"
# include "Poco/Net/HTTPMessage.h"
# include "Poco/Net/NetException.h"
# include "Poco/Net/HTTPSClientSession.h"
# include <string>

// Poco Namespaces
using Poco::TimeoutException;
using Poco::Net::HTTPMessage;
using Poco::Net::NetException;
using Poco::Net::HTTPSClientSession;
using Poco::Net::WebSocketException;

namespace Connection{

	namespace ConnectionUtils{

		CWebSocketImpl::CWebSocketImpl(std::string serverHostName, int serverPort, bool isSecureConnection, 
			std::string serverEndPointUrl):CSocket(),IReferenceCounter()
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO,  "CWebSocketImpl: Creating Socket Object %s:%d isSecure - %b Url - %s", serverHostName,
				serverPort, isSecureConnection, serverEndPointUrl);
			}

			setConnectionDetails(serverHostName, serverPort, isSecureConnection);
			CWebSocketImpl::serverEndpointUrl  = serverEndPointUrl;

			// Assign default values to proxy details
			setProxySwitch(false);

			//Initialize Other required arguments
			setPocoObjectReferences();

			initialize();
		}


		CWebSocketImpl::CWebSocketImpl(std::string serverHostName, int serverPort, bool isSecureConnection, 
			std::string serverEndPointUrl, std::string loggerPath, bool isSharedLog):CSocket(loggerPath, isSharedLog),IReferenceCounter()
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO,  "CWebSocketImpl: Creating Socket Object %s:%d isSecure - %b Url - %s", serverHostName,
				serverPort, isSecureConnection, serverEndPointUrl);
			}

			setConnectionDetails(serverHostName, serverPort, isSecureConnection);
			CWebSocketImpl::serverEndpointUrl  = serverEndPointUrl;

			// Assign default values to proxy details
			setProxySwitch(false);

			//Initialize Other required arguments
			setPocoObjectReferences();

			initialize();
		}



		CWebSocketImpl::CWebSocketImpl(std::string serverHostName,
				int serverPort,
				bool isSecureConnection,
				std::string proxyHost,
				int proxyPort,
				std::string proxyUser,
				std::string proxyPassword,
				std::string serverEndPointUrl):CSocket(),IReferenceCounter()
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO,  "CWebSocketImpl: Creating Socket Object with proxy %s:%d isSecure - %b Url - %s", serverHostName,
				serverPort, isSecureConnection, serverEndPointUrl);
			}

			setConnectionDetails(serverHostName, serverPort, isSecureConnection, proxyHost, proxyPort, proxyUser, proxyPassword, -1);
			CWebSocketImpl::serverEndpointUrl  = serverEndPointUrl;

			// Assign default values to proxy details
			setProxySwitch(true);

			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO,  "CWebSocketImpl: Proxy Information - Proxy Host: %s, Proxy Port: %d, Proxy Credentials: %s : %s",proxyHost,
				proxyPort, proxyUser, proxyPassword);
			}

			//Initialize Other required arguments
			setPocoObjectReferences();
			
			// Setup POCO objects based on parameters
			initialize();
		}

		CWebSocketImpl::~CWebSocketImpl()
		{
		}

		void CWebSocketImpl::setPocoObjectReferences()
		{
			CWebSocketImpl::lpClientSession    = NULL;
			CWebSocketImpl::lpHttpRequest      = NULL;
			CWebSocketImpl::lpWebSocket        = NULL;
			CWebSocketImpl::lpWssContext       = NULL;
		}

		void CWebSocketImpl::initialize()
		{
			std::string hostName = getServerHostName();
			int hostPort = getServerPort();
			
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initialize - Get Request Url - %s", serverEndpointUrl);
			}

			lpHttpRequest = new HTTPRequest(Poco::Net::HTTPRequest::HTTP_GET, "/" + serverEndpointUrl, HTTPMessage::HTTP_1_1);

			if(isSecureMode())
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initialize - Initializing Secure ClientSession %s:%d", hostName, hostPort);
				}
				
				//TODO: Make caPath and CertVerification Mode Configurable
				lpWssContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE,"", Poco::Net::Context::VERIFY_NONE,1, false, SOCKCONF_DEFAULT_CIPHERSUITE_LIST);
				lpClientSession = (HTTPClientSession *) new HTTPSClientSession(hostName, hostPort, lpWssContext);
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initialize - Initializing Non Secure ClientSession %s:%d", hostName,hostPort);
				}
				
				lpClientSession = new HTTPClientSession(hostName, hostPort);
			}
		}

		bool CWebSocketImpl::connect()
		{
			bool connectResult = false;

			try
			{

				if( (lpHttpRequest != NULL) && (lpClientSession != NULL) )
				{
					lpWebSocket = new WebSocket(*lpClientSession, *lpHttpRequest, httpResponse);

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::connect - WebSocket Connection Successful!");
					}
					

					if( lpWebSocket != NULL )
					{
						initSocketConfigurations();
						connectResult = true;
					}
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::connect - HTTPRequest and ClientSession parameters not initialized");
					}
					
					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(WebSocketException &wsExcep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::connect - WebSocketException - %s", std::string(wsExcep.what()));
				}
				
				setLastError(wsExcep.code(), std::string((char*)wsExcep.what()));
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::connect - Exception - %s", std::string(excep.what()));
				}
				
				connectResult = false;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return connectResult;
		}


		bool CWebSocketImpl::connectViaProxy()
		{
			bool connectResult = false;

			try
			{
				if( (!getProxyHostName().empty()) && (getProxyPort() != 0) )
				{
					lpClientSession->setProxyHost(getProxyHostName());
					lpClientSession->setProxyPort(getProxyPort());

					std::string proxyUser = getProxyUserName();
					std::string proxyPwd  = getProxyPassword();

					if( (!proxyUser.empty()) && (!proxyPwd.empty()) )
					{
						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_INFO, "ProxyDetails - %s:%d %s:%s", getProxyHostName(), getProxyPort(), proxyUser, proxyPwd);
						}
						
						lpClientSession->setProxyCredentials(proxyUser, proxyPwd);
					}
					else
					{
						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_INFO, "ProxyDetails without credentials - %s:%d", getProxyHostName(), getProxyPort());
						}
						
					}

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::connectViaProxy - Initiating connection with Proxy information");
					}

					connectResult = connect();
				}
				else
				{
					setLastError(SOCKUTIL_ERROR_PROXY_NOT_CONFIGURED);
				}
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::connectViaProxy - Exception - %s", std::string(excep.what()));
				}

				connectResult = false;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return connectResult;
			
		}

		bool CWebSocketImpl::initSocketConfigurations()
		{
			bool configResult = false;
			if(lpWebSocket != NULL)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initSocketConfigurations - Initializing Socket Configurations");
				}
				
				// TODO: Instead of constants use getters of socketConfHandler, when making these options configurable
				Poco::Timespan receiveTimeOut(SOCKCONF_DEFAULT_RECEIVE_TIMEOUT_SECS, 0);
				lpWebSocket->setReceiveTimeout(receiveTimeOut);
				lpWebSocket->setSendBufferSize(SOCKCONF_DEFAULT_SEND_BUFFER_SIZE);
				lpWebSocket->setReceiveBufferSize(SOCKCONF_DEFAULT_RECEIVE_BUFFER_SIZE);

				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initSocketConfigurations - Default Receive Timeout - %d", lpWebSocket->getReceiveTimeout().totalSeconds());
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initSocketConfigurations - Default Send Timeout - %d", lpWebSocket->getSendTimeout().totalSeconds());
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initSocketConfigurations - Send Buffer Size - %d", lpWebSocket->getSendBufferSize());
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::initSocketConfigurations - Receive Buffer Size - %d", lpWebSocket->getReceiveBufferSize());
				}

				configResult = true;
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::initSocketConfigurations - lpWebSocket is not initialized");
				}
				
				setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
			}	

			return configResult;
		}


		int CWebSocketImpl::receiveBytes(BYTE* receiveBuf, int bufferSize)
		{
			int receivedLength = -1;
			int readFlag = 0;

			try
			{
				if(lpWebSocket != NULL)
				{
					// TODO: Analyze other flag options for receiveframe
					receivedLength = lpWebSocket->receiveFrame(receiveBuf, bufferSize, readFlag);
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveBytes - Socket not initialized properly");
					}
					
					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(TimeoutException &toExcep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveBytes - TimeoutException - %s", std::string(toExcep.what()));
				}
				
				receivedLength = -1;
				setLastError(toExcep.code(), std::string((char*)toExcep.what()));
			}
			catch(NetException &netExcep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveBytes - NetException - %s", std::string(netExcep.what()));
				}
				
				receivedLength = -1;
				setLastError(netExcep.code(), std::string((char*)netExcep.what()));
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveBytes - Exception - %s", std::string(excep.what()));
				}
				
				receivedLength = -1;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return receivedLength;
		}


		int CWebSocketImpl::receiveString(char* receiveBuf, int bufferSize)
		{
			int receivedLength = -1;
			int readFlag = 0;

			try
			{
				if(lpWebSocket != NULL)
				{
					// TODO: Check available values of flag
					receivedLength = lpWebSocket->receiveFrame(receiveBuf, bufferSize, readFlag);
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveString - Socket not initialized properly");
					}
					
					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(TimeoutException &toExcep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveString - TimeoutException - %s", std::string(toExcep.what()));
				}
				
				receivedLength = -1;
				setLastError(toExcep.code(), std::string((char*)toExcep.what()));
			}
			catch(NetException &netExcep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveString - NetException - %s", std::string(netExcep.what()));
				}
				
				receivedLength = -1;
				setLastError(netExcep.code(), std::string((char*)netExcep.what()));
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::receiveString - Exception - %s", std::string(excep.what()));
				}
				
				receivedLength = -1;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return receivedLength;
		}

		int CWebSocketImpl::sendString(char* sendBuf)
		{
			return sendString (sendBuf, strlen(sendBuf) );
		}

		int CWebSocketImpl::sendString(char* sendBuf, int bufferLength)
		{
			int bytesSent = 0;

			try
			{
				if(lpWebSocket != NULL)
				{
					bytesSent = lpWebSocket->sendFrame(sendBuf, bufferLength, WebSocket::FRAME_TEXT);
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::sendString - Socket not initialized properly");
					}
					
					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::sendString - Exception - %s", std::string(excep.what()));
				}

				bytesSent = -1;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return bytesSent;
		}

		int CWebSocketImpl::sendBytes(BYTE* sendBuf, int bufferLength)
		{
			int bytesSent = 0;

			try
			{
				if(lpWebSocket != NULL)
				{
					bytesSent = lpWebSocket->sendFrame(sendBuf, bufferLength, WebSocket::FRAME_BINARY);
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::sendBytes - Socket not initialized properly");
					}
					
					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::sendBytes - Exception - %s", std::string(excep.what()));
				}
				
				bytesSent = -1;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}

			return bytesSent;
		}

		void CWebSocketImpl::close()
		{
			if(lpWebSocket != NULL)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::close - Closing WebSocket!");
				}
				
				lpWebSocket->close();
				lpWebSocket = NULL;
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::close - Socket not initialized properly");
				}
				
				setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
			}
		}


		void CWebSocketImpl::shutDownSocket()
		{
			if(lpWebSocket != NULL)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CWebSocketImpl::shutDownSocket - Shutting down WebSocket!");
				}

				lpWebSocket->shutdown();
				lpWebSocket = NULL;
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CWebSocketImpl::shutDownSocket - Socket not initialized properly");
				}

				setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
			}
		}

		bool CWebSocketImpl::isSocketLive()
		{
			return (lpWebSocket != NULL);
		}
	}
}
	



