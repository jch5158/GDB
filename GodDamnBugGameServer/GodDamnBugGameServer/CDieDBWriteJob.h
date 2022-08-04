#pragma once

class CDieDBWriteJob : public CDBWriteJob
{
public:

	CDieDBWriteJob(void);

	~CDieDBWriteJob(void);

	virtual void DBWrite(void) final;

	void SetDieDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer);

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

};

