#include "stdafx.h"


CCristalPickDBWriteJob::CCristalPickDBWriteJob(void)
	: mCharacterType(0)
	, mPosX(0.0f)
	, mPosY(0.0f)
	, mTileX(0)
	, mTileY(0)
	, mRotation(0)
	, mCristalCount(0)
	, mHP(0)
	, mEXP(0)
	, mLevel(0)
	, mbDieFlag(FALSE)
	, mGetCristalCount(0)
{
}

CCristalPickDBWriteJob::~CCristalPickDBWriteJob(void)
{
}

void CCristalPickDBWriteJob::DBWrite(void)
{
REQUERY:

	if (mpDBConnector->Query((WCHAR*)L"begin;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[RestartDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}
	

	// 캐릭터 DB 저장
	if (mpDBConnector->Query((WCHAR*)L"UPDATE `character` SET characterType = '%d', posX = '%lf', posY = '%lf', tileX = '%d', tileY = '%d', rotation = '%d', cristal = '%d', hp = '%d', exp = '%lld', level = '%d', die = '%d' WHERE accountno = '%lld'",
		mCharacterType, mPosX, mPosY, mTileX, mTileY, mRotation, mCristalCount, mHP, mEXP, mLevel, mbDieFlag, mpPlayer->GetAccountNo()) == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}



	// 게임 로그
	if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `logdb`.`gamelog` (`logtime`, `accountno`,`servername`, `type`, `code`, `param1`, `param2`) VALUES(NOW(), '%lld', 'Game', '4', '41', '%d', '%d');",
		mpPlayer->GetAccountNo(), mGetCristalCount, mCristalCount) == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}


	if (mpDBConnector->Query((WCHAR*)L"commit;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[RestartDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}
	
	return;
}

void CCristalPickDBWriteJob::SetCristalPickDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer, INT getCristalCount)
{
	mpPlayer = pPlayer;
	mpDBConnector = pDBConnector;	
	
	mCharacterType = pPlayer->GetCharacterType();
	mPosX = pPlayer->GetPosX();
	mPosY = pPlayer->GetPosY();
	mTileX = pPlayer->GetTileX();
	mTileY = pPlayer->GetTileY();
	mRotation = pPlayer->GetRotation();
	mCristalCount = pPlayer->GetCristalCount();
	mHP = pPlayer->GetHP();
	mEXP = pPlayer->GetEXP();
	mLevel = pPlayer->GetLevel();
	mbDieFlag = pPlayer->GetDieFlag();

	mGetCristalCount = getCristalCount;

	return;
}


