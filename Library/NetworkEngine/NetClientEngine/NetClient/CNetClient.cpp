#include "stdafx.h"


// public 

CNetClient::CNetClient(void)
	: mIocpHandle(INVALID_HANDLE_VALUE)
	, mServerIP{0,}
	, mServerPort(0)
	, mWorkerThreadCount(0)
	, mRunningThreadCount(0)
	, mpWorkerThreadHandle(nullptr)
	, mpWorkerThreadID(nullptr)
	, mTimeoutThreadHandle(INVALID_HANDLE_VALUE)
	, mSessionClearEvent(CreateEvent(NULL,FALSE,FALSE,NULL))
	, mTimeoutThreadID(0)
	, mbNagleFlag(TRUE)
	, mbTimeoutFlag(FALSE)
	, mTimeoutPeriod(0)
	, mSession()
{
	WSADATA wsa;

	if (WSAStartup(WINSOCK_VERSION, &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}
}

CNetClient::~CNetClient(void)
{
	WSACleanup();
}



BOOL CNetClient::Start(const WCHAR* serverIP, WORD serverPort, BOOL bNagleFlag, BOOL bTimeoutFlag, DWORD timeoutPeriod, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount)
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

	if (timeoutPeriod < 0)
	{
		return FALSE;
	}

	
	mServerIP = (WCHAR*)serverIP;

	mServerPort = serverPort;

	mbNagleFlag = bNagleFlag;

	mbTimeoutFlag = bTimeoutFlag;

	mTimeoutPeriod = timeoutPeriod;

	CMessage::mHeaderCode = headerCode;

	CMessage::mStaticKey = staticKey;

	mRunningThreadCount = runningThreadCount;

	mWorkerThreadCount = workerThreadCount;		

	if (setupNetwork() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CNetClient::Stop(void)
{		
	if (mIocpHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	closeTimeoutThread();

	OnStop();

	Disconnect();

	checkSessionConnection();

	closeWorkerThreads();

	closeIOCP();

	return TRUE;
}


void CNetClient::closeWorkerThreads(void)
{
	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);
	}

	if (WAIT_FAILED == WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandle, TRUE, INFINITE))
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

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

void CNetClient::closeIOCP(void)
{
	CloseHandle(mIocpHandle);

	mIocpHandle = INVALID_HANDLE_VALUE;

	return;
}


void CNetClient::closeTimeoutThread(void)
{
	if (mbTimeoutFlag == FALSE)
	{
		return;
	}

	mbTimeoutFlag = FALSE;
	
	if (WaitForSingleObject(mTimeoutThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[cleanupTimeoutThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mTimeoutThreadHandle);

	return;
}

void CNetClient::checkSessionConnection(void)
{
	if (WaitForSingleObject(mSessionClearEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[cleanupTimeoutThread] WaitForSingleObject Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	return;
}


inline BOOL CNetClient::SendPacket(CMessage* pMessage)
{	
	if (acquireSession() == FALSE)
	{
		return FALSE;
	}

	// 헤더 셋팅 그리고 체크섬과 같이 Encoding
	pMessage->encode();

	// SendQ에 Enqueue 하는 순간 다른 스레드에서 CMessage에 대한 참조가 가능해진다. 그러기 때문에 referenceCount를 증가시킨다.
	pMessage->AddReferenceCount();

	mSession.mSendQueue.Enqueue(pMessage);
	
	sendPost();

	leaveSession();

	return TRUE;
}

inline BOOL CNetClient::Disconnect(void)
{
	if (acquireSession() == FALSE)
	{
		return FALSE;
	}

	if (mSession.mbDisconnectFlag == FALSE)
	{
		mSession.mbDisconnectFlag = TRUE;
		  	
		CancelIoEx((HANDLE)mSession.mSocket, nullptr);
	}		

	leaveSession();

	return TRUE;
}

inline BOOL CNetClient::GetTimeoutFlag(void)
{
	return mbTimeoutFlag;
}

inline DWORD CNetClient::GetRunningThreadCount(void)
{
	return mRunningThreadCount;
}

inline DWORD CNetClient::GetWorkerThreadCount(void)
{
	return mWorkerThreadCount;
}


inline BOOL CNetClient::GetConnectionState(void)
{
	if (mSession.mSessionState.bUseFlag == 1)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

inline BOOL CNetClient::acquireSession(void)
{
	InterlockedIncrement64((INT64*)&mSession.mSessionState);

	if (mSession.mSessionState.bUseFlag == 1)
	{
		leaveSession();

		return FALSE;
	}

	return TRUE;
}

inline void CNetClient::leaveSession(void)
{
	if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
	{
		releaseSession();
	}

	return;
}


void CNetClient::releaseSession(void)
{	
	stSessionState releaseSessionState = { 0,1 };

	if ((INT64)0 != InterlockedCompareExchange64((INT64*)&mSession.mSessionState, *(INT64*)&releaseSessionState, (INT64)0))
	{
		return;
	}

	OnServerLeave();
	
	closesocket(mSession.mSocket);

	CLockFreeQueue<CMessage*>* pSendQueue = &mSession.mSendQueue;

	// SendQueue 에 남아있는 메시지들의 referenceCount를 차감시킨다.
	CMessage* pMessage;
	while (pSendQueue->Dequeue(&pMessage) == TRUE)
	{
		pMessage->Free();
	}

	INT messageCount = mSession.mMessageCount;

	mSession.mMessageCount = 0;

	// 완료통지를 받아야할 SendQueue에 남아있는 메시지들을 정리한다.
	CMessage** pSendCompletionArray = mSession.mSendCompletionArray;

	for (INT index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	// recv 버퍼에 남아있는 데이터를 정리한다.
	mSession.mRecvRingBuffer.ClearBuffer();

	SetEvent(mSessionClearEvent);

	return;
}

inline void CNetClient::recvPost(void)
{
	INT retval;

	WSABUF wsabufRecv[2];

	CRingBuffer* pRingBuffer = &mSession.mRecvRingBuffer;

	INT freeSize = pRingBuffer->GetFreeSize();
	INT directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = directEnqueueSize;
	
	wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
	wsabufRecv[1].len = freeSize - directEnqueueSize;

	DWORD flags = 0;

	InterlockedIncrement64((INT64*)&mSession.mSessionState);

	ZeroMemory(&mSession.mRecvOverlapped, sizeof(OVERLAPPED));

	retval = WSARecv(mSession.mSocket, wsabufRecv, 2, nullptr, &flags, &mSession.mRecvOverlapped, nullptr);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
			{
				releaseSession();
			}
		}
	}

	if (mSession.mbDisconnectFlag == TRUE)
	{
		CancelIoEx((HANDLE)mSession.mSocket, &mSession.mRecvOverlapped);
	}

	return;
}



inline void CNetClient::sendPost(void)
{
	INT retval;

	CLockFreeQueue<CMessage*>* pSendQueue = &mSession.mSendQueue;

SEND_START:

	INT useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
	{
		return;
	}

	if (FALSE == (BOOL)InterlockedExchange((LONG*)&mSession.mbSendFlag, FALSE))
	{
		return;
	}

	WSABUF wsabufSend[netclient::MAX_WSABUF_SIZE];
	CMessage** pSendCompletionArray = mSession.mSendCompletionArray;

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
		InterlockedExchange((LONG*)&mSession.mbSendFlag, TRUE);
	
		goto SEND_START;
	}

	mSession.mMessageCount = index;

	InterlockedIncrement64((INT64*)&mSession.mSessionState);
	
	ZeroMemory(&mSession.mSendOverlapped, sizeof(OVERLAPPED));

	retval = WSASend(mSession.mSocket, wsabufSend, index, nullptr, 0, &mSession.mSendOverlapped, nullptr);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
			{
				releaseSession();
			}
		}
	}

	return;
}



BOOL CNetClient::setupSocket(void)
{
	INT retval;

	mSession.mSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mSession.mSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[setupSocket] socket Error Code : %d", WSAGetLastError());

		
		return FALSE;
	}

	// KeepAlive 옵션
	BOOL enable = TRUE;
	retval = setsockopt(mSession.mSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		
		return FALSE;
	}

	if (mbNagleFlag == FALSE)
	{
		INT optval = TRUE;
		retval = setsockopt(mSession.mSocket, IPPROTO_TCP, TCP_NODELAY, (CHAR*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[setupSocket] Nagle No Delay setsockopt Error Code : %d", WSAGetLastError());

			return FALSE;
		}
	}

	// RST 옵션
	linger optval = { 0, };
	optval.l_onoff = 1;
	optval.l_linger = 0;
	retval = setsockopt(mSession.mSocket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		
		return FALSE;
	}

	// 송신버퍼 0으로 만들기
	/*INT sendLen = 0;
	retval = setsockopt(mConnectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendLen, sizeof(sendLen));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LOG_LEVEL_ERROR, L"NetClient", L"[setupSocket] Zero setsockopt Error Code : %d", WSAGetLastError(void));
		return FALSE;
	}*/

	return TRUE;
}


void CNetClient::connectServer(void)
{
	INT retval;

	SOCKADDR_IN serverAddr = { 0, };	
	serverAddr.sin_family = AF_INET;	
	serverAddr.sin_port = htons(mServerPort);
	
	InetPtonW(AF_INET, mServerIP, &serverAddr.sin_addr);

	retval = connect(mSession.mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[setupSocket] bindSocket Error Code : %d", WSAGetLastError());	

		CCrashDump::Crash();
	}

	if (initializeSession() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[connectServer] initializeSession is Failed");

		CCrashDump::Crash();
	}

	return;
}


BOOL CNetClient::initializeSession(void)
{
	mSession.mbDisconnectFlag = FALSE;

	mSession.mbSendFlag = TRUE;

	mSession.mMessageCount = 0;

	mSession.mSessionState = { 1,0 };

	associateSocketWithIOCP();

	OnServerJoin();

	recvPost();

	if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
	{
		releaseSession();

		return FALSE;
	}

	return TRUE;
}

void CNetClient::associateSocketWithIOCP(void)
{
	if (CreateIoCompletionPort((HANDLE)mSession.mSocket, mIocpHandle, (ULONG_PTR)nullptr, NULL) == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[associateSocketWithIOCP] CreateIoCompletionPort Error Code : % d", WSAGetLastError());

		CCrashDump::Crash();
	}

	return;
}




BOOL CNetClient::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[setupIOCP] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	return TRUE;
}



BOOL CNetClient::setupWorkerThreads(void)
{
	mpWorkerThreadHandle = new HANDLE[mWorkerThreadCount];

	mpWorkerThreadID = new UINT[mWorkerThreadCount];

	for (INT index = 0; index < mWorkerThreadCount; ++index)
	{
		mpWorkerThreadHandle[index] = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)ExecuteWorkerThread, this, NULL, &mpWorkerThreadID[index]);
		if (mpWorkerThreadHandle[index] == NULL || mpWorkerThreadHandle[index] == INVALID_HANDLE_VALUE)
		{
			delete[] mpWorkerThreadHandle;

			delete[] mpWorkerThreadID;

			return FALSE;
		}	
	}

	return TRUE;
}

BOOL CNetClient::setupTimoutThread(void)
{
	if (mbTimeoutFlag == FALSE)
	{
		return TRUE;
	}

	mTimeoutThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (_beginthreadex_proc_type)ExecuteTimeoutThread, this, NULL, &mTimeoutThreadID);
	if (mTimeoutThreadHandle == NULL || mTimeoutThreadHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL CNetClient::setupNetwork(void)
{
	if (setupSocket() == FALSE)
	{
		return FALSE;
	}

	if (setupIOCP() == FALSE)
	{
		return FALSE;
	}

	if (setupTimoutThread() == FALSE)
	{
		return FALSE;
	}

	if (setupWorkerThreads() == FALSE)
	{
		return FALSE;
	}

	connectServer();

	return TRUE;
}



inline void CNetClient::checkCompleteRecv(DWORD transferred)
{
	INT retval;

	CRingBuffer* pRecvRingBuffer = &mSession.mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		INT recvSize = pRecvRingBuffer->GetUseSize();

		if (recvSize < sizeof(CMessage::stNetHeader))
		{
			break;
		}

		CMessage::stNetHeader header;
		retval = pRecvRingBuffer->Peek((char*)&header, sizeof(header));
		if (retval != sizeof(header))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[checkCompleteRecv] RingBuffer Error Retval", retval);

			CCrashDump::Crash();
		}

		if (header.code != CMessage::mHeaderCode)
		{
			Disconnect();
					
			break;
		}

		if (header.payloadSize > 9000)
		{
			Disconnect();
	
			break;
		}

		if (recvSize < sizeof(CMessage::stNetHeader) + header.payloadSize)
		{
			break;
		}

		pRecvRingBuffer->MoveFront(sizeof(CMessage::stNetHeader));

		CMessage *pMessage = CMessage::Alloc();

		retval = pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 링버퍼의 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[checkCompleteRecv] RingBuffer Error Retval, payloadSize : %d", retval, header.payloadSize);

			CCrashDump::Crash();
		}

		retval = pMessage->MoveWritePos(header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[checkCompleteRecv] Message Error Retval, payloadSize : %d", retval, header.payloadSize);

			CCrashDump::Crash();
		}
		

		if (pMessage->decode(&header) == FALSE)
		{
			Disconnect();

			pMessage->Free();

			return;
		}

		OnRecv(pMessage);

		pMessage->Free();
	}

	recvPost();

	return;
}


inline void CNetClient::checkCompleteSend(void)
{
	INT messageCount = 0;

	CMessage** pSendCompletionArray = mSession.mSendCompletionArray;

	messageCount = mSession.mMessageCount;

	for (INT index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	mSession.mMessageCount = 0;

	// OOOE 
	InterlockedExchange((LONG*)&mSession.mbSendFlag, TRUE);

	sendPost();

	return;
}


DWORD CNetClient::ExecuteWorkerThread(LPVOID pParam)
{
	srand((unsigned)time(NULL));

	CNetClient* pThis = (CNetClient*)pParam;

	pThis->WorkerThread();

	return 1;
}

void CNetClient::WorkerThread(void)
{
	for (;;)
	{
		DWORD transferred = 0;

		CSession* pSession = nullptr;

		LPOVERLAPPED pOverlapped = nullptr;

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[UpdateWorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

			break;
		}

		if (transferred != 0 && mSession.mbDisconnectFlag == FALSE)
		{
			if (&mSession.mRecvOverlapped == pOverlapped)
			{
				checkCompleteRecv(transferred);
			}
			else if (&mSession.mSendOverlapped == pOverlapped)
			{
				checkCompleteSend();
			}
			else
			{
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetClient", L"[WorkerThread] pOverlapped Exception Error Code : %d\n", WSAGetLastError());

				CCrashDump::Crash();
			}
		}

		if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
		{
			releaseSession();
		}
	}

	return;
}



DWORD CNetClient::ExecuteTimeoutThread(LPVOID pParam)
{
	CNetClient* pThis = (CNetClient*)pParam;

	pThis->TimeoutThread();

	return 1;
}

void CNetClient::TimeoutThread(void)
{	
	while (mbTimeoutFlag == TRUE)
	{	
		OnTimeout();

		Sleep(mTimeoutPeriod);
	}

	return;
}



