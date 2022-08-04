#pragma once

#include <iostream>
#include <Windows.h>

class CSRWLockShared
{
public:

	CSRWLockShared(SRWLOCK* pSRWLock)
		:mpSRWLock(pSRWLock)
	{
		AcquireSRWLockShared(pSRWLock);
	}

	~CSRWLockShared(void)
	{
		ReleaseSRWLockShared(mpSRWLock);
	}

private:

	CSRWLockShared(void)
		:mpSRWLock(nullptr)
	{}

	SRWLOCK* mpSRWLock;

};

