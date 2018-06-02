#pragma once

#include "Poco/CountingStream.h"
//Timer related functions
#include "Poco/Timer.h"
#include "Poco/Thread.h"
#include "Poco/Stopwatch.h"

using Poco::Timer;
using Poco::TimerCallback;
using Poco::Thread;
using Poco::Stopwatch;

using Poco::CountingInputStream;
using Poco::CountingOutputStream;

using namespace std;


class FileProgressHandler
{
public:
        FileProgressHandler(shared_ptr<CountingIOS> const& stream, unsigned long long bytes_count, file_progress_callback callback)
        {
			counting_io_stream			= stream;

			progress_callback			= callback;

			total_processed_data		= 0;

			file_size					= bytes_count;
			
			_sw.start();
        }

		~FileProgressHandler()
		{
			counting_io_stream = nullptr;
		}

        void onTimer(Timer& timer)
        {
			try
			{
				if (progress_callback)
				{
					unsigned long long data_processed = 0;

					counting_io_stream ? data_processed = counting_io_stream->chars() : __noop;

					if (data_processed > total_processed_data)
					{
						total_processed_data = data_processed;

						(file_size > 0 && total_processed_data > 0) ? progress_callback((100 * total_processed_data) / file_size) : __noop;
					}
				}
			}
			catch(const std::bad_function_call& e) 
			{

			}
			catch(const std::bad_weak_ptr& e)
			{

			}
			catch(const std::exception& e)
			{

			}
        }

private:

        Stopwatch												 _sw;
		file_progress_callback									progress_callback;

		unsigned long long										total_processed_data;

		shared_ptr<CountingIOS>									counting_io_stream;
		unsigned long long										file_size;						
};