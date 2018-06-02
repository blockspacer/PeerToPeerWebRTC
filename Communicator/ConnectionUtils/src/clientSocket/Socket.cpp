# include "clientSocket\Socket.h"
# include "clientSocket\socketErrors.h"
# include <string>
# include "utils\WindowsHelper.h"

using Poco::Logger;


namespace Connection{

	namespace ConnectionUtils{

		DWORD WINAPI senderThread(LPVOID lparam)
		{
			DWORD errorCode						= ERROR_SUCCESS;
			SmartPtr<CSocket> socket			= reinterpret_cast <CSocket *> (lparam);

			try
			{
				{
					lock_guard<recursive_mutex> lock(socket->mtxSenderThread);

					if (socket->bSenderThreadRunning)
					{
						return 0;
					}

					socket->bSenderThreadRunning = true;
				}

				if (!socket)
				{
					return E_FAIL;
				}

				std::pair<unsigned long long , std::pair < int, std::vector <char> >> queueData;

				std::vector <char>	&tempdataBuff			= queueData.second.second;
				int &n_data_size							= queueData.second.first;
				queueData.second.second.reserve(SEND_BUFFER_LENGTH);

#ifdef __DO_BUFFER__
				std::vector< char > dataToSend;
				dataToSend.reserve(SEND_BUFFER_LENGTH);
#endif

				if (socket->sockUtilLogger)
				{
					socket->sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::senderThread - Entering Sender Thread!!");
				}

				while(socket->bRunSenderThread)
				{
					if(socket->sendQueue.QueueReadData(queueData))
					{
#ifdef __DO_BUFFER__
						while( n_data_size < 1024 && dataToSend.size() < 1024 )
						{
							dataToSend.insert(dataToSend.end(), &(tempdataBuff[0]), &(tempdataBuff[0]) + n_data_size);
							tempdataBuff.clear();

							if (false == socket->sendQueue.QueueTryReadData(queueData))
							{
								break;
							}
						}

						if(!dataToSend.empty())
						{
							int bytesSent = socket->sendString(&dataToSend[0], dataToSend.size());	

							dataToSend.clear();

							if(bytesSent < 0)
							{
								if (socket->sockUtilLogger)
								{
									socket->sockUtilLogger->log(CLogger::LOG_ERROR, "CSocket::senderThread - Sending failed. Quitting Sender thread.");
								}
							}
						}
#endif // do buffering

						if(!tempdataBuff.empty())
						{
					
							{
								int bytesSent = socket->sendString(&tempdataBuff[0], n_data_size);	

								tempdataBuff.clear();

								if(bytesSent < 0)
								{
									socket->sockUtilLogger->log(CLogger::LOG_ERROR, "CSocket::senderThread - Sending failed. Quitting Sender thread.");

									break;
								}
							}
						}
					}
				}
			}
			catch(std::exception &e)
			{
				if (socket->sockUtilLogger)
				{
					socket->sockUtilLogger->log(CLogger::LOG_ERROR, "CSocket::senderThread - Exception, details %s", e.what());
				}
			}	
			catch(...)
			{
				if (socket->sockUtilLogger)
				{
					socket->sockUtilLogger->log(CLogger::LOG_ERROR, "CSocket::senderThread - Unknown Exception. Quitting Sender thread.");
				}
			}			

			// Once Sender thread is quit, it should not be started again for same instance.
			if (socket->sockUtilLogger)
			{
				socket->sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::senderThread - SenderThread Quit!!");
			}

			if (socket)
			{
				socket->bRunSenderThread = false;
				socket->bSenderThreadRunning = false;

				socket->shutDownSocket();
			}

			socket = NULL;

			return errorCode;
		}
		

		// Constructor - Initializes CLogger Object
		CSocket::CSocket(std::string logPath, bool isSharedLog):IConnection()
		{
			sockUtilLogger = CLogger::getInstance(logPath, isSharedLog);
			hSenderThread		 = NULL;
			bRunSenderThread	 = false;
			bSenderThreadRunning = false;
		}
		
		// Used to Set Custom error Codes.
		void CSocket::setLastError(int errorCode)
		{
			setLastError(errorCode, std::string(getErrorMessage(errorCode)));
		}
		
		// Used to Set POCO and Windows error codes.
		void CSocket::setLastError(int errorCode, std::string & errorMsg)
		{
			if (sockUtilLogger)
			{
				sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::setLastError - Setting Error - %d : %s", errorCode, std::string(errorMsg));
			}
			
			IConnection::setLastError(errorCode, errorMsg);
		}

		// Asynchronous Sending Handling

		bool CSocket::checkSenderThreadStatus()
		{
			bool result = true;

			if((hSenderThread == NULL || false == bSenderThreadRunning) && (isSocketLive()))
			{
				if (hSenderThread)
				{
					CloseHandle(hSenderThread);
					hSenderThread = NULL;
				}

				if (sockUtilLogger)
				{
					sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::checkSenderThreadStatus - Creating SenderThread!!");
				}

				bRunSenderThread = true;

				hSenderThread = CreateThread(NULL, 0, senderThread, this, NULL, NULL);

				if(hSenderThread == NULL)
				{
					setLastWinErrorCode(GetLastError());

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::sendStringAsync - Unable to start sender thread - %d", getLastError());
					}

					result = false;
				}
				else
				{
					//SetThreadPriority(hSenderThread, THREAD_PRIORITY_NORMAL);

					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::checkSenderThreadStatus - SenderThread created successfully!!");
					}
				}
			}

			return result;
		}

		bool CSocket::sendStringAsync(char* sendBuffer, int bufferLength)
		{
			//bool result = checkSenderThreadStatus(); // not necessary, disabling this check to avoid unnecessary checks to improve speed

			if((sendBuffer != NULL) && (bufferLength > 0))
			{
				std::pair < unsigned long long,  std::pair<int, std::vector <char> > > dataPair;
				dataPair.second.second.assign(sendBuffer, sendBuffer + bufferLength);
				dataPair.second.first = bufferLength;

				sendQueue.QueueWriteData(dataPair);

				return TRUE;
			}

			return FALSE;
		}

		bool CSocket::sendStringAsync(char* sendBuffer)
		{
			return sendStringAsync(sendBuffer, strlen(sendBuffer));
		}

		bool CSocket::sendBytesAsync(unsigned char* sendBuffer, int bufferLength)
		{
			return sendStringAsync((char *) sendBuffer, bufferLength);
		}

		bool CSocket::sendVectorAsync(std::vector <char>& data)
		{
			//bool result = checkSenderThreadStatus(); // not necessary, disabling this check to avoid unnecessary checks to improve speed

			if((!data.empty()))
			{
				std::pair < unsigned long long,  std::pair<int, std::vector <char> > > dataPair;
				dataPair.second.second = data;
				dataPair.second.first = data.size();

				sendQueue.QueueWriteData(dataPair);

				return TRUE;
			}

			return FALSE;
		}

		void CSocket::DestroySocket()
		{
			bRunSenderThread = false;
			sendQueue.NotifyWaitingThreads();

			int retries = 50;

			while (bSenderThreadRunning && retries-- > 0)
			{
				DWORD dwEvent = WaitForSingleObject (  hSenderThread , 100 );

				if (WAIT_OBJECT_0 == dwEvent) 
				{
					if (sockUtilLogger)
					{
						sockUtilLogger->log(CLogger::LOG_INFO, "CSocket::DestroySocket - sender thread stopped");
					}


					break;
				}
			}

			hSenderThread = NULL;

			sendQueue.clearQueue();
		}
	}

}



