#include "stdafx.h"
#include "CSRWLockShared.h"
#include "CSRWLockExclusive.h"

int gNum;

HANDLE gEvent;

DWORD WINAPI WorkerThread(void* pParam)
{
	WaitForSingleObject(gEvent, INFINITE);

	SRWLOCK* pSRWLock = (SRWLOCK*)pParam;

	for (int count = 0; count < 100000; ++count)
	{
		{
			CSRWLockExclusive srwLock(pSRWLock);

			gNum++;
		}
	}

	return 1;
}


int main()
{
	HANDLE handles[3];
	
	CreateEvent(nullptr, true, false, nullptr);

	SRWLOCK srwLock;

	InitializeSRWLock(&srwLock);

	for (int count = 0; count < 3; ++count)
	{
		handles[count] = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)WorkerThread, &srwLock, 0, 0);
	}

	Sleep(1000);

	SetEvent(gEvent);

	WaitForMultipleObjects(3, handles, true, INFINITE);

	wprintf(L"%d", gNum);

	return 1;
}
