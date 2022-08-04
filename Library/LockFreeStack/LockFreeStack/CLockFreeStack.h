#pragma once


#include <iostream>
#include <Windows.h>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"

/*
struct stLog
{
	DWORD threadID;
	DWORD logTag;	
	long long count;
	long long tempCount;
	void* ptr;
	void* tempPtr;
};


void SaveMemoryLog(DWORD logTag, long long count, long long tempCount, void *ptr = nullptr, void* tempPtr = nullptr)
{
	static stLog gMemoryLog[1000];

	static unsigned int index = -1;

	int nowIdx = InterlockedIncrement(&index) % 1000;

	gMemoryLog[nowIdx].threadID = GetCurrentThreadId();
	gMemoryLog[nowIdx].logTag = logTag;
	gMemoryLog[nowIdx].count = count;
	gMemoryLog[nowIdx].tempCount = tempCount;
	gMemoryLog[nowIdx].ptr = ptr;
	gMemoryLog[nowIdx].tempPtr = tempPtr;

	return;
}
*/

template <class DATA>
class CLockFreeStack
{
private:

	struct stNode
	{
		DATA mData;
		stNode* mpNextNode;
	};

	// IntercloedExchange128은 16Byte 경계에 무조건 서야함
	__declspec(align(16)) 
	struct stInt128
	{
		long long countLowPart;
		stNode* pointerHighPart;
	};


public:

	CLockFreeStack()
		: mCount(0)
		, mTopInt128{ 0, }
		, mpNodeFreeList(nullptr)
	{
		static CTLSLockFreeObjectFreeList<stNode> nodeFreeList;

		mpNodeFreeList = &nodeFreeList;
	}

	~CLockFreeStack() = default;

	CLockFreeStack(const CLockFreeStack&) = delete;
	CLockFreeStack& operator=(const CLockFreeStack&) = delete;

	void Push(DATA data)
	{
		stNode* newNode = mpNodeFreeList->Alloc();

		newNode->mData = data;

		stNode* pTempTopNode;

		do
		{
			pTempTopNode = mTopInt128.pointerHighPart;

			newNode->mpNextNode = pTempTopNode;

		} while (pTempTopNode != (stNode*)InterlockedCompareExchange64((long long*)&mTopInt128.pointerHighPart, (long long)newNode, (long long)pTempTopNode));

		InterlockedIncrement(&mCount);

		return;
	}

	// 개선 POP
	bool Pop(DATA* data)
	{
		if (data == nullptr)
			return false;

		if (InterlockedDecrement(&mCount) < 0)
		{
			InterlockedIncrement(&mCount);

			return false;
		}

		stInt128 compareInt128;

		do
		{		
			compareInt128.countLowPart = mTopInt128.countLowPart;

			compareInt128.pointerHighPart = mTopInt128.pointerHighPart;

		} while (true != (bool)InterlockedCompareExchange128((long long*)&mTopInt128, (long long)compareInt128.pointerHighPart->mpNextNode, compareInt128.countLowPart + 1, (long long*)&compareInt128));

		*data = compareInt128.pointerHighPart->mData;

		if (mpNodeFreeList->Free(compareInt128.pointerHighPart) == false)
			CCrashDump::Crash();

		return true;
	}

	long Size() const
	{
		return mCount;
	}


private:

	stInt128 mTopInt128;

	long mCount;

	CTLSLockFreeObjectFreeList<stNode> *mpNodeFreeList;
};
