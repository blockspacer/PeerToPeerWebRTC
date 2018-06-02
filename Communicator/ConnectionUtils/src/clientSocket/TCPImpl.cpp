# include "clientSocket\TCPImpl.h"
# include "windows.h"
# include <sstream>
# include "logger\Logger.h"
# include "clientSocket\SocketErrors.h"
# include "Poco/Base64Encoder.h"
# include <string>
# include "clientSocket\SocketConfHandler.h"

using Poco::Base64Encoder;

namespace Connection{

	namespace ConnectionUtils{

		CTCPImpl::CTCPImpl(std::string serverHostName, int serverPort, bool isSecureConnection):CSocket(),IReferenceCounter()
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl: Creating Socket Object %s:%d isSecure - %b", serverHostName,
					serverPort, isSecureConnection);
			}

			setConnectionDetails(serverHostName, serverPort, isSecureConnection);
			setProxySwitch(false);

			initialize();
		}


		CTCPImpl::CTCPImpl(std::string serverHostName, int serverPort, bool isSecureConnection, std::string loggerPath, bool isSharedLog):CSocket(loggerPath, isSharedLog),IReferenceCounter()
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO,  "CTCPImpl: Creating Socket Object with Logger %s:%d isSecure - %b", serverHostName,
				serverPort, isSecureConnection);
			}

			setConnectionDetails(serverHostName, serverPort, isSecureConnection);
			setProxySwitch(false);

			initialize();
		}

		CTCPImpl::CTCPImpl(std::string serverHostName,
				int serverPort,
				bool isSecureConnection,
				std::string proxyHost,
				int proxyPort,
				std::string proxyUser,
				std::string proxyPassword):CSocket(),IReferenceCounter()
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO,  "CTCPImpl: Creating Socket Object with proxy %s:%d isSecure - %b", serverHostName,
				serverPort, isSecureConnection);

				sockUtilLogger->log(CLogger::LOG_INFO,  "CWebSocketImpl: Proxy Information - Proxy Host: %s, Proxy Port: %d, Proxy Credentials: %s : %s",proxyHost,
				proxyPort, proxyUser, proxyPassword);
			}

			setConnectionDetails(serverHostName, serverPort, isSecureConnection, proxyHost, proxyPort, proxyUser, proxyPassword, -1);

			// Setup POCO objects based on parameters
			initialize();
		}

		CTCPImpl::~CTCPImpl()
		{

			if ( sslContext && lpSSL )
			{
				SSL_CTX_free ( sslContext ) ;
				SSL_free ( lpSSL ) ;
			}

			sk_SSL_COMP_free ( SSL_COMP_get_compression_methods ( ) ) ;

			ERR_remove_state ( 0 ) ;
			ERR_free_strings ( ) ;
			EVP_cleanup ( ) ; 
			CRYPTO_cleanup_all_ex_data ( ) ;

		}
		
		void CTCPImpl::setTCPObjectReferences()
		{
			isTunnelHandShakeStarted = false;
			isShutdownInitiated = false;
			CTCPImpl::tcpSocket = NULL;
			CTCPImpl::sslContext = NULL;
		}

		bool CTCPImpl::isSocketLive()
		{
			if(isSecureMode())
			{
				return (lpSSL != NULL);
			}
			else
			{
				return (tcpSocket != NULL);
			}
		}

		void CTCPImpl::initialize()
		{
			WSADATA wsaData;
			is_connected_to_peer = false;

			setTCPObjectReferences();
			sendQueue.clearQueue();
			checkSenderThreadStatus();

			if(WSAStartup(MAKEWORD ( 2 , 0 ),&wsaData) == 0)
			{
				if (NULL != tcpSocket)
				{
					closesocket(tcpSocket);
					tcpSocket = NULL;
				}

				tcpSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

				if(tcpSocket > 0)
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::initialize: Normal Socket initialized successfully with custom buffers");
					}
					
					BOOL bNagVal = FALSE ;
					setsockopt ( tcpSocket , IPPROTO_TCP , TCP_NODELAY , ( char * ) & bNagVal , sizeof ( BOOL ) )  ; // Naggles
					setKeepAlive (1);
					setsockopt ( tcpSocket, SOL_SOCKET , SO_SNDBUF ,  ( const char * ) & SOCKCONF_DEFAULT_SEND_BUFFER_SIZE, sizeof ( int ) ) ;
					setsockopt ( tcpSocket, SOL_SOCKET , SO_RCVBUF ,  ( const char * ) & SOCKCONF_DEFAULT_RECEIVE_BUFFER_SIZE, sizeof ( int ) ) ;
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::initialize: Unable to get Socket pointer - %d", WSAGetLastError());
					}

					WSACleanup();
				}
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::initialize: WSAStartup failed !");
				}
				
				WSACleanup();
			}
		}


		void CTCPImpl::initializeSSLContext()
		{
			try
			{
				SSL_library_init();
				SSLeay_add_ssl_algorithms ( ) ;
				SSL_load_error_strings ( ) ;

				BIO_new_fp ( stderr , BIO_NOCLOSE ) ;

				const SSL_METHOD *m_ssl_client_method = TLSv1_2_client_method ( );

				if(m_ssl_client_method)
				{
					sslContext = SSL_CTX_new ( m_ssl_client_method ) ;

					SSL_CTX_set_mode(sslContext, SSL_MODE_AUTO_RETRY);
				}

				// It can be made configurable and moved to sockConfHandler
				SSL_CTX_set_cipher_list ( sslContext, "ECDHE-RSA-AES128-GCM-SHA256, ECDHE-RSA-AES256-GCM-SHA384, \
																	  ECDHE-RSA-AES128-SHA256, ECDHE-RSA-AES256-SHA384, AES128-GCM-SHA256, \
																	  AES256-GCM-SHA384, AES128-SHA256, AES256-SHA256, \
																	  ECDHE-RSA-AES128-SHA256, ECDHE-RSA-AES256-SHA384, AES256-SHA, \
																	  AES128-SHA256" ) ;
				sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::initializeSSLContext - SSL Context Initialized Successfully");				
			}

			catch(std::exception ex)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::initializeSSLContext: Exception while initializing SSL Context - %s", std::string(ex.what()));
				}
				
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)ex.what()));
			}
		}

		bool CTCPImpl::connect()
		{
			is_connected_to_peer = false;

			if (tcpSocket == NULL)
			{
				initialize();
			}

			if( tcpSocket != NULL )
			{
				serverHost = gethostbyname(getServerHostName().c_str());
				SOCKADDR_IN socketAddress ;
				socketAddress.sin_family = AF_INET ;
				socketAddress.sin_port = htons ( (unsigned short int) getServerPort() ) ;

				if (serverHost)
				{
					socketAddress.sin_addr = *( reinterpret_cast<in_addr*>(serverHost->h_addr) ) ;
				}

				ZeroMemory ( &socketAddress.sin_zero , sizeof(socketAddress.sin_zero) ) ;

				if( ::connect(tcpSocket, (struct sockaddr *)&socketAddress, sizeof(SOCKADDR_IN)) != -1 )
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::connect - TCP Socket connected successfully");
					}

					if(isSecureMode() == true)
					{
						initializeSSLContext();
						is_connected_to_peer = doSSLConnect();
					}
					else
					{
						is_connected_to_peer = true;
					}
				}
				else
				{
					int wsaErrorCode = WSAGetLastError();
					setTcpErrorCode(wsaErrorCode);

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::connect - Connection failed - %d:%s", wsaErrorCode, std::string(getLastErrorMsg()));
					}
					
				}
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::connect - Socket Not Intialized");
				}

				setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
			}

			if (is_connected_to_peer)
			{
				is_connected_to_peer = checkSenderThreadStatus();
			}

			return is_connected_to_peer;
		}

		bool CTCPImpl::doSSLConnect()
		{
			bool sslConnectResult = false;

			try
			{

				ERR_clear_error();

				if (!sslContext)
				{
					initializeSSLContext();
				}

				lpSSL = SSL_new(sslContext);

				if(lpSSL != NULL && tcpSocket)
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::doSSLConnect - Started SSL Connect");
					}
					
					if (SSL_set_fd(lpSSL, tcpSocket))
					{
						int connResult = SSL_connect(lpSSL);

						if(connResult <= 0)
						{
							int sslErrorCode = SSL_get_error(lpSSL, connResult);

							ERR_clear_error();

							char sslErrorString[MAX_ERROR_MSG_LEN] = {'\0'};

							ERR_error_string(connResult, sslErrorString);
							setLastError(sslErrorCode, std::string(sslErrorString));

							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::doSSLConnect - Connection Error - %d:%s", sslErrorCode, std::string(sslErrorString));
							}
						}
						else
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::doSSLConnect - SSL Connection successful");
							}
							
							sslConnectResult = true;
						}
					}
					else
					{
						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::doSSLConnect - SSL_set_fd failed. Setting Socket Initialization error");
						}
						
						setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
					}
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::doSSLConnect - Socket not initialized");
					}

					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(std::exception &excep)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CTCPImpl::doSSLConnect - Exception - %s", std::string(excep.what()));
				}

				sslConnectResult = false;
				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}
			catch(...)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_ERROR, "CTCPImpl::doSSLConnect - Unhandled exception while performing SSLConnect");
				}
			}


			if (!sslConnectResult)
			{
				if (sslContext)
				{
					SSL_CTX_free(sslContext);
					sslContext = NULL;
				}

				if (lpSSL)
				{
					SSL_free(lpSSL);
					lpSSL  = NULL;
				}
			}

			return sslConnectResult;
		}

		
	
		bool CTCPImpl::connectViaProxy()
		{
			is_connected_to_peer = false;

			if (tcpSocket == NULL)
			{
				initialize();
			}

			if( (tcpSocket != NULL) && (!getProxyHostName().empty()) && (getProxyPort() != 0) )
			{
				serverHost = gethostbyname(getProxyHostName().c_str());
				SOCKADDR_IN socketAddress ;
				socketAddress.sin_family = AF_INET ;
				socketAddress.sin_port = htons ( (unsigned short int) getProxyPort() ) ;

				if (serverHost)
				{
					socketAddress.sin_addr = *( reinterpret_cast<in_addr*>(serverHost->h_addr) ) ;
				}

				ZeroMemory ( &socketAddress.sin_zero , sizeof(socketAddress.sin_zero) ) ;

				if( ::connect(tcpSocket, (struct sockaddr *)&socketAddress, sizeof(SOCKADDR_IN)) != -1 )
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::connectViaProxy - TCP Socket Successfully connected to proxy");
					}

					
					is_connected_to_peer = doTunnelHandshake();

					if((is_connected_to_peer == true) && (isSecureMode() == true))
					{
						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::connectViaProxy - Switching to SSL Mode");
						}

						initializeSSLContext();
						is_connected_to_peer = doSSLConnect();
					}	
					else if(is_connected_to_peer == false)
					{
						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::connectViaProxy - Failed");
						}
					}

				}
				else
				{
					int wsaErrorCode = WSAGetLastError();
					setTcpErrorCode(wsaErrorCode);

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::connectViaProxy - Connection failed - %d:%s", wsaErrorCode, std::string(getLastErrorMsg()));
					}
				}

			}
			else
			{
				setLastError(SOCKUTIL_ERROR_PROXY_NOT_CONFIGURED);
			}

			if (is_connected_to_peer)
			{
				is_connected_to_peer = checkSenderThreadStatus();
			}

			return is_connected_to_peer;
		}


		// unused method
		int CTCPImpl::receiveBytes(BYTE* receiveBuf, int bufferSize)
		{
			int bytesReceived = -1;
			int bufferOffset = 0;
			int bytesRemaining = bufferSize;
			
			//BYTE *tempReadBuf = (BYTE*) calloc(1, (sizeof(BYTE) * bufferSize));

			while(bytesRemaining > 0)
			{
				//memset(tempReadBuf, 0, bufferSize);

				BYTE *tempReadBuf = receiveBuf + bufferOffset;

				int bytesRead = receiveString((char *)tempReadBuf, bytesRemaining);

				if((bytesRead != -1) && (bytesRead <= bytesRemaining))
				{
					//memcpy( receiveBuf + bufferOffset, tempReadBuf, bytesRead );
					bufferOffset = bufferOffset + bytesRead;
					bytesRemaining = bytesRemaining - bytesRead;
				}
				else if((bytesRead != -1) && (bytesRead > bytesRemaining))
				{
					//memcpy( receiveBuf + bufferOffset, tempReadBuf, bytesRemaining );
					bufferOffset = bufferOffset + bytesRemaining;
					bytesRemaining = 0;

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveBytes: Extra data read.. Few data will be lost");
					}
				}
				else if(bytesRead == -1)
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveBytes: Socket Error while reading byte data");
					}

					break;
				}
			}

			return bufferOffset;
		}


		int CTCPImpl::receiveString(char* receiveBuf, int bufferSize)
		{
			int bytesReceived = -1;

			try
			{
				if ((isSecureMode() == true) && (lpSSL != NULL))
				{
					bytesReceived = SSL_read(lpSSL, receiveBuf, bufferSize) ;

					if ((bytesReceived <= 0))
					{
						int sslErrorCode = lpSSL ? SSL_get_error(lpSSL, bytesReceived) : -1;

						ERR_clear_error();

						char sslErrorString[MAX_ERROR_MSG_LEN] = {'\0'};

						ERR_error_string(sslErrorCode, sslErrorString);

						int wsaError = WSAGetLastError();

						if(isShutdownInitiated == false)
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveString - SSL_read failed with error - %d, bytes received %d, error string %s, wsaError %d", sslErrorCode, bytesReceived, std::string(sslErrorString), wsaError);
							}
														// Always return -1 incase of failure

							setLastError(sslErrorCode, std::string(sslErrorString));

							bytesReceived = -1;
						} 
						else
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveString - Socket was Shutdown, SSL_read failed with error - %d, bytes received %d, error string %s , wsaError %d", sslErrorCode, bytesReceived, std::string(sslErrorString), wsaError);
							}
										
							setLastError(SOCKUTIL_ERROR_ALREADY_SHUTDOWN);
							bytesReceived = -1;
						}
					}
				}
				else if(tcpSocket != NULL)
				{
					bytesReceived = recv(tcpSocket,receiveBuf ,bufferSize ,0);

					if ((bytesReceived <= 0))
					{
						int wsaError = WSAGetLastError();

						if((isShutdownInitiated == false))
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveString - recv failed with Error code - %d", wsaError);
							}

							setTcpErrorCode(wsaError);
							// Return -1 in case of error avoid zero
							bytesReceived = -1;
						}
						else
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveString - Socket was Shutdown ... recv failed with error - %d", wsaError);
							}

							setTcpErrorCode(wsaError);
							setLastError(SOCKUTIL_ERROR_ALREADY_SHUTDOWN);
							bytesReceived = -1;
						}
					}
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveString - Socket Not Intialized");
					}

					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}
			}
			catch(...)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::receiveString - Exception while reading from socket");
				}
				
				setLastError(SOCKUTIL_ERROR_UNKNOWN);
				bytesReceived = -1;
			}


			return bytesReceived;		
		}

		int CTCPImpl::sendBytes(BYTE* sendBuf, int bufferLength)
		{
			return sendString((char *)sendBuf, bufferLength);
		}

		
		int CTCPImpl::sendString(char* sendBuf, int bufferLength)
		{
			try
			{
				int bytesSent = 0;

				if((isSecureMode() == true) && (lpSSL != NULL))
				{
					bytesSent = SSL_write(lpSSL, sendBuf, bufferLength);
				
					if ((bytesSent <= 0))
					{
						int sslErrorCode		= lpSSL ? SSL_get_error(lpSSL, bytesSent) : 0;

						ERR_clear_error();

						char sslErrorString[MAX_ERROR_MSG_LEN]	= {0};

						ERR_error_string(bytesSent, sslErrorString);

						int wsaError = WSAGetLastError();

						if((isShutdownInitiated == false))
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::sendString - Socket Error - %d:%s , wsa error_code %d", sslErrorCode, std::string(sslErrorString), wsaError);
							}

							setLastError(sslErrorCode, std::string(sslErrorString));
						
							// Always return -1 incase of failure
							bytesSent = -1;
						}
						else
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::sendString - Socket was Shutdown ... SSL_write failed with error - %d:%s , wsa error_code %d", sslErrorCode, std::string(sslErrorString), wsaError);
							}

							setLastError(SOCKUTIL_ERROR_ALREADY_SHUTDOWN);

							bytesSent = -1;
						}

					}

				}
				else if(tcpSocket != NULL)
				{
					bytesSent = send(tcpSocket, sendBuf, bufferLength, 0) ;


					if(bytesSent == SOCKET_ERROR)
					{
						int wsaError = WSAGetLastError();

						if((isShutdownInitiated == false))
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::sendString - recv failed with Error code - %d", wsaError);
							}
						
							setTcpErrorCode(wsaError);
							// Return -1 in case of error avoid zero
							bytesSent = -1;
						}
						else
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::sendString - Socket was Shutdown ... send failed with error - %d", wsaError);
							}
						
							setLastError(SOCKUTIL_ERROR_ALREADY_SHUTDOWN);
							bytesSent = -1;
						}
					}
				}
				else
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::sendString - Socket Not Intialized");
					}
				
					setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
				}

				return bytesSent;
			}
			catch(std::exception &e)
			{
				sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::SendString - Exception in SendString");
				setLastError(SOCKUTIL_ERROR_UNKNOWN);
			}

			return -1;
		}

		int CTCPImpl::sendString(char* sendBuf)
		{
			return sendString (sendBuf, strlen(sendBuf) );
		}

		void CTCPImpl::shutDownSocket()
		{
			try
			{
				if (false == isShutdownInitiated)
				{
					isShutdownInitiated = true;

					if( is_connected_to_peer && (isSecureMode() == true) && (lpSSL != NULL) )
					{
						int retries = 50;

						while ( SSL_pending(lpSSL) && retries-- > 0)
						{
							Sleep(100);
						}

						close();

						int shutDownResult = SSL_shutdown(lpSSL);

						if(shutDownResult == 0)
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::shutDownSocket - Shutdown returned, but yet to get Server status");
							}

							// Call SSL_shutdown again to get response from the server.
							shutDownResult = SSL_shutdown(lpSSL);

							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::shutDownSocket - Second shutdown completed. Return Code : %d", shutDownResult);
							}
						}
						else if(shutDownResult < 0)
						{
							int sslErrorCode		= lpSSL ? SSL_get_error(lpSSL, shutDownResult) : -1;

							ERR_clear_error();

							char sslErrorString[MAX_ERROR_MSG_LEN]	= {'\0'};

							ERR_error_string(shutDownResult, sslErrorString);

							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::shutDownSocket - SSL Error - %d:%s", sslErrorCode, std::string(sslErrorString));
							}

							setLastError(sslErrorCode, std::string(sslErrorString));
						}
						else if(shutDownResult == 1)
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::shutDownSocket - SSL Socket Shutdown successfully");
							}
						}
						else
						{
							if (sockUtilLogger)
							{
								sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::shutDownSocket - Unexpected return code from SSL_shutdown");
							}
						}

						if (sslContext)
						{
							SSL_CTX_free(sslContext);
							sslContext = NULL;
						}

						SSL_free(lpSSL);
						lpSSL  = NULL;
					}
					else if((isSecureMode() == true) && (sslContext == NULL))
					{
						close();

						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::shutDownSocket - Socket Not Intialized");
						}

						setLastError(SOCKUTIL_ERROR_SOCKET_NOT_INITIALIZED);
					}
					else
					{
						close();

						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::shutDownSocket - Shutdown for non secure socket");
						}
					}
				}
			}
			catch(...)
			{
				sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::shutDownSocket - Exception while shutting down socket");
				setLastError(SOCKUTIL_ERROR_UNKNOWN);
			}
		}

		void CTCPImpl::close()
		{
			if(tcpSocket != NULL)
			{
				try
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::close - Closing Socket");
					}

					if(closesocket(tcpSocket) == 0)
					{
						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::close - Socket closed successfully");
						}

						tcpSocket = NULL;
					}
					else
					{
						int wsaError = WSAGetLastError();

						if (sockUtilLogger)
						{
							sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::close - closesocket failed with Error code - %d", wsaError);
						}
						
						setTcpErrorCode(wsaError);
					}
				}
				catch(...)
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::close - Exception while closing socket");
					}

					setLastError(SOCKUTIL_ERROR_UNKNOWN);
				}
			}
		}

		void CTCPImpl::setTcpErrorCode(int wsaErrorCode)
		{
			LPSTR errorMsg = 0;

			if(FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				wsaErrorCode,
				0,
				(LPSTR)&errorMsg,
				0,
				NULL) != 0)
			{
				setLastError(wsaErrorCode, std::string(errorMsg));
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::setTcpErrorCode: Unable to get Error message. Error - %d", GetLastError());
				}

				setLastError(wsaErrorCode);
			}

			if(errorMsg != NULL)
			{
				LocalFree(errorMsg);
			}
		}

		void CTCPImpl::detectErrorSetSocketOption(int* errCode,string& errMsg)
		{
			*errCode = WSAGetLastError();

			if ( *errCode == WSANOTINITIALISED )
				errMsg.append("A successful WSAStartup must occur before using this function.");
			else if ( *errCode == WSAENETDOWN )
				errMsg.append("The network subsystem has failed.");
			else if ( *errCode == WSAEFAULT )
				errMsg.append("optval is not in a valid part of the process address space or optlen parameter is too small.");
			else if ( *errCode == WSAEINPROGRESS )
				errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.");
			else if ( *errCode == WSAEINVAL )
				errMsg.append("level is not valid, or the information in optval is not valid.");
			else if ( *errCode == WSAENETRESET )
				errMsg.append("Connection has timed out when SO_KEEPALIVE is set.");
			else if ( *errCode == WSAENOPROTOOPT )
				errMsg.append("The option is unknown or unsupported for the specified provider or socket (see SO_GROUP_PRIORITY limitations).");
			else if ( *errCode == WSAENOTCONN )
				errMsg.append("Connection has been reset when SO_KEEPALIVE is set.");
			else if ( *errCode == WSAENOTSOCK )
				errMsg.append("The descriptor is not a socket.");
			else errMsg.append("unknown problem!");
		}

		void CTCPImpl::setKeepAlive(int aliveToggle)
		{
			try 
			{
				if ( setsockopt(tcpSocket, SOL_SOCKET, SO_KEEPALIVE,(char *)&aliveToggle,sizeof(aliveToggle)) == -1 )
				{
					int errorCode;
					string errorMsg = "ALIVE option:";
					detectErrorSetSocketOption(&errorCode,errorMsg);

					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::SendString - Exception in setKeepAlive, error message %s", errorMsg);
				}
			}
			catch(std::exception &e)
			{
				sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::SendString - Exception in setKeepAlive");
			}
		} 

		bool CTCPImpl::doTunnelHandshake( )
		{
			bool tunnelHandshakeResult = false;
			
			// Get base64 encoded Username:password string
			std::string tunnelAuth = getAuthHeader( ) ;

			// Form ConnectMessage to send to proxy server
			std::stringstream connMsgStream;
			connMsgStream << "CONNECT " << getServerHostName() << ":" << getServerPort() << " HTTP/1.0\r\nProxy-Authorization: Basic "
				<< tunnelAuth << "\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322)\r\n\r\n" ;

			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO, "Connect Msg - %s", connMsgStream.str());
			}

			std::string connectMessage = connMsgStream.str();

			// Change connection mode to insecure for communication with proxy server - TODO: Secure proxy Communication
			bool isSecureMode = CSocket::isSecureMode();
			setConnectionMode(false);

			if(sendString((char *)connectMessage.c_str(), connectMessage.length()) > 0)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::doTunnelHandShake - Tunnel Handshake started successfully");
				}
				
				tunnelHandshakeResult = getProxyServerResponse();
			}
			else
			{
				int error = ::WSAGetLastError ( );

				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::doTunnelHandShake - Sending ConnectMsg via tunnel failed. Error - %d", error);
				}
				
				setTcpErrorCode(error);
			}

			// Reset connection mode
			setConnectionMode(isSecureMode);

			return tunnelHandshakeResult ;
		}

		std::string CTCPImpl::getAuthHeader()
		{
			char* userNamePass = (char *) calloc(1, (MAX_PATH * sizeof(char)));
			sprintf( userNamePass, "%s:%s", getProxyHostName().c_str(), getProxyPassword().c_str());

			std::ostringstream encoderStream;
			Base64Encoder b64Encoder(encoderStream);
			b64Encoder << userNamePass;
			b64Encoder.close();

			std::string authStr(encoderStream.str());


			if(userNamePass != NULL)
			{
				free(userNamePass);
			}

			return authStr;
		}

		bool CTCPImpl::getProxyServerResponse()
		{
			bool proxyServerResponse = false;

			char* proxyResponseBuffer = (char *) calloc( 1, (sizeof(char) * MAX_PATH));

			if(receiveString(proxyResponseBuffer, MAX_PATH) > 0)
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::getProxyServerResponse - Response from Proxy Server - %s", std::string(proxyResponseBuffer));
				}
				
				if ( (strnicmp(proxyResponseBuffer,"HTTP/1.0 200 ",13) == 0) ||
					(strnicmp(proxyResponseBuffer,"HTTP/1.1 200 ",13) == 0) )
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CTCPImpl::getProxyServerResponse - Proxy Response Successful");
					}

					proxyServerResponse = true ;
				}
				else if ( (strnicmp(proxyResponseBuffer,"HTTP/1.0 403 Forbidden",22) == 0 )
					||(strnicmp(proxyResponseBuffer,"HTTP/1.1 403 Forbidden",22) == 0 ))
				{
					setLastError( SOCKUTIL_ERROR_HTTP_FORBIDDEN );
				}
				else if ( (strnicmp(proxyResponseBuffer,"HTTP/1.0 407 Proxy Authentication Required",42) == 0 )
					||(strnicmp(proxyResponseBuffer,"HTTP/1.1 407 Proxy Authentication Required",42) == 0 ))
				{
					setLastError( SOCKUTIL_ERROR_HTTP_PROXY_AUTH_FAILED );
				}
				else if ( (strnicmp(proxyResponseBuffer,"HTTP/1.0 Bad Request",24) == 0 )
					||(strnicmp(proxyResponseBuffer,"HTTP/1.1 400 Bad Request",24) == 0 ))
				{
					setLastError( SOCKUTIL_ERROR_HTTP_BAD_REQUEST );
				}
				else
				{
					setLastError( SOCKUTIL_ERROR_UNKNOWN, std::string(("Unknown error response from Proxy server")) );
				}
			}
			else
			{
				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_WARNING, "CTCPImpl::getProxyServerResponse - Unable to receive response from proxy server");
				}
			}

			if(proxyResponseBuffer != NULL)
			{
				free(proxyResponseBuffer);
			}

			return proxyServerResponse;
		}
	}
}