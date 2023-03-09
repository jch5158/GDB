#pragma once

#include <iostream>
#include <Windows.h>
#include "CrashDump/CrashDump/CCrashDump.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"



template <class DATA>
class CLockFreeQueue
{
private:

	struct stNode
	{
		DATA mData;

		stNode* mpNextNode;
	};

	// Interlocked 쓰는 변수들끼리 같은 캐시라인에 존재하면 경합이 발생된다. 
	// Head,Tail 2개의 멤버변수에 대해서 Interlocked 를 사용하기에 align64를 사용해서 서로 다른 캐시라인에 있도록 하여
	// Interlocked 끼리의 경합을 최대한 회피한다.
	__declspec(align(64)) 
	struct stInt128
	{
		long long countLowPart;
		stNode* pointerHighPart;
	};


	struct stLogData
	{
		DWORD threadID;

		DWORD logTag;

		void* head;
		long long headCount;

		void* tempHead;
		long long tempHeadCount;

		void* tail;
		long long tailCount;

		void* tempTail;
		long long tempTailCount;
	};

	inline static stLogData arr[1000];

	inline static int index = -1;

	void SaveMLog(DWORD logTag, void* head, long long headCount, void* tail, long long tailCount, void* tempHead, long long tempHeadCount, void* tempTail, long long tempTailCount)
	{	
		int idx = InterlockedIncrement((long*)&index) % 1000;

		arr[idx].threadID = GetCurrentThreadId();

		arr[idx].logTag = logTag;

		arr[idx].head = head;
		arr[idx].headCount = headCount;

		arr[idx].tail = tail;
		arr[idx].tailCount = tailCount;

		arr[idx].tempHead = tempHead;
		arr[idx].tempHeadCount = tempHeadCount;

		arr[idx].tempTail = tempTail;
		arr[idx].tempTailCount = tempTailCount;

		return;
	}


public:

	CLockFreeQueue(void)
		: mHead{ 0, }
		, mTail{ 0, }
		, mQueueUseSize(0)
		, mpNodeFreeList(nullptr)
	{
		// 전역 객체의 생성자 호출 순서는 보장될 수 없기 떄문에 전역 멤버객체로 들으면 안 된다.
		static CTLSLockFreeObjectFreeList<stNode> nodeFreeList;

		mpNodeFreeList = &nodeFreeList;

		stNode* dummyNode = mpNodeFreeList->Alloc();

		*dummyNode = { 0, };

		mHead.pointerHighPart = dummyNode;

		mTail.pointerHighPart = dummyNode;
	}

	~CLockFreeQueue(void)
	{
		if (mpNodeFreeList->Free(mHead.pointerHighPart) == false)
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CLockFreeQueue",L"[~CLockFreeQueue] Free is Failed");
	}

	CLockFreeQueue(const CLockFreeQueue&) = delete;
	CLockFreeQueue& operator=(const CLockFreeQueue&) = delete;

	void Enqueue(const DATA data)
	{
		stNode* pNewNode = mpNodeFreeList->Alloc();

		pNewNode->mpNextNode = nullptr;
		pNewNode->mData = data;

		stInt128 tailTemp;

		for (;;)
		{
			tailTemp.countLowPart = mTail.countLowPart;
			tailTemp.pointerHighPart = mTail.pointerHighPart;

			// 테일의 Next가 nullptr이 아닐 경우 테일을 밀어주는 작업을 한다.
			if (tailTemp.pointerHighPart->mpNextNode == nullptr) 
			{ 
				if ((long long)nullptr == InterlockedCompareExchange64((long long*)&tailTemp.pointerHighPart->mpNextNode, (long long)pNewNode, (long long)nullptr))
				{
					// 이 코드에서 밀지 못하는 건 어쩔 수 없음 다음 Enqueue, Dequeue 가 밀어줄 상황이다. 
					InterlockedCompareExchange128((long long*)&mTail, (long long)pNewNode, tailTemp.countLowPart + 1, (long long*)&tailTemp);

					break;
				}
			}
			else 
			{
				InterlockedCompareExchange128((long long*)&mTail, (long long)tailTemp.pointerHighPart->mpNextNode, tailTemp.countLowPart + 1, (long long*)&tailTemp);
			}
		}

		InterlockedIncrement(&mQueueUseSize);

		return;
	}

	// 수정
	bool Dequeue(DATA* data)
	{
		if (InterlockedDecrement(&mQueueUseSize) < 0)
		{
			InterlockedIncrement(&mQueueUseSize);

			return false;
		}

		stInt128 headTemp;
		stInt128 tailTemp;

		for (;;)
		{
			headTemp.countLowPart = mHead.countLowPart;
			headTemp.pointerHighPart = mHead.pointerHighPart;

			// 필수적으로 미리 pDequeueNode에 보관해야 함
			// 큐의 변화로 지역에 보관한 head가 다른 스레드에서는 신규 노드로 나와 head의 next가 nullptr 로 초기화될 수 있음
			stNode* pDequeueNode = headTemp.pointerHighPart->mpNextNode;
			if (pDequeueNode == nullptr)
				continue;

			tailTemp.countLowPart = mTail.countLowPart;
			tailTemp.pointerHighPart = mTail.pointerHighPart;

			// 테일의 next가 nullptr이 아니고. head와 Tail이 같은 노드를 가리키면 밀어준다.
			if (headTemp.pointerHighPart == tailTemp.pointerHighPart && tailTemp.pointerHighPart->mpNextNode != nullptr)
			{
				InterlockedCompareExchange128((long long*)&mTail, (long long)tailTemp.pointerHighPart->mpNextNode, tailTemp.countLowPart + 1, (long long*)&tailTemp);

				continue;
			}

			// InterlockedExchange 이후에는 다른 스레드에서 head 에 대해서 Dequeue 할 수 있기 때문에 Interlocked128 전에 data를 저장한다.
			*data = pDequeueNode->mData;

			if (0 != InterlockedCompareExchange128((long long*)&mHead, (long long)pDequeueNode, headTemp.countLowPart + 1, (long long*)&headTemp))
				break;
		}

		if (mpNodeFreeList->Free(headTemp.pointerHighPart) == false)
			CCrashDump::Crash();

		return true;
	}

	long GetUseSize(void) const
	{
		return mQueueUseSize;
	}

private:

	stInt128 mHead;

	stInt128 mTail;

	long mQueueUseSize;
	
	CTLSLockFreeObjectFreeList<stNode> *mpNodeFreeList;
};