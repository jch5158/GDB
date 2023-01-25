#pragma once

#include <iostream>
#include <Windows.h>
#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"

template <class DATA>
class CLockFreeObjectFreeList
{
private:

	struct stNode
	{
		// 오브젝트의 데이터
		DATA data;

		// 오버플로우를 체크할 체크섬입니다.
		long long overCheckSum;

		// ~CFreeList() 소멸자에서 해당 리스트를 순회해서 delete 
		stNode* pNextNode;
	};

	__declspec(align(16)) 
	struct stInt128
	{
		long long lowCountPart;
		stNode* highPointerPart;
	};

public:

	CLockFreeObjectFreeList()
		:mTopNode {0,}
		,mHeap(nullptr)
		,mbPlacementNewFlag(false)
		,mAllocNodeCount(0)
	{
		mHeap = HeapCreate(0, 0, 0);
	}

	explicit CLockFreeObjectFreeList(int nodeCount, bool bPlacementNewFlag)
		: mTopNode{ 0, }
		, mHeap(nullptr)
		, mbPlacementNewFlag(bPlacementNewFlag)
		, mAllocNodeCount(0)
	{
		mHeap = HeapCreate(0, 0, 0);

		// 초기 블럭 개수만큼 노드를 생성한다.
		for (int count = 0; count < nodeCount; ++count)
		{
			// 노드의 동적할당.
			// stNode* blockPtr = (stNode*)malloc(sizeof(stNode));
			stNode* blockPtr = (stNode*)HeapAlloc(mHeap, 0, sizeof(stNode));
			if (blockPtr == nullptr)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LockFreeObjectFreeList", L"[CLockFreeObjectFreeList] Error Code : %d", GetLastError());

				CCrashDump::Crash();

				return;
			}

			// PlacementNew가 True일 경우 매 Alloc 시 생성자 호출됨
			if (bPlacementNewFlag == false)
				new(&blockPtr->data) DATA();

			blockPtr->overCheckSum = 0x1996122651582378;

			// 새로 생성한 노드의 다음 노드는 Alloc Top 노드가 된다.
			blockPtr->pNextNode = mTopNode.highPointerPart;

			// 새로 생성한 노드가 Top 노드가 된다.
			mTopNode.highPointerPart = blockPtr;
		}
	}

	~CLockFreeObjectFreeList(void)
	{
		stNode* pTempNode;

		for (;;)
		{
			if (mTopNode.highPointerPart == nullptr)
				break;

			// 해제할 포인터를 pTempNode에 담는다.
			pTempNode = mTopNode.highPointerPart;

			if (mbPlacementNewFlag == false)
				pTempNode->data.~DATA();

			// pTempNode에 다음 노드 값을 top 노드로 바꾼다.
			mTopNode.highPointerPart = pTempNode->pNextNode;

			//free(pTempNode);
			HeapFree(mHeap, 0, pTempNode);
		}

		HeapDestroy(mHeap);
	}

	CLockFreeObjectFreeList(const CLockFreeObjectFreeList&) = delete;
	CLockFreeObjectFreeList& operator=(const CLockFreeObjectFreeList&) = delete;

	// Pop
	DATA* Alloc(void)
	{
		InterlockedIncrement(&mAllocNodeCount);

		stInt128 tempInt128;

		do
		{
			tempInt128.lowCountPart = mTopNode.lowCountPart;
			tempInt128.highPointerPart = mTopNode.highPointerPart;

			if (tempInt128.highPointerPart == nullptr)
			{
				// tempInt128.highPointerPart = (stNode*)malloc(sizeof(stNode));
				tempInt128.highPointerPart = (stNode*)HeapAlloc(mHeap, 0, sizeof(stNode));
				if (tempInt128.highPointerPart == nullptr)
				{
					CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LockFreeObjectFreeList", L"[Alloc] malloc return nullptr Error Code : %d", GetLastError());

					CCrashDump::Crash();
				}

				// Placement New가 False여도 최초 메모리 확보시에는 생성자 호출되어야 함
				new(&tempInt128.highPointerPart->data) DATA();

				tempInt128.highPointerPart->overCheckSum = 0x1996122651582378;

				tempInt128.highPointerPart->pNextNode = nullptr;

				return &tempInt128.highPointerPart->data;
			}

		} while (0 == InterlockedCompareExchange128((long long*)&mTopNode, (long long)tempInt128.highPointerPart->pNextNode, tempInt128.lowCountPart + 1, (long long*)&tempInt128));


		// 플레이스먼트 new 가 TRUE일 경우 생성자를 호출한다.
		if (mbPlacementNewFlag == true)
			new(&tempInt128.highPointerPart->data) DATA();

		return &tempInt128.highPointerPart->data;
	}



	// Push
	bool Free(DATA* data)
	{
		if (data == nullptr)
			return false;

		// char 포인터로 받은 후 주소 연산하여 노드에 대입한다.
		stNode* newFreeTop = (stNode*)data;
		if (newFreeTop->overCheckSum != 0x1996122651582378)
			return false;

		// placement new가 TRUE일 경우 소멸자를 호출하고, 프리 리스트에 넣기 전에 먼저 호출해준다.
		if (mbPlacementNewFlag == true)
			data->~DATA();

		stNode* pTempFreeNode;

		do
		{
			pTempFreeNode = mTopNode.highPointerPart;

			newFreeTop->pNextNode = pTempFreeNode;

		} while (pTempFreeNode != (stNode*)InterlockedCompareExchange64((long long*)&mTopNode.highPointerPart, (long long)newFreeTop, (long long)pTempFreeNode));

		InterlockedDecrement(&mAllocNodeCount);

		return true;
	}
	
	// Alloc() 을 통해 사용중인 노드 개수
	long GetAllocNodeCount() const
	{
		return mAllocNodeCount;
	}

private:

	// ~CFreeList() 소멸자에서 해당 리스트를 순회해서 Relese 
	stInt128 mTopNode;

	HANDLE mHeap;

	// placement new를 호출할지 말지를 셋팅
	bool mbPlacementNewFlag;

	// Alloc() 을 통해 사용중인 개수
	long mAllocNodeCount;
};