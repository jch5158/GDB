#pragma once
class CBaseObject
{
public:

	CBaseObject(void);

	~CBaseObject(void);

	UINT64 GetClientID(void) const;

	FLOAT GetPosX(void) const;

	FLOAT GetPosY(void) const;

	INT GetTileX(void) const;

	INT GetTileY(void) const;

	INT GetRotation(void) const;

	INT GetCurSectorX(void) const;

	INT GetCurSectorY(void) const;

	INT GetOldSectorX(void) const;

	INT GetOldSectorY(void) const;

	BOOL GetDeleteFlag(void) const;
	
	void SetDeleteFlag(BOOL bDeleteFlag);

	virtual void Update(void) = 0;

	virtual BOOL DeleteObject(void) = 0;

protected:

	UINT64 mClientID;

	FLOAT mPosX;
	FLOAT mPosY;

	INT mTileX;
	INT mTileY;

	INT mRotation;

	stSector mCurSector;
	stSector mOldSector;

	BOOL mbDeleteFlag;
};

