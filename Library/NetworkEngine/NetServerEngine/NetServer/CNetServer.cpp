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
	// ������ Session Index ��ȣ
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

	// ���� ����
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

	// ��� ���� �׸��� üũ���� ���� Encoding
	pMessage->encode();

	// SendQ�� Enqueue �ϴ� ���� �ٸ� �����忡�� CMessage�� ���� ������ ����������. �׷��� ������ referenceCount�� ������Ų��.
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

	// bUseFlag ���� sessionID�� ���� �������־�� �Ѵ�.
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

	// mSessionArray �� �ʰ��ϴ� index�� ���ð�� return nullptr
	if (index >= mMaxClientCount)
		return nullptr;

	return &mSessionArray[index];
}


CNetServer::stSession* CNetServer::acquireSession(unsigned long long sessionID)
{
	stSession* pSession = findSession(sessionID);
	if (pSession == nullptr)
		return nullptr;

	// session�� ���� referenceCount�� ������Ų��. 
	InterlockedIncrement64((long long*)&pSession->mSessionState);

	// ���� SessionID �� releaseSession���� 0���� �ٲ��� �ʴ´ٸ��� useFlag�� ���� ���ؾ��Ѵ�.
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

	//KeepAlive �ɼ�
	//regedit ���� keepalive �� ����
	int enable = 1;
	int retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	//////// keepalive ����� ���� ��Ű�� ���� timeout ��
	//int keepidle = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&keepidle, sizeof(keepidle));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPIDLE setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}

	////////keepalive probe packet�� ������ ����
	//int keepcnt = 5;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPCNT,(char*)&keepcnt, sizeof(keepcnt));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPCNT setsockopt Error Code : %d", WSAGetLastError());

	//	return false;
	//}
	//

	////////probe packet�� ������ ����
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


	// RST �ɼ�
	linger optval = { 0, };
	optval.l_onoff = 1;
	optval.l_linger = 0;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		return false;
	}

	//�۽Ź��� 0���� �����
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


// ��ü Session ����
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

	// ��Ƽ ������ ���������� IoCount���� ������� ���� ��ȯ ����ȭ�� ���� �� ����. �׷��� �߰����� �����͸� ���� ���Ͽ� �� �� ���� ��ȯ�ǵ��� �Ѵ�.
	if (0 != InterlockedCompareExchange64((long long*)&pSession->mSessionState, *(long long*)&releaseSessionState, 0))
		return;

	// ���� ��ȯ �ڵ鷯 ȣ��
	OnClientLeave(pSession->mSessionID);

	closesocket(pSession->mSocket);

	clearSendQueue(&pSession->mSendQueue);

	clearSendCompletionArray(pSession);

	// recv ���ۿ� �����ִ� �����͸� �����Ѵ�.
	pSession->mRecvRingBuffer.ClearBuffer();

	// ��� ���� ���� index��ȯ
	mReleaseSessionStack.Push(pSession->mIndex);

	InterlockedDecrement(&mCurrentClientCount);

	// ��ü Session ���� ���� �÷���
	if (mbSessionClearFlag == true)
	{
		if (mCurrentClientCount == 0)
			SetEvent(mSessionClearEvent);
	}

	return;
}



void CNetServer::clearSendQueue(CLockFreeQueue<CMessage*>* pSendQueue)
{
	// SendQueue �� �����ִ� �޽������� referenceCount�� ������Ų��.
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

	// �Ϸ������� �޾ƾ��� mSendCompletionArray�� �����ִ� �޽������� �����Ѵ�.
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
		// �ٸ� ���ǿ��� send �� �Ϸ������� �Դٸ��� ���� queue �� ��Ŷ�� ���� ���� �ִ�. 
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

	// �ش� Session �� ���������� ���������� �Ǵ��� ����� 
	// WSASend �� �ش� Session�� ������ �� ����������, PQCS�� �������� �ʾƼ� �ָ��ϴ�.
	InterlockedIncrement64((long long*)&pSession->mSessionState);

	// Overapped�� �׽� �ʱ�ȭ�Ͽ� ���
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

	// ���� �����ϴ� ��������� ���������� �����ϵ��� �Ѵ�.
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

	// mSocket �� ��� �����̿��� ���� ���ۿ� �����Ͱ� ���� �� WOULDBLOCK�� �ƴ� WSA_IO_PENDING �� ������ �߻��ȴ�.
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

	// �۽� �Ϸ��� ��Ŷ�� ��ȯ
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

		// ��� ����� Ȯ��
		CMessage::stNetHeader header;
		int retval = pRecvRingBuffer->Peek((char*)&header, sizeof(header));
		if (retval != sizeof(header))
		{
			// �� �������� ������ ���� ����̴�.	
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval : %d", retval);

			CCrashDump::Crash();
		}

		// ��� �ڵ� Ȯ��
		if (header.code != CMessage::mHeaderCode)
		{
			Disconnect(pSession->mSessionID);

			break;
		}

		// ���̷ε� ����� ������������ ���� Ȯ��
		if (header.payloadSize > mMaxMessageSize)
		{
			Disconnect(pSession->mSessionID);

			break;
		}

		// ��� + ���̷ε� �����ŭ recv �����Ͱ� �ִ��� Ȯ��
		if (recvSize < sizeof(CMessage::stNetHeader) + header.payloadSize)
			break;

		pRecvRingBuffer->MoveFront(sizeof(CMessage::stNetHeader));

		CMessage* pMessage = CMessage::Alloc();

		retval = pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize);
		if (retval != header.payloadSize)
		{
			// �� �������� ������ ���� ����̴�.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval : %d", retval);

			CCrashDump::Crash();
		}

		retval = pMessage->MoveWritePos(header.payloadSize);
		if (retval != header.payloadSize)
		{
			// �� CMessage ������ ���� ����̴�.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] Message Error Retval : %d", retval);

			CCrashDump::Crash();
		}

		if (pMessage->decode(&header) == false)
		{
			Disconnect(pSession->mSessionID);

			pMessage->Free();

			return;
		}

		// timeout �ð� ����
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

		// ��Ŀ �����尡 �ʴ� ��� ��������� üũ�Ѵ�.
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
	// Timeout�� 60�ʶ���� 30�� ������ session���� ������� loop�� ����.
	unsigned int sleepTime = mTimeoutSec / 2 * 1000;

	unsigned int timeout = mTimeoutSec * 1000;

	// this �� ���� ������ ����
	auto &pSessionArray = mSessionArray;

	while (mbTimeout == true)
	{
		for (int index = 0; index < mMaxClientCount; ++index)
		{	
			if (acquireSession(pSessionArray[index].mSessionID) == nullptr)
				continue;

			unsigned int nowTime = timeGetTime();
			unsigned int recvTime = pSessionArray[index].mRecvTime;

			// nowTime�� ������ �����ϰ� ���� �޽����� recv �޾Ҵٸ� nowTime �� recvTime ���� ���� �� ����
			// �׷� ��� unsinged ������ Ÿ���̱� ������ ��� �����÷ο찡 �߻��� �� ����
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
	case ERROR_SUCCESS:                 // ���� ����

		return false;

	case ERROR_NETNAME_DELETED:         // ���� ����             

		return false;

	case ERROR_OPERATION_ABORTED:       // I/O ���

		return false;

	case ERROR_IO_PENDING:              // �񵿱� I/O

		return false;

	case WSAECONNRESET:         

		return false;

	default:

		break;
	}

	return true;
}
