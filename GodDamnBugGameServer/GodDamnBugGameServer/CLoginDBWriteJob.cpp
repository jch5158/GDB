#include "stdafx.h"


CLoginDBWriteJob::CLoginDBWriteJob(void)
	: mStatus(0)
	, mCharacterType(0)
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
	, mClientPort(0)
	, mClientIP{ 0, }
{
}

CLoginDBWriteJob::~CLoginDBWriteJob(void)
{
}

void CLoginDBWriteJob::DBWrite(void)
{
REQUERY:

	if (mpDBConnector->Query((WCHAR*)L"begin;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	if (mpDBConnector->Query((WCHAR*)L"UPDATE `accountdb`.`status` SET `status` = '%d' WHERE `accountno` = '%lld'", mStatus, mpPlayer->GetAccountNo()) == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s",mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}



	if (mCharacterType != 0)
	{
		// 캐릭터 정보를 DB 에 저장한다.
		//if (mpDBConnector->Query((WCHAR*)L"UPDATE `character` SET characterType = '%d', posX = '%lf', posY = '%lf', tileX = '%d', tileY = '%d', rotation = '%d', cristal = '%d', hp = '%d', exp = '%lld', level = '%d', die = '%d' WHERE accountno = '%lld'",
		//	mCharacterType, mPosX, mPosY, mTileX, mTileY, mRotation, mCristalCount, mHP, mEXP, mLevel, mbDieFlag, mAccountNo) == FALSE)
		//{
		//	if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		//	{
		//		mpDBConnector->Reconnect();

		//		goto REQUERY;
		//	}

		//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		//	CCrashDump::Crash();
		//}

		if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `logdb`.`gamelog` (`logtime`, `accountno`,`servername`, `type`, `code`, `param1`, `param2`, `param3`, `param4`, `message`) VALUES(NOW(), '%lld', 'Game', '1', '11', '%d', '%d', '%d', '%d', '%s:%d');",
			mpPlayer->GetAccountNo(), mTileX, mTileY, mCristalCount, mHP, mClientIP, mClientPort) == FALSE)
		{
			if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
			{
				mpDBConnector->Reconnect();

				goto REQUERY;
			}

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

			CCrashDump::Crash();
		}
	}
	else
	{
		if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `logdb`.`gamelog` (`logtime`, `accountno`,`servername`, `type`, `code`,`message`) VALUES(NOW(), '%lld', 'Game', '1', '11', '%s:%d');",
			mpPlayer->GetAccountNo(), mClientIP, mClientPort) == FALSE)
		{
			if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
			{
				mpDBConnector->Reconnect();

				goto REQUERY;
			}

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

			CCrashDump::Crash();
		}
	}

	if (mpDBConnector->Query((WCHAR*)L"commit;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LoginDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	return;
}


void CLoginDBWriteJob::SetLoginDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer, BYTE status)
{	
	mpPlayer = pPlayer;
	mpDBConnector = pDBConnector;
	
	mCharacterType = pPlayer->GetCharacterType();

	// mCharacterType이 0이라면 캐릭터 선택을 아직 하지 못하였음을 의미한다.
	if (mCharacterType != 0)
	{
		mStatus = status;
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
	}

	mClientPort = pPlayer->GetClientPort();
	pPlayer->GetClientIP(mClientIP, IP_LENGTH);

	return;
}

