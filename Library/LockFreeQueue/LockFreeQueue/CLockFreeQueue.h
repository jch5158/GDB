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

	// Interlocked ���� �����鳢�� ���� ĳ�ö��ο� �����ϸ� ������ �߻��ȴ�. 
	// Head,Tail 2���� ��������� ���ؼ� Interlocked �� ����ϱ⿡ align64�� ����ؼ� ���� �ٸ� ĳ�ö��ο� �ֵ��� �Ͽ�
	// Interlocked ������ ������ �ִ��� ȸ���Ѵ�.
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
		// ���� ��ü�� ������ ȣ�� ������ ����� �� ���� ������ ���� �����ü�� ������ �� �ȴ�.
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

			// ������ Next�� nullptr�� �ƴ� ��� ������ �о��ִ� �۾��� �Ѵ�.
			if (tailTemp.pointerHighPart->mpNextNode == nullptr) 
			{ 
				if ((long long)nullptr == InterlockedCompareExchange64((long long*)&tailTemp.pointerHighPart->mpNextNode, (long long)pNewNode, (long long)nullptr))
				{
					// �� �ڵ忡�� ���� ���ϴ� �� ��¿ �� ���� ���� Enqueue, Dequeue �� �о��� ��Ȳ�̴�. 
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

	// ����
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

			// �ʼ������� �̸� pDequeueNode�� �����ؾ� ��
			// ť�� ��ȭ�� ������ ������ head�� �ٸ� �����忡���� �ű� ���� ���� head�� next�� nullptr �� �ʱ�ȭ�� �� ����
			stNode* pDequeueNode = headTemp.pointerHighPart->mpNextNode;
			if (pDequeueNode == nullptr)
				continue;

			tailTemp.countLowPart = mTail.countLowPart;
			tailTemp.pointerHighPart = mTail.pointerHighPart;

			// ������ next�� nullptr�� �ƴϰ�. head�� Tail�� ���� ��带 ����Ű�� �о��ش�.
			if (headTemp.pointerHighPart == tailTemp.pointerHighPart && tailTemp.pointerHighPart->mpNextNode != nullptr)
			{
				InterlockedCompareExchange128((long long*)&mTail, (long long)tailTemp.pointerHighPart->mpNextNode, tailTemp.countLowPart + 1, (long long*)&tailTemp);

				continue;
			}

			// InterlockedExchange ���Ŀ��� �ٸ� �����忡�� head �� ���ؼ� Dequeue �� �� �ֱ� ������ Interlocked128 ���� data�� �����Ѵ�.
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