#include "stdafx.h"

CGodDamnBug::CGodDamnBug(void)
	: mObjectID(0)
	, mDBWriteThreadTPS(0)
	, mbDBWriteThreadFlag(FALSE)
	, mDBWriteThreadID(0)
	, mpLanLoginClient(nullptr)
	, mDBWriteThreadHandle(INVALID_HANDLE_VALUE)
	, mDBWriteJobEvent(CreateEvent(NULL, FALSE, FALSE, nullptr))
	, mLoginRequestCount(0)
	, mPlayerMapLock{ 0, }
	, mRedisConnector()
	, mDBConnector()
	, mDBWriteJobQueue()
	, mPlayerMap()
	, mPlayerSectorMap()
	, mMonsterSectorMap()
	, mCristalSectorMap()
{
	InitializeSRWLock(&mPlayerMapLock);

}

CGodDamnBug::~CGodDamnBug(void)
{
	CloseHandle(mDBWriteJobEvent);
}



CRedisConnector* CGodDamnBug::GetRedisConnector(void)
{
	return &mRedisConnector;
}


CTLSDBConnector* CGodDamnBug::GetDBConnector(void)
{
	return &mDBConnector;
}

CLanLoginClient* CGodDamnBug::GetLanLoginClient(void) const
{
	return mpLanLoginClient;
}

LONG CGodDamnBug::GetDBWriteThreadTPS(void) const
{
	return mDBWriteThreadTPS;
}

DWORD CGodDamnBug::GetDBWriteThreadQueueSize(void)
{
	return mDBWriteJobQueue.GetUseSize();
}



BOOL CGodDamnBug::InsertPlayerMap(UINT64 accountNo, CPlayer* pPlayer)
{
	return mPlayerMap.insert(std::make_pair(accountNo, pPlayer)).second;
}


BOOL CGodDamnBug::ErasePlayerMap(UINT64 accountNo)
{
	return (BOOL)mPlayerMap.erase(accountNo);
}

CPlayer* CGodDamnBug::FindPlayerFromPlayerMap(UINT64 accountNo)
{
	auto iter = mPlayerMap.find(accountNo);
	if (iter == mPlayerMap.end())
	{
		return nullptr;
	}

	return iter->second;
}


BOOL CGodDamnBug::InsertPlayerSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID, CPlayer* pPlayer)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[InsertPlayerSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}	

	return mPlayerSectorMap[sectorPosY][sectorPosX].insert(std::make_pair(clientID, pPlayer)).second;
}


BOOL CGodDamnBug::ErasePlayerSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[ErasePlayerSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return (BOOL)mPlayerSectorMap[sectorPosY][sectorPosX].erase(clientID);
}


std::unordered_map<UINT64, CPlayer*>* CGodDamnBug::FindPlayerSectorMap(INT sectorPosX, INT sectorPosY)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[FindPlayerSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}



	return &mPlayerSectorMap[sectorPosY][sectorPosX];
}



BOOL CGodDamnBug::InsertCristalSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID, CCristal* pCristal)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[InsertCristalSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return mCristalSectorMap[sectorPosY][sectorPosX].insert(std::make_pair(clientID, pCristal)).second;
}


BOOL CGodDamnBug::EraseCristalSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[EraseCristalSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return (BOOL)mCristalSectorMap[sectorPosY][sectorPosX].erase(clientID);
}

std::unordered_map<UINT64, CCristal*>* CGodDamnBug::FindCristalSectorMap(INT sectorPosX, INT sectorPosY)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[FindCristalSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return &mCristalSectorMap[sectorPosY][sectorPosX];
}



BOOL CGodDamnBug::InsertMonsterSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID, CMonster* pMonster)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[InsertMonsterSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return mMonsterSectorMap[sectorPosY][sectorPosX].insert(std::make_pair(clientID, pMonster)).second;
}

BOOL CGodDamnBug::EraseMonsterSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[EraseMonsterSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return (BOOL)mMonsterSectorMap[sectorPosY][sectorPosX].erase(clientID);
}

std::unordered_map<UINT64, CMonster*>* CGodDamnBug::FindMonsterSectorMap(INT sectorPosX, INT sectorPosY)
{
	if (sectorPosX >= X_SECTOR_RANGE || sectorPosY >= Y_SECTOR_RANGE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[FindMonsterSectorMap] SectorPosX : %d, SectorPosY : %d", sectorPosX, sectorPosY);

		CCrashDump::Crash();

		return FALSE;
	}

	return &mMonsterSectorMap[sectorPosY][sectorPosX];
}







BOOL CGodDamnBug::InsertAuthReleasePlayerMap(UINT64 accountNo, CPlayer* pPlayer)
{
	return mAuthReleasePlayerMap.insert(std::make_pair(accountNo, pPlayer)).second;	
}


BOOL CGodDamnBug::EraseAuthReleasePlayerMap(UINT64 accountNo)
{
	return (BOOL)mAuthReleasePlayerMap.erase(accountNo);
}


CPlayer* CGodDamnBug::FindPlayerFromAuthReleasePlayerMap(UINT64 accountNo)
{
	auto iter = mAuthReleasePlayerMap.find(accountNo);
	if (iter == mAuthReleasePlayerMap.end())
	{
		return nullptr;
	}

	return iter->second;
}





void CGodDamnBug::SetLoginClient(CLanLoginClient* pLanLoginClient)
{
	mpLanLoginClient = pLanLoginClient;

	return;
}

UINT64 CGodDamnBug::GetObjectID(void)
{
	return InterlockedIncrement(&mObjectID);
}

void CGodDamnBug::SendPacketOneSector(INT sectorPosX, INT sectorPosY, CPlayer* pExceptionPlayer, CMessage* pMessage)
{
	for (auto iter : mPlayerSectorMap[sectorPosY][sectorPosX])
	{
		if (iter.second != pExceptionPlayer)
		{
			iter.second->SendPacket(pMessage);
		}
	}

	return;
}

void CGodDamnBug::SendPacketAroundSector(INT sectorPosX, INT sectorPosY, CPlayer* pExceptionPlayer, CMessage* pMessage)
{
	stSectorAround sectorAround;

	goddamnbug::GetSectorAround(sectorPosX, sectorPosY, &sectorAround);

	for (INT count = 0; count < sectorAround.count; ++count)
	{
		SendPacketOneSector(sectorAround.around[count].posX, sectorAround.around[count].posY, pExceptionPlayer, pMessage);
	}

	return;
}



bool CGodDamnBug::OnStart(void)
{
	return setupDBWriteThread();
}

void CGodDamnBug::OnStartAuthenticThread(void)
{
	mRedisConnector.Connect();

	connectDB();

	// 플레이어 기본 스탯 설정
	setPlayerStats();
	
	return;
}

void CGodDamnBug::OnStartUpdateThread(void)
{
	connectDB();

	// 몬스터 기본 스탯 설정
	setMonsterStats();

	// 크리스탈 금액 설정
	setCristalAmount();

	setMonsters();

	return;
}

// accept 직후 바로 호출
bool CGodDamnBug::OnConnectionRequest(WCHAR* userIP, WORD userPort)
{

	return TRUE;
}

void CGodDamnBug::OnAuthUpdate(void)
{
	// AuthThread 에서 끊긴 플레이어 정리
	authReleasePlayers();

	return;
}

// UpdateThread 에서 켄텐츠 처리용으로 1 Loop당 처리하는 함수이다.
void CGodDamnBug::OnGameUpdate(void)
{	
	CObjectManager::GetInstance()->Update();

	return;
}

// Player 와 Session 매핑을 위한 오버라이딩 함수
void CGodDamnBug::OnAssociateSessionWithPlayer(CSession** pSession)
{
	*pSession = new CPlayer;
	
	((CPlayer*)*pSession)->SetGodDamnBugPtr(this);

	return;
}

void CGodDamnBug::OnReleaseSessionWithPlayer(CSession* pSession)
{
	delete pSession;

	return;
}



void CGodDamnBug::OnCloseAuthenticThread(void)
{
	mRedisConnector.Disconnect();

	disconnectDB();

	return;
}

void CGodDamnBug::OnCloseUpdateThread(void)
{
	disconnectDB();

	return;
}

void CGodDamnBug::OnStop(void)
{
	closeDBWriteThread();

	return;
}


void CGodDamnBug::connectDB(void)
{
	if (mDBConnector.Connect((WCHAR*)L"127.0.0.1", 10950, (WCHAR*)L"gamedb", (WCHAR*)L"chanhun", (WCHAR*)L"Cksgns123$") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[OnStartAuthenticThread] AccountDB Connect Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	return;
}

void CGodDamnBug::disconnectDB(void)
{
	mDBConnector.Disconnect();

	return;
}


void CGodDamnBug::setPlayerStats(void)
{
	INT damage;
	INT HP;
	INT recoveryHP;

	getPlayerStatsFromGameDB(&damage, &HP, &recoveryHP);

	CPlayer::SetPlayerDefaultValue(damage, HP, recoveryHP);

	return;
}


void CGodDamnBug::getPlayerStatsFromGameDB(INT* pDamage, INT* pHP, INT* pRecoveryHP)
{
REQUERY:

	if (mDBConnector.Query((WCHAR*)L"SELECT * FROM data_player;") == FALSE)
	{
		if (mDBConnector.CheckReconnectErrorCode() == TRUE)
		{
			mDBConnector.Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[getDBPlayerDefaultValue] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	mDBConnector.StoreResult();

	MYSQL_ROW mysqlRow = mDBConnector.FetchRow();
	if (mysqlRow == nullptr)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[getDBPlayerDefaultValue] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();

		return;
	}

	*pDamage = atoi(mysqlRow[0]);
	*pHP = atoi(mysqlRow[1]);
	*pRecoveryHP = atoi(mysqlRow[2]);

	mDBConnector.FreeResult();

	return;
}



void CGodDamnBug::setMonsterStats(void)
{
	INT HP;
	INT damage;

	getMonsterStatsFromGameDB((INT)eMonsterType::Monster1, &HP, &damage);

	CMonster::SetMonsterDefaultValue(HP, damage);

	return;
}



void CGodDamnBug::getMonsterStatsFromGameDB(INT monsterType, INT* pHP, INT* pDamage)
{
REQUERY:

	if (mDBConnector.Query((WCHAR*)L"SELECT `hp`,`damage` FROM data_monster WHERE monster_type = %d;", monsterType) == FALSE)
	{
		if (mDBConnector.CheckReconnectErrorCode() == TRUE)
		{
			mDBConnector.Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[getMonsterStatsFromGameDB] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	mDBConnector.StoreResult();


	MYSQL_ROW mysqlRow = mDBConnector.FetchRow();
	if (mysqlRow == nullptr)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[getMonsterStatsFromGameDB] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();

		return;
	}

	*pHP = atoi(mysqlRow[0]);
	*pDamage = atoi(mysqlRow[1]);

	mDBConnector.FreeResult();

	return;
}





void CGodDamnBug::setCristalAmount(void)
{
	INT typeAmountArray[3];

	getCristalAmountFromGameDB(typeAmountArray);

	CCristal::SetCristalTypeAmount(typeAmountArray);

	return;
}

void CGodDamnBug::getCristalAmountFromGameDB(INT typeAmountArray[])
{

	for (INT count = (INT)eCristalType::Cristal1; count <= (INT)eCristalType::Cristal3; ++count)
	{

	REQUERY:

		if (mDBConnector.Query((WCHAR*)L"SELECT `amount` FROM data_cristal where cristal_type = %d;", count) == FALSE)
		{
			if (mDBConnector.CheckReconnectErrorCode() == TRUE)
			{
				mDBConnector.Reconnect();

				goto REQUERY;
			}

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[getDBPlayerDefaultValue] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

			CCrashDump::Crash();
		}

		mDBConnector.StoreResult();

		MYSQL_ROW mysqlRow = mDBConnector.FetchRow();
		if (mysqlRow == nullptr)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[getDBPlayerDefaultValue] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

			CCrashDump::Crash();

			return;
		}

		typeAmountArray[count - 1] = atoi(mysqlRow[0]);

		mDBConnector.FreeResult();
	}

	return;
}





BOOL CGodDamnBug::setupDBWriteThread(void)
{
	mbDBWriteThreadFlag = TRUE;

	mDBWriteThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteDBWriteThread, this, NULL, (UINT*)&mDBWriteThreadID);
	if (mDBWriteThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[setupDBWriteThread] _beginthreadex Error Code : %d", GetLastError());

		return FALSE;
	}

	return TRUE;
}

void CGodDamnBug::closeDBWriteThread(void)
{
	mbDBWriteThreadFlag = FALSE;

	if (WaitForSingleObject(mDBWriteThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[mDBWriteThreadHandle] WaitForSingObject Error Code : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mDBWriteThreadHandle);

	return;
}

DWORD WINAPI CGodDamnBug::ExecuteDBWriteThread(void* pParam)
{
	CGodDamnBug* pGodDamnBug = (CGodDamnBug*)pParam;

	pGodDamnBug->DBWriteThread();

	return 1;
}

void CGodDamnBug::DBWriteThread(void)
{
	connectDB();

	for (;;)
	{
		CDBWriteJob* pDBWriteJob;

		if (mDBWriteJobQueue.Dequeue(&pDBWriteJob) == FALSE)
		{
			if (mbDBWriteThreadFlag == FALSE)
			{
				break;
			}

			//
			if (WaitForSingleObject(mDBWriteJobEvent, INFINITE) != WAIT_OBJECT_0)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[DBWriteThread] WaitForSingObject Error Code : %d", GetLastError());

				CCrashDump::Crash();
			}

			continue;
		}

		pDBWriteJob->DBWrite();

		pDBWriteJob->DecrementDBWriteCount();

		delete pDBWriteJob;

		InterlockedIncrement(&mDBWriteThreadTPS);
	}

	disconnectDB();

	return;
}



BOOL CGodDamnBug::GetIDWithNickFromAccountDB(UINT64 accountNo, WCHAR* pPlayerID, DWORD cbPlayerID, WCHAR* pPlayerNick, DWORD cbPlayerNick, BYTE* pStatus)
{
REQUERY:

	if (mDBConnector.Query((WCHAR*)L"SELECT * FROM `accountdb`.`v_account` WHERE `accountno` = '%lld';", accountNo) == FALSE)
	{
		if (mDBConnector.CheckReconnectErrorCode() == TRUE)
		{
			mDBConnector.Reconnect();

			goto REQUERY;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[SetDBPlayerInfo] MySQL Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	mDBConnector.StoreResult();

	MYSQL_ROW mysqlRow = mDBConnector.FetchRow();
	if (mysqlRow == nullptr)
	{
		*pStatus = dfLOGIN_STATUS_ACCOUNT_MISS;

		return FALSE;
	}

	if (mysqlRow[3] == nullptr)
	{
		*pStatus = dfLOGIN_STATUS_STATUS_MISS;

		return FALSE;
	}

	MultiByteToWideChar(CP_ACP, 0, mysqlRow[1], (INT)strlen(mysqlRow[1]) + 1, pPlayerID, cbPlayerID);

	MultiByteToWideChar(CP_ACP, 0, mysqlRow[2], (INT)strlen(mysqlRow[2]) + 1, pPlayerNick, cbPlayerNick);

	mDBConnector.FreeResult();

	return TRUE;
}



void goddamnbug::GetAttackCollisionBox(INT rotation, INT tileX, INT tileY, INT collisionRange, stCollisionBox* pCollisionBox)
{	
	switch (rotation / 90)
	{
	case 0:

		pCollisionBox->pointOne.tileX = tileX;
		pCollisionBox->pointOne.tileY = tileY;
		pCollisionBox->pointTwo.tileX = tileX + collisionRange;
		pCollisionBox->pointTwo.tileY = tileY + collisionRange;

		break;

	case 1:

		pCollisionBox->pointOne.tileX = tileX;
		pCollisionBox->pointOne.tileY = tileY - collisionRange;
		pCollisionBox->pointTwo.tileX = tileX + collisionRange;
		pCollisionBox->pointTwo.tileY = tileY;

		break;

	case 2:

		pCollisionBox->pointOne.tileX = tileX - collisionRange;
		pCollisionBox->pointOne.tileY = tileY - collisionRange;
		pCollisionBox->pointTwo.tileX = tileX;
		pCollisionBox->pointTwo.tileY = tileY;

		break;

	case 3:

		pCollisionBox->pointOne.tileX = tileX - collisionRange;
		pCollisionBox->pointOne.tileY = tileY;
		pCollisionBox->pointTwo.tileX = tileX;
		pCollisionBox->pointTwo.tileY = tileY + collisionRange;

		break;

	case 4:

		pCollisionBox->pointOne.tileX = tileX;
		pCollisionBox->pointOne.tileY = tileY;
		pCollisionBox->pointTwo.tileX = tileX + collisionRange;
		pCollisionBox->pointTwo.tileY = tileY + collisionRange;

		break;

	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[GetAttackCollisionBox] Rotation : %d", rotation);

		// 패킷 검증을 하지 못하였음을 의미
		CCrashDump::Crash();
	
		break;
	}


	return;
}


void goddamnbug::GetAttackSectorAround16(INT rotation, stSector curSector, stSectorAround16* pAttackSectorAround16)
{
	// 방향에 따라 
	switch (rotation / 90)
	{
	case 0:

		goddamnbug::GetSectorAround16(curSector.posX - 1, curSector.posY - 1, pAttackSectorAround16);

		break;

	case 1:

		goddamnbug::GetSectorAround16(curSector.posX - 1, curSector.posY - 2, pAttackSectorAround16);

		break;

	case 2:

		goddamnbug::GetSectorAround16(curSector.posX - 2, curSector.posY - 2, pAttackSectorAround16);

		break;

	case 3:

		goddamnbug::GetSectorAround16(curSector.posX - 2, curSector.posY - 1, pAttackSectorAround16);

		break;

	case 4:

		goddamnbug::GetSectorAround16(curSector.posX - 1, curSector.posY - 1, pAttackSectorAround16);

		break;

	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CGodDamnBug", L"[GetAttackSectorAround16] Rotation : %d", rotation);

		// 패킷 검증을 하지 못하였음을 의미
		CCrashDump::Crash();

		break;
	}

	return;
}


void CGodDamnBug::CreateCristal(FLOAT posX, FLOAT posY, INT tileX, INT tileY, stSector curSector)
{
	CCristal* pCristal = CCristal::Alloc(GetObjectID(), posX, posY, tileX, tileY, curSector, this);

	// 생성된 크리스탈은 오브젝트 매니저에 Insert
	CObjectManager::GetInstance()->InsertObject(pCristal);

	InsertCristalSectorMap(curSector.posX, curSector.posY, pCristal->GetClientID(), pCristal);

	SendAroundCreateCristal(pCristal);

	return;
}

void CGodDamnBug::RemoveCristal(CCristal* pCristal)
{
	EraseCristalSectorMap(pCristal->GetCurSectorX(), pCristal->GetCurSectorY(), pCristal->GetClientID());

	pCristal->SetDeleteFlag(TRUE);

	return;
}

void CGodDamnBug::InsertLoginDBWriteJob(BYTE status, CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CLoginDBWriteJob;

	((CLoginDBWriteJob*)pDBWriteJob)->SetLoginDBWriteJob(&mDBConnector, pPlayer, status);
	
	pPlayer->IncrementDBWriteCount();
	
	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}


void CGodDamnBug::InsertLogoutDBWriteJob(BYTE status, CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CLogoutDBWriteJob;

	((CLogoutDBWriteJob*)pDBWriteJob)->SetLogoutDBWriteJob(&mDBConnector, pPlayer, status);

	pPlayer->IncrementDBWriteCount();

	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}


void CGodDamnBug::InsertDieDBWriteJob(CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CDieDBWriteJob;

	((CDieDBWriteJob*)pDBWriteJob)->SetDieDBWriteJob(&mDBConnector, pPlayer);

	pPlayer->IncrementDBWriteCount();

	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}


void CGodDamnBug::InsertCreateCharacterDBWriteJob(CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CCreateCharacterDBWriteJob;

	((CCreateCharacterDBWriteJob*)pDBWriteJob)->SetCreateCharacterDBWriteJob(&mDBConnector, pPlayer);

	pPlayer->IncrementDBWriteCount();

	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}

void CGodDamnBug::InsertRestartDBWriteJob(CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CRestartDBWriteJob;

	((CRestartDBWriteJob*)pDBWriteJob)->SetRestartDBWriteJob(&mDBConnector,pPlayer);

	pPlayer->IncrementDBWriteCount();

	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}


void CGodDamnBug::InsertCristalPickDBWriteJob(INT getCristalCount, CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CCristalPickDBWriteJob;

	((CCristalPickDBWriteJob*)pDBWriteJob)->SetCristalPickDBWriteJob(&mDBConnector, pPlayer, getCristalCount);

	pPlayer->IncrementDBWriteCount();

	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}

void CGodDamnBug::InsertHPRecoveryDBWriteJob(CPlayer* pPlayer)
{
	CDBWriteJob* pDBWriteJob = new CHPRecoveryDBWriteJob;

	((CHPRecoveryDBWriteJob*)pDBWriteJob)->SetHPRecoveryDBWriteJob(&mDBConnector, pPlayer);
	
	pPlayer->IncrementDBWriteCount();

	mDBWriteJobQueue.Enqueue(pDBWriteJob);

	SetEvent(mDBWriteJobEvent);

	return;
}


void CGodDamnBug::InitializeDBWriteThreadTPS(void)
{
	InterlockedExchange(&mDBWriteThreadTPS, 0);

	return;
}


BOOL CGodDamnBug::CompareSessionToken(UINT64 accountNo, CHAR* pSessionToken)
{
	if (accountNo > 999999)	
	{
		if (mRedisConnector.CompareToken(accountNo, pSessionToken) == FALSE)
			return false;
	}

	return TRUE;
}


void goddamnbug::GetSector(INT tileX, INT tileY, stSector* pSector)
{
	pSector->posX = X_SECTOR(tileX);

	pSector->posY = Y_SECTOR(tileY);

	return;
}


void goddamnbug::GetSectorAround(INT sectorPosX, INT sectorPosY, stSectorAround* pSectorAround)
{
	sectorPosX -= 1;
	sectorPosY -= 1;

	INT sectorAroundCount = 0;

	for (INT countY = 0; countY < 3; ++countY)
	{
		if (sectorPosY + countY < 0 || sectorPosY + countY >= Y_SECTOR_RANGE)
		{
			continue;
		}

		for (INT countX = 0; countX < 3; ++countX)
		{
			if (sectorPosX + countX < 0 || sectorPosX + countX >= X_SECTOR_RANGE)
			{
				continue;
			}

			pSectorAround->around[sectorAroundCount].posY = sectorPosY + countY;
			pSectorAround->around[sectorAroundCount].posX = sectorPosX + countX;

			++sectorAroundCount;
		}
	}

	pSectorAround->count = sectorAroundCount;

	return;
}

void goddamnbug::GetSectorAround16(INT sectorPosX, INT sectorPosY, stSectorAround16* pSectorAround16)
{

	INT sectorAroundCount = 0;

	for (INT countY = 0; countY < 4; ++countY)
	{
		if (sectorPosY + countY < 0 || sectorPosY + countY >= Y_SECTOR_RANGE)
		{
			continue;
		}

		for (INT countX = 0; countX < 4; ++countX)
		{
			if (sectorPosX + countX < 0 || sectorPosX + countX >= X_SECTOR_RANGE)
			{
				continue;
			}

			pSectorAround16->around[sectorAroundCount].posY = sectorPosY + countY;
			pSectorAround16->around[sectorAroundCount].posX = sectorPosX + countX;

			++sectorAroundCount;
		}
	}

	pSectorAround16->count = sectorAroundCount;

	return;
}



void goddamnbug::GetUpdateSectorAround(stSector oldSector, stSector curSector, stSectorAround* pRemoveSectorAround, stSectorAround* pAddSectorAround)
{
	BOOL bFindFlag;

	INT removeSectorCount = 0;
	INT addSectorCount = 0;

	stSectorAround oldSectorAround;
	stSectorAround curSectorAround;

	GetSectorAround(oldSector.posX, oldSector.posY, &oldSectorAround);
	GetSectorAround(curSector.posX, curSector.posY, &curSectorAround);

	for (INT oldCount = 0; oldCount < oldSectorAround.count; ++oldCount)
	{
		bFindFlag = FALSE;

		for (INT curCount = 0; curCount < curSectorAround.count; ++curCount)
		{
			// oldSectorAround 와 curSectorAround가 하나라도 겹치는게 없다면은 pPlayer 영향권 섹터에서 지워질 섹터이다.
			if (oldSectorAround.around[oldCount].posX == curSectorAround.around[curCount].posX
				&& oldSectorAround.around[oldCount].posY == curSectorAround.around[curCount].posY)
			{
				bFindFlag = TRUE;
				break;
			}
		}

		if (bFindFlag == FALSE)
		{
			pRemoveSectorAround->around[removeSectorCount] = oldSectorAround.around[oldCount];

			++removeSectorCount;
		}
	}

	pRemoveSectorAround->count = removeSectorCount;



	for (INT curCount = 0; curCount < curSectorAround.count; ++curCount)
	{
		bFindFlag = FALSE;

		for (INT oldCount = 0; oldCount < oldSectorAround.count; ++oldCount)
		{
			// curSectorAround와 oldSectorAround 하나라도 겹치는게 없다면은 pPlayer 영향권 섹터에 추가될 섹터이다.
			if (curSectorAround.around[curCount].posX == oldSectorAround.around[oldCount].posX
				&& curSectorAround.around[curCount].posY == oldSectorAround.around[oldCount].posY)
			{
				bFindFlag = TRUE;
				break;
			}
		}

		if (bFindFlag == FALSE)
		{
			pAddSectorAround->around[addSectorCount] = curSectorAround.around[curCount];

			++addSectorCount;
		}
	}

	pAddSectorAround->count = addSectorCount;

	return;
}


void goddamnbug::PackingLoginResponse(BYTE status, UINT64 accountNo, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_LOGIN << status << accountNo;

	return;
}

void goddamnbug::PackingCharacterSelectResponse(BYTE status, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << status;

	return;
}

void goddamnbug::PackingCreateMyCharacter(UINT64 clientID, BYTE characterType, WCHAR* pNickName, DWORD cbNickName, FLOAT posX, FLOAT posY, WORD rotaion, INT cristalCount, INT HP, INT64 EXP, WORD level, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER << clientID << characterType;

	pMessage->PutPayload((CHAR*)pNickName, cbNickName);

	pMessage->MoveWritePos(cbNickName);

	*pMessage << posX << posY << rotaion << cristalCount << HP << EXP << level;

	return;
}


void goddamnbug::PackingCreateOthreadCharacter(UINT64 clientID, BYTE characterType, WCHAR* pNickName, DWORD cbNickName, FLOAT posX, FLOAT posY, WORD rotation, WORD level, BYTE bRespawnFlag, BYTE bSitFlag, BYTE bDieFlag, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER << clientID << characterType;

	pMessage->PutPayload((CHAR*)pNickName, cbNickName);

	pMessage->MoveWritePos(cbNickName);

	*pMessage << posX << posY << rotation << level << bRespawnFlag << bSitFlag << bDieFlag;

	return;
}

void goddamnbug::PackingCreateMonster(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE bRespawnFlag, CMessage *pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER << clientID << posX << posY << rotation << bRespawnFlag;

	return;
}


void goddamnbug::PackingRemoveObject(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT << clientID;

	return;
}

void goddamnbug::PackingCharacterMoveStart(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE vKey, BYTE hKey, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_MOVE_CHARACTER << clientID << posX << posY << rotation << vKey << hKey;

	return;
}

void goddamnbug::PackingCharacterMoveStop(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_STOP_CHARACTER << clientID << posX << posY << rotation;

	return;
}


void goddamnbug::PackingMonsterMoveStart(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_MOVE_MONSTER << clientID << posX << posY << rotation;

	return;
}


void goddamnbug::PackingPlayerAttack1Action(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_ATTACK1 << clientID;

	return;
}


void goddamnbug::PackingPlayerAttack2Action(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_ATTACK2 << clientID;

	return;
}


void goddamnbug::PackingMonsterAttackAction(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_MONSTER_ATTACK << clientID;

	return;
}




void goddamnbug::PackingCristalPickAction(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_PICK << clientID;

	return;
}

void goddamnbug::PackingCristalPick(UINT64 clientID, UINT64 cristalClientID, INT playerCristalAmount, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_PICK_CRISTAL << clientID << cristalClientID << playerCristalAmount;

	return;
}

void goddamnbug::PackingCharacterSit(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_SIT << clientID;

	return;
}

void goddamnbug::PackingAttackDamage(UINT64 attackClientID, UINT64 victimClientID, INT damage, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_DAMAGE << attackClientID << victimClientID << damage;

	return;
}


void goddamnbug::PackingPlayerDie(UINT64 clientID, INT minusCristal, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_PLAYER_DIE << clientID << minusCristal;

	return;
}

void goddamnbug::PackingMonsterDie(UINT64 clientID, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_MONSTER_DIE << clientID;

	return;
}


void goddamnbug::PackingCreateCristal(UINT64 clientID, BYTE cristalType, FLOAT posX, FLOAT posY, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_CRISTAL << clientID << cristalType << posX << posY;

	return;
}


void goddamnbug::PackingPlayerHP(INT HP, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_PLAYER_HP << HP;

	return;
}

void goddamnbug::PackingPlayerRestart(CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_PLAYER_RESTART;

	return;
}


void CGodDamnBug::setMonsters(void)
{
	setArea1Monsters();

	setArea2Monsters();

	setArea3Monsters();

	setArea4Monsters();

	setArea5Monsters();

	setArea6Monsters();

	setArea7Monsters();

	return;
}


void CGodDamnBug::setArea1Monsters(void)
{
	for (INT count = 0; count < AREA1_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea1, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}


void CGodDamnBug::setArea2Monsters(void)
{
	for (INT count = 0; count < AREA2_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea2, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}

void CGodDamnBug::setArea3Monsters(void)
{
	for (INT count = 0; count < AREA2_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea3, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}


void CGodDamnBug::setArea4Monsters(void)
{
	for (INT count = 0; count < AREA2_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea4, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}
void CGodDamnBug::setArea5Monsters(void)
{
	for (INT count = 0; count < AREA2_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea5, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}

void CGodDamnBug::setArea6Monsters(void)
{
	for (INT count = 0; count < AREA2_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea6, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}


void CGodDamnBug::setArea7Monsters(void)
{
	for (INT count = 0; count < AREA2_MONSTER_COUNT; ++count)
	{
		CMonster* pMonster = CMonster::Alloc(GetObjectID(), (INT)eMonsterType::Monster1, (INT)eMonsterActivityArea::MonsterArea7, this);

		CObjectManager::GetInstance()->InsertObject(pMonster);

		InsertMonsterSectorMap(pMonster->GetCurSectorX(), pMonster->GetCurSectorY(), pMonster->GetClientID(), pMonster);
	}

	return;
}


void CGodDamnBug::authReleasePlayers(void)
{
	auto iterE = mAuthReleasePlayerMap.end();

	for (auto iter = mAuthReleasePlayerMap.begin(); iter != iterE;)
	{
		CPlayer* pPlayer = iter->second;
		++iter;

		if (pPlayer->GetDBWriteCount() == 0)
		{		
			{
				CSRWLockExclusive srwLock(&mPlayerMapLock);

				ErasePlayerMap(pPlayer->GetAccountNo());
			}

			EraseAuthReleasePlayerMap(pPlayer->GetAccountNo());

			pPlayer->SetRelease();
		}
	}

	return;
}



void CGodDamnBug::SendAroundCreateCristal(CCristal* pCristal)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingCreateCristal(pCristal->GetClientID(), pCristal->GetCristalType(), pCristal->GetPosX(), pCristal->GetPosY(), pMessage);

	SendPacketAroundSector(pCristal->GetCurSectorX(), pCristal->GetCurSectorY(), nullptr, pMessage);

	pMessage->Free();

	return;
}

void CGodDamnBug::SendAroundRemoveCristal(CCristal* pCristal)
{
	CMessage* pMessage = CMessage::Alloc();

	goddamnbug::PackingRemoveObject(pCristal->GetClientID(), pMessage);

	SendPacketAroundSector(pCristal->GetCurSectorX(), pCristal->GetCurSectorY(), nullptr, pMessage);

	pMessage->Free();

	return;
}


void CGodDamnBug::IncrementLoginRequestCount(void)
{
	++mLoginRequestCount;

	return;
}

INT64 CGodDamnBug::GetLoginRequestCount(void) const
{
	return mLoginRequestCount;
}