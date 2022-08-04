#pragma once
#include <iostream>
#include <Windows.h>
#include <strsafe.h>
#include "SystemLog/SystemLog/CSystemLog.h"
#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#pragma comment(lib,"mysql/lib/vs14/mysqlclient.lib")

namespace dbconnector
{	
	constexpr int MAX_QUERY_LENGTH = 2000;

	constexpr int MAX_ERROR_MESSAGE_LENGTH = 300;
};


// DB 연결 및 쿼리 전송 그리고 예외처리 정도만 하는 클래스이다. 
class CDBConnector
{
public:

	CDBConnector(void)
		: mMySQL{ 0, }
		, mpMySQL(nullptr)
		, mpMySQLResult(nullptr)
		, mbConnectStateFlag(false)
		, mLastErrorCode(0)
		, mConnectPort(0)
		, mConnectIP()
		, mSchema()
		, mUserID()
		, mUserPassword()
		, mLastErrorMessage(dbconnector::MAX_ERROR_MESSAGE_LENGTH,L'\0')
		, mWideByteQuery(dbconnector::MAX_QUERY_LENGTH,L'\0')
		, mMultiByteQuery(dbconnector::MAX_QUERY_LENGTH, '\0')
		
	{
		mysql_init(&mMySQL);

		// MYSQL ping 호출 시 재연결 설정
		bool reconnect = true; // 1;

		mysql_options(&mMySQL, MYSQL_OPT_RECONNECT, &reconnect);
	}

	~CDBConnector(void)
	{}


	CDBConnector(const CDBConnector&) = delete;
	CDBConnector& operator=(const CDBConnector&) = delete;

	bool Connect(const wchar_t* pConnectIP, unsigned short connectPort, const wchar_t* pSchema, const wchar_t* pUserID, const wchar_t* pUserPass)
	{	
		setConnectInfo(pConnectIP, connectPort, pSchema, pUserID, pUserPass);
		size_t size;	

		std::string connectIP(wcslen(pConnectIP) + 1, '\0');
		wcstombs_s(&size, &connectIP[0], connectIP.size(), pConnectIP, connectIP.size());

		std::string schema(wcslen(pSchema) + 1, '\0');
		wcstombs_s(&size, &schema[0], schema.size(), pSchema, schema.size());

		std::string userID(wcslen(pUserID) + 1,'\0');
		wcstombs_s(&size, &userID[0], userID.size(), pUserID, userID.size());

		std::string userPass(wcslen(pUserPass) + 1, '\0');
		wcstombs_s(&size, &userPass[0], userPass.size(), pUserPass, userPass.size());

		mpMySQL = mysql_real_connect(&mMySQL, connectIP.c_str(), userID.c_str(), userPass.c_str(), schema.c_str(), connectPort, nullptr, 0);
		if (mpMySQL == nullptr)
		{
			mLastErrorCode = mysql_errno(&mMySQL);

			setWideByteErrorMessage(mysql_error(&mMySQL));

			return false;
		}

		mbConnectStateFlag = true;

		return true;
	}

	bool Reconnect(void)
	{
		if (CheckReconnectErrorCode() == true)
		{
			if (mysql_ping(mpMySQL) != 0)
			{
				mLastErrorCode = mysql_errno(mpMySQL);

				setWideByteErrorMessage(mysql_error(mpMySQL));

				return false;
			}
		}
		else
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"[DBConnector]", L"[Reconnect] LastErrorCode : %d, Error Message : %s", mLastErrorCode, mLastErrorMessage);

			CCrashDump::Crash();
		}

		return true;
	}

	void Disconnect(void)
	{
		if (mpMySQL == nullptr)
			return;

		mysql_close(mpMySQL);

		mbConnectStateFlag = false;

		mpMySQL = nullptr;

		return;
	}


	// euckr 등의 문자셋을 지정
	bool MySQLCharacterSet(const wchar_t* pCharacterSet)
	{	
		std::string multiByteStr(wcslen(pCharacterSet) + 1, '\0');

		size_t size;
		wcstombs_s(&size, &multiByteStr[0], multiByteStr.size(), pCharacterSet, multiByteStr.size());

		if (mysql_set_character_set(mpMySQL, multiByteStr.c_str()) != 0)
		{
			mLastErrorCode = mysql_errno(mpMySQL);

			setWideByteErrorMessage(mysql_error(&mMySQL));

			return false;
		}

		return true;
	}


	bool Query(const wchar_t* pQueryFormat, ...)
	{
		// 가변인자 처리
		va_list va;
		va_start(va, pQueryFormat);
		HRESULT retval = StringCchVPrintfW(&mWideByteQuery[0], dbconnector::MAX_QUERY_LENGTH, pQueryFormat, va);
		if (FAILED(retval))
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"DBConnector", L"[Query] Error Code : %d, return : %d, format : %s, va : %s", GetLastError(), retval, pQueryFormat, va);

			CCrashDump::Crash();
		}

		size_t size;
		wcstombs_s(&size, &mMultiByteQuery[0], mMultiByteQuery.size(), mWideByteQuery.c_str(), mWideByteQuery.size());

		if (mysql_query(mpMySQL, mMultiByteQuery.c_str()) != 0)
		{		
			mLastErrorCode = mysql_errno(mpMySQL);

			setWideByteErrorMessage(mysql_error(&mMySQL));

			return false;
		}

		return true;
	}

	void StoreResult(void)
	{
		// 결과 전체를 미리 가져옴
		mpMySQLResult = mysql_store_result(mpMySQL);
		
		return;
	}
	
	MYSQL_ROW FetchRow(void)
	{
		// 행을 가져온다.
		return mysql_fetch_row(mpMySQLResult);
	}


	void FreeResult(void)
	{
		// 응답받은 데이터 정리
		mysql_free_result(mpMySQLResult);

		return;
	}

	bool GetConnectStateFlag(void) const
	{
		return mbConnectStateFlag;
	}

	unsigned int GetLastError(void) const
	{
		return mLastErrorCode;
	}


	const wchar_t* GetLastErrorMessage(void) const
	{
		return mLastErrorMessage.c_str();
	}


	bool CheckReconnectErrorCode(void) const
	{
		switch (mLastErrorCode)
		{
		case CR_SERVER_GONE_ERROR:

			break;
		case CR_SERVER_LOST:

			break;
		case CR_CONN_HOST_ERROR:

			break;
		case CR_SERVER_HANDSHAKE_ERR:

			break;
		default:
			return false;
		}

		return true;
	}

	void SetWideByteQuery(const wchar_t *pQuery)
	{
		mWideByteQuery = pQuery;
	}

	std::wstring GetWideByteQuery(void) const
	{
		return mWideByteQuery;
	}

private:

	void setConnectInfo(const wchar_t* pConnectIP, unsigned short connectPort, const wchar_t* pSchema, const wchar_t* pUserID, const wchar_t* pUserPassword)
	{
		mConnectIP = pConnectIP;

		mConnectPort = connectPort;

		mSchema = pSchema;

		mUserID = pUserID;

		mUserPassword = pUserPassword;

		return;
	}


	void setWideByteErrorMessage(const char* errorMessage)
	{
		size_t size;
		mbstowcs_s(&size, &mLastErrorMessage[0], mLastErrorMessage.size(), errorMessage, strlen(errorMessage));
		return;
	}
	

	MYSQL mMySQL;

	MYSQL* mpMySQL;

	MYSQL_RES* mpMySQLResult;

	bool mbConnectStateFlag;

	unsigned int mLastErrorCode;

	unsigned short mConnectPort;

	std::wstring mConnectIP;

	std::wstring mSchema;

	std::wstring mUserID;

	std::wstring mUserPassword;

	std::wstring mLastErrorMessage;

	std::wstring mWideByteQuery;

	std::string mMultiByteQuery;
};


