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
		// ������Ʈ�� ������
		DATA data;

		// �����÷ο츦 üũ�� üũ���Դϴ�.
		long long overCheckSum;

		// ~CFreeList() �Ҹ��ڿ��� �ش� ����Ʈ�� ��ȸ�ؼ� delete 
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

		// �ʱ� �� ������ŭ ��带 �����Ѵ�.
		for (int count = 0; count < nodeCount; ++count)
		{
			// ����� �����Ҵ�.
			// stNode* blockPtr = (stNode*)malloc(sizeof(stNode));
			stNode* blockPtr = (stNode*)HeapAlloc(mHeap, 0, sizeof(stNode));
			if (blockPtr == nullptr)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LockFreeObjectFreeList", L"[CLockFreeObjectFreeList] Error Code : %d", GetLastError());

				CCrashDump::Crash();

				return;
			}

			// PlacementNew�� True�� ��� �� Alloc �� ������ ȣ���
			if (bPlacementNewFlag == false)
				new(&blockPtr->data) DATA();

			blockPtr->overCheckSum = 0x1996122651582378;

			// ���� ������ ����� ���� ���� Alloc Top ��尡 �ȴ�.
			blockPtr->pNextNode = mTopNode.highPointerPart;

			// ���� ������ ��尡 Top ��尡 �ȴ�.
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

			// ������ �����͸� pTempNode�� ��´�.
			pTempNode = mTopNode.highPointerPart;

			if (mbPlacementNewFlag == false)
				pTempNode->data.~DATA();

			// pTempNode�� ���� ��� ���� top ���� �ٲ۴�.
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

				// Placement New�� False���� ���� �޸� Ȯ���ÿ��� ������ ȣ��Ǿ�� ��
				new(&tempInt128.highPointerPart->data) DATA();

				tempInt128.highPointerPart->overCheckSum = 0x1996122651582378;

				tempInt128.highPointerPart->pNextNode = nullptr;

				return &tempInt128.highPointerPart->data;
			}

		} while (0 == InterlockedCompareExchange128((long long*)&mTopNode, (long long)tempInt128.highPointerPart->pNextNode, tempInt128.lowCountPart + 1, (long long*)&tempInt128));


		// �÷��̽���Ʈ new �� TRUE�� ��� �����ڸ� ȣ���Ѵ�.
		if (mbPlacementNewFlag == true)
			new(&tempInt128.highPointerPart->data) DATA();

		return &tempInt128.highPointerPart->data;
	}



	// Push
	bool Free(DATA* data)
	{
		if (data == nullptr)
			return false;

		// char �����ͷ� ���� �� �ּ� �����Ͽ� ��忡 �����Ѵ�.
		stNode* newFreeTop = (stNode*)data;
		if (newFreeTop->overCheckSum != 0x1996122651582378)
			return false;

		// placement new�� TRUE�� ��� �Ҹ��ڸ� ȣ���ϰ�, ���� ����Ʈ�� �ֱ� ���� ���� ȣ�����ش�.
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
	
	// Alloc() �� ���� ������� ��� ����
	long GetAllocNodeCount() const
	{
		return mAllocNodeCount;
	}

private:

	// ~CFreeList() �Ҹ��ڿ��� �ش� ����Ʈ�� ��ȸ�ؼ� Relese 
	stInt128 mTopNode;

	HANDLE mHeap;

	// placement new�� ȣ������ ������ ����
	bool mbPlacementNewFlag;

	// Alloc() �� ���� ������� ����
	long mAllocNodeCount;
};