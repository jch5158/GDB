#include "stdafx.h"

CChattingServer::CChattingServer(void)
	: mUpdateTPS(0)
	, mbStopFlag(FALSE)
	, mbUpdateLoopFlag(TRUE)
	, mbWhiteIPModeFlag(FALSE)
	, mPlayerCount(0)
	, mUpdateThreadID(0)
	, mSectorAround{ 0, }
	, mJobQueue()
	, mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	, mUpdateEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
	, mPlayerMap()
	, mSectorArray()
{

	// ���͸� �̸� ���س��´�.
	for (INT indexY = 0; indexY < chattingserver::SECTOR_MAX_HEIGHT; ++indexY)
	{
		for (INT indexX = 0; indexX < chattingserver::SECTOR_MAX_WIDTH; ++indexX)
		{
			getSectorAround(indexY, indexX, &mSectorAround[indexY][indexX]);
		}
	}
}

CChattingServer::~CChattingServer(void)
{
	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CCrashDump::Crash();
	}

	CloseHandle(mUpdateThreadHandle);
}

DWORD CChattingServer::ExecuteUpdateThread(void* pParam)
{
	CChattingServer* pChattingServer = (CChattingServer*)pParam;

	pChattingServer->UpdateThread();

	return 1;
}

void CChattingServer::UpdateThread(void)
{
	CJob* pJob = nullptr;

	while (mbUpdateLoopFlag == TRUE)
	{
		if (mJobQueue.Dequeue(&pJob) == FALSE)
		{
			if (WaitForSingleObject(mUpdateEvent, INFINITE) != WAIT_OBJECT_0)
			{
				CCrashDump::Crash();
			}

			continue;
		}

		{
			//CPerformanceProfiler profiler(L"Update Thread");

			jobProcedure(pJob);

			pJob->Free();

			InterlockedIncrement(&mUpdateTPS);
		}
	}

	return;
}


DWORD CChattingServer::GetJobQueueStatus(void)
{
	return mJobQueue.GetUseSize();
}


DWORD CChattingServer::GetPlayerCount(void)
{
	return mPlayerCount;
}

bool CChattingServer::OnStart(void)
{	
	if (setupUpdateThread() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}


// �ű� ������ �����Ͽ����� JobQueue�� Job�� ������ �˷��ش�.
void CChattingServer::OnClientJoin(UINT64 sessionID)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::CLIENT_JOIN_JOB);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}


// ������ �����Ͽ����� JobQueue�� ���� �˷��ش�.
void CChattingServer::OnClientLeave(UINT64 sessionID)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::CLIENT_LEAVE_JOB);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}


void CChattingServer::OnRecv(UINT64 sessionID, CMessage* pMessage)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::MESSAGE_JOB, pMessage);

	pMessage->AddReferenceCount();

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}

void CChattingServer::OnStartAcceptThread(void)
{
	return;
}

void CChattingServer::OnCloseAcceptThread(void)
{
	return;
}


void CChattingServer::OnStartWorkerThread(void)
{

	return;
}

void CChattingServer::OnCloseWorkerThread(void)
{

	return;
}

// ȭ��Ʈ IP
bool CChattingServer::OnConnectionRequest(const wchar_t* userIP, unsigned short userPort)
{

	if (mbWhiteIPModeFlag == TRUE)
	{
		// TODO : DB ��ȸ
	}


	return TRUE;
}

void CChattingServer::OnError(unsigned int errorCode, const wchar_t* errorMessage)
{


}

void CChattingServer::OnStop(void)
{
	CJob* pJob = createJob(0, CJob::eJobType::SERVER_STOP_JOB, nullptr);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}

INT CChattingServer::GetJobQueueUseSize(void) const
{ 
	return mJobQueue.GetUseSize();
}


void CChattingServer::SetWhiteIPModeFlag(BOOL bWhiteIPModeFlag)
{
	mbWhiteIPModeFlag = bWhiteIPModeFlag;

	return;
}

BOOL CChattingServer::GetWhiteIPModeFlag(void) const
{
	return mbWhiteIPModeFlag;
}


DWORD CChattingServer::GetUpdateTPS(void) const
{
	return mUpdateTPS;
}

void CChattingServer::InitializeChatTPS(void)
{
	InterlockedExchange(&mUpdateTPS, 0);
	
	return;
}

void CChattingServer::jobProcedure(CJob* pJob)
{
	switch (pJob->mJobType)
	{
	case CJob::eJobType::CLIENT_JOIN_JOB:

		jobProcedureClientJoin(pJob->mSessionID);

		break;
	case CJob::eJobType::MESSAGE_JOB:

		jobProcedureMessage(pJob->mSessionID, pJob->mpMessage);

		break;
	case CJob::eJobType::CLIENT_LEAVE_JOB:

		jobProcedureClientLeave(pJob->mSessionID);

		break;
	case CJob::eJobType::SERVER_STOP_JOB:

		jobProcedureServerStop();

		break;
	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[jobProcedure] jobType : %d", pJob->mJobType);

		CCrashDump::Crash();

		break;
	}

	return;
}

void CChattingServer::jobProcedureMessage(UINT64 sessionID, CMessage* pMessage)
{
	WORD messageType;

	*pMessage >> messageType;

	if (recvProcedure(sessionID, messageType, pMessage) == FALSE)
	{
		Disconnect(sessionID);
	}

	pMessage->Free();

	return;
}


CJob* CChattingServer::createJob(UINT64 sessionID, CJob::eJobType jobType, CMessage* pMessage)
{
	CJob* pJob = CJob::Alloc();

	pJob->mSessionID = sessionID;

	pJob->mJobType = jobType;

	pJob->mpMessage = pMessage;

	return pJob;
}


BOOL CChattingServer::setupUpdateThread(void)
{
	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)&mUpdateThreadID);
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[OnStart] _beginthreadex Error Code : %d", GetLastError());

		return FALSE;
	}

	return TRUE;
}


CPlayer* CChattingServer::findPlayer(UINT64 sessionID)
{
	auto iter = mPlayerMap.find(sessionID);

	if (iter == mPlayerMap.end())
	{
		return nullptr;
	}

	return iter->second;
}


//CChattingServer::CConnectionState* CChattingServer::findConnectionState(UINT64 sessionID)
//{
//	auto iter = mConnectionStateMap.find(sessionID);
//
//	if (iter == mConnectionStateMap.end())
//	{
//		return nullptr;
//	}
//
//	return iter->second;
//}


BOOL CChattingServer::createPlayer(UINT64 acountNumber, UINT64 sessionID, CPlayer** pPlayer)
{
	if (nullptr != findPlayer(sessionID))
	{
		return FALSE;
	}

	*pPlayer = CPlayer::Alloc();

	(*pPlayer)->mAccountNumber = acountNumber;

	(*pPlayer)->mSessionID = sessionID;

	mPlayerMap.insert(std::pair<UINT64, CPlayer*>(sessionID, *pPlayer));

	++mPlayerCount;

	return TRUE;
}

void CChattingServer::deletePlayer(UINT64 sessionID)
{
	CPlayer* pPlayer = findPlayer(sessionID);
	if (pPlayer == nullptr)
	{
		return;
	}

	removeSector(pPlayer);

	--mPlayerCount;

	mPlayerMap.erase(sessionID);

	if (pPlayer->Free() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[deletePlayer] Player Free Error, sessionID : %llx", sessionID);

		CCrashDump::Crash();
	}
}


void CChattingServer::removeSector(CPlayer* pPlayer)
{
	// ���� �ڸ� ���� ���̶���� return
	if (pPlayer->mbSectorSetFlag == FALSE)
	{
		// �ڸ����� Flag�� TRUE�� �������ش�.
		pPlayer->mbSectorSetFlag = TRUE;

		return;
	}

	mSectorArray[pPlayer->mSectorY][pPlayer->mSectorX].erase(pPlayer->mSessionID);

	return;
}

void CChattingServer::addSector(CPlayer* pPlayer)
{
	mSectorArray[pPlayer->mSectorY][pPlayer->mSectorX].insert(std::pair<UINT64, CPlayer*>(pPlayer->mSessionID, pPlayer));

	return;
}


// Sector ���� ����� ���ؼ� ���ڴ� short�� �޴´�.
void CChattingServer::getSectorAround(INT posY, INT posX, stSectorAround* pSectorAround)
{
	posY -= 1;
	posX -= 1;

	for (INT countY = 0; countY < 3; ++countY)
	{
		if (posY + countY < 0 || posY + countY >= chattingserver::SECTOR_MAX_HEIGHT)
		{
			continue;
		}

		for (INT countX = 0; countX < 3; ++countX)
		{
			if (posX + countX < 0 || posX + countX >= chattingserver::SECTOR_MAX_WIDTH)
			{
				continue;
			}

			pSectorAround->aroundSector[pSectorAround->count].posY = posY + countY;
			pSectorAround->aroundSector[pSectorAround->count].posX = posX + countX;

			++pSectorAround->count;
		}
	}
}


void CChattingServer::sendOneSector(stSector* pSector, CMessage* pMessage)
{
	std::unordered_map<UINT64, CPlayer*>* pPlayerMap = &mSectorArray[pSector->posY][pSector->posX];

	// ������� for���� ����Ѵ�.
	for (auto& iter : *pPlayerMap)
	{
		//++gSendPacketAround;
		SendPacket(iter.first, pMessage);
	}

	return;
}

void CChattingServer::sendAroundSector(stSectorAround* pSectorAround, CMessage* pMessage)
{
	//CPerformanceProfiler profiler(L"sendAroundSector");

	stSector* pSectorArray = pSectorAround->aroundSector;

	INT count = pSectorAround->count;

	for (INT index = 0; index < count; ++index)
	{
		sendOneSector(&pSectorArray[index], pMessage);
	}
}

BOOL CChattingServer::recvProcedureLoginRequest(UINT64 sessionID, CMessage* pMessage)
{
	UINT64 accountNumber;

	*pMessage >> accountNumber;

	CPlayer* pPlayer;

	if (createPlayer(accountNumber, sessionID, &pPlayer) == FALSE)
	{
		Disconnect(sessionID);

		return FALSE;
	}

	pMessage->GetPayload((CHAR*)pPlayer->mPlayerID, player::PLAYER_STRING_LENGTH * sizeof(WCHAR));

	pMessage->MoveReadPos(player::PLAYER_STRING_LENGTH * sizeof(WCHAR));

	pMessage->GetPayload((CHAR*)pPlayer->mPlayerNickName, player::PLAYER_STRING_LENGTH * sizeof(WCHAR));

	pMessage->MoveReadPos(player::PLAYER_STRING_LENGTH * sizeof(WCHAR));

	sendLoginResponse(TRUE, pPlayer);

	return TRUE;
}


void CChattingServer::sendLoginResponse(bool bLoginFlag, CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	// �α��� �Ϸ� �޽����� �����.
	packingLoginResponseMessage(bLoginFlag, pPlayer->mAccountNumber, pMessage);

	// �α��� �Ϸ� �޽����� �����Ѵ�.
	SendPacket(pPlayer->mSessionID, pMessage);

	pMessage->Free();

	return;
}




void CChattingServer::packingLoginResponseMessage(bool bLoginFlag, UINT64 accountNumber, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << bLoginFlag << accountNumber;

	return;
}


// ���� �̵� 
BOOL CChattingServer::recvProcedureSectorMoveRequest(UINT64 sessionID, CMessage* pMessage)
{
	//CPerformanceProfiler profiler(L"Move Request");

	// ���� Ȯ��
	// ���� �̵���û ��Ŷ���� �۰� �Դٸ��� return FALSE
	if (pMessage->GetUseSize() != 12)
	{
		return FALSE;
	}

	UINT64 acountNumber;
	WORD sectorX;
	WORD sectorY;

	*pMessage >> acountNumber >> sectorX >> sectorY;


	// �α������� �ʾҴµ� ���� �̵���û�� �������� ���´�.
	CPlayer* pPlayer = findPlayer(sessionID);
	//if (pPlayer == nullptr)
	//{	return FALSE;
	//}


	// ��ī��Ʈ �ѹ��� ��ġ�ϴ��� Ȯ���Ѵ�. ��ġ���� �ʴٸ��� return FALSE
	//if (pPlayer->mAccountNumber != acountNumber)
	//{
	//	return FALSE;
	//}

	// unsigned �̱� ������ SECTOR_MAX_WIDTH, SECTOR_MAX_HEIGHT ���� ū���� Ȯ���ϸ� �ȴ�.  
	//if (sectorX > SECTOR_MAX_WIDTH || sectorY > SECTOR_MAX_HEIGHT)
	//{
	//	return FALSE;
	//}

	removeSector(pPlayer);

	pPlayer->mSectorX = sectorX;
	pPlayer->mSectorY = sectorY;

	addSector(pPlayer);

	sendSectorMoveResponse(pPlayer);

	return TRUE;
}

// ���� �̵� ��� �۽�
void CChattingServer::sendSectorMoveResponse(CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	packingSectorMoveMessage(pPlayer->mAccountNumber, pPlayer->mSectorX, pPlayer->mSectorY, pMessage);

	SendPacket(pPlayer->mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CChattingServer::packingSectorMoveMessage(UINT64 accountNumber, WORD sectorX, WORD sectorY, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNumber << sectorX << sectorY;

	return;
}


// ä�� �޽��� ��û
BOOL CChattingServer::recvProcedureChatRequest(UINT64 sessionID, CMessage* pMessage)
{
	//CPerformanceProfiler profiler(L"Chat Request");

	// ��ī��Ʈ �ѹ��� ä�� ���� ���� �� ������ return FALSE
	//if (pMessage->GetUseSize() < 10)
	//{
	//	return FALSE;
	//}

	UINT64 acountNumber;

	WORD chatLength;

	*pMessage >> acountNumber >> chatLength;

	// ���� �α������� �ʾҴٸ��� return FALSE;
	CPlayer* pPlayer = findPlayer(sessionID);
	//if (pPlayer == nullptr)
	//{
	//	return FALSE;
	//}

	// ���� Ȯ��
	// ��ī��Ʈ �ѹ��� ��ġ���� �������� return FALSE;
	if (pPlayer->mAccountNumber != acountNumber)
	{
		return FALSE;
	}

	// ���� ���� ������ ���� �ʾҴٸ��� return FALSE
	/*if (pPlayer->mbSectorSetFlag == FALSE)
	{
		return FALSE;
	}*/

	// ���� �޽��� ������� �޽��� ���̿� �ٸ��ų� ä�� �޽��� ���̸� �ʰ��ϴ� �޽����� �Դٸ� return FALSE
	/*if (pMessage->GetUseSize() != chatLength || chatLength > MAX_CHAT_LENGTH * sizeof(WCHAR))
	{
		return FALSE;
	}*/

	WCHAR pChat[chattingserver::MAX_CHAT_LENGTH];

	pMessage->GetPayload((CHAR*)pChat, chatLength);

	pMessage->MoveReadPos(chatLength);

	sendChatResponse(chatLength, pChat, pPlayer);

	return TRUE;
}

void CChattingServer::sendChatResponse(WORD chatLength, WCHAR* pChat, CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	packingChatMessage(pPlayer->mAccountNumber, pPlayer->mPlayerID, pPlayer->mPlayerNickName, chatLength, pChat, pMessage);

	// ä�� �۽��� �ֺ� ���ͷ� �޽����� �Ѹ���.
	sendAroundSector(&mSectorAround[pPlayer->mSectorY][pPlayer->mSectorX], pMessage);

	pMessage->Free();

	return;
}


void CChattingServer::packingChatMessage(UINT64 accountNumber, WCHAR* pPlayerID, WCHAR* pPlayerNickName, WORD chatLength, WCHAR* pChat, CMessage* pMessage)
{

	*pMessage << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE << accountNumber;

	pMessage->PutPayload((CHAR*)pPlayerID, sizeof(WCHAR) * player::PLAYER_STRING_LENGTH);

	pMessage->MoveWritePos(sizeof(WCHAR) * player::PLAYER_STRING_LENGTH);

	pMessage->PutPayload((CHAR*)pPlayerNickName, sizeof(WCHAR) * player::PLAYER_STRING_LENGTH);

	pMessage->MoveWritePos(sizeof(WCHAR) * player::PLAYER_STRING_LENGTH);

	*pMessage << chatLength;

	pMessage->PutPayload((CHAR*)pChat, chatLength);

	pMessage->MoveWritePos(chatLength);
}

BOOL CChattingServer::recvProcedureHeartbeatRequest(UINT64 sessionID)
{
	//CConnectionState* pConnectionState = findConnectionState(sessionID);
	//if (pConnectionState == nullptr)
	//{
	//	return FALSE;
	//}

	//pConnectionState->mTimeout = timeGetTime();

	return TRUE;
}

void CChattingServer::jobProcedureClientJoin(UINT64 sessionID)
{
	//CConnectionState* pConnectionState = CConnectionState::Alloc();

	//mConnectionStateMap.insert(std::make_pair(sessionID, pConnectionState));

	return;
}




void CChattingServer::jobProcedureClientLeave(UINT64 sessionID)
{
	deletePlayer(sessionID);

	if (mbStopFlag == TRUE)
	{
		if (mPlayerMap.empty() == TRUE)
		{
			mbUpdateLoopFlag = FALSE;
		}
	}

	return;
}

void CChattingServer::jobProcedureServerStop(void)
{
	mbStopFlag = TRUE;

	if (mPlayerMap.empty() == TRUE)
	{
		mbUpdateLoopFlag = FALSE;
	}

	return;
}

// �޽��� �������� 
BOOL CChattingServer::recvProcedure(UINT64 sessionID, DWORD messageType, CMessage* pMessage)
{
	switch (messageType)
	{
		// �α��� ��û �޽���
	case en_PACKET_CS_CHAT_REQ_LOGIN:

		return recvProcedureLoginRequest(sessionID, pMessage);

		// ���� �̵� �޽���
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:

		return recvProcedureSectorMoveRequest(sessionID, pMessage);

		// ä�� �޽���
	case en_PACKET_CS_CHAT_REQ_MESSAGE:

		return recvProcedureChatRequest(sessionID, pMessage);

		// ��Ʈ��Ʈ �޽��� ( ���� ��� X )
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:

		return recvProcedureHeartbeatRequest(sessionID);

	default:

		// ���� Ȯ��		
		return FALSE;
	}

	return TRUE;
}

