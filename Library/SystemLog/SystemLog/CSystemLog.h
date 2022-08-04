#pragma once

#include <iostream>
#include <Windows.h>
#include <strsafe.h>
#include "CrashDump/CrashDump/CCrashDump.h"


namespace systemlog
{
	constexpr DWORD LOG_DIRECTORY_NAME_LENGTH = MAX_PATH;
	
	constexpr DWORD LOG_STRING_LENGTH = 3000;
}


class CSystemLog
{
private:

	class CCriticalSection 
	{
	public:
		CCriticalSection(CRITICAL_SECTION *pCriticalSection)
			: mpCriticalSection(pCriticalSection)
		{
			EnterCriticalSection(pCriticalSection);
		}

		~CCriticalSection(void)
		{
			LeaveCriticalSection(mpCriticalSection);
		}

		CRITICAL_SECTION* mpCriticalSection;
	};

	CSystemLog()
		: mbDirectorySetFlag(FALSE)
		, mCriticalSection({ 0, })
		, mLogLevel(eLogLevel::LogLevelDebug)
		, mLogCount(0)
		, mDirectoryName{ 0, }
	{
		InitializeCriticalSection(&mCriticalSection);
	}

	~CSystemLog()
	{
		DeleteCriticalSection(&mCriticalSection);
	}

public:

	enum class eLogLevel
	{	
		LogLevelDebug = 1,
		LogLevelNotice,
		LogLevelWarning,
		LogLevelError
	};


	// 싱글톤
	static CSystemLog* GetInstance(void)
	{
		static CSystemLog systemLog;

		return &systemLog;
	}

	BOOL SetLogDirectory(const WCHAR* directoryName)
	{	
		if (directoryName == nullptr)
		{
			return FALSE;
		}
		
		HRESULT retval;

		retval = StringCchCopyW(mDirectoryName, systemlog::LOG_DIRECTORY_NAME_LENGTH, directoryName);
		if (FAILED(retval))
		{	
			return FALSE;
		}

		mbDirectorySetFlag = TRUE;

		int ret = _wmkdir(mDirectoryName);
	
		return TRUE;
	}

	BOOL SetLogLevel(eLogLevel logLevel)
	{
		switch (logLevel)
		{
		case eLogLevel::LogLevelDebug:

			mLogLevel = logLevel;

			break;

		case eLogLevel::LogLevelNotice:

			mLogLevel = logLevel;

			break;

		case eLogLevel::LogLevelWarning:

			mLogLevel = logLevel;

			break;

		case eLogLevel::LogLevelError:

			mLogLevel = logLevel;
	
			break;
		default:
			return FALSE;
		}		

		return TRUE;
	}


	void Log(BOOL bConsolePrintFlag, eLogLevel logLevel, const WCHAR* logType, const WCHAR* format, ...)
	{
		if (mLogLevel > logLevel)
		{
			return;
		}

		if (mbDirectorySetFlag == FALSE)
		{
			if (SetLogDirectory(L"DefaultDirectory") == FALSE)
			{
				CCrashDump::Crash();
			}
		}

		INT64 logCount = InterlockedIncrement64(&mLogCount);

		WCHAR logString[systemlog::LOG_STRING_LENGTH] = { 0, };
		
		tm nowTime = { 0, };

		__time64_t time64 = NULL;
	
		if (setLogTime(&nowTime, &time64, logType) == FALSE)
		{
			return;
		}
		
		if (setLogInformation(&nowTime, logLevel, logType, logString, logCount) == FALSE)
		{
			return;
		}

		WCHAR logContents[systemlog::LOG_STRING_LENGTH] = { 0, };

		va_list va = NULL;

		va_start(va, format);

		HRESULT retval = NULL;

		retval = StringCchVPrintfW(logContents, _countof(logContents), format, va);
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ Log Function StringCchVPrintfW is Failed ] Error Code : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
			return;
		}

		retval = StringCchCatW(logString, _countof(logString), logContents);	
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ Log Function StringCchCatW is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
			return;
		}

		retval = StringCchCatW(logString, _countof(logString), L"\n\n");
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ Log Function StringCchCatW is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
			return;
		}

		if (printLogFile(&nowTime, logType, logString) == FALSE)
		{
			return;
		}

		if (bConsolePrintFlag == TRUE)
		{
			wprintf_s(logString);
		}

		return;
	}


	void LogHex(BOOL bConsolePrintFlag, eLogLevel logLevel, const WCHAR* logType, const WCHAR* pLogTitle, BYTE* pMemory, INT memorySize)
	{
		if (mLogLevel > logLevel)
		{
			return;
		}

		if (mbDirectorySetFlag == FALSE)
		{
			if (SetLogDirectory(L"DefaultDirectory") == FALSE)
			{
				CCrashDump::Crash();
			}
		}

		INT64 logCount = InterlockedIncrement64(&mLogCount);

		WCHAR logString[systemlog::LOG_STRING_LENGTH] = { 0, };

		tm nowTime = { 0, };

		__time64_t time64 = NULL;

		if (setLogTime(&nowTime, &time64, logType) == FALSE)
		{
			return;
		}

		if (setLogInformation(&nowTime, logLevel, logType, logString, logCount) == FALSE)
		{
			return;
		}

		HRESULT retval;

		WCHAR logTitle[MAX_PATH] = { 0, };

		retval = StringCchPrintfW(logTitle, _countof(logTitle), L"%s [ size : %d ]\n", pLogTitle, memorySize);
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchPrintfW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(),retval, logType, __LINE__);
			return;
		}

		retval = StringCchCatW(logString, _countof(logString), logTitle);
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchCatW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
			return;
		}

		// 문자 1개 2BYTE, NULL 2BYTE
		WCHAR hexBuffer[100] = { 0, };
		INT index = 0;
	
		while (index < memorySize)
		{
			retval = StringCchPrintfW(hexBuffer, _countof(hexBuffer), L"   0x%p   ", &pMemory[index]);
			if (FAILED(retval))
			{
				Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchPrintfW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
				return;
			}


			retval = StringCchCatW(logString, _countof(logString), hexBuffer);
			if (FAILED(retval))
			{
				Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchCatW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
				return;
			}

			for (INT count = 0; count < 8; ++count)
			{
				if (index < memorySize)
				{
					if (count == 7)
					{
						retval = StringCchPrintfW(hexBuffer, _countof(hexBuffer), L"%02x \n", pMemory[index]);
					}
					else
					{
						retval = StringCchPrintfW(hexBuffer, _countof(hexBuffer), L"%02x ", pMemory[index]);
					}
				}
				else
				{
					if (count == 7)
					{
						retval = StringCchPrintfW(hexBuffer, _countof(hexBuffer), L"00 \n");
					}
					else
					{
						retval = StringCchPrintfW(hexBuffer, _countof(hexBuffer), L"00 ");
					}
				}

				if (FAILED(retval))
				{
					Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchPrintfW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
					return;
				}

				retval = StringCchCatW(logString, _countof(logString), hexBuffer);
				if (FAILED(retval))
				{
					Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchCatW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
					return;
				}

				++index;
			}
		}

		retval = StringCchCatW(logString, _countof(logString), L"\n");
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ LogHex Function StringCchCatW Function is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);
			return;
		}

		if (printLogFile(&nowTime, logType, logString) == FALSE)
		{
			return;
		}

		if (bConsolePrintFlag == TRUE)
		{
			wprintf_s(logString);
		}

		return;
	}
	
	DWORD GetLogLevel(void)
	{
		return (DWORD)mLogLevel;
	}

private:

	BOOL setLogTime(tm* nowTime, __time64_t* time64, const WCHAR* logType)
	{
		errno_t retval;

		// 시스템 클록에 따라 자정 (00:00:00), 1970 년 1 월 1 일 Utc (협정 세계시) 이후 경과 된 시간 (초)을 반환 합니다.
		_time64(time64);

		// time_t 값으로 저장 된 시간을 변환 하 고 결과를 tm형식의 구조에 저장 합니다.
		retval = _localtime64_s(nowTime, time64);
		if (retval != 0)
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ Time Setting is Failed ] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);

			return FALSE;
		}

		return TRUE;
	}


	BOOL setLogInformation(tm* nowTime, eLogLevel logLevel, const WCHAR* logType, WCHAR* logString, INT64 logCount)
	{
		switch (logLevel)
		{
		case eLogLevel::LogLevelDebug:

			StringCchPrintfW(logString, systemlog::LOG_STRING_LENGTH, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d] [DEBUG] [%08lld] : ", logType, nowTime->tm_year + 1900, nowTime->tm_mon + 1, nowTime->tm_mday, nowTime->tm_hour, nowTime->tm_min, nowTime->tm_sec, logCount);

			break;

		case eLogLevel::LogLevelNotice:

			StringCchPrintfW(logString, systemlog::LOG_STRING_LENGTH, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d] [NOTICE] [%08lld] : ", logType, nowTime->tm_year + 1900, nowTime->tm_mon + 1, nowTime->tm_mday, nowTime->tm_hour, nowTime->tm_min, nowTime->tm_sec, logCount);

			break;

		case eLogLevel::LogLevelWarning:

			StringCchPrintfW(logString, systemlog::LOG_STRING_LENGTH, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d] [WARNING] [%08lld] : ", logType, nowTime->tm_year + 1900, nowTime->tm_mon + 1, nowTime->tm_mday, nowTime->tm_hour, nowTime->tm_min, nowTime->tm_sec, logCount);

			break;

		case eLogLevel::LogLevelError:

			StringCchPrintfW(logString, systemlog::LOG_STRING_LENGTH, L"[%s] [%04d-%02d-%02d %02d:%02d:%02d] [ERROR] [%08lld] : ", logType, nowTime->tm_year + 1900, nowTime->tm_mon + 1, nowTime->tm_mday, nowTime->tm_hour, nowTime->tm_min, nowTime->tm_sec, logCount);

			break;
		default:

			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[ Log Info Setting is Failed ] ErrorCoed : %d, LogLevel : %d, LogType : %s, Line : %d", GetLastError(), logLevel, logType, __LINE__);

			return FALSE;
		}

		return TRUE;
	}

	BOOL printLogFile(tm* nowTime, const WCHAR* logType, WCHAR* logString)
	{
		WCHAR fileName[MAX_PATH] = { 0, };

		HRESULT retval = NULL;

		retval = StringCchPrintfW(fileName, _countof(fileName), L"%s/[ %04d-%02d ]_[ %s ]", mDirectoryName, nowTime->tm_year + 1900, nowTime->tm_mon + 1, logType);
		if (FAILED(retval))
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[PrintLogFile Function StringCchPrintfW is Failed] ErrorCoed : %d, return : %d, LogType : %s, Line : %d", GetLastError(), retval, logType, __LINE__);

			return FALSE;
		}

		// file 개방 독점적 접근을 위한 Lock
		CCriticalSection criticalSection(&mCriticalSection);

		FILE* fp = nullptr;
		_wfopen_s(&fp, fileName, L"a+t");
		if (fp == nullptr)
		{
			Log(TRUE, eLogLevel::LogLevelError, L"Log Failed", L"[File Open is Failed] errorCode : %d, logType, fileName : %s", GetLastError(), logType, fileName);
			
			return FALSE;
		}
		else 
		{
			fwprintf_s(fp, logString);

			fclose(fp);
		}

		return TRUE;
	}

	BOOL mbDirectorySetFlag;

	CRITICAL_SECTION mCriticalSection;

	eLogLevel mLogLevel;
	
	INT64 mLogCount;

	WCHAR mDirectoryName[systemlog::LOG_DIRECTORY_NAME_LENGTH];
};