#pragma once

class CRedisJob
{
public:

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;

	inline static CRedisJob* Alloc(void)
	{
		return mTlsJobFreeList.Alloc();
	}

	inline bool Free(void)
	{
		if (mTlsJobFreeList.Free(this) == false)
		{
			return false;
		}

		return true;
	}

	UINT64 mSessionID;

	CMessage* mpMessage;

private:

	CRedisJob(void)
		: mSessionID(-1)
		, mpMessage(nullptr)
	{

	}

	~CRedisJob(void)
	{

	}

	inline static CTLSLockFreeObjectFreeList<CRedisJob> mTlsJobFreeList = { 100, false };
};

