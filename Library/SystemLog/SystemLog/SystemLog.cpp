
#include <process.h>
#include <time.h>
#include "CSystemLog.h"

char gStr[30] = { 0, };

HANDLE gEvent = NULL;

HANDLE gHandleArray[10] = { 0, };

unsigned __stdcall TextThread(void* pParam)
{
	srand((unsigned)time(NULL));

	WaitForSingleObject(gEvent, INFINITE);

	for (int count = 0; count < 200; ++count)
	{
		CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"TEST", L"%s", L"TEXT");
	}
	
	return 1;
}

unsigned __stdcall HexThread(void* pParam)
{
	srand((unsigned)time(NULL));

	WaitForSingleObject(gEvent, INFINITE);

	for (int count = 0; count < 200; ++count)
	{
		CSystemLog::GetInstance()->LogHex(true, CSystemLog::eLogLevel::LogLevelError, L"TEST_HEX", L"String", (BYTE*)gStr, rand() % 30);
	}

	return 1;
}


int wmain()
{		
	//CSystemLog::GetInstance()->SetLogDirectory(L"TEST");

	CSystemLog::GetInstance()->SetLogLevel(CSystemLog::eLogLevel::LogLevelError);
	
	for (int index = 0; index < 30; ++index)
	{
		gStr[index] = index;
	}


	gEvent = CreateEvent(NULL, true, false, NULL);

	for (int index = 0; index < 5; ++index)
	{
		gHandleArray[index] = (HANDLE)_beginthreadex(NULL, 0, TextThread, NULL, NULL, NULL);
	}

	for (int index = 5; index < 10; ++index)
	{
		gHandleArray[index] = (HANDLE)_beginthreadex(NULL, 0, HexThread, NULL, NULL, NULL);
	}

	Sleep(2000);


	if (SetEvent(gEvent) == false)
	{
		wprintf(L"SetEvent Failed\n");
	}
	

	WaitForMultipleObjects(10, gHandleArray, true, INFINITE);

	
}

