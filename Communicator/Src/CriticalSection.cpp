/************************************************************************************************************************************************************************************************************
*@file			CriticalSection.cpp
*
*@brief			wrapping up the platform specific critical section
*
*@author		RAMESH KUMAR K
*
*@date			OCT 2016
**************************************************************************************************************************************************************************************************************/


# include "stdafx.h"
# include "ICriticalSection.h"


BOOL ICriticalSection::InitializeCriticalSection(UINT spincount)
{
	return InitializeCriticalSectionAndSpinCount ( this, spincount ) ;
}


BOOL ICriticalSection::AquireLock()
{
	EnterCriticalSection(this);

	return TRUE;
}

BOOL ICriticalSection::TryCaptureMutexLock()
{
	return TryEnterCriticalSection(this);
}

void ICriticalSection::ReleaseLock()
{
	LeaveCriticalSection(this);
}