#include "stdafx.h"


CCristal::CCristal(void)
	: mCristalType(0)
	, mAmount(0)
	, mCreateTime(0)
{}

CCristal::~CCristal(void){}


void CCristal::SetCristalTypeAmount(INT typeAmountArray[])
{	
	for (INT count = (INT)eCristalType::Cristal1; count <= (INT)eCristalType::Cristal3; ++count)
	{
		mCristalAmountMap.insert(std::make_pair(count, typeAmountArray[count - 1]));
	}

	return;
}


CCristal* CCristal::Alloc(UINT64 clientID, FLOAT posX, FLOAT posY, INT tileX, INT tileY, stSector curSector, CGodDamnBug *pGodDamnBug)
{
	CCristal* pCristal = mCristalFreeList.Alloc();

	pCristal->mClientID = clientID;

	pCristal->mCristalType = gatchaCristalType();

	pCristal->setCristalAmount();

	pCristal->mCreateTime = timeGetTime();

	pCristal->mPosX = posX;
	pCristal->mPosY = posY;

	pCristal->mTileX = tileX;
	pCristal->mTileY = tileY;

	pCristal->mCurSector.posX = curSector.posX;
	pCristal->mCurSector.posY = curSector.posY;

	pCristal->mbDeleteFlag = FALSE;

	pCristal->mpGodDamnBug = pGodDamnBug;
		
	return pCristal;
}


void CCristal::Free(void)
{
	if (mCristalFreeList.Free(this) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[Free] Cristal Free is Failed");

		CCrashDump::Crash();
	}

	return;
}

INT CCristal::GetCristalAmount(void) const
{	
	return mAmount;
}


BYTE CCristal::GetCristalType(void) const
{
	return mCristalType;
}

void CCristal::Update(void)
{
	checkCristalDeleteTime();

	return;
}

BOOL CCristal::DeleteObject(void)
{
	if (mbDeleteFlag == TRUE)
	{
		mpGodDamnBug->SendAroundRemoveCristal(this);

		mpGodDamnBug->RemoveCristal(this);

		Free();

		return TRUE;
	}

	return FALSE;
}

void CCristal::checkCristalDeleteTime(void)
{
	if (mbDeleteFlag == TRUE)
	{
		return;
	}

	if (timeGetTime() - mCreateTime > CRISTAL_TIME)
	{
		mbDeleteFlag = TRUE;
	}

	return;
}

BYTE CCristal::gatchaCristalType(void)
{
	DWORD randValue = rand() % 10;

	if (randValue < 6)
	{
		return (BYTE)eCristalType::Cristal1;
	}
	else if (randValue >= 6 && randValue < 9)
	{
		return (BYTE)eCristalType::Cristal2;
	}
	else
	{
		return (BYTE)eCristalType::Cristal3;
	}
}


void CCristal::setCristalAmount(void)
{
	auto iter = mCristalAmountMap.find(mCristalType);
	if (iter == mCristalAmountMap.end())
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[setCristalAmount] mCristalType : %d", mCristalType);

		CCrashDump::Crash();
	}

	mAmount = iter->second;

	return;
}