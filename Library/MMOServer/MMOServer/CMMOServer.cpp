#include "stdafx.h"
#include "CMMOServer.h"



CMMOServer::CMMOServer(void)
	: mListenSocket(INVALID_SOCKET)
    , mSessionArray()
	, mIocpHandle(INVALID_HANDLE_VALUE)
	, mReleaseSessionStack()
	, mAcceptInfoQueue()

	, mServerIP()
	, mServerPort(0)
	, mSendFrame(0)
	, mAuthFrame(0)
	, mUpdateFrame(0)
	, mMaxMessageSize(0)
	, mMaxClientCount(0)
	, mRunningThreadCount(0)
	, mWorkerThreadCount(0)
	, mbNagleFlag(true)
	, mTimeoutSec(0)

	, mWorkerThreadIndex(-1)
	, mWakeupPerSecond(0)
	, mpWakeupProcessTimeArray(nullptr)
	, mpWakeupWaitTimeArray(nullptr)
	, mMaxWakeupPerSecond(0)
	, mMaxWakeupProcessTime(0)

	, mAcceptTotal(0)
	, mCurrentClientCount(0)
	, mAuthClientCount(0)
	, mGameClientCount(0)
	, mAcceptTPS(0)
	, mUpdateTPS(0)
	, mAuthenticTPS(0)
	, mSendTPS(0)
	, mRecvTPS(0)


	, mbUpdateThreadFlag(true)
	, mbAutheticThreadFlag(true)
	, mbSendThreadFlag(true)	
	, mbSessionClearFlag(false)
	, mSessionClearEvent(CreateEvent(NULL, false, false, NULL))

	, mAcceptThreadID(0)
	, mAuthThreadID(0)
	, mUpdateThreadID(0)
	, mSendThreadID(0)
	, mpWorkerThreadIDArray(nullptr)
	, mAcceptThreadHandle(INVALID_HANDLE_VALUE)
	, mAuthenticThreadHandle(INVALID_HANDLE_VALUE)
	, mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	, mSendThreadHandle(INVALID_HANDLE_VALUE)
	, mpWorkerThreadHandleArray(nullptr)
{
	WSADATA wsa;

	if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}
}

CMMOServer::~CMMOServer(void)
{
	WSACleanup();
}


CMMOServer::CSession::CSession(void)
	: mSessionMode(eSessionMode::NoneMode)
	, mSessionID(0)
	, mIndex(0)
	, mMsgRecvTime(0)

	, mSocket(INVALID_SOCKET)
	, mClientAddr{ 0, }
	, mSendOverlapped{ 0, }
	, mRecvOverlapped{ 0, }

	, mbSendFlag(false)
	, mIoCount(0)

	, mbAuthToGameFlag(false)
	, mbDisconnectFlag(false)
	, mbLogoutFlag(false)
	, mbReleaseFlag(false)


	, mMessageCount(0)
	, mSendCompletionArray{ 0, }
	, mRecvRingBuffer()
	, mRecvCompletionQueue()
	, mSendQueue()
{
	// 고정된 Session Index 번호
	static int sessionIndex;

	mIndex = sessionIndex;

	++sessionIndex;
}


void CMMOServer::CSession::SendPacket(CMessage* pMessage)
{
	// 패킷 인코딩
	pMessage->encode();

	// 패킷에 대한 참조카운트 증가
	pMessage->AddReferenceCount();

	// Send Q에 Enqueue
	if (mSendQueue.Enqueue(pMessage) == false)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"SendQueue is Full, sessionID : %lld", mSessionID);

		Disconnect();
	}

	// send 는 주기적으로 sendThread 가 한다.
	return;
}


void CMMOServer::CSession::Disconnect(void)
{
	if (mbDisconnectFlag == false)
	{
		mbDisconnectFlag = true;

		CancelIoEx((HANDLE)mSocket, nullptr);
	}

	return;
}

void CMMOServer::CSession::SetAuthToGame()
{
	mbAuthToGameFlag = true;

	return;
}


void CMMOServer::CSession::SetRelease()
{
	mbReleaseFlag = true;

	return;
}

bool CMMOServer::CSession::GetAuthToGameFlag(void) const
{
	return mbAuthToGameFlag;
}

bool CMMOServer::CSession::CheckLogoutInAuth(void) const
{
	return mSessionMode == eSessionMode::LogoutInAuthMode;
}

bool CMMOServer::CSession::CheckLogoutInGame(void) const
{
	return mSessionMode == eSessionMode::AuthToGameMode;
}

void CMMOServer::CSession::GetClientIP(WCHAR* pIP, DWORD bufferCch) const
{
	InetNtopW(AF_INET, &mClientAddr.sin_addr, pIP, bufferCch);

	return;
}

WORD CMMOServer::CSession::GetClientPort(void) const
{
	return ntohs(mClientAddr.sin_port);
}


// WSASend한 Message 들을 Free 해준다.
void CMMOServer::CSession::removeSendCompletionArray(void)
{
	int messageCount = mMessageCount;

	mMessageCount = 0;

	// 완료통지를 받아야할 mSendCompletionArray에 남아있는 메시지들을 정리한다.
	CMessage** pMessageArray = mSendCompletionArray;

	for (int index = 0; index < messageCount; ++index)
		pMessageArray[index]->Free();

	return;
}

// SendRingBuffer에 남아있는 Message 들을 Free 해준다.
void CMMOServer::CSession::removeSendQueue(void)
{
	int useSize = mSendQueue.GetUseSize();
	if (useSize == 0)
		return;

	CMessage** pMessageArray = mSendQueue.GetFrontBufferPtr();

	int directDeqSize = mSendQueue.GetDirectDequeueSize();
	for (int index = 0; index < directDeqSize; ++index)
		pMessageArray[index]->Free();


	pMessageArray = mSendQueue.GetBufferPtr();

	int remainSize = useSize - directDeqSize;
	for (int index = 0; index < remainSize; ++index)
		pMessageArray[index]->Free();

	mSendQueue.MoveFront(directDeqSize + remainSize);
	
	return;
}

void CMMOServer::CSession::removeRecvCompletionQueue(void)
{
	int useSize = mRecvCompletionQueue.GetUseSize();
	if (useSize == 0)
		return;

	CMessage** pMessageArray = mRecvCompletionQueue.GetFrontBufferPtr();

	int directDeqSize = mRecvCompletionQueue.GetDirectDequeueSize();
	for (int index = 0; index < directDeqSize; ++index)
		pMessageArray[index]->Free();

	pMessageArray = mRecvCompletionQueue.GetBufferPtr();

	int remainSize = useSize - directDeqSize;
	for (int index = 0; index < remainSize; ++index)
		pMessageArray[index]->Free();

	mRecvCompletionQueue.MoveFront(directDeqSize + remainSize);

	return;
}


// 오픈 IP / 포트 / 네이글 옵션 / TimeoutFlag / headerCode / staticKey/ runningThread/ WorkerThread 개수 / 최대접속자 수
bool CMMOServer::Start(const WCHAR* pServerIP, WORD serverPort, DWORD sendFrame, DWORD updateFame, DWORD authenticFrame, DWORD maxMessageSize, bool bNagleFlag, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount, DWORD timeoutSec)
{
	if (pServerIP == nullptr || serverPort > USHRT_MAX || sendFrame < 1 || updateFame < 1 || authenticFrame < 1 
		|| runningThreadCount < 1 || workerThreadCount < 1 || maxClientCount < 1 || timeoutSec < 0)
		return false;

	mServerIP = pServerIP;
	mServerPort = serverPort;
	
	mSendFrame = sendFrame;
	mUpdateFrame = updateFame;
	mAuthFrame = authenticFrame;

	mMaxMessageSize = maxMessageSize;

	mRunningThreadCount = runningThreadCount;
	mWorkerThreadCount = workerThreadCount;

	mMaxClientCount = maxClientCount;

	CMessage::mHeaderCode = headerCode;
	CMessage::mStaticKey = staticKey;

	mbNagleFlag = bNagleFlag;
	mTimeoutSec = timeoutSec * 1000;

	if (setupMMOServerNetwork() == false)
		return false;

	if (setupMMOServerThread() == false)
		return false;

	if (OnStart() == false)
		return false;

	return true;
}

bool CMMOServer::Stop(void)
{
	// AcceptThread를 Close
	if (closeAcceptThread() == false)
		return false;

	// 모든 세션들을 Disconnect 처리 
	kickSessions();
	
	// 인증 스레드 Close
	closeAutenticThread();

	// UpdateThread Close
	closeUpdateThread();

	// SendThread Close
	closeSendThread();

	// WorkerThread Close
	closeWorkerThread();

	// Close IOCP
	closeIOCP();

	// 스레드들이 Session 들을 직접 순회하기 때문에 OnStop을 나중에 호출해서 Player 들을 정리
	OnStop();

	// 모든 해제
	cleanupSessionArray();

	return true;
}


UINT64 CMMOServer::GetAcceptTotal(void) const
{
	return mAcceptTotal;
}

int CMMOServer::GetAcceptTPS(void) const
{
	return mAcceptTPS;
}

int CMMOServer::GetUpdateTPS(void) const
{
	return mUpdateTPS;
}

int CMMOServer::GetAuthenticTPS(void) const
{
	return mAuthenticTPS;
}

int CMMOServer::GetSendTPS(void) const
{
	return mSendTPS;
}

int CMMOServer::GetRecvTPS(void) const
{
	return mRecvTPS;
}


int CMMOServer::GetMaxClientCount(void) const
{
	return mMaxClientCount;
}

int CMMOServer::GetCurrentClientCount(void) const
{
	return mCurrentClientCount;
}

int CMMOServer::GetRunningThreadCount(void) const
{
	return mRunningThreadCount;
}

int CMMOServer::GetWorkerThreadCount(void) const
{
	return mWorkerThreadCount;
}

const WCHAR* CMMOServer::GetServerBindIP(void) const
{
	return mServerIP.c_str();
}

WORD CMMOServer::GetServerBindPort(void) const
{
	return mServerPort;
}

bool CMMOServer::GetNagleFlag(void) const
{
	return mbNagleFlag;
}

int CMMOServer::GetAuthClientCount(void) const
{
	return mAuthClientCount;
}

int CMMOServer::GetGameClientCount(void) const
{
	return mGameClientCount;
}

DWORD CMMOServer::GetWakeupProcessTime(void) const
{
	DWORD maxProcessTime = 0;

	for (int count = 0; count <= mWorkerThreadIndex; ++count)
	{
		DWORD processTime = mpWakeupProcessTimeArray[count];

		if (maxProcessTime < processTime)
			maxProcessTime = processTime;
	}

	return maxProcessTime;
}

DWORD CMMOServer::GetMaxWakeupWaitTime(void) const
{
	DWORD nowTime = timeGetTime();

	DWORD maxWaitTime = UINT_MAX;

	for (int idx = 0; idx <= mWorkerThreadIndex; ++idx)
	{
		DWORD waitTime = mpWakeupWaitTimeArray[idx];

		if (maxWaitTime > waitTime)
			maxWaitTime = waitTime;
	}

	return nowTime - maxWaitTime;
}


DWORD CMMOServer::GetMaxWakeupProcessTime(void) const
{
	return mMaxWakeupProcessTime;
}

int CMMOServer::GetMaxWakeupPerSecond(void) const
{
	return mMaxWakeupPerSecond;
}

int CMMOServer::GetWakeupPerSecond(void)
{
	int wakeupPerSecond = mWakeupPerSecond;

	if (mMaxWakeupPerSecond < wakeupPerSecond)
		mMaxWakeupPerSecond = wakeupPerSecond;

	InterlockedExchange((long*)&mWakeupPerSecond, 0);

	return wakeupPerSecond;
}

void CMMOServer::InitializeTPS(void)
{
	InterlockedExchange((long*)&mAcceptTPS, 0);
	InterlockedExchange((long*)&mAuthenticTPS, 0);
	InterlockedExchange((long*)&mUpdateTPS, 0);
	InterlockedExchange((long*)&mRecvTPS, 0);
	InterlockedExchange((long*)&mSendTPS, 0);

	return;
}

DWORD CMMOServer::ExecuteAcceptThread(void* pParam)
{
	srand((UINT)time(NULL));

	((CMMOServer*)pParam)->AcceptThread();

	return 1;
}

DWORD CMMOServer::ExecuteAuthenticThread(void* pParam)
{
	srand((UINT)time(NULL));

	((CMMOServer*)pParam)->AuthenticThread();

	return 1;
}


DWORD CMMOServer::ExecuteUpdateThread(void* pParam)
{
	srand((UINT)time(NULL));

	((CMMOServer*)pParam)->UpdateThread();

	return 1;
}

DWORD CMMOServer::ExecuteSendThread(void* pParam)
{
	srand((UINT)time(NULL));

	((CMMOServer*)pParam)->SendThread();

	return 1;
}


DWORD CMMOServer::ExecuteWorkerThread(void* pParam)
{
	srand((UINT)time(NULL));

	((CMMOServer*)pParam)->WorkerThread();

	return 1;
}


void CMMOServer::AcceptThread(void)
{
	for (;;)
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr = { 0, };
		INT addrlen = sizeof(clientAddr);

		clientSocket = accept(mListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSocket == INVALID_SOCKET)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[AcceptThread] Accept, Error Code : %d", WSAGetLastError());

			break;
		}

		++mAcceptTotal;

		InterlockedIncrement((long*)&mAcceptTPS);

		WCHAR clientIP[50];
		InetNtopW(AF_INET, &clientAddr.sin_addr, clientIP, _countof(clientIP));
		if (OnConnectionRequest(clientIP, ntohs(clientAddr.sin_port)) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[AcceptThread] Black IP : %s", clientIP);

			closesocket(clientSocket);

			continue;
		}

		if (mCurrentClientCount >= mMaxClientCount)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[AcceptThread] CurrentClient is full, Max Client : %d, Current Client : %d", mMaxClientCount, mCurrentClientCount);

			closesocket(clientSocket);

			continue;
		}

		// 접속자 수 증가
		InterlockedIncrement((long*)&mCurrentClientCount);

		stAcceptInfo acceptInfo;

		acceptInfo.clientSocket = clientSocket;

		acceptInfo.clientAddr = clientAddr;

		if (mAcceptInfoQueue.Enqueue(acceptInfo) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[AcceptThread] AcceptInfo Enqueue is Error");

			CCrashDump::Crash();
		}
	}

	return;
}



void CMMOServer::AuthenticThread(void)
{
	OnStartAuthenticThread();

	int sleepTime = 1000 / mAuthFrame;

	while (mbAutheticThreadFlag == true)
	{
		// 신규 접속자 Dequeue
		checkAcceptInfo();

		// Auth 모드 세션 메시지 처리
		checkMessageOfAuthMode();

		authSessionTimeout();

		// 게임 스타일에 맞게 호출한다.
		// 캐릭터 생성 후 DB쓰기 요청 후 바로 새로운 유저가 들어왔다면은 아직 DB에 쓰는 작업을 하는 중이기 때문에 생성했던 캐릭터에 대한 정보를 DB에서 못 읽게 됨
		// 해당 처리를 OnAuthenticUpdate에서 처리하자
		OnAuthUpdate();

		InterlockedIncrement((long*)&mAuthenticTPS);

		// LogoutFlag가 true인 Auth 모드 세션을 확인 후 LogoutInAuth 모드로 변경
		checkLogoutFlagOfAuthMode();

		// LogoutInAuthMode에서 WaitLogoutMode으로 전환 최종 처리는 UpdateThread에서 함
		checkLogoutInAuthMode();

		checkLeaveAuthToGameMode();

		Sleep(sleepTime);
	}

	OnCloseAuthenticThread();

	return;
}


void CMMOServer::UpdateThread(void)
{
	OnStartUpdateThread();

	int sleepTime = 1000 / mUpdateFrame;

	while (mbUpdateThreadFlag == true)
	{
		// AuthToGameMode 세션들을 대상으로 OnGameClienJoint(); 호출 및 GameMode 전환
		joinAuthToGameMode();

		// GameMode 세션들의 Recv 패킷 처리
		checkMessageOfGameMode();

		// timeout
		updateSessionTimeout();

		// 클라이언트의 요청 ( 패킷 수신 ) 외에 기본적으로 항시 처리 되어야 할 게임 컨텐츠 부분 로직 OnGameUpdate() 호출
		OnGameUpdate();

		InterlockedIncrement((long*)&mUpdateTPS);

		// mbLogoutFlag가 true이면서 GameMode를 대상으로 LogoutInGame 모드 전환
		checkLogoutFlagOfGameMode();

		// SendFlag를 통해 현재 Send중이 아닌것을 확인 후 WaitLogout 모드로 전환
		checkLogoutInGameMode();

		// ReleaseFlag와 WaitLogout 모드를 확인 후 releaseSession 처리
		checkWaitLogoutMode();

		Sleep(sleepTime);
	}

	OnCloseUpdateThread();

	return;
}


// WorkerThread로 변경 가능할듯
// ModeAuth, ModeGame 상태인 세션의 mSendQueue에 보낼것이 있다면 무작정 WSASend 를 요청한다.
// 적당히 Sleep() 을 걸면 된다.10ms 정도? 게임 스타일에 맞게
void CMMOServer::SendThread(void)
{
	int sleepTime = 1000 / mSendFrame;

	while (mbSendThreadFlag == true)
	{
		for (int index = 0; index < mMaxClientCount; ++index)
		{
			if (mSessionArray[index]->mbSendFlag == true)
			{
				if (mSessionArray[index]->mSessionMode == eSessionMode::GameMode || mSessionArray[index]->mSessionMode == eSessionMode::AuthMode)
					sendPost(mSessionArray[index]);
			}
		}

		Sleep(sleepTime);
	}

	return;
}



void CMMOServer::WorkerThread(void)
{
	DWORD processCount = 0;

	DWORD processTotalTime = 0;

	int threadIndex = InterlockedIncrement((long*)&mWorkerThreadIndex);

	DWORD initializeTime = timeGetTime();

	for (;;)
	{
		DWORD transferred = 0;

		// CompletionKey는 필수적으로 nullptr 처리
		CSession* pSession = nullptr;

		// Overapped 필수적으로 nullptr 처리
		LPOVERLAPPED pOverlapped = nullptr;

		mpWakeupWaitTimeArray[threadIndex] = timeGetTime();

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		InterlockedIncrement((long*)&mWakeupPerSecond);

		DWORD startTime = timeGetTime();

		// WorkerThread 종료
		if (pOverlapped == nullptr)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[WorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

			break;
		}

		if (transferred != 0 && pSession->mbDisconnectFlag == false)
		{
			if (&pSession->mRecvOverlapped == pOverlapped)
				checkCompletionRecv(pSession, transferred);
			else if (&pSession->mSendOverlapped == pOverlapped)
				checkCompletionSend(pSession);
			else
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[WorkerThread] Exception Overapped, transffered : %d, Error Code : %d\n", transferred, WSAGetLastError());
		}

		DWORD errorCode = WSAGetLastError();
		if (errorCode != ERROR_SUCCESS)
		{
			if (isLoggingErrorCode(errorCode) == true)
				CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[WorkerThread] Error Code : %d", errorCode);
		}

		if (InterlockedDecrement(&pSession->mIoCount) == 0)
		{
			pSession->mbSendFlag = true;

			pSession->mbLogoutFlag = true;
		}

		DWORD nowTime = timeGetTime();

		processTotalTime += nowTime - startTime;
		++processCount;

		if (nowTime - initializeTime > 1000)
		{
			mpWakeupProcessTimeArray[threadIndex] = processTotalTime / processCount;

			if (mMaxWakeupProcessTime < mpWakeupProcessTimeArray[threadIndex])
				mMaxWakeupProcessTime = mpWakeupProcessTimeArray[threadIndex];

			processTotalTime = 0;
			processCount = 0;
			initializeTime = timeGetTime();
		}
	}

	return;
}



bool CMMOServer::setupMMOServerNetwork(void)
{
	// 리슨 소켓 생성
	if (setupListenSocket() == false)
		return false;

	// 소켓 바인딩
	if (bindListenSocket() == false)
		return false;

	// 리슨
	if (listenSocket() == false)
		return false;

	// IOCP 셋팅
	if (setupIOCP() == false)
		return false;

	// Player 셋팅
	setupSessionPointerArray();

	return true;
}


bool CMMOServer::setupListenSocket(void)
{
	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] socket Error Code : %d", WSAGetLastError());

		return false;
	}

	// KeepAlive 옵션
	// Window regedit 에서 keepalive 시간 줄일 수 있음
	int enable = 1;
	int retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	////// keepalive 기능을 동작 시키기 위한 timeout 값
	//INT keepidle = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPIDLE, (CHAR*)&keepidle, sizeof(keepidle));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] TCP_KEEPIDLE setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}

	//////keepalive probe packet을 보내는 갯수
	//INT keepcnt = 5;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPCNT,(CHAR*)&keepcnt, sizeof(keepcnt));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] TCP_KEEPCNT setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}
	//

	//////probe packet을 보내는 간격
	//INT keepintvl = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPINTVL, (CHAR*)&keepintvl, sizeof(keepintvl));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] TCP_KEEPINTVL setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}

	if (mbNagleFlag == false)
	{
		int optval = true;
		retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] Nagle No Delay setsockopt Error Code : %d", WSAGetLastError());

			return false;
		}
	}


	// RST 옵션
	linger optval = { 0, };
	optval.l_onoff = 1;
	optval.l_linger = 0;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	//송신버퍼 0으로 만들기
	//int sendLen = 0;
	//retval = setsockopt(mListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendLen, sizeof(sendLen));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] Zero setsockopt Error Code : %d", WSAGetLastError());
	//	return false;
	//}

	return true;
}



bool CMMOServer::bindListenSocket(void)
{
	SOCKADDR_IN serverAddr = { 0, };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mServerPort);
	InetPtonW(AF_INET, mServerIP.c_str(), &serverAddr.sin_addr);

	int retval = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[bindListenSocket] bind Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}



bool CMMOServer::listenSocket(void)
{
	int retval = listen(mListenSocket, SOMAXCONN_HINT(65535));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[listenSocket] listen Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}


bool CMMOServer::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupIOCP] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}

void CMMOServer::setupSessionPointerArray(void)
{
	mSessionArray.resize(mMaxClientCount);

	for (int index = 0; index < mMaxClientCount; ++index)
	{
		OnAssociateSessionWithPlayer(&mSessionArray[index]);

		mReleaseSessionStack.Push(mSessionArray[index]->mIndex);
	}

	return;
}


bool CMMOServer::setupMMOServerThread(void)
{
	if (setupAcceptThread() == false)
		return false;

	if (setupWorkerThread() == false)
		return false;

	if (setupSendThread() == false)
		return false;

	if (setupAuthenticThread() == false)
		return false;

	if (setupUpdateThread() == false)
		return false;

	return true;
}


bool CMMOServer::setupAcceptThread(void)
{
	mAcceptThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteAcceptThread, this, NULL, (UINT*)&mAcceptThreadID);
	if (mAcceptThreadHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupAcceptThread] _beginthreadex Error Code : % d", WSAGetLastError());

		return false;
	}

	return true;
}

bool CMMOServer::setupAuthenticThread(void)
{
	mAuthenticThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteAuthenticThread, this, NULL, (UINT*)&mAuthThreadID);
	if (mAuthenticThreadHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupAuthenticThread] _beginthreadex Error Code : % d", WSAGetLastError());

		return false;
	}

	return true;
}

bool CMMOServer::setupUpdateThread(void)
{
	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)&mUpdateThreadID);
	if (mUpdateThreadHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupUpdateThread] _beginthreadex Error Code : % d", WSAGetLastError());

		return false;
	}

	return true;
}


bool CMMOServer::setupSendThread(void)
{
	mSendThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteSendThread, this, NULL, (UINT*)&mSendThreadID);
	if (mSendThreadHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSendThread] _beginthreadex Error Code : % d", WSAGetLastError());

		return false;
	}

	return true;
}



bool CMMOServer::setupWorkerThread(void)
{
	mpWorkerThreadIDArray = new DWORD[mWorkerThreadCount];

	mpWorkerThreadHandleArray = new HANDLE[mWorkerThreadCount];

	mpWakeupProcessTimeArray = new DWORD[mWorkerThreadCount];
	ZeroMemory(mpWakeupProcessTimeArray, sizeof(DWORD) * mWorkerThreadCount);

	mpWakeupWaitTimeArray = new DWORD[mWorkerThreadCount];
	ZeroMemory(mpWakeupWaitTimeArray, sizeof(DWORD) * mWorkerThreadCount);

	for (DWORD index = 0; index < mWorkerThreadCount; ++index)
	{
		mpWorkerThreadHandleArray[index] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteWorkerThread, this, NULL, (UINT*)&mpWorkerThreadIDArray[index]);
		if (mpWorkerThreadHandleArray[index] == NULL)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupWorkerThread] _beginthreadex Error Code : % d", WSAGetLastError());

			return false;
		}
	}

	return true;
}


void CMMOServer::initializeSessions(stAcceptInfo* pAcceptInfoArray, DWORD arraySize)
{
	if (pAcceptInfoArray == nullptr)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[initializeSessions] pAcceptInfoArray is nullptr");

		CCrashDump::Crash();

		return;
	}

	for (int index = 0; index < arraySize; ++index)
	{
		static UINT64 sessionID = 0;

		int sessionIdx;

		if (mReleaseSessionStack.Pop(&sessionIdx) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[initializeSessions] sessionIdx Pop Failed");

			CCrashDump::Crash();

			return;
		}

		CSession* pSession = mSessionArray[sessionIdx];

		// OnAuthClientJoin() 에서 Send를 할 수 있기 때문에 증가시켜야 한다.
		InterlockedIncrement(&pSession->mIoCount);
		
		pSession->mSessionID = sessionID++;
		
		pSession->mSocket = pAcceptInfoArray[index].clientSocket;

		pSession->mClientAddr = pAcceptInfoArray[index].clientAddr;

		pSession->mbDisconnectFlag = false;

		pSession->mbLogoutFlag = false;

		pSession->mbReleaseFlag = false;

		pSession->mbAuthToGameFlag = false;

		pSession->mbSendFlag = true;

		pSession->mMsgRecvTime = timeGetTime();

		// IOCP 핸들과 소켓 연결 그리고 completionKey 연결
		associateSocketWithIOCP(pSession->mSocket, pSession);

		// AuthClient에 들어왔음을 게임쪽에 알림
		pSession->OnAuthClientJoin();

		// OnAuthClientJoin()을 호출시킨 후 AuthMode로 변경해준다.
		pSession->mSessionMode = eSessionMode::AuthMode;

		// AuthMode Client 증가
		++mAuthClientCount;

		recvPost(pSession);

		if (InterlockedDecrement(&pSession->mIoCount) == 0)
			pSession->mbLogoutFlag = true;
	}

	return;
}

void CMMOServer::associateSocketWithIOCP(SOCKET socket, void* completionKey)
{
	if (CreateIoCompletionPort((HANDLE)socket, mIocpHandle, (ULONG_PTR)completionKey, NULL) == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[associateSocketWithIOCP] CreateIoCompletionPort Error Code : % d", WSAGetLastError());

		CCrashDump::Crash();
	}

	return;
}


void CMMOServer::releaseSession(CSession* pSession)
{
	pSession->mSessionMode = eSessionMode::NoneMode;

	closesocket(pSession->mSocket);

	// WSASend한 Message 들을 Free 해준다.
	pSession->removeSendCompletionArray();

	// SendRingBuffer에 남아있는 Message 들을 Free 해준다.
	pSession->removeSendQueue();

	// RecvCompletionRingBuffer에 남아있는 메시지들을 Free 해준다.
	pSession->removeRecvCompletionQueue();

	// Recv 남은 버퍼 정리
	pSession->mRecvRingBuffer.ClearBuffer();

	// 정리가 완료된  Session을 mReleaseSessionQueue에 Enqeueu 한다.
	mReleaseSessionStack.Push(pSession->mIndex);

	InterlockedDecrement((long*)&mCurrentClientCount);

	// 전체 Session 끊기 위한 플래그
	if (mbSessionClearFlag == true)
	{
		if (mCurrentClientCount == 0)
			SetEvent(mSessionClearEvent);
	}

	return;
}


void CMMOServer::recvPost(CSession* pSession)
{
	int wsabufSize = 1;

	WSABUF wsabufRecv[2];

	// 자주 접근하는 메멉변수에 대해서는 지역변수로 접근하도록 한다.
	CRingBuffer* pRingBuffer = &pSession->mRecvRingBuffer;

	// GetDirectEnqueueSize 보다 먼저 읽어야 함 
	int freeSize = pRingBuffer->GetFreeSize();
	if (freeSize == 0)
	{
		CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[recvPost] mRecvRingBuffer is Full");

		pSession->Disconnect();

		return;
	}

	// 링 버퍼가 끊어지점까지의 길이
	int directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	// rear 시작점
	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = directEnqueueSize;

	int remainSize = freeSize - directEnqueueSize;
	if (remainSize >= 0)
	{
		wsabufSize += 1;

		// 버퍼의 시작점
		wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
		wsabufRecv[1].len = remainSize;
	}

	DWORD flags = 0;
	DWORD recvBytes = 0;

	// IOCount 증가
	InterlockedIncrement(&pSession->mIoCount);

	// Overapped 초기화 필수
	ZeroMemory(&pSession->mRecvOverlapped, sizeof(OVERLAPPED));

	// mSocket 이 넌블락 소켓이여도 수신 버퍼에 데이터가 없을 때 WOULDBLOCK이 아닌 WSA_IO_PENDING 이 에러가 발생된다.
	if (WSARecv(pSession->mSocket, wsabufRecv, wsabufSize, &recvBytes, &flags, &pSession->mRecvOverlapped, nullptr) == SOCKET_ERROR)
	{
		DWORD errorCode = WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{
			if (isLoggingErrorCode(errorCode) == true)
				CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[recvPost] Error Code : %d", errorCode);

			if (InterlockedDecrement(&pSession->mIoCount) == 0)
				pSession->mbLogoutFlag = true;
		}
	}

	if (pSession->mbDisconnectFlag == true)
		CancelIoEx((HANDLE)pSession->mSocket, &pSession->mRecvOverlapped);

	return;
}


void CMMOServer::sendPost(CSession* pSession)
{
	if (pSession->mbLogoutFlag == true)
		return;

	// SendFlag를 true 로 변경하는 스레드와 false 로 변경하는 스레드는 1개 씩 있는 상황에서 굳이 Interlocked가 필요할까?
	// AuthenticThread, UpdateThread 에서 WaitLogoutMode로 변경 전 SendFlag를 true에서 false 로 변경하면서 판단하기 때문에 Interlocked 이 필요하다.
	if (InterlockedExchange8((char*)&pSession->mbSendFlag, false) == false)
		return;


	// ((bool)InterlockedExchange((LONG*)&pSession->mbSendFlag, false) 보다 밑에 있어야 한다. 만약 위에 있다면 현재 끊김으로 인해 정리중인
	// 세션을 대상으로 GetUseSize(); 를 호출하게 될 터이고 Clear(); 함수 내부에서는 mFront와 mRear를 각각 초기화해주는 코드로 인해서 GetUseSize(); 값이 잘못 될 수 있다.
	CTemplateQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

	int useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
	{
		// useSize가 0일 경우 mbSendFlag를 다시 true로 변경해주고 return 해주어야 한다.
		pSession->mbSendFlag = true;

		return;
	}

	WSABUF wsabufSend[mmoserver::MAX_WSABUF_SIZE];
	if (useSize > mmoserver::MAX_WSABUF_SIZE)
		useSize = mmoserver::MAX_WSABUF_SIZE;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	pSendQueue->Dequeue(pSendCompletionArray, useSize);
	for (int index = 0; index < useSize; ++index)
	{
		wsabufSend[index].buf = pSendCompletionArray[index]->GetMessagePtr();
		wsabufSend[index].len = pSendCompletionArray[index]->GetUseSize();
	}

	pSession->mMessageCount = useSize;

	InterlockedIncrement(&pSession->mIoCount);

	// Overapped는 필수적으로 초기화
	ZeroMemory(&pSession->mSendOverlapped, sizeof(OVERLAPPED));

	int retval = WSASend(pSession->mSocket, wsabufSend, useSize, nullptr, 0, &pSession->mSendOverlapped, nullptr);
	if (retval == SOCKET_ERROR)
	{
		DWORD errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			if (isLoggingErrorCode(errorCode) == true)
				CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[sendPost] Error Code : %d", errorCode);

			if (InterlockedDecrement(&pSession->mIoCount) == 0)
			{
				// UpdateThread, AuthenticThread 에서 WaitLogoutMode로 변경하기 위해서 InterlockedExchange로 SendFlag를 true에서 false로
				// 변경하면서 체크하기 때문에 SendFlag를 true로 변경해주어야 한다.	
				pSession->mbSendFlag = true;

				// mbLogoutFlag 가 true 인 Session에 대해서는 SendThread가 WSASend() 호출이 되지 않는다.
				pSession->mbLogoutFlag = true;
			}
		}
	}

	// SendTPS 측정
	InterlockedAdd((long*)&mSendTPS, useSize);

	return;
}

void CMMOServer::checkCompletionRecv(CSession* pSession, DWORD transferred)
{
	CRingBuffer* pRecvRingBuffer = &pSession->mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		int recvSize = pRecvRingBuffer->GetUseSize();

		// 헤더 사이즈가 만큼 recv 하였는지 확인
		if (recvSize < sizeof(CMessage::stNetHeader))
			break;

		CMessage::stNetHeader header;
		if (pRecvRingBuffer->Peek((CHAR*)&header, sizeof(header)) != sizeof(header))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		// 1Byte headCode 확인 
		if (header.code != CMessage::mHeaderCode)
		{
			pSession->Disconnect();

			break;
		}

		// Start 함수 호출 시 정해준 최대 Message 사이즈를 초과할 경우 끊는다.
		if (header.payloadSize > mMaxMessageSize)
		{
			pSession->Disconnect();

			break;
		}


		// 헤더 + 페이로드 들어왔는지 확인
		if (recvSize < sizeof(CMessage::stNetHeader) + header.payloadSize)
			break;


		// header 사이즈 만큼 링버퍼 front를 옮긴다.
		pRecvRingBuffer->MoveFront(sizeof(CMessage::stNetHeader));

		// 패킷 생성
		CMessage* pMessage = CMessage::Alloc();

		// 링버퍼에 있는 수신 데이터를 패킷에 셋팅
		if (pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize) != header.payloadSize)
		{
			// 내 링버퍼의 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		// 패킷 페이로드 사이즈만큼 링버퍼 front를 옮긴다.
		if (pMessage->MoveWritePos(header.payloadSize) != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] Message Error Retval");

			CCrashDump::Crash();
		}

		// 디코딩
		if (pMessage->decode(&header) == false)
		{
			pSession->Disconnect();

			pMessage->Free();

			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] decode return false");

			return;
		}

		pSession->mMsgRecvTime = timeGetTime();

		// mRecvCompletionQueue 에 Enqueue 시 다른 스레드에서 해당 메시지를 참조할 수 있기 때문에 참조 카운트를 증가시켜야 한다.
		pMessage->AddReferenceCount();

		// 완성된 패킷은 mRecvCompletionQueue 에 Enqueue하며, 이는 AuthenticThread, UpdateThread 에서 처리한다.
		if (pSession->mRecvCompletionQueue.Enqueue(pMessage) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] Enqueue Failed");

			CCrashDump::Crash();
		}

		// RecvTPS 측정
		InterlockedIncrement((long*)&mRecvTPS);

		pMessage->Free();
	}

	recvPost(pSession);

	return;
}


// 수신에 대한 완료통지 처리
void CMMOServer::checkCompletionSend(CSession* pSession)
{
	// 반복문에서 pSession 포인터를 이용해서 mMessageCount를 지속적으로 접근하기 때문에 mMessageCount는 지역에 복사한다.
	int messageCount = pSession->mMessageCount;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	// 완료통지를 받은 후 mSendCompletionArray 배열에 있는 패킷을 반환 정리한다.
	for (int index = 0; index < messageCount; ++index)
		pSendCompletionArray[index]->Free();

	pSession->mMessageCount = 0;

	// SendFlag를 true로 변경한다.
	pSession->mbSendFlag = true;

	return;
}


void CMMOServer::checkAcceptInfo(void)
{
	DWORD directDeqSize = mAcceptInfoQueue.GetDirectDequeueSize();
	if (directDeqSize <= 0)
		return;

	initializeSessions(mAcceptInfoQueue.GetFrontBufferPtr(), directDeqSize);

	mAcceptInfoQueue.MoveFront(directDeqSize);

	return;
}



// 세션 배열을 모두 돌면서 AuthMode 세션을 확인하여 해당 세션의 Completion Recv Message를 OnAuthMessage()를 호출하여 처리요청
void CMMOServer::checkMessageOfAuthMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::AuthMode)
		{
			CTemplateQueue<CMessage*>* pRecvCompletionQueue = &mSessionArray[index]->mRecvCompletionQueue;

			int useSize = pRecvCompletionQueue->GetUseSize();
			if (useSize == 0)
				continue;

			for (int msgIdx = 0; msgIdx < useSize; ++msgIdx)
			{
				CMessage* pMessage;
				pRecvCompletionQueue->Dequeue(&pMessage);

				try
				{
					mSessionArray[index]->OnAuthMessage(pMessage);
				}
				catch (const message::CExceptionObject& exp)
				{
					CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkMessageOfAuthMode] Exp Error, sessionID : %lld",mSessionArray[index]->mSessionID);

					exp.PrintExceptionData();

					mSessionArray[index]->Disconnect();

					pMessage->Free();
				
					break;
				}

				pMessage->Free();
			}
		}
	}

	return;
}


void CMMOServer::authSessionTimeout(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::AuthMode && mTimeoutSec < timeGetTime() - mSessionArray[index]->mMsgRecvTime)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[authSessionTimeout] sessionID : %lld, Timeout : %d", mSessionArray[index]->mSessionID, mSessionArray[index]->mMsgRecvTime);

			mSessionArray[index]->Disconnect();		
		}
	}

	return;
}


// 세션 배열을 돌면서 AuthMode에서 LogoutFlag가 true인 Session을 대상으로 ModeLogoutInAuth 로 변경한다.
void CMMOServer::checkLogoutFlagOfAuthMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		// logoutFlag가 true이면은 LogoutInAuthMode로 전환
		if (mSessionArray[index]->mbLogoutFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::AuthMode)
		{
			// AuthMode Client 감소
			--mAuthClientCount;

			mSessionArray[index]->mSessionMode = eSessionMode::LogoutInAuthMode;
		}
	}

	return;
}

// SendFlag( WSASend 보내기 작업이 없다는 확인 ) 가 true인 경우 WaitLogoutMode로 변경 [ SendThread 때문에 SendFlag를 확인해야 한다. ]
// OnAuthClientLeave() 호출 해준다.
void CMMOServer::checkLogoutInAuthMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::LogoutInAuthMode)
		{
			if (InterlockedExchange8((char*)&mSessionArray[index]->mbSendFlag, false) == true)
			{
				mSessionArray[index]->OnAuthClientLeave();

				mSessionArray[index]->mSessionMode = eSessionMode::WaitLogoutMode;
			}
		}
	}

	return;
}

// 세션 배열을 돌면서 AuthToGameFlag 변수가 true이며, AUTH 모드인 세션을 AuthToGameMode로 변경
// AuthToGameFlag는 컨텐츠쪽에서 변경해준다. 네트워크 라이브러리 내부에서 컨텐츠 쪽에서 해야할 일이 언제 끝나는지 판단이 불가능하다.
// OnAuthClientLeave() 호출	
void CMMOServer::checkLeaveAuthToGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mbAuthToGameFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::AuthMode)
		{
			// AuthMode 플레이어가 UpdateThread로 이동했기 때문에 AuthMode Player를 감소시킨다.
			--mAuthClientCount;

			mSessionArray[index]->OnAuthClientLeave();

			// OnAuthClientLeave() 호출 후 모드를 변경해야 한다. 먼저 모드를 변경하면 UpdateThread와 AuthenticThread 2개의 스레드에 Session이 존재하기 때문이다.
			mSessionArray[index]->mSessionMode = eSessionMode::AuthToGameMode;

			// 다시 FALG를 false로 변경해준다. 해당 FLAG가 true면은 계속 if문의 두 번째 조건까지 체크하게 된다.
			mSessionArray[index]->mbAuthToGameFlag = false;
		}
	}

	return;
}



// 세션 배열을 돌면서 AuthToGameMode의 세션을 GAME 모드로 변경한다. 
// OnGameClientJoin() 호출
void CMMOServer::joinAuthToGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::AuthToGameMode)
		{
			mSessionArray[index]->OnGameClientJoin();

			// UpdateMode Client 증가
			++mGameClientCount;

			mSessionArray[index]->mSessionMode = eSessionMode::GameMode;
		}
	}

	return;
}


// 세션 배열을 모두 돌면서 GameMode 세션을 확인하여 해당 세션의 CompletionRecvPacket을 처리요청
void CMMOServer::checkMessageOfGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::GameMode)
		{
			CTemplateQueue<CMessage*>* pRecvCompletionQueue = &mSessionArray[index]->mRecvCompletionQueue;

			int useSize = pRecvCompletionQueue->GetUseSize();
			if (useSize == 0)
				continue;

			for (int msgIdx = 0; msgIdx < useSize; ++msgIdx)
			{
				CMessage* pMessage;
				pRecvCompletionQueue->Dequeue(&pMessage);

				try
				{
					mSessionArray[index]->OnGameMessage(pMessage);
				}
				catch (const message::CExceptionObject& exp)
				{
					CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkMessageOfGameMode] exp Error, sessionID : %lld", mSessionArray[index]->mSessionID);

					exp.PrintExceptionData();

					mSessionArray[index]->Disconnect();

					pMessage->Free();

					break;
				}

				pMessage->Free();
			}
		}
	}

	return;
}

void CMMOServer::updateSessionTimeout(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::GameMode && mTimeoutSec < timeGetTime() - mSessionArray[index]->mMsgRecvTime)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[updateSessionTimeout] sessionID : %lld, timeout : %d", mSessionArray[index]->mSessionID, mSessionArray[index]->mMsgRecvTime);

			mSessionArray[index]->Disconnect();
		}
	}

	return;
}



// 세션 배열을 돌면서 GameMode이면서 LogoutFlag 가 true인 Session으르 LogoutInGameMode로 변경한다.
void CMMOServer::checkLogoutFlagOfGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		// mbLogoutFlag 모드를 먼저 비교하여 mbLogoutFlag가 false 인 Session을 대상으로는 SessionMode까지 비교하지 않게 한다.
		if (mSessionArray[index]->mbLogoutFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::GameMode)
		{
			// UpdateMode Client 감소
			--mGameClientCount;

			mSessionArray[index]->mSessionMode = eSessionMode::LogoutInGameMode;
		}
	}

	return;
}


// 세션 배열을 돌면서 LogoutInGameMode 의 세션이 SendFlag 가 true인 경우 ( 즉, Send를 하지 않는 경우 ) WaitLogoutMode 변경
void CMMOServer::checkLogoutInGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::LogoutInGameMode)
		{
			if (InterlockedExchange8((char*)&mSessionArray[index]->mbSendFlag, false) == true)
			{
				// 게임 클라이언트가 나갔음을 컨텐츠측으로 알려준다.
				mSessionArray[index]->OnGameClientLeave();

				mSessionArray[index]->mSessionMode = eSessionMode::WaitLogoutMode;
			}
		}
	}

	return;
}



// 세션 배열을 돌면서 WaitLogout 모드의 세션을 최종적으로 릴리즈 처리한다.
// mReleaseSessionQueue 에 push 하는건 Update Thread 밖에 없기 때문에 LockFreeStack이 아닌 TemplateRingBuffer로 사용해도 된다.
void CMMOServer::checkWaitLogoutMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mbReleaseFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::WaitLogoutMode)
			releaseSession(mSessionArray[index]);
	}

	return;
}


bool CMMOServer::closeAcceptThread(void)
{
	if (mListenSocket == INVALID_SOCKET)
		return false;

	closesocket(mListenSocket);

	mListenSocket = INVALID_SOCKET;

	if (WAIT_OBJECT_0 != WaitForSingleObject(mAcceptThreadHandle, INFINITE))
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[closeAcceptThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mAcceptThreadHandle);

	return true;
}

void CMMOServer::closeAutenticThread(void)
{
	mbAutheticThreadFlag = false;

	if (WaitForSingleObject(mAuthenticThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[closeAutenticThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mAuthenticThreadHandle);

	return;
}

void CMMOServer::closeUpdateThread(void)
{
	mbUpdateThreadFlag = false;

	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[closeUpdateThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mUpdateThreadHandle);

	return;
}


void CMMOServer::closeSendThread(void)
{
	mbSendThreadFlag = false;

	if (WaitForSingleObject(mSendThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[closeSendThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mSendThreadHandle);

	return;
}

void CMMOServer::closeWorkerThread(void)
{
	for (DWORD index = 0; index < mWorkerThreadCount; ++index)
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);

	if (WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandleArray, true, INFINITE) == WAIT_FAILED)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	for (DWORD index = 0; index < mWorkerThreadCount; ++index)
		CloseHandle(mpWorkerThreadHandleArray[index]);

	delete[] mpWorkerThreadIDArray;

	delete[] mpWorkerThreadHandleArray;

	return;
}



void CMMOServer::cleanupSessionArray(void)
{
	for (int idx = 0; idx < mMaxClientCount; ++idx)
		OnReleaseSessionWithPlayer(mSessionArray[idx]);

	return;
}


void CMMOServer::closeIOCP(void)
{
	CloseHandle(mIocpHandle);

	return;
}

void CMMOServer::kickSessions(void)
{
	mbSessionClearFlag = true;

	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mbLogoutFlag == false)
			mSessionArray[index]->Disconnect();
	}

	if (mReleaseSessionStack.Size() != mMaxClientCount)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(mSessionClearEvent, INFINITE))
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[kickSessions] WaitForSingleObject Error Code : %d", WSAGetLastError());

			CCrashDump::Crash();
		}
	}

	return;
}


bool CMMOServer::isLoggingErrorCode(unsigned int errorCode) const
{
	switch (errorCode)
	{
	case ERROR_SUCCESS:

		return false;

	case ERROR_NETNAME_DELETED:

		return false;

	case ERROR_OPERATION_ABORTED:

		return false;

	case ERROR_IO_PENDING:

		return false;

	case WSAECONNRESET:

		return false;

	default:

		break;
	}

	return true;
}
