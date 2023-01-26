#include "CTest1.h"
#include "CTest2.h"
//#include "CLockFreeObjectFreeList.h"
#include "CTLSLockFreeObjectFreeList.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include <process.h>
#include <conio.h>

#define dfTHREAD_COUNT 6

#define dfMAX_OBECT_COUNT 18

#define dfOBJECT_COUNT 3

class CPlayer {
public:

	CPlayer(){}

	~CPlayer(){}

	int arr[50];
};


constexpr int THREAD_COUNT = 6;
HANDLE gEvent;
HANDLE handles[THREAD_COUNT];

CTLSLockFreeObjectFreeList<CPlayer> gFreeList(6, TRUE);

// 0. Alloc (스레드당 10000 개 x 10 개 스레드 총 10만개)
// 1. 0x0000000055555555 과 0 이 맞는지 확인.
// 2. Interlocked + 1 (Data + 1 / Count + 1)
// 3. 약간대기
// 4. 여전히 0x0000000055555556 과 1 이 맞는지 확인.
// 5. Interlocked - 1 (Data - 1 / Count - 1)
// 6. 약간대기
// 7. 0x0000000055555555 과 Count == 0 확인.
// 8. Free
// 반복.

//unsigned int WINAPI FreeListTest(LPVOID pParam)
//{
//	int index = 0;
//	
//	DWORD threadID = GetCurrentThreadId();
//
//	CTest1** testArray = (CTest1**)malloc(sizeof(CTest1*) * dfOBJECT_COUNT);
//
//	for (;;)
//	{
//		for (index = 0; index < dfOBJECT_COUNT; ++index)
//		{
//			testArray[index] = gFreeList.Alloc();
//		}
//
//		for (index = 0; index < dfOBJECT_COUNT; ++index)
//		{
//			if (testArray[index]->mData != 0x0000000055555555 || testArray[index]->mCount != 0)
//			{
//				CCrashDump::Crash();
//			}
//
//			InterlockedIncrement64(&testArray[index]->mData);
//
//			InterlockedIncrement(&testArray[index]->mCount);
//		}
//
//		Sleep(0);
//
//		for (index = 0; index < dfOBJECT_COUNT; ++index)
//		{
//			if (testArray[index]->mData != 0x0000000055555556 || testArray[index]->mCount != 1)
//			{
//				CCrashDump::Crash();
//			}
//
//			InterlockedDecrement64(&testArray[index]->mData);
//
//			InterlockedDecrement(&testArray[index]->mCount);
//		}
//
//		Sleep(0);
//
//		for (index = 0; index < dfOBJECT_COUNT; ++index)
//		{
//			if (testArray[index]->mData != 0x0000000055555555 || testArray[index]->mCount != 0)
//			{
//				CCrashDump::Crash();
//			}
//			if (!gFreeList.Free(testArray[index]))
//			{
//				CCrashDump::Crash();
//			}
//		}
//	}
//
//	free(testArray);
//
//	return 1;
//}
//
//void threadStart(LPVOID pParam)
//{
//	HANDLE handleArray[dfTHREAD_COUNT] = { 0, };
//
//	for (int index = 0; index < dfTHREAD_COUNT; ++index)
//	{
//		handleArray[index] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)FreeListTest, NULL, NULL, NULL);
//	}
//
//	WaitForMultipleObjects(dfTHREAD_COUNT, handleArray, true, INFINITE);
//
//	for (int index = 0; index < dfTHREAD_COUNT; ++index)
//	{
//		CloseHandle(handleArray[index]);
//	}
//
//	return;
//}

unsigned TestThread(void* p) {

	WaitForSingleObject(gEvent, INFINITE);

	CPlayer* test[1000];

	for (int i = 0; i < 10000; ++i) 
	{
		{
			CTLSPerformanceProfiler performance(L"Alloc");

			for (int z = 0; z < 1000; ++z)
			{
				test[z] = gFreeList.Alloc();

				//test[z] = new CPlayer;
			}
		}

		{
			CTLSPerformanceProfiler performance(L"Free");

			for (int z = 0; z < 1000; ++z) 
			{
				gFreeList.Free(test[z]);

				// delete test[z];
			}
		}
	}

	return 1;
}


int wmain()
{	
	CCrashDump::GetInstance();
	CTLSPerformanceProfiler::SetPerformanceProfiler(L"TLSLockFreeFreeList", 6);
	gEvent = CreateEvent(nullptr, true, false, nullptr);

	for (int i = 0; i < THREAD_COUNT; ++i) {
		handles[i] = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)TestThread, nullptr, 0, nullptr);
	}

	Sleep(2000);
	std::cout << "Start";
	SetEvent(gEvent);

	WaitForMultipleObjects(THREAD_COUNT, handles, true, INFINITE);

	CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();

	//_getch();
	//
	//_beginthreadex(NULL, 0, (_beginthreadex_proc_type)threadStart, NULL, NULL, NULL);

	//int allocNodeCount = 0;

	//for (;;)
	//{
	//	allocNodeCount = gFreeList.GetAllocNodeCount();
	//	if (allocNodeCount > dfMAX_OBECT_COUNT)
	//	{
	//		CCrashDump::Crash();
	//	}

	//	wprintf_s(L"mAllocCount : %d\n", allocNodeCount);

	//	Sleep(1000);
	//}
	//
	return 1;
}

