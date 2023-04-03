#include "stdafx.h"
#include "CNetServer.h"



CNetServer::stSession::stSession(void)
	: mbSendFlag(false)
	, mSessionState{ 0,1 }

	, mSocket(INVALID_SOCKET)
	, mClientAddr{ 0, }

	, mRecvOverlapped{ 0, }
	, mSendOverlapped{ 0, }

	, mSessionID(0)
	, mIndex(0)
	, mRecvTime(0)
	, mbDisconnectFlag(false)

	, mRecvRingBuffer()
	, mSendQueue()

	, mMessageCount(0)
	, mSendCompletionArray{ 0, }
{
	// 고정된 Session Index 번호
	static unsigned long long sessionIndex = 0;

	mIndex = sessionIndex++;
}



CNetServer::CNetServer(void)
	: mAcceptTotal(0)
	, mCurrentClientCount(0)

	, mWorkerThreadIndex(-1)
	, mWakeupPerSecond(0)
	, mMaxWakeupPerSecond(0)
	, mMaxWakeupProcessTime(0)
	, mpWakeupProcessTimeArray(nullptr)
	, mpWakeupWaitTimeArray(nullptr)

	, mAcceptTPS(0)
	, mSendTPS(0)
	, mRecvTPS(0)

	, mSessionArray(nullptr)
	, mReleaseSessionStack()

	, mServerIP()
	, mServerPort(0)
	, mMaxClientCount(0)
	, mMaxMessageSize(0)
	, mWorkerThreadCount(0)
	, mRunningThreadCount(0)
	, mTimeoutSec(0)
	, mbTimeout(false)
	, mbNagleFlag(true)

	, mIocpHandle(NULL)
	, mListenSocket(INVALID_SOCKET)

	, mServerStartEvent(CreateEvent(NULL, true, false, nullptr))

	, mbSessionClearFlag(false)
	, mSessionClearEvent(CreateEvent(NULL, false, false, nullptr))


	, mAcceptThreadID(0)
	, mTimeoutThreadID(0)
	, mpWorkerThreadsID(nullptr)


	, mpWorkerThreadHandles(nullptr)
	, mTimeoutThreadHandle(INVALID_HANDLE_VALUE)
	, mAcceptThreadHandle(INVALID_HANDLE_VALUE)

{
	WSADATA wsa;

	if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}
}

CNetServer::~CNetServer(void)
{
	WSACleanup();
}




bool CNetServer::Start(const wchar_t* pServerIP, unsigned short serverPort, int maxMessageSize, bool bNagleFlag, bool bTimeout, unsigned char headerCode, unsigned char staticKey, unsigned int timeoutSec, int runningThreadCount, int workerThreadCount, int maxClientCount)
{
	if (pServerIP == nullptr || runningThreadCount < 1 ||  workerThreadCount < 1 || maxClientCount < 1)
		return false;
		
	mServerIP = pServerIP;

	mServerPort = serverPort;

	mMaxMessageSize = maxMessageSize;

	mRunningThreadCount = runningThreadCount;

	mWorkerThreadCount = workerThreadCount;

	mMaxClientCount = maxClientCount;

	CMessage::mHeaderCode = headerCode;
	
	CMessage::mStaticKey = staticKey;

	mbNagleFlag = bNagleFlag;

	mbTimeout = bTimeout;

	mTimeoutSec = timeoutSec;

	if (setupNetwork() == false)
		return false;

	if (OnStart() == false)
		return false;

	// 서버 시작
	SetEvent(mServerStartEvent);

	return true;
}

bool CNetServer::Stop(void)
{	
	if (closeAcceptThread() == false)
		return false;

	OnStop();

	kickSessions();

	closeTimeoutThread();

	closeWorkerThreads();

	cleanupReleaseStack();

	closeIOCP();

	return true;
}



const wchar_t* CNetServer::GetServerBindIP(void) const
{
	return mServerIP.c_str();
}

unsigned short CNetServer::GetServerBindPort(void) const
{
	return mServerPort;
}

bool CNetServer::GetNagleFlag(void) const
{
	return mbNagleFlag;
}

int CNetServer::GetMaxClientCount(void) const
{
	return mMaxClientCount;
}

int CNetServer::GetRunningThreadCount(void) const
{
	return mRunningThreadCount;
}

int CNetServer::GetWorkerThreadCount(void) const
{
	return mWorkerThreadCount;
}




bool CNetServer::SendPacket(unsigned long long sessionID, CMessage* pMessage)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
		return false;

	// 헤더 셋팅 그리고 체크섬과 같이 Encoding
	pMessage->encode();

	// SendQ에 Enqueue 하는 순간 다른 스레드에서 CMessage에 대한 참조가 가능해진다. 그러기 때문에 referenceCount를 증가시킨다.
	pMessage->AddReferenceCount();

	pSession->mSendQueue.Enqueue(pMessage);

	sendPost(pSession);

	InterlockedIncrement(&mSendTPS);

	leaveSession(pSession);

	return true;
}

bool CNetServer::Disconnect(unsigned long long sessionID)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
		return false;

	if (pSession->mbDisconnectFlag == false)
	{
		pSession->mbDisconnectFlag = true;

		CancelIoEx((HANDLE)pSession->mSocket, nullptr);
	}

	leaveSession(pSession);

	return true;
}

bool CNetServer::GetConnectionState(unsigned long long sessionID)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
		return false;

	leaveSession(pSession);

	return true;
}



unsigned long long CNetServer::GetAcceptTotal(void) const
{
	return mAcceptTotal;
}


long CNetServer::GetCurrentClientCount(void) const
{
	return mCurrentClientCount;
}




bool CNetServer::GetClientIP(unsigned long long sessionID, wchar_t* pClientIP, int bufferCch)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
		return false;

	InetNtopW(AF_INET, &pSession->mClientAddr.sin_addr, pClientIP, bufferCch);

	leaveSession(pSession);

	return true;
}

bool CNetServer::GetClientPort(unsigned long long sessionID, unsigned short* pClientPort)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
		return false;

	*pClientPort = ntohs(pSession->mClientAddr.sin_port);

	leaveSession(pSession);

	return true;
}


long CNetServer::GetAcceptTPS(void) const
{
	return mAcceptTPS;
}

long CNetServer::GetRecvTPS(void) const
{
	return mRecvTPS;
}

long CNetServer::GetSendTPS(void) const
{
	return mSendTPS;
}



void CNetServer::InitializeTPS(void)
{
	mAcceptTPS = 0;

	mSendTPS = 0;

	mRecvTPS = 0;

	return;
}


int CNetServer::GetWakeupPerSecond(void)
{
	int wakeupPerSecond = mWakeupPerSecond;

	if (mMaxWakeupPerSecond < wakeupPerSecond)
		mMaxWakeupPerSecond = wakeupPerSecond;

	InterlockedExchange(&mWakeupPerSecond, 0);

	return wakeupPerSecond;
}

int CNetServer::GetMaxWakeupPerSecond(void) const
{
	return mMaxWakeupPerSecond;
}

unsigned int CNetServer::GetWakeupProcessTime(void)
{
	unsigned int maxProcessTime = 0;

	for (int count = 0; count <= mWorkerThreadIndex; ++count)
	{
		unsigned int processTime = mpWakeupProcessTimeArray[count];

		if (maxProcessTime < processTime)
			maxProcessTime = processTime;
	}

	return maxProcessTime;
}


unsigned int CNetServer::GetMaxWakeupProcessTime(void) const
{
	return mMaxWakeupProcessTime;
}


unsigned int CNetServer::GetMaxWakeupWaitTime(void)
{
	unsigned int nowTime = timeGetTime();
	unsigned int maxWaitTime = UINT_MAX;

	for (int count = 0; count <= mWorkerThreadIndex; ++count)
	{
		unsigned int waitTime = mpWakeupWaitTimeArray[count];

		if (maxWaitTime > waitTime)
			maxWaitTime = waitTime;
	}

	return nowTime - maxWaitTime;
}


void CNetServer::associateSocketWithIOCP(stSession* pSession)
{
	if (CreateIoCompletionPort((HANDLE)pSession->mSocket, mIocpHandle, (ULONG_PTR)pSession, NULL) == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[associateSocketWithIOCP] CreateIoCompletionPort Error Code : % d", WSAGetLastError());

		CCrashDump::Crash();
	}

	return;
}

bool CNetServer::initializeSession(SOCKET clientSocket, SOCKADDR_IN clientAddr)
{
	static unsigned long long userID = 1;

	unsigned long long index;

	if (mReleaseSessionStack.Pop(&index) == false)
		return false;

	InterlockedIncrement(&mCurrentClientCount);

	stSession* pSession = &mSessionArray[index];

	InterlockedIncrement64((long long*)&pSession->mSessionState);

	// bUseFlag 보다 sessionID를 먼저 셋팅해주어야 한다.
	pSession->mSessionID = dfSESSION_ID(index, userID++);

	pSession->mSocket = clientSocket;

	pSession->mClientAddr = clientAddr;

	pSession->mRecvTime = timeGetTime();

	pSession->mSessionState.bUseFlag = 0;

	pSession->mbSendFlag = true;

	pSession->mbDisconnectFlag = false;

	associateSocketWithIOCP(pSession);

	OnClientJoin(pSession->mSessionID);

	recvPost(pSession);

	if (InterlockedDecrement64((long long*)&pSession->mSessionState) == 0)
		releaseSession(pSession);

	return true;
}


CNetServer::stSession* CNetServer::findSession(unsigned long long sessionID)
{
	unsigned long long index = dfSESSION_INDEX(sessionID);

	// mSessionArray 를 초과하는 index가 나올경우 return nullptr
	if (index >= mMaxClientCount)
		return nullptr;

	return &mSessionArray[index];
}


CNetServer::stSession* CNetServer::acquireSession(unsigned long long sessionID)
{
	stSession* pSession = findSession(sessionID);
	if (pSession == nullptr)
		return nullptr;

	// session에 대한 referenceCount를 증가시킨다. 
	InterlockedIncrement64((long long*)&pSession->mSessionState);

	// 만약 SessionID 를 releaseSession에서 0으로 바꾸지 않는다면은 useFlag를 먼저 비교해야한다.
	if (pSession->mSessionState.bUseFlag == 1 || pSession->mSessionID != sessionID)
	{
		leaveSession(pSession);

		return nullptr;
	}

	return pSession;
}


void CNetServer::leaveSession(stSession* pSession)
{
	if (InterlockedDecrement64((long long*)&pSession->mSessionState) == 0)
		releaseSession(pSession);

	return;
}



bool CNetServer::setupNetwork(void)
{
	if (setupSocket() == false)
		return false;

	if (bindSocket() == false)
		return false;

	if (listenSocket() == false)
		return false;

	if (setupIOCP() == false)
		return false;

	if (setupSessionArray() == false)
		return false;

	setupThread();

	return true;
}


bool CNetServer::setupSocket(void)
{
	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] socket Error Code : %d", WSAGetLastError());

		return false;
	}

	//KeepAlive 옵션
	//regedit 에서 keepalive 값 변경
	int enable = 1;
	int retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	//////// keepalive 기능을 동작 시키기 위한 timeout 값
	//int keepidle = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&keepidle, sizeof(keepidle));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPIDLE setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}

	////////keepalive probe packet을 보내는 갯수
	//int keepcnt = 5;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPCNT,(char*)&keepcnt, sizeof(keepcnt));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPCNT setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}
	//

	////////probe packet을 보내는 간격
	//int keepintvl = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&keepintvl, sizeof(keepintvl));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPINTVL setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}

	if (mbNagleFlag == false)
	{
		int optval = true;
		retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] Nagle No Delay setsockopt Error Code : %d", WSAGetLastError());

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
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	//송신버퍼 0으로 만들기
	/*int sendLen = 0;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendLen, sizeof(sendLen));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] Zero setsockopt Error Code : %d", WSAGetLastError());
		return false;
	}*/

	return true;
}


bool CNetServer::bindSocket(void)
{
	SOCKADDR_IN serverAddr = { 0, };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mServerPort);
	InetPtonW(AF_INET, mServerIP.c_str(), &serverAddr.sin_addr);

	int retval = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] bindSocket Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}


bool CNetServer::listenSocket(void)
{
	int retval = listen(mListenSocket, SOMAXCONN_HINT(USHRT_MAX));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[listenSocket] listen Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}


bool CNetServer::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupIOCP] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}


// 전체 Session 셋팅
bool CNetServer::setupSessionArray(void)
{
	mSessionArray = new stSession[mMaxClientCount];

	for (int index = mMaxClientCount - 1; 0 <= index; --index)
		mReleaseSessionStack.Push(mSessionArray[index].mIndex);

	return true;
}



void CNetServer::setupThread(void)
{
	setupAcceptThread();

	setupWorkerThreads();

	if (mbTimeout == true)
		setupTimeoutThread();

	return;
}

void CNetServer::setupAcceptThread(void)
{
	mAcceptThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)ExecuteAcceptThread, this, NULL, &mAcceptThreadID);

	return;
}



void CNetServer::setupWorkerThreads(void)
{
	mpWorkerThreadHandles = new HANDLE[mWorkerThreadCount];
	mpWorkerThreadsID = new unsigned int[mWorkerThreadCount];

	mpWakeupProcessTimeArray = new unsigned int[mWorkerThreadCount];
	ZeroMemory(mpWakeupProcessTimeArray, sizeof(unsigned int) * mWorkerThreadCount);
	mpWakeupWaitTimeArray = new unsigned int[mWorkerThreadCount];
	ZeroMemory(mpWakeupWaitTimeArray, sizeof(unsigned int) * mWorkerThreadCount);

	for (int index = 0; index < mWorkerThreadCount; ++index)
		mpWorkerThreadHandles[index] = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)ExecuteWorkerThread, this, NULL, &mpWorkerThreadsID[index]);

	return;
}

void CNetServer::setupTimeoutThread(void)
{
	mTimeoutThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)ExecuteTimeoutThread, this, NULL, &mTimeoutThreadID);

	return;
}


void CNetServer::waitServerStartEvent(void)
{
	if (WaitForSingleObject(mServerStartEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[waitStartEvent] WaitForSingleObject Error Code : %d", GetLastError());

		CCrashDump::Crash();
	}

	return;
}



bool CNetServer::closeAcceptThread(void)
{
	if (mListenSocket == INVALID_SOCKET)
		return false;

	closesocket(mListenSocket);

	mListenSocket = INVALID_SOCKET;

	if (WaitForSingleObject(mAcceptThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[closeAccept] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mAcceptThreadHandle);

	return true;
}


void CNetServer::kickSessions(void)
{
	mbSessionClearFlag = true;

	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index].mSessionState.bUseFlag == 0)
			Disconnect(mSessionArray[index].mSessionID);
	}

	if (mReleaseSessionStack.Size() != mMaxClientCount)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(mSessionClearEvent, INFINITE))
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[kickSessions] Error Code : %d", WSAGetLastError());

			CCrashDump::Crash();
		}
	}

	return;
}




void CNetServer::closeTimeoutThread(void)
{
	if (mbTimeout == false)
		return;

	mbTimeout = false;

	if (WaitForSingleObject(mTimeoutThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[closeTimeoutThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mTimeoutThreadHandle);

	return;
}




void CNetServer::closeWorkerThreads(void)
{
	for (int index = 0; index < mWorkerThreadCount; ++index)
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);

	if (WAIT_FAILED == WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandles, true, INFINITE))
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	for (int index = 0; index < mWorkerThreadCount; ++index)
		CloseHandle(mpWorkerThreadHandles[index]);

	delete[] mpWorkerThreadHandles;

	delete[] mpWorkerThreadsID;

	delete[] mpWakeupProcessTimeArray;

	delete[] mpWakeupWaitTimeArray;

	return;
}



void CNetServer::cleanupReleaseStack(void)
{
	unsigned long long  index = 0;

	for (;;)
	{
		if (mReleaseSessionStack.Pop(&index) == false)
			break;
	}

	return;
}


void CNetServer::closeIOCP(void)
{
	CloseHandle(mIocpHandle);

	return;
}




void CNetServer::releaseSession(stSession* pSession)
{
	stSessionState releaseSessionState = { 0,1 };

	// 멀티 스레드 구조에서는 IoCount만을 가지고는 세션 반환 동기화를 맞출 수 없다. 그래서 추가적인 데이터를 같이 비교하여 딱 한 번만 반환되도록 한다.
	if (0 != InterlockedCompareExchange64((long long*)&pSession->mSessionState, *(long long*)&releaseSessionState, 0))
		return;

	// 세션 반환 핸들러 호출
	OnClientLeave(pSession->mSessionID);

	closesocket(pSession->mSocket);

	clearSendQueue(&pSession->mSendQueue);

	clearSendCompletionArray(pSession);

	// recv 버퍼에 남아있는 데이터를 정리한다.
	pSession->mRecvRingBuffer.ClearBuffer();

	// 사용 가능 세션 index반환
	mReleaseSessionStack.Push(pSession->mIndex);

	InterlockedDecrement(&mCurrentClientCount);

	// 전체 Session 끊기 위한 플래그
	if (mbSessionClearFlag == true)
	{
		if (mCurrentClientCount == 0)
			SetEvent(mSessionClearEvent);
	}

	return;
}



void CNetServer::clearSendQueue(CLockFreeQueue<CMessage*>* pSendQueue)
{
	// SendQueue 에 남아있는 메시지들의 referenceCount를 차감시킨다.
	CMessage* pMessage;

	while (pSendQueue->Dequeue(&pMessage) == true)
		pMessage->Free();

	return;
}

void CNetServer::clearSendCompletionArray(stSession* pSession)
{
	int messageCount = pSession->mMessageCount;

	pSession->mMessageCount = 0;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	// 완료통지를 받아야할 mSendCompletionArray에 남아있는 메시지들을 정리한다.
	for (int index = 0; index < messageCount; ++index)
		pSendCompletionArray[index]->Free();

	return;
}





void CNetServer::sendPost(stSession* pSession)
{
	CLockFreeQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

SEND_START:

	int useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
		return;

	if (false == (bool)InterlockedExchange8((char*)&pSession->mbSendFlag, false))
		return;

	WSABUF wsabufSend[netserver::MAX_WSABUF_SIZE];
	if (useSize < netserver::MAX_WSABUF_SIZE)
		useSize = netserver::MAX_WSABUF_SIZE;


	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	int index;
	for (index = 0; index < useSize; ++index)
	{
		// 다른 세션에서 send 후 완료통지가 왔다면은 현재 queue 에 패킷이 없을 수도 있다. 
		if (pSendQueue->Dequeue(&pSendCompletionArray[index]) == false)
		{
			wsabufSend[index].buf = pSendCompletionArray[index]->GetMessagePtr();
			wsabufSend[index].len = pSendCompletionArray[index]->GetUseSize();
		}
		else
			break;
	}

	if (index == 0)
	{
		InterlockedExchange8((char*)&pSession->mbSendFlag, true);

		goto SEND_START;
	}

	pSession->mMessageCount = index;

	// 해당 Session 이 끊어진건지 연결중인지 판단이 어려움 
	// WSASend 는 해당 Session이 끊겼을 때 실패하지만, PQCS는 실패하지 않아서 애매하다.
	InterlockedIncrement64((long long*)&pSession->mSessionState);

	// Overapped는 항시 초기화하여 사용
	ZeroMemory(&pSession->mSendOverlapped, sizeof(OVERLAPPED));

	if (WSASend(pSession->mSocket, wsabufSend, index, nullptr, 0, &pSession->mSendOverlapped, nullptr) == SOCKET_ERROR)
	{
		unsigned int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			if (isLoggingErrorCode(errorCode) == true)
				CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[recvPost] Error Code : %d", errorCode);

			if (InterlockedDecrement64((long long*)&pSession->mSessionState) == 0)
				releaseSession(pSession);
		}
	}

	return;
}



void CNetServer::recvPost(stSession* pSession)
{
	WSABUF wsabufRecv[2];

	// 자주 접근하는 멤버변수는 지역변수로 접근하도록 한다.
	CRingBuffer* pRingBuffer = &pSession->mRecvRingBuffer;

	int freeSize = pRingBuffer->GetFreeSize();
	if (freeSize == 0)
	{
		CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[recvPost] mRecvRingBuffer is Full");

		Disconnect(pSession->mSessionID);

		return;
	}

	int directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = directEnqueueSize;

	int wsabufSize = 1;
	int remainSize = freeSize - directEnqueueSize;
	if (remainSize != 0)
	{
		wsabufSize += 1;

		wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
		wsabufRecv[1].len = remainSize;
	}

	unsigned long flags = 0;

	InterlockedIncrement64((long long*)&pSession->mSessionState);

	ZeroMemory(&pSession->mRecvOverlapped, sizeof(OVERLAPPED));

	// mSocket 이 블락 소켓이여도 수신 버퍼에 데이터가 없을 때 WOULDBLOCK이 아닌 WSA_IO_PENDING 이 에러가 발생된다.
	if (WSARecv(pSession->mSocket, wsabufRecv, wsabufSize, nullptr, &flags, &pSession->mRecvOverlapped, nullptr) == SOCKET_ERROR)
	{
		unsigned int errorCode = WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			if (isLoggingErrorCode(errorCode) == true)
				CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[recvPost] Error Code : %d", errorCode);

			if (InterlockedDecrement64((long long*)&pSession->mSessionState) == 0)
				releaseSession(pSession);
		}
	}

	if (pSession->mbDisconnectFlag == true)
		CancelIoEx((HANDLE)pSession->mSocket, &pSession->mRecvOverlapped);

	return;
}


void CNetServer::checkCompletionSend(stSession* pSession)
{
	int messageCount = pSession->mMessageCount;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	// 송신 완료한 패킷을 반환
	for (int index = 0; index < messageCount; ++index)
		pSendCompletionArray[index]->Free();

	pSession->mMessageCount = 0;

	// OOOE 
	InterlockedExchange8((char*)&pSession->mbSendFlag, true);

	sendPost(pSession);

	return;
}

void CNetServer::checkCompletionRecv(stSession* pSession, unsigned long transferred)
{
	CRingBuffer* pRecvRingBuffer = &pSession->mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		int recvSize = pRecvRingBuffer->GetUseSize();
		if (recvSize < sizeof(CMessage::stNetHeader))
			break;

		// 헤더 사이즈를 확인
		CMessage::stNetHeader header;
		int retval = pRecvRingBuffer->Peek((char*)&header, sizeof(header));
		if (retval != sizeof(header))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval : %d", retval);

			CCrashDump::Crash();
		}

		// 헤더 코드 확인
		if (header.code != CMessage::mHeaderCode)
		{
			Disconnect(pSession->mSessionID);

			break;
		}

		// 페이로드 사이즈가 비정상적으로 긴지 확인
		if (header.payloadSize > mMaxMessageSize)
		{
			Disconnect(pSession->mSessionID);

			break;
		}

		// 헤더 + 헤이로드 사이즈만큼 recv 데이터가 있는지 확인
		if (recvSize < sizeof(CMessage::stNetHeader) + header.payloadSize)
			break;

		pRecvRingBuffer->MoveFront(sizeof(CMessage::stNetHeader));

		CMessage* pMessage = CMessage::Alloc();

		retval = pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 링버퍼의 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval : %d", retval);

			CCrashDump::Crash();
		}

		retval = pMessage->MoveWritePos(header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] Message Error Retval : %d", retval);

			CCrashDump::Crash();
		}

		if (pMessage->decode(&header) == false)
		{
			Disconnect(pSession->mSessionID);

			pMessage->Free();

			return;
		}

		// timeout 시간 갱신
		pSession->mRecvTime = timeGetTime();

		try
		{
			OnRecv(pSession->mSessionID, pMessage);
		}
		catch (const message::CExceptionObject& exp)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] Msg Exception : %lld", pSession->mSessionID);

			exp.PrintExceptionData();

			Disconnect(pSession->mSessionID);
		}

		InterlockedIncrement(&mRecvTPS);

		pMessage->Free();
	}

	recvPost(pSession);

	return;
}



unsigned CNetServer::ExecuteAcceptThread(void* pParam)
{
	((CNetServer*)pParam)->AcceptThread();

	return 1;
}


unsigned CNetServer::ExecuteWorkerThread(void* pParam)
{
	srand((unsigned)time(NULL));

	((CNetServer*)pParam)->WorkerThread();

	return 1;
}


unsigned CNetServer::ExecuteTimeoutThread(void* pParam)
{
	((CNetServer*)pParam)->TimeoutThread();

	return 1;
}

void CNetServer::AcceptThread(void)
{
	waitServerStartEvent();

	OnStartAcceptThread();

	for (;;)
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr = { 0, };
		int addrlen = sizeof(clientAddr);

		clientSocket = accept(mListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSocket == INVALID_SOCKET)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] Accept, Error Code : %d", WSAGetLastError());

			break;
		}

		InterlockedIncrement(&mAcceptTPS);

		++mAcceptTotal;

		wchar_t clientIP[50];
		InetNtopW(AF_INET, &clientAddr.sin_addr, clientIP, _countof(clientIP));
		if (OnConnectionRequest(clientIP, ntohs(clientAddr.sin_port)) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] Black IP : %s", clientIP);

			closesocket(clientSocket);

			continue;
		}

		if (mCurrentClientCount >= mMaxClientCount)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] CurrentClient is full, Max Client : %d, Current Client : %d", mMaxClientCount, mCurrentClientCount);

			closesocket(clientSocket);

			continue;
		}

		if (initializeSession(clientSocket, clientAddr) == false)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] initializeSession Return Value is false");

			CCrashDump::Crash();
		}
	}

	OnCloseAcceptThread();

	return;
}


void CNetServer::WorkerThread(void)
{
	waitServerStartEvent();

	OnStartWorkerThread();

	unsigned int processCount = 0;

	unsigned int processTotalTime = 0;

	int threadIndex = InterlockedIncrement(&mWorkerThreadIndex);

	unsigned int initializeTime = timeGetTime();

	for (;;)
	{
		unsigned long transferred = 0;
		stSession* pSession = nullptr;
		LPOVERLAPPED pOverlapped = nullptr;

		mpWakeupWaitTimeArray[threadIndex] = timeGetTime();

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);

		// 워커 스레드가 초당 몇번 깨어나는지를 체크한다.
		InterlockedIncrement(&mWakeupPerSecond);

		unsigned int startTime = timeGetTime();
		if (pOverlapped == nullptr)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[WorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

			break;
		}

		if (transferred != 0 && pSession->mbDisconnectFlag == false)
		{
			if (&pSession->mRecvOverlapped == pOverlapped)
			{
				checkCompletionRecv(pSession, transferred);
			}
			else if (&pSession->mSendOverlapped == pOverlapped)
			{
				checkCompletionSend(pSession);
			}
			else
			{
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[WorkerThread] Exception Overapped, transffered : %d, Error Code : %d, Session ID : %llx\n", transferred, WSAGetLastError(), pSession->mSessionID);

				Disconnect(pSession->mSessionID);
			}
		}

		unsigned int errorCode = WSAGetLastError();
		if (isLoggingErrorCode(errorCode) == true)
			CSystemLog::GetInstance()->Log(false, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[WorkerThread] Error Code : %d", errorCode);
	
		if (InterlockedDecrement64((long long*)&pSession->mSessionState) == 0)
			releaseSession(pSession);


		unsigned int nowTime = timeGetTime();

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

	OnCloseWorkerThread();

	return;
}



void CNetServer::TimeoutThread(void)
{
	// Timeout이 60초라면은 30초 단위로 session들을 대상으로 loop를 돈다.
	unsigned int sleepTime = mTimeoutSec / 2 * 1000;

	unsigned int timeout = mTimeoutSec * 1000;

	// this 를 통한 접근을 삭제
	auto &pSessionArray = mSessionArray;

	while (mbTimeout == true)
	{
		for (int index = 0; index < mMaxClientCount; ++index)
		{	
			if (acquireSession(pSessionArray[index].mSessionID) == nullptr)
				continue;

			unsigned int nowTime = timeGetTime();
			unsigned int recvTime = pSessionArray[index].mRecvTime;

			// nowTime을 변수에 저장하고 이후 메시지를 recv 받았다면 nowTime 이 recvTime 보다 작을 수 있음
			// 그럴 경우 unsinged 데이터 타입이기 때문에 산술 오버플로우가 발생될 수 있음
			if (nowTime > recvTime && nowTime - recvTime > timeout)
				Disconnect(pSessionArray[index].mSessionID);

			leaveSession(&pSessionArray[index]);
		}

		Sleep(sleepTime);
	}

	return;
}


void CNetServer::SendThread(void)
{
	for (;;)
	{
		for (int idx = 0; idx < mMaxClientCount; ++idx)
		{
			stSession *pSession = acquireSession(mSessionArray[idx].mSessionID);
			if (pSession == nullptr)
				continue;
	
			sendPost(pSession);

			leaveSession(pSession);
		}

		Sleep(16);
	}

	return;
}


bool CNetServer::isLoggingErrorCode(unsigned int errorCode) const
{
	switch (errorCode)
	{
	case ERROR_SUCCESS:                 // 에러 없음

		return false;

	case ERROR_NETNAME_DELETED:         // 연결 끊김             

		return false;

	case ERROR_OPERATION_ABORTED:       // I/O 취소

		return false;

	case ERROR_IO_PENDING:              // 비동기 I/O

		return false;

	case WSAECONNRESET:         

		return false;

	default:

		break;
	}

	return true;
}
