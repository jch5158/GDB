#include "stdafx.h"
#include "CNetServer.h"



UINT64 gAcceptTotal = 0;

DWORD gUpdateTPS = 0;

DWORD gAcceptTPS = 0;

DWORD gSendTPS = 0;

DWORD gRecvTPS = 0;



//DWORD gSectorPlayer = 0;
//
//DWORD gSendPacket = 0;
//
//DWORD gSendPacketAround = 0;

// public 

CNetServer::CNetServer(void)
	: mIocpHandle(NULL)
	, mListenSocket(INVALID_SOCKET)
	, mServerIP{0,}
	, mServerPort(0)
	, mMaxClientCount(0)
	, mWorkerThreadCount(0)
	, mRunningThreadCount(0)
	, mpWorkerThreadHandle(nullptr)
	, mpWorkerThreadID(nullptr)
	, mAcceptThreadHandle(INVALID_HANDLE_VALUE)
	, mAcceptThreadID(0)
	, mTimeoutThreadHandle(INVALID_HANDLE_VALUE)
	, mTimeoutThreadID(0)
	, mbSessionClearFlag(FALSE)
	, mbTimeoutFlag(FALSE)
	, mbNagleFlag(TRUE)
	, mSessionClearEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
	, mSessionArray(nullptr)
	, mReleaseSessionStack()
	, mCurrentClientCount(0)
{
	WSADATA wsa;

	if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

}

CNetServer::~CNetServer(void)
{
	WSACleanup();
}




BOOL CNetServer::Start(const WCHAR* serverIP, DWORD serverPort, BOOL bNagleFlag, BOOL bTimeoutFlag, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount)
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

	wcscpy_s(mServerIP, serverIP);

	mServerPort = serverPort;

	mRunningThreadCount = runningThreadCount;

	mWorkerThreadCount = workerThreadCount;

	mMaxClientCount = maxClientCount;

	CMessage::mHeaderCode = headerCode;
	
	CMessage::mStaticKey = staticKey;

	mbNagleFlag = bNagleFlag;

	mbTimeoutFlag = bTimeoutFlag;

	if (setupNetwork() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CNetServer::Stop(void)
{	
	if (closeAcceptThread() == FALSE)
	{
		return FALSE;
	}

	closeTimeoutThread();

	OnStop();

	cleanupSessionArray();

	cleanupWorkerThread();

	return TRUE;
}

BOOL CNetServer::closeAcceptThread(void)
{	
	if (mListenSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	closesocket(mListenSocket);

	mListenSocket = INVALID_SOCKET;

	if (WAIT_OBJECT_0 != WaitForSingleObject(mAcceptThreadHandle, INFINITE))
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[closeAccept] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mAcceptThreadHandle);

	return TRUE;
}

void CNetServer::cleanupWorkerThread(void)
{
	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);
	}

	if (WAIT_FAILED == WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandle, TRUE, INFINITE))
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		CloseHandle(mpWorkerThreadHandle[index]);
	}

	CloseHandle(mIocpHandle);

	delete[] mpWorkerThreadHandle;

	delete[] mpWorkerThreadID;

}

inline void CNetServer::closeTimeoutThread(void)
{
	if (mbTimeoutFlag == FALSE)
	{
		return;
	}

	mbTimeoutFlag = FALSE;
	
	if (WAIT_OBJECT_0 != WaitForSingleObject(mTimeoutThreadHandle, INFINITE))
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[cleanupTimeoutThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mTimeoutThreadHandle);

	return;
}



BOOL CNetServer::GetTimeoutFlag(void)
{
	return mbTimeoutFlag;
}


BOOL CNetServer::GetNagleFlag(void)
{
	return mbNagleFlag;
}

DWORD CNetServer::GetClientCount(void)
{
	return mCurrentClientCount;
}

DWORD CNetServer::GetMaxClientCount(void)
{
	return mMaxClientCount;
}

DWORD CNetServer::GetRunningThreadCount(void)
{
	return mRunningThreadCount;
}

DWORD CNetServer::GetWorkerThreadCount(void)
{
	return mWorkerThreadCount;
}




inline CNetServer::stSession* CNetServer::createSession(SOCKET clientSocket, SOCKADDR_IN clientAddr)
{
	static UINT64 userID = 1;

	stSession* pSession = nullptr;

	UINT64 index;

	if (mReleaseSessionStack.Pop(&index) == TRUE)
	{
		InterlockedIncrement(&mCurrentClientCount);

		pSession = &mSessionArray[index];

		InterlockedIncrement64((LONGLONG*)&pSession->mSessionState);

		pSession->mSessionID = dfSESSION_ID(index, userID);
		
		pSession->mSocket = clientSocket;	
		
		pSession->mClientAddr = clientAddr;
		
		pSession->mSessionState.bUseFlag = 0;
		
		pSession->mbSendFlag = TRUE;
	
		pSession->mbDisconnectFlag = FALSE;

		++userID;

		return pSession;
	}

	return pSession;
}



inline CNetServer::stSession* CNetServer::findSession(UINT64 sessionID)
{
	UINT64 index = 0;

	index = dfSESSION_INDEX(sessionID);

	// mSessionArray 를 초과하는 index가 나올경우 return nullptr
	if (index < 0 || index >= mMaxClientCount)
	{
		return nullptr;
	}

	return &mSessionArray[index];
}


inline CNetServer::stSession* CNetServer::acquireSession(UINT64 sessionID)
{
	stSession* pSession = findSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	// session에 대한 referenceCount를 증가시킨다. 
	InterlockedIncrement64((LONGLONG*)&pSession->mSessionState);

	// 만약 SessionID 를 releaseSession에서 0으로 바꾸지 않는다면은 useFlag를 먼저 비교해야한다.
	if ( pSession->mSessionState.bUseFlag == 1 || pSession->mSessionID != sessionID)
	{		
		leaveSession(pSession);

		return nullptr;
	}

	return pSession;
}



inline void CNetServer::leaveSession(stSession* pSession)
{
	if (InterlockedDecrement64((LONGLONG*)&pSession->mSessionState) == 0)
	{
		releaseSession(pSession);
	}

	return;
}

//__inline bool CNetServer::SendPacket(UINT64 sessionID, CMessage* pMessage)
//{
//	stSession* pSession = acquireSession(sessionID);
//	if (pSession == nullptr)
//	{
//		//CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[SendPacket] Client Not Found, sessionID : %llx", sessionID);
//
//		return FALSE;
//	}
//
//	// 헤더 셋팅 그리고 체크섬과 같이 Encoding
//	pMessage->encode();
//
//	// SendQ에 Enqueue 하는 순간 다른 스레드에서 CMessage에 대한 참조가 가능해진다. 그러기 때문에 referenceCount를 증가시킨다.
//	pMessage->AddReferenceCount();
//
//	pSession->mSendQueue.Enqueue(pMessage);
//
//	sendPost(pSession);
//
//	++gSendTPS;
//
//	leaveSession(pSession);
//
//	return TRUE;
//}



inline BOOL CNetServer::Disconnect(UINT64 sessionID)
{

	stSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
	{
		return FALSE;
	}

	if (pSession->mbDisconnectFlag == FALSE)
	{
		pSession->mbDisconnectFlag = TRUE;

		CancelIoEx((HANDLE)pSession->mSocket, nullptr);
	}

	leaveSession(pSession);

	return TRUE;
}



inline void CNetServer::releaseSession(stSession* pSession)
{	
	stSessionState releaseSessionState = { 0,1 };

	if ((LONGLONG)0 != InterlockedCompareExchange64((LONGLONG*)&pSession->mSessionState, *(LONGLONG*)&releaseSessionState, (LONGLONG)0))
	{
		return;
	}

	OnClientLeave(pSession->mSessionID);
	
	closesocket(pSession->mSocket);

	//pSession->mSessionID = 0;	

	CLockFreeQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

	// SendQueue 에 남아있는 메시지들의 referenceCount를 차감시킨다.
	CMessage* pMessage = nullptr;
	while (pSendQueue->Dequeue(&pMessage) == TRUE)
	{
		pMessage->Free();
	}

	DWORD messageCount = pSession->mMessageCount;

	pSession->mMessageCount = 0;

	// 완료통지를 받아야할 mSendCompletionArray에 남아있는 메시지들을 정리한다.
	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	for (INT index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	// recv 버퍼에 남아있는 데이터를 정리한다.
	pSession->mRecvRingBuffer.ClearBuffer();

	mReleaseSessionStack.Push(pSession->mIndex);

	InterlockedDecrement(&mCurrentClientCount);

	// 전체 Session 끊기 위한 플래그
	if (mbSessionClearFlag == TRUE)
	{
		if (mCurrentClientCount == 0)
		{
			SetEvent(mSessionClearEvent);
		}
	}
}

inline void CNetServer::recvPost(stSession* pSession)
{
	WSABUF wsabufRecv[2];

	CRingBuffer* pRingBuffer = &pSession->mRecvRingBuffer;

	INT freeSize = pRingBuffer->GetFreeSize();
	INT directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = (DWORD)directEnqueueSize;

	wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
	wsabufRecv[1].len = (DWORD)(freeSize - directEnqueueSize);

	//if (freeSize - directEnqueueSize < 0)
	//{
	//	CCrashDump::Crash();
	//}

	DWORD flags = 0;
	DWORD recvBytes = 0;

	InterlockedIncrement64((LONGLONG*)&pSession->mSessionState);

	ZeroMemory(&pSession->mRecvOverlapped, sizeof(OVERLAPPED));

	// mSocket 이 블락 소켓이여도 수신 버퍼에 데이터가 없을 때 WOULDBLOCK이 아닌 WSA_IO_PENDING 이 에러가 발생된다.
	if (WSARecv(pSession->mSocket, wsabufRecv, 2, &recvBytes, &flags, &pSession->mRecvOverlapped, nullptr) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (InterlockedDecrement64((LONGLONG*)&pSession->mSessionState) == 0)
			{
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

inline void CNetServer::sendPost(stSession* pSession)
{
	CLockFreeQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

SEND_START:

	DWORD useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
	{
		return;
	}

	if (FALSE == (BOOL)InterlockedExchange((LONG*)&pSession->mbSendFlag, FALSE))
	{
		return;
	}

	if (useSize > MAX_WSABUF_SIZE)
	{
		useSize = MAX_WSABUF_SIZE;
	}

	CMessage **pSendCompletionArray = pSession->mSendCompletionArray;

	INT index;
	for (index = 0; index < useSize; ++index)
	{
		if (pSendQueue->Dequeue((CMessage**)&pSendCompletionArray[index]) == FALSE)
		{
			break;
		}
	}

	if (index == 0)
	{
		InterlockedExchange((LONG*)&pSession->mbSendFlag, TRUE);

		goto SEND_START;
	}

	pSession->mMessageCount = index;

	// 해당 Session 이 끊어진건지 연결중인지 판단이 어려움 
	// WSASend 는 해당 Session이 끊겼을 때 실패하지만, PQCS는 실패하지 않아서 애매하다.
	InterlockedIncrement64((LONGLONG*)&pSession->mSessionState);

	if (pSession->mSessionState.bUseFlag == 0)
	{
		if (PostQueuedCompletionStatus(mIocpHandle, 1, (ULONG_PTR)pSession, &pSession->mSendOverlapped) == FALSE)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[sendPost] PostQueuedCompletionStatus Error Code : %d", WSAGetLastError());

			CCrashDump::Crash();
		}
	}
	else
	{
		if (InterlockedDecrement64((LONGLONG*)&pSession->mSessionState) == 0)
		{
			releaseSession(pSession);
		}
	}

	return;
}

inline void CNetServer::cleanupSessionArray(void)
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
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[cleanUpSessionArray] Error Code : %d", WSAGetLastError());

			CCrashDump::Crash();
		}
	}

	UINT64 index = 0;

	for (;;)
	{
		if (mReleaseSessionStack.Pop(&index) == FALSE)
		{
			break;
		}
	}

	delete[] mSessionArray;
}



BOOL CNetServer::setupSocket(void)
{
	INT retval = 0;

	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] socket Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	////KeepAlive 옵션
	//int enable = 1;
	//retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

	//	return FALSE;
	//}

	////// keepalive 기능을 동작 시키기 위한 timeout 값
	//int keepidle = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&keepidle, sizeof(keepidle));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPIDLE setsockopt Error Code : %d", WSAGetLastError());

	//	return FALSE;
	//}

	//////keepalive probe packet을 보내는 갯수
	//int keepcnt = 5;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPCNT,(char*)&keepcnt, sizeof(keepcnt));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPCNT setsockopt Error Code : %d", WSAGetLastError());

	//	return FALSE;
	//}
	//

	//////probe packet을 보내는 간격
	//int keepintvl = 60;
	//retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&keepintvl, sizeof(keepintvl));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] TCP_KEEPINTVL setsockopt Error Code : %d", WSAGetLastError());

	//	return FALSE;
	//}

	if (mbNagleFlag == FALSE)
	{
		int optval = TRUE;
		retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
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
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	//송신버퍼 0으로 만들기
	//int sendLen = 0;
	//retval = setsockopt(mListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendLen, sizeof(sendLen));
	//if (retval == SOCKET_ERROR)
	//{
	//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] Zero setsockopt Error Code : %d", WSAGetLastError());
	//	return FALSE;
	//}

	return TRUE;
}


BOOL CNetServer::bindSocket(void)
{
	INT retval = 0;

	SOCKADDR_IN serverAddr = { 0, };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mServerPort);
	InetPtonW(AF_INET, mServerIP, &serverAddr.sin_addr);

	retval = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] bindSocket Error Code : %d", WSAGetLastError());

		return FALSE;
	}


	return TRUE;
}


BOOL CNetServer::listenSocket(void)
{
	INT retval = 0;

	retval = listen(mListenSocket, SOMAXCONN_HINT(65535));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[listenSocket] listen Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	return TRUE;
}


BOOL CNetServer::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupIOCP] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	return TRUE;
}


// 전체 Session 셋팅
BOOL CNetServer::setupSessionArray(void)
{
	mSessionArray = new stSession[mMaxClientCount];

	for (INT index = mMaxClientCount - 1; 0 <= index; --index)
	{
		mReleaseSessionStack.Push(mSessionArray[index].mIndex);
	}

	return TRUE;
}

void CNetServer::setupAcceptThread(void)
{
	mAcceptThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)AcceptThread, this, NULL, (UINT*)&mAcceptThreadID);

	return;
}

void CNetServer::setupWorkerThread(void)
{
	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		mpWorkerThreadHandle[index] = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)WorkerThread, this, NULL, (UINT*)&mpWorkerThreadID[index]);
	}

	return;
}

void CNetServer::setupTimoutThread(void)
{
	if (mbTimeoutFlag == FALSE)
	{
		return;
	}

	mTimeoutThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)TimeoutThread, this, NULL, (UINT*)&mTimeoutThreadID);


	return;
}



void CNetServer::setupThread(void)
{	
	mpWorkerThreadHandle = new HANDLE[mWorkerThreadCount];

	mpWorkerThreadID = new DWORD[mWorkerThreadCount];
	
	setupAcceptThread();

	setupWorkerThread();

	setupTimoutThread();

	return;
}


BOOL CNetServer::setupNetwork(void)
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



inline void CNetServer::checkCompletionRecv(stSession* pSession, DWORD transferred)
{
	CRingBuffer* pRecvRingBuffer = &pSession->mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		INT recvSize = pRecvRingBuffer->GetUseSize();

		if (recvSize < sizeof(CMessage::stNetHeader))
		{
			break;
		}

		CMessage::stNetHeader header;
		if (pRecvRingBuffer->Peek((CHAR*)&header, sizeof(header)) != sizeof(header))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		if (header.code != CMessage::mHeaderCode)
		{
			Disconnect(pSession->mSessionID);
					
			break;
		}

		if (header.payloadSize > 9000)
		{
			Disconnect(pSession->mSessionID);
	
			break;
		}

		if (recvSize < sizeof(CMessage::stNetHeader) + header.payloadSize)
		{
			break;
		}

		pRecvRingBuffer->MoveFront(sizeof(CMessage::stNetHeader));

		CMessage *pMessage = CMessage::Alloc();

		if (pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize) != header.payloadSize)
		{
			// 내 링버퍼의 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		if (pMessage->MoveWritePos(header.payloadSize) != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] Message Error Retval");

			CCrashDump::Crash();
		}	

		if (pMessage->decode(&header) == FALSE)
		{
			Disconnect(pSession->mSessionID);

			pMessage->Free();

			return;
		}

		OnRecv(pSession->mSessionID, pMessage);

		InterlockedIncrement(&gRecvTPS);

		pMessage->Free();
	}

	recvPost(pSession);

	return;
}


inline void CNetServer::checkCompletionSend(stSession* pSession)
{
	DWORD messageCount = pSession->mMessageCount;

	// 여기서는 필요 없음
	//ZeroMemory(&pSession->mSendOverlapped, sizeof(OVERLAPPED));

	WSABUF wsabufSend[MAX_WSABUF_SIZE];
	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	for (INT index = 0; index < messageCount; ++index)
	{
		wsabufSend[index].buf = pSendCompletionArray[index]->GetMessagePtr();
		wsabufSend[index].len = pSendCompletionArray[index]->GetUseSize();		
	}

	DWORD sendBytes = 0;

	// Overapped 포인터에 nullptr 을 넣어서 동기로 처리되도록 한다. 넌블락 소켓으로 전환하여 수신 버퍼가 꽉찰 경우 WOULD_BLOCK을 확인해서 끊는다.
	if (WSASend(pSession->mSocket, wsabufSend, messageCount, &sendBytes, 0, nullptr, nullptr) == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			Disconnect(pSession->mSessionID);
		}
	}
	else
	{
		for (INT index = 0; index < messageCount; ++index)
		{
			pSendCompletionArray[index]->Free();
		}

		pSession->mMessageCount = 0;

		// OOOE 
		InterlockedExchange((LONG*)&pSession->mbSendFlag, TRUE);

		sendPost(pSession);
	}

	return;
}


DWORD CNetServer::AcceptThread(LPVOID lpParam)
{
	CNetServer* pThis = (CNetServer*)lpParam;

	pThis->executeAcceptThread();

	return 1;
}


void CNetServer::executeAcceptThread(void)
{
	for (;;)
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr = { 0, };
		INT addrlen = sizeof(clientAddr);

		clientSocket = accept(mListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSocket == INVALID_SOCKET)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] Accept, Error Code : %d", WSAGetLastError());

			break;
		}

		++gAcceptTotal;

		++gAcceptTPS;

		{
			//CPerformanceProfiler profiler(L"AcceptThread");

			WCHAR clientIP[100] = { 0, };
			InetNtopW(AF_INET, &clientAddr.sin_addr, clientIP, _countof(clientIP));
			if (OnConnectionRequest(clientIP, ntohs(clientAddr.sin_port)) == FALSE)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] Black IP : %s", clientIP);

				closesocket(clientSocket);

				continue;
			}

			if (mCurrentClientCount >= mMaxClientCount)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] CurrentClient is full, Max Client : %d, Current Client : %d", mMaxClientCount, mCurrentClientCount);

				closesocket(clientSocket);

				continue;
			}

			// 넌블럭 소켓으로 전환 Listen 소켓에 적용하게 된다면은 accept가 넌블럭으로 처리되기 때문에 client 소켓에만 넌블럭 설정을 한다.
			ULONG on = 1;
			if (ioctlsocket(clientSocket, FIONBIO, &on) == SOCKET_ERROR)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] ioctlsocket Error Code : %d", WSAGetLastError());

				closesocket(clientSocket);

				continue;
			}
		
			// 동기처리에 대한 완료통지 안 받기 
			//if (SetFileCompletionNotificationModes((HANDLE)clientSocket, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) == FALSE)
			//{
			//	CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] SetFileCompletionNotificationModes Error Code : %d", WSAGetLastError());

			//	closesocket(clientSocket);

			//	continue;
			//}


			stSession* pSession = createSession(clientSocket, clientAddr);
			if (pSession == nullptr)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] CreateSession Error");

				CCrashDump::Crash();
				// 이거 없으면 경고문구 나옴
				continue;
			}


			if (CreateIoCompletionPort((HANDLE)clientSocket, mIocpHandle, (ULONG_PTR)pSession, NULL) == NULL)
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] CreateSession Error CreateIoCompletionPort Error Code : % d", WSAGetLastError());
				CCrashDump::Crash();
			}

			OnClientJoin(pSession->mSessionID);

			recvPost(pSession);

			if (InterlockedDecrement64((LONGLONG*)&pSession->mSessionState) == 0)
			{
				releaseSession(pSession);
			}
		}
	}


	return;
}


DWORD CNetServer::WorkerThread(LPVOID lpParam)
{
	srand((unsigned)time(NULL));

	CNetServer* pThis = (CNetServer*)lpParam;

	pThis->executeWorkerThread();

	return 1;
}

void CNetServer::executeWorkerThread(void)
{
	for (;;)
	{
		DWORD transferred = 0;

		stSession* pSession = nullptr;

		LPOVERLAPPED pOverlapped = nullptr;

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);

		if (pOverlapped == nullptr)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateWorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

			break;
		}

		if (transferred != 0 && pSession->mbDisconnectFlag == FALSE)
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
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateWorkerThread] Exception Overapped, transffered : %d, Error Code : %d, Session ID : %llx\n", transferred, WSAGetLastError(), pSession->mSessionID);
			}
		}

		if (InterlockedDecrement64((LONGLONG*)&pSession->mSessionState) == 0)
		{
			releaseSession(pSession);
		}
	}

	return;
}



DWORD CNetServer::TimeoutThread(LPVOID lpParam)
{
	CNetServer* pThis = (CNetServer*)lpParam;

	pThis->executeTimeoutThread();

	return 1;
}

void CNetServer::executeTimeoutThread(void)
{	
	while (mbTimeoutFlag == TRUE)
	{
		for (INT index = 0; index < mMaxClientCount; ++index)
		{
			if (mSessionArray[index].mSessionState.bUseFlag == 0)
			{
				OnTimeout(mSessionArray[index].mSessionID);	
			}			
		}

		Sleep(1000);
	}

	return;
}



