#pragma once

#include <iostream>
#include <Windows.h>
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"

class CLockFreeFreeListManager
{
public:

	CLockFreeFreeListManager(void)
		: m512Byte()
		, m1024Byte()
		, m1536Byte()
		, m2048Byte()
		, m2560Byte()
		, m3072Byte()
		, m3584Byte()
		, m4096Byte()
	{}

	~CLockFreeFreeListManager(void) = default;

	CLockFreeFreeListManager(const CLockFreeFreeListManager&) = delete;
	CLockFreeFreeListManager& operator=(const CLockFreeFreeListManager&) = delete;

	void* Alloc(int size)
	{
		long long* ptr;

		switch ((CLockFreeFreeListManager::eBucket)((size + 7) / 512))
		{
		case CLockFreeFreeListManager::eBucket::Bucket512:

			ptr = (long long*)m512Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket512;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket1024:

			ptr = (long long*)m1024Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket1024;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket1536:

			ptr = (long long*)m1536Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket1536;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket2048:

			ptr = (long long*)m2048Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket2048;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket2560:

			ptr = (long long*)m2560Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket2560;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket3072:

			ptr = (long long*)m3072Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket3072;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket3584:

			ptr = (long long*)m3584Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket3584;

			return (void*)(ptr + 1);

		case CLockFreeFreeListManager::eBucket::Bucket4096:

			ptr = (long long*)m4096Byte.Alloc();

			*ptr = (long long)CLockFreeFreeListManager::eBucket::Bucket4096;

			return (void*)(ptr + 1);

		default:

			ptr = (long long*)malloc(size);
			if (ptr == nullptr)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Alloc] malloc error, Error Code : %d", GetLastError());

				CCrashDump::Crash();

				break;
			}

			*ptr = (long long)CLockFreeFreeListManager::eBucket::BucketMax;

			return (void*)(ptr + 1);
		}
	}

	void Free(void* ptr)
	{
		long long* bucket = (long long*)ptr - 1;

		switch ((CLockFreeFreeListManager::eBucket)(*bucket))
		{
		case CLockFreeFreeListManager::eBucket::Bucket512:

			if (m512Byte.Free((BYTE(*)[512])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket512 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::Bucket1024:

			if (m1024Byte.Free((BYTE(*)[1024])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket1024 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::Bucket1536:

			if (m1536Byte.Free((BYTE(*)[1536])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket1536 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::Bucket2048:

			if (m2048Byte.Free((BYTE(*)[2048])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket2048 Free is Failed");

				CCrashDump::Crash();
			}

			return;

		case CLockFreeFreeListManager::eBucket::Bucket2560:

			if (m2560Byte.Free((BYTE(*)[2560])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket2560 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::Bucket3072:

			if (m3072Byte.Free((BYTE(*)[3072])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket3072 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::Bucket3584:

			if (m3584Byte.Free((BYTE(*)[3584])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket3584 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::Bucket4096:

			if (m4096Byte.Free((BYTE(*)[4096])bucket) == FALSE)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket4096 Free is Failed");

				CCrashDump::Crash();
			}

			return;
		case CLockFreeFreeListManager::eBucket::BucketMax:

			free(bucket);

			return;

		default:

			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LockFreeFreeListManager", L"[Free] Bucket Number is wrong : %lld", (CLockFreeFreeListManager::eBucket)(*bucket));

			CCrashDump::Crash();

			break;
		}

		return;
	}

	int Get512ByteAllocCount(void) const
	{
		return m512Byte.GetAllocCount();
	}

	int Get1024ByteAllocCount(void) const
	{
		return m1024Byte.GetAllocCount();
	}

	int Get1536ByteAllocCount(void) const
	{
		return m1536Byte.GetAllocCount();
	}

	int Get2048ByteAllocCount(void) const
	{
		return m2048Byte.GetAllocCount();
	}
	
	int Get2560ByteAllocCount(void) const
	{
		return m2560Byte.GetAllocCount();
	}

	int Get3072ByteAllocCount(void) const
	{
		return m3072Byte.GetAllocCount();
	}

	int Get3584ByteAllocCount(void) const
	{
		return m3584Byte.GetAllocCount();
	}
	
	int Get4096ByteAllocCount(void) const
	{
		return m4096Byte.GetAllocCount();
	}

private:

	enum class eBucket : long long
	{
		Bucket512,
		Bucket1024,
		Bucket1536,
		Bucket2048,
		Bucket2560,
		Bucket3072,
		Bucket3584,
		Bucket4096,
		BucketMax
	};

	CTLSLockFreeObjectFreeList<BYTE[512]> m512Byte;

	CTLSLockFreeObjectFreeList<BYTE[1024]> m1024Byte;

	CTLSLockFreeObjectFreeList<BYTE[1536]> m1536Byte;

	CTLSLockFreeObjectFreeList<BYTE[2048]> m2048Byte;

	CTLSLockFreeObjectFreeList<BYTE[2560]> m2560Byte;

	CTLSLockFreeObjectFreeList<BYTE[3072]> m3072Byte;

	CTLSLockFreeObjectFreeList<BYTE[3584]> m3584Byte;

	CTLSLockFreeObjectFreeList<BYTE[4096]> m4096Byte;
};




