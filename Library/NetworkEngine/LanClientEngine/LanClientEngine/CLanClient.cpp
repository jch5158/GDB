#include "stdafx.h"
#include "CLanClient.h"


// public 
CLanClient::CLanClient(void)
	: mClientStartEvent(CreateEvent(NULL,TRUE,FALSE,nullptr))
	, mIocpHandle(INVALID_HANDLE_VALUE)
	, mSessionClearEvent(CreateEvent(NULL,FALSE,FALSE,nullptr))
	, mbSessionClearFlag(FALSE)
	, mbNagleFlag(TRUE)
	, mServerIP{0,}
	, mServerPort(0)
	, mWorkerThreadCount(0)
	, mRunningThreadCount(0)
	, mpWorkerThreadHandle(nullptr)
	, mpWorkerThreadID(nullptr)
	, mSession()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}
}

CLanClient::~CLanClient(void)
{
	
	WSACleanup();
}



BOOL CLanClient::Start(const WCHAR* serverIP, WORD serverPort, BOOL bNagleFlag, DWORD runningThreadCount, DWORD workerThreadCount)
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
	
	HRESULT retval = StringCbCopyW(mServerIP, lanclient::MAX_IP_LENGTH, serverIP);
	if (FAILED(retval) == TRUE)
	{
		return FALSE;
	}

	mServerPort = serverPort;

	mbNagleFlag = bNagleFlag;

	mRunningThreadCount = runningThreadCount;

	mWorkerThreadCount = workerThreadCount;

	if (setupNetwork() == FALSE)
	{
		return FALSE;
	}

	if (OnStart() == FALSE)
	{
		return FALSE;
	}

	SetEvent(mClientStartEvent);

	return TRUE;
}

BOOL CLanClient::Stop(void)
{	
	if (mIocpHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	
	mbSessionClearFlag = TRUE;

	OnStop();

	Disconnect(mSession.mSessionID);

	if (mSession.mSessionState.bUseFlag == 0)
	{
		checkSessionClearEvent();
	}

	cleanupWorkerThreads();

	closeIOCP();

	return TRUE;
}


BOOL CLanClient::Connect(void)
{
	BOOL retval = TRUE;

	if (mbSessionClearFlag == FALSE)
	{
		setupSocket();

		retval = serverConnect();
	}

	return retval;
}

void CLanClient::checkSessionClearEvent(void)
{
	if (WaitForSingleObject(mSessionClearEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"[LanClient]", L"[checkSessionClearEvent] WaitForSingleObject Error : %d", GetLastError());
	
		CCrashDump::Crash();
	}

	return;
}


void CLanClient::cleanupWorkerThreads(void)
{
	for (DWORD index = 0; index < mWorkerThreadCount; ++index)
	{
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);
	}

	if (WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandle, TRUE, INFINITE) == WAIT_FAILED)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	for (DWORD index = 0; index < mWorkerThreadCount; ++index)
	{
		CloseHandle(mpWorkerThreadHandle[index]);
	}
	
	delete[] mpWorkerThreadHandle;

	delete[] mpWorkerThreadID;

	return;
}




void CLanClient::closeIOCP(void)
{
	CloseHandle(mIocpHandle);

	mIocpHandle = INVALID_HANDLE_VALUE;

	return;
}


void CLanClient::waitClientStartEvent(void)
{
	if (WaitForSingleObject(mClientStartEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[waitClientStartEvent] WaitForMultipleObjects Error Code : %d", GetLastError());

		CCrashDump::Crash();
	}

	return;
}


BOOL CLanClient::SendPacket(UINT64 sessionID,CMessage* pMessage)
{	
	if (acquireSession(sessionID) == FALSE)
	{
		return FALSE;
	}

	CMessage::stLanHeader header;

	header.payloadSize = (WORD)pMessage->GetUseSize();

	pMessage->setLanHeader(&header);

	// SendQ에 Enqueue 하는 순간 다른 스레드에서 CMessage에 대한 참조가 가능해진다. 그러기 때문에 referenceCount를 증가시킨다.
	pMessage->AddReferenceCount();

	mSession.mSendQueue.Enqueue(pMessage);
	
	sendPost();

	leaveSession();

	return TRUE;
}

BOOL CLanClient::Disconnect(UINT64 sessionID)
{
	if (acquireSession(sessionID) == FALSE)
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

WCHAR* CLanClient::GetConnectIP(void) const
{
	return (WCHAR*)mServerIP;
}

WORD CLanClient::GetConnectPort(void) const
{
	return (WORD)mServerPort;
}


DWORD CLanClient::GetRunningThreadCount(void) const
{
	return mRunningThreadCount;
}

DWORD CLanClient::GetWorkerThreadCount(void) const
{
	return mWorkerThreadCount;
}

BOOL CLanClient::GetConnectionState(void) const
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

BOOL CLanClient::GetNagleFlag(void) const
{
	return mbNagleFlag;
}


BOOL CLanClient::acquireSession(UINT64 sessionID)
{
	InterlockedIncrement64((INT64*)&mSession.mSessionState);

	if (mSession.mSessionState.bUseFlag == 1 || mSession.mSessionID != sessionID)
	{
		leaveSession();

		return FALSE;
	}

	return TRUE;
}

void CLanClient::leaveSession(void)
{
	if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
	{
		releaseSession();
	}

	return;
}


void CLanClient::releaseSession(void)
{	
	stSessionState releaseSessionState = { 0,1 };

	if ((INT64)0 != InterlockedCompareExchange64((INT64*)&mSession.mSessionState, *(INT64*)&releaseSessionState, (INT64)0))
	{
		return;
	}

	closesocket(mSession.mSocket);

	mSession.mSocket = INVALID_SOCKET;

	clearSendQueue();

	clearSendCompletionArray();

	// recv 버퍼에 남아있는 데이터를 정리한다.
	mSession.mRecvRingBuffer.ClearBuffer();
	
	OnServerLeave(mSession.mSessionID);

	if (mbSessionClearFlag == TRUE)
	{
		SetEvent(mSessionClearEvent);
	}

	return;
}

void CLanClient::clearSendQueue(void)
{
	CLockFreeQueue<CMessage*>* pSendQueue = &mSession.mSendQueue;

	// SendQueue 에 남아있는 메시지들의 referenceCount를 차감시킨다.
	CMessage* pMessage;
	while (pSendQueue->Dequeue(&pMessage) == TRUE)
	{
		pMessage->Free();
	}

	return;
}

void CLanClient::clearSendCompletionArray(void)
{
	INT messageCount = mSession.mMessageCount;

	mSession.mMessageCount = 0;

	// 완료통지를 받아야할 SendQueue에 남아있는 메시지들을 정리한다.
	CMessage** pSendCompletionArray = mSession.mSendCompletionArray;

	for (INT index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	return;
}

BOOL CLanClient::serverConnect(void)
{
	SOCKADDR_IN serverAddr = { 0, };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((WORD)mServerPort);
	InetPtonW(AF_INET, mServerIP, &serverAddr.sin_addr);

	INT retval = connect(mSession.mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[connectServer] connect Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	if (initializeSession() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[connectServer] initializeSession is Failed");

		CCrashDump::Crash();
	}

	return TRUE;
}

void CLanClient::recvPost(void)
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

	retval = WSARecv(mSession.mSocket, wsabufRecv, 2, NULL, &flags, &mSession.mRecvOverlapped, nullptr);
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


void CLanClient::sendPost(void)
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

	WSABUF wsabufSend[lanclient::MAX_WSABUF_SIZE];
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



BOOL CLanClient::setupSocket(void)
{
	INT retval;

	// 이미 소켓이 셋팅되어있다면 다시 셋팅하지 않는다.
	if (mSession.mSocket != INVALID_SOCKET)
	{
		return TRUE;
	}

	mSession.mSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mSession.mSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[setupSocket] socket Error Code : %d", WSAGetLastError());
	
		return FALSE;
	}

	// KeepAlive 옵션
	BOOL enable = TRUE;
	retval = setsockopt(mSession.mSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		
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
	retval = setsockopt(mSession.mSocket, SOL_SOCKET, SO_LINGER, (CHAR*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[setupSocket] RST setsockopt Error Code : %d", WSAGetLastError());

		
		return FALSE;
	}

	// 송신버퍼 0으로 만들기
	/*INT sendLen = 0;
	retval = setsockopt(mConnectSocket, SOL_SOCKET, SO_SNDBUF, (CHAR*)&sendLen, sizeof(sendLen));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LOG_LEVEL_ERROR, L"LanClient", L"[setupSocket] Zero setsockopt Error Code : %d", WSAGetLastError(void));
		return FALSE;
	}*/

	return TRUE;
}

BOOL CLanClient::initializeSession(void)
{
	static UINT64 sessionID = 0;

	InterlockedIncrement64((INT64*)&mSession.mSessionState);

	// bUseFlag 보다 sessionID를 먼저 셋팅해주어야 한다.
	mSession.mSessionID = sessionID++;

	mSession.mbDisconnectFlag = FALSE;

	mSession.mbSendFlag = TRUE;

	mSession.mMessageCount = 0;

	mSession.mSessionState.bUseFlag = 0;

	associateSocketWithIOCP();

	OnServerJoin(mSession.mSessionID);

	recvPost();

	if (InterlockedDecrement64((INT64*)&mSession.mSessionState) == 0)
	{
		releaseSession();

		return FALSE;
	}

	return TRUE;
}

void CLanClient::associateSocketWithIOCP(void)
{
	if (CreateIoCompletionPort((HANDLE)mSession.mSocket, mIocpHandle, (ULONG_PTR)nullptr, NULL) == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[associateSocketWithIOCP] CreateIoCompletionPort Error Code : % d", WSAGetLastError());

		CCrashDump::Crash();
	}

	return;
}




BOOL CLanClient::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[setupIOCP] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		return FALSE;
	}

	return TRUE;
}





BOOL CLanClient::setupWorkerThreads(void)
{	
	mpWorkerThreadHandle = new HANDLE[mWorkerThreadCount];

	mpWorkerThreadID = new UINT[mWorkerThreadCount];
	
	for (DWORD index = 0; index < mWorkerThreadCount; ++index)
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


BOOL CLanClient::setupNetwork(void)
{
	if (setupSocket() == FALSE)
	{
		return FALSE;
	}

	if (setupIOCP() == FALSE)
	{	
		return FALSE;
	}

	if (setupWorkerThreads() == FALSE)
	{
		return FALSE;
	}

	//if (Connect() == FALSE)
	//{
	//	return FALSE;
	//}

	return TRUE;
}



void CLanClient::checkCompleteRecv(DWORD transferred)
{
	INT retval;

	CRingBuffer* pRecvRingBuffer = &mSession.mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		INT recvSize = pRecvRingBuffer->GetUseSize();

		if (recvSize < sizeof(CMessage::stLanHeader))
		{
			break;
		}

		CMessage::stLanHeader header;
		retval = pRecvRingBuffer->Peek((CHAR*)&header, sizeof(header));
		if (retval != sizeof(header))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[checkCompleteRecv] RingBuffer Error Retval", retval);

			CCrashDump::Crash();
		}


		if (recvSize < sizeof(CMessage::stLanHeader) + header.payloadSize)
		{
			break;
		}

		pRecvRingBuffer->MoveFront(sizeof(CMessage::stLanHeader));

		CMessage *pMessage = CMessage::Alloc();

		retval = pRecvRingBuffer->Dequeue(pMessage->GetMessagePtr(), header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 링버퍼의 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[checkCompleteRecv] RingBuffer Error Retval, payloadSize : %d", retval, header.payloadSize);

			CCrashDump::Crash();
		}

		retval = pMessage->MoveWritePos(header.payloadSize);
		if (retval != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[checkCompleteRecv] Message Error Retval, payloadSize : %d", retval, header.payloadSize);

			CCrashDump::Crash();
		}
	
		try
		{
			OnRecv(mSession.mSessionID, pMessage);
		}
		catch (const message::CExceptionObject &exceptionObject)
		{
			exceptionObject.PrintExceptionData();

			Disconnect(mSession.mSessionID);
		}
		
		pMessage->Free();
	}

	recvPost();

	return;
}


void CLanClient::checkCompleteSend(void)
{
	CMessage** pSendCompletionArray = mSession.mSendCompletionArray;

	INT messageCount = mSession.mMessageCount;

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


DWORD CLanClient::ExecuteWorkerThread(LPVOID lpParam)
{
	srand((unsigned)time(NULL));

	CLanClient* pThis = (CLanClient*)lpParam;

	pThis->WorkerThread();

	return 1;
}

void CLanClient::WorkerThread(void)
{
	waitClientStartEvent();

	for (;;)
	{
		DWORD transferred = 0;

		stSession* pSession = nullptr;

		LPOVERLAPPED pOverlapped = nullptr;

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (pOverlapped == nullptr)
		{
			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[WorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

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
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanClient", L"[WorkerThread] pOverlapped Exception Error Code : %d\n", WSAGetLastError());

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



