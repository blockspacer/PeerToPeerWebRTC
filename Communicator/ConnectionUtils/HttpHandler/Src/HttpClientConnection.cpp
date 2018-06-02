/************************************************************************************************************************************************************************************************************
* FileName   : HttpClient.cpp
*
* Description: Http client utility with proxy support, using openssl 1.2 and poco, this is being written in mind to support windows XP operating system that cannot support TLS1.2
*
* Date		 : 6/6/2017
*
* Author     : Ramesh Kumar K
*
************************************************************************************************************************************************************************************************************/

//error messages
#include "clientSocket\socketErrors.h"

//poco headers
#include "Poco/Exception.h"
#include "Poco/Net/SSLException.h"
#include "Poco/StreamCopier.h"

#include "Poco/Path.h"
#include "Poco/URIStreamOpener.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/FTPStreamFactory.h"

//ssl related headers
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/SSLManager.h"

#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>


#include "Poco/Net/HTTPIOStream.h"
#include "Poco/Net/HTTPCredentials.h"
#include "Poco/Net/NetException.h"
#include "Poco/URIStreamOpener.h"
#include "Poco/UnbufferedStreamBuf.h"
#include "Poco/NullStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/Net/FilePartSource.h"


using Poco::URIStreamFactory;
using Poco::URIStreamOpener;
using Poco::UnbufferedStreamBuf;


using Poco::URIStreamOpener;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Exception;
using Poco::Net::HTTPStreamFactory;
using Poco::Net::FTPStreamFactory;

#include "HttpClientconnection.h"

#include "FileProgressHandler.h"

using Poco::Logger;


#define MAX_REDIRECTION_RETRY_COUNT				10

namespace HttpHandler
{

	namespace HttpClient
	{
		bool HttpClientConnection::b_factory_initialized = false;

		std::string GetHttpMethod(HTTP_METHODS http_method)
		{
			std::string method;

			switch (http_method)
			{
			case HTTP_METHODS::HTTP_GET:
			{
				method = HTTPRequest::HTTP_GET;
			}
			break;

			case HTTP_METHODS::HTTP_POST:
			{
				method = HTTPRequest::HTTP_POST;
			}
			break;
			case HTTP_METHODS::HTTP_PUT:
			{
				method = HTTPRequest::HTTP_PUT;
			}
			break;

			default:

			{
				method = HTTPRequest::HTTP_GET;
			}
			}

			return method;
		}

		HttpClientConnection::HttpClientConnection()
		{
			_buseProxy = false;
			lastErrorMsg = "";
			lastErrorCode = 0;
			log_level = 0;
			l_connection_timeout = -1;

			httpLogger = nullptr;

			serverHostName = "";
			proxyHostName = "";
			proxyUserName = "";

			proxyPassword = "";

			serverPort = 0;

			proxyPort = 0;

			isSecureConnection = true;
			b_session_initialized_with_target = false;

			_pHttpClientSession = nullptr;
		}

		HttpClientConnection::HttpClientConnection(std::string host, int port, bool use_https, long long timeout, bool use_proxy, std::string proxy_host, int proxy_port, std::string proxy_user_name, std::string proxy_password, std::string logPath)
		{
			_buseProxy = use_proxy;
			lastErrorMsg = "";
			lastErrorCode = 0;
			log_level = 0;

			if (logPath.size() > 0)
			{
				httpLogger = shared_ptr<CLogger>(CLogger::getInstance(logPath, true));
			}

			serverHostName = host;
			proxyHostName = proxy_host;
			proxyUserName = proxy_user_name;
			l_connection_timeout = timeout;

			proxyPassword = proxy_password;

			serverPort = port;

			proxyPort = proxy_port;

			isSecureConnection = use_https;
			b_session_initialized_with_target = false;

			_pHttpClientSession = nullptr;
		}

		HttpClientConnection::~HttpClientConnection()
		{
			DestroySession();
		}

		bool HttpClientConnection::InitSession()
		{
			try
			{
				if (!b_factory_initialized)
				{
					b_factory_initialized = true;

					Poco::Net::HTTPStreamFactory::registerFactory();
					Poco::Net::HTTPSStreamFactory::registerFactory();
				}

				if (isSecureConnection)
				{
					// http://stackoverflow.com/questions/18315472/https-request-in-c-using-poco
					Poco::Net::initializeSSL();
					Poco::Net::SSLManager::InvalidCertificateHandlerPtr ptrHandler(new Poco::Net::AcceptCertificateHandler(false));

					ptrContext = (new Poco::Net::Context(Poco::Net::Context::CLIENT_USE,
						"", "", "", Poco::Net::Context::VERIFY_STRICT,
						9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));

					Poco::Net::SSLManager::instance().initializeClient(0, ptrHandler, ptrContext);

					_pHttpClientSession = shared_ptr<HTTPSClientSession>(new HTTPSClientSession(serverHostName, serverPort, ptrContext));
				}
				else
				{
					_pHttpClientSession = shared_ptr<HTTPClientSession>(new HTTPClientSession(serverHostName, serverPort));
				}

				if (_buseProxy && !ConfigureProxy())
				{
					if (httpLogger)
					{
						httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Failed to configure proxy");
					}
				}

				b_session_initialized_with_target = true;

				return true;
			}
			catch (const Poco::Net::SSLException& e)
			{
				std::string error_message = e.displayText();

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: SSLException at InitSession to open http connection with server %s , port %d, error message %s", serverHostName, serverPort, error_message);
				}
			}
			catch (Exception &e)
			{
				std::string error_message = e.displayText();

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception at InitSession to open http connection with server %s , page %s, error message %s", serverHostName, serverPort, error_message);
				}
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in InitSession , error details UNKNWON");
				}
			}

			return false;
		}

		bool HttpClientConnection::ConfigureProxy()
		{
			try
			{
				if (!_pHttpClientSession)
				{
					if (httpLogger)
					{
						httpLogger->log(CLogger::LOG_INFO, "HttpClientConnection::ConfigureProxy - failed to init connection with Proxy information, session instance is null");
					}

					return false;
				}

				if ((!getProxyHostName().empty()) && (getProxyPort() != 0))
				{
					_pHttpClientSession->setProxyHost(getProxyHostName());
					_pHttpClientSession->setProxyPort(getProxyPort());

					std::string proxyUser = getProxyUserName();
					std::string proxyPwd = getProxyPassword();

					if ((!proxyUser.empty()) && (!proxyPwd.empty()))
					{
						if (httpLogger)
						{
							httpLogger->log(CLogger::LOG_INFO, "ProxyDetails - %s:%d %s:%s", getProxyHostName(), getProxyPort(), proxyUser, proxyPwd);
						}

						_pHttpClientSession->setProxyCredentials(proxyUser, proxyPwd);
					}
					else
					{
						if (httpLogger)
						{
							httpLogger->log(CLogger::LOG_INFO, "ProxyDetails without credentials - %s:%d", getProxyHostName(), getProxyPort());
						}
					}

					if (httpLogger)
					{
						httpLogger->log(CLogger::LOG_INFO, "HttpClientConnection::ConfigureProxy - Initiating connection with Proxy information");
					}
				}
				else
				{
					setLastError(SOCKUTIL_ERROR_PROXY_NOT_CONFIGURED);
				}

				return true;
			}
			catch (std::exception &excep)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_ERROR, "HttpClientConnection::ConfigureProxy - Exception - %s", std::string(excep.what()));
				}

				setLastError(SOCKUTIL_STD_EXCEPTION, std::string((char*)excep.what()));
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in ConfigureProxy , error details UNKNWON");
				}
			}

			return false;

		}

		void HttpClientConnection::DestroySession()
		{
			_pHttpClientSession = nullptr;
		}

		bool HttpClientConnection::SendRequest(const std::string& page_name, const std::string& data, HTTP_METHODS http_method, const std::string &content_type)
		{
			try {


				int redirectiocount = 0;
				bool retry = false;
				string complete_pagename = "";

				// prepare path

				if (isSecureConnection)
				{
					complete_pagename = "https://";
				}
				else
				{
					complete_pagename = "http://";
				}

				if ((DEFAULT_SSL_PORT != serverPort) && (DEFAULT_NON_SSL_PORT != serverPort))
				{
					char port_array[100];

					memset(port_array, 0, 100);
					itoa(serverPort, port_array, 10);

					complete_pagename += serverHostName + ":" + std::string(port_array) + page_name;// assuming the file url itself will contain leading forward slash
				}
				else
				{
					complete_pagename += serverHostName + page_name;// assuming the file url itself will contain leading forward slash
				}

				URI proxyUri;
				URI resolvedURI(complete_pagename);

				if (!b_session_initialized_with_target)
				{
					InitSession();
				}

				/*
				* Let the loop run for max redirection retry count, incase of any normal redirection or proxy retry error messages
				*/

				while (redirectiocount < MAX_REDIRECTION_RETRY_COUNT && !retry)
				{
					std::string path = resolvedURI.getPathAndQuery();

					if (path.empty())
					{
						path = "/";
					}

					if (!_pHttpClientSession)
					{
						if (isSecureConnection)
						{
							_pHttpClientSession = shared_ptr<HTTPSClientSession>(new HTTPSClientSession(resolvedURI.getHost(), resolvedURI.getPort(), ptrContext));
						}
						else
						{
							_pHttpClientSession = shared_ptr<HTTPClientSession>(new HTTPClientSession(resolvedURI.getHost(), resolvedURI.getPort()));
						}
					}

					if (!_pHttpClientSession)
					{
						setLastError(SOCKUTIL_ERROR_MEMORY_FAILURE_OCCURED);

						return false;
					}

					if (proxyUri.empty())
					{
						if (_buseProxy)
						{
							_pHttpClientSession->setProxy(proxyHostName, proxyPort);
						}
					}
					else
					{
						_pHttpClientSession->setProxy(proxyUri.getHost(), proxyUri.getPort());
					}

					if (-1 != l_connection_timeout)
					{
						_pHttpClientSession->setTimeout(Poco::Timespan(l_connection_timeout, 0));
					}

					_pHttpClientSession->setKeepAlive(true);

					// send request
					HTTPRequest req(((HTTP_METHODS::HTTP_POST == http_method || HTTP_METHODS::HTTP_PUT == http_method) && "" != data) ? GetHttpMethod(http_method) : GetHttpMethod(HTTP_METHODS::HTTP_GET), path, HTTPMessage::HTTP_1_1);

					if ((HTTP_METHODS::HTTP_POST == http_method || HTTP_METHODS::HTTP_PUT == http_method) && "" != data)
					{
						if ("" == content_type)
						{
							req.setContentType("application/x-www-form-urlencoded");
						}
						else
						{
							req.setContentType(content_type);
						}

						req.setKeepAlive(true);

						std::string reqBody(data);

						req.setContentLength(reqBody.length());

						std::ostream& myOStream = _pHttpClientSession->sendRequest(req); // sends request, returns open stream
						myOStream << reqBody;  // sends the body
					}
					else
					{
						_pHttpClientSession->sendRequest(req);
					}

					// get response
					HTTPResponse res;

					// print response
					istream &rs = _pHttpClientSession->receiveResponse(res);

					responseStr = "";


					if (HTTP_ERROR_CODES::HTTP_OK != res.getStatus())
					{
						bool moved = (res.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY ||
							res.getStatus() == HTTPResponse::HTTP_FOUND ||
							res.getStatus() == HTTPResponse::HTTP_SEE_OTHER ||
							res.getStatus() == HTTPResponse::HTTP_TEMPORARY_REDIRECT);
						if (moved)
						{

							resolvedURI.resolve(res.get("Location"));

							//throw URIRedirection(resolvedURI.toString());
							retry = true;

							b_session_initialized_with_target = false;

							_pHttpClientSession = nullptr;

							redirectiocount++;
						}
						else if (res.getStatus() == HTTPResponse::HTTP_USEPROXY && !retry)
						{
							// The requested resource MUST be accessed through the proxy 
							// given by the Location field. The Location field gives the 
							// URI of the proxy. The recipient is expected to repeat this 
							// single request via the proxy. 305 responses MUST only be generated by origin servers.
							// only use for one single request!

							proxyUri.resolve(res.get("Location"));

							_pHttpClientSession = nullptr;

							retry = true; // only allow useproxy once
							b_session_initialized_with_target = false;
						}
						else
						{
							setLastError(res.getStatus());

							if (httpLogger)
							{
								httpLogger->log(CLogger::LOG_INFO, "HttpClient:: HttpSendRequest failed to open http connection with %s, error code %d, reason %s", page_name, res.getStatus(), res.getReason());
							}

							//retry = false;

							return false;
						}
					}
					else
					{
						StreamCopier::copyToString(rs, responseStr);

						break;
					}
				}

				return true;
			}
			catch (const Poco::Net::SSLException& e)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				std::string error_message = e.displayText();

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: SSLException at HttpSendRequest to open http connection with server %s , page %s, port %d, error message %s", serverHostName, page_name, serverPort, error_message);
				}
			}
			catch (Exception &e)
			{

				std::string error_message = e.displayText();

				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception at HttpSendRequest to open http connection with server %s , page %s, port %d, error message %s", serverHostName, page_name, serverPort, error_message);
				}
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in HttpSendRequest , error details UNKNWON");
				}
			}

			return false;
		}

		bool HttpClientConnection::SendRequest(const std::string& page_name, vector<char> &data, HTTP_METHODS http_method, const std::string &content_type)
		{
			try {


				int redirectiocount = 0;
				bool retry = false;
				string complete_pagename = "";

				// prepare path

				if (isSecureConnection)
				{
					complete_pagename = "https://";
				}
				else
				{
					complete_pagename = "http://";
				}

				if ((DEFAULT_SSL_PORT != serverPort) && (DEFAULT_NON_SSL_PORT != serverPort))
				{
					char port_array[100];

					memset(port_array, 0, 100);
					itoa(serverPort, port_array, 10);

					complete_pagename += serverHostName + ":" + std::string(port_array) + page_name;// assuming the file url itself will contain leading forward slash
				}
				else
				{
					complete_pagename += serverHostName + page_name;// assuming the file url itself will contain leading forward slash
				}

				URI proxyUri;
				URI resolvedURI(complete_pagename);

				if (!b_session_initialized_with_target)
				{
					InitSession();
				}

				/*
				* Let the loop run for max redirection retry count, incase of any normal redirection or proxy retry error messages
				*/

				while (redirectiocount < MAX_REDIRECTION_RETRY_COUNT && !retry)
				{
					std::string path = resolvedURI.getPathAndQuery();

					if (path.empty())
					{
						path = "/";
					}

					if (!_pHttpClientSession)
					{
						if (isSecureConnection)
						{
							_pHttpClientSession = shared_ptr<HTTPSClientSession>(new HTTPSClientSession(resolvedURI.getHost(), resolvedURI.getPort(), ptrContext));
						}
						else
						{
							_pHttpClientSession = shared_ptr<HTTPClientSession>(new HTTPClientSession(resolvedURI.getHost(), resolvedURI.getPort()));
						}
					}

					if (!_pHttpClientSession)
					{
						setLastError(SOCKUTIL_ERROR_MEMORY_FAILURE_OCCURED);

						return false;
					}

					if (proxyUri.empty())
					{
						if (_buseProxy)
						{
							_pHttpClientSession->setProxy(proxyHostName, proxyPort);
						}
					}
					else
					{
						_pHttpClientSession->setProxy(proxyUri.getHost(), proxyUri.getPort());
					}

					if (-1 != l_connection_timeout)
					{
						_pHttpClientSession->setTimeout(Poco::Timespan(l_connection_timeout, 0));
					}

					_pHttpClientSession->setKeepAlive(true);

					// send request
					HTTPRequest req(((HTTP_METHODS::HTTP_POST == http_method || HTTP_METHODS::HTTP_PUT == http_method) && data.size() > 0) ? GetHttpMethod(http_method) : GetHttpMethod(HTTP_METHODS::HTTP_GET), path, HTTPMessage::HTTP_1_1);

					if ((HTTP_METHODS::HTTP_POST == http_method || HTTP_METHODS::HTTP_PUT == http_method) && data.size() > 0)
					{
						if ("" == content_type)
						{
							req.setContentType("application/x-www-form-urlencoded");
						}
						else
						{
							req.setContentType(content_type);
						}

						req.setKeepAlive(true);

						req.setContentLength(data.size());

						std::ostream& myOStream = _pHttpClientSession->sendRequest(req); // sends request, returns open stream
						myOStream.write(&data[0], data.size());  // sends the body
					}
					else
					{
						_pHttpClientSession->sendRequest(req);
					}

					// get response
					HTTPResponse res;

					// print response
					istream &rs = _pHttpClientSession->receiveResponse(res);

					responseStr = "";


					if (HTTP_ERROR_CODES::HTTP_OK != res.getStatus())
					{
						bool moved = (res.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY ||
							res.getStatus() == HTTPResponse::HTTP_FOUND ||
							res.getStatus() == HTTPResponse::HTTP_SEE_OTHER ||
							res.getStatus() == HTTPResponse::HTTP_TEMPORARY_REDIRECT);
						if (moved)
						{

							resolvedURI.resolve(res.get("Location"));

							//throw URIRedirection(resolvedURI.toString());
							retry = true;

							b_session_initialized_with_target = false;

							_pHttpClientSession = nullptr;

							redirectiocount++;
						}
						else if (res.getStatus() == HTTPResponse::HTTP_USEPROXY && !retry)
						{
							// The requested resource MUST be accessed through the proxy 
							// given by the Location field. The Location field gives the 
							// URI of the proxy. The recipient is expected to repeat this 
							// single request via the proxy. 305 responses MUST only be generated by origin servers.
							// only use for one single request!

							proxyUri.resolve(res.get("Location"));

							_pHttpClientSession = nullptr;

							retry = true; // only allow useproxy once
							b_session_initialized_with_target = false;
						}
						else
						{
							setLastError(res.getStatus());

							if (httpLogger)
							{
								httpLogger->log(CLogger::LOG_INFO, "HttpClient:: HttpSendRequest failed to open http connection with %s, error code %d, reason %s", page_name, res.getStatus(), res.getReason());
							}

							//retry = false;

							return false;
						}
					}
					else
					{
						StreamCopier::copyToString(rs, responseStr);

						break;
					}
				}

				return true;
			}
			catch (const Poco::Net::SSLException& e)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				std::string error_message = e.displayText();

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: SSLException at HttpSendRequest to open http connection with server %s , page %s, port %d, error message %s", serverHostName, page_name, serverPort, error_message);
				}
			}
			catch (Exception &e)
			{

				std::string error_message = e.displayText();

				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception at HttpSendRequest to open http connection with server %s , page %s, port %d, error message %s", serverHostName, page_name, serverPort, error_message);
				}
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in HttpSendRequest , error details UNKNWON");
				}
			}

			return false;
		}

		bool HttpClientConnection::ReadResponseAsString(std::string &content)
		{
			try {

				content = responseStr;

				responseStr = "";

				return true;
			}
			catch (Poco::Exception& e) {

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in ReadResponseAsString , error code: %d", e.code());
				}
			}
			catch (...)
			{
				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in ReadResponseAsString , error details UNKNWON");
				}
			}

			return false;
		}

		bool HttpClientConnection::ReadResponseBytes(uint8_t	**content)
		{
			return true;
		}

		bool HttpClientConnection::DownloadFile(const std::string& file_url, const std::string file_location, int callback_interval_in_millis) // leaving file_location empty means we are expected to use current directory
		{

			try
			{
				std::string complete_page_url = "";
				std::ofstream file_stream;
				std::shared_ptr<std::istream> pStr = nullptr;


				if (isSecureConnection)
				{
					complete_page_url = "https://";
				}
				else
				{
					complete_page_url = "http://";
				}


				if ((DEFAULT_SSL_PORT != serverPort) && (DEFAULT_NON_SSL_PORT != serverPort))
				{
					char port_array[100];

					memset(port_array, 0, 100);
					itoa(serverPort, port_array, 10);

					complete_page_url += serverHostName + ":" + std::string(port_array) + file_url;// assuming the file url itself will contain leading forward slash
				}
				else
				{
					complete_page_url += serverHostName + file_url;// assuming the file url itself will contain leading forward slash
				}


				// Create the URI from the URL to the file.
				URI uri(complete_page_url);

				//std::auto_ptr<std::istream> pStr(URIStreamOpener::defaultOpener().open(uri));
				//StreamCopier::copyStream(*pStr.get(), std::cout);

				if (isSecureConnection)
				{
					std::shared_ptr<HTTPSStreamFactory> https_stream_factory = nullptr;

					if (_buseProxy)
					{
						https_stream_factory = std::shared_ptr<HTTPSStreamFactory>(new HTTPSStreamFactory(proxyHostName, proxyPort, getProxyUserName(), getProxyPassword()));
					}
					else
					{
						https_stream_factory = std::shared_ptr<HTTPSStreamFactory>(new HTTPSStreamFactory());
					}

					if (https_stream_factory)
					{
						pStr = std::shared_ptr<std::istream>(https_stream_factory->open(uri));
					}
				}
				else
				{
					std::shared_ptr<HTTPStreamFactory> http_stream_factory = nullptr;

					if (_buseProxy)
					{
						http_stream_factory = std::shared_ptr<HTTPStreamFactory>(new HTTPStreamFactory(proxyHostName, proxyPort, getProxyUserName(), getProxyPassword()));
					}
					else
					{
						http_stream_factory = std::shared_ptr<HTTPStreamFactory>(new HTTPStreamFactory());
					}

					if (http_stream_factory)
					{
						pStr = std::shared_ptr<std::istream>(http_stream_factory->open(uri));
					}
				}

				if (pStr)
				{
					file_stream.open(file_location, ios::out | ios::trunc | ios::binary);

					shared_ptr<CountingOutputStream> cout_stream = shared_ptr<CountingOutputStream>(new CountingOutputStream(file_stream));


					{
						StreamCopier::copyStream(*pStr.get(), *cout_stream);
					}

					file_stream.close();
				}

				return true;
			}
			catch (std::exception &exc)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in DownloadFile , error code: %s", exc.what());
				}
			}
			catch (Exception &exc)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in DownloadFile , error code: %d", exc.what());
				}
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in DownloadFile , error details UNKNWON");
				}
			}

			return false;
		}


		std::istream* HttpClientConnection::openHttp(const URI& uri, unsigned long long &content_size)
		{
			HTTPClientSession* pSession = 0;
			HTTPResponse res;
			bool retry = false;
			bool authorize = false;

			try
			{
				poco_assert(uri.getScheme() == "http");
				URI resolvedURI(uri);

				std::string username;
				std::string password;

				URI proxyUri;

				do
				{
					if (!pSession)
					{
						pSession = new HTTPClientSession(resolvedURI.getHost(), resolvedURI.getPort());

						if (!pSession)
						{
							return nullptr;
						}

						if (proxyUri.empty())
						{
							if (!proxyHostName.empty())
							{
								pSession->setProxy(proxyHostName, proxyPort);
								pSession->setProxyCredentials(getProxyUserName(), getProxyPassword());
							}
						}
						else
						{
							pSession->setProxy(proxyUri.getHost(), proxyUri.getPort());

							if (!proxyUserName.empty())
							{
								pSession->setProxyCredentials(getProxyUserName(), getProxyPassword());
							}
						}
					}

					std::string path = resolvedURI.getPathAndQuery();

					if (path.empty()) path = "/";

					HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);

					if (authorize)
					{
						HTTPCredentials::extractCredentials(uri, username, password);
						HTTPCredentials cred(username, password);
						cred.authenticate(req, res);
					}

					pSession->sendRequest(req);

					std::istream& rs = pSession->receiveResponse(res);

					bool moved = (res.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY ||
						res.getStatus() == HTTPResponse::HTTP_FOUND ||
						res.getStatus() == HTTPResponse::HTTP_SEE_OTHER ||
						res.getStatus() == HTTPResponse::HTTP_TEMPORARY_REDIRECT);
					if (moved)
					{
						resolvedURI.resolve(res.get("Location"));
						if (!username.empty())
						{
							resolvedURI.setUserInfo(username + ":" + password);
						}

						throw URIRedirection(resolvedURI.toString());
					}
					else if (res.getStatus() == HTTPResponse::HTTP_OK)
					{
						content_size = res.getContentLength();

						return new HTTPResponseStream(rs, pSession);
					}
					else if (res.getStatus() == HTTPResponse::HTTP_USEPROXY && !retry)
					{
						// The requested resource MUST be accessed through the proxy 
						// given by the Location field. The Location field gives the 
						// URI of the proxy. The recipient is expected to repeat this 
						// single request via the proxy. 305 responses MUST only be generated by origin servers.
						// only use for one single request!

						proxyUri.resolve(res.get("Location"));
						if (pSession)
						{
							delete pSession;
						}
						pSession = 0;
						retry = true; // only allow useproxy once
					}
					else if (res.getStatus() == HTTPResponse::HTTP_UNAUTHORIZED && !authorize)
					{
						authorize = true;
						retry = true;
						Poco::NullOutputStream null;
						Poco::StreamCopier::copyStream(rs, null);
					}

					else throw HTTPException(res.getReason(), uri.toString());
				} while (retry);

				throw HTTPException("Too many redirects", uri.toString());
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (pSession)
				{
					delete pSession;
				}

				throw;
			}
		}

		std::istream* HttpClientConnection::openHttps(const URI& uri, unsigned long long &content_size)
		{
			HTTPSClientSession* pSession = 0;
			bool retry = false;
			bool authorize = false;

			try
			{
				HTTPResponse res;

				poco_assert(uri.getScheme() == "https");

				URI resolvedURI(uri);
				URI proxyUri;

				std::string username;
				std::string password;

				do
				{
					if (!pSession)
					{
						pSession = new HTTPSClientSession(resolvedURI.getHost(), resolvedURI.getPort());

						if (!pSession)
						{
							return nullptr;
						}

						if (proxyUri.empty())
						{
							if (!proxyHostName.empty())
							{
								pSession->setProxy(proxyHostName, proxyPort);
								pSession->setProxyCredentials(getProxyUserName(), getProxyPassword());
							}
						}
						else
						{
							pSession->setProxy(proxyUri.getHost(), proxyUri.getPort());

							if (!proxyUserName.empty())
							{
								pSession->setProxyCredentials(getProxyUserName(), getProxyPassword());
							}
						}
					}

					std::string path = resolvedURI.getPathAndQuery();

					if (path.empty()) path = "/";

					HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);

					if (authorize)
					{
						HTTPCredentials::extractCredentials(uri, username, password);
						HTTPCredentials cred(username, password);
						cred.authenticate(req, res);
					}

					pSession->sendRequest(req);

					std::istream& rs = pSession->receiveResponse(res);

					bool moved = (res.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY ||
						res.getStatus() == HTTPResponse::HTTP_FOUND ||
						res.getStatus() == HTTPResponse::HTTP_SEE_OTHER ||
						res.getStatus() == HTTPResponse::HTTP_TEMPORARY_REDIRECT);
					if (moved)
					{
						resolvedURI.resolve(res.get("Location"));
						if (!username.empty())
						{
							resolvedURI.setUserInfo(username + ":" + password);
						}

						throw URIRedirection(resolvedURI.toString());
					}
					else if (res.getStatus() == HTTPResponse::HTTP_OK)
					{
						content_size = res.getContentLength();

						return new HTTPResponseStream(rs, pSession);
					}
					else if (res.getStatus() == HTTPResponse::HTTP_USEPROXY && !retry)
					{
						// The requested resource MUST be accessed through the proxy 
						// given by the Location field. The Location field gives the 
						// URI of the proxy. The recipient is expected to repeat this 
						// single request via the proxy. 305 responses MUST only be generated by origin servers.
						// only use for one single request!

						proxyUri.resolve(res.get("Location"));
						if (pSession)
						{
							delete pSession;
						}
						pSession = 0;
						retry = true; // only allow useproxy once
					}
					else if (res.getStatus() == HTTPResponse::HTTP_UNAUTHORIZED && !authorize)
					{
						authorize = true;
						retry = true;
						Poco::NullOutputStream null;
						Poco::StreamCopier::copyStream(rs, null);
					}

					else throw HTTPException(res.getReason(), uri.toString());
				} while (retry);

				throw HTTPException("Too many redirects", uri.toString());
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (pSession)
				{
					delete pSession;
				}

				throw;
			}
		}

		bool HttpClientConnection::PrepareForm()
		{
			form.setEncoding(HTMLForm::ENCODING_MULTIPART);

			return true;
		}

		bool HttpClientConnection::HtmlFormAddKeyValurePair(std::string &key, std::string &value)
		{
			form.set(key, value);

			return true;
		}

		// Add the file binary data to the form
		bool HttpClientConnection::HtmlFormAddFile(std::string file_name, std::string file_path)
		{
			form.addPart(file_name, new FilePartSource(file_path + "\\" + file_name));

			return true;
		}

		bool HttpClientConnection::SubmitForm(const std::string& page_name)
		{
			try {


				int redirectiocount = 0;
				bool retry = false;
				string complete_pagename = "";

				// prepare path

				if (isSecureConnection)
				{
					complete_pagename = "https://";
				}
				else
				{
					complete_pagename = "http://";
				}

				if ((DEFAULT_SSL_PORT != serverPort) && (DEFAULT_NON_SSL_PORT != serverPort))
				{
					char port_array[10];

					memset(port_array, 0, 100);
					itoa(serverPort, port_array, 10);

					complete_pagename += serverHostName + ":" + std::string(port_array) + page_name;// assuming the file url itself will contain leading forward slash
				}
				else
				{
					complete_pagename += serverHostName + page_name;// assuming the file url itself will contain leading forward slash
				}

				URI proxyUri;
				URI resolvedURI(complete_pagename);

				if (!b_session_initialized_with_target)
				{
					InitSession();
				}

				/*
				* Let the loop run for max redirection retry count, incase of any normal redirection or proxy retry error messages
				*/

				while (redirectiocount < MAX_REDIRECTION_RETRY_COUNT && !retry)
				{
					std::string path = resolvedURI.getPathAndQuery();

					if (path.empty())
					{
						path = "/";
					}

					if (!_pHttpClientSession)
					{
						if (isSecureConnection)
						{
							_pHttpClientSession = shared_ptr<HTTPSClientSession>(new HTTPSClientSession(resolvedURI.getHost(), resolvedURI.getPort(), ptrContext));
						}
						else
						{
							_pHttpClientSession = shared_ptr<HTTPClientSession>(new HTTPClientSession(resolvedURI.getHost(), resolvedURI.getPort()));
						}
					}

					if (!_pHttpClientSession)
					{
						setLastError(SOCKUTIL_ERROR_MEMORY_FAILURE_OCCURED);

						return false;
					}

					if (proxyUri.empty())
					{
						if (_buseProxy)
						{
							_pHttpClientSession->setProxy(proxyHostName, proxyPort);
						}
					}
					else
					{
						_pHttpClientSession->setProxy(proxyUri.getHost(), proxyUri.getPort());
					}

					if (-1 != l_connection_timeout)
					{
						_pHttpClientSession->setTimeout(Poco::Timespan(l_connection_timeout, 0));
					}

					_pHttpClientSession->setKeepAlive(true);

					// send request
					HTTPRequest req(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);

					{

						req.setKeepAlive(true);

						form.prepareSubmit(req);

						std::ostream& myOStream = _pHttpClientSession->sendRequest(req); // sends request, returns open stream

						form.write(myOStream);
					}

					// get response
					HTTPResponse res;

					// print response
					istream &rs = _pHttpClientSession->receiveResponse(res);

					responseStr = "";


					if (HTTP_ERROR_CODES::HTTP_OK != res.getStatus())
					{
						bool moved = (res.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY ||
							res.getStatus() == HTTPResponse::HTTP_FOUND ||
							res.getStatus() == HTTPResponse::HTTP_SEE_OTHER ||
							res.getStatus() == HTTPResponse::HTTP_TEMPORARY_REDIRECT);
						if (moved)
						{

							resolvedURI.resolve(res.get("Location"));

							//throw URIRedirection(resolvedURI.toString());
							retry = true;

							b_session_initialized_with_target = false;

							_pHttpClientSession = nullptr;

							redirectiocount++;
						}
						else if (res.getStatus() == HTTPResponse::HTTP_USEPROXY && !retry)
						{
							// The requested resource MUST be accessed through the proxy 
							// given by the Location field. The Location field gives the 
							// URI of the proxy. The recipient is expected to repeat this 
							// single request via the proxy. 305 responses MUST only be generated by origin servers.
							// only use for one single request!

							proxyUri.resolve(res.get("Location"));

							_pHttpClientSession = nullptr;

							retry = true; // only allow useproxy once
							b_session_initialized_with_target = false;
						}
						else
						{
							setLastError(res.getStatus());

							if (httpLogger)
							{
								httpLogger->log(CLogger::LOG_INFO, "HttpClient:: SubmitForm failed to open http connection with %s, error code %d, reason %s", page_name, res.getStatus(), res.getReason());
							}

							//retry = false;

							return false;
						}
					}
					else
					{
						StreamCopier::copyToString(rs, responseStr);

						break;
					}
				}

				return true;
			}
			catch (const Poco::Net::SSLException& e)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				std::string error_message = e.displayText();

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: SSLException at SubmitForm to open http connection with server %s , page %s, port %d, error message %s", serverHostName, page_name, serverPort, error_message);
				}
			}
			catch (Exception &e)
			{

				std::string error_message = e.displayText();

				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception at SubmitForm to open http connection with server %s , page %s, port %d, error message %s", serverHostName, page_name, serverPort, error_message);
				}
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in SubmitForm , error details UNKNWON");
				}
			}

			return false;
		}

		bool HttpClientConnection::DownloadFileWithProgress(const std::string& file_url, const std::string file_location, file_progress_callback  callback, int callback_interval_in_millis) // leaving file_location empty means we are expected to use current directory
		{

			try
			{
				std::string complete_page_url = "";
				std::ofstream file_stream;
				std::shared_ptr<std::istream> pStr = nullptr;
				unsigned long long file_size = 0;


				if (isSecureConnection)
				{
					complete_page_url = "https://";
				}
				else
				{
					complete_page_url = "http://";
				}


				if ((DEFAULT_SSL_PORT != serverPort) && (DEFAULT_NON_SSL_PORT != serverPort))
				{
					char port_array[100];

					memset(port_array, 0, 100);
					itoa(serverPort, port_array, 10);

					complete_page_url += serverHostName + ":" + std::string(port_array) + file_url;// assuming the file url itself will contain leading forward slash
				}
				else
				{
					complete_page_url += serverHostName + file_url;// assuming the file url itself will contain leading forward slash
				}


				// Create the URI from the URL to the file.
				URI uri(complete_page_url);

				//std::auto_ptr<std::istream> pStr(URIStreamOpener::defaultOpener().open(uri));
				//StreamCopier::copyStream(*pStr.get(), std::cout);

				if (isSecureConnection)
				{
					pStr = std::shared_ptr<std::istream>(openHttps(uri, file_size));
				}
				else
				{

					pStr = std::shared_ptr<std::istream>(openHttp(uri, file_size));
				}

				if (pStr)
				{
					file_stream.open(file_location, ios::out | ios::trunc | ios::binary);

					shared_ptr<Timer> timer = nullptr;

					shared_ptr<CountingOutputStream> cout_stream = shared_ptr<CountingOutputStream>(new CountingOutputStream(file_stream));

					if (callback && callback_interval_in_millis > 0)
					{
						shared_ptr<FileProgressHandler> file_progress = shared_ptr<FileProgressHandler>(new FileProgressHandler(cout_stream, file_size, callback));

						if (file_progress)
						{
							TimerCallback<FileProgressHandler> callback(*(file_progress.get()), &FileProgressHandler::onTimer);

							timer = shared_ptr<Timer>(new Timer(0, callback_interval_in_millis));

							if (timer)
							{
								timer->start(callback);
							}
						}

						StreamCopier::copyStream(*pStr.get(), *cout_stream);

						if (timer)
						{
							timer->stop();
						}
					}
					else
					{
						StreamCopier::copyStream(*pStr.get(), *cout_stream);
					}

					file_stream.close();
				}

				return true;
			}
			catch (std::exception &exc)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in DownloadFile , error code: %s", exc.what());
				}
			}
			catch (Exception &exc)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in DownloadFile , error code: %d", exc.what());
				}
			}
			catch (...)
			{
				setLastError(SOCKUTIL_ERROR_EXCEPTION_OCCURED);

				if (httpLogger)
				{
					httpLogger->log(CLogger::LOG_INFO, "HttpClient:: Exception in DownloadFile , error details UNKNWON");
				}
			}

			return false;
		}


		// Used to Set Custom error Codes.
		void HttpClientConnection::setLastError(int errorCode)
		{
			setLastError(errorCode, std::string(getErrorMessage(errorCode)));
		}

		// Used to Set POCO and Windows error codes.
		void HttpClientConnection::setLastError(int errorCode, std::string & errorMsg)
		{
			lastErrorCode = errorCode;
			lastErrorMsg = errorMsg;
		}

		void HttpClientConnection::setLastWinErrorCode(int errorCode)
		{
			LPSTR errorMsg = 0;

			if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				errorCode,
				0,
				(LPSTR)&errorMsg,
				0,
				NULL) != 0)
			{
				setLastError(errorCode, std::string(errorMsg));
			}
			else
			{
				setLastError(errorCode);
			}

			if (errorMsg != NULL)
			{
				LocalFree(errorMsg);
			}
		}

		int HttpClientConnection::getLastError()
		{
			return lastErrorCode;
		}

		std::string HttpClientConnection::getLastErrorMsg()
		{
			return lastErrorMsg;
		}

		// Server Details getters and setters

		void HttpClientConnection::setConnectionDetails(std::string hostName, int hostPort, bool isSecureMode,
			std::string proxyHostName, int proxyPort,
			std::string proxyUser, std::string proxyPassword, long long timeout)
		{
			setTimeout(timeout);
			setServerHostName(hostName);
			setServerPort(hostPort);
			setConnectionMode(isSecureMode);
			setProxyDetails(proxyHostName, proxyPort, proxyUser, proxyPassword);
		}

		void HttpClientConnection::setServerHostName(std::string hostName)
		{
			serverHostName = hostName;
		}

		void HttpClientConnection::setServerPort(int port)
		{
			serverPort = port;
		}

		void HttpClientConnection::setConnectionMode(bool isSecure)
		{
			isSecureConnection = isSecure;
		}

		void HttpClientConnection::setProxyDetails(std::string proxyHostName, int proxyPort, std::string proxyUser, std::string proxyPassword)
		{
			setProxyHostName(proxyHostName);
			setProxyPort(proxyPort);
			setProxyUserName(proxyUser);
			setProxyPassword(proxyPassword);
		}

		void HttpClientConnection::setProxySwitch(bool isProxyEnabled)
		{
			_buseProxy = isProxyEnabled;
		}

		void HttpClientConnection::setProxyHostName(std::string hostName)
		{
			proxyHostName = hostName;
		}

		void HttpClientConnection::setProxyPort(int port)
		{
			proxyPort = port;
		}

		void HttpClientConnection::setProxyUserName(std::string userName)
		{
			proxyUserName = userName;
		}

		void HttpClientConnection::setProxyPassword(std::string password)
		{
			proxyPassword = password;
		}

		std::string HttpClientConnection::getServerHostName()
		{
			return serverHostName;
		}

		int HttpClientConnection::getServerPort()
		{
			return serverPort;
		}

		bool HttpClientConnection::isSecureMode()
		{
			return isSecureConnection;
		}

		bool HttpClientConnection::isProxyEnabled()
		{
			return _buseProxy;
		}

		std::string HttpClientConnection::getProxyHostName()
		{
			return proxyHostName;
		}

		std::string HttpClientConnection::getProxyUserName()
		{
			return proxyUserName;
		}

		std::string HttpClientConnection::getProxyPassword()
		{
			return proxyPassword;
		}

		int HttpClientConnection::getProxyPort()
		{
			return proxyPort;
		}


	};

};