
#include "stdafx.h"

#include "CrashDump/CrashDump/CCrashDump.h"
#include "CTest.h"
#include "CLockFreeStack.h"
#include <stack>

constexpr INT dfTHREAD_COUNT = 6;

//constexpr INT dfOBJECT_COUNT = 1;

//constexpr INT dfMAX_OBECT_COUNT = dfTHREAD_COUNT * dfOBJECT_COUNT;


HANDLE gTestEvent = NULL;

BOOL gbLoopFlag = TRUE;

BOOL gbControlModeFlag = FALSE;


CLockFreeStack<CTest*> gLockFreeStack;

 /*
 0. Alloc (스레드당 10000 개 x 10 개 스레드 총 10만개)
 1. 0x0000000055555555 과 0 이 맞는지 확인.
 2. Interlocked + 1 (Data + 1 / Count + 1)
 3. 약간대기
 4. 여전히 0x0000000055555556 과 1 이 맞는지 확인.
 5. Interlocked - 1 (Data - 1 / Count - 1)
 6. 약간대기
 7. 0x0000000055555555 과 Count == 0 확인.
 8. Free
 반복.
 */


DWORD WINAPI LockFreeStackThread(LPVOID pParam)
{
	DWORD threadID = GetCurrentThreadId();

	srand(time(NULL) + threadID);

	if (WaitForSingleObject(gTestEvent, INFINITE) == WAIT_FAILED)
	{
		CCrashDump::Crash();
	}

	int count = rand() % 9 + 1;

	CTest* p = new CTest[count];
	for (int index = 0; index < count; ++index)
		gLockFreeStack.Push(&p[index]);

	CTest** testArray = (CTest**)malloc(sizeof(CTest*) * count);

	//INT count = 0; count < 1000000; ++count
	for (;;)
	{
		for (INT index = 0; index < count; ++index)
		{
			for (;;)
			{
				if (gLockFreeStack.Pop(&testArray[index]) == TRUE)
					break;
			}
		}

		//Sleep(0);

		for (INT index = 0; index < count; ++index)
		{

			if (testArray[index]->mData != 0x0000000055555555 || testArray[index]->mCount != 0)
			{
				CCrashDump::Crash();
			}


			InterlockedIncrement64(&testArray[index]->mData);
			InterlockedIncrement(&testArray[index]->mCount);
		}
	
		//Sleep(0);

		for (INT index = 0; index < count; ++index)
		{
			if (testArray[index]->mData != 0x0000000055555556 || testArray[index]->mCount != 1)
			{
				CCrashDump::Crash();
			}


			InterlockedDecrement64(&testArray[index]->mData);
			InterlockedDecrement(&testArray[index]->mCount);
		}

		//Sleep(0);

		for (INT index = 0; index < count; ++index)
		{

			if (testArray[index]->mData != 0x0000000055555555 || testArray[index]->mCount != 0)
			{
				CCrashDump::Crash();
			}

			gLockFreeStack.Push(testArray[index]);
		}	

		//Sleep(0);
	}

	free(testArray);

	return 1;
}


DWORD WINAPI LockFreeStackTestStart(LPVOID pParam)
{
	gTestEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	HANDLE handleArray[dfTHREAD_COUNT] = { 0, };

	for (INT index = 0; index < dfTHREAD_COUNT; ++index)
	{
		handleArray[index] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)LockFreeStackThread, NULL, NULL, NULL);
	}

	Sleep(2000);

	SetEvent(gTestEvent);

	if (WaitForMultipleObjects(dfTHREAD_COUNT, handleArray, TRUE, INFINITE) == WAIT_FAILED)
	{
		CCrashDump::Crash();
	}

	for (INT index = 0; index < dfTHREAD_COUNT; ++index)
	{
		CloseHandle(handleArray[index]);
	}


	//if (CPerformanceProfiler::PrintPerformanceProfile() == TRUE)
	//{
	//	wprintf_s(L"print complete\n");
	//}

	//if (CPerformanceProfiler::PrintTotalPerformanceProfile() == TRUE)
	//{
	//	wprintf_s(L"print complete\n");
	//}

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

			gbControlModeFlag = TRUE;
		}

		if (controlKey == L'l' || controlKey == L'L')
		{
			wprintf(L"입력 모드 종료\n");
			gbControlModeFlag = FALSE;
		}

		if ((controlKey == L'q' || controlKey == L'Q') && gbControlModeFlag)
		{
			gbLoopFlag = FALSE;
		}
	}

	return;
}

//
//constexpr int THREAD_COUNT = 6;
//HANDLE gEvent;
//HANDLE gHandles[THREAD_COUNT];
//CLockFreeStack<long long> gLockFreeStack;
//std::stack<long long> gSTLStack;
//CRITICAL_SECTION gCritical;
//SRWLOCK gSrwLock;
//
//
//
//unsigned PushThread(void* p) {
//	WaitForSingleObject(gEvent, INFINITE);
//
//	for (int i = 0; i < 1000; ++i) 
//	{
//		{
//			CTLSPerformanceProfiler profiler(L"Push");
//			
//			for (int y = 0; y < 10000; ++y) 
//			{
//				//gLockFreeStack.Push(10);
//
//				AcquireSRWLockExclusive(&gSrwLock);
//
//				////EnterCriticalSection(&gCritical);
//
//				gSTLStack.push(10);
//					
//				////LeaveCriticalSection(&gCritical);
//
//				ReleaseSRWLockExclusive(&gSrwLock);
//			}
//		}
//	}
//
//	return 1;
//}
//
//unsigned PopThread(void* p) {
//
//	WaitForSingleObject(gEvent, INFINITE);
//
//	for (int i = 0; i < 1000; ++i)
//	{
//		{
//			CTLSPerformanceProfiler profiler(L"Pop");
//
//			for (int y = 0; y < 10000; ++y)
//			{
//				long long num;
//
//				//if (gLockFreeStack.Size() == 0 || gLockFreeStack.Pop(&num) == false) {
//				//	--y;
//				//	continue;
//				//}
//
//
//				AcquireSRWLockExclusive(&gSrwLock);
//
//				////EnterCriticalSection(&gCritical);
//
//				if (gSTLStack.size() == 0) {
//					//LeaveCriticalSection(&gCritical);
//					ReleaseSRWLockExclusive(&gSrwLock);
//					--y;
//					continue;
//				}
//
//				num = gSTLStack.top();
//				gSTLStack.pop();
//
//				////LeaveCriticalSection(&gCritical);
//
//				ReleaseSRWLockExclusive(&gSrwLock);
//			}
//		}
//	}
//
//	return 1;
//}


int wmain()
{
	//InitializeCriticalSection(&gCritical);

	//InitializeSRWLock(&gSrwLock);

	//gEvent = CreateEvent(nullptr, true, false, nullptr);

	//CTLSPerformanceProfiler::SetPerformanceProfiler(L"SRWLock Stack", THREAD_COUNT);

	//for (int i = 0; i < 3; ++i) {
	//	gHandles[i] = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)PushThread, nullptr, 0, nullptr);
	//}

	//for (int i = 3; i < 6; ++i) {
	//	gHandles[i] = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)PopThread, nullptr, 0, nullptr);
	//}

	//Sleep(2000);
	//std::cout << "Start";
	//SetEvent(gEvent);

	//WaitForMultipleObjects(THREAD_COUNT, gHandles, true, INFINITE);
	//CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();


	_wsetlocale(LC_ALL, L"");

	CCrashDump::GetInstance();

	_getwch();

	wprintf_s(L"begin\n");

	HANDLE testHandle = (HANDLE)_beginthreadex(NULL, 0,(_beginthreadex_proc_type)LockFreeStackTestStart, NULL, NULL, NULL);

	for (;;)
	{
		ServerControling();

		//wprintf(L"Object FreeList Size : %d\n", CLockFreeObjectFreeList<CLockFreeStack<CTest*>::stNode>::GetAllocNodeCount());

		wprintf(L"Stack Size : %d\n", gLockFreeStack.Size());
	
		if (WaitForSingleObject(testHandle, 1000) == WAIT_OBJECT_0)
		{
			break;
		}
	}

	int count = 0;
	for (;;)
	{
		CTest* pTemp;

		if (gLockFreeStack.Pop(&pTemp) == FALSE)
		{
			break;
		}

		++count;
	}


	std::cout << count << std::endl;

	wprintf(L"finish, Stack Size : %d\n", gLockFreeStack.Size());

	return 1;
}