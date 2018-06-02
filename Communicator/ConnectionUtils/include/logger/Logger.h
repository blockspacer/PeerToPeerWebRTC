/***********************************************************************************

CLogger.h - CLogger class wraps Poco::Logger. Poco's logger doesn't allow to share
			 the same log file between multiple applications simultaneously. So wrapper
			 has been introduced. If there is a way to share same logfile, this wrapper 
			 can be removed.

@author: Ramesh Kumar K

*************************************************************************************/

# ifndef SOCKUTILS_LOGGER_H
# define SOCKUTILS_LOGGER_H

#include "Poco/Logger.h"
# include <string>
# include "ReferenceCounter.h"

namespace Connection{

	
	class CLogger : public IReferenceCounter
	{
	private:

		Poco::Logger& pocoLoggerRef;

		// If true - same log file is simultaneously shared by multiple applications
		bool isSharedLog;

		// Singleton Instance
		static CLogger* pLoggerIns;

		// Constructor
		CLogger(std::string logPath, std::string logSize, std::string archiveType, 
			std::string formatString, bool isLogLocalTime, bool isShared);

		// Static method that initializes Poco::Logger ref
		// TODO: Configurable logger setting must be read from file.
		static Poco::Logger& CLogger::initLoggerObject(std::string logPath, 
			std::string logSize, std::string archiveType, std::string formatString, bool isLogLocalTime);

	public:

		// Log levels
		static const int LOG_INFO = 1;
		static const int LOG_WARNING = 2;
		static const int LOG_ERROR = 3;

		// Logger methods that wrap poco::logger methods.
		// Wrapper introduced to share same log file between multiple applications simultaneously.
		// TODO: Convert arguments from va_args
		void log(int level, char* logMsg);
		void log(int level, char* logMsg, Poco::Any value1);
		void log(int level, char* logMsg, Poco::Any value1, Poco::Any value2);
		void log(int level, char* logMsg, Poco::Any value1, Poco::Any value2, Poco::Any value3);
		void log(int level, char* logMsg, Poco::Any value1, Poco::Any value2, Poco::Any value3, Poco::Any value4);
		void log(int level, char* logMsg, Poco::Any value1, Poco::Any value2, Poco::Any value3, Poco::Any value4, Poco::Any value5);


		// Static method to create/get instance of CLogger class
		static CLogger* getInstance(std::string logPath = "", bool isSharedLog = false, std::string logSize = "", 
			std::string archiveType = "", std::string formatString = "", bool isLogLocalTime = true);
	};
}

# endif

/* TODO:

1. Implement Access log.

*/

	