#pragma once

#include <iostream>
#include <Windows.h>

// Dump 클래스에 필요한 헤더
#include <psapi.h>
#include <dbghelp.h>

// Dump 클래스에 필요한 Lib
#pragma comment(lib, "DbgHelp.Lib")


class CCrashDump
{
private:

	CCrashDump(void)
	{
		//mDumpCount = 0;

		_invalid_parameter_handler oldHandler;
		_invalid_parameter_handler newHandler;

		newHandler = myInvalidParamterHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler); // CRT 함수에 null 포인터 등을 넣었을 때

		// 나타날 수 있는 에러들은 되도록이면 다 끄자.
		_CrtSetReportMode(_CRT_WARN, 0);						 // CRT 오류 메시지 표시 중단, 바로 덤프 남기도록
		_CrtSetReportMode(_CRT_ASSERT, 0);						 // CRT 오류 메시지 표시 중단, 바로 덤프 남기도록
		_CrtSetReportMode(_CRT_ERROR, 0);						 // CRT 오류 메시지 표시 중단, 바로 덤프 남기도록

		_CrtSetReportHook(_custom_Report_hook);


		// pure virtual function called 에러 핸들러를 상ㅇ자 정의 함수로 우회시킨다.
		_set_purecall_handler(mPurecallHandler);

		SetHandlerDump();
	}

	~CCrashDump()
	{
	}

public:

	static CCrashDump* GetInstance()
	{
		static CCrashDump crashDump;

		return &crashDump;
	}


	static void Crash(void)
	{
		LONG* ptr = nullptr;
		*ptr = 0;
	}

	static LONG WINAPI MyExceptionFilter(PEXCEPTION_POINTERS pExceptionPointer)
	{
		LONG workingMemory = 0;

		SYSTEMTIME stNowTime;

		LONG DumptCount = InterlockedIncrement(&mDumpCount);

		
		// 현재 프로세스의 메모리 사용량을 얻어온다.
		HANDLE hProcess = NULL;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = GetCurrentProcess();
		if (NULL == hProcess)
		{
			return 0;
		}

		// 메모리 덤프 크기 구하기
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			workingMemory = (LONG)(pmc.WorkingSetSize / 1024 / 1024);
		}

		CloseHandle(hProcess);


		// 현재 날짜와 시간을 알아온다.
		WCHAR filename[MAX_PATH];

		GetLocalTime(&stNowTime);

		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumptCount, workingMemory);

		wprintf_s(L"\n\n\n\n !!! Crash Error !!! %d%02d%02d_%02d.%02d.%02d",
			stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);

		wprintf_s(L"Now Save Dump File...\n");

		// 파일 생성
		HANDLE hDumpFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation = { 0, };

			MinidumpExceptionInformation.ThreadId = GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;

			// 외부에서 해당 프로세스의 덤프를 남길 때 사용하는 플래그인 것 같다.
			// 근데 TRUE,FALSE를 넣던 작동의 변화가 없다. 
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&MinidumpExceptionInformation,
				NULL,
				NULL
			);

			CloseHandle(hDumpFile);

			wprintf_s(L"CrashDump Save Finish ! \n");
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}


	// Invalid Paramter handler
	static void myInvalidParamterHandler(const WCHAR* expression, const WCHAR* function, const WCHAR* file, UINT line, uintptr_t pReserved)
	{
		Crash();
	}

	static LONG _custom_Report_hook(INT ireposttype, char* message, INT* returnvalue)
	{
		Crash();
		return TRUE;
	}

	static void mPurecallHandler(void)
	{
		Crash();
	}

	inline static LONG mDumpCount = 0;
};