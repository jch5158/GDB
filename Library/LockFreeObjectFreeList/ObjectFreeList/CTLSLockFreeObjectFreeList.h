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


		// ��ȯ�� �� stChunkData �ڿ� PChunk�� ���� �����͸� ���� ��ȯ�� Chunk�� ���� �����Ѵ�.
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

		// Chunk�� ��ȯ�� �� �� ��������� 0���� �ʱ�ȭ�Ѵ�.
		void ResetChunk(void)
		{
			mAllocIndex = 0;
			mFreeCount = 0;
		}

		int mTlsIndex;

		// Alloc index�� �̿��ؼ� Chunk �����͸� �󸶳� ����ߴ����� Ȯ���� �� �ִ�.
		int mAllocIndex;

		// Free index�� �̿��ؼ� Alloc�� �����Ͱ� �󸶳� ��ȯ�Ǿ����� Ȯ���� �� �ִ�. 
		long mFreeCount;

		// Alloc �� ������ �迭
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

		// �ܺο��� ���ø� �̳� Ŭ���� �տ��� typename ��� �ʿ�
		CChunk* pChunk = ((typename CChunk::stChunkData*)data)->pChunk;

		// �ٸ� ������Ʈ Ǯ���� �Ҵ���� Chunk�� �°��� Ȯ��
		if (pChunk->GetTisIndex() != mTlsIndex)
			return false;

		if (pChunk->IsChunkFree() == true)
		{
			// ������ ����ϱ� ���� ûũ ����
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



