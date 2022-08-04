#include "stdafx.h"


CBaseObject::CBaseObject(void)
	: mClientID(0)
	, mPosX(0.0f)
	, mPosY(0.0f)
	, mTileX(0)
	, mTileY(0)
	, mRotation(0)
	, mCurSector{ 0, }
	, mOldSector{ 0, }
	, mbDeleteFlag(FALSE)
{

}

CBaseObject::~CBaseObject(void)
{

}

UINT64 CBaseObject::GetClientID(void) const
{
	return mClientID;
}

FLOAT CBaseObject::GetPosX(void) const
{
	return mPosX;
}

FLOAT CBaseObject::GetPosY(void) const
{
	return mPosY;
}

INT CBaseObject::GetTileX(void) const
{
	return mTileX;
}

INT CBaseObject::GetTileY(void) const
{
	return mTileY;
}

INT CBaseObject::GetRotation(void) const
{
	return mRotation;
}

INT CBaseObject::GetCurSectorX(void) const
{
	return mCurSector.posX;
}

INT CBaseObject::GetCurSectorY(void) const
{
	return mCurSector.posY;
}

INT CBaseObject::GetOldSectorX(void) const
{
	return mOldSector.posX;
}

INT CBaseObject::GetOldSectorY(void) const
{
	return mOldSector.posY;
}

BOOL CBaseObject::GetDeleteFlag(void) const
{
	return mbDeleteFlag;
}

void CBaseObject::SetDeleteFlag(BOOL bDeleteFlag)
{
	mbDeleteFlag = bDeleteFlag;

	return;
}