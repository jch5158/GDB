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
		//CPerformanceProfiler performance(L"Player Alloc");

		CPlayer* pPlayer = mPlayerFreeList.Alloc();

		pPlayer->Clear();

		return pPlayer;
	}

	BOOL Free(void)
	{
		//CPerformanceProfiler performance(L"Player Free");

		if (mPlayerFreeList.Free(this) == FALSE)
		{
			return FALSE;
		}

		return TRUE;
	}

	void Clear(void)
	{
		mSectorX = -1;
		mSectorY = -1;
		mbSectorSetFlag = FALSE;
		mSessionID = -1;
		mAccountNumber = -1;
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

	//inline static CLockFreeObjectFreeList<CPlayer> mPlayerFreeList = { 100, FALSE };

	inline static CTLSLockFreeObjectFreeList<CPlayer> mPlayerFreeList;
};
