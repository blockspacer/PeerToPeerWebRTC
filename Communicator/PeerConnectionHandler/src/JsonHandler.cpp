
#include "stdafx.h"
# include "JsonHandler.h"


JsonHandler::JsonHandler(void)
{
	Me = NULL ;

	m_isRoot = false ;
}


JsonHandler::~JsonHandler(void)
{
	if ( m_isRoot )
	{
		if ( Me != NULL || is_error( Me ) || !json_object_is_type ( Me , json_type_null ) )
		{
			json_object_put ( Me ) ;
			Me = NULL ;
		}
	}
}

bool JsonHandler :: Release ( )
{
	if ( m_isRoot )
	{
		if ( Me != NULL || is_error( Me ) || !json_object_is_type ( Me , json_type_null ) )
		{
			json_object_put ( Me ) ;
			Me = NULL ;
			return true ;
		}
	}

	return false ;
}

std::string JsonHandler::toString()
{
	return json_object_to_json_string(Me);
}

bool JsonHandler::GetJKeyValue ( json_object * current_object , std::string JKeyName , std::string &out_value  )
{

	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object )  ) //error in response to json_object parser
	{
		return false ;
	}
	json_object * resultObj = json_object_object_get ( current_object , std::string(JKeyName).c_str() ) ;
	if (   ! resultObj || is_error( resultObj )   ) //error in response to json_object parser
	{
		return false ;
	}
	out_value = std::string( json_object_get_string ( resultObj ) ) ;
	return true ;
}

bool JsonHandler::GetJKeyValueA ( json_object * current_object , std::string JKeyName , std::string &out_value  )
{

	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object )  ) //error in response to json_object parser
	{
		return false ;
	}
	json_object * resultObj = json_object_object_get ( current_object , std::string(JKeyName).c_str()) ;
	if (   ! resultObj || is_error( resultObj )   ) //error in response to json_object parser
	{
		return false ;
	}

	out_value = std::string( json_object_get_string ( resultObj ) ) ;

	return true ;
}


json_object * JsonHandler::GetJKeyOfName ( json_object * current_object , std::string JKeyName  )
{
	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object ) ) //error in response to json_object parser
	{
		return NULL ;
	}
	json_object * out_obj = json_object_object_get ( current_object , std::string(JKeyName).c_str()) ;
	if (   ! out_obj || is_error( out_obj )   ) //error in response to json_object parser
	{
		return NULL ;
	}
	return out_obj ;
}


json_object * JsonHandler::GetJKeyAndName ( json_object * current_object , std::string JKeyName , std::string &out_value  )
{
	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object ) ) //error in response to json_object parser
	{
		return NULL ;
	}

	json_object * out_obj = json_object_object_get ( current_object , std::string(JKeyName).c_str()) ;
	if (   ! out_obj || is_error( out_obj ) || !json_object_is_type ( out_obj , json_type_object )  ) //error in response to json_object parser
	{
		return NULL ;
	}
	out_value = std::string( json_object_get_string ( out_obj ) ) ;
	return out_obj ;
}


json_object * JsonHandler::GetJKeyIntValue ( json_object * current_object , std::string JKeyName , int &out_value  )
{
	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object ) ) //error in response to json_object parser
	{
		return NULL ;
	}

	json_object * out_obj = json_object_object_get ( current_object , std::string(JKeyName).c_str()) ;
	if (   ! out_obj || is_error( out_obj ) || !json_object_is_type ( out_obj , json_type_int )  ) //error in response to json_object parser
	{
		return NULL ;
	}
	out_value =  json_object_get_int ( out_obj ) ;
	return out_obj ;
}

bool	JsonHandler::GetJKeyBoolValue ( json_object * current_object , std::string JKeyName , bool &out_value  ) 
{
	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object ) ) //error in response to json_object parser
	{
		out_value = false ;
		return false ;
	}

	json_object * out_obj = json_object_object_get ( current_object , std::string(JKeyName).c_str()) ;
	if (   ! out_obj || is_error( out_obj ) || !json_object_is_type ( out_obj , json_type_boolean )  ) //error in response to json_object parser
	{
		out_value = false ;
		return false ;
	}

	out_value = false ;
	if ( json_object_get_boolean(out_obj) )
	{
		out_value =  true ;
	}

	return true ;
}

bool	JsonHandler::GetjkeyDoubleValue ( json_object * current_object , std::string JKeyName , double &out_value ) 
{
	if (   ! current_object || is_error( current_object ) || !json_object_is_type ( current_object , json_type_object ) ) //error in response to json_object parser
	{
		out_value = 0 ;
		return false ;
	}

	json_object * out_obj = json_object_object_get ( current_object , std::string(JKeyName).c_str()) ;
	if (   ! out_obj || is_error( out_obj ) || !json_object_is_type ( out_obj , json_type_double )  ) //error in response to json_object parser
	{
		out_value = 0 ;
		return false ;
	}

	out_value = json_object_get_double(out_obj) ;

	return true ;
}

bool JsonHandler::ParseJson ( std::string json_string ) 
{
	bool res = false ;

	if ( Me || !IsNull () )
	{
	
		std::string str ; 
			
		bool result = get_string ( str ) ;

		json_object_put ( Me ) ;
		Me = NULL ;
	}

	Me  =  json_tokener_parse (  json_string.c_str() ) ;

	if ( !Me || json_object_is_type ( Me , json_type_null ) )
	{
		return false ;
	}

	m_isRoot = true ;

	return true ;
}

bool JsonHandler::get_string ( std::string &result , std::string key )
{
	result = ("") ;
	if ( IsNull ( ) )
	{
		return false ;
 	}

	std::string json_str ;

	if ( key == ("") || key.empty() )
	{
		json_str = std::string ( json_object_get_string ( Me ) ) ;
	}
	else 
	{
		 if ( !GetJKeyValue ( Me , key , json_str ) )
		 {
			 return false ;
		 }
	}

	result = json_str ;
	return true ;
}

bool JsonHandler::get_stringA ( std::string &result , std::string key )
{
	result = "" ;
	if ( IsNull ( ) )
	{
		return false ;
 	}

	std::string json_str ;

	if ( key == ("") || key.empty() )
	{
		json_str = json_object_get_string ( Me )  ;
	}
	else 
	{
		 if ( !GetJKeyValueA ( Me , key , json_str ) )
		 {
			 return false ;
		 }
	}

	result = json_str ;
	return true ;
}

bool JsonHandler::get_boolean ( bool &result , std::string key )
{
	result = false ;
	if ( IsNull ( ) )
	{
		return false ;
 	}

	bool json_boolean = false ;

	if ( key == ("") || key.empty() )
	{
		if ( json_object_get_boolean( Me ) != FALSE )
		{
			json_boolean = true ;
		}
	}
	else 
	{
		if ( !GetJKeyBoolValue ( Me , key , json_boolean ) )
		 {
			 return false ;
		 }
	}

	result = json_boolean ;
	return true ;
}

bool JsonHandler::get_double ( double &result , std::string key )
{
	result = 0 ;
	if ( IsNull ( ) )
	{
		return false ;
 	}

	double json_double = 0 ;

	if ( key == ("") || key.empty() )
	{
		json_double = json_object_get_double( Me ) ;
	}
	else 
	{
		if ( !GetjkeyDoubleValue ( Me , key , json_double ) )
		{
			return false ;
		}
	}

	result = json_double ;
	return true ;
}

bool JsonHandler::get_int ( int &result , std::string key )
{
	result = 0 ;
	if ( IsNull ( ) )
	{
		return false ;
 	}

	int json_int = -1 ;

	if ( key == ("") || key.empty() )
	{
		json_int = json_object_get_int( Me ) ;
	}
	else 
	{
		if ( !GetJKeyIntValue ( Me , key , json_int ) )
		{
			return false ;
		}
	}

	result = json_int ;
	return true ;
}

bool JsonHandler::get_json_item ( JsonHandler & json_item , std::string key )
{
	if ( IsNull ( ) )
	{
		return false ;
 	}

	int json_int = -1 ;

	if ( key == ("") || key.empty() )
	{
		return false ;
	}
	else 
	{
		json_item.Me = GetJKeyOfName ( Me , key ) ;

		if ( json_item.Me == NULL )
		{
			return false ;
		}
	}

	return true ;
}

bool JsonHandler::get_json_array_item ( JsonHandler & json_item , int idx )
{
	if ( IsNull ( ) )
	{
		return false ;
 	}

	int json_int = -1 ;

	if ( idx < 0 )
	{
		return false ;
	}
	else 
	{
		json_item.Me = json_object_array_get_idx ( Me , idx ) ;
		if ( json_item.Me == NULL )
		{
			return false ;
		}
	}

	return true ;
}

int JsonHandler::get_lenght ( bool &result )
{
	result = false ;
	int size = -1 ;

	if ( !IsJsonArray() )
	{
		result = true ;
		return -1 ;
	}

	size = json_object_array_length ( Me ) ;

	result = true ;
	return size ;
}

bool JsonHandler::IsNull ( )
{
	if ( json_object_is_type ( Me , json_type_null ) ) 
	{
		return true ;
	}
	return false ;
}

bool JsonHandler::IsBoolean ( ) 
{
	if ( json_object_is_type ( Me , json_type_boolean ) ) 
	{
		return true ;
	}
	return false ;
}

bool JsonHandler::IsDouble ( ) 
{
	if ( json_object_is_type ( Me , json_type_double ) ) 
	{
		return true ;
	}
	return false ;
}

bool JsonHandler::IsInt ( ) 
{
	if ( json_object_is_type ( Me , json_type_int ) ) 
	{
		return true ;
	}
	return false ;
}

bool JsonHandler::IsJsonObject ( ) 
{
	if ( json_object_is_type ( Me , json_type_object ) ) 
	{
		return true ;
	}
	return false ;
}

bool JsonHandler::IsJsonArray ( ) 
{
	if ( json_object_is_type ( Me , json_type_array ) ) 
	{
		return true ;
	}
	return false ;
}

bool JsonHandler::IsString ( ) 
{
	if ( json_object_is_type ( Me , json_type_string ) ) 
	{
		return true ;
	}
	return false ;
}


///////////////////// Insert methods //////////////////////

bool JsonHandler :: create_json_object ( bool is_root ) 
{
	if (   ! Me || is_error( Me ) ) //error in getting current json object
	{
		Me = json_object_new_object ( ) ;

		if (   ! Me || is_error( Me ) || !IsJsonObject() ) //error in getting current json object
		{
			return false ;
		}
	}
	else if ( !IsJsonObject() )
	{
		return false ;
	}

	m_isRoot = is_root ;

	return true ;
}

bool JsonHandler :: create_json_array_object ( ) 
{
	if (   ! Me || is_error( Me ) ) //error in getting current json object
	{
		Me = json_object_new_array ( ) ;

		if (   ! Me || is_error( Me ) || !IsJsonArray() ) //error in getting current json object
		{
			return false ;
		}
	}
	else if ( !IsJsonArray() )
	{
		return false ;
	}

	return true ;
}


bool JsonHandler :: add_string ( std::string key , std::string value ) 
{
	return add_stringA ( key , std::string(value) ) ;
}

bool JsonHandler :: add_stringA ( std::string key , std::string value ) 
{
	if (   ! Me || is_error( Me ) || !IsJsonObject() ) //error in getting current json object
	{
		return false ;
	}

	json_object * val = json_object_new_string ( value.c_str() ) ;

	if (   ! val || is_error( val ) ) //error in response to json_object creation
	{
		return false ;
	}

	json_object_object_add ( Me , std::string(key).c_str() , val ) ;

	return true ;
}

bool JsonHandler :: add_int ( std::string key , int value ) 
{
	if (   ! Me || is_error( Me ) || !IsJsonObject() ) //error in getting current json object
	{
		return false ;
	}

	json_object * val = json_object_new_int ( value ) ;

	if (   ! val || is_error( val ) ) //error in response to json_object creation
	{
		return false ;
	}

	json_object_object_add ( Me , std::string(key).c_str(), val ) ;

	return true ;
}

bool JsonHandler :: add_boolean ( std::string key , bool value ) 
{
	if (   ! Me || is_error( Me ) || !IsJsonObject() ) //error in getting current json object
	{
		return false ;
	}

	json_object * val = json_object_new_boolean ( value ) ;

	if (   ! val || is_error( val ) ) //error in response to json_object creation
	{
		return false ;
	}

	json_object_object_add ( Me , std::string(key).c_str(), val ) ;

	return true ;
}

bool JsonHandler :: add_double ( std::string key , double value ) 
{
	if (   ! Me || is_error( Me ) || !IsJsonObject() ) //error in getting current json object
	{
		return false ;
	}

	json_object * val = json_object_new_double ( value ) ;

	if (   ! val || is_error( val ) ) //error in response to json_object creation
	{
		return false ;
	}

	json_object_object_add ( Me , std::string(key).c_str(), val ) ;

	return true ;
}

bool JsonHandler :: add_json_item ( std::string key , JsonHandler value ) 
{
	if (   ! Me || is_error( Me ) || !IsJsonObject() ) //error in getting current json object
	{
		return false ;
	}

	json_object * val = value.get_json_object() ;

	if (   ! val || is_error( val ) ) //error in response to json_object creation
	{
		return false ;
	}

	json_object_object_add ( Me , std::string(key).c_str(), val ) ;

	return true ;
}





