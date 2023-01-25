#include "CRingBuffer.h"
#include "CTemplateRingBuffer.h"
#include "PerformanceProfiler/PerformanceProfiler/CPerformanceProfiler.h"

#include <process.h>
#include <string.h>
#include <time.h>
#include <conio.h>

char testText[] = "1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 12345";

char* textPtr = testText;

int textLength = strlen(testText);

char printText[100];


int enqueueSize = 0;

int index = 0;

HANDLE gEvent;

class CTest
{
public:

	CTest()
	{}

	~CTest()
	{}

	void Print(void)
	{
		wprintf_s(L"good : %d\n", num);
	}

	int num = 0;

};


//CTemplateRingBuffer<char> cQ;

CRingBuffer cQ;

//CTemplateRingBuffer<CTest*> cQ;

void EnqueueTest()
{
	int randValue = 0;

	int directEnqSize = 0;

	randValue = rand() % 20;

	if (index + randValue > textLength)
	{
		randValue = textLength - index;
	}


	directEnqSize = cQ.GetDirectEnqueueSize();
	if (randValue > directEnqSize)
	{
		randValue = directEnqSize;
	}

	enqueueSize = cQ.Enqueue(textPtr, randValue);
	index += enqueueSize;

	if (index >= textLength)
	{
		index = 0;
	}

	textPtr = &testText[index];



	//int randValue = 0;

	//int directEnqSize = 0;

	//randValue = rand() % 20;

	//directEnqSize = cQ.GetDirectEnqueueSize();
	//if (randValue > directEnqSize)
	//{
	//	randValue = directEnqSize;
	//}

	//CTest* ptr[20];

	//for (int index = 0; index < randValue; ++index)
	//{
	//	static int num = 0;

	//	ptr[index] = new CTest;
	//	ptr[index]->num = num++;
	//}

	//enqueueSize = cQ.Enqueue(ptr, randValue);	


	//CTest* ptr = new CTest;

	//static int num = 0;

	//ptr->num = num;

	//if (cQ.Enqueue(ptr) == TRUE)
	//{
	//	++num;
	//}

	return;
}

void DequeueTest()
{
	int randValue = 0;

	int direcvDeqSize = 0;

	randValue = rand() % 30 ;

	direcvDeqSize = cQ.GetDirectDequeueSize();
	if (randValue > direcvDeqSize)
	{
		randValue = direcvDeqSize;
	}

	cQ.Dequeue(printText, randValue);

	//int peekSize;
	//peekSize = cQ.Peek(printText, randValue);

	//cQ.MoveFront(peekSize);

	printf_s(printText);

	memset(printText, 0, 100);


	//int randValue = 0;

	//int direcvDeqSize = 0;

	//randValue = rand() % 50;

	//direcvDeqSize = cQ.GetDirectDequeueSize();
	//if (randValue > direcvDeqSize)
	//{
	//	randValue = direcvDeqSize;
	//}

	//CTest* test[50];
	//int retval = cQ.Dequeue(test, randValue);

	//for (int index = 0; index < retval; ++index)
	//{
	//	test[index]->Print();
	//	delete test[index];
	//}

	//CTest* test;
	//if (cQ.Dequeue(&test) == TRUE)
	//{
	//	test->Print();
	//
	//	delete test;
	//}

	return;
}


DWORD WINAPI EnqueueThread(LPVOID lpParam)
{
	srand((unsigned int)time(NULL));

	WaitForSingleObject(gEvent, INFINITE);

	DWORD time = timeGetTime();

	while (true)
	{
		EnqueueTest();

		Sleep(10);
	}

	return 1;
}

DWORD WINAPI DequeueThread(LPVOID lpParam)
{
	srand((unsigned int)time(NULL));

	WaitForSingleObject(gEvent, INFINITE);

	DWORD time = timeGetTime();

	while (true)
	{
		DequeueTest();

		Sleep(10);
	}

	return 1;
}


int main()
{
	srand((unsigned int)time(NULL));

	memset(printText, '\0', 100);

	gEvent = CreateEvent(NULL, true, false, nullptr);

	HANDLE handles[2];

	handles[0] = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)EnqueueThread, NULL, NULL, NULL);

	handles[1] = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)DequeueThread, NULL, NULL, NULL);

	Sleep(1000);

	SetEvent(gEvent);

	WaitForMultipleObjects(2, handles, true, INFINITE);

	return 1;
}
