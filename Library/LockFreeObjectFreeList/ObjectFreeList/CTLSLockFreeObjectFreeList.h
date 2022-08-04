#pragma once

#include "CLockFreeObjectFreeList.h"


namespace tlslockfreeObjfreelist
{
	constexpr int CHUNK_DATA_LENGTH = 500;
}

template <class DATA>
class CTLSLockFreeObjectFreeList
{
public:

	class CChunk
	{
	private:

		template<class DATA>
		friend class CLockFreeObjectFreeList;

		template<class DATA>
		friend class CTLSLockFreeObjectFreeList;
	
		CChunk()
			: mTlsIndex(-1)
			, mAllocIndex(0)
			, mFreeCount(0)
		{
			for (int index = 0; index < tlslockfreeObjfreelist::CHUNK_DATA_LENGTH; ++index)
				mChunkDataArray[index].pChunk = this;
		}

		~CChunk() = default;

		CChunk(const CChunk&) = delete;
		CChunk& operator=(const CChunk&) = delete;


		// 반환할 때 stChunkData 뒤에 PChunk에 대한 포인터를 통해 반환할 Chunk에 직접 접근한다.
		struct stChunkData
		{
			stChunkData()
				:data()
				, pChunk(nullptr)
			{}

			~stChunkData() = default;

			stChunkData(const stChunkData&) = delete;
			stChunkData& operator=(const stChunkData&) = delete;

			DATA data;
			CChunk* pChunk;
		};

		bool IsChunkFree()
		{
			return InterlockedIncrement(&mFreeCount) == tlslockfreeObjfreelist::CHUNK_DATA_LENGTH;
		}

		DATA* GetChunkData()
		{
			return &mChunkDataArray[mAllocIndex++].data;
		}

		void SetTlsIndex(int tlsIndex)
		{
			mTlsIndex = tlsIndex;

			return;
		}

		int GetTisIndex() const
		{
			return mTlsIndex;
		}

		bool IsChunkDataEmpty() const
		{
			return mAllocIndex == tlslockfreeObjfreelist::CHUNK_DATA_LENGTH;
		}

		// Chunk를 반환할 때 각 멤버변수를 0으로 초기화한다.
		void ResetChunk(void)
		{
			mAllocIndex = 0;
			mFreeCount = 0;
		}

		int mTlsIndex;

		// Alloc index를 이용해서 Chunk 데이터를 얼마나 사용했는지를 확인할 수 있다.
		int mAllocIndex;

		// Free index를 이용해서 Alloc된 데이터가 얼마나 반환되었는지 확인할 수 있다. 
		long mFreeCount;

		// Alloc 할 데이터 배열
		stChunkData mChunkDataArray[tlslockfreeObjfreelist::CHUNK_DATA_LENGTH];
	};

	CTLSLockFreeObjectFreeList()
		:mTlsIndex(-1)
		,mObjectFreeList(0,false)
	{
		init();
	}

	explicit CTLSLockFreeObjectFreeList(int nodeCount, bool bPlacementNewFlag)
		: mTlsIndex(-1)
		, mObjectFreeList(nodeCount, bPlacementNewFlag)
	{	
		init();
	}

	~CTLSLockFreeObjectFreeList()
	{
		release();
	}

	CTLSLockFreeObjectFreeList(const CTLSLockFreeObjectFreeList&) = delete;
	CTLSLockFreeObjectFreeList& operator=(const CTLSLockFreeObjectFreeList&) = delete;

	DATA* Alloc()
	{
		CChunk* pChunk = (CChunk*)TlsGetValue(mTlsIndex);
		if (pChunk == nullptr)
			pChunk = chunkAlloc();

		DATA* pData = pChunk->GetChunkData();

		if (pChunk->IsChunkDataEmpty() == TRUE)
			chunkAlloc();

		return pData;
	}

	bool Free(DATA* data)
	{
		if (data == nullptr)
			return false;

		// 외부에서 템플릿 이너 클래스 앞에는 typename 사용 필요
		CChunk* pChunk = ((typename CChunk::stChunkData*)data)->pChunk;

		// 다른 오브젝트 풀에서 할당받은 Chunk가 온건지 확인
		if (pChunk->GetTisIndex() != mTlsIndex)
			return false;

		if (pChunk->IsChunkFree() == true)
		{
			// 다음에 사용하기 위한 청크 리셋
			pChunk->ResetChunk();

			return mObjectFreeList.Free(pChunk);	
		}

		return true;
	}

	int GetAllocCount(void) const
	{
		return mObjectFreeList.GetAllocNodeCount();
	}


private:

	void init()
	{
		mTlsIndex = TlsAlloc();
		if (mTlsIndex == TLS_OUT_OF_INDEXES)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSLockFreeFreeList", L"[CTLSLockFreeObjectFreeList] TlsAlloc Error Code : %d", GetLastError());

			CCrashDump::Crash();
		}

		return;
	}

	void release()
	{
		if (TlsFree(mTlsIndex) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSLockFreeFreeList", L"[~CTLSLockFreeObjectFreeList] TlsFree Error Code : %d", GetLastError());

			CCrashDump::Crash();
		}

		mTlsIndex = -1;

		return;
	}

	CChunk* chunkAlloc()
	{
		CChunk* pChunk = mObjectFreeList.Alloc();

		pChunk->SetTlsIndex(mTlsIndex);

		if (TlsSetValue(mTlsIndex, (void*)pChunk) == 0)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"TLSLockFreeFreeList", L"[Alloc] TlsSetValue Error Code : %d", GetLastError());

			CCrashDump::Crash();
		}

		return pChunk;
	}

	int mTlsIndex;

	CLockFreeObjectFreeList<CChunk> mObjectFreeList;
};



