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
	// ������ Session Index ��ȣ
	static int sessionIndex;

	mIndex = sessionIndex;

	++sessionIndex;
}


void CMMOServer::CSession::SendPacket(CMessage* pMessage)
{
	// ��Ŷ ���ڵ�
	pMessage->encode();

	// ��Ŷ�� ���� ����ī��Ʈ ����
	pMessage->AddReferenceCount();

	// Send Q�� Enqueue
	if (mSendQueue.Enqueue(pMessage) == false)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"SendQueue is Full, sessionID : %lld", mSessionID);

		Disconnect();
	}

	// send �� �ֱ������� sendThread �� �Ѵ�.
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


// WSASend�� Message ���� Free ���ش�.
void CMMOServer::CSession::removeSendCompletionArray(void)
{
	int messageCount = mMessageCount;

	mMessageCount = 0;

	// �Ϸ������� �޾ƾ��� mSendCompletionArray�� �����ִ� �޽������� �����Ѵ�.
	CMessage** pMessageArray = mSendCompletionArray;

	for (int index = 0; index < messageCount; ++index)
		pMessageArray[index]->Free();

	return;
}

// SendRingBuffer�� �����ִ� Message ���� Free ���ش�.
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


// ���� IP / ��Ʈ / ���̱� �ɼ� / TimeoutFlag / headerCode / staticKey/ runningThread/ WorkerThread ���� / �ִ������� ��
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
	// AcceptThread�� Close
	if (closeAcceptThread() == false)
		return false;

	// ��� ���ǵ��� Disconnect ó�� 
	kickSessions();
	
	// ���� ������ Close
	closeAutenticThread();

	// UpdateThread Close
	closeUpdateThread();

	// SendThread Close
	closeSendThread();

	// WorkerThread Close
	closeWorkerThread();

	// Close IOCP
	closeIOCP();

	// ��������� Session ���� ���� ��ȸ�ϱ� ������ OnStop�� ���߿� ȣ���ؼ� Player ���� ����
	OnStop();

	// ��� ����
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

		// ������ �� ����
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
		// �ű� ������ Dequeue
		checkAcceptInfo();

		// Auth ��� ���� �޽��� ó��
		checkMessageOfAuthMode();

		authSessionTimeout();

		// ���� ��Ÿ�Ͽ� �°� ȣ���Ѵ�.
		// ĳ���� ���� �� DB���� ��û �� �ٷ� ���ο� ������ ���Դٸ��� ���� DB�� ���� �۾��� �ϴ� ���̱� ������ �����ߴ� ĳ���Ϳ� ���� ������ DB���� �� �а� ��
		// �ش� ó���� OnAuthenticUpdate���� ó������
		OnAuthUpdate();

		InterlockedIncrement((long*)&mAuthenticTPS);

		// LogoutFlag�� true�� Auth ��� ������ Ȯ�� �� LogoutInAuth ���� ����
		checkLogoutFlagOfAuthMode();

		// LogoutInAuthMode���� WaitLogoutMode���� ��ȯ ���� ó���� UpdateThread���� ��
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
		// AuthToGameMode ���ǵ��� ������� OnGameClienJoint(); ȣ�� �� GameMode ��ȯ
		joinAuthToGameMode();

		// GameMode ���ǵ��� Recv ��Ŷ ó��
		checkMessageOfGameMode();

		// timeout
		updateSessionTimeout();

		// Ŭ���̾�Ʈ�� ��û ( ��Ŷ ���� ) �ܿ� �⺻������ �׽� ó�� �Ǿ�� �� ���� ������ �κ� ���� OnGameUpdate() ȣ��
		OnGameUpdate();

		InterlockedIncrement((long*)&mUpdateTPS);

		// mbLogoutFlag�� true�̸鼭 GameMode�� ������� LogoutInGame ��� ��ȯ
		checkLogoutFlagOfGameMode();

		// SendFlag�� ���� ���� Send���� �ƴѰ��� Ȯ�� �� WaitLogout ���� ��ȯ
		checkLogoutInGameMode();

		// ReleaseFlag�� WaitLogout ��带 Ȯ�� �� releaseSession ó��
		checkWaitLogoutMode();

		Sleep(sleepTime);
	}

	OnCloseUpdateThread();

	return;
}


// WorkerThread�� ���� �����ҵ�
// ModeAuth, ModeGame ������ ������ mSendQueue�� �������� �ִٸ� ������ WSASend �� ��û�Ѵ�.
// ������ Sleep() �� �ɸ� �ȴ�.10ms ����? ���� ��Ÿ�Ͽ� �°�
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

		// CompletionKey�� �ʼ������� nullptr ó��
		CSession* pSession = nullptr;

		// Overapped �ʼ������� nullptr ó��
		LPOVERLAPPED pOverlapped = nullptr;

		mpWakeupWaitTimeArray[threadIndex] = timeGetTime();

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		InterlockedIncrement((long*)&mWakeupPerSecond);

		DWORD startTime = timeGetTime();

		// WorkerThread ����
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
	// ���� ���� ����
	if (setupListenSocket() == false)
		return false;

	// ���� ���ε�
	if (bindListenSocket() == false)
		return false;

	// ����
	if (listenSocket() == false)
		return false;

	// IOCP ����
	if (setupIOCP() == false)
		return false;

	// Player ����
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

	// KeepAlive �ɼ�
	// Window regedit ���� keepalive �ð� ���� �� ����
	int enable = 1;
	int retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	////// keepalive ����� ���� ��Ű�� ���� timeout ��
	//INT keepidle = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPIDLE, (CHAR*)&keepidle, sizeof(keepidle));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] TCP_KEEPIDLE setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}

	//////keepalive probe packet�� ������ ����
	//INT keepcnt = 5;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPCNT,(CHAR*)&keepcnt, sizeof(keepcnt));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] TCP_KEEPCNT setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}
	//

	//////probe packet�� ������ ����
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


	// RST �ɼ�
	linger optval = { 0, };
	optval.l_onoff = 1;
	optval.l_linger = 0;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	//�۽Ź��� 0���� �����
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

		// OnAuthClientJoin() ���� Send�� �� �� �ֱ� ������ �������Ѿ� �Ѵ�.
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

		// IOCP �ڵ�� ���� ���� �׸��� completionKey ����
		associateSocketWithIOCP(pSession->mSocket, pSession);

		// AuthClient�� �������� �����ʿ� �˸�
		pSession->OnAuthClientJoin();

		// OnAuthClientJoin()�� ȣ���Ų �� AuthMode�� �������ش�.
		pSession->mSessionMode = eSessionMode::AuthMode;

		// AuthMode Client ����
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

	// WSASend�� Message ���� Free ���ش�.
	pSession->removeSendCompletionArray();

	// SendRingBuffer�� �����ִ� Message ���� Free ���ش�.
	pSession->removeSendQueue();

	// RecvCompletionRingBuffer�� �����ִ� �޽������� Free ���ش�.
	pSession->removeRecvCompletionQueue();

	// Recv ���� ���� ����
	pSession->mRecvRingBuffer.ClearBuffer();

	// ������ �Ϸ��  Session�� mReleaseSessionQueue�� Enqeueu �Ѵ�.
	mReleaseSessionStack.Push(pSession->mIndex);

	InterlockedDecrement((long*)&mCurrentClientCount);

	// ��ü Session ���� ���� �÷���
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

	// ���� �����ϴ� �޸ٺ����� ���ؼ��� ���������� �����ϵ��� �Ѵ�.
	CRingBuffer* pRingBuffer = &pSession->mRecvRingBuffer;

	// GetDirectEnqueueSize ���� ���� �о�� �� 
	int freeSize = pRingBuffer->GetFreeSize();
	if (freeSize == 0)
	{
		CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[recvPost] mRecvRingBuffer is Full");

		pSession->Disconnect();

		return;
	}

	// �� ���۰� �������������� ����
	int directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	// rear ������
	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = directEnqueueSize;

	int remainSize = freeSize - directEnqueueSize;
	if (remainSize >= 0)
	{
		wsabufSize += 1;

		// ������ ������
		wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
		wsabufRecv[1].len = remainSize;
	}

	DWORD flags = 0;
	DWORD recvBytes = 0;

	// IOCount ����
	InterlockedIncrement(&pSession->mIoCount);

	// Overapped �ʱ�ȭ �ʼ�
	ZeroMemory(&pSession->mRecvOverlapped, sizeof(OVERLAPPED));

	// mSocket �� �ͺ�� �����̿��� ���� ���ۿ� �����Ͱ� ���� �� WOULDBLOCK�� �ƴ� WSA_IO_PENDING �� ������ �߻��ȴ�.
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

	// SendFlag�� true �� �����ϴ� ������� false �� �����ϴ� ������� 1�� �� �ִ� ��Ȳ���� ���� Interlocked�� �ʿ��ұ�?
	// AuthenticThread, UpdateThread ���� WaitLogoutMode�� ���� �� SendFlag�� true���� false �� �����ϸ鼭 �Ǵ��ϱ� ������ Interlocked �� �ʿ��ϴ�.
	if (InterlockedExchange8((char*)&pSession->mbSendFlag, false) == false)
		return;


	// ((bool)InterlockedExchange((LONG*)&pSession->mbSendFlag, false) ���� �ؿ� �־�� �Ѵ�. ���� ���� �ִٸ� ���� �������� ���� ��������
	// ������ ������� GetUseSize(); �� ȣ���ϰ� �� ���̰� Clear(); �Լ� ���ο����� mFront�� mRear�� ���� �ʱ�ȭ���ִ� �ڵ�� ���ؼ� GetUseSize(); ���� �߸� �� �� �ִ�.
	CTemplateQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

	int useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
	{
		// useSize�� 0�� ��� mbSendFlag�� �ٽ� true�� �������ְ� return ���־�� �Ѵ�.
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

	// Overapped�� �ʼ������� �ʱ�ȭ
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
				// UpdateThread, AuthenticThread ���� WaitLogoutMode�� �����ϱ� ���ؼ� InterlockedExchange�� SendFlag�� true���� false��
				// �����ϸ鼭 üũ�ϱ� ������ SendFlag�� true�� �������־�� �Ѵ�.	
				pSession->mbSendFlag = true;

				// mbLogoutFlag �� true �� Session�� ���ؼ��� SendThread�� WSASend() ȣ���� ���� �ʴ´�.
				pSession->mbLogoutFlag = true;
			}
		}
	}

	// SendTPS ����
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

		// ��� ����� ��ŭ recv �Ͽ����� Ȯ��
		if (recvSize < sizeof(CMessage::stNetHeader))
			break;

		CMessage::stNetHeader header;
		if (pRecvRingBuffer->Peek((CHAR*)&header, sizeof(header)) != sizeof(header))
		{
			// �� �������� ������ ���� ����̴�.	
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		// 1Byte headCode Ȯ�� 
		if (header.code != CMessage::mHeaderCode)
		{
			pSession->Disconnect();

			break;
		}

		// Start �Լ� ȣ�� �� ������ �ִ� Message ����� �ʰ��� ��� ���´�.
		if (header.payloadSize > mMaxMessageSize)
		{
			pSession->Disconnect();

			break;
		}


		// ��� + ���̷ε� ���Դ��� Ȯ��
		if (recvSize < sizeof(CMessage::stNetHeader) + header.payloadSize)
			break;


		// header ������ ��ŭ ������ front�� �ű��.
		pRecvRingBuffer->MoveFront(sizeof(CMessage::stNetHeader));

		// ��Ŷ ����
		CMessage* pMessage = CMessage::Alloc();

		// �����ۿ� �ִ� ���� �����͸� ��Ŷ�� ����
		if (pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize) != header.payloadSize)
		{
			// �� �������� ������ ���� ����̴�.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		// ��Ŷ ���̷ε� �����ŭ ������ front�� �ű��.
		if (pMessage->MoveWritePos(header.payloadSize) != header.payloadSize)
		{
			// �� CMessage ������ ���� ����̴�.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] Message Error Retval");

			CCrashDump::Crash();
		}

		// ���ڵ�
		if (pMessage->decode(&header) == false)
		{
			pSession->Disconnect();

			pMessage->Free();

			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] decode return false");

			return;
		}

		pSession->mMsgRecvTime = timeGetTime();

		// mRecvCompletionQueue �� Enqueue �� �ٸ� �����忡�� �ش� �޽����� ������ �� �ֱ� ������ ���� ī��Ʈ�� �������Ѿ� �Ѵ�.
		pMessage->AddReferenceCount();

		// �ϼ��� ��Ŷ�� mRecvCompletionQueue �� Enqueue�ϸ�, �̴� AuthenticThread, UpdateThread ���� ó���Ѵ�.
		if (pSession->mRecvCompletionQueue.Enqueue(pMessage) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[checkCompleteRecv] Enqueue Failed");

			CCrashDump::Crash();
		}

		// RecvTPS ����
		InterlockedIncrement((long*)&mRecvTPS);

		pMessage->Free();
	}

	recvPost(pSession);

	return;
}


// ���ſ� ���� �Ϸ����� ó��
void CMMOServer::checkCompletionSend(CSession* pSession)
{
	// �ݺ������� pSession �����͸� �̿��ؼ� mMessageCount�� ���������� �����ϱ� ������ mMessageCount�� ������ �����Ѵ�.
	int messageCount = pSession->mMessageCount;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	// �Ϸ������� ���� �� mSendCompletionArray �迭�� �ִ� ��Ŷ�� ��ȯ �����Ѵ�.
	for (int index = 0; index < messageCount; ++index)
		pSendCompletionArray[index]->Free();

	pSession->mMessageCount = 0;

	// SendFlag�� true�� �����Ѵ�.
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



// ���� �迭�� ��� ���鼭 AuthMode ������ Ȯ���Ͽ� �ش� ������ Completion Recv Message�� OnAuthMessage()�� ȣ���Ͽ� ó����û
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


// ���� �迭�� ���鼭 AuthMode���� LogoutFlag�� true�� Session�� ������� ModeLogoutInAuth �� �����Ѵ�.
void CMMOServer::checkLogoutFlagOfAuthMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		// logoutFlag�� true�̸��� LogoutInAuthMode�� ��ȯ
		if (mSessionArray[index]->mbLogoutFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::AuthMode)
		{
			// AuthMode Client ����
			--mAuthClientCount;

			mSessionArray[index]->mSessionMode = eSessionMode::LogoutInAuthMode;
		}
	}

	return;
}

// SendFlag( WSASend ������ �۾��� ���ٴ� Ȯ�� ) �� true�� ��� WaitLogoutMode�� ���� [ SendThread ������ SendFlag�� Ȯ���ؾ� �Ѵ�. ]
// OnAuthClientLeave() ȣ�� ���ش�.
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

// ���� �迭�� ���鼭 AuthToGameFlag ������ true�̸�, AUTH ����� ������ AuthToGameMode�� ����
// AuthToGameFlag�� �������ʿ��� �������ش�. ��Ʈ��ũ ���̺귯�� ���ο��� ������ �ʿ��� �ؾ��� ���� ���� �������� �Ǵ��� �Ұ����ϴ�.
// OnAuthClientLeave() ȣ��	
void CMMOServer::checkLeaveAuthToGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mbAuthToGameFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::AuthMode)
		{
			// AuthMode �÷��̾ UpdateThread�� �̵��߱� ������ AuthMode Player�� ���ҽ�Ų��.
			--mAuthClientCount;

			mSessionArray[index]->OnAuthClientLeave();

			// OnAuthClientLeave() ȣ�� �� ��带 �����ؾ� �Ѵ�. ���� ��带 �����ϸ� UpdateThread�� AuthenticThread 2���� �����忡 Session�� �����ϱ� �����̴�.
			mSessionArray[index]->mSessionMode = eSessionMode::AuthToGameMode;

			// �ٽ� FALG�� false�� �������ش�. �ش� FLAG�� true���� ��� if���� �� ��° ���Ǳ��� üũ�ϰ� �ȴ�.
			mSessionArray[index]->mbAuthToGameFlag = false;
		}
	}

	return;
}



// ���� �迭�� ���鼭 AuthToGameMode�� ������ GAME ���� �����Ѵ�. 
// OnGameClientJoin() ȣ��
void CMMOServer::joinAuthToGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::AuthToGameMode)
		{
			mSessionArray[index]->OnGameClientJoin();

			// UpdateMode Client ����
			++mGameClientCount;

			mSessionArray[index]->mSessionMode = eSessionMode::GameMode;
		}
	}

	return;
}


// ���� �迭�� ��� ���鼭 GameMode ������ Ȯ���Ͽ� �ش� ������ CompletionRecvPacket�� ó����û
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



// ���� �迭�� ���鼭 GameMode�̸鼭 LogoutFlag �� true�� Session���� LogoutInGameMode�� �����Ѵ�.
void CMMOServer::checkLogoutFlagOfGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		// mbLogoutFlag ��带 ���� ���Ͽ� mbLogoutFlag�� false �� Session�� ������δ� SessionMode���� ������ �ʰ� �Ѵ�.
		if (mSessionArray[index]->mbLogoutFlag == true && mSessionArray[index]->mSessionMode == eSessionMode::GameMode)
		{
			// UpdateMode Client ����
			--mGameClientCount;

			mSessionArray[index]->mSessionMode = eSessionMode::LogoutInGameMode;
		}
	}

	return;
}


// ���� �迭�� ���鼭 LogoutInGameMode �� ������ SendFlag �� true�� ��� ( ��, Send�� ���� �ʴ� ��� ) WaitLogoutMode ����
void CMMOServer::checkLogoutInGameMode(void)
{
	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index]->mSessionMode == eSessionMode::LogoutInGameMode)
		{
			if (InterlockedExchange8((char*)&mSessionArray[index]->mbSendFlag, false) == true)
			{
				// ���� Ŭ���̾�Ʈ�� �������� ������������ �˷��ش�.
				mSessionArray[index]->OnGameClientLeave();

				mSessionArray[index]->mSessionMode = eSessionMode::WaitLogoutMode;
			}
		}
	}

	return;
}



// ���� �迭�� ���鼭 WaitLogout ����� ������ ���������� ������ ó���Ѵ�.
// mReleaseSessionQueue �� push �ϴ°� Update Thread �ۿ� ���� ������ LockFreeStack�� �ƴ� TemplateRingBuffer�� ����ص� �ȴ�.
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
