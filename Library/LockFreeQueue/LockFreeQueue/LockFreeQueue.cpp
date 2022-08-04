#include "stdafx.h"
#include "CTest.h"

constexpr INT THREAD_COUNT = 6;

//constexpr INT dfOBJECT_COUNT = 1;

//constexpr INT dfMAX_OBECT_COUNT = THREAD_COUNT * dfOBJECT_COUNT;


BOOL gLoopFlag = TRUE;

BOOL gControlModeFlag = FALSE;

CLockFreeQueue<CTest*> gLockFreeQueue;

HANDLE gEvent = NULL;

DWORD WINAPI LockFreeQueueTestThread(LPVOID pParam)
{
	DWORD threadID = GetCurrentThreadId();

	srand(time(NULL) + threadID);

	if (WaitForSingleObject(gEvent, INFINITE) == WAIT_FAILED)
	{
		CCrashDump::Crash();
	}

	int count = rand() % 9 + 1;

	CTest* p = new CTest[count];
	for (int index = 0; index < count; ++index)
		gLockFreeQueue.Enqueue(&p[index]);


	CTest** testArray = (CTest**)malloc(sizeof(CTest*) * count);
	if (testArray == nullptr)
	{
		CCrashDump::Crash();

		return 1;
	}

	for (;;)
	{
		for (INT index = 0; index < count; ++index)
		{
			if (gLockFreeQueue.Dequeue(&testArray[index]) == FALSE)
			{
				CCrashDump::Crash();
			}
		}

		for (INT index = 0; index < count; ++index)
		{
			if (testArray[index]->mData != 0x0000000055555555 || testArray[index]->mCount != 0)
			{
				CCrashDump::Crash();
			}

			InterlockedIncrement64(&testArray[index]->mData);

			InterlockedIncrement(&testArray[index]->mCount);
		}

		Sleep(0);

		for (INT index = 0; index < count; ++index)
		{
			if (testArray[index]->mData != 0x0000000055555556 || testArray[index]->mCount != 1)
			{
				CCrashDump::Crash();
			}

			InterlockedDecrement64(&testArray[index]->mData);

			InterlockedDecrement(&testArray[index]->mCount);
		}

		Sleep(0);

		for (INT index = 0; index < count; ++index)
		{
			if (testArray[index]->mData != 0x0000000055555555 || testArray[index]->mCount != 0)
			{
				CCrashDump::Crash();
			}

			gLockFreeQueue.Enqueue(testArray[index]);
		}

	}

	free(testArray);

	return 1;
}


DWORD WINAPI TestThread(LPVOID pParam)
{
	gEvent = CreateEvent(NULL, TRUE, FALSE, nullptr);

	HANDLE handleArray[THREAD_COUNT] = { 0, };

	for (INT index = 0; index < THREAD_COUNT; ++index)
	{
		handleArray[index] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)LockFreeQueueTestThread, NULL, NULL, NULL);
	}

	wprintf_s(L"good\n");

	Sleep(1000);

	SetEvent(gEvent);

	if (WaitForMultipleObjects(THREAD_COUNT, handleArray, TRUE, INFINITE) == WAIT_FAILED)
	{
		CCrashDump::Crash();
	}

	for (INT index = 0; index < THREAD_COUNT; ++index)
	{
		CloseHandle(handleArray[index]);
	}

	return 1;
}

void ServerControling(void)
{

	if (_kbhit())
	{
		WCHAR controlKey = _getwch();

		if (controlKey == L'u' || controlKey == L'U')
		{
			wprintf_s(L"Q 입력 시 테스트 종료 \n");
			wprintf_s(L"L 입력 시 입력모드 종료\n");

			gControlModeFlag = TRUE;
		}

		if (controlKey == L'p' || controlKey == L'P')
		{
			CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();
		}

		if (controlKey == L'l' || controlKey == L'L')
		{
			wprintf(L"입력 모드 종료\n");
			gControlModeFlag = FALSE;
		}

		if ((controlKey == L'q' || controlKey == L'Q') && gControlModeFlag)
		{
			gLoopFlag = FALSE;
		}
	}
}

INT wmain()
{
	_wsetlocale(LC_ALL, L"");	

	//CTLSPerformanceProfiler::SetPerformanceProfiler(L"LockFreeQueue", THREAD_COUNT);

	//CTest* test[dfMAX_OBECT_COUNT];

	//for (INT index = 0; index < dfMAX_OBECT_COUNT; ++index)
	//{
	//	test[index] = new CTest;
	//	gLockFreeQueue.Enqueue(test[index]);
	//}

	_getch();

	HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)TestThread, NULL, NULL, NULL);

	for (;;)
	{

		ServerControling();

		wprintf(L"Queue Size : %d\n\n", gLockFreeQueue.GetUseSize());

		if (WaitForSingleObject(threadHandle, 1000) == WAIT_OBJECT_0)
		{
			break;
		}
	}


	CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();

	
	return 1;
}

