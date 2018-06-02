#pragma once
/************************************************************************************************************************************************************************************************************
*@file		SmartMutex.h
*
*@brief		wrapping up the platform specific mutex with exeption free support
*
*@author	RAMESH KUMAR K

**************************************************************************************************************************************************************************************************************/

template <class T>
struct LockGuard
{
     LockGuard(T& mutex) : _m_mutex(mutex) 
     { 
         _m_mutex.AquireLock();  // TODO operating system specific
     };

     ~LockGuard() 
     { 
          _m_mutex.ReleaseLock(); // TODO operating system specific
     }

   private:

     LockGuard(const LockGuard&); // or use c++0x ` = delete`

     T& _m_mutex;
};