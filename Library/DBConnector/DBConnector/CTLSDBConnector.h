#pragma once

#include "CDBConnector.h"


class CTLSDBConnector
{
public:

	CTLSDBConnector(void)
		:mTLSIndex(0)
	{
		mTLSIndex = TlsAlloc();
		if (mTLSIndex == TLS_OUT_OF_INDEXES)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSDBConnector", L"[CTLSDBConnector] Error Code : %d", GetLastError());

			CCrashDump::Crash();
		}
	}

	~CTLSDBConnector(void)
	{
		if (TlsFree(mTLSIndex) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSDBConnector", L"[~CTLSDBConnector] Error Code : %d", GetLastError());

			CCrashDump::Crash();
		}
	}

	bool Connect(const wchar_t* pConnectIP, unsigned short connectPort, const wchar_t* pSchema, const wchar_t* pUserID, const wchar_t* pUserPassword)
	{
		CDBConnector* pDBConnector = getDBConnector();

		if (pDBConnector->Connect(pConnectIP, connectPort, pSchema, pUserID, pUserPassword) == false)
			return false;

		return true;
	}

	bool Reconnect(void)
	{
		CDBConnector* pDBConnector = getDBConnector();
		if (pDBConnector->Reconnect() == false)
			return false;

		return true;
	}

	void Disconnect(void)
	{
		CDBConnector* pDBConnector = getDBConnector();

		pDBConnector->Disconnect();

		delete pDBConnector;

		if (TlsSetValue(mTLSIndex, nullptr) == 0)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSDBConnector", L"[Disconnect] Error Code : %d", GetLastError());

			CCrashDump::Crash();
		}

		return;
	}

	bool MySQLCharacterSet(wchar_t* pCharacterSet)
	{
		CDBConnector* pDBConnector = getDBConnector();
		if (pDBConnector->MySQLCharacterSet(pCharacterSet) == false)
			return false;

		return true;
	}


	bool Query(const wchar_t* pQueryFormat, ...)
	{
		CDBConnector* pDBConnector = getDBConnector();

		wchar_t wideByteQuery[dbconnector::MAX_QUERY_LENGTH];

		va_list va;
		va_start(va, pQueryFormat);

		HRESULT retval = StringCchVPrintfW(wideByteQuery, dbconnector::MAX_QUERY_LENGTH, pQueryFormat, va);
		if (FAILED(retval))
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"DBConnector", L"[Query] Error Code : %d, return : %d, format : %s, va : %s", GetLastError(), retval, pQueryFormat, va);

			CCrashDump::Crash();
		}

		if (pDBConnector->Query(wideByteQuery) == false)
			return false;

		return true;
	}

	void StoreResult(void)
	{
		getDBConnector()->StoreResult();

		return;
	}

	MYSQL_ROW FetchRow(void)
	{		
		return getDBConnector()->FetchRow();
	}

	void FreeResult(void)
	{		
		getDBConnector()->FreeResult();

		return;
	}

	bool GetConnectStateFlag(void)
	{	
		return getDBConnector()->GetConnectStateFlag();
	}

	unsigned int GetLastError(void)
	{	
		return getDBConnector()->GetLastError();
	}

	const wchar_t* GetLastErrorMessage(void)
	{	
		return getDBConnector()->GetLastErrorMessage();
	}

	bool CheckReconnectErrorCode(void)
	{
		return getDBConnector()->CheckReconnectErrorCode();
	}

private:

	CDBConnector* getDBConnector(void)
	{
		CDBConnector* pDBConnector = (CDBConnector*)TlsGetValue(mTLSIndex);
		if (pDBConnector == nullptr)
		{
			pDBConnector = new CDBConnector;

			if (TlsSetValue(mTLSIndex, (PVOID)pDBConnector) == 0)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSDBConnector", L"[setDBConnector] Error Code : %d", GetLastError());

				CCrashDump::Crash();
			}
		}

		return pDBConnector;
	}

	unsigned int mTLSIndex;
};

