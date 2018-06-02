# include "stdafx.h"
# include "Windows.h"
# include "HttpClient.h"
# include "HttpClientConnection.h"
# include "ClientSocketFactory.h"
# include <INITGUID.H>
# include "HttpClientInterfaces.h"
# include <memory>

using namespace HttpHandler::HttpClient;

HRESULT	ClientConnectionFactory::QueryInterface(REFIID riid, void* ppv)
{
	HRESULT hr = E_NOINTERFACE;

	if (NULL != ppv)
	{
		if ( CLS_ID_HTTP_CLIENT_CONNECTION == riid )
		{
				
			*((std::shared_ptr<HttpClient>*)ppv)  = static_cast<std::shared_ptr<HttpClient>>(new HttpClientConnection());

			hr = S_OK;
		}
		else
		{
			*((std::shared_ptr<HttpClient>*)ppv)  = nullptr;

			hr = E_NOINTERFACE;
		}
	}

	return true;

}