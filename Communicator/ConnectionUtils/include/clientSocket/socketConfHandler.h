/*********************************************************************************************

socketConfHandler.h - Header file contains constants for Socket Configurations.

* Methods to load values from file and getters/setters for various conf params must be added.
* Currently Socket params are not configurable, hence going with constants.

@author: Ramesh Kumar K

**********************************************************************************************/


# ifndef CLIENTSOCKET_SOCKCONFHANDLER_H
# define CLIENTSOCKET_SOCKCONFHANDLER_H

# include <string>

namespace Connection{
	
	namespace ConnectionUtils{

		static const long            SOCKCONF_DEFAULT_RECEIVE_TIMEOUT_SECS = 300    ;
		static const int			 SOCKCONF_DEFAULT_SEND_BUFFER_SIZE     = 1024 * 500  ;
		static const int			 SOCKCONF_DEFAULT_RECEIVE_BUFFER_SIZE  = 1024 * 500  ;
		static const std::string     SOCKCONF_DEFAULT_CIPHERSUITE_LIST     = "ALL:!ADH:!RC4:!LOW:!EXP:!MD5:@STRENGTH" ;

		// Maintain constants that are common to all socket implementations
		// When making these options configurable, write this as singleton class that gets and sets value.
	}
}

# endif

