#include "stdafx.h"

CCreateCharacterDBWriteJob::CCreateCharacterDBWriteJob(void)
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
{
}

CCreateCharacterDBWriteJob::~CCreateCharacterDBWriteJob(void)
{

	
}

void CCreateCharacterDBWriteJob::DBWrite(void)
{
REQUERY:

	if (mpDBConnector->Query((WCHAR*)L"begin;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[CreateDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}


	if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `character` (`accountno`,`charactertype`, `posx`, `posy`, `tilex`, `tiley`, `rotation`, `cristal`, `hp`, `exp`, `level`, `die`) VALUES ('%lld','%d', '%lf', '%lf', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d');",
		mpPlayer->GetAccountNo(), mCharacterType, mPosX, mPosY, mTileX, mTileY, mRotation, mCristalCount, mHP, mEXP, mLevel, mbDieFlag) == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[CreateDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	// 게임 로그
	if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `logdb`.`gamelog` (`logtime`, `accountno`,`servername`, `type`, `code`, `param1`) VALUES(NOW(), '%lld', 'Game', '3', '32', '%d');",
		mpPlayer->GetAccountNo(), mCharacterType) == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[DieDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	if (mpDBConnector->Query((WCHAR*)L"commit;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[CreateDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	
	return;
}

void CCreateCharacterDBWriteJob::SetCreateCharacterDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer)
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

	return;
}

