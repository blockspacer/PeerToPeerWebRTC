# include "logger\Logger.h"
# include "Poco/FileChannel.h"
# include "Poco/FormattingChannel.h"
# include "Poco/PatternFormatter.h"
# include "Poco/AutoPtr.h"
# include <string>

using Poco::FormattingChannel;
using Poco::PatternFormatter;
using Poco::FileChannel;
using Poco::AutoPtr;
using Poco::Logger;


namespace Connection{


	Poco::Logger& CLogger::initLoggerObject(std::string logPath, std::string logSize, std::string archiveType, std::string formatString, bool isLogLocalTime)
	{
		AutoPtr<FileChannel> fileChannel(new FileChannel);

		if(logPath.empty())
		{
			// default log path
			// TODO: Should be made configurable
			fileChannel->setProperty("path", "..\\logs\\Connection.log");
		}
		else
		{
			fileChannel->setProperty("path", logPath);
		}


		// Rotate when size reach 2 MB
		if(logSize.empty())
		{
			fileChannel->setProperty("rotation", "2 M");
		}
		else
		{
			fileChannel->setProperty("rotation", logSize);
		}

		// Maintain the Archived files with number in extension (timestamp can also be used)
		if(archiveType.empty())
		{
			fileChannel->setProperty("archive", "number");
		}
		else
		{
			fileChannel->setProperty("archive", archiveType);
		}

		AutoPtr<PatternFormatter> logPattern(new PatternFormatter);

		if( !formatString.empty() )
		{
			logPattern->setProperty("pattern", formatString);
		}
		else
		{
			logPattern->setProperty("pattern", "%Y-%m-%d %H:%M:%S %s: %t");
		}
		
		if(isLogLocalTime == true)
		{
			logPattern->setProperty("times", "local");
		}

		AutoPtr<FormattingChannel> formattingChannel(new FormattingChannel(logPattern, fileChannel));

		Logger& sockLogger =  Logger::create("SockUtilLogger", formattingChannel, Poco::Message::PRIO_TRACE);

		return sockLogger;
	}

	CLogger* CLogger::pLoggerIns = NULL;

	CLogger* CLogger::getInstance(std::string logPath, bool isSharedLog, std::string logSize, 
			std::string archiveType, std::string formatString, bool isLogLocalTime)
	{
		if(pLoggerIns == NULL)
		{
			pLoggerIns = new CLogger(logPath, logSize, archiveType, formatString, isLogLocalTime, isSharedLog);
		}

		return pLoggerIns;
	}

	CLogger::CLogger(std::string logPath, std::string logSize, std::string archiveType, 
		std::string formatString, bool isLogLocalTime, bool isShared):pocoLoggerRef(initLoggerObject(logPath, logSize, archiveType,
		formatString, isLogLocalTime))
	{
		isSharedLog = isShared;
	}


	void CLogger::log(int level, char* logMsg)
	{
		try
		{
			if(level == CLogger::LOG_INFO)
			{
				pocoLoggerRef.information(logMsg);
			}
			else if(level == CLogger::LOG_ERROR)
			{
				pocoLoggerRef.error(logMsg);
			}
			else if(level == CLogger::LOG_WARNING)
			{
				pocoLoggerRef.warning(logMsg);
			}
			else
			{
				pocoLoggerRef.information(logMsg);
			}

			if(isSharedLog == true)
			{
				pocoLoggerRef.getChannel()->close();
			}
		}
		catch(...)
		{
		}
	}

	void CLogger::log(int level, char* logMsg, Poco::Any value1)
	{
		try
		{
			if(level == CLogger::LOG_INFO)
			{
				pocoLoggerRef.information(logMsg, value1);
			}
			else if(level == CLogger::LOG_ERROR)
			{
				pocoLoggerRef.error(logMsg, value1);
			}
			else if(level == CLogger::LOG_WARNING)
			{
				pocoLoggerRef.warning(logMsg, value1);
			}
			else
			{
				pocoLoggerRef.information(logMsg, value1);
			}

			if(isSharedLog == true)
			{
				pocoLoggerRef.getChannel()->close();
			}
		}
		catch(...)
		{
		}
	}


	void CLogger::log(int level, char* logMsg, Poco::Any value1, Poco::Any value2)
	{
		try
		{
			if(level == CLogger::LOG_INFO)
			{
				pocoLoggerRef.information(logMsg, value1, value2);
			}
			else if(level == CLogger::LOG_ERROR)
			{
				pocoLoggerRef.error(logMsg, value1, value2);
			}
			else if(level == CLogger::LOG_WARNING)
			{
				pocoLoggerRef.warning(logMsg, value1, value2);
			}
			else
			{
				pocoLoggerRef.information(logMsg, value1, value2);
			}

			if(isSharedLog == true)
			{
				pocoLoggerRef.getChannel()->close();
			}
		}
		catch(...)
		{
		}
	}


	void CLogger::log(int level, char* logMsg, Poco::Any value1, Poco::Any value2, Poco::Any value3)
	{
		try
		{
			if(level == CLogger::LOG_INFO)
			{
				pocoLoggerRef.information(logMsg, value1, value2, value3);
			}
			else if(level == CLogger::LOG_ERROR)
			{
				pocoLoggerRef.error(logMsg, value1, value2, value3);
			}
			else if(level == CLogger::LOG_WARNING)
			{
				pocoLoggerRef.warning(logMsg, value1, value2, value3);
			}
			else
			{
				pocoLoggerRef.information(logMsg, value1, value2, value3);
			}

			if(isSharedLog == true)
			{
				pocoLoggerRef.getChannel()->close();
			}
		}
		catch(...)
		{
		}
	}


	void CLogger::log(int level, char* logMsg, Poco::Any value1, Poco::Any value2, Poco::Any value3, Poco::Any value4)
	{
		try
		{
			if(level == CLogger::LOG_INFO)
			{
				pocoLoggerRef.information(logMsg, value1, value2, value3, value4);
			}
			else if(level == CLogger::LOG_ERROR)
			{
				pocoLoggerRef.error(logMsg, value1, value2, value3, value4);
			}
			else if(level == CLogger::LOG_WARNING)
			{
				pocoLoggerRef.warning(logMsg, value1, value2, value3, value4);
			}
			else
			{
				pocoLoggerRef.information(logMsg, value1, value2, value3, value4);
			}

			if(isSharedLog == true)
			{
				pocoLoggerRef.getChannel()->close();
			}
		}
		catch(...)
		{
		}
	}


	void CLogger::log(int level, char* logMsg, Poco::Any value1, Poco::Any value2, Poco::Any value3, Poco::Any value4, Poco::Any value5)
	{
		try
		{
			if(level == CLogger::LOG_INFO)
			{
				pocoLoggerRef.information(logMsg, value1, value2, value3, value4, value5);
			}
			else if(level == CLogger::LOG_ERROR)
			{
				pocoLoggerRef.error(logMsg, value1, value2, value3, value4, value5);
			}
			else if(level == CLogger::LOG_WARNING)
			{
				pocoLoggerRef.warning(logMsg, value1, value2, value3, value4, value5);
			}
			else
			{
				pocoLoggerRef.information(logMsg, value1, value2, value3, value4, value5);
			}

			if(isSharedLog == true)
			{
				pocoLoggerRef.getChannel()->close();
			}
		}
		catch(...)
		{
		}
	}



}