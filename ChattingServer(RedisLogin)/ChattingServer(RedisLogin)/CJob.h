#pragma once


class CJob
{
private:

	CJob(void);

	~CJob(void) = default;

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;

public:

	enum class eJobType
	{
		CLIENT_JOIN_JOB,
		LOGIN_JOB,
		MESSAGE_JOB,
		CLIENT_LEAVE_JOB,
		SERVER_STOP_JOB
	};

	static CJob* Alloc(void);

	void Free(void);

	eJobType mJobType;

	UINT64 mSessionID;

	CMessage* mpMessage;

	BOOL mLoginResult;
	
	UINT64 mAccountNumber;
	
	WCHAR mPlayerID[player::PLAYER_STRING_LENGTH];
	
	WCHAR mPlayerNickName[player::PLAYER_STRING_LENGTH];

	CHAR mSessionKey[64];

private:

	static CTLSLockFreeObjectFreeList<CJob> mJobFreeList;
};
