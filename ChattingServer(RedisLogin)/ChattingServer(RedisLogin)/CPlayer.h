#pragma once

namespace player
{
	constexpr int PLAYER_STRING_LENGTH = 20;
}

class CPlayer
{
public:

	template <class DATA>
	friend class CLockFreeObjectFreeList;

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;


	static inline CPlayer* Alloc(void)
	{
		CPlayer* pPlayer = mPlayerFreeList.Alloc();

		pPlayer->Clear();

		return pPlayer;
	}

	void Free(void)
	{
		if (mPlayerFreeList.Free(this) == false)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CChattingServer", L"[Free] Player Free is Failed");
	
			CCrashDump::Crash();
		}

		return;
	}

	void Clear(void)
	{
		mSectorX = -1;
		mSectorY = -1;
		mbSectorSetFlag = FALSE;
		mSessionID = -1;
		mAccountNumber = -1;

		return;
	}

	// DWORD로 마음대로 바꿀 수 없음
	LONG mSectorY;

	LONG mSectorX;

	BOOL mbSectorSetFlag;

	UINT64 mSessionID;

	UINT64 mAccountNumber;

	WCHAR mPlayerID[player::PLAYER_STRING_LENGTH];

	WCHAR mPlayerNickName[player::PLAYER_STRING_LENGTH];

private:

	CPlayer(void)
		: mbSectorSetFlag(FALSE)
		, mSectorY(-1)
		, mSectorX(-1)
		, mAccountNumber(-1)
		, mPlayerID{ 0, }
		, mPlayerNickName{ 0, }
	{
	}

	~CPlayer(void)
	{
	}


	inline static CTLSLockFreeObjectFreeList<CPlayer> mPlayerFreeList;
};
