#include "stdafx.h"


CLogoutDBWriteJob::CLogoutDBWriteJob(void)
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
{

}

CLogoutDBWriteJob::~CLogoutDBWriteJob(void)
{

}

void CLogoutDBWriteJob::DBWrite(void)
{
REQUERY:

	if (mpDBConnector->Query((WCHAR*)L"begin;") == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	// accountdb status ���̺��� �������ش�.
	if (mpDBConnector->Query((WCHAR*)L"UPDATE `accountdb`.`status` SET `status` = '%d' WHERE `accountno` = '%lld'", mStatus, mpPlayer->GetAccountNo()) == FALSE)
	{
		if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			mpDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());	

		CCrashDump::Crash();
	}

	if (mCharacterType != 0)
	{
		// �α׾ƿ��Ͽ����� ĳ���� ������ DB �� �����Ѵ�.
		if (mpDBConnector->Query((WCHAR*)L"UPDATE `character` SET characterType = '%d', posX = '%lf', posY = '%lf', tileX = '%d', tileY = '%d', rotation = '%d', cristal = '%d', hp = '%d', exp = '%lld', level = '%d', die = '%d' WHERE accountno = '%lld'",
			mCharacterType, mPosX, mPosY, mTileX, mTileY, mRotation, mCristalCount, mHP, mEXP, mLevel, mbDieFlag, mpPlayer->GetAccountNo()) == FALSE)
		{
			if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
			{
				mpDBConnector->Reconnect();

				goto REQUERY;
			}

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

			CCrashDump::Crash();
		}

		if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `logdb`.`gamelog` (`logtime`, `accountno`,`servername`, `type`, `code`, `param1`, `param2`, `param3`, `param4`) VALUES(NOW(), '%lld', 'Game', '12', '11', '%d', '%d', '%d', '%d');",
			mpPlayer->GetAccountNo(), mTileX, mTileY, mCristalCount, mHP) == FALSE)
		{
			if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
			{
				mpDBConnector->Reconnect();

				goto REQUERY;
			}

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

			CCrashDump::Crash();
		}
	}
	else
	{
		if (mpDBConnector->Query((WCHAR*)L"INSERT INTO `logdb`.`gamelog` (`logtime`, `accountno`,`servername`, `type`, `code`) VALUES(NOW(), '%lld', 'Game', '12', '11');",
			mpPlayer->GetAccountNo()) == FALSE)
		{
			if (mpDBConnector->CheckReconnectErrorCode() == TRUE)
			{
				mpDBConnector->Reconnect();

				goto REQUERY;
			}

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

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

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[LogoutDBWrite] MySQL Error Code : %d, Error Message : %s", mpDBConnector->GetLastError(), mpDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}
	
	return;
}

void CLogoutDBWriteJob::SetLogoutDBWriteJob(CTLSDBConnector* pDBConnector, CPlayer* pPlayer, CHAR status)
{	
	mpPlayer = pPlayer;
	mpDBConnector = pDBConnector;

	mStatus = status;
	mCharacterType = pPlayer->GetCharacterType();

	// mCharacterType�� 0�̶�� ĳ���� ������ �����ʰ� �����Ͽ����� �ǹ�
	if (mCharacterType != 0)
	{
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
	

	return;
}

