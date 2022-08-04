#pragma once

#include "LockFreeObjectFreeList/ObjectFreeList/CLockFreeObjectFreeList.h"

namespace tlslockfreefreelist
{
	constexpr INT CHUNK_DATA_LENGTH = 500;
}


template <class DATA>
class CTLSLockFreeFreeList
{
private:
	
	friend class CLockFreeFreeListManager;

	class CChunk;

	CTLSLockFreeFreeList(INT nodeCount)
		: mAllocCount(0)
		, mTlsIndex(-1)
		, mFreeList(nodeCount, false)
	{
		mTlsIndex = TlsAlloc();
		if (mTlsIndex == TLS_OUT_OF_INDEXES)
			CCrashDump::Crash();
	}

	~CTLSLockFreeFreeList()
	{
		if (TlsFree(mTlsIndex) == FALSE)
			CCrashDump::Crash();		
	}

	void* Alloc(void)
	{		
		CChunk* pChunk = (CChunk*)TlsGetValue(mTlsIndex);
		if (pChunk == nullptr)
		{
			pChunk = (CChunk*)mFreeList.Alloc();

			// 다른 청크가 올 수도 있으니 tlsindex로 체크한다.
			pChunk->SetTlsIndex(mTlsIndex);

			if (TlsSetValue(mTlsIndex, (PVOID)pChunk) == 0)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSLockFreeFreeList", L"[Alloc] TlsSetValue error, Error Code : %d", GetLastError());

				CCrashDump::Crash();
			}
		}

		DATA* pData = pChunk->GetChunkData();

		if (pChunk->IsChunkDataEmpty() == TRUE)
		{
			pChunk = (CChunk*)mFreeList.Alloc();

			// 다른 청크가 올 수도 있으니 tlsindex로 체크한다.
			pChunk->SetTlsIndex(mTlsIndex);

			if (TlsSetValue(mTlsIndex, (PVOID)pChunk) == 0)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSLockFreeFreeList", L"[Alloc] TlsSetValue error, Error Code : %d", GetLastError());

				CCrashDump::Crash();
			}
		}

		InterlockedIncrement(&mAllocCount);

		return pData;
	}

	BOOL Free(void* data)
	{
		CChunk* pChunk = ((typename CChunk::stChunkData*)data)->pChunk;

		// 다른 오브젝트 풀에서 할당받은 Chunk가 온건지 확인
		if (pChunk->GetTisIndex() != mTlsIndex)
			return FALSE;

		if (pChunk->IsChunkFree() == TRUE)
		{
			// 다음에 사용하기 위한 청크 리셋
			pChunk->ResetChunk();

			return mFreeList.Free(pChunk);
		}

		InterlockedDecrement(&mAllocCount);

		return TRUE;
	}

	LONG GetAllocCount(void) const
	{
		return mAllocCount;
	}

	class CChunk
	{

	public:

		CChunk(void)
			: mTlsIndex(0)
			, mAllocIndex(0)
			, mFreeCount(0)
		{
			for (INT index = 0; index < tlslockfreefreelist::CHUNK_DATA_LENGTH; ++index)
			{
				mChunkDataArray[index].pChunk = this;
			}
		}

		~CChunk(void){}

		struct stChunkData
		{
			DATA data;
			CChunk* pChunk;
		};

		BOOL IsChunkFree(void)
		{
			return InterlockedIncrement(&mFreeCount) == tlslockfreefreelist::CHUNK_DATA_LENGTH;
		}

		DATA* GetChunkData(void)
		{
			return &mChunkDataArray[mAllocIndex++].data;
		}

		void SetTlsIndex(DWORD tlsIndex)
		{
			mTlsIndex = tlsIndex;

			return;
		}

		DWORD GetTisIndex(void)
		{
			return mTlsIndex;
		}


		BOOL IsChunkDataEmpty(void)
		{
			return mAllocIndex == tlslockfreefreelist::CHUNK_DATA_LENGTH;
		}

		void ResetChunk(void)
		{
			mAllocIndex = 0;
			mFreeCount = 0;
		}


	private:

		DWORD mTlsIndex;

		LONG mAllocIndex;

		LONG mFreeCount;

		stChunkData mChunkDataArray[tlslockfreefreelist::CHUNK_DATA_LENGTH];
	};

	LONG mAllocCount;

	DWORD mTlsIndex;

	// CChunk에 대한 생성자는 호출되어야 함
	CLockFreeObjectFreeList<CChunk> mFreeList;
};

