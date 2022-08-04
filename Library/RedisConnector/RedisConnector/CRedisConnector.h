#pragma once

#include <cpp_redis/cpp_redis>

#include <strsafe.h>
#include <iostream>
#include <Windows.h>
#include <vector>
#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"

#pragma comment (lib,"Ws2_32.lib")
#pragma comment (lib,"cpp_redis.lib")
#pragma comment (lib,"tacopie.lib")

class CRedisConnector
{
public:

	CRedisConnector(void)
		:mbConnectFlag(false)
		, mConnectPort(6379)
		, mConnectIP(L"127.0.0.1")
		, mRedisClient()
	{}

	~CRedisConnector(void) = default;

	CRedisConnector(const CRedisConnector&) = delete;
	CRedisConnector operator=(const CRedisConnector&) = delete;

	static void CallWSAStartup(void)
	{
		WSAData wsa;

		if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"RedisLibrary", L"[SetWSAStartup] WSAStartup Error Code : %d", WSAGetLastError());

			CCrashDump::Crash();
		}

		return;
	}

	static void CallWSACleanup(void)
	{
		WSACleanup();

		return;
	}


	void Connect(void)
	{
		mRedisClient.connect();

		return;
	}

	void Disconnect(void)
	{
		if (mbConnectFlag == false)
			return;

		mRedisClient.disconnect();

		mbConnectFlag = false;

		return;
	}

	bool Connect(const wchar_t* pConnectIP, unsigned short connectPort)
	{
		if (pConnectIP == nullptr)
			return false;

		mConnectIP = pConnectIP;

		std::string multiByteConnectIP(mConnectIP.size(), '\0');

		size_t size;
		wcstombs_s(&size, &multiByteConnectIP[0], multiByteConnectIP.size(), mConnectIP.c_str(), mConnectIP.size());

		mConnectPort = connectPort;

		mRedisClient.connect(multiByteConnectIP, mConnectPort);

		mbConnectFlag = true;

		return true;
	}

	bool Set(long long key, const char* pValue)
	{
		if (pValue == nullptr)
			return false;

		try
		{
			mRedisClient.set(std::to_string(key), pValue);

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"Set", exp.what());

			return false;
		}

		return true;
	}


	bool SetEx(long long key, long long seconds, const char* pValue)
	{
		if (pValue == nullptr)
			return false;

		try
		{
			mRedisClient.setex(std::to_string(key), seconds, pValue);

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"SetEx", exp.what());

			return false;
		}

		return true;
	}



	bool Set(long long key, long long value)
	{
		try
		{
			mRedisClient.set(std::to_string(key), std::to_string(value));

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"Set", exp.what());

			return false;
		}

		return true;
	}


	bool SetEx(long long key, long long seconds, long long value)
	{
		try
		{
			mRedisClient.setex(std::to_string(key), seconds, std::to_string(value));

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"SetEx", exp.what());

			return false;
		}

		return true;
	}



	bool Set(const char* pKey, const char* pValue)
	{
		if (pKey == nullptr || pValue == nullptr)
			return false;

		try
		{
			mRedisClient.set(pKey, pValue);

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"Set", exp.what());

			return false;
		}

		return true;
	}


	bool SetEx(const char* pKey, long long seconds, const char* pValue)
	{
		if (pKey == nullptr || pValue == nullptr)
			return false;

		try
		{
			mRedisClient.setex(pKey, seconds, pValue);

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"SetEx", exp.what());

			return false;
		}

		return true;
	}


	bool Set(const char* pKey, long long value)
	{
		if (pKey == nullptr)
			return false;

		try
		{
			mRedisClient.set(pKey, std::to_string(value));

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"Set", exp.what());

			return false;
		}

		return true;
	}

	bool SetEx(const char* pKey, long long seconds, long long value)
	{
		if (pKey == nullptr)
			return false;

		try
		{
			mRedisClient.setex(pKey, seconds, std::to_string(value));

			mRedisClient.sync_commit();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"SetEx", exp.what());

			return false;
		}

		return true;
	}


	bool GetValue(long long key, long long* pValue)
	{
		if (pValue == nullptr)
			return false;

		try
		{
			auto value = mRedisClient.get(std::to_string(key));

			mRedisClient.sync_commit();

			*pValue = atoll(value.get().as_string().c_str());
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"GetValue", exp.what());

			return false;
		}

		return true;
	}

	bool GetValue(const char* pKey, long long* pValue)
	{
		if (pKey == nullptr || pValue == nullptr)
			return false;

		try
		{
			auto value = mRedisClient.get(pKey);

			mRedisClient.sync_commit();

			*pValue = atoll(value.get().as_string().c_str());
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"GetValue", exp.what());

			return false;
		}

		return true;
	}

	bool GetString(long long key, std::string& value)
	{
		try
		{
			auto retval = mRedisClient.get(std::to_string(key));

			mRedisClient.sync_commit();

			value = retval.get().as_string();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"GetString", exp.what());

			return false;
		}

		return true;
	}

	bool GetString(const char* pKey, std::string& value)
	{
		if (pKey == nullptr)
			return false;

		try
		{
			auto retval = mRedisClient.get(pKey);

			mRedisClient.sync_commit();

			value = retval.get().as_string();
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"GetString", exp.what());

			return false;
		}

		return true;
	}



	bool RemoveKey(long long key)
	{
		bool bRetvalFlag;

		mRemoveKeyVector.push_back(std::to_string(key));

		try
		{
			mRedisClient.del(mRemoveKeyVector);

			mRedisClient.sync_commit();

			bRetvalFlag = true;
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"RemoveKey", exp.what());

			bRetvalFlag = false;
		}

		mRemoveKeyVector.clear();

		return bRetvalFlag;
	}


	bool RemoveKey(const char* pKey)
	{
		if (pKey == nullptr)
			return false;

		bool bRetvalFlag;

		mRemoveKeyVector.push_back(pKey);

		try
		{
			mRedisClient.del(mRemoveKeyVector);

			mRedisClient.sync_commit();

			bRetvalFlag = true;
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"RemoveKey", exp.what());

			bRetvalFlag = false;
		}

		mRemoveKeyVector.clear();

		return bRetvalFlag;
	}


	bool CompareToken(const char* pKey, const char* pToken)
	{
		if (pKey == nullptr || pToken == nullptr)
			return false;

		try
		{
			auto retval = mRedisClient.get(pKey);

			mRedisClient.sync_commit();

			if (retval.get().as_string() == pToken)
				return true;
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"CompareToken", exp.what());

			return false;
		}
	}

	bool CompareToken(long long key, const char* pToken)
	{
		if (pToken == nullptr)
			return false;

		try
		{
			auto retval = mRedisClient.get(std::to_string(key));

			mRedisClient.sync_commit();

			if (retval.get().as_string() == pToken)
				return true;
		}
		catch (const cpp_redis::redis_error& exp)
		{
			logginError(L"CompareToken", exp.what());

			return false;
		}
	}

	bool GetConnectFlag(void) const
	{
		return mbConnectFlag;
	}

	const wchar_t* GetConnectIP(void) const
	{
		return mConnectIP.c_str();
	}

	unsigned short GetConnectPort(void) const
	{
		return mConnectPort;
	}

private:


	void logginError(const wchar_t* pFunction, const char* pErrorMsg) const
	{
		std::wstring errorMsg(strlen(pErrorMsg), L'\0');

		size_t size;
		mbstowcs_s(&size, &errorMsg[0], errorMsg.size(), pErrorMsg, strlen(pErrorMsg));

		CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"RedisLibrary", L"[%s] Error Message : %s", pFunction, errorMsg.c_str());

		return;
	}


	bool mbConnectFlag;

	std::wstring mConnectIP;

	unsigned short mConnectPort;

	// redis 라이브러리가 key 삭제 시 vector를 받음
	std::vector<std::string> mRemoveKeyVector;

	cpp_redis::client mRedisClient;
};

