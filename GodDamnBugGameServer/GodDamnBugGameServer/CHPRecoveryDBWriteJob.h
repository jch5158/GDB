#pragma once

class CHPRecoveryDBWriteJob : public CDBWriteJob
{
public:

	CHPRecoveryDBWriteJob(void);

	~CHPRecoveryDBWriteJob(void);

	virtual void DBWrite(void) final;

	void SetHPRecoveryDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer);


private:

	BYTE mCharacterType;

	FLOAT mPosX;

	FLOAT mPosY;

	INT mTileX;

	INT mTileY;

	INT mRotation;

	INT mCristalCount;

	INT mHP;

	UINT64 mEXP;

	INT mLevel;

	BOOL mbDieFlag;

	INT mOldHP;

	DWORD mSitTimeSec;
};

