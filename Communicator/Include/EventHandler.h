//////////////////////////////////////////////////////////////////////////////////
//$Id$
//////////////////////////////////////////////////////////////////////////////////

# ifndef __EVENT_HANDLER_H__
# define __EVENT_HANDLER_H__

# include <vector>

using namespace std;

/*
 Declare Events below starting with __event
 Hook events in your subscriber constructor, starting with __hook
 Unhook in destructor, __unhook
 To call an event, just call event_handler->EVENT_NAME ( param1 , param2 , .. )


 For more info on events
 Example http://msdn.microsoft.com/en-in/library/ee2k0a7d(v=vs.80).aspx

 Note : We CANNOT use CString as params (or any class that has __try/object winding..)
*/

[ event_source ( native ) ]

class EventHandler
{

public :

// Variables

	// Common Events

	EventHandler();
	~EventHandler();

	__event	bool peer_connection_sdp_offer_handler(std::string &);

	__event	bool peer_connection_candidate_info_handler(std::string &);

	__event	bool peer_connection_error_handler(std::string &);

	__event	bool peer_connection_data_channel_state_handler(std::string &);

} ;

extern EventHandler event_handler ;

void InitEventHandler ( ) ;


# endif //__EVENT_HANDLER_H__