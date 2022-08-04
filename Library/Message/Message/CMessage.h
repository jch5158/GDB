#pragma once

#include <iostream>
#include <Windows.h>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"

namespace message
{
	constexpr int MAX_BUFFER_SIZE = 500;

	class CExceptionObject
	{
	public:

		explicit CExceptionObject(char* buffer, int bufferSize, std::wstring &errorStr)
			:mpMemoryLog(new char[bufferSize])
			,mBufferSize(bufferSize)
			,mErrorStr(errorStr)
		{
			memcpy(mpMemoryLog, buffer, bufferSize);
		}

		CExceptionObject(const CExceptionObject& rhs)
			:mpMemoryLog(new char[rhs.mBufferSize])
			,mBufferSize(rhs.mBufferSize)
			,mErrorStr(rhs.mErrorStr)
		{
			memcpy(mpMemoryLog, rhs.mpMemoryLog, mBufferSize);
		}

		CExceptionObject& operator=(const CExceptionObject& rhs)
		{
			if (this == &rhs)
				return *this;

			delete[] mpMemoryLog;

			mpMemoryLog = new char[rhs.mBufferSize];
			mBufferSize = rhs.mBufferSize;
			mErrorStr = rhs.mErrorStr;

			memcpy(mpMemoryLog, rhs.mpMemoryLog, mBufferSize);
		}

		~CExceptionObject()
		{
			delete[] mpMemoryLog;
		}

		CExceptionObject() = delete;

		void PrintExceptionData() const
		{
			CSystemLog::GetInstance()->LogHex(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ExceptionObject", mErrorStr.c_str(), (unsigned char*)mpMemoryLog, mBufferSize);

			return;
		}

	private:

		char* mpMemoryLog;

		int mBufferSize;

		std::wstring mErrorStr;
	};
}

class CMessage
{
private:

	#pragma pack(1)
	struct stLanHeader
	{
		// 메시지 사이즈
		unsigned short payloadSize;
	};

	struct stNetHeader
	{
		char code;
		unsigned short payloadSize;
		char randomKey;
		char checkSum;
	};
	#pragma pack()

public:

	// 헤더 셋팅을 위한 friend
	friend class CSession;

	friend class CPlayer;

	friend class CLanClient;

	friend class CLanServer;

	friend class CNetClient;

	friend class CNetServer;

	friend class CMMOServer;

	// CObjectFreeList에서 생성자 및 소멸자를 위한 friend
	template <class DATA>
	friend class CLockFreeObjectFreeList;

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;

	void Release()
	{
		free(mpBufferPtr);
	}

	void Clear()
	{
		mbEndcodingFlag = false;
		mFront = -1;
		mRear = -1;
		mDataSize = 0;
		mpHeaderPtr = mpBufferPtr + sizeof(stNetHeader);
	}

	int GetBufferSize() const
	{
		return mBufferLen;
	}

	int GetUseSize() const
	{
		return mDataSize;
	}

	char* GetMessagePtr() const
	{
		return mpHeaderPtr;
	}

	char* GetPayloadPtr() const
	{
		return &mpPayloadPtr[mFront + 1];
	}

	int MoveWritePos(int size)
	{
		if (mDataSize + size > message::MAX_BUFFER_SIZE)
			size = message::MAX_BUFFER_SIZE - mDataSize;

		mDataSize += size;

		mRear += size;

		return size;
	}

	int MoveReadPos(int size)
	{
		if (mDataSize < size)
			size = mDataSize;

		mDataSize -= size;

		mFront += size;

		return size;
	}

	int GetPayload(char* chpDest, int size)
	{
		if (mDataSize < size)
			size = mDataSize;

		memcpy(chpDest, &mpPayloadPtr[mFront + 1], size);

		return size;
	}

	int PutPayload(char* chpSrc, int size)
	{
		if (mDataSize + size > message::MAX_BUFFER_SIZE)
			size = message::MAX_BUFFER_SIZE - mDataSize;

		memcpy(&mpPayloadPtr[mRear + 1], chpSrc, size);

		return size;
	}

	void AddReferenceCount()
	{
		InterlockedIncrement(&mReferenceCount);
	}

	long SubReferenceCount()
	{
		return InterlockedDecrement(&mReferenceCount);
	}

	template<class T>
	CMessage& operator << (const T value)
	{
		if (mDataSize + sizeof(T) > message::MAX_BUFFER_SIZE)
		{
			std::string str = typeid(T).name();

			std::wstring wstr(str.begin(), str.end());

			wstr += L" <<";

			message::CExceptionObject exception(mpPayloadPtr, mDataSize, wstr);

			throw exception;
		}

		*((T*)&mpPayloadPtr[mRear + 1]) = value;

		mRear += sizeof(T);

		mDataSize += sizeof(T);

		return *this;
	}


	template<class T>
	CMessage& operator >> (T& value)
	{
		if (mDataSize < sizeof(T))
		{
			std::string str = typeid(T).name();

			std::wstring wstr(str.begin(), str.end());

			wstr += L" >>";

			message::CExceptionObject exception(mpPayloadPtr, mDataSize, wstr);

			throw exception;
		}

		value = *((T*)&mpPayloadPtr[mFront + 1]);

		mFront += sizeof(T);

		mDataSize -= sizeof(T);

		return *this;
	}
	
	static CMessage* Alloc()
	{
		CMessage* pMessage = mMessageFreeList.Alloc();
			
		pMessage->Clear();

		pMessage->AddReferenceCount();

		return pMessage;
	}

	void Free()	
	{	
		int referenceCount = SubReferenceCount();

		if (referenceCount == 0)
		{
			if (mMessageFreeList.Free(this) == FALSE)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CMessage", L"[Free] Message Free Error");

				CCrashDump::Crash();
			}
		}
		else if (referenceCount < 0)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CMessage", L"[Free] referenceCount Error : %d\n", referenceCount);

			CCrashDump::Crash();
		}

		return;
	}

	inline static int GetAllocCount()
	{
		return mMessageFreeList.GetAllocCount();
	}

private:

	CMessage()
		: mbEndcodingFlag(false)
		, mFront(-1)
		, mRear(-1)
		, mDataSize(0)
		, mBufferLen(message::MAX_BUFFER_SIZE)
		, mReferenceCount(0)
		, mpBufferPtr(nullptr)
		, mpHeaderPtr(nullptr)
		, mpPayloadPtr(nullptr)
	{				
		init();
	}

	explicit CMessage(int size)
		: mbEndcodingFlag(false)
		, mFront(-1)
		, mRear(-1)
		, mDataSize(0)
		, mBufferLen(size)
		, mReferenceCount(0)
		, mpBufferPtr(nullptr)
		, mpHeaderPtr(nullptr)
		, mpPayloadPtr(nullptr)
	{
		init();
	}

	~CMessage(void)
	{
		Release();
	}

	CMessage(const CMessage&) = delete;
	CMessage& operator=(const CMessage&) = delete;

	void init()
	{
		mpBufferPtr = (char*)malloc(mBufferLen);

		mpHeaderPtr = mpBufferPtr + sizeof(stNetHeader);

		mpPayloadPtr = mpHeaderPtr;
	}

	void encode(void)
	{
		if (mbEndcodingFlag == true)
			return;

		stNetHeader netHeader;

		netHeader.code = mHeaderCode;
		netHeader.payloadSize = mDataSize;
		netHeader.randomKey = rand();
		netHeader.checkSum = makeCheckSum();

		setNetHeader(&netHeader);

		char encodeKey = 0;
		char encodeData = 0;

		// 체크썸까지 encode
		char* pEncodeData = (char*)(mpPayloadPtr - 1);

		// 인코딩 대상은 체크섬 + 페이로드로서 sizeof(stNetHeader) - 1 을 한다.
		int length = mDataSize - sizeof(stNetHeader) + 1;
		for (int index = 0; index < length; ++index)
		{
			// encodeKey의 초기값은 0이고 해당 값을 randomKey와 같이 xor 연산
			encodeKey = pEncodeData[index] ^ (encodeKey + netHeader.randomKey + index + 1);

			// encodeData의 초기값은 0이고 해당 값을 staticKey와 같이 xor 연산
			encodeData = encodeKey ^ (encodeData + mStaticKey + index + 1);

			pEncodeData[index] = encodeData;
		}

		mbEndcodingFlag = true;

		return;
	}


	bool decode()
	{
		char randomKey =  ((stNetHeader*)GetMessagePtr())->randomKey;
		char decodeKey = 0;
		char encodeKey = 0;
		char decodeData = 0;
		char encodeData = 0;
		char* pDecodeData = (char*)(mpPayloadPtr - 1);

		int length = mDataSize - sizeof(stNetHeader) + 1;	
		for (int index = 0; index < length; ++index)
		{
			char temp = pDecodeData[index];

			decodeKey = pDecodeData[index] ^ (encodeKey + randomKey + index + 1);
			decodeData = decodeKey ^ (encodeData + mStaticKey + index + 1);

			pDecodeData[index] = decodeData;

			encodeKey = decodeData ^ (encodeKey + randomKey + index + 1);
			encodeData = temp;
		}
		
		removeNetHeader();

		return pDecodeData[0] == makeCheckSum();
	}


	bool decode(stNetHeader* pNetHeader)
	{
		char randomKey = pNetHeader->randomKey;
		char decodeKey = 0;
		char encodeKey = 0;
		char decodeData = 0;
		char encodeData = 0;

		decodeKey = pNetHeader->checkSum ^ (encodeKey + randomKey + 1);
		decodeData = decodeKey ^ (encodeData + mStaticKey + 1);

		pNetHeader->checkSum = decodeData;

		encodeKey = decodeData ^ (encodeKey + randomKey + 1);
		encodeData = encodeKey ^ (encodeData + mStaticKey + 1);

		int length = mDataSize;
		for (int index = 0; index < length; ++index)
		{
			decodeKey = mpPayloadPtr[index] ^ (encodeKey + randomKey + index + 2);
			decodeData = decodeKey ^ (encodeData + mStaticKey + index + 2);

			mpPayloadPtr[index] = decodeData;

			encodeKey = decodeData ^ (encodeKey + randomKey + index + 2);
			encodeData = encodeKey ^ (encodeData + mStaticKey + index + 2);
		}

		return pNetHeader->checkSum == makeCheckSum();
	}

	char makeCheckSum(void)
	{
		int checkSum = 0;
		
		for (int offset = 0; offset < mDataSize; ++offset)
			checkSum += *(mpPayloadPtr + offset);
		
		return checkSum & 0x000000ff;
	}

	void setLanHeader(stLanHeader* pLanHeader)
	{			
		mpHeaderPtr -= sizeof(stLanHeader);

		*((stLanHeader*)mpHeaderPtr) = *pLanHeader;

		mDataSize += sizeof(stLanHeader);
	}

	void setNetHeader(stNetHeader* pNetHeader)
	{
		mpHeaderPtr -= sizeof(stNetHeader);

		*((stNetHeader*)mpHeaderPtr) = *pNetHeader;

		mDataSize += sizeof(stNetHeader);
	}	

	void removeLanHeader()
	{
		mpHeaderPtr += sizeof(stLanHeader);

		mDataSize -= sizeof(stLanHeader);
	}

	void removeNetHeader()
	{
		mpHeaderPtr += sizeof(stNetHeader);

		mDataSize -= sizeof(stNetHeader);
	}

	inline static CTLSLockFreeObjectFreeList<CMessage> mMessageFreeList;	
	inline static char mHeaderCode = 0;
	inline static char mStaticKey = 0;


	bool mbEndcodingFlag;	
	int mFront;
	int mRear;
	int mBufferLen;
	int mDataSize;
	long mReferenceCount;

	// 헤더 이후 데이터 부터 가리킴
	char* mpPayloadPtr;

	// 메시지의 헤더를 가리키는 포인터입니다.
	char* mpHeaderPtr;

	// 동적할당 받은 버퍼 가리키는 포인터
	char* mpBufferPtr;
};

