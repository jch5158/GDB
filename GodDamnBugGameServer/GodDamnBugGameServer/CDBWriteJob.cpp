#include "stdafx.h"


CDBWriteJob::CDBWriteJob(void)
	: mpPlayer(nullptr)
	, mpDBConnector(nullptr)
{
}

CDBWriteJob::~CDBWriteJob(void)
{
}

void CDBWriteJob::DecrementDBWriteCount(void)
{
	mpPlayer->DecrementDBWriteCount();

	return;
}


// class IDBWriteJob

void* CDBWriteJob::operator new(size_t size)
{
	return mFreeList.Alloc(size);
}


void CDBWriteJob::operator delete(void* ptr)
{
	mFreeList.Free(ptr);

	return;
}


