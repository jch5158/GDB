#pragma once


class CJob
{
public:

	enum class eJobType
	{
		CLIENT_JOIN_JOB,
		MESSAGE_JOB,
		CLIENT_LEAVE_JOB,
		SERVER_STOP_JOB
	};

	template <class DATA>
	friend class CLockFreeObjectFreeList;

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;


	inline static CJob* Alloc(void)
	{
		//CPerformanceProfiler performance(L"Job Alloc");

		CJob* pJob = mJobFreeList.Alloc();

		//pJob->Clear();

		return pJob;
	}

	void Free(void)
	{
		//CPerformanceProfiler performance(L"Job Free");

		if (mJobFreeList.Free(this) == FALSE)
		{
			CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"[ChattingServer]", L"[Free] Job Free Error");

			CCrashDump::Crash();

			return;
		}

		return;
	}


	inline void Clear(void)
	{
		mJobType = eJobType::CLIENT_JOIN_JOB;
		mSessionID = -1;
		mpMessage = nullptr;
	}

	eJobType mJobType;

	UINT64 mSessionID;

	CMessage* mpMessage;

	BOOL mLoginResult;

	UINT64 mAccountNumber;

	WCHAR mPlayerID[player::PLAYER_STRING_LENGTH];

	WCHAR mPlayerNickName[player::PLAYER_STRING_LENGTH];

	CHAR mSessionKey[64];

private:

	CJob(void)
		: mJobType(eJobType::CLIENT_JOIN_JOB)
		, mSessionID(-1)
		, mpMessage(nullptr)
		, mPlayerID{ 0, }
		, mPlayerNickName{ 0, }
		, mSessionKey{ 0, }
	{
	}

	~CJob(void)
	{

	}

	//inline static CLockFreeObjectFreeList<CJob> mJobFreeList = { 100, FALSE };

	inline static CTLSLockFreeObjectFreeList<CJob> mJobFreeList;
};

