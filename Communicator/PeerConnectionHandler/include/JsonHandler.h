//// 
//  Writing this class for updating json as smoothly
//
//

# ifndef __JSON_PARSING_HANDLER_H__
# define __JSON_PARSING_HANDLER_H__

# include "stdafx.h"
# include "json.h"
#include "iostream"
#include "string"

class JsonHandler
{
public:
	JsonHandler(void);
	~JsonHandler(void);

private :



protected :
	json_object * Me ;

	//If it is root it will deleted while scope is finished
	bool m_isRoot ;

public :

	//Method for parsing json
	bool ParseJson ( std::string json_string ) ;

	//Delete object
	bool Release ( ) ;

	/* Check Data type of object */
	bool IsNull ( ) ;

	bool IsBoolean ( ) ;

	bool IsDouble ( ) ;

	bool IsInt ( ) ;

	bool IsJsonObject ( ) ;

	bool IsJsonArray ( ) ;

	bool IsString ( ) ;

	////////////////////////////////////////////////////////////////////////////
	/*Get type specyfic values*/

	int get_lenght ( bool &result ) ;

	////////////////////////////////////////////////////////////////////////////
	/*Getting json values*/

	bool get_string (std::string &result , std::string key=("") ) ;

	bool get_stringA (std::string &result , std::string key=("") ) ;

	//return will status of the method failed/succeeded
	bool get_boolean ( bool &result , std::string key=("") ) ;

	bool get_double ( double &result , std::string key=("") ) ;

	bool get_int ( int &result , std::string key=("") ) ;

	bool get_json_item ( JsonHandler & json_item , std::string key=("") ) ;

	bool get_json_array_item ( JsonHandler & json_item , int idx ) ;

	json_object * get_json_object ( ) { return Me; } ;

	//// inser Methods

	bool create_json_object ( bool is_root = false /*for main object it should be zero*/ ) ;

	bool create_json_array_object ( ) ;

	bool add_string (std::string key , std::string value ) ;

	bool add_stringA (std::string key , std::string value ) ;

	bool add_int (std::string key , int value ) ;

	bool add_boolean (std::string key , bool value ) ;

	bool add_double (std::string key , double value ) ;

	bool add_json_item (std::string key , JsonHandler value ) ;

	//bool add_json_array_item ( CString key , JsonHandler value ) ;

	std::string toString();

private :
	bool GetJKeyValue ( json_object * current_object , std::string JKeyName , std::string &out_value  ) ;

	bool GetJKeyValueA ( json_object * current_object , std::string JKeyName , std::string &out_value  ) ;

	json_object * GetJKeyOfName ( json_object * current_object , std::string JKeyName  ) ;

	json_object * GetJKeyAndName ( json_object * current_object , std::string JKeyName , std::string &out_value  ) ;

	json_object * GetJKeyIntValue ( json_object * current_object , std::string JKeyName , int &out_value  ) ;

	bool	GetJKeyBoolValue	( json_object * current_object , std::string JKeyName , bool &out_value  ) ;

	bool	GetjkeyDoubleValue ( json_object * current_object , std::string JKeyName , double &out_value ) ;

} ;


# endif //__JSON_PARSING_HANDLER_H__