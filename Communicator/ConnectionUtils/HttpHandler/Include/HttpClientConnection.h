# pragma once

# include <string>
# include "stdint.h"
# include "ConnectionParams.h"
# include "HttpClient.h"
# include <memory>

# include "logger\Logger.h"

#include <Poco/Net/HTTPClientSession.h>
#include "Poco/Net/Context.h"
#include "Poco/Net/HTMLForm.h"

#include "Poco/URI.h"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <memory>

using namespace Poco::Net;
using namespace Poco;
using Poco::URI;

using namespace Connection;

using namespace std;

namespace HttpHandler
{
	namespace HttpClient
	{
		class HttpClientConnection : public HttpClient
		{

		public:

			HttpClientConnection																	( );

			HttpClientConnection																	( std::string host, int port = 443, bool use_https = true, long long timeout = -1, bool use_proxy = false, std::string proxy_host = "", int proxy_port = -1, std::string proxy_user_name = "", std::string proxy_password = "", std::string logPath = "");

		public:

			~HttpClientConnection																	( );

		public:

			bool			InitSession																( );

			void			DestroySession															( );

			bool			SendRequest																( const std::string& url, const std::string& data, HTTP_METHODS http_method, const std::string &content_type = "application/x-www-form-urlencoded" );

			bool			SendRequest																( const std::string& url, vector<char>& data, HTTP_METHODS http_method, const std::string &content_type = "application/x-www-form-urlencoded" );

			bool			PrepareForm																( );

			bool			HtmlFormAddKeyValurePair												( std::string &key, std::string &value );

			bool			HtmlFormAddFile															(std::string file_name, std::string file_path);

			bool			SubmitForm																( const std::string& page_name );
				
			bool			ReadResponseAsString													( std::string &content );

			bool			ReadResponseBytes														( uint8_t	**content );

			bool			DownloadFile															( const std::string& file_url, const std::string file_location = "", int callback_interval_in_millis = 0 ); // leaving file_location empty means we are expected to use current directory

			bool			DownloadFileWithProgress												( const std::string& file_url, const std::string file_location = "", file_progress_callback callback = nullptr, int callback_interval_in_millis = 0 ); // leaving file_location empty means we are expected to use current directory

			//override

		protected:

			// Setting error informations - Only allowed to call from implementation methods
			void			setLastError															( int errorCode, std::string & errorMsg );

			void			setLastError															( int errorCode );

			void			setLastWinErrorCode														( int errorCode );

			bool			ConfigureProxy															( );


		private:

			std::istream*	openHttp																(const URI& uri, unsigned long long &content_size);

			std::istream*	openHttps																(const URI& uri, unsigned long long &content_size);


		public:

			// Error Handling
			int				getLastError															( );

			std::string		getLastErrorMsg															( );
			
			// Getter Setters for Server Information

			void			setServerHostName														(std::string hostName);

			void			setServerPort															(int port);

			void			setConnectionMode														(bool isSecure);

			void			setConnectionDetails													(std::string hostName, int hostPort = 443, bool isSecureMode = true,
																										std::string proxyHostName = "", int proxyPort = 0, 
																										std::string proxyUser = "", std::string proxyPassword = "", long long timeout = -1);

			void			setProxyDetails															(std::string proxyHostName, int proxyPort, 
																										std::string proxyUser, std::string proxyPassword);

			void			setProxySwitch															(bool _buseProxy);

			void			setProxyHostName														(std::string hostName);

			void			setProxyPort															(int port);

			void			setProxyUserName														(std::string userName);

			void			setProxyPassword														(std::string password);

			std::string		getServerHostName														();

			int				getServerPort															();

			bool			isSecureMode															();

			bool			isProxyEnabled															();

			std::string		getProxyHostName														();

			std::string		getProxyUserName														();

			std::string		getProxyPassword														();

			int				getProxyPort															();

			void			setTimeout																(unsigned long long timeout)
			{
				l_connection_timeout								= timeout;
			}

		private:

			unsigned long long		l_connection_timeout;


		private:

			// Logger Object
			shared_ptr<CLogger>																	httpLogger;

			std::string																				responseStr;

			// prepare session
			shared_ptr<HTTPClientSession>	_pHttpClientSession;

			Poco::AutoPtr<Context>			ptrContext;

			bool							b_session_initialized_with_target;

			static bool						b_factory_initialized;

			HTMLForm form;
		};
	};
};