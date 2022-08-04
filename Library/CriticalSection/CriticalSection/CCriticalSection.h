#pragma once

#include <iostream>
#include <Windows.h>

class CCriticalSection
{
public:

	CCriticalSection(CRITICAL_SECTION *pCriticalSection)
		:mpCriticalSection(pCriticalSection)
	{
		EnterCriticalSection(pCriticalSection);
	}

	~CCriticalSection(void)
	{
		LeaveCriticalSection(mpCriticalSection);
	}

private:

	CRITICAL_SECTION* mpCriticalSection;

};

