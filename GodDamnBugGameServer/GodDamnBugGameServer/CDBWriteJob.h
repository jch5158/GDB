#pragma once


class CPlayer;

class CGodDamnBug;

class CLoginDBWriteJob;

class CDBWriteJob
{
public:

	friend class CGodDamnBug;

	CDBWriteJob(void);

	virtual ~CDBWriteJob(void);

	virtual void DBWrite(void) = 0;

	void DecrementDBWriteCount(void);

	void* operator new(size_t size);

	void operator delete(void* ptr);

protected:

	CPlayer* mpPlayer;

	CTLSDBConnector* mpDBConnector;
	

public:

	inline static CLockFreeFreeListManager mFreeList;
};

