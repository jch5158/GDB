#include "stdafx.h"


CPlayer::CPlayer(void)
	: mAccountNo(0)
	, mCharacterType(0)
	, mCristalCount(0)
	, mHP(0)
	, mEXP(0)
	, mLevel(0)
	, mbDieFlag(FALSE)
	, mDamage(0)
	, mRecoveryHP(0)
	, mbSitFlag(FALSE)
	, mbLoginFlag(FALSE)
	, mVKey(0)
	, mHKey(0)
	, mOldHP(0)
	, mRecoveryHPStartTick(0)
	, mRecoveryHPLastTick(0)
	, mDBWriteCount(0)
	, mpGodDamnBug(nullptr)
	, mPlayerID{ 0, }
	, mPlayerNick{ 0, }
{
}

CPlayer::~CPlayer(void)
{
}


void CPlayer::OnAuthClientJoin(void)
{
	setAuthClientJoinState();

	return;
}

void CPlayer::OnAuthClientLeave(void)
{
	// mAccountNo ��ü�� ���õ��� �ʰ� �������� �ٷ� ���� �� �ֵ��� mbReleaseFlag�� TRUE �� ����
	if (mAccountNo == ULLONG_MAX)
		SetRelease();
	else if (CheckLogoutInAuth() == TRUE)
	{
		// AuthToGameMode �� ��쿡�� AuthReleaseMap�� insert ���� ����

		// �α��ο� ���� ���� ���� ����
		if (mbLoginFlag == TRUE)
			mpGodDamnBug->InsertLogoutDBWriteJob(dfLOGIN_STATUS_NONE, this);

		mpGodDamnBug->InsertAuthReleasePlayerMap(mAccountNo, this);
	}

	return;
}

void CPlayer::OnAuthMessage(CMessage* pMessage)
{
	WORD messageType;

	*pMessage >> messageType;

	if (authRecvProcedure(messageType, pMessage) == FALSE)
	{
		Disconnect();
	}

	return;
}

void CPlayer::OnGameClientJoin(void)
{
	// ������Ʈ �Ŵ����� ��ϵǾ� Update ó�� �ǵ��� ��
	CObjectManager::GetInstance()->InsertObject(this);

	// �׾��ִ� ���¿��ٸ� Player �츮��
	if (mbDieFlag == TRUE)
	{
		playerRestart(FALSE);
	}
	else
	{
		mpGodDamnBug->InsertPlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID, this);

		// �� ĳ���� ������û ������
		sendCreateMyCharacter();

		// �ֺ� ���Ϳ� �ִ� ĳ���� ������ ������û ������
		sendCreateAroundOthrerObject();

		// �ֺ� ���Ϳ� �ִ� �ٸ� �����鿡�� �� ĳ���� ���� ��û ������
		sendAroundRespawnCharacter();
	}

	return;
}

void CPlayer::OnGameClientLeave(void)
{
	// �ɾ��ִ��� Ȯ��
	if (mbSitFlag == TRUE)
	{
		mbSitFlag = FALSE;

		// �ɾ��־��ٸ� ���� �α׾ƿ��� �ɾƼ� ���� �������� ������ HPRecovory �����͸� �����Ѵ�.
		mpGodDamnBug->InsertHPRecoveryDBWriteJob(this);
	}

	// ���� �ʿ��� �����Ѵ�.
	mpGodDamnBug->ErasePlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID);

	// Ŭ���̾�Ʈ ������Ʈ ������û �Ѹ���
	sendAroundRemoveObject(mCurSector.posX, mCurSector.posY, mClientID);

	// �α׾ƿ� DB Insert
	mpGodDamnBug->InsertLogoutDBWriteJob(dfLOGIN_STATUS_NONE, this);

	mbDeleteFlag = TRUE;

	return;
}

void CPlayer::OnGameMessage(CMessage* pMessage)
{
	WORD messageType;

	*pMessage >> messageType;

	if (gameRecvProcedure(messageType, pMessage) == FALSE)
	{
		Disconnect();
	}

	return;
}


void CPlayer::SetGodDamnBugPtr(CGodDamnBug* pGodDamnBug)
{
	mpGodDamnBug = pGodDamnBug;

	return;
}

void CPlayer::SetPlayerDefaultValue(INT damage, INT HP, INT recoveryHP)
{
	mInitializeDamage = damage;

	mInitializeHP = HP;

	mInitializeRecoveryHP = recoveryHP;

	return;
}


BOOL CPlayer::isDuplicateLogin(BYTE* pStatus)
{
	CSRWLockShared srwLock(&mpGodDamnBug->mPlayerMapLock);

	// ���� �������� �÷��̾ �ִ��� Ȯ���Ѵ�.
	CPlayer* pPlayer = mpGodDamnBug->FindPlayerFromPlayerMap(mAccountNo);
	if (pPlayer == nullptr)
	{
		return FALSE;
	}

	// ������ �������ִ� Disconnect();
	pPlayer->Disconnect();

	return TRUE;
}


BOOL CPlayer::getPlayerInfoFromGameDB(void)
{
	CTLSDBConnector* pDBConnector = mpGodDamnBug->GetDBConnector();

REQUERY:

	if (pDBConnector->Query((WCHAR*)L"SELECT * FROM `character` where accountno = %lld;", mAccountNo) == FALSE)
	{
		if (pDBConnector->CheckReconnectErrorCode() == TRUE)
		{
			pDBConnector->Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[SetDBPlayerInfo] MySQL Error Code : %d, Error Message : %s", pDBConnector->GetLastError(), pDBConnector->GetLastErrorMessage());

		CCrashDump::Crash();
	}

	pDBConnector->StoreResult();

	MYSQL_ROW mysqlRow = pDBConnector->FetchRow();
	if (mysqlRow == nullptr)
	{
		return FALSE;
	}


	SetPlayerInfo(mpGodDamnBug->GetObjectID(), atoi(mysqlRow[1]), (FLOAT)atof(mysqlRow[2]), (FLOAT)atof(mysqlRow[3]), atoi(mysqlRow[4]), atoi(mysqlRow[5]), atoi(mysqlRow[6]), atoi(mysqlRow[7]), atoi(mysqlRow[8]), atoll(mysqlRow[9]), atoi(mysqlRow[10]), atoi(mysqlRow[11]), mInitializeDamage, mInitializeRecoveryHP);

	pDBConnector->FreeResult();

	return TRUE;
}




BOOL CPlayer::authRecvProcedure(WORD messageType, CMessage* pMessage)
{
	switch (messageType)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN: // �α��� 

		return recvLoginRequest(pMessage);

	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT: // ĳ���� ����

		return recvCharacterSelectRequest(pMessage);

	case en_PACKET_CS_GAME_REQ_HEARTBEAT:

		break;

	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[authRecvProcedure] Message Type  : %d", messageType);

		return FALSE;
	}

	return TRUE;
}



BOOL CPlayer::gameRecvProcedure(WORD messageType, CMessage* pMessage)
{
	// TODO : �������� ó���Ұ�

	switch (messageType)
	{
	case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER: // ĳ���� �̵�

		return recvMoveCharacter(pMessage);

	case en_PACKET_CS_GAME_REQ_STOP_CHARACTER: // ĳ���� �̵� ����

		return recvMoveStopCharacter(pMessage);

	case en_PACKET_CS_GAME_REQ_ATTACK1: // ĳ���� ���� 1

		return recvCharacterAttack1(pMessage);

	case en_PACKET_CS_GAME_REQ_ATTACK2: // ĳ���� ���� 2

		return recvCharacterAttack2(pMessage);

	case en_PACKET_CS_GAME_REQ_PICK: // ������ �ݱ�

		return recvCristalPick(pMessage);

	case en_PACKET_CS_GAME_REQ_SIT: // �ɱ�

		return recvCharacterSit(pMessage);

	case en_PACKET_CS_GAME_REQ_PLAYER_RESTART: // ��� �� �����

		return recvPlayerRestart(pMessage);

	case en_PACKET_CS_GAME_REQ_HEARTBEAT:

		break;

	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[gameRecvProcedure] Message Type  : %d", messageType);


		return FALSE;
	}

	return TRUE;
}


BOOL CPlayer::recvLoginRequest(CMessage* pMessage)
{
	BYTE status;

	*pMessage >> mAccountNo;

	do
	{
		if (mpGodDamnBug->CompareSessionToken(mAccountNo, pMessage->GetPayloadPtr()) == FALSE)
		{
			status = dfGAME_LOGIN_FAIL;

			break;
		}

		// �ߺ� �α����� Ȯ���Ѵ�. FALSE �� ��� ���� �������� �ƴϴ� ���� ������ �����Ѵ�.
		if (isDuplicateLogin(&status) == TRUE)
		{
			status = dfGAME_LOGIN_FAIL;

			break;
		}

		// Account ID �� Nick ��ȸ
		if (mpGodDamnBug->GetIDWithNickFromAccountDB(mAccountNo, mPlayerID, player::PLAYER_STRING, mPlayerNick, player::PLAYER_STRING, &status) == FALSE)
		{
			break;
		}

		mbLoginFlag = TRUE;

		// DB Ȯ�� �� ĳ���Ͱ� ���� ��� �÷��̾� ������ �ϰ� ���� ��� ĳ���� ���� status�� ��û�Ѵ�.
		if (getPlayerInfoFromGameDB() == TRUE)
		{
			status = dfGAME_LOGIN_OK;

			// AuthToGameFlag�� �����Ѵ�.
			SetAuthToGame();
		}
		else
		{
			status = dfGAME_LOGIN_NOCHARACTER;
		}

		{
			CSRWLockExclusive srwLock(&mpGodDamnBug->mPlayerMapLock);

			// PlayerMap �� Insert �Ѵ�.
			mpGodDamnBug->InsertPlayerMap(mAccountNo, this);
		}

	} while (0);

	sendLoginResponse(status);

	mpGodDamnBug->InsertLoginDBWriteJob(status, this);

	mpGodDamnBug->IncrementLoginRequestCount();

	//mpGodDamnBug->GetLanLoginClient()->NotificationClientLoginSuccess(mAccountNo);

	return TRUE;
}


BOOL CPlayer::recvCharacterSelectRequest(CMessage* pMessage)
{
	BYTE characterType;

	*pMessage >> characterType;

	if (validateRecvCharacterSelectRequest(characterType) == FALSE)
	{
		return FALSE;
	}

	// �÷��̾� �⺻ ������ ��ǥ ����
	SetPlayerRespawnCoordinate();

	// �÷��̾� ������ �����մϴ�.
	SetPlayerInfo(mpGodDamnBug->GetObjectID(), characterType, mPosX, mPosY, mTileX, mTileY, rand() % 360, 0, mInitializeHP, 0, 1, 0, mInitializeDamage, mInitializeRecoveryHP);

	// ĳ���� ������ �����Ͽ����� �۽�
	sendCharacterSelectResponse(TRUE);

	// ĳ���� �����Ͽ����� DB�� ����
	mpGodDamnBug->InsertCreateCharacterDBWriteJob(this);

	// AuthToGameFlag�� �����Ѵ�.
	SetAuthToGame();

	return TRUE;
}


BOOL CPlayer::recvMoveCharacter(CMessage* pMessage)
{
	if (mbDieFlag == TRUE)
	{
		return TRUE;
	}

	UINT64 clientID;

	FLOAT posX;
	FLOAT posY;

	WORD rotation;

	BYTE vKey;
	BYTE hKey;

	*pMessage >> clientID >> posX >> posY >> rotation >> vKey >> hKey;

	// mbLoginFlag �� FALSE�� ���¿��� ��ǥ�� ���� �޽����� ó���ع����� ��ǥ�� �ٲ� �� �ֱ� ������
	// ���Ϳ� ���� ����ȭ�� Ʋ������. 
	if (validateRecvMoveCharacter(clientID, posX, posY, rotation, vKey, hKey) == FALSE)
	{
		return FALSE;
	}

	mPosX = posX;
	mPosY = posY;

	mTileX = (INT)X_TILE(posX);
	mTileY = (INT)Y_TILE(posY);

	mRotation = rotation;

	mVKey = vKey;
	mHKey = hKey;

	sendAroundCharacterMoveStart();

	if (updatePlayerSector() == TRUE)
	{
		sendAroundUpdateCharacterSector();
	}

	return TRUE;
}

BOOL CPlayer::recvMoveStopCharacter(CMessage* pMessage)
{
	// ���Ϳ� ���ؼ� ������ �ޱ� ������ �̹� �������� �޽����� �ʰ� ó���Ǿ� mbDieFlag�� TRUE�� �� �� ����
	if (mbDieFlag == TRUE)
	{
		return TRUE;
	}

	UINT64 clientID;

	FLOAT posX;
	FLOAT posY;

	WORD rotation;

	*pMessage >> clientID >> posX >> posY >> rotation;

	if (validateRecvMoveStopCharacter(clientID, posX, posY, rotation) == FALSE)
	{
		return FALSE;
	}

	mPosX = posX;
	mPosY = posY;

	mTileX = (INT)X_TILE(posX);
	mTileY = (INT)Y_TILE(posY);

	mRotation = rotation;

	mVKey = 0;
	mHKey = 0;

	if (mbSitFlag == TRUE)
	{
		mbSitFlag = FALSE;

		sendPlayerHP();

		mpGodDamnBug->InsertHPRecoveryDBWriteJob(this);
	}

	if (updatePlayerSector() == TRUE)
	{
		sendAroundUpdateCharacterSector();
	}

	sendAroundCharacterMoveStop();

	return TRUE;
}



BOOL CPlayer::recvCharacterAttack1(CMessage* pMessage)
{
	if (mbDieFlag == TRUE)
	{
		return TRUE;
	}

	UINT64 clientID;

	*pMessage >> clientID;

	// ������ �ߺ� �α������� ���� ����ȭ�� ���� �ʿ䰡 ����
	if (validateRecvCharacterAttack1(clientID) == FALSE)
	{
		return FALSE;
	}

	// ���� ��� �Ѹ���
	sendAroundCharacterAttack1();

	CMonster* pVictimMonster;

	if (getAttack1VictimMonster(&pVictimMonster) == FALSE)
	{
		return TRUE;
	}

	// ������ �Ѹ���
	sendAroundAttack1Damage(pVictimMonster, mDamage);

	pVictimMonster->CalculatePlayerAttack(this, mDamage);

	return TRUE;
}

BOOL CPlayer::recvCharacterAttack2(CMessage* pMessage)
{
	if (mbDieFlag == TRUE)
	{
		return TRUE;
	}

	UINT64 clientID;

	*pMessage >> clientID;

	if (validateRecvCharacterAttack2(clientID) == FALSE)
	{
		return FALSE;
	}

	// ���� ��� �Ѹ���
	sendAroundCharacterAttack2();

	CMonster* pVictimMonster;

	if (getAttack2VictimMonster(&pVictimMonster) == FALSE)
	{
		return TRUE;
	}

	// ������ �Ѹ���
	sendAroundAttack2Damage(pVictimMonster, mDamage);

	pVictimMonster->CalculatePlayerAttack(this, mDamage);

	return TRUE;
}


//
//BOOL CPlayer::recvCharacterAttack2(CMessage* pMessage)
//{
//	if (mbDieFlag == TRUE)
//	{
//		return TRUE;
//	}
//
//	UINT64 clientID;
//
//	*pMessage >> clientID;
//
//	if (validateRecvCharacterAttack2(clientID) == FALSE)
//	{
//		return FALSE;
//	}
//
//	// ���� ��� �Ѹ���
//	sendAroundCharacterAttack2();
//
//	DWORD findVictimMonsterCount;
//
//	CMonster* pVictimMonsterArray[ATTACK2_VICTIM_COUNT];
//
//	CGodDamnBug::stSectorAround16 pSectorAround16;
//
//	if (getAttack2VictimMonsters(pVictimMonsterArray, _countof(pVictimMonsterArray), &findVictimMonsterCount, &pSectorAround16) == FALSE)
//	{
//		return TRUE;
//	}
//
//	// ������ �̸�� �Ѹ���
//	//sendAroundAttack2Damage(pVictimMonsterArray, findVictimMonsterCount, mDamage / 2, &pSectorAround16);
//
//	for (DWORD count = 0; count < findVictimMonsterCount; ++count)
//	{
//		pVictimMonsterArray[count]->UpdateAttackDamage(mAccountNo, mDamage / 2);
//	}
//
//	return TRUE;
//}



BOOL CPlayer::recvCristalPick(CMessage* pMessage)
{
	if (mbDieFlag == TRUE)
	{
		return TRUE;
	}

	UINT64 clientID;

	CCristal* pCristal;

	*pMessage >> clientID;


	if (validateRecvCristalPick(clientID) == FALSE)
	{
		return FALSE;
	}

	sendAroundCristalPickAction();

	if (pickCristal(&pCristal) == FALSE)
	{
		return TRUE;
	}

	mCristalCount += pCristal->GetCristalAmount();

	sendAroundCristalPick(pCristal->GetClientID());

	mpGodDamnBug->RemoveCristal(pCristal);

	mpGodDamnBug->InsertCristalPickDBWriteJob(pCristal->GetCristalAmount(), this);

	return TRUE;
}

BOOL CPlayer::recvCharacterSit(CMessage* pMessage)
{
	if (mbDieFlag == TRUE)
	{
		return TRUE;
	}

	UINT64 clientID;

	*pMessage >> clientID;

	if (validateRecvCharacterSit(clientID) == FALSE)
	{
		return FALSE;
	}

	mOldHP = mHP;

	// �ɱ� �÷��׷� ����
	mbSitFlag = TRUE;

	mRecoveryHPStartTick = timeGetTime();

	sendAroundCharacterSit();

	return TRUE;
}

BOOL CPlayer::recvPlayerRestart(CMessage* pMessage)
{

	if (validateRecvPlayerRestart() == FALSE)
	{
		return FALSE;
	}

	playerRestart(TRUE);

	return TRUE;
}


BOOL CPlayer::validateRecvCharacterSelectRequest(BYTE characterType)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}


	// ������ ĳ���� Ÿ���� ������ ����ٸ� return FALSE
	// Ư�� ��Ŷ�� ������ ���´ٴ� �� �ǹ̰� ���� �׳� ���������.
	if (characterType >= (BYTE)eCharacterType::LastType)
	{
		return FALSE;
	}

	return TRUE;
}



BOOL CPlayer::validateRecvMoveCharacter(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE vKey, BYTE hKey)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}

	if (mClientID != clientID)
	{
		return FALSE;
	}

	if (mbSitFlag == TRUE)
	{
		return FALSE;
	}

	if (posX >= X_RANGE || posX < 0 || posY >= Y_RANGE || posY < 0)
	{
		return FALSE;
	}

	if (rotation > 360)
	{
		return FALSE;
	}

	if (vKey > 2 || hKey > 2)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL CPlayer::validateRecvMoveStopCharacter(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}

	if (mClientID != clientID)
	{
		return FALSE;
	}

	if (posX >= X_RANGE || posX < 0 || posY >= Y_RANGE || posY < 0)
	{
		return FALSE;
	}

	if (rotation > 360)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL CPlayer::validateRecvCharacterAttack1(UINT64 clientID)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}

	if (mClientID != clientID)
	{
		return FALSE;
	}

	if (mbSitFlag == TRUE)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL CPlayer::validateRecvCharacterAttack2(UINT64 clientID)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}

	if (mClientID != clientID)
	{
		return FALSE;
	}

	if (mbSitFlag == TRUE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CPlayer::validateRecvCristalPick(UINT64 clientID)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}


	if (mClientID != clientID)
	{
		return FALSE;
	}


	if (mbSitFlag == TRUE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CPlayer::validateRecvCharacterSit(UINT64 clientID)
{

	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}

	if (mClientID != clientID)
	{
		return FALSE;
	}

	if (mbSitFlag == TRUE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CPlayer::validateRecvPlayerRestart(void)
{
	if (mbLoginFlag == FALSE)
	{
		return FALSE;
	}

	if (mbSitFlag == TRUE)
	{
		return FALSE;
	}

	if (mbDieFlag == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}



void CPlayer::sendLoginResponse(BYTE status)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingLoginResponse(status, mAccountNo, pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendCharacterSelectResponse(BYTE status)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCharacterSelectResponse(status, pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}


void CPlayer::sendCreateMyCharacter(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCreateMyCharacter(mClientID, mCharacterType, mPlayerNick, sizeof(WCHAR) * player::PLAYER_STRING, mPosX, mPosY, mRotation, mCristalCount, mHP, mEXP, mLevel, pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendAroundRespawnCharacter(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCreateOthreadCharacter(mClientID, mCharacterType, mPlayerNick, player::PLAYER_STRING * sizeof(WCHAR), mPosX, mPosY, mRotation, mLevel, TRUE, mbSitFlag, mbDieFlag, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendCreateAroundOthrerObject(void)
{
	stSectorAround sectorAround;

	goddamnbug::GetSectorAround(mCurSector.posX, mCurSector.posY, &sectorAround);

	for (INT count = 0; count < sectorAround.count; ++count)
	{
		for (const auto &iter : *mpGodDamnBug->FindPlayerSectorMap(sectorAround.around[count].posX, sectorAround.around[count].posY))
		{
			if (this != iter.second)
			{
				CPlayer* pOtherPlayer = iter.second;

				CMessage* pMessage = CMessage::Alloc();

				goddamnbug::PackingCreateOthreadCharacter(pOtherPlayer->mClientID, pOtherPlayer->mCharacterType, pOtherPlayer->mPlayerNick, player::PLAYER_STRING * sizeof(WCHAR), pOtherPlayer->mPosX, pOtherPlayer->mPosY, pOtherPlayer->mRotation, pOtherPlayer->mLevel, FALSE, pOtherPlayer->mbSitFlag, pOtherPlayer->mbDieFlag, pMessage);

				SendPacket(pMessage);

				pMessage->Free();
			}
		}

		for (const auto &iter : *mpGodDamnBug->FindMonsterSectorMap(sectorAround.around[count].posX, sectorAround.around[count].posY))
		{
			CMonster* pMonster = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateMonster(pMonster->GetClientID(), pMonster->GetPosX(), pMonster->GetPosY(), pMonster->GetRotation(), FALSE, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}


		for (const auto &iter : *mpGodDamnBug->FindCristalSectorMap(sectorAround.around[count].posX, sectorAround.around[count].posY))
		{
			CCristal* pCristal = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateCristal(pCristal->GetClientID(), pCristal->GetCristalType(), pCristal->GetPosX(), pCristal->GetPosY(), pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}
	}

	return;
}

void CPlayer::sendAroundUpdateCharacterSector(void)
{
	stSectorAround removeSectorAround;
	stSectorAround addSectorAround;

	goddamnbug::GetUpdateSectorAround(mOldSector, mCurSector, &removeSectorAround, &addSectorAround);

	// �����Ǵ� ���Ϳ� clientID ������Ʈ ���� ��û ������
	sendRemoveSectorDeleteMyCharacter(&removeSectorAround);

	// �����Ǵ� ���Ϳ� �ִ� ������Ʈ ������û ������
	sendRemoveSectorDeleteObject(&removeSectorAround);

	// �߰��Ǵ� ���Ϳ� pPlayer ĳ���� ���� ��û ������
	sendAddSectorCreateMyCharacter(&addSectorAround);

	// �߰��Ǵ� ���Ϳ� �ִ� ������Ʈ pPlayer���� ������û ������
	sendAddSectorCreateObject(&addSectorAround);

	return;
}

void CPlayer::sendRemoveSectorDeleteMyCharacter(stSectorAround* pRemoveSectorAround)
{
	CMessage* pMessage = CMessage::Alloc();

	// ������Ʈ ���� ��Ŷ �����
	goddamnbug::PackingRemoveObject(mClientID, pMessage);

	for (INT count = 0; count < pRemoveSectorAround->count; ++count)
	{
		// �����Ǵ� ���Ϳ� ���� �Ѹ���
		mpGodDamnBug->SendPacketOneSector(pRemoveSectorAround->around[count].posX, pRemoveSectorAround->around[count].posY, nullptr, pMessage);
	}

	pMessage->Free();

	return;
}


void CPlayer::sendRemoveSectorDeleteObject(stSectorAround* pRemoveSectorAround)
{
	for (INT count = 0; count < pRemoveSectorAround->count; ++count)
	{
		// �����Ǵ� ������ �÷��̾� ������Ʈ ����
		for (const auto &iter : *mpGodDamnBug->FindPlayerSectorMap(pRemoveSectorAround->around[count].posX, pRemoveSectorAround->around[count].posY))
		{
			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingRemoveObject(iter.second->mClientID, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}


		// �����Ǵ� ������ ���� ������Ʈ ����
		for (const auto &iter : *mpGodDamnBug->FindMonsterSectorMap(pRemoveSectorAround->around[count].posX, pRemoveSectorAround->around[count].posY))
		{
			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingRemoveObject(iter.second->GetClientID(), pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}


		for (const auto &iter : *mpGodDamnBug->FindCristalSectorMap(pRemoveSectorAround->around[count].posX, pRemoveSectorAround->around[count].posY))
		{
			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingRemoveObject(iter.second->GetClientID(), pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}
	}

	return;
}





void CPlayer::sendAddSectorCreateMyCharacter(stSectorAround* pAddSectorAround)
{
	CMessage* pMessage = CMessage::Alloc();

	// pPlayer ĳ���� ���� ��Ŷ �����
	goddamnbug::PackingCreateOthreadCharacter(mClientID, mCharacterType, mPlayerNick, player::PLAYER_STRING * sizeof(WCHAR), mPosX, mPosY, mRotation, mLevel, FALSE, mbSitFlag, mbDieFlag, pMessage);

	for (INT count = 0; count < pAddSectorAround->count; ++count)
	{
		// �߰��Ǵ� ���Ϳ� �Ѹ���
		mpGodDamnBug->SendPacketOneSector(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY, nullptr, pMessage);
	}

	pMessage->Free();

	return;
}

// �ֺ� ���Ϳ� �ش� ������Ʈ ������û ������
void CPlayer::sendAroundRemoveObject(INT sectorPosX, INT sectorPosY, UINT64 clientID)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingRemoveObject(clientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(sectorPosX, sectorPosY, this, pMessage);

	pMessage->Free();

	return;
}


void CPlayer::sendAddSectorCreateObject(stSectorAround* pAddSectorAround)
{
	for (INT count = 0; count < pAddSectorAround->count; ++count)
	{
		// �߰��Ǵ� ������ �÷��̾� ������û
		for (const auto &iter : *mpGodDamnBug->FindPlayerSectorMap(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY))
		{
			CPlayer* pAddPlayer = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateOthreadCharacter(pAddPlayer->mClientID, pAddPlayer->mCharacterType, pAddPlayer->mPlayerNick, player::PLAYER_STRING * sizeof(WCHAR), pAddPlayer->mPosX, pAddPlayer->mPosY, pAddPlayer->mRotation, pAddPlayer->mLevel, FALSE, pAddPlayer->mbSitFlag, pAddPlayer->mbDieFlag, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}

		// �߰��Ǵ� ������ ���� ������û
		for (const auto &iter : *mpGodDamnBug->FindMonsterSectorMap(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY))
		{
			CMonster* pMonster = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateMonster(pMonster->GetClientID(), pMonster->GetPosX(), pMonster->GetPosY(), pMonster->GetRotation(), FALSE, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}

		// �߰��Ǵ� ������ ũ����Ż ������û
		for (const auto &iter : *mpGodDamnBug->FindCristalSectorMap(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY))
		{
			CCristal* pCristal = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateCristal(pCristal->GetClientID(), pCristal->GetCristalType(), pCristal->GetPosX(), pCristal->GetPosY(), pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}
	}

	return;
}


void CPlayer::sendAroundCharacterMoveStart(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCharacterMoveStart(mClientID, mPosX, mPosY, mRotation, mVKey, mHKey, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendAroundCharacterMoveStop(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCharacterMoveStop(mClientID, mPosX, mPosY, mRotation, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendAroundCharacterAttack1(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingPlayerAttack1Action(mClientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}


void CPlayer::sendAroundCharacterAttack2(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingPlayerAttack2Action(mClientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}


void CPlayer::sendAroundCristalPickAction(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCristalPickAction(mClientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}


void CPlayer::sendAroundCristalPick(UINT64 cristalClientID)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCristalPick(mClientID, cristalClientID, mCristalCount, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, nullptr, pMessage);

	pMessage->Free();

	return;
}


void CPlayer::sendAroundCharacterSit(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCharacterSit(mClientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, this, pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendAroundAttack1Damage(CMonster* pVictimMonster, INT damage)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingAttackDamage(mClientID, pVictimMonster->GetClientID(), damage, pMessage);

	// pVictimPlayer ���� �����ؼ� ��Ŷ�� �Ѹ���.
	mpGodDamnBug->SendPacketAroundSector(pVictimMonster->GetCurSectorX(), pVictimMonster->GetCurSectorY(), nullptr, pMessage);

	pMessage->Free();

	return;
}

//void CPlayer::sendAroundAttack2Damage(CMonster** pVicTimMonsterArray, DWORD victimMonsterCount, INT damage, CGodDamnBug::stSectorAround16* pSectorAround16)
//{
//	for (INT sectorCount = 0; sectorCount < pSectorAround16->count; ++sectorCount)
//	{
//		for (DWORD count = 0; count < victimMonsterCount; ++count)
//		{		
//			CMessage* pMessage = CMessage::Alloc();
//
//			CGodDamnBug::PackingAttackDamage(mClientID, pVicTimMonsterArray[count]->mClientID, damage, pMessage);
//
//			mpGodDamnBug->SendPacketOneSector(pSectorAround16->around[sectorCount].posX, pSectorAround16->around[sectorCount].posY, nullptr, pMessage);
//		
//			pMessage->Free();
//		}
//	}
//
//	return;
//}

void CPlayer::sendAroundAttack2Damage(CMonster* pVictimMonster, INT damage)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingAttackDamage(mClientID, pVictimMonster->GetClientID(), damage, pMessage);

	// pVictimPlayer ���� �����ؼ� ��Ŷ�� �Ѹ���.
	mpGodDamnBug->SendPacketAroundSector(pVictimMonster->GetCurSectorX(), pVictimMonster->GetCurSectorY(), nullptr, pMessage);

	pMessage->Free();

}




void CPlayer::sendAroundPlayerDie(INT minusCristal)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingPlayerDie(mClientID, minusCristal, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, nullptr, pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendPlayerHP(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingPlayerHP(mHP, pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}

void CPlayer::sendPlayerRestart(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingPlayerRestart(pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}


void CPlayer::SetPlayerInfo(UINT64 clientID, BYTE characterType, FLOAT posX, FLOAT posY, INT tileX, INT tileY, WORD rotation, INT cristalCount, INT HP, INT64 EXP, WORD level, BOOL bDieFlag, INT damage, INT recoveryHP)
{
	mClientID = clientID;

	mCharacterType = characterType;

	mPosX = posX;

	mPosY = posY;

	mTileX = tileX;

	mTileY = tileY;

	mRotation = rotation;

	mCristalCount = cristalCount;

	mHP = HP;

	mEXP = EXP;

	mLevel = level;

	mbDieFlag = bDieFlag;

	mDamage = damage;

	mRecoveryHP = recoveryHP;

	mbSitFlag = FALSE;

	mVKey = 0;

	mHKey = 0;

	mbDieFlag = bDieFlag;

	// �÷��̾� ���� ����
	goddamnbug::GetSector(tileX, tileY, &mCurSector);

	//pPlayer->mOldSector = mCurSector;

	mbDeleteFlag = FALSE;

	return;
}


void CPlayer::SetPlayerRespawnCoordinate(void)
{
	mTileX = 85 + (rand() % 21);
	mTileY = 93 + (rand() % 31);

	mPosX = X_BLOCK(mTileX);
	mPosY = Y_BLOCK(mTileY);

	return;
}


LONG CPlayer::GetDBWriteCount(void) const
{
	return mDBWriteCount;
}

UINT64 CPlayer::GetAccountNo(void) const
{
	return mAccountNo;
}

BOOL CPlayer::GetDieFlag(void) const
{
	return mbDieFlag;
}

INT64 CPlayer::GetEXP(void) const
{
	return mEXP;
}

WORD CPlayer::GetLevel(void) const
{
	return mLevel;
}

INT CPlayer::GetHP(void) const
{
	return mHP;
}

INT CPlayer::GetCristalCount(void) const
{
	return mCristalCount;
}

BYTE CPlayer::GetCharacterType(void) const
{
	return mCharacterType;
}

INT CPlayer::GetOldHP(void) const
{
	return mOldHP;
}

DWORD CPlayer::GetRecoveryHPStartTick(void) const
{
	return mRecoveryHPStartTick;
}



void CPlayer::HPRecovery(void)
{
	if (mbSitFlag == FALSE)
	{
		return;
	}

	DWORD nowTime = timeGetTime();

	if (nowTime - mRecoveryHPLastTick >= 1000)
	{
		if (mHP + mRecoveryHP < mInitializeHP)
		{
			mHP += mRecoveryHP;
		}
		else
		{
			mHP = mInitializeHP;
		}

		mRecoveryHPLastTick = nowTime;
	}

	return;
}


void CPlayer::UpdateAttackDamage(INT damage)
{
	if (mbSitFlag == TRUE)
	{
		mbSitFlag = FALSE;

		sendPlayerHP();

		mpGodDamnBug->InsertHPRecoveryDBWriteJob(this);
	}

	mHP -= damage;

	if (mHP <= 0)
	{
		mHP = 0;

		mbDieFlag = TRUE;

		INT minusCristal;

		if (mCristalCount < 1000)
		{
			minusCristal = mCristalCount;

			mCristalCount = 0;
		}
		else
		{
			minusCristal = 1000;

			mCristalCount -= 1000;
		}

		sendAroundPlayerDie(minusCristal);

		mpGodDamnBug->CreateCristal(mPosX, mPosY, mTileX, mTileY, mCurSector);

		// �÷��̾� ������ DB�� �����Ѵ�.
		mpGodDamnBug->InsertDieDBWriteJob(this);
	}

	return;
}


void CPlayer::Update(void)
{
	HPRecovery();

	return;
}

BOOL CPlayer::DeleteObject(void)
{
	if (mbDeleteFlag == TRUE && mDBWriteCount == 0)
	{
		{
			CSRWLockExclusive srwLock(&mpGodDamnBug->mPlayerMapLock);

			// �÷��̾� �ʿ��� �����Ѵ�.
			mpGodDamnBug->ErasePlayerMap(mAccountNo);
		}

		SetRelease();

		return TRUE;
	}

	return FALSE;
}

LONG CPlayer::IncrementDBWriteCount(void)
{
	return InterlockedIncrement(&mDBWriteCount);
}

LONG CPlayer::DecrementDBWriteCount(void)
{
	return InterlockedDecrement(&mDBWriteCount);
}



BOOL CPlayer::updatePlayerSector(void)
{
	stSector sector;

	goddamnbug::GetSector(mTileX, mTileY, &sector);

	if (mCurSector.posX == sector.posX && mCurSector.posY == sector.posY)
	{
		return FALSE;
	}

	mOldSector = mCurSector;

	mpGodDamnBug->ErasePlayerSectorMap(mOldSector.posX, mOldSector.posY, mClientID);

	mCurSector = sector;

	mpGodDamnBug->InsertPlayerSectorMap(sector.posX, sector.posY, mClientID, this);

	return TRUE;
}

BOOL CPlayer::getAttack1VictimMonster(CMonster** pVictimMonster)
{
	stCollisionBox collisionBox;
	goddamnbug::GetAttackCollisionBox(mRotation, mTileX, mTileY, PLAYER_ATTACK1_RANGE, &collisionBox);

	stSectorAround sectorAround;
	goddamnbug::GetSectorAround(mCurSector.posX, mCurSector.posY, &sectorAround);

	for (INT count = 0; count < sectorAround.count; ++count)
	{
		for (const auto& iter : *mpGodDamnBug->FindMonsterSectorMap(sectorAround.around[count].posX, sectorAround.around[count].posY))
		{
			if (iter.second->GetTileX() >= collisionBox.pointOne.tileX && iter.second->GetTileY() >= collisionBox.pointOne.tileY
				&& iter.second->GetTileX() <= collisionBox.pointTwo.tileX && iter.second->GetTileY() <= collisionBox.pointTwo.tileY)
			{
				*pVictimMonster = iter.second;

				return TRUE;
			}
		}
	}

	return FALSE;
}

BOOL CPlayer::getAttack2VictimMonster(CMonster** pVictimMonster)
{
	stCollisionBox collisionBox;
	goddamnbug::GetAttackCollisionBox(mRotation, mTileX, mTileY, PLAYER_ATTACK2_RANGE, &collisionBox);

	stSectorAround sectorAround;
	goddamnbug::GetSectorAround(mCurSector.posX, mCurSector.posY, &sectorAround);

	for (INT count = 0; count < sectorAround.count; ++count)
	{
		for (const auto iter : *mpGodDamnBug->FindMonsterSectorMap(sectorAround.around[count].posX, sectorAround.around[count].posY))
		{
			if (iter.second->GetTileX() >= collisionBox.pointOne.tileX && iter.second->GetTileY() >= collisionBox.pointOne.tileY
				&& iter.second->GetTileX() <= collisionBox.pointTwo.tileX && iter.second->GetTileY() <= collisionBox.pointTwo.tileY)
			{
				*pVictimMonster = iter.second;

				return TRUE;
			}
		}
	}

	return FALSE;
}

//
//BOOL CPlayer::getAttack2VictimMonsters(CMonster** pVictimMonsterArray, DWORD arrayLength, DWORD* pFindVictimMonsterCount, CGodDamnBug::stSectorAround16* pSectorAround16)
//{
//	BOOL bFindFlag = FALSE;
//
//	stCollisionBox collisionBox;
//	goddamnbug::GetAttackCollisionBox(mRotation, mTileX, mTileY, PLAYER_ATTACK2_RANGE, &collisionBox);
//
//	goddamnbug::GetAttackSectorAround16(mRotation, mCurSector, pSectorAround16);
//
//	INT findPlayerCount = 0;
//
//	for (INT count = 0; count < pSectorAround16->count; ++count)
//	{
//		for (const auto iter : mpGodDamnBug->mMonsterSectorMap[pSectorAround16->around[count].posY][pSectorAround16->around[count].posX])
//		{
//			if (iter.second->mTileX >= collisionBox.pointOne.tileX && iter.second->mTileY >= collisionBox.pointOne.tileY
//				&& iter.second->mTileX <= collisionBox.pointTwo.tileX && iter.second->mTileY <= collisionBox.pointTwo.tileY)
//			{
//				pVictimMonsterArray[findPlayerCount++] = iter.second;
//
//				bFindFlag = TRUE;
//
//				if (findPlayerCount == arrayLength)
//				{
//					count = pSectorAround16->count;
//					break;
//				}
//			}
//		}
//	}
//
//	*pFindVictimMonsterCount = findPlayerCount;
//
//	return bFindFlag;
//}


void CPlayer::playerRestart(BOOL bObjectRemoveFlag)
{
	// �������� ���¿��� ����Ͽ� �� �ڽ��� �����϶�� Ŭ���̾�Ʈ�鿡�� �˷���� ��
	if (bObjectRemoveFlag == TRUE)
	{
		sendAroundRemoveObject(mCurSector.posX, mCurSector.posY, mClientID);

		mpGodDamnBug->ErasePlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID);
	}


	// ������� ���� HP �� ��ǥ�� ���� �Ҵ� �޴´�.
	setRestartPlayerInfo();

	// ���� ������ ��ǥ�� �������� ���Ϳ� �ٽ� �־��ش�.
	mpGodDamnBug->InsertPlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID, this);

	// �÷��̾� ����� �޽��� �۽�
	sendPlayerRestart();

	// �� ĳ���� ���� �޽��� �۽�
	sendCreateMyCharacter();

	// �� ĳ���� �����϶�� �ֺ��� �Ѹ���
	sendAroundRespawnCharacter();

	// �ֺ� �ٸ� ������Ʈ �����϶�� ������ �Ѹ���
	sendCreateAroundOthrerObject();

	mpGodDamnBug->InsertRestartDBWriteJob(this);

	return;
}

void CPlayer::getCristalPickCollisionBox(stCollisionBox* pCollisionBox)
{
	pCollisionBox->pointOne.tileX = mTileX - CRISTAL_PICK_RANGE / 2;
	pCollisionBox->pointOne.tileY = mTileY - CRISTAL_PICK_RANGE / 2;

	pCollisionBox->pointTwo.tileX = mTileX + CRISTAL_PICK_RANGE / 2;
	pCollisionBox->pointTwo.tileY = mTileY + CRISTAL_PICK_RANGE / 2;

	return;
}


BOOL CPlayer::pickCristal(CCristal** pCristal)
{
	stCollisionBox collisionBox;
	getCristalPickCollisionBox(&collisionBox);

	stSectorAround sectorAround;
	goddamnbug::GetSectorAround(mCurSector.posX, mCurSector.posY, &sectorAround);

	for (INT count = 0; count < sectorAround.count; ++count)
	{
		for (auto iter : *mpGodDamnBug->FindCristalSectorMap(sectorAround.around[count].posX, sectorAround.around[count].posY))
		{
			if (iter.second->GetTileX() >= collisionBox.pointOne.tileX && iter.second->GetTileY() >= collisionBox.pointOne.tileY
				&& iter.second->GetTileX() <= collisionBox.pointTwo.tileX && iter.second->GetTileY() <= collisionBox.pointTwo.tileY)
			{
				*pCristal = iter.second;

				return TRUE;
			}
		}
	}

	return FALSE;
}

void CPlayer::setRestartPlayerInfo(void)
{
	mbDieFlag = FALSE;

	mHP = mInitializeHP;

	SetPlayerRespawnCoordinate();

	goddamnbug::GetSector(mTileX, mTileY, &mCurSector);

	return;
}

void CPlayer::setAuthClientJoinState(void)
{
	mAccountNo = ULLONG_MAX;

	mCharacterType = 0;

	mbLoginFlag = FALSE;

	return;
}
