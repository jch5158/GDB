#include "stdafx.h"


CMonster::CMonster(void)
	: mMonsterType(0)
	, mActivityArea(0)
	, mbDieFlag(FALSE)
	, mHP(0)
	, mDamage(0)
	, mpAttackedPlayer(nullptr)
	, mLastSufferTick(0)
	, mLastMoveTick(0)
	, mLastMoveForAttackTick(0)
	, mLastAttackTick(0)
	, mLastDieTick(0)
	, mpGodDamnBug(nullptr)
{}

CMonster::~CMonster(void){}

void CMonster::SetMonsterDefaultValue(INT initializeHP, INT initializeDamage)
{
	mInitializeHP = initializeHP;

	mInitializeDamage = initializeDamage;

	return;
}

CMonster* CMonster::Alloc(UINT64 clientID, INT monsterType, INT activityArea, CGodDamnBug *pGodDamnBug)
{
	CMonster* pMonster = mMonsterFreeList.Alloc();

	pMonster->setMonsterInfo(clientID, monsterType, activityArea);
	
	pMonster->mpGodDamnBug = pGodDamnBug;

	return pMonster;
}


void CMonster::Free(void)
{
	if (mMonsterFreeList.Free(this) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[Free] Monster Free is Failed");

		CCrashDump::Crash();
	}

	return;
}

void CMonster::setMonsterRespawnCoordinate(void)
{
	switch ((eMonsterActivityArea)mActivityArea)
	{
	case eMonsterActivityArea::MonsterArea1:

		mTileX = (X_MONSTER_RESPAWN_CENTER_1 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_1 - 16) + (rand() % 33);

		break;
	
	case eMonsterActivityArea::MonsterArea2:

		mTileX = (X_MONSTER_RESPAWN_CENTER_2 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_2 - 16) + (rand() % 33);

		break;
	
	case eMonsterActivityArea::MonsterArea3:

		mTileX = (X_MONSTER_RESPAWN_CENTER_3 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_3 - 16) + (rand() % 33);

		break;

	case eMonsterActivityArea::MonsterArea4:

		mTileX = (X_MONSTER_RESPAWN_CENTER_4 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_4 - 16) + (rand() % 33);

		break;
	
	case eMonsterActivityArea::MonsterArea5:

		mTileX = (X_MONSTER_RESPAWN_CENTER_5 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_5 - 16) + (rand() % 33);

		break;
	
	case eMonsterActivityArea::MonsterArea6:

		mTileX = (X_MONSTER_RESPAWN_CENTER_6 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_6 - 16) + (rand() % 33);

		break;
	
	case eMonsterActivityArea::MonsterArea7:

		mTileX = (X_MONSTER_RESPAWN_CENTER_7 - 16) + (rand() % 33);
		mTileY = (Y_MONSTER_RESPAWN_CENTER_7 - 16) + (rand() % 33);

		break;
	
	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[setMonsterRespawnCoordinate] mActivityArea Value Error : %d", mActivityArea);

		CCrashDump::Crash();

		break;
	}

	mPosX = X_BLOCK(mTileX);
	mPosY = Y_BLOCK(mTileY);

	mCurSector.posX = X_SECTOR(mTileX);
	mCurSector.posY = Y_SECTOR(mTileY);

	return;
}


void CMonster::setMonsterTypeStats(void)
{
	switch ((eMonsterType)mMonsterType)
	{
	case eMonsterType::Monster1:
		
		mHP = mInitializeHP;

		mDamage = mInitializeDamage;

		break;
	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[setMonsterTypeStats] monsterType Error : %d", mMonsterType);

		CCrashDump::Crash();

		break;
	}

	return;
}


void CMonster::setMonsterInfo(UINT64 clientID, INT monsterType, INT activityArea)
{
	mClientID = clientID;

	mMonsterType = monsterType;

	mActivityArea = activityArea;

	// Ÿ�Կ� ���� ���� ü��, �������� �����Ѵ�.
	setMonsterTypeStats();

	// ������ ������ ��ǥ�� �����Ѵ�.
	setMonsterRespawnCoordinate();

	// ���� ���͸� �������.
	goddamnbug::GetSector(mTileX, mTileY, &mCurSector);

	mbDeleteFlag = FALSE;

	mbDieFlag = FALSE;

	mRotation = rand() % 360;

	mpAttackedPlayer = nullptr;

	mLastMoveForAttackTick = 0;

	mLastSufferTick = 0;

	mLastMoveTick = 0;

	mLastAttackTick = 0;

	mLastDieTick = 0;

	return;
}



void CMonster::Update(void)
{
	revivalMonster();

	moveMonster();

	attackToAttackedPlayer();

	return;
}

BOOL CMonster::DeleteObject(void)
{
	if (mbDeleteFlag == TRUE)
	{
		Free();

		return TRUE;
	}

	return FALSE;
}

void CMonster::CalculatePlayerAttack(CPlayer* pAttackedPlayer, INT damage)
{
	mLastSufferTick = timeGetTime();

	// � Ŭ���̾�Ʈ�� �����ߴ��� Ȯ���Ѵ�.
	mpAttackedPlayer = pAttackedPlayer;

	mHP -= damage;

	if (mHP <= 0)
	{
		// �׾����� ������ �÷��̾� �ʱ�ȭ
		mHP = 0;

		mbDieFlag = TRUE;

		mLastDieTick = timeGetTime();

		mpGodDamnBug->EraseMonsterSectorMap(mCurSector.posX, mCurSector.posY, mClientID);

		sendAroundMonsterDie();

		mpGodDamnBug->CreateCristal(mPosX, mPosY, mTileX, mTileY, mCurSector);
	}

	return;
}

void CMonster::revivalMonster(void)
{
	if (mbDieFlag == FALSE)
	{
		return;
	}

	// ������ �ð� �ڿ� ���� ��Ȱ �ڿ� ���� ��Ȱ
	if (timeGetTime() - mLastDieTick < MONSTER_REVIVAL)
	{
		return;
	}


	// Ÿ�Կ� ���� ���� ü��, �������� �����Ѵ�.
	setMonsterTypeStats();

	// ������ ������ ��ǥ�� �����Ѵ�.
	setMonsterRespawnCoordinate();

	// ���� ���͸� �������.
	goddamnbug::GetSector(mTileX, mTileY, &mCurSector);

	mbDieFlag = FALSE;

	mRotation = rand() % 360;

	mpAttackedPlayer = nullptr;

	mLastMoveForAttackTick = 0;

	mLastSufferTick = 0;

	mLastMoveTick = 0;

	mLastAttackTick = 0;

	mLastDieTick = 0;

	mpGodDamnBug->InsertMonsterSectorMap(mCurSector.posX, mCurSector.posY, mClientID, this);

	sendAroundCreateMonster();

	return;
}

void CMonster::moveMonster(void)
{
	if (mbDieFlag == TRUE)
	{
		return;
	}
	
	if (timeGetTime() - mLastMoveTick < MONSTER_SPEED)
	{
		return;
	}

	FLOAT moveX;
	FLOAT moveY;

	INT moveTileX;
	INT moveTileY;

	INT rotation;
	
	if (mpAttackedPlayer != nullptr)
	{	
		if (mpAttackedPlayer->GetDieFlag() == TRUE || mpAttackedPlayer->GetDeleteFlag() == TRUE)
		{
			mpAttackedPlayer = nullptr;

			mLastMoveForAttackTick = 0;

			// ���� ������ ����.
			rotation = rand() % 360;

			// ������ �� ��ǥ�� ����.
			getMonsterMoveCoordinate(rotation, &moveX, &moveY);
		}
		else
		{
			// �����ߴ� �÷��̾ ����� ���Ϳ� �ִ��� Ȯ���Ѵ�.
			if (checkAttackedPlayerSector(mpAttackedPlayer) == FALSE)
			{
				return;
			}

			// ����� ���Ϳ� �ִ� �÷��̾�� �������� ������ ����.
			getRotationForAttackedPlayer(mpAttackedPlayer, &rotation);

			// ������ �� ��ǥ�� ����.
			getMonsterToAttackedCoordinate(mpAttackedPlayer, rotation, &moveX, &moveY);

			if (timeGetTime() - mLastSufferTick > MONSTER_RAGE_TIME)
			{
				// MONSTER_RAGE_TIME ����
				mpAttackedPlayer = nullptr;

				mLastMoveForAttackTick = 0;
			}
			else
			{
				mLastMoveForAttackTick = timeGetTime();
			}
		}
	}
	else
	{
		// ���� ������ ����.
		rotation = rand() % 360;

		// ������ �� ��ǥ�� ����.
		getMonsterMoveCoordinate(rotation, &moveX, &moveY);
	}


	moveTileX = (INT)X_TILE(moveX);
	moveTileY = (INT)Y_TILE(moveY);

	// ������ ����ٸ� ���� �����ӿ��� �����̼� ���� ���� �����δ�.
	if (chaeckMonsterMoveRange(moveTileX, moveTileY) == FALSE)
	{
		return;
	}

	mRotation = getGameRotation(rotation);

	mPosX = moveX;
	mPosY = moveY;

	mTileX = moveTileX;
	mTileY = moveTileY;

	// ���� �̵��� ���� �� 
	sendAroundMonsterMoveStart();

	if (updateMonsterSector() == TRUE)
	{
		// ���� ������ ��ȭ�� �־��ٸ� �ֺ� Ŭ���̾�Ʈ���� ��ȭ�� ������ �˷��ش�.
		sendAroundUpdateMonsterSector();
	}

	// ������ ó���� �Ϸ�Ǿ��ٸ� mLastMoveTick �� ���Ž����־� �̵��ӵ��� �����Ѵ�.
	mLastMoveTick = timeGetTime();

	return;
}



void CMonster::attackToAttackedPlayer(void)
{
	if (mbDieFlag == TRUE)
	{
		return;
	}

	if (mpAttackedPlayer == nullptr)
	{
		return;
	}

	do
	{
		if (mpAttackedPlayer->GetDieFlag() == TRUE)
		{
			mpAttackedPlayer = nullptr;

			mLastMoveForAttackTick = 0;

			break;
		}

		// �����̰� 1�ʵڿ� ������ �� �ִ�.
		if (timeGetTime() - mLastMoveForAttackTick < 1000 || mLastMoveForAttackTick == 0)
		{
			break;
		}

		if (timeGetTime() - mLastAttackTick < MONSTER_ATTACK_SPEED)
		{
			break;
		}

		if (checkAttackDistance(mpAttackedPlayer) == FALSE)
		{
			break;
		}

		// ���� �׼� �Ѹ���
		sendAroundMonsterAttackAction();

		// ������ ó��
		sendAroundMonsterAttackDamage(mpAttackedPlayer);

		mpAttackedPlayer->UpdateAttackDamage(mDamage);

		mLastAttackTick = timeGetTime();

	} while (0);

	return;
}

BOOL CMonster::checkAttackDistance(CPlayer* pPlayer)
{
	stCollisionBox collisionBox;
	goddamnbug::GetAttackCollisionBox(mRotation, mTileX, mTileY, MONSTER_ATTACK_RANGE, &collisionBox);

	if (pPlayer->GetTileX() >= collisionBox.pointOne.tileX && pPlayer->GetTileY() >= collisionBox.pointOne.tileY
		&& pPlayer->GetTileX() <= collisionBox.pointTwo.tileX && pPlayer->GetTileY() <= collisionBox.pointTwo.tileY)
	{
		return TRUE;
	}

	return FALSE;
}




BOOL CMonster::chaeckMonsterMoveRange(INT moveTileX, INT moveTileY)
{

	switch ((eMonsterActivityArea)mActivityArea)
	{
	case eMonsterActivityArea::MonsterArea1:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_1 + MONSTER_RESPAWN_RANGE_1 || moveTileX < X_MONSTER_RESPAWN_CENTER_1 - MONSTER_RESPAWN_RANGE_1
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_1 + MONSTER_RESPAWN_RANGE_1 || moveTileY < Y_MONSTER_RESPAWN_CENTER_1 - MONSTER_RESPAWN_RANGE_1)		
		{
			return FALSE;
		}	

		break;
	case eMonsterActivityArea::MonsterArea2:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_2 + MONSTER_RESPAWN_RANGE_2 || moveTileX < X_MONSTER_RESPAWN_CENTER_2 - MONSTER_RESPAWN_RANGE_2
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_2 + MONSTER_RESPAWN_RANGE_2 || moveTileY < Y_MONSTER_RESPAWN_CENTER_2 - MONSTER_RESPAWN_RANGE_2)
		{
			return FALSE;
		}

		break;
	case eMonsterActivityArea::MonsterArea3:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_3 + MONSTER_RESPAWN_RANGE_3 || moveTileX < X_MONSTER_RESPAWN_CENTER_3 - MONSTER_RESPAWN_RANGE_3
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_3 + MONSTER_RESPAWN_RANGE_3 ||  moveTileY < Y_MONSTER_RESPAWN_CENTER_3 - MONSTER_RESPAWN_RANGE_3)
		{
			return FALSE;
		}

		break;
	case eMonsterActivityArea::MonsterArea4:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_4 + MONSTER_RESPAWN_RANGE_4 || moveTileX < X_MONSTER_RESPAWN_CENTER_4 - MONSTER_RESPAWN_RANGE_4
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_4 + MONSTER_RESPAWN_RANGE_4 || moveTileY < Y_MONSTER_RESPAWN_CENTER_4 - MONSTER_RESPAWN_RANGE_4)
		{
			return FALSE;
		}


		break;
	case eMonsterActivityArea::MonsterArea5:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_5 + MONSTER_RESPAWN_RANGE_5 || moveTileX < X_MONSTER_RESPAWN_CENTER_5 - MONSTER_RESPAWN_RANGE_5
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_5 + MONSTER_RESPAWN_RANGE_5 || moveTileY < Y_MONSTER_RESPAWN_CENTER_5 - MONSTER_RESPAWN_RANGE_5)
		{
			return FALSE;
		}

		break;
	case eMonsterActivityArea::MonsterArea6:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_6 + MONSTER_RESPAWN_RANGE_6 || moveTileX < X_MONSTER_RESPAWN_CENTER_6 - MONSTER_RESPAWN_RANGE_6
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_6 + MONSTER_RESPAWN_RANGE_6 || moveTileY < Y_MONSTER_RESPAWN_CENTER_6 - MONSTER_RESPAWN_RANGE_6)
		{
			return FALSE;
		}

		break;
	case eMonsterActivityArea::MonsterArea7:

		if (moveTileX > X_MONSTER_RESPAWN_CENTER_7 + MONSTER_RESPAWN_RANGE_7 || moveTileX < X_MONSTER_RESPAWN_CENTER_7 - MONSTER_RESPAWN_RANGE_7
			|| moveTileY > Y_MONSTER_RESPAWN_CENTER_7 + MONSTER_RESPAWN_RANGE_7 || moveTileY < Y_MONSTER_RESPAWN_CENTER_7 - MONSTER_RESPAWN_RANGE_7)
		{
			return FALSE;
		}

		break;
	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[chaeckMonsterMoveRange] mActivityArea Value Error : %d", mActivityArea);

		CCrashDump::Crash();

		break;
	}


	return TRUE;
}


INT CMonster::getGameRotation(INT rotation)
{
	INT mathRotation = 90 - rotation;

	if (mathRotation < 0)
	{
		mathRotation += 360;
	}

	return mathRotation;
}

void CMonster::getMonsterMoveCoordinate(INT rotation, FLOAT* moveX, FLOAT* moveY)
{
	FLOAT radian = (FLOAT)DEGREE_TO_RADIAN(rotation);

	// INT�� Ÿ���� Ÿ���� �ƴ� ���� �������� �̵��Ÿ��� ����Ѵ�.
	FLOAT blockMoveRange = (FLOAT)MONSTER_TILE_MOVE_RANGE / 2.0f;

	// ������ ���� ���̸� ����.
	FLOAT vectorLength = sqrtf((blockMoveRange * blockMoveRange) * 2);

	// �̵� ���� �� X ��ǥ�� ��´�.
	*moveX = mPosX + cosf(radian) * vectorLength;

	// �̵� ���� �� Y ��ǥ�� ��´�.
	*moveY = mPosY + sinf(radian) * vectorLength;

	return;
}

void CMonster::getMonsterToAttackedCoordinate(CPlayer* pPlayer, INT rotation, FLOAT* pMoveX, FLOAT* pMoveY)
{
	FLOAT radian = (FLOAT)DEGREE_TO_RADIAN(rotation);

	// INT�� Ÿ���� Ÿ���� �ƴ� ���� �������� �̵��Ÿ��� ����Ѵ�.
	FLOAT blockMoveRange = (FLOAT)MONSTER_TILE_MOVE_RANGE / 2.0f;

	// ������ ���� ���̸� ����.
	FLOAT vectorLength = sqrtf((blockMoveRange * blockMoveRange) * 2);

	FLOAT distanceX = cosf(radian) * vectorLength;
	
	FLOAT monsterX = mPosX + distanceX;
	if (distanceX <= 0.0f)
	{
		// �Ѿ�� �������� �ʴ´�.
		if (pPlayer->GetPosX() > monsterX)
		{
			*pMoveX = mPosX;
		}
		else
		{
			*pMoveX = monsterX;
		}
	}
	else
	{
		if (pPlayer->GetPosX() < monsterX)
		{
			*pMoveX = mPosX;
		}
		else
		{
			*pMoveX = monsterX;
		}
	}

	FLOAT distanceY = sinf(radian) * vectorLength;

	// �̵� ���� �� Y ��ǥ�� ��´�.
	FLOAT monsterY = mPosY + distanceY;
	if (distanceY <= 0.0f)
	{
		if (pPlayer->GetPosY() > monsterY)
		{
			*pMoveY = mPosY;
		}
		else
		{
			*pMoveY = monsterY;
		}
	}
	else
	{
		if (pPlayer->GetPosY() < monsterY)
		{
			*pMoveY = mPosY;
		}
		else
		{
			*pMoveY = monsterY;
		}
	}

	return;
}




BOOL CMonster::updateMonsterSector(void)
{
	stSector sector;

	goddamnbug::GetSector(mTileX, mTileY, &sector);

	if (mCurSector.posX == sector.posX && mCurSector.posY == sector.posY)
	{
		return FALSE;
	}

	mOldSector = mCurSector;

	mpGodDamnBug->EraseMonsterSectorMap(mOldSector.posX, mOldSector.posY, mClientID);

	mCurSector = sector;

	mpGodDamnBug->InsertMonsterSectorMap(sector.posX, sector.posY, mClientID, this);

	return TRUE;
}

void CMonster::sendAroundCreateMonster(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCreateMonster(mClientID, mPosX, mPosY, mRotation, TRUE, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, nullptr, pMessage);

	pMessage->Free();

	return;
}

void CMonster::sendAroundUpdateMonsterSector(void)
{
	stSectorAround removeSectorAround;
	stSectorAround addSectorAround;

	goddamnbug::GetUpdateSectorAround(mOldSector, mCurSector, &removeSectorAround, &addSectorAround);

	sendRemoveAroundDeleteObject(&removeSectorAround);

	sendAddAroundCreateObject(&addSectorAround);

	return;
}


void CMonster::sendRemoveAroundDeleteObject(stSectorAround* pRemoveSectorAround)
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

void CMonster::sendAddAroundCreateObject(stSectorAround* pAddSectorAround)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCreateMonster(mClientID, mPosX, mPosY, mRotation, FALSE, pMessage);

	for (INT count = 0; count < pAddSectorAround->count; ++count)
	{
		mpGodDamnBug->SendPacketOneSector(pAddSectorAround->around[count].posX, pAddSectorAround->around[count].posY, nullptr, pMessage);
	}

	pMessage->Free();

	return;
}

void CMonster::sendAroundMonsterMoveStart(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingMonsterMoveStart(mClientID, mPosX, mPosY, mRotation, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, nullptr, pMessage);

	pMessage->Free();

	return;
}

void CMonster::sendAroundMonsterDie(void)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingMonsterDie(mClientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, nullptr, pMessage);

	pMessage->Free();

	return;
}

void CMonster::sendAroundMonsterAttackAction(void)
{
	CMessage* pMessage = CMessage::Alloc();
	
	goddamnbug::PackingMonsterAttackAction(mClientID, pMessage);

	mpGodDamnBug->SendPacketAroundSector(mCurSector.posX, mCurSector.posY, nullptr, pMessage);
	

	pMessage->Free();

	return;
}

void CMonster::sendAroundMonsterAttackDamage(CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingAttackDamage(mClientID, pPlayer->GetClientID(), mDamage, pMessage);

	mpGodDamnBug->SendPacketAroundSector(pPlayer->GetCurSectorX(), pPlayer->GetCurSectorY(), nullptr, pMessage);

	pMessage->Free();

	return;
}


BOOL CMonster::checkAttackedPlayerSector(CPlayer* pPlayer)
{
	if (abs(pPlayer->GetCurSectorX() - mCurSector.posX) < 2 && abs(pPlayer->GetCurSectorY() - mCurSector.posY) < 2)
	{
		return TRUE;
	}

	return FALSE;
}


void CMonster::getRotationForAttackedPlayer(CPlayer* pPlayer, INT* pRotation)
{	
	FLOAT radian = atan2((FLOAT)(pPlayer->GetPosY() - mPosY), (FLOAT)(pPlayer->GetPosX() - mPosX));

	FLOAT rotation = RADIAN_TO_DEGREE(radian);
	if (rotation < 0)
	{
		rotation = 360 + rotation;
	}

	*pRotation = (INT)rotation;

	return;
}