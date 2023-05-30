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
	// mAccountNo 자체도 셋팅되지 않고 끊겼으면 바로 끊길 수 있도록 mbReleaseFlag를 TRUE 로 변경
	if (mAccountNo == ULLONG_MAX)
		SetRelease();
	else if (CheckLogoutInAuth() == TRUE)
	{
		// AuthToGameMode 일 경우에는 AuthReleaseMap이 insert 하지 않음

		// 로그인에 성공 했을 때만 남김
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
	// 오브젝트 매니저에 등록되어 Update 처리 되도록 함
	CObjectManager::GetInstance()->InsertObject(this);

	// 죽어있는 상태였다면 Player 살리기
	if (mbDieFlag == TRUE)
	{
		playerRestart(FALSE);
	}
	else
	{
		mpGodDamnBug->InsertPlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID, this);

		// 내 캐릭터 생성요청 보내기
		sendCreateMyCharacter();

		// 주변 섹터에 있는 캐릭터 나에게 생성요청 보내기
		sendCreateAroundOthrerObject();

		// 주변 섹터에 있는 다른 유저들에게 내 캐릭터 생성 요청 보내기
		sendAroundRespawnCharacter();
	}

	return;
}

void CPlayer::OnGameClientLeave(void)
{
	// 앉아있는지 확인
	if (mbSitFlag == TRUE)
	{
		mbSitFlag = FALSE;

		// 앉아있었다면 게임 로그아웃시 앉아서 쉬는 컨텐츠가 끝나기 HPRecovory 데이터를 저장한다.
		mpGodDamnBug->InsertHPRecoveryDBWriteJob(this);
	}

	// 섹터 맵에서 제거한다.
	mpGodDamnBug->ErasePlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID);

	// 클라이언트 오브젝트 삭제요청 뿌리기
	sendAroundRemoveObject(mCurSector.posX, mCurSector.posY, mClientID);

	// 로그아웃 DB Insert
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

	// 현재 접속중인 플레이어가 있는지 확인한다.
	CPlayer* pPlayer = mpGodDamnBug->FindPlayerFromPlayerMap(mAccountNo);
	if (pPlayer == nullptr)
	{
		return FALSE;
	}

	// 기존에 접속해있던 Disconnect();
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
	case en_PACKET_CS_GAME_REQ_LOGIN: // 로그인 

		return recvLoginRequest(pMessage);

	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT: // 캐릭터 선택

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
	// TODO : 공통으로 처리할거

	switch (messageType)
	{
	case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER: // 캐릭터 이동

		return recvMoveCharacter(pMessage);

	case en_PACKET_CS_GAME_REQ_STOP_CHARACTER: // 캐릭터 이동 멈춤

		return recvMoveStopCharacter(pMessage);

	case en_PACKET_CS_GAME_REQ_ATTACK1: // 캐릭터 공격 1

		return recvCharacterAttack1(pMessage);

	case en_PACKET_CS_GAME_REQ_ATTACK2: // 캐릭터 공격 2

		return recvCharacterAttack2(pMessage);

	case en_PACKET_CS_GAME_REQ_PICK: // 아이템 줍기

		return recvCristalPick(pMessage);

	case en_PACKET_CS_GAME_REQ_SIT: // 앉기

		return recvCharacterSit(pMessage);

	case en_PACKET_CS_GAME_REQ_PLAYER_RESTART: // 사망 후 재시작

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

		// 중복 로그인을 확인한다. FALSE 일 경우 현재 접속중이 아니니 다음 로직을 진행한다.
		if (isDuplicateLogin(&status) == TRUE)
		{
			status = dfGAME_LOGIN_FAIL;

			break;
		}

		// Account ID 및 Nick 조회
		if (mpGodDamnBug->GetIDWithNickFromAccountDB(mAccountNo, mPlayerID, player::PLAYER_STRING, mPlayerNick, player::PLAYER_STRING, &status) == FALSE)
		{
			break;
		}

		mbLoginFlag = TRUE;

		// DB 확인 후 캐릭터가 있을 경우 플레이어 셋팅을 하고 없을 경우 캐릭터 생성 status로 요청한다.
		if (getPlayerInfoFromGameDB() == TRUE)
		{
			status = dfGAME_LOGIN_OK;

			// AuthToGameFlag를 셋팅한다.
			SetAuthToGame();
		}
		else
		{
			status = dfGAME_LOGIN_NOCHARACTER;
		}

		{
			CSRWLockExclusive srwLock(&mpGodDamnBug->mPlayerMapLock);

			// PlayerMap 에 Insert 한다.
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

	// 플레이어 기본 리스폰 좌표 셋팅
	SetPlayerRespawnCoordinate();

	// 플레이어 정보를 셋팅합니다.
	SetPlayerInfo(mpGodDamnBug->GetObjectID(), characterType, mPosX, mPosY, mTileX, mTileY, rand() % 360, 0, mInitializeHP, 0, 1, 0, mInitializeDamage, mInitializeRecoveryHP);

	// 캐릭터 설정의 성공하였음을 송신
	sendCharacterSelectResponse(TRUE);

	// 캐릭터 선택하였음을 DB에 저장
	mpGodDamnBug->InsertCreateCharacterDBWriteJob(this);

	// AuthToGameFlag를 셋팅한다.
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

	// mbLoginFlag 가 FALSE인 상태에서 좌표에 대한 메시지를 처리해버리면 좌표가 바뀔 수 있기 때문에
	// 섹터에 대한 동기화가 틀어진다. 
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
	// 몬스터에 의해서 공격을 받기 때문에 이미 보내놓은 메시지가 늦게 처리되어 mbDieFlag가 TRUE가 될 수 있음
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

	// 공격은 중복 로그인으로 인한 동기화를 맞출 필요가 없음
	if (validateRecvCharacterAttack1(clientID) == FALSE)
	{
		return FALSE;
	}

	// 어택 모션 뿌리기
	sendAroundCharacterAttack1();

	CMonster* pVictimMonster;

	if (getAttack1VictimMonster(&pVictimMonster) == FALSE)
	{
		return TRUE;
	}

	// 데미지 뿌리기
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

	// 공격 모션 뿌리기
	sendAroundCharacterAttack2();

	CMonster* pVictimMonster;

	if (getAttack2VictimMonster(&pVictimMonster) == FALSE)
	{
		return TRUE;
	}

	// 데미지 뿌리기
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
//	// 공격 모션 뿌리기
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
//	// 데미지 이모션 뿌리기
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

	// 앉기 플래그로 변경
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


	// 정해진 캐릭터 타입의 범위를 벗어났다면 return FALSE
	// 특정 패킷을 보내고 끊는다는 건 의미가 없음 그냥 끊어버리자.
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

	// 삭제되는 섹터에 clientID 오브젝트 삭제 요청 보내기
	sendRemoveSectorDeleteMyCharacter(&removeSectorAround);

	// 삭제되는 섹터에 있는 오브젝트 삭제요청 보내기
	sendRemoveSectorDeleteObject(&removeSectorAround);

	// 추가되는 섹터에 pPlayer 캐릭터 생성 요청 보내기
	sendAddSectorCreateMyCharacter(&addSectorAround);

	// 추가되는 섹터에 있는 오브젝트 pPlayer한테 생성요청 보내기
	sendAddSectorCreateObject(&addSectorAround);

	return;
}

void CPlayer::sendRemoveSectorDeleteMyCharacter(stSectorAround* pRemoveSectorAround)
{
	CMessage* pMessage = CMessage::Alloc();

	// 오브젝트 삭제 패킷 만들기
	goddamnbug::PackingRemoveObject(mClientID, pMessage);

	for (INT count = 0; count < pRemoveSectorAround->count; ++count)
	{
		// 삭제되는 섹터에 전부 뿌리기
		mpGodDamnBug->SendPacketOneSector(pRemoveSectorAround->around[count].posX, pRemoveSectorAround->around[count].posY, nullptr, pMessage);
	}

	pMessage->Free();

	return;
}


void CPlayer::sendRemoveSectorDeleteObject(stSectorAround* pRemoveSectorAround)
{
	for (INT count = 0; count < pRemoveSectorAround->count; ++count)
	{
		// 삭제되는 영역의 플레이어 오브젝트 삭제
		for (const auto &iter : *mpGodDamnBug->FindPlayerSectorMap(pRemoveSectorAround->around[count].posX, pRemoveSectorAround->around[count].posY))
		{
			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingRemoveObject(iter.second->mClientID, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}


		// 삭제되는 영역의 몬스터 오브젝트 삭제
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

	// pPlayer 캐릭터 생성 패킷 만들기
	goddamnbug::PackingCreateOthreadCharacter(mClientID, mCharacterType, mPlayerNick, player::PLAYER_STRING * sizeof(WCHAR), mPosX, mPosY, mRotation, mLevel, FALSE, mbSitFlag, mbDieFlag, pMessage);

	for (INT count = 0; count < pAddSectorAround->count; ++count)
	{
		// 추가되는 섹터에 뿌리기
		mpGodDamnBug->SendPacketOneSector(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY, nullptr, pMessage);
	}

	pMessage->Free();

	return;
}

// 주변 섹터에 해당 오브젝트 삭제요청 보내기
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
		// 추가되는 섹터의 플레이어 생성요청
		for (const auto &iter : *mpGodDamnBug->FindPlayerSectorMap(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY))
		{
			CPlayer* pAddPlayer = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateOthreadCharacter(pAddPlayer->mClientID, pAddPlayer->mCharacterType, pAddPlayer->mPlayerNick, player::PLAYER_STRING * sizeof(WCHAR), pAddPlayer->mPosX, pAddPlayer->mPosY, pAddPlayer->mRotation, pAddPlayer->mLevel, FALSE, pAddPlayer->mbSitFlag, pAddPlayer->mbDieFlag, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}

		// 추가되는 섹터의 몬스터 생성요청
		for (const auto &iter : *mpGodDamnBug->FindMonsterSectorMap(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY))
		{
			CMonster* pMonster = iter.second;

			CMessage* pMessage = CMessage::Alloc();

			goddamnbug::PackingCreateMonster(pMonster->GetClientID(), pMonster->GetPosX(), pMonster->GetPosY(), pMonster->GetRotation(), FALSE, pMessage);

			SendPacket(pMessage);

			pMessage->Free();
		}

		// 추가되는 섹터의 크리스탈 생성요청
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

	// pVictimPlayer 까지 포함해서 패킷을 뿌린다.
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

	// pVictimPlayer 까지 포함해서 패킷을 뿌린다.
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

	// 플레이어 섹터 셋팅
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

		// 플레이어 죽음을 DB에 저장한다.
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

			// 플레이어 맵에서 제거한다.
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
	// 접속중인 상태에서 사망하여 나 자신을 삭제하라고 클라이언트들에게 알려줘야 함
	if (bObjectRemoveFlag == TRUE)
	{
		sendAroundRemoveObject(mCurSector.posX, mCurSector.posY, mClientID);

		mpGodDamnBug->ErasePlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID);
	}


	// 재시작을 위한 HP 및 좌표를 새로 할당 받는다.
	setRestartPlayerInfo();

	// 새로 셋팅한 좌표를 기준으로 섹터에 다시 넣어준다.
	mpGodDamnBug->InsertPlayerSectorMap(mCurSector.posX, mCurSector.posY, mClientID, this);

	// 플레이어 재시작 메시지 송신
	sendPlayerRestart();

	// 내 캐릭터 생성 메시지 송신
	sendCreateMyCharacter();

	// 내 캐릭터 생성하라고 주변에 뿌리기
	sendAroundRespawnCharacter();

	// 주변 다른 오브젝트 생성하라고 나한테 뿌리기
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
