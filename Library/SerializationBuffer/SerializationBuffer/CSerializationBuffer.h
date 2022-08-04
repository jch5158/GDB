#pragma once

#include <iostream>
#include <Windows.h>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"

namespace serializationbuffer
{
	constexpr INT MAX_BUFFER_SIZE = 2000;
}


class CSerializationBuffer
{
public:

	CSerializationBuffer(void)
		: mFront(-1)
		, mRear(-1)
		, mDataSize(0)
		, mBufferLen(serializationbuffer::MAX_BUFFER_SIZE)
		, mReferenceCount(0)
		, mpBufferPtr(nullptr)
	{
		mpBufferPtr = (CHAR*)malloc(mBufferLen);
	}

	CSerializationBuffer(DWORD size)
		: mFront(-1)
		, mRear(-1)
		, mDataSize(0)
		, mBufferLen(size)
		, mReferenceCount(0)
		, mpBufferPtr(nullptr)
	{
		mpBufferPtr = (CHAR*)malloc(mBufferLen);
	}


	~CSerializationBuffer(void)
	{
		Release();
	}


	class CExceptionObject
	{
	public:

		CExceptionObject(CHAR* buffer, DWORD bufferSize, const WCHAR* data)
		{
			mpSerializationBufferLog = buffer;

			mBufferSize = bufferSize;

			wcscpy_s(mErrorTextLog, _countof(mErrorTextLog), data);
		}

		CExceptionObject(const CExceptionObject& exceptionObject)
		{
			mBufferSize = exceptionObject.mBufferSize;

			mpSerializationBufferLog = exceptionObject.mpSerializationBufferLog;

			wcscpy_s(mErrorTextLog, _countof(mErrorTextLog), exceptionObject.mErrorTextLog);
		}

		~CExceptionObject()
		{

		}

		void PrintExceptionData(void)
		{
			CSystemLog::GetInstance()->LogHex(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ExceptionObject", (const WCHAR*)mErrorTextLog, (BYTE*)mpSerializationBufferLog, mBufferSize);
		}

	private:

		DWORD mBufferSize;

		CHAR* mpSerializationBufferLog;

		WCHAR mErrorTextLog[200];
	};


	inline void Release(void)
	{
		free(mpBufferPtr);

		return;
	}

	inline void Clear(void)
	{
		mFront = -1;
		mRear = -1;
		mDataSize = 0;

		return;
	}

	inline DWORD GetBufferSize(void) const
	{
		return mBufferLen;
	}

	inline DWORD GetUseSize(void) const
	{
		return mDataSize;
	}


	inline CHAR* GetBufferPtr(void) const
	{
		return mpBufferPtr;
	}

	inline INT MoveFront(INT iSize)
	{
		mDataSize += iSize;

		mRear += iSize;

		return iSize;
	}

	inline INT MoveRear(INT iSize)
	{
		mDataSize -= iSize;

		mFront += iSize;

		return iSize;
	}

	inline DWORD GetBufferData(CHAR* chpDest, DWORD iSize)
	{
		memcpy(chpDest, &mpBufferPtr[mFront + 1], iSize);

		return iSize;
	}

	inline DWORD PutBufferData(CHAR* chpSrc, DWORD iSize)
	{
		memcpy(&mpBufferPtr[mRear + 1], chpSrc, iSize);

		return iSize;
	}

	inline void AddReferenceCount(void)
	{
		InterlockedIncrement(&mReferenceCount);
	}

	inline LONG SubReferenceCount(void)
	{
		return InterlockedDecrement(&mReferenceCount);
	}

	inline CSerializationBuffer& operator << (bool value)
	{
		mpBufferPtr[mRear + 1] = value;

		mRear += sizeof(bool);

		mDataSize += sizeof(bool);

		return *this;
	}


	inline CSerializationBuffer& operator << (CHAR value)
	{
		mpBufferPtr[mRear + 1] = value;

		mRear += sizeof(CHAR);

		mDataSize += sizeof(CHAR);

		return *this;
	}

	inline CSerializationBuffer& operator << (BYTE value)
	{
		mpBufferPtr[mRear + 1] = (BYTE)value;

		mRear += sizeof(BYTE);

		mDataSize += sizeof(BYTE);

		return *this;
	}


	inline CSerializationBuffer& operator << (SHORT value)
	{
		*((SHORT*)&mpBufferPtr[mRear + 1]) = value;

		mRear += sizeof(SHORT);

		mDataSize += sizeof(SHORT);

		return *this;
	}

	inline CSerializationBuffer& operator << (WORD value)
	{
		*((WORD*)&mpBufferPtr[mRear + 1]) = value;

		mRear += sizeof(WORD);

		mDataSize += sizeof(WORD);

		return *this;
	}

	inline CSerializationBuffer& operator << (INT value)
	{
		*((INT*)&mpBufferPtr[mRear + 1]) = value;

		mRear += sizeof(INT);

		mDataSize += sizeof(INT);

		return *this;
	}

	inline CSerializationBuffer& operator << (DWORD value)
	{
		*((DWORD*)&mpBufferPtr[mRear + 1]) = value;

		mRear += sizeof(DWORD);

		mDataSize += sizeof(DWORD);

		return *this;
	}


	inline CSerializationBuffer& operator << (INT64 value)
	{
		*((INT64*)&mpBufferPtr[mRear + 1]) = value;

		mRear += sizeof(INT64);

		mDataSize += sizeof(INT64);

		return *this;
	}

	inline CSerializationBuffer& operator << (UINT64 value)
	{
		*((UINT64*)&mpBufferPtr[mRear + 1]) = value;

		mRear += sizeof(UINT64);

		mDataSize += sizeof(UINT64);

		return *this;
	}

	inline CSerializationBuffer& operator >> (bool& value)
	{
		if (mDataSize < sizeof(bool))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" bool");

			throw exception;
		}

		value = *((bool*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(bool);

		mDataSize -= sizeof(bool);

		return *this;
	}


	inline CSerializationBuffer& operator >> (CHAR& value)
	{
		if (mDataSize < sizeof(CHAR))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" CHAR");

			throw exception;
		}

		value = *((CHAR*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(CHAR);

		mDataSize -= sizeof(CHAR);

		return *this;
	}

	inline CSerializationBuffer& operator >> (BYTE& value)
	{
		if (mDataSize < sizeof(BYTE))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" BYTE");

			throw exception;
		}

		value = *((BYTE*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(BYTE);

		mDataSize -= sizeof(BYTE);

		return *this;
	}

	inline CSerializationBuffer& operator >> (SHORT& value)
	{
		if (mDataSize < sizeof(SHORT))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" SHORT");

			throw exception;
		}

		value = *((SHORT*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(SHORT);

		mDataSize -= sizeof(SHORT);

		return *this;
	}

	inline CSerializationBuffer& operator >> (WORD& value)
	{
		if (mDataSize < sizeof(WORD))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" WORD\n");

			throw exception;
		}

		value = *((WORD*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(WORD);

		mDataSize -= sizeof(WORD);

		return *this;
	}

	inline CSerializationBuffer& operator >> (INT& value)
	{
		if (mDataSize < sizeof(INT))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" INT");

			throw exception;
		}

		value = *((INT*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(INT);

		mDataSize -= sizeof(INT);

		return *this;
	}

	inline CSerializationBuffer& operator >> (DWORD& value)
	{
		if (mDataSize < sizeof(DWORD))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" DWORD");

			throw exception;
		}

		value = *((DWORD*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(DWORD);

		mDataSize -= sizeof(DWORD);

		return *this;
	}


	inline CSerializationBuffer& operator >> (INT64& value)
	{
		if (mDataSize < sizeof(INT64))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" INT64");

			throw exception;
		}

		value = *((INT64*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(INT64);

		mDataSize -= sizeof(INT64);

		return *this;
	}

	inline CSerializationBuffer& operator >> (UINT64& value)
	{
		if (mDataSize < sizeof(UINT64))
		{
			CExceptionObject exception(mpBufferPtr, mRear + 1, L" UINT64");

			throw exception;
		}

		value = *((UINT64*)&mpBufferPtr[mFront + 1]);

		mFront += sizeof(UINT64);

		mDataSize -= sizeof(UINT64);

		return *this;
	}


	static inline CSerializationBuffer* Alloc(void)
	{
		CSerializationBuffer* pSerializationBuffer = mMessageFreeList.Alloc();

		pSerializationBuffer->Clear();

		pSerializationBuffer->AddReferenceCount();

		return pSerializationBuffer;
	}


	inline BOOL Free(void)
	{
		if (SubReferenceCount() == 0)
		{
			if (mMessageFreeList.Free(this) == FALSE)
			{
				return FALSE;
			}
		}

		return TRUE;
	}


private:

	inline static CTLSLockFreeObjectFreeList<CSerializationBuffer> mMessageFreeList;

	INT mFront;

	INT mRear;

	// 동적할당 받은 버퍼 가리키는 포인터
	CHAR* mpBufferPtr;

	DWORD mDataSize;

	LONG mReferenceCount;

	DWORD mBufferLen;
};

