#pragma once

class CPlayer;

class CLogoutDBWriteJob : public CDBWriteJob
{
public:

	CLogoutDBWriteJob(void);

	~CLogoutDBWriteJob(void);

	virtual void DBWrite(void) final;
	
	//BYTE characterType, FLOAT posX, FLOAT posY, INT tileX, INT tileY, INT rotation, INT cristalCount, INT HP, UINT64 EXP, INT level, BOOL bDieFlag
	void SetLogoutDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer, CHAR status);


private:

	// 로그인 실패도 남겨야함
	CHAR mStatus;

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

