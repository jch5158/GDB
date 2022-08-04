#include "stdafx.h"




CChattingServer::CChattingServer(void)
	: mbStopFlag(false)
	, mbUpdateLoopFlag(true)

	, mAuthenticThreadID(0)
	, mUpdateThreadID(0)
	, mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	, mAuthenticThreadHandle(INVALID_HANDLE_VALUE)

	, mUpdateEvent(CreateEvent(NULL, false, false, NULL))
	, mAuthenticEvent(CreateEvent(NULL, false, false, NULL))

	, mUpdateTPS(0)
	, mPlayerCount(0)

	, mRedisConnector()

	, mJobQueue()
	, mAuthenticJobQueue()

	, mAccountNoMap()
	, mPlayerMap()
	, mPlayerSectorMap()
	, mSectorAround{ 0, }

{
	CRedisConnector::CallWSAStartup();

	// �ֺ� ���͸� �̸� ���س��´�.
	for (int indexY = 0; indexY < chattingserver::SECTOR_MAX_HEIGHT; ++indexY)
	{
		for (int indexX = 0; indexX < chattingserver::SECTOR_MAX_WIDTH; ++indexX)
			getSectorAround(indexY, indexX, &mSectorAround[indexY][indexX]);	
	}
}

CChattingServer::~CChattingServer(void)
{
	closeWaitUpdateThread();

	closeWaitAuthenticThread();

	CloseHandle(mUpdateEvent);

	CloseHandle(mAuthenticEvent);
}

bool CChattingServer::OnStart(void)
{
	if (setupUpdateThread() == false)
		return false;

	if (setupAuthenticThread() == false)
		return false;

	return true;
}

void CChattingServer::OnStop(void)
{
	CJob* pJob = createJob(0, CJob::eJobType::SERVER_STOP_JOB, nullptr);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}

void CChattingServer::OnStartAcceptThread(void)
{
	return;
}

void CChattingServer::OnStartWorkerThread(void)
{
	return;
}

void CChattingServer::OnCloseAcceptThread(void)
{
	return;
}


void CChattingServer::OnCloseWorkerThread(void)
{
	return;
}

bool CChattingServer::OnConnectionRequest(const wchar_t* userIP, unsigned short userPort)
{
	return false;
}


void CChattingServer::OnClientJoin(unsigned long long sessionID)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::CLIENT_JOIN_JOB);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}

void CChattingServer::OnClientLeave(unsigned long long sessionID)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::CLIENT_LEAVE_JOB);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}



void CChattingServer::OnRecv(unsigned long long sessionID, CMessage* pMessage)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::MESSAGE_JOB, pMessage);

	pMessage->AddReferenceCount();

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}



void CChattingServer::OnError(unsigned int errorCode, const wchar_t* errorMessage)
{
	return;
}


int CChattingServer::GetJobQueueStatus(void) const
{
	return mJobQueue.GetUseSize();
}


int CChattingServer::GetPlayerCount(void) const
{
	return mPlayerCount;
}


long CChattingServer::GetUpdateTPS(void) const
{
	return mUpdateTPS;
}


void CChattingServer::InitializeUpdateTPS(void)
{
	mUpdateTPS = 0;

	return;
}


unsigned CChattingServer::ExecuteUpdateThread(void* pParam)
{
	CChattingServer* pChattingServer = (CChattingServer*)pParam;

	pChattingServer->UpdateThread();

	return 1;
}

unsigned CChattingServer::ExecuteAuthenticThread(void* pParam)
{
	CChattingServer* pChattingServer = (CChattingServer*)pParam;

	pChattingServer->AuthenticThread();

	return 1;
}

void CChattingServer::UpdateThread(void)
{
	CJob* pJob = nullptr;

	while (mbUpdateLoopFlag == true)
	{
		if (mJobQueue.Dequeue(&pJob) == false)
		{
			if (WaitForSingleObject(mUpdateEvent, INFINITE) != WAIT_OBJECT_0)
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"WaitSingForObject Error : %d", GetLastError());

				CCrashDump::Crash();
			}

			continue;
		}

		jobProcedure(pJob);

		pJob->Free();

		InterlockedIncrement(&mUpdateTPS);
	}

	return;
}


void CChattingServer::AuthenticThread(void)
{
	mRedisConnector.Connect();

	for (;;)
	{
		int useSize = mAuthenticJobQueue.GetUseSize();
		if (useSize == 0)
		{
			if (WaitForSingleObject(mAuthenticEvent, INFINITE) != WAIT_OBJECT_0)
				CCrashDump::Crash();

			continue;
		}

		CJob* pAuthenticJob;

		for (int cnt = 0; cnt < useSize; ++cnt)
		{
			mAuthenticJobQueue.Dequeue(&pAuthenticJob);

			pAuthenticJob->mJobType = CJob::eJobType::LOGIN_JOB;

			// ���� ������ ��ū ������ ���� �ʴ´�.
			if (pAuthenticJob->mAccountNumber > 999999)
			{
				if (mRedisConnector.CompareToken(pAuthenticJob->mAccountNumber, pAuthenticJob->mSessionKey) == true)
					pAuthenticJob->mLoginResult = true;
				else
					pAuthenticJob->mLoginResult = false;
			}
			else
				pAuthenticJob->mLoginResult = true;

			mJobQueue.Enqueue(pAuthenticJob);

			SetEvent(mUpdateEvent);
		}
	}

	mRedisConnector.Disconnect();

	return;
}



bool CChattingServer::setupUpdateThread(void)
{
	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)mUpdateThreadID);
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[setupUpdateThread] _beginthreadex Error : %d", GetLastError());

		return false;
	}

	return true;
}



bool CChattingServer::setupAuthenticThread(void)
{
	mAuthenticThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteAuthenticThread, this, NULL, (UINT*)mAuthenticThreadID);
	if (mAuthenticThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[setupAuthenticThread] _beginthreadex Error : %d", GetLastError());

		return false;
	}

	return true;
}


void CChattingServer::closeWaitUpdateThread(void)
{
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
		return;

	// Update ������ �ñ׳��� ��ٸ�
	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[closeWaitUpdateThread] WaitForSingleObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	// ������ �ڵ� ��ȯ
	CloseHandle(mUpdateThreadHandle);

	mUpdateThreadHandle = INVALID_HANDLE_VALUE;

	return;
}

void CChattingServer::closeWaitAuthenticThread(void)
{
	if (mAuthenticThreadHandle == INVALID_HANDLE_VALUE)
		return;

	// Authentic ������ �ñ׳��� ��ٸ� 
	if (WaitForSingleObject(mAuthenticThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[closeWaitAuthenticThread] WaitForSingleObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mAuthenticThreadHandle);

	mAuthenticThreadHandle = INVALID_HANDLE_VALUE;

	return;
}






bool CChattingServer::insertAccountNoMap(unsigned long long sessionID, unsigned long long accountNo)
{
	return mAccountNoMap.insert(std::make_pair(sessionID, accountNo)).second;
}

bool CChattingServer::eraseAccountNoMap(unsigned long long sessionID)
{
	return mAccountNoMap.erase(sessionID);
}

bool CChattingServer::findAccountNoFromAccountMap(unsigned long long sessionID, unsigned long long* pAccountNo)
{
	auto iter = mAccountNoMap.find(sessionID);
	if (iter == mAccountNoMap.end())
		return false;

	*pAccountNo = iter->second;

	return true;
}



bool CChattingServer::insertPlayerMap(unsigned long long accountNo, CPlayer* pPlayer)
{
	return mPlayerMap.insert(std::make_pair(accountNo,pPlayer)).second;
}


bool CChattingServer::erasePlayerMap(unsigned long long accountNo)
{
	return mPlayerMap.erase(accountNo);
}

CPlayer* CChattingServer::findPlayerFromPlayerMap(unsigned long long accountNo) const
{	
	auto iter = mPlayerMap.find(accountNo);
	if (iter == mPlayerMap.end())
		return nullptr;

	return iter->second;
}



bool CChattingServer::insertPlayerSectorMap(int sectorPosX, int sectorPosY, unsigned long long accountNo, CPlayer* pPlayer)
{
	if (sectorPosX < 0 || sectorPosX >= chattingserver::SECTOR_MAX_WIDTH || sectorPosY < 0 || sectorPosY >= chattingserver::SECTOR_MAX_HEIGHT)
		return false;

	return mPlayerSectorMap[sectorPosY][sectorPosX].insert(std::make_pair(accountNo, pPlayer)).second;
}

bool CChattingServer::erasePlayerSectorMap(int sectorPosX, int sectorPosY, unsigned long long accountNo)
{
	if (sectorPosX < 0 || sectorPosX >= chattingserver::SECTOR_MAX_WIDTH || sectorPosY < 0 || sectorPosY >= chattingserver::SECTOR_MAX_HEIGHT)
		return false;

	return mPlayerSectorMap[sectorPosY][sectorPosX].erase(accountNo);
}


void CChattingServer::removeSector(CPlayer* pPlayer)
{
	// ���� �ڸ� ���� ���̶���� return
	if (pPlayer->mbSectorSetFlag == false)
	{
		// �ڸ����� Flag�� true�� �������ش�.
		pPlayer->mbSectorSetFlag = true;

		return;
	}

	erasePlayerSectorMap(pPlayer->mSectorX, pPlayer->mSectorY, pPlayer->mAccountNumber);

	return;
}


bool CChattingServer::createPlayer(unsigned long long accountNumber, unsigned long long sessionID, CPlayer** pPlayer)
{
	CPlayer* pDuplicatePlayer = findPlayerFromPlayerMap(accountNumber);
	if (pDuplicatePlayer != nullptr)
	{
		// ���� ����
		Disconnect(pDuplicatePlayer->mSessionID);

		return false;
	}

	*pPlayer = CPlayer::Alloc();

	(*pPlayer)->mAccountNumber = accountNumber;

	(*pPlayer)->mSessionID = sessionID;

	return true;
}

bool CChattingServer::deletePlayer(unsigned long long accountNumber, unsigned long long sessionID)
{
	CPlayer* pPlayer = findPlayerFromPlayerMap(accountNumber);
	if (pPlayer == nullptr)
		return false;

	removeSector(pPlayer);

	erasePlayerMap(accountNumber);

	pPlayer->Free();

	--mPlayerCount;

	return true;
}







void CChattingServer::jobProcedure(CJob* pJob)
{
	switch (pJob->mJobType)
	{
	case CJob::eJobType::CLIENT_JOIN_JOB:

		clientJoinJobRequest(pJob->mSessionID);

		return;
	case CJob::eJobType::LOGIN_JOB:
		
		loginJobRequest(pJob->mSessionID, pJob);

		return;
	case CJob::eJobType::MESSAGE_JOB:

		messageJobRequest(pJob->mSessionID, pJob->mpMessage);

		return;
	case CJob::eJobType::CLIENT_LEAVE_JOB:
		
		clientLeaveJobRequest(pJob->mSessionID);

		return;

	case CJob::eJobType::SERVER_STOP_JOB:

		serverStopJobRequest();

		return;

	default:

		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[jobProcedure] jobType : %d", pJob->mJobType);

		CCrashDump::Crash();

		return;
	}

	return;
}

void CChattingServer::clientJoinJobRequest(unsigned long long sessionID)
{
	return;
}

void CChattingServer::loginJobRequest(unsigned long long sessionID, CJob* pJob)
{
	// �ش� ������ ���� ���� ���θ� Ȯ���Ѵ�.
	if (GetConnectionState(sessionID) == false)
		return;

	CPlayer* pPlayer;


	// ���� ����
	if (pJob->mLoginResult == false)
	{
		sendLoginResponse(false, pJob->mAccountNumber, sessionID);
		Disconnect(pJob->mSessionID);
	}
	else if (createPlayer(pJob->mAccountNumber, sessionID, &pPlayer) == true)
	{
		wcscpy_s(pPlayer->mPlayerID, pJob->mPlayerID);

		wcscpy_s(pPlayer->mPlayerNickName, pJob->mPlayerNickName);

		// sessionID, accountNo ����
		if (insertAccountNoMap(sessionID, pJob->mAccountNumber) == false)
			CCrashDump::Crash();

		if (insertPlayerMap(pPlayer->mAccountNumber, pPlayer) == false)
			CCrashDump::Crash();

		++mPlayerCount;

		sendLoginResponse(pJob->mLoginResult, pJob->mAccountNumber, sessionID);
	}
	else
	{
		sendLoginResponse(false, pJob->mAccountNumber, sessionID);

		// �ߺ� �α������� ���� �÷��̾�� ���� �÷��̾ ���´�.
		Disconnect(pPlayer->mSessionID);
		Disconnect(pJob->mSessionID);
	}

	return;
}


void CChattingServer::messageJobRequest(unsigned long long sessionID, CMessage* pMessage)
{
	unsigned short messageType;

	*pMessage >> messageType;

	if (messageProcedure(sessionID, messageType, pMessage) == false)
		Disconnect(sessionID);

	pMessage->Free();
	
	return;
}

void CChattingServer::clientLeaveJobRequest(unsigned long long sessionID)
{
	unsigned long long accountNo;

	if (findAccountNoFromAccountMap(sessionID, &accountNo) == true)
		deletePlayer(accountNo, sessionID);

	eraseAccountNoMap(sessionID);

	if (mbStopFlag == true && mPlayerMap.empty() == true)
		mbUpdateLoopFlag = false;

	return;
}

void CChattingServer::serverStopJobRequest(void)
{
	mbStopFlag = true;

	if (mPlayerMap.empty() == true)
		mbUpdateLoopFlag = false;

	return;
}




// Sector ���� ����� ���ؼ� ���ڴ� short�� �޴´�.
void CChattingServer::getSectorAround(int posY, int posX, stSectorAround* pSectorAround)
{	
	posY -= 1;
	posX -= 1;

	for (int countY = 0; countY < 3; ++countY)
	{
		if (posY + countY < 0 || posY + countY >= chattingserver::SECTOR_MAX_HEIGHT)
			continue;

		for (int countX = 0; countX < 3; ++countX)
		{
			if (posX + countX < 0 || posX + countX >= chattingserver::SECTOR_MAX_WIDTH)
				continue;

			pSectorAround->aroundSector[pSectorAround->count].posY = posY + countY;
			pSectorAround->aroundSector[pSectorAround->count].posX = posX + countX;

			++pSectorAround->count;
		}
	}

	return;
}


void CChattingServer::sendOneSector(stSector* pSector, CMessage* pMessage)
{
	if (pSector->posX < 0 || pSector->posX >= chattingserver::SECTOR_MAX_WIDTH || pSector->posY < 0 || pSector->posY >= chattingserver::SECTOR_MAX_HEIGHT)
		return;

	// ������� for���� ����Ѵ�.
	for (const auto &iter : mPlayerSectorMap[pSector->posY][pSector->posX])
		SendPacket(iter.second->mSessionID, pMessage);

	return;
}

void CChattingServer::sendAroundSector(stSectorAround* pSectorAround, CMessage* pMessage)
{
	stSector* pSectorArray = pSectorAround->aroundSector;

	int count = pSectorAround->count;
	for (int index = 0; index < count; ++index)
		sendOneSector(&pSectorArray[index], pMessage);

	return;
}


CJob* CChattingServer::createJob(unsigned long long sessionID, CJob::eJobType jobType, CMessage* pMessage)
{
	CJob* pJob = CJob::Alloc();

	pJob->mSessionID = sessionID;

	pJob->mJobType = jobType;

	pJob->mpMessage = pMessage;

	return pJob;
}

// �޽��� �������� 
bool CChattingServer::messageProcedure(unsigned long long sessionID, unsigned short messageType, CMessage* pMessage)
{
	switch (messageType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:           // �α��� ��û �޽���

		return loginRequest(sessionID, pMessage);

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:     // ���� �̵� �޽���

		return sectorMoveRequest(sessionID, pMessage);

	case en_PACKET_CS_CHAT_REQ_MESSAGE:         // ä�� �޽���

		return chatRequest(sessionID, pMessage);

		
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:       // ��Ʈ��Ʈ �޽��� ( ���� ��� X )

		return heartbeatRequest(sessionID);

	default:

		// ���� Ȯ��		
		return false;
	}

	return true;
}




bool CChattingServer::loginRequest(unsigned long long sessionID, CMessage* pMessage)
{			
	CJob* pAuthenticJob = CJob::Alloc();

	pAuthenticJob->mSessionID = sessionID;

	*pMessage >> pAuthenticJob->mAccountNumber;

	pMessage->GetPayload((char*)pAuthenticJob->mPlayerID, player::PLAYER_STRING_LENGTH * sizeof(wchar_t));

	pMessage->MoveReadPos(player::PLAYER_STRING_LENGTH * sizeof(wchar_t));

	pMessage->GetPayload((char*)pAuthenticJob->mPlayerNickName, player::PLAYER_STRING_LENGTH * sizeof(wchar_t));

	pMessage->MoveReadPos(player::PLAYER_STRING_LENGTH * sizeof(wchar_t));

	pMessage->GetPayload((char*)pAuthenticJob->mSessionKey, 64);

	pMessage->MoveReadPos(64);

	if (mAuthenticJobQueue.Enqueue(pAuthenticJob) == false)
	{
		pMessage->Free();

		return false;
	}

	SetEvent(mAuthenticEvent);

	return true;
}

bool CChattingServer::sectorMoveRequest(unsigned long long sessionID, CMessage* pMessage)
{
	unsigned long long accountNumber;
	unsigned short sectorX;
	unsigned short sectorY;

	*pMessage >> accountNumber >> sectorX >> sectorY;

	// �α������� �ʾҴµ� ���� �̵���û�� �������� ���´�.
	CPlayer* pPlayer = findPlayerFromPlayerMap(accountNumber);
	if (pPlayer == nullptr)
		return false;

	// ��ī��Ʈ �ѹ��� ��ġ�ϴ��� Ȯ���Ѵ�. ��ġ���� �ʴٸ��� return false
	if (pPlayer->mAccountNumber != accountNumber)
		return false;

	// unsigned �̱� ������ SECTOR_MAX_WIDTH, SECTOR_MAX_HEIGHT ���� ū���� Ȯ���ϸ� �ȴ�.  
	if (sectorX > chattingserver::SECTOR_MAX_WIDTH || sectorY > chattingserver::SECTOR_MAX_HEIGHT)
		return false;

	removeSector(pPlayer);

	pPlayer->mSectorX = sectorX;
	pPlayer->mSectorY = sectorY;

	insertPlayerSectorMap(pPlayer->mSectorX, pPlayer->mSectorY, pPlayer->mAccountNumber, pPlayer);

	sendSectorMoveResponse(pPlayer);

	return true;
}


bool CChattingServer::chatRequest(unsigned long long sessionID, CMessage* pMessage)
{
	unsigned long long accountNumber;

	unsigned short chatLength;

	*pMessage >> accountNumber >> chatLength;

	// ���� �α������� �ʾҴٸ��� return false;
	CPlayer* pPlayer = findPlayerFromPlayerMap(accountNumber);
	if (pPlayer == nullptr)
		return false;

	// ���� Ȯ��
	// ��ī��Ʈ �ѹ��� ��ġ���� �������� return false;
	if (pPlayer->mAccountNumber != accountNumber)
		return false;

	// ���� ���� ������ ���� �ʾҴٸ��� return false
	if (pPlayer->mbSectorSetFlag == false)
		return false;

	wchar_t pChat[chattingserver::MAX_CHAT_LENGTH];

	pMessage->GetPayload((char*)pChat, chatLength);

	pMessage->MoveReadPos(chatLength);

	sendChatResponse(chatLength, pChat, pPlayer);

	return true;
}

bool CChattingServer::heartbeatRequest(unsigned long long sessionID)
{
	return true;
}


void CChattingServer::sendLoginResponse(bool bLoginFlag, unsigned long long accountNo, unsigned long long sessionID)
{
	CMessage* pMessage = CMessage::Alloc();

	// �α��� �Ϸ� �޽����� �����.
	chattingserver::packingLoginReponse(bLoginFlag, accountNo, pMessage);

	// �α��� �Ϸ� �޽����� �����Ѵ�.
	SendPacket(sessionID, pMessage);

	pMessage->Free();

	return;
}



// ���� �̵� ��� �۽�
void CChattingServer::sendSectorMoveResponse(CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	chattingserver::packingSectorMoveResponse(pPlayer->mAccountNumber, pPlayer->mSectorX, pPlayer->mSectorY, pMessage);
	
	SendPacket(pPlayer->mSessionID, pMessage);	

	pMessage->Free();

	return;
}



// ä�� �޽��� ��û
void CChattingServer::sendChatResponse(unsigned short chatLength, wchar_t* pChat, CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	chattingserver::packingChatResponse(pPlayer->mAccountNumber, pPlayer->mPlayerID, sizeof(wchar_t) * player::PLAYER_STRING_LENGTH, pPlayer->mPlayerNickName, sizeof(wchar_t) * player::PLAYER_STRING_LENGTH, chatLength, pChat, pMessage);

	// ä�� �۽��� �ֺ� ���ͷ� �޽����� �Ѹ���.
	sendAroundSector(&mSectorAround[pPlayer->mSectorY][pPlayer->mSectorX], pMessage);

	pMessage->Free();
	
	return;
}



void chattingserver::packingLoginReponse(bool bLoginFlag, unsigned long long accountNumber, CMessage* pMessage)
{
	*pMessage << (unsigned short)en_PACKET_CS_CHAT_RES_LOGIN << bLoginFlag << accountNumber;

	return;
}

void chattingserver::packingSectorMoveResponse(unsigned long long accountNumber, unsigned short sectorX, unsigned short sectorY, CMessage* pMessage)
{
	*pMessage << (unsigned short)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNumber << sectorX << sectorY;

	return;
}

void chattingserver::packingChatResponse(unsigned long long accountNumber, wchar_t* pPlayerID, unsigned int cbPlayerID, wchar_t* pPlayerNickName, unsigned int cbPlayerNickName, unsigned short chatLength, wchar_t* pChat, CMessage* pMessage)
{
	*pMessage << (unsigned short)en_PACKET_CS_CHAT_RES_MESSAGE << accountNumber;

	pMessage->PutPayload((char*)pPlayerID, cbPlayerID);

	pMessage->MoveWritePos(cbPlayerID);

	pMessage->PutPayload((char*)pPlayerNickName, cbPlayerNickName);

	pMessage->MoveWritePos(cbPlayerNickName);

	*pMessage << chatLength;

	pMessage->PutPayload((char*)pChat, chatLength);

	pMessage->MoveWritePos(chatLength);

	return;
}



