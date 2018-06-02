#pragma once
/************************************************************************************************************************************************************************************************************
*@file		CriticalSection.h
*
*@brief		wrapping up the platform specific critical section
*
*@author	RAMESH KUMAR K

**************************************************************************************************************************************************************************************************************/


# include "stdafx.h"
# include "ReferenceCounter.h"

#define DEFAULT_SPIN_COUNT_FOR_MUTEXES 5000

class ICriticalSection : public CRITICAL_SECTION, public IReferenceCounter
{

public:

  ICriticalSection()
  { 
	  ::InitializeCriticalSection(this); 
  }
  ~ICriticalSection() 
  {   
	  DeleteCriticalSection(this);  
  }

  void Initialize (int iterations)
  {
	  InitializeCriticalSection(iterations);
  }


  ICriticalSection(UINT spincount) 
  { 
	  InitializeCriticalSection(spincount);
  }


  BOOL InitializeCriticalSection ( UINT Spin_count ) ;

  BOOL AquireLock();

  BOOL TryCaptureMutexLock();

  void ReleaseLock();

};