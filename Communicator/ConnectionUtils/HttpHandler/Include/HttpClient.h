# pragma once

# include <string>
# include "stdint.h"
# include "ConnectionParams.h"
# include <functional>
# include <vector>

using namespace Connection;

using namespace std;

enum HTTP_METHODS{HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_HEAD};

enum HTTP_ERROR_CODES {HTTP_OK = 200, HTTP_NOT_REACHABLE = 404, HTTP_EXCEPTION = 600};

typedef std::function<void(int)> file_progress_callback ;

namespace HttpHandler
{
	namespace HttpClient
	{
		class HttpClient : public ClientConnection
		{

		public:

			virtual bool			InitSession																( ) = 0;

			virtual void			DestroySession															( ) = 0;

			virtual bool			SendRequest																( const std::string& url, const std::string& data, HTTP_METHODS http_method, const std::string &content_type = "application/x-www-form-urlencoded") = 0;

			virtual bool			SendRequest																( const std::string& url, vector<char> &data, HTTP_METHODS http_method, const std::string &content_type = "application/x-www-form-urlencoded") = 0;

			virtual bool			SubmitForm																( const std::string& page_name ) = 0;

			virtual bool			PrepareForm																( ) = 0;

			virtual bool			HtmlFormAddKeyValurePair												( std::string &key, std::string &value ) = 0;

			virtual bool			HtmlFormAddFile															( std::string file_name, std::string file_path ) = 0;

			virtual bool			ReadResponseAsString													( std::string &content ) =0;

			virtual bool			ReadResponseBytes														( uint8_t	**content )  =0;

			virtual bool			DownloadFile															( const std::string& file_url, const std::string file_location = "", int callback_interval_in_millis = 0 ) =0; // leaving file_location empty means we are expected to use current directory

			virtual bool			DownloadFileWithProgress												( const std::string& file_url, const std::string file_location = "", file_progress_callback callback = nullptr, int callback_interval_in_millis = 0 ) = 0; // leaving file_location empty means we are expected to use current directory


			//override

		protected:

			// Setting error informations - Only allowed to call from implementation methods
			virtual void			setLastError															( int errorCode, std::string &errorMsg ) =0;

			virtual void			setLastError															( int errorCode ) =0;

			virtual void			setLastWinErrorCode														( int errorCode ) =0;


		public:

			// Error Handling
			virtual int				getLastError															( ) =0;

			virtual std::string		getLastErrorMsg															( ) =0;
			
			// Getter Setters for Server Information

			virtual void			setServerHostName														(std::string hostName) =0;

			virtual void			setServerPort															(int port) =0;

			virtual void			setConnectionMode														(bool isSecure) =0;

			virtual void			setConnectionDetails													(std::string hostName, int hostPort = 443, bool isSecureMode = true,
																											std::string proxyHostName = "", int proxyPort = 0, 
																											std::string proxyUser = "", std::string proxyPassword = "", long long timeout = -1) =0;

			virtual void			setProxyDetails															(std::string proxyHostName, int proxyPort, 
																											std::string proxyUser, std::string proxyPassword) =0;

			virtual void			setProxySwitch															(bool _buseProxy) = 0;

			virtual void			setProxyHostName														(std::string hostName) =0;

			virtual void			setProxyPort															(int port) =0;

			virtual void			setProxyUserName														(std::string userName) =0;

			virtual void			setProxyPassword														(std::string password) =0;

			virtual std::string		getServerHostName														() =0;

			virtual int				getServerPort															() =0;

			virtual bool			isSecureMode															() =0;

			virtual bool			isProxyEnabled															() =0;

			virtual std::string		getProxyHostName														() =0;

			virtual std::string		getProxyUserName														() =0;

			virtual std::string		getProxyPassword														() =0;

			virtual int				getProxyPort															() =0;

			virtual void			setTimeout																(unsigned long long timeout) = 0;

		};
	};
};