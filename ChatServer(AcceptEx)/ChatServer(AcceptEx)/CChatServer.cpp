#include "stdafx.h"
#include "CChatServer.h"


CChatServer::CChatServer(void)
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

	// 섹터를 미리 구해놓는다.
	for (INT indexY = 0; indexY < chatserver::SECTOR_MAX_HEIGHT; ++indexY)
	{
		for (INT indexX = 0; indexX < chatserver::SECTOR_MAX_WIDTH; ++indexX)
		{
			getSectorAround(indexY, indexX, &mSectorAround[indexY][indexX]);
		}
	}
}

CChatServer::~CChatServer(void)
{
	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CCrashDump::Crash();
	}

	CloseHandle(mUpdateThreadHandle);
}

DWORD CChatServer::ExecuteUpdateThread(void* pParam)
{
	CChatServer* pChattingServer = (CChatServer*)pParam;

	pChattingServer->UpdateThread();

	return 1;
}

void CChatServer::UpdateThread(void)
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


DWORD CChatServer::GetJobQueueStatus(void)
{
	return mJobQueue.GetUseSize();
}


DWORD CChatServer::GetPlayerCount(void)
{
	return mPlayerCount;
}

BOOL CChatServer::OnStart(void)
{
	if (setupUpdateThread() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}


// 신규 세션이 접속하였음을 JobQueue에 Job을 던져서 알려준다.
void CChatServer::OnClientJoin(UINT64 sessionID)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::CLIENT_JOIN_JOB);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}


// 세션이 종료하였음을 JobQueue를 통해 알려준다.
void CChatServer::OnClientLeave(UINT64 sessionID)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::CLIENT_LEAVE_JOB);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}


void CChatServer::OnRecv(UINT64 sessionID, CMessage* pMessage)
{
	CJob* pJob = createJob(sessionID, CJob::eJobType::MESSAGE_JOB, pMessage);

	pMessage->AddReferenceCount();

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}

void CChatServer::OnStartAcceptThread(void)
{
	return;
}

void CChatServer::OnCloseAcceptThread(void)
{
	return;
}


void CChatServer::OnStartWorkerThread(void)
{

	return;
}

void CChatServer::OnCloseWorkerThread(void)
{

	return;
}

// 화이트 IP
bool CChatServer::OnConnectionRequest(const WCHAR* userIP, WORD userPort)
{

	if (mbWhiteIPModeFlag == TRUE)
	{
		// TODO : DB 조회
	}


	return TRUE;
}

void CChatServer::OnError(int errorCode, const WCHAR* errorMessage)
{

	return;
}

void CChatServer::OnStop(void)
{
	CJob* pJob = createJob(0, CJob::eJobType::SERVER_STOP_JOB, nullptr);

	mJobQueue.Enqueue(pJob);

	SetEvent(mUpdateEvent);

	return;
}

INT CChatServer::GetJobQueueUseSize(void) const
{
	return mJobQueue.GetUseSize();
}


void CChatServer::SetWhiteIPModeFlag(BOOL bWhiteIPModeFlag)
{
	mbWhiteIPModeFlag = bWhiteIPModeFlag;

	return;
}

BOOL CChatServer::GetWhiteIPModeFlag(void) const
{
	return mbWhiteIPModeFlag;
}


DWORD CChatServer::GetUpdateTPS(void) const
{
	return mUpdateTPS;
}

void CChatServer::InitializeChatTPS(void)
{
	InterlockedExchange(&mUpdateTPS, 0);

	return;
}

void CChatServer::jobProcedure(CJob* pJob)
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

void CChatServer::jobProcedureMessage(UINT64 sessionID, CMessage* pMessage)
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


CJob* CChatServer::createJob(UINT64 sessionID, CJob::eJobType jobType, CMessage* pMessage)
{
	CJob* pJob = CJob::Alloc();

	pJob->mSessionID = sessionID;

	pJob->mJobType = jobType;

	pJob->mpMessage = pMessage;

	return pJob;
}


BOOL CChatServer::setupUpdateThread(void)
{
	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)&mUpdateThreadID);
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[OnStart] _beginthreadex Error Code : %d", GetLastError());

		return FALSE;
	}

	return TRUE;
}


CPlayer* CChatServer::findPlayer(UINT64 sessionID)
{
	auto iter = mPlayerMap.find(sessionID);

	if (iter == mPlayerMap.end())
	{
		return nullptr;
	}

	return iter->second;
}


//CChatServer::CConnectionState* CChatServer::findConnectionState(UINT64 sessionID)
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


BOOL CChatServer::createPlayer(UINT64 acountNumber, UINT64 sessionID, CPlayer** pPlayer)
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

void CChatServer::deletePlayer(UINT64 sessionID)
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


void CChatServer::removeSector(CPlayer* pPlayer)
{
	// 아직 자리 셋팅 전이라면은 return
	if (pPlayer->mbSectorSetFlag == FALSE)
	{
		// 자리셋팅 Flag를 TRUE로 변경해준다.
		pPlayer->mbSectorSetFlag = TRUE;

		return;
	}

	mSectorArray[pPlayer->mSectorY][pPlayer->mSectorX].erase(pPlayer->mSessionID);

	return;
}

void CChatServer::addSector(CPlayer* pPlayer)
{
	mSectorArray[pPlayer->mSectorY][pPlayer->mSectorX].insert(std::pair<UINT64, CPlayer*>(pPlayer->mSessionID, pPlayer));

	return;
}


// Sector 음수 계산을 위해서 인자는 short를 받는다.
void CChatServer::getSectorAround(INT posY, INT posX, stSectorAround* pSectorAround)
{
	posY -= 1;
	posX -= 1;

	for (INT countY = 0; countY < 3; ++countY)
	{
		if (posY + countY < 0 || posY + countY >= chatserver::SECTOR_MAX_HEIGHT)
		{
			continue;
		}

		for (INT countX = 0; countX < 3; ++countX)
		{
			if (posX + countX < 0 || posX + countX >= chatserver::SECTOR_MAX_WIDTH)
			{
				continue;
			}

			pSectorAround->aroundSector[pSectorAround->count].posY = posY + countY;
			pSectorAround->aroundSector[pSectorAround->count].posX = posX + countX;

			++pSectorAround->count;
		}
	}
}


void CChatServer::sendOneSector(stSector* pSector, CMessage* pMessage)
{
	std::unordered_map<UINT64, CPlayer*>* pPlayerMap = &mSectorArray[pSector->posY][pSector->posX];

	// 범위기반 for문을 사용한다.
	for (auto& iter : *pPlayerMap)
	{
		//++gSendPacketAround;
		SendPacket(iter.first, pMessage);
	}

	return;
}

void CChatServer::sendAroundSector(stSectorAround* pSectorAround, CMessage* pMessage)
{
	//CPerformanceProfiler profiler(L"sendAroundSector");

	stSector* pSectorArray = pSectorAround->aroundSector;

	INT count = pSectorAround->count;

	for (INT index = 0; index < count; ++index)
	{
		sendOneSector(&pSectorArray[index], pMessage);
	}
}

BOOL CChatServer::recvProcedureLoginRequest(UINT64 sessionID, CMessage* pMessage)
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


void CChatServer::sendLoginResponse(bool bLoginFlag, CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	// 로그인 완료 메시지를 만든다.
	packingLoginResponseMessage(bLoginFlag, pPlayer->mAccountNumber, pMessage);

	// 로그인 완료 메시지를 응답한다.
	SendPacket(pPlayer->mSessionID, pMessage);

	pMessage->Free();

	return;
}




void CChatServer::packingLoginResponseMessage(bool bLoginFlag, UINT64 accountNumber, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << bLoginFlag << accountNumber;

	return;
}


// 섹터 이동 
BOOL CChatServer::recvProcedureSectorMoveRequest(UINT64 sessionID, CMessage* pMessage)
{
	//CPerformanceProfiler profiler(L"Move Request");

	// 공격 확인
	// 섹터 이동요청 패킷보다 작게 왔다면은 return FALSE
	if (pMessage->GetUseSize() != 12)
	{
		return FALSE;
	}

	UINT64 acountNumber;
	WORD sectorX;
	WORD sectorY;

	*pMessage >> acountNumber >> sectorX >> sectorY;


	// 로그인하지 않았는데 섹터 이동요청을 보내면은 끊는다.
	CPlayer* pPlayer = findPlayer(sessionID);
	//if (pPlayer == nullptr)
	//{	return FALSE;
	//}


	// 어카운트 넘버가 일치하는지 확인한다. 일치하지 않다면은 return FALSE
	//if (pPlayer->mAccountNumber != acountNumber)
	//{
	//	return FALSE;
	//}

	// unsigned 이기 때문에 SECTOR_MAX_WIDTH, SECTOR_MAX_HEIGHT 보다 큰지만 확인하면 된다.  
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

// 섹터 이동 결과 송신
void CChatServer::sendSectorMoveResponse(CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	packingSectorMoveMessage(pPlayer->mAccountNumber, pPlayer->mSectorX, pPlayer->mSectorY, pMessage);

	SendPacket(pPlayer->mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CChatServer::packingSectorMoveMessage(UINT64 accountNumber, WORD sectorX, WORD sectorY, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNumber << sectorX << sectorY;

	return;
}


// 채팅 메시지 요청
BOOL CChatServer::recvProcedureChatRequest(UINT64 sessionID, CMessage* pMessage)
{
	//CPerformanceProfiler profiler(L"Chat Request");

	// 어카운트 넘버와 채팅 길이 뽑을 수 없으면 return FALSE
	//if (pMessage->GetUseSize() < 10)
	//{
	//	return FALSE;
	//}

	UINT64 acountNumber;

	WORD chatLength;

	*pMessage >> acountNumber >> chatLength;

	// 아직 로그인하지 않았다면은 return FALSE;
	CPlayer* pPlayer = findPlayer(sessionID);
	//if (pPlayer == nullptr)
	//{
	//	return FALSE;
	//}

	// 공격 확인
	// 어카운트 넘버가 일치하지 않으면은 return FALSE;
	if (pPlayer->mAccountNumber != acountNumber)
	{
		return FALSE;
	}

	// 아직 섹터 셋팅을 하지 않았다면은 return FALSE
	/*if (pPlayer->mbSectorSetFlag == FALSE)
	{
		return FALSE;
	}*/

	// 남은 메시지 사이즈와 메시지 길이와 다르거나 채팅 메시지 길이를 초과하는 메시지가 왔다면 return FALSE
	/*if (pMessage->GetUseSize() != chatLength || chatLength > MAX_CHAT_LENGTH * sizeof(WCHAR))
	{
		return FALSE;
	}*/

	WCHAR pChat[chatserver::MAX_CHAT_LENGTH];

	pMessage->GetPayload((CHAR*)pChat, chatLength);

	pMessage->MoveReadPos(chatLength);

	sendChatResponse(chatLength, pChat, pPlayer);

	return TRUE;
}

void CChatServer::sendChatResponse(WORD chatLength, WCHAR* pChat, CPlayer* pPlayer)
{
	CMessage* pMessage = CMessage::Alloc();

	packingChatMessage(pPlayer->mAccountNumber, pPlayer->mPlayerID, pPlayer->mPlayerNickName, chatLength, pChat, pMessage);

	// 채팅 송신자 주변 섹터로 메시지를 뿌린다.
	sendAroundSector(&mSectorAround[pPlayer->mSectorY][pPlayer->mSectorX], pMessage);

	pMessage->Free();

	return;
}


void CChatServer::packingChatMessage(UINT64 accountNumber, WCHAR* pPlayerID, WCHAR* pPlayerNickName, WORD chatLength, WCHAR* pChat, CMessage* pMessage)
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

BOOL CChatServer::recvProcedureHeartbeatRequest(UINT64 sessionID)
{
	//CConnectionState* pConnectionState = findConnectionState(sessionID);
	//if (pConnectionState == nullptr)
	//{
	//	return FALSE;
	//}

	//pConnectionState->mTimeout = timeGetTime();

	return TRUE;
}

void CChatServer::jobProcedureClientJoin(UINT64 sessionID)
{
	//CConnectionState* pConnectionState = CConnectionState::Alloc();

	//mConnectionStateMap.insert(std::make_pair(sessionID, pConnectionState));

	return;
}




void CChatServer::jobProcedureClientLeave(UINT64 sessionID)
{
	//CConnectionState* pConnectionState = findConnectionState(sessionID);

	//// 로그인 처리중이기 때문에 플레이어 삭제없이 return 한다.
	//if (pConnectionState->mbLoginProcFlag == TRUE)
	//{
	//	pConnectionState->mbConnectionFlag = FALSE;

	//	return;
	//}
	//else
	//{
	//	mConnectionStateMap.erase(sessionID);

	//	pConnectionState->Free();
	//}

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

void CChatServer::jobProcedureServerStop(void)
{
	mbStopFlag = TRUE;

	if (mPlayerMap.empty() == TRUE)
	{
		mbUpdateLoopFlag = FALSE;
	}

	return;
}

// 메시지 프로토콜 
BOOL CChatServer::recvProcedure(UINT64 sessionID, DWORD messageType, CMessage* pMessage)
{
	switch (messageType)
	{
		// 로그인 요청 메시지
	case en_PACKET_CS_CHAT_REQ_LOGIN:

		return recvProcedureLoginRequest(sessionID, pMessage);

		// 섹터 이동 메시지
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:

		return recvProcedureSectorMoveRequest(sessionID, pMessage);

		// 채팅 메시지
	case en_PACKET_CS_CHAT_REQ_MESSAGE:

		return recvProcedureChatRequest(sessionID, pMessage);

		// 하트비트 메시지 ( 현재 사용 X )
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:

		return recvProcedureHeartbeatRequest(sessionID);

	default:

		// 공격 확인		
		return FALSE;
	}

	return TRUE;
}

