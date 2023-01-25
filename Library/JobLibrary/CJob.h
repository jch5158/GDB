#pragma once

constexpr int MAX_JOB_STRING = 20;

class CJob
{
public:


	enum class eJobType
	{
		CLIENT_JOIN_JOB,
		LOGIN_JOB,
		MESSAGE_JOB,
		CLIENT_LEAVE_JOB,
		SERVER_STOP_JOB
	};

	template <class DATA>
	friend class CLockFreeObjectFreeList;

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;


	static inline CJob* Alloc(void)
	{	
		//CPerformanceProfiler performance(L"Job Alloc");

		CJob* pJob = mJobFreeList.Alloc();

		pJob->Clear();

		return pJob;
	}

	inline bool Free(void)
	{
		//CPerformanceProfiler performance(L"Job Free");

		if (mJobFreeList.Free(this) == false)
		{
			return false;
		}

		return true;
	}


	inline void Clear(void)
	{
		mJobType = eJobType::CLIENT_JOIN_JOB;
		mSessionID = -1;
		mpMessage = nullptr;
	}

	eJobType mJobType;
	ULONGLONG mSessionID;
	CMessage* mpMessage;
	BOOL mLoginResult;
	ULONGLONG mAccountNumber;
	WCHAR mPlayerID[MAX_JOB_STRING];
	WCHAR mPlayerNickName[MAX_JOB_STRING];

private:

	CJob(void)
		: mJobType(eJobType::CLIENT_JOIN_JOB)
		, mSessionID(-1)
		, mpMessage(nullptr)
		, mPlayerID{0,}
		, mPlayerNickName{0,}
	{
	}

	~CJob(void)
	{

	}

	//inline static CLockFreeObjectFreeList<CJob> mJobFreeList = { 100, false };

	inline static CTLSLockFreeObjectFreeList<CJob> mJobFreeList = { 100, false };

};

