# include "Windows.h"

HANDLE getEvent(LPSTR eventName)
{
	return ::CreateEventA(NULL, false, false, eventName);
}