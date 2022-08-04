#include "stdafx.h"

// public 

CLanServer::CLanServer(void)
	: mAcceptTotal(0)
	, mAcceptTPS(0)
	, mSendTPS(0)
	, mRecvTPS(0)
	, mServerStartEvent(CreateEvent(NULL,TRUE,FALSE,nullptr))
	, mIocpHandle(NULL)
	, mListenSocket(INVALID_SOCKET)
	, mServerIP{ 0, }
	, mServerPort(0)
	, mMaxClientCount(0)
	, mWorkerThreadCount(0)
	, mRunningThreadCount(0)
	, mpWorkerThreadHandle(nullptr)
	, mpWorkerThreadID(nullptr)
	, mAcceptThreadHandle(INVALID_HANDLE_VALUE)
	, mAcceptThreadID(0)
	, mbSessionClearFlag(FALSE)
	, mbNagleFlag(FALSE)
	, mSessionClearEvent(CreateEvent(NULL, FALSE, FALSE, nullptr))
	, mSessionArray(nullptr)
	, mReleaseSessionStack()
	, mCurrentClientCount(0)
{
	WSADATA wsa;

	if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelDebug, L"LanServer", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

}

CLanServer::~CLanServer(void)
{
	WSACleanup();
}

BOOL CLanServer::Start(const WCHAR* serverIP,WORD serverPort, BOOL bNagleFlag, DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount)
{	
	if (serverIP == nullptr)
	{
		return FALSE;
	}

	if (serverPort > USHRT_MAX)
	{
		return FALSE;
	}

	if (runningThreadCount < 1)
	{
		return FALSE;
	}

	if (workerThreadCount < 1)
	{
		return FALSE;
	}

	if (maxClientCount < 1)
	{
		return FALSE;
	}

	mServerIP = (WCHAR*)serverIP;

	mServerPort = serverPort;

	mbNagleFlag = bNagleFlag;

	mRunningThreadCount = runningThreadCount;

	mWorkerThreadCount = workerThreadCount;

	mMaxClientCount = maxClientCount;

	if (setupNetwork() == FALSE)
	{
		return FALSE;
	}

	if (OnStart() == FALSE)
	{
		return FALSE;
	}

	SetEvent(mServerStartEvent);

	return TRUE;
}

BOOL CLanServer::Stop(void)
{
	if (closeAcceptThread() == FALSE)
	{
		return FALSE;
	}

	kickSessions();

	OnStop();
	
	cleanupWorkerThread();

	cleanupReleaseStack();

	cleanupSessionArray();

	closeIOCP();

	return TRUE;
}

void CLanServer::cleanupReleaseStack(void)
{
	UINT64 index = 0;

	for (;;)
	{
		if (mReleaseSessionStack.Pop(&index) == FALSE)
		{
			break;
		}
	}

	return;
}

void CLanServer::cleanupSessionArray(void)
{
	delete[] mSessionArray;

	return;
}


void CLanServer::closeIOCP(void)
{
	CloseHandle(mIocpHandle);

	return;
}


BOOL CLanServer::closeAcceptThread(void)
{
	if (mListenSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	closesocket(mListenSocket);

	mListenSocket = INVALID_SOCKET;

	if (WAIT_OBJECT_0 != WaitForSingleObject(mAcceptThreadHandle, INFINITE))
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[closeAccept] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mAcceptThreadHandle);

	return TRUE;
}

void CLanServer::cleanupWorkerThread(void)
{
	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);
	}

	if (WAIT_FAILED == WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandle, TRUE, INFINITE))
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		CloseHandle(mpWorkerThreadHandle[index]);
	}

	delete[] mpWorkerThreadHandle;

	delete[] mpWorkerThreadID;

	return;
}


BOOL CLanServer::SendPacket(UINT64 sessionID, CMessage* pMessage)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[SendPacket] Client Not Found, sessionID : %lld", sessionID);

		return FALSE;
	}

	CMessage::stLanHeader header;

	header.payloadSize = (WORD)pMessage->GetUseSize();

	pMessage->setLanHeader(&header);

	pMessage->AddReferenceCount();

	pSession->mSendQueue.Enqueue(pMessage);

	sendPost(pSession);

	InterlockedIncrement(&mSendTPS);

	leaveSession(pSession);

	return TRUE;
}


BOOL CLanServer::Disconnect(UINT64 sessionID)
{
	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
	{
		return FALSE;
	}

	if (pSession->mbDisconnectFlag == FALSE)
	{
		pSession->mbDisconnectFlag = TRUE;

		CancelIoEx((HANDLE)pSession->mSocket, NULL);
	}

	leaveSession(pSession);

	return TRUE;
}

void CLanServer::InitializeTPS(void)
{
	InterlockedExchange(&mAcceptTPS, 0);

	InterlockedExchange(&mSendTPS, 0);

	InterlockedExchange(&mRecvTPS, 0);

	return;
}


INT CLanServer::GetCurrentClientCount(void) const
{
	return mCurrentClientCount;
}


INT CLanServer::GetMaxClientCount(void) const
{
	return mMaxClientCount;
}

INT CLanServer::GetWorkerThreadCount(void) const
{
	return mWorkerThreadCount;
}

INT CLanServer::GetRunningThreadCount(void) const
{
	return mRunningThreadCount;
}

BOOL CLanServer::GetNagleFlag(void) const
{
	return mbNagleFlag;
}

UINT64 CLanServer::GetAcceptTotal(void) const
{
	return mAcceptTotal;
}


WCHAR* CLanServer::GetServerBindIP(void) const
{
	return (WCHAR*)mServerIP;
}

WORD CLanServer::GetServerBindPort(void) const
{
	return mServerPort;
}


// private

BOOL CLanServer::initializeSession(SOCKET clientSocket, SOCKADDR_IN clientAddr)
{
	static UINT64 userID = 1;

	stSession* pSession;

	UINT64 index;

	if (mReleaseSessionStack.Pop(&index) == FALSE)
	{
		return FALSE;
	}

	InterlockedIncrement(&mCurrentClientCount);

	pSession = &mSessionArray[index];

	InterlockedIncrement64((INT64*)&pSession->mSessionState);

	pSession->mSessionID = dfSESSION_ID(index, userID);

	++userID;

	pSession->mSocket = clientSocket;

	pSession->mClientAddr = clientAddr;

	pSession->mSessionState.bUseFlag = 0;

	pSession->mbSendFlag = TRUE;

	pSession->mbDisconnectFlag = FALSE;
	
	associateSocketWithIOCP(pSession);

	OnClientJoin(pSession->mSessionID);

	recvPost(pSession);

	if (InterlockedDecrement64((INT64*)&pSession->mSessionState) == 0)
	{
		releaseSession(pSession);
	}

	return TRUE;
}

CLanServer::stSession* CLanServer::findSession(UINT64 sessionID)
{
	INT index = 0;

	index = dfSESSION_INDEX(sessionID);

	// mSessionArray 를 초과하는 index가 나올경우 return nullptr
	if (index >= mMaxClientCount)
	{
		return nullptr;
	}

	return &mSessionArray[index];
}


CLanServer::stSession* CLanServer::acquireSession(UINT64 sessionID)
{
	stSession* pSession = findSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	// session에 대한 referenceCount를 증가시킨다. 
	InterlockedIncrement64((INT64*)&pSession->mSessionState);

	// 만약 SessionID 를 releaseSession에서 0으로 바꾸지 않는다면은 useFlag를 먼저 비교해야한다.
	if (pSession->mSessionState.bUseFlag == 1 || pSession->mSessionID != sessionID )
	{
		leaveSession(pSession);

		return nullptr;
	}

	return pSession;
}


void CLanServer::associateSocketWithIOCP(stSession *pSession)
{
	if (CreateIoCompletionPort((HANDLE)pSession->mSocket, mIocpHandle, (ULONG_PTR)pSession, NULL) == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CLanServer", L"[associateSocketWithIOCP] CreateIoCompletionPort Error Code : % d", WSAGetLastError());

		CCrashDump::Crash();
	}

	return;
}


void CLanServer::leaveSession(stSession* pSession)
{
	if (InterlockedDecrement64((INT64*)&pSession->mSessionState) == 0)
	{
		releaseSession(pSession);
	}

	return;
}



void CLanServer::releaseSession(stSession* pSession)
{
	// 생성자 호출
	stSessionState releaseSessionState = { 0,1 };

	if (FALSE != (BOOL)InterlockedCompareExchange64((INT64*)&pSession->mSessionState, *(INT64*)&releaseSessionState, (INT64)0))
	{
		return;
	}

	OnClientLeave(pSession->mSessionID);

	closesocket(pSession->mSocket);

	cleanupSendQueue(&pSession->mSendQueue);

	cleanupSendCompletionArray(pSession);

	pSession->mRecvRingBuffer.ClearBuffer();

	mReleaseSessionStack.Push(pSession->mIndex);

	InterlockedDecrement(&mCurrentClientCount);

	if (mbSessionClearFlag == TRUE)
	{
		if (mCurrentClientCount == 0)
		{
			SetEvent(mSessionClearEvent);
		}
	}

	return;
}


void CLanServer::cleanupSendQueue(CLockFreeQueue<CMessage*> *pSendQueue)
{
	CMessage* pMessage;

	while (pSendQueue->Dequeue(&pMessage) == TRUE)
	{
		pMessage->Free();
	}

	return;
}

void CLanServer::cleanupSendCompletionArray(stSession* pSession)
{
	INT messageCount = pSession->mMessageCount;

	pSession->mMessageCount = 0;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	for (INT index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	return;
}


void CLanServer::recvPost(stSession* pSession)
{
	INT retval;

	WSABUF wsabufRecv[2];

	CRingBuffer* pRingBuffer = &pSession->mRecvRingBuffer;

	// 값이 중간에 바뀔 수 있기 때문에 미리 변수에서 담아서 사용해야 한다.
	INT freeSize = pRingBuffer->GetFreeSize();
	INT directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = directEnqueueSize;

	wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
	wsabufRecv[1].len = freeSize - directEnqueueSize;

	//if (freeSize - directEnqueueSize < 0)
	//{
	//	_LOG(TRUE, eLogList::LOG_LEVEL_ERROR, L"[recvPost Error] freeSize : %d, directEnqueueSize : %d\n", freeSize, directEnqueueSize);

	//	CCrashDump::Crash();
	//}

	DWORD flags = 0;
	
	InterlockedIncrement64((INT64*)&pSession->mSessionState);

	ZeroMemory(&pSession->mRecvOverlapped, sizeof(OVERLAPPED));

	retval = WSARecv(pSession->mSocket, wsabufRecv, 2, nullptr, &flags, &pSession->mRecvOverlapped, nullptr);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{			
			if (InterlockedDecrement64((INT64*)&pSession->mSessionState) == 0)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelDebug, L"LanServer", L"[WSARecv] ErrorCode : %d, Session ID : %lld", WSAGetLastError(), pSession->mSessionID);

				releaseSession(pSession);
			}
		}
	}

	if (pSession->mbDisconnectFlag == TRUE)
	{
		CancelIoEx((HANDLE)pSession->mSocket, &pSession->mRecvOverlapped);
	}

	return;
}

void CLanServer::sendPost(stSession* pSession)
{
	INT retval;

	CLockFreeQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

SEND_START:

	INT useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
	{
		return;
	}

	if (FALSE == (BOOL)InterlockedExchange((LONG*)&pSession->mbSendFlag, FALSE))
	{
		return;
	}

	WSABUF wsabufSend[lanserver::MAX_WSABUF_SIZE] = { 0, };
	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	INT index;
	for (index = 0; index < useSize; ++index)
	{
		if (pSendQueue->Dequeue(&pSendCompletionArray[index]) == TRUE)
		{
			wsabufSend[index].buf = pSendCompletionArray[index]->GetMessagePtr();
			wsabufSend[index].len = pSendCompletionArray[index]->GetUseSize();
		}
		else
		{
			break;
		}
	}

	if (index == 0)
	{
		// OOOE
		InterlockedExchange((LONG*)&pSession->mbSendFlag, TRUE);

		goto SEND_START;
	}

	pSession->mMessageCount = index;

	InterlockedIncrement64((INT64*)&pSession->mSessionState);

	ZeroMemory(&pSession->mSendOverlapped, sizeof(OVERLAPPED));

	retval = WSASend(pSession->mSocket, wsabufSend, index, nullptr, 0, &pSession->mSendOverlapped, nullptr);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			// Send 는 동기로 처리하기 때문에 WSA_IO_PENDING 리턴 시 끊는다.
			if (InterlockedDecrement64((INT64*)&pSession->mSessionState) == 0)
			{
				releaseSession(pSession);
			}
		}
	}

	return;
}

void CLanServer::kickSessions(void)
{
	mbSessionClearFlag = TRUE;

	for (INT index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index].mSessionState.bUseFlag == 0)
		{
			Disconnect(mSessionArray[index].mSessionID);
		}
	}

	if (mReleaseSessionStack.Size() != mMaxClientCount)
	{
		if (WAIT_OBJECT_0 != WaitForSingleObject(mSessionClearEvent, INFINITE))
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelDebug, L"LanServer", L"[cleanUpSessionArray] Error Code : %d", WSAGetLastError());

			CCrashDump::Crash();
		}
	}

	return;
}



BOOL CLanServer::setupSocket(void)
{
	INT retval = 0;

	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[setupSocket] socket Error Code : %d", WSAGetLastError());

		WSACleanup();
		return FALSE;
	}

	// KeepAlive 옵션
	BOOL enable = TRUE;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		WSACleanup();
		return FALSE;
	}

	if (mbNagleFlag == FALSE)
	{
		INT optval = TRUE;
		retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_NODELAY, (CHAR*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] Nagle No Delay setsockopt Error Code : %d", WSAGetLastError());

			return FALSE;
		}
	}

	// RST 옵션
	linger optval = { 0, };
	optval.l_onoff = 1;
	optval.l_linger = 0;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_LINGER, (CHAR*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		WSACleanup();
		return FALSE;
	}


	// 송신버퍼 0으로 만들기
	/*INT sendLen = 0;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_SNDBUF, (CHAR*)&sendLen, sizeof(sendLen));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LOG_LEVEL_ERROR, L"LanServer", L"[setupSocket] Zero setsockopt Error Code : %d", WSAGetLastError());
		return FALSE;
	}*/

	return TRUE;
}


BOOL CLanServer::bindSocket(void)
{
	INT retval = 0;

	SOCKADDR_IN serverAddr = { 0, };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mServerPort);
	InetPtonW(AF_INET, mServerIP, &serverAddr.sin_addr);

	retval = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[setupSocket] bindSocket Error Code : %d", WSAGetLastError());

		WSACleanup();
		return FALSE;
	}


	return TRUE;
}


BOOL CLanServer::listenSocket(void)
{
	INT retval = 0;

	retval = listen(mListenSocket, SOMAXCONN_HINT(USHRT_MAX));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[listenSocket] listen Error Code : %d", WSAGetLastError());

		WSACleanup();
		return FALSE;
	}

	return TRUE;
}


BOOL CLanServer::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[setupIocp] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		WSACleanup();

		return FALSE;
	}

	return TRUE;
}


BOOL CLanServer::setupSessionArray(void)
{
	mSessionArray = new stSession[mMaxClientCount];

	for (INT index = mMaxClientCount - 1; 0 <= index; --index)
	{
		mReleaseSessionStack.Push(mSessionArray[index].mIndex);
	}

	return TRUE;
}

void CLanServer::setupAcceptThread(void)
{
	mAcceptThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)ExecuteAcceptThread, this, NULL, &mAcceptThreadID);


	return;
}

void CLanServer::setupWorkerThread(void)
{
	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		mpWorkerThreadHandle[index] = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)ExecuteWorkerThread, this, NULL, &mpWorkerThreadID[index]);
	}

	return;
}


void CLanServer::setupThread(void)
{
	mpWorkerThreadHandle = new HANDLE[mWorkerThreadCount];

	mpWorkerThreadID = new UINT[mWorkerThreadCount];

	setupAcceptThread();

	setupWorkerThread();

	return;
}

void CLanServer::waitServerStartEvent(void)
{
	if (WaitForSingleObject(mServerStartEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CLanServer", L"[waitServerStartEvent] WaitForSingObject Error Code : %d", GetLastError());

		CCrashDump::Crash();
	}

	return;
}




BOOL CLanServer::setupNetwork(void)
{	
	if (setupSocket() == FALSE)
	{
		return FALSE;
	}

	if (bindSocket() == FALSE)
	{
		return FALSE;
	}

	if (listenSocket() == FALSE)
	{
		return FALSE;
	}

	if (setupIOCP() == FALSE)
	{
		return FALSE;
	}

	if (setupSessionArray() == FALSE)
	{
		return FALSE;
	}

	setupThread();

	return TRUE;
}



void CLanServer::checkCompleteRecv(stSession* pSession, DWORD transferred)
{
	INT retval;

	CRingBuffer* pRecvRingBuffer = &pSession->mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		INT recvSize = pRecvRingBuffer->GetUseSize();

		if (recvSize < sizeof(CMessage::stLanHeader))
		{
			break;
		}

		CMessage::stLanHeader header;
		retval = pRecvRingBuffer->Peek((CHAR*)&header, sizeof(CMessage::stLanHeader));
		if (retval != sizeof(CMessage::stLanHeader))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[checkCompleteRecv] RingBuffer Error Retval", retval);

			CCrashDump::Crash();
		}

		if (recvSize < sizeof(header) + header.payloadSize)
		{
			break;
		}

		pRecvRingBuffer->MoveFront(sizeof(header));

		CMessage* pMessage = CMessage::Alloc();

		retval = pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 링버퍼의 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[checkCompleteRecv] RingBuffer Error Retval, payloadSize : %d", retval, header.payloadSize);

			CCrashDump::Crash();
		}

		retval = pMessage->MoveWritePos(header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[checkCompleteRecv] Message Error Retval, payloadSize : %d", retval, header.payloadSize);

			CCrashDump::Crash();
		}

		try
		{
			OnRecv(pSession->mSessionID, pMessage);
		}
		catch (message::CExceptionObject exceptionObject)
		{
			exceptionObject.PrintExceptionData();

			Disconnect(pSession->mSessionID);
		}

		InterlockedIncrement(&mRecvTPS);

		pMessage->Free();
	}

	recvPost(pSession);

	return;
}


void CLanServer::checkCompleteSend(stSession* pSession, DWORD transferred)
{
	INT messageCount = pSession->mMessageCount;

	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	for (INT index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	// mbSendFlag 를 TRUE로 바꾸기전에 초기화하자. 
	pSession->mMessageCount = 0;

	// OOOE
	InterlockedExchange((LONG*)&pSession->mbSendFlag, TRUE);

	sendPost(pSession);

	return;
}


DWORD CLanServer::ExecuteAcceptThread(LPVOID lpParam)
{
	CLanServer* pThis = (CLanServer*)lpParam;

	pThis->AcceptThread();

	return 1;
}


void CLanServer::AcceptThread(void)
{
	waitServerStartEvent();

	OnStartAcceptThread();

	for (;;)
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr = { 0, };
		INT addrlen = sizeof(clientAddr);

		clientSocket = accept(mListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSocket == INVALID_SOCKET)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[AcceptThread] Accept, Error Code : %d", WSAGetLastError());

			break;
		}

		++mAcceptTotal;

		InterlockedIncrement(&mAcceptTPS);

		WCHAR clientIP[100] = { 0, };
		InetNtopW(AF_INET, &clientAddr.sin_addr, clientIP, _countof(clientIP));
		if (OnConnectionRequest(clientIP, ntohs(clientAddr.sin_port)) == FALSE)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[AcceptThread] Black IP : %s", clientIP);

			closesocket(clientSocket);

			continue;
		}

		if (mCurrentClientCount >= mMaxClientCount)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[AcceptThread] CurrentClient is full, Max Client : %d, Current Client : %d", mMaxClientCount, mCurrentClientCount);

			closesocket(clientSocket);

			continue;
		}

		if( initializeSession(clientSocket, clientAddr) == FALSE)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[AcceptThread] initializeSession return value is FALSE");

			CCrashDump::Crash();

			// 이거 없으면 경고문구 나옴
			continue;
		}
	}

	void OnCloseAcceptThread(void);

	return;
}


DWORD CLanServer::ExecuteWorkerThread(LPVOID lpParam)
{
	srand((unsigned)time(NULL));

	CLanServer* pThis = (CLanServer*)lpParam;

	pThis->WorkerThread();

	return 1;
}

void CLanServer::WorkerThread(void)
{
	waitServerStartEvent();

	OnStartWorkerThread();

	for (;;)
	{
		DWORD transferred = 0;

		stSession* pSession = nullptr;

		LPOVERLAPPED pOverlapped = nullptr;

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[WorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

			break;
		}


		if (transferred != 0 && pSession->mbDisconnectFlag == FALSE)
		{
			if (&pSession->mRecvOverlapped == pOverlapped)
			{
				checkCompleteRecv(pSession, transferred);
			}
			else if (&pSession->mSendOverlapped == pOverlapped)
			{
				checkCompleteSend(pSession, transferred);
			}
			else
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[WorkerThread] Exception Overapped, transffered : %d, Error Code : %d, Session ID : %lld\n", transferred, WSAGetLastError(), pSession->mSessionID);
			}
		}

		if (InterlockedDecrement64((INT64*)&pSession->mSessionState) == 0)
		{	
			releaseSession(pSession);
		}
	}

	OnCloseWorkerThread();

	return;
}
