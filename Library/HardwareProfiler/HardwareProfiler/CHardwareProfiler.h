#pragma once

#include <iostream>
#include <Windows.h>
#include <strsafe.h>
#include <Pdh.h>
#include <psapi.h>
#include "CrashDump/CrashDump/CCrashDump.h"

#pragma comment(lib,"Pdh.lib")

class CHardwareProfiler
{
public:

	static CHardwareProfiler* GetInstance()
	{
		static CHardwareProfiler hardwareProfiler;

		return &hardwareProfiler;
	}

	static void SetHardwareProfiler(BOOL bPrivateBytesFlag, BOOL bNonpagedPoolFlag, BOOL bAvailableBytesFlag, BOOL bSendBytesFlag, BOOL bRecvBytesFlag, const WCHAR* pNetworkAdaptorName, const WCHAR* pProcessName = nullptr)
	{
		mbPrivateBytesFlag = bPrivateBytesFlag;

		mbNonpagedPoolFlag = bNonpagedPoolFlag;

		mbAvailableBytesFlag = bAvailableBytesFlag;

		mbSendBytesFlag = bSendBytesFlag;

		mbRecvBytesFlag = bRecvBytesFlag;

		setNetworkAdaptorName(pNetworkAdaptorName);

		setProcessFileName(pProcessName);
		
		return;
	}


	void UpdateHardwareProfiler(void)
	{
		PdhCollectQueryData(mHaredwareQuery);

		return;
	}

	DOUBLE GetPrivateBytes(void)
	{
		PDH_FMT_COUNTERVALUE counterValue;
	
		if(PdhGetFormattedCounterValue(mPrivateBytesCounter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS)
		{
			return -1;
		}

		return counterValue.doubleValue;
	}

	DOUBLE GetNonpagedPool(void)
	{
		PDH_FMT_COUNTERVALUE counterValue;

		if (PdhGetFormattedCounterValue(mNonpagedPoolCounter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS)
		{
			return -1;
		}

		return counterValue.doubleValue;
	}

	DOUBLE GetAvailableMegaBytes(void)
	{
		PDH_FMT_COUNTERVALUE counterValue;

		if (PdhGetFormattedCounterValue(mAvailableBytesCounter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS)
		{
			return -1;
		}

		return counterValue.doubleValue;
	}

	DOUBLE GetSendBytes(void)
	{
		PDH_FMT_COUNTERVALUE counterValue;

		if (PdhGetFormattedCounterValue(mSendBytesCounter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS)
		{
			return -1;
		}

		return counterValue.doubleValue;
	}


	DOUBLE GetRecvBytes(void)
	{
		PDH_FMT_COUNTERVALUE counterValue;

		if (PdhGetFormattedCounterValue(mRecvBytesCounter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS)
		{
			return -1;
		}

		return counterValue.doubleValue;
	}


private:

	CHardwareProfiler(void)
		: mHaredwareQuery(INVALID_HANDLE_VALUE)
		, mPrivateBytesCounter{ 0, }
		, mNonpagedPoolCounter{ 0, }
		, mAvailableBytesCounter{ 0, }
		, mSendBytesCounter{ 0, }
		, mRecvBytesCounter{ 0, }
	{
		PdhOpenQuery(NULL, NULL, &mHaredwareQuery);

		setPrivateBytesQuery();
			
		setNonpagedBytes();	

		setAvailableMegaBytesQuery();
	
		setSendBytesQuery();

		setRecvBytesQuery();

	}

	~CHardwareProfiler(void)
	{

	}


	static inline void setNetworkAdaptorName(const WCHAR* pNetworkAdaptorName)
	{
		if (pNetworkAdaptorName != nullptr)
		{
			wcscpy_s(mpNetworkAdaptorName, pNetworkAdaptorName);
		}

		return;
	}

	static inline void setProcessFileName(const WCHAR* pProcessName)
	{
		if (pProcessName == nullptr)
		{
			GetModuleBaseNameW(GetCurrentProcess(), NULL, mpProcessName, MAX_PATH);

			WCHAR* ptr = mpProcessName;

			INT index = 0;

			for (;;)
			{
				// .exe Á¦°Å
				if (ptr[index] == L'.')
				{
					ptr[index] = L'\0';

					break;
				}

				++index;
			}
		}
		else
		{
			wcscpy_s(mpProcessName, pProcessName);
		}

		return;
	}


	void setPrivateBytesQuery(void)
	{
		if (mbPrivateBytesFlag == TRUE)
		{
			INT retval = 0;

			WCHAR query[MAX_PATH] = { 0, };

			retval = StringCchPrintfW(query, _countof(query), L"\\Process(%s)\\Private Bytes", mpProcessName);
			if (FAILED(retval))
			{
				CCrashDump::Crash();

				return;
			}

			PdhAddCounter(mHaredwareQuery, query, NULL, &mPrivateBytesCounter);
		}

		return;
	}

	void setNonpagedBytes(void)
	{
		if (mbNonpagedPoolFlag == TRUE)
		{
			PdhAddCounter(mHaredwareQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &mNonpagedPoolCounter);
		}

		return;
	}


	void setAvailableMegaBytesQuery(void)
	{
		if (mbAvailableBytesFlag == TRUE)
		{
			PdhAddCounter(mHaredwareQuery, L"\\Memory\\Available MBytes", NULL, &mAvailableBytesCounter);
		}

		return;
	}

	void setSendBytesQuery(void)
	{
		if (mbSendBytesFlag == TRUE)
		{
			INT retval = 0;

			WCHAR query[MAX_PATH] = { 0, };

			retval = StringCchPrintfW(query, _countof(query), L"\\Network Interface(%s)\\Bytes Sent/sec", mpNetworkAdaptorName);
			if (FAILED(retval))
			{
				CCrashDump::Crash();

				return;
			}

			PdhAddCounter(mHaredwareQuery, query, NULL, &mSendBytesCounter);
		}

		return;
	}

	void setRecvBytesQuery(void)
	{
		
		if (mbRecvBytesFlag == TRUE)
		{
			INT retval = 0;

			WCHAR query[MAX_PATH] = { 0, };

			retval = StringCchPrintfW(query, _countof(query), L"\\Network Interface(%s)\\Bytes Received/sec", mpNetworkAdaptorName);
			if (FAILED(retval))
			{
				CCrashDump::Crash();

				return;
			}

			PdhAddCounter(mHaredwareQuery, query, NULL, &mRecvBytesCounter);
		}

		return;
	}


	static inline BOOL mbPrivateBytesFlag = TRUE;

	static inline BOOL mbNonpagedPoolFlag = TRUE;

	static inline BOOL mbAvailableBytesFlag = TRUE;

	static inline BOOL mbSendBytesFlag = TRUE;

	static inline BOOL mbRecvBytesFlag = TRUE;

	static inline WCHAR mpNetworkAdaptorName[MAX_PATH] = { 0, };

	static inline WCHAR mpProcessName[MAX_PATH] = { 0, };

	PDH_HQUERY mHaredwareQuery;

	PDH_HCOUNTER mPrivateBytesCounter;

	PDH_HCOUNTER mNonpagedPoolCounter;

	PDH_HCOUNTER mAvailableBytesCounter;

	PDH_HCOUNTER mSendBytesCounter;

	PDH_HCOUNTER mRecvBytesCounter;

};

