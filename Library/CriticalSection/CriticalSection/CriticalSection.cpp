#include "stdafx.h"
#include "CCriticalSection.h"
#include <process.h>

constexpr DWORD THREAD_COUNT = 5;

CRITICAL_SECTION gCriticalSection;

INT num;

DWORD WINAPI WorkerThread(void* pParam)
{
	for (INT count = 0; count < 1000000; ++count)
	{
		{
			CCriticalSection criticalSection(&gCriticalSection);

			++num;
		}
	}

	return 1;
}

int main()
{
	InitializeCriticalSection(&gCriticalSection);

	HANDLE threadHandle[THREAD_COUNT];

	for (INT count = 0; count < THREAD_COUNT; ++count)
	{
		threadHandle[count] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)WorkerThread, 0, 0, 0);
	}
	
	WaitForMultipleObjects(THREAD_COUNT, threadHandle, TRUE, INFINITE);

	wprintf_s(L"%d\n", num);

	return 1;
}

