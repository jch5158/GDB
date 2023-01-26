#include "stdafx.h"
#include <process.h>

constexpr int THREAD_COUNT = 6;
constexpr int M_SIZE = 2000;
CLockFreeFreeListManager gMemoryFreeList;
HANDLE handles[THREAD_COUNT];
HANDLE gEvent;


unsigned WorkerThread1(void* p) {

	WaitForSingleObject(gEvent, INFINITE);

	void* memories[1000];

	for (int i = 0; i < 10000; ++i) {

		{
			CTLSPerformanceProfiler profiler(L"malloc");

			for (int z = 0; z < 1000; ++z) {
				memories[z] = malloc(M_SIZE);
			}
		}

		{
			CTLSPerformanceProfiler profiler(L"free");

			for (int z = 0; z < 1000; ++z) {
				free(memories[z]);
			}
		}
	}

	return 1;
}

unsigned WorkerThread2(void* p) {

	WaitForSingleObject(gEvent, INFINITE);

	void* memories[1000];

	for (int i = 0; i < 10000; ++i) {

		{
			CTLSPerformanceProfiler profiler(L"Alloc");

			for (int z = 0; z < 1000; ++z) {
				memories[z] = gMemoryFreeList.Alloc(M_SIZE);
			}
		}

		{
			CTLSPerformanceProfiler profiler(L"Free");

			for (int z = 0; z < 1000; ++z) {
				gMemoryFreeList.Free(memories[z]);
			}
		}
	}

	return 1;
}


int main()
{
	LARGE_INTEGER l;
	QueryPerformanceFrequency(&l);

	std::cout << l.QuadPart;

	//CTLSPerformanceProfiler::SetPerformanceProfiler(L"Alloc, Free 2000", THREAD_COUNT);

	//gEvent = CreateEvent(nullptr, true, false, nullptr);

	//for (int i = 0; i < THREAD_COUNT; ++i) {
	//	handles[i] = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)WorkerThread2, nullptr, 0, nullptr);
	//}

	//Sleep(2000);

	//std::cout << "Start";

	//SetEvent(gEvent);

	//WaitForMultipleObjects(THREAD_COUNT, handles, true, INFINITE);
	//CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();

	return 1;
}
