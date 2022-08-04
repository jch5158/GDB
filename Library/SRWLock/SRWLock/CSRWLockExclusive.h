#pragma once

#include <iostream>
#include <Windows.h>


class CSRWLockExclusive
{
public:

	CSRWLockExclusive(SRWLOCK* pSRWLock)
		:mpSRWLock(pSRWLock)
	{
		AcquireSRWLockExclusive(pSRWLock);
	}

	~CSRWLockExclusive(void)
	{
		ReleaseSRWLockExclusive(mpSRWLock);
	}

private:

	CSRWLockExclusive(void)
		:mpSRWLock(nullptr)
	{}

	SRWLOCK* mpSRWLock;
};

