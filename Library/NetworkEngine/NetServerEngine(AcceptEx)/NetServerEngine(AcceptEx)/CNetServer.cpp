#include "stdafx.h"
#include "CNetServer.h"



CNetServer::CNetServer(void)
	: mAcceptTotal(0)
	, mAcceptTPS(0)
	, mSendTPS(0)
	, mRecvTPS(0)
	, mWakeupPerSecond(0)
	, mWakeupOnRecvPerSecond(0)
	, mWorkerThreadIndex(-1)
	, mWakeupProcessTime{ 0, }
	, mWakeupWaitTime{ 0, }
	, mMaxWakeupPerSecond(0)
	, mMaxWakeupOnRecvPerSecond(0)
	, mMaxWakeupProcessTime(0)
	, mIocpHandle(NULL)
	, mListenSocket(INVALID_SOCKET)
	, mServerIP{0,}
	, mServerPort(0)
	, mMaxClientCount(0)
	, mWorkerThreadCount(0)
	, mRunningThreadCount(0)
	, mpWorkerThreadHandle(nullptr)
	, mpWorkerThreadID(nullptr)
	, mbSessionClearFlag(false)
	, mbNagleFlag(true)
	, mSessionClearEvent(CreateEvent(NULL, false, false, NULL))
	, mSessionArray(nullptr)
	, mMaxMessageSize(0)
	, mCurrentClientCount(0)
{

}

CNetServer::~CNetServer(void)
{

}



//const WCHAR* pServerIP, DWORD serverPort, DWORD maxMessageSize,BOOL bNagleFlag, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount
bool CNetServer::Start(const WCHAR* serverIp, unsigned short serverPort, int maxMessageSize, bool bNagleFlag, BYTE headerCode, BYTE staticKey, int runningThreadCount, int workerThreadCount, int maxClientCount)
{	
	if (serverPort > USHRT_MAX)
	{
		return false;
	}

	
	if (runningThreadCount < 1)
	{
		return false;
	}

	
	if (workerThreadCount < 1)
	{
		return false;
	}


	if (maxClientCount < 1)
	{
		return false;
	}

	wcscpy_s(mServerIP, serverIp);

	mServerPort = serverPort;

	mRunningThreadCount = runningThreadCount;

	mWorkerThreadCount = workerThreadCount;

	mMaxClientCount = maxClientCount;

	CMessage::mHeaderCode = headerCode;
	
	CMessage::mStaticKey = staticKey;

	mMaxMessageSize = maxMessageSize;

	mbNagleFlag = bNagleFlag;


	if (setupNetwork() == false)
	{
		return false;
	}

	if (OnStart() == false)
	{
		return false;
	}

	return true;
}

bool CNetServer::Stop(void)
{
	if (mListenSocket == INVALID_SOCKET)
	{
		return false;
	}

	closesocket(mListenSocket);

	mListenSocket = INVALID_SOCKET;

	OnStop();

	cleanupSessionArray();

	cleanupWorkerThread();

	WSACleanup();

	return true;
}


void CNetServer::cleanupWorkerThread(void)
{
	for (int index = 0; index < mWorkerThreadCount; ++index)
	{
		PostQueuedCompletionStatus(mIocpHandle, NULL, NULL, nullptr);
	}

	if (WAIT_FAILED == WaitForMultipleObjects(mWorkerThreadCount, mpWorkerThreadHandle, true, INFINITE))
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[cleanupWorkerThread] WaitForMultipleObjects Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	for (int index = 0; index < mWorkerThreadCount; ++index)
	{
		CloseHandle(mpWorkerThreadHandle[index]);
	}

	CloseHandle(mIocpHandle);

	delete[] mpWorkerThreadHandle;

	delete[] mpWorkerThreadID;

	return;
}



bool CNetServer::GetNagleFlag(void)
{
	return mbNagleFlag;
}

long CNetServer::GetCurrentClientCount(void)
{
	return mCurrentClientCount;
}

long CNetServer::GetMaxClientCount(void)
{
	return mMaxClientCount;
}

long CNetServer::GetRunningThreadCount(void)
{
	return mRunningThreadCount;
}

long CNetServer::GetWorkerThreadCount(void)
{
	return mWorkerThreadCount;
}


UINT64 CNetServer::GetAcceptTotal(void) const
{
	return mAcceptTotal;
}

DWORD CNetServer::GetAcceptTPS(void) const
{
	return mAcceptTPS;
}

DWORD CNetServer::GetRecvTPS(void) const
{
	return mRecvTPS;
}

DWORD CNetServer::GetSendTPS(void) const
{
	return mSendTPS;
}



void CNetServer::InitializeTPS(void)
{
	InterlockedExchange(&mAcceptTPS, 0);

	InterlockedExchange(&mRecvTPS, 0);
	
	InterlockedExchange(&mSendTPS, 0);

	return;
}




DWORD CNetServer::GetWakeupProcessTime(void)
{
	DWORD maxProcessTime = 0;

	for (LONG count = 0; count <= mWorkerThreadIndex; ++count)
	{
		DWORD processTime = mWakeupProcessTime[count];
		mWakeupProcessTime[count] = 0;

		if (maxProcessTime < processTime)
		{
			maxProcessTime = processTime;
		}
	}

	return maxProcessTime;
}

DWORD CNetServer::GetWakeupWaitTime(void)
{
	DWORD maxWaitTime = 0;

	for (LONG count = 0; count <= mWorkerThreadIndex; ++count)
	{
		DWORD waitTime = mWakeupWaitTime[count];
		mWakeupWaitTime[count] = 0;

		if (maxWaitTime < waitTime)
		{
			maxWaitTime = waitTime;
		}
	}

	return maxWaitTime;
}


DWORD CNetServer::GetMaxWakeupProcessTime(void) const
{
	return mMaxWakeupProcessTime;
}

LONG CNetServer::GetWakeupPerSecond(void)
{
	LONG wakeupPerSecond = mWakeupPerSecond;

	if (mMaxWakeupPerSecond < wakeupPerSecond)
	{
		mMaxWakeupPerSecond = wakeupPerSecond;
	}

	InterlockedExchange(&mWakeupPerSecond, 0);

	return wakeupPerSecond;
}

LONG CNetServer::GetMaxWakeupPerSecond(void) const
{
	return mMaxWakeupPerSecond;
}


LONG CNetServer::GetWakeupOnRecvPerSecond(void)
{
	LONG wakeupOnRecvPerSecond = mWakeupOnRecvPerSecond;
	if (mMaxWakeupOnRecvPerSecond < wakeupOnRecvPerSecond)
	{
		mMaxWakeupOnRecvPerSecond = wakeupOnRecvPerSecond;
	}

	InterlockedExchange(&mWakeupOnRecvPerSecond, 0);

	return wakeupOnRecvPerSecond;
}

LONG CNetServer::GetMaxWakeupOnRecvPerSecond(void) const
{
	return mMaxWakeupOnRecvPerSecond;
}



void CNetServer::createSession(CSession* pSession)
{
	static UINT64 userID = 0;

	InterlockedIncrement(&mCurrentClientCount);

	InterlockedIncrement((long*)&pSession->mSessionState);

	pSession->mSessionID = dfSESSION_ID(pSession->mIndex, InterlockedIncrement64((LONG64*)&userID));
	//pSession->mClientAddr = clientAddr;

	PSOCKADDR pRemoteSockAddr = nullptr;
	PSOCKADDR pLocalSockAddr = nullptr;
	INT remoteSockAddrLen = 0;
	INT localSockAddrLen = 0;
	GetAcceptExSockaddrs(&pSession->mBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &pLocalSockAddr, &localSockAddrLen, &pRemoteSockAddr, &remoteSockAddrLen);

	memcpy_s(&pSession->mClientAddr, sizeof(pSession->mClientAddr), pRemoteSockAddr, remoteSockAddrLen);

	pSession->mSessionState.bUseFlag = 0;
	pSession->mbSendFlag = true;
	pSession->mbDisconnectFlag = false;

	return;
}



CSession* CNetServer::findSession(UINT64 sessionID)
{
	UINT64 index = dfSESSION_INDEX(sessionID);

	// mSessionArray 를 초과하는 index가 나올경우 return nullptr
	if (index < 0 || index >= mMaxClientCount)
	{
		return nullptr;
	}

	return &mSessionArray[index];
}


CSession* CNetServer::acquireSession(UINT64 sessionID)
{
	CSession* pSession = findSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	// session에 대한 referenceCount를 증가시킨다. 
	InterlockedIncrement((long*)&pSession->mSessionState);

	// 만약 SessionID 를 releaseSession에서 0으로 바꾸지 않는다면은 useFlag를 먼저 비교해야한다.
	if ( pSession->mSessionState.bUseFlag == 1 || pSession->mSessionID != sessionID)
	{		
		leaveSession(pSession);

		return nullptr;
	}

	return pSession;
}



void CNetServer::leaveSession(CSession* pSession)
{
	if (InterlockedDecrement((long*)&pSession->mSessionState) == 0)
	{
		releaseSession(pSession);
	}

	return;
}

bool CNetServer::SendPacket(UINT64 sessionID, CMessage* pMessage)
{
	CSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
	{
		return false;
	}

	pMessage->encode();

	pMessage->AddReferenceCount();

	pSession->mSendQueue.Enqueue(pMessage);

	sendPost(pSession);

	InterlockedIncrement(&mSendTPS);

	leaveSession(pSession);

	return true;
}

bool CNetServer::Disconnect(UINT64 sessionID)
{
	CSession* pSession = acquireSession(sessionID);
	if (pSession == nullptr)
	{
		return false;
	}

	if (pSession->mbDisconnectFlag == false)
	{
		pSession->mbDisconnectFlag = true;

		CancelIoEx((HANDLE)pSession->mSocket, nullptr);
	}

	leaveSession(pSession);

	return true;
}


void CNetServer::releaseSocket(CSession* pSession)
{
	LPFN_DISCONNECTEX pFnDisconnect = (LPFN_DISCONNECTEX)GetSockExtAPI(pSession->mSocket, WSAID_DISCONNECTEX);
	if (pFnDisconnect(pSession->mSocket, NULL, TF_REUSE_SOCKET, 0) == false)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[releaseSocket] Error Code : %d\n", WSAGetLastError());

		CCrashDump::Crash();
	}

	acceptPost(pSession);
}

void CNetServer::releaseSession(CSession* pSession)
{	
	CSession::stSessionState releaseSessionState = { 0,1 };

	if ((long)0 != InterlockedCompareExchange((long*)&pSession->mSessionState, *(long*)&releaseSessionState, (long)0))
	{
		return;
	}

	OnClientLeave(pSession->mSessionID);

	// AcceptEx 에서는 하면 안 됨
	//closesocket(pSession->mSocket);

	releaseSocket(pSession);

	CLockFreeQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

	// SendQueue 에 남아있는 메시지들의 referenceCount를 차감시킨다.
	CMessage* pMessage;
	while (pSendQueue->Dequeue(&pMessage) == true)
	{
		pMessage->Free();
	}

	int messageCount = pSession->mMessageCount;

	pSession->mMessageCount = 0;

	// 완료통지를 받아야할 SendQueue에 남아있는 메시지들을 정리한다.
	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	for (int index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	// recv 버퍼에 남아있는 데이터를 정리한다.
	pSession->mRecvRingBuffer.ClearBuffer();

	InterlockedDecrement(&mCurrentClientCount);

	// 전체 Session 끊기 위한 플래그
	if (mbSessionClearFlag == true)
	{
		if (mCurrentClientCount == 0)
		{
			SetEvent(mSessionClearEvent);
		}
	}

	return;
}

void CNetServer::recvPost(CSession* pSession)
{
	INT wsabufSize = 1;

	WSABUF wsabufRecv[2];

	CRingBuffer* pRingBuffer = &pSession->mRecvRingBuffer;

	INT freeSize = pRingBuffer->GetFreeSize();
	if (freeSize == 0)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[recvPost] mRecvRingBuffer is Full");

		Disconnect(pSession->mSessionID);

		return;
	}

	INT directEnqueueSize = pRingBuffer->GetDirectEnqueueSize();

	wsabufRecv[0].buf = pRingBuffer->GetRearBufferPtr();
	wsabufRecv[0].len = directEnqueueSize;

	INT remainSize = freeSize - directEnqueueSize;
	if (remainSize != 0)
	{
		wsabufSize += 1;

		wsabufRecv[1].buf = pRingBuffer->GetBufferPtr();
		wsabufRecv[1].len = remainSize;
	}

	DWORD flags = 0;

	InterlockedIncrement((long*)&pSession->mSessionState);

	ZeroMemory(&pSession->mRecvOverlapped, sizeof(OVERLAPPED));

	if (WSARecv(pSession->mSocket, wsabufRecv, wsabufSize, nullptr, &flags, &pSession->mRecvOverlapped, nullptr) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (InterlockedDecrement((long*)&pSession->mSessionState) == 0)
			{
				releaseSession(pSession);
			}
		}
	}

	if (pSession->mbDisconnectFlag == true)
	{
		CancelIoEx((HANDLE)pSession->mSocket, &pSession->mRecvOverlapped);
	}


	return;
}

void CNetServer::sendPost(CSession* pSession)
{
	CLockFreeQueue<CMessage*>* pSendQueue = &pSession->mSendQueue;

SEND_START:

	int useSize = pSendQueue->GetUseSize();
	if (useSize <= 0)
	{
		return;
	}

	if (false == (bool)InterlockedExchange8((char*)&pSession->mbSendFlag, false))
	{
		return;
	}

	WSABUF wsabufSend[netserver::MAX_WSABUF_SIZE];
	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	int index;
	for (index = 0; index < useSize; ++index)
	{
		if (pSendQueue->Dequeue(&pSendCompletionArray[index]) == true)
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
		InterlockedExchange8((char*)&pSession->mbSendFlag, true);

		goto SEND_START;
	}

	pSession->mMessageCount = index;

	InterlockedIncrement((long*)&pSession->mSessionState);

	ZeroMemory(&pSession->mSendOverlapped, sizeof(OVERLAPPED));

	if (WSASend(pSession->mSocket, wsabufSend, index, nullptr, 0, &pSession->mSendOverlapped, nullptr) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (InterlockedDecrement((long*)&pSession->mSessionState) == 0)
			{
				releaseSession(pSession);
			}
		}
	}

	return;
}

void CNetServer::cleanupSessionArray(void)
{
	mbSessionClearFlag = true;

	for (int index = 0; index < mMaxClientCount; ++index)
	{
		if (mSessionArray[index].mSessionState.bUseFlag == 0)
		{
			Disconnect(mSessionArray[index].mSessionID);
		}
	}


	if (WAIT_OBJECT_0 != WaitForSingleObject(mSessionClearEvent, INFINITE))
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[cleanUpSessionArray] Error Code : %d", WSAGetLastError());

		CCrashDump::Crash();
	}

	delete[] mSessionArray;
}

bool CNetServer::setupWsa(void)
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[WSAStartup] Error Code : %d", WSAGetLastError());

		return false;
	}

	return true;
}


bool CNetServer::setupSocket(void)
{
	int retval = 0;

	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] socket Error Code : %d", WSAGetLastError());

		WSACleanup();
		return false;
	}

	//KeepAlive 옵션
	int enable = 1;
	retval = setsockopt(mListenSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		WSACleanup();
	
		return false;
	}


	if (mbNagleFlag == false)
	{
		int optval = true;
		retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] Nagle No Delay setsockopt Error Code : %d", WSAGetLastError());

			WSACleanup();

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

		WSACleanup();
		return false;
	}

	return true;
}


bool CNetServer::setupClientSocket(SOCKET clientSocket)
{
	//KeepAlive 옵션
	int enable = 1;
	int retval = setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] KeepAlive setsockopt Error Code : %d", WSAGetLastError());

		WSACleanup();

		return false;
	}

	if (mbNagleFlag == false)
	{
		int optval = true;
		retval = setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] Nagle No Delay setsockopt Error Code : %d", WSAGetLastError());

			WSACleanup();

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

		WSACleanup();
		return false;
	}

	return true;
}


bool CNetServer::bindSocket(void)
{
	int retval = 0;

	SOCKADDR_IN serverAddr = { 0, };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mServerPort);
	InetPtonW(AF_INET, mServerIP, &serverAddr.sin_addr);

	retval = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupSocket] bindSocket Error Code : %d", WSAGetLastError());

		WSACleanup();
		return false;
	}


	return true;
}


bool CNetServer::listenSocket(void)
{
	int	retval = listen(mListenSocket, SOMAXCONN_HINT(65535));
	if (retval == SOCKET_ERROR)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[listenSocket] listen Error Code : %d", WSAGetLastError());

		WSACleanup();
		return false;
	}

	return true;
}


bool CNetServer::setupIOCP(void)
{
	mIocpHandle = CreateIoCompletionPort((HANDLE)mListenSocket, NULL, NULL, mRunningThreadCount);
	if (mIocpHandle == NULL)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[setupIOCP] CreateIoCompletionPort Error Code : %d", WSAGetLastError());

		WSACleanup();

		return false;
	}

	return true;
}


// 전체 Session 셋팅
bool CNetServer::setupSessionArray(void)
{
	if (mMaxClientCount <= 0)
	{
		WSACleanup();
		return false;
	}

	mSessionArray = new CSession[mMaxClientCount];

	for (int index = mMaxClientCount - 1; 0 <= index; --index)
	{
		setupClientSocket(mSessionArray[index].mSocket);

		if (CreateIoCompletionPort((HANDLE)mSessionArray[index].mSocket, mIocpHandle, (ULONG_PTR)&mSessionArray[index], NULL) == NULL)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] CreateSession Error CreateIoCompletionPort Error Code : % d", WSAGetLastError());
			CCrashDump::Crash();
		}

		acceptPost(&mSessionArray[index]);
	}

	return true;
}

void CNetServer::setupWorkerThread(void)
{
	for (int index = 0; index < mWorkerThreadCount; ++index)
	{
		mpWorkerThreadHandle[index] = (HANDLE)_beginthreadex(NULL, NULL, ExecuteWorkerThread, this, NULL, &mpWorkerThreadID[index]);
	}

	return;
}





void CNetServer::setupThread(void)
{	
	mpWorkerThreadHandle = new HANDLE[mWorkerThreadCount];

	mpWorkerThreadID = new unsigned int[mWorkerThreadCount];

	setupWorkerThread();

	return;
}


bool CNetServer::setupNetwork(void)
{
	if (setupWsa() == false)
	{
		return false;
	}

	if (setupSocket() == false)
	{
		return false;
	}

	if (bindSocket() == false)
	{
		return false;
	}

	if (listenSocket() == false)
	{
		return false;
	}

	if (setupIOCP() == false)
	{
		return false;
	}

	if (setupSessionArray() == false)
	{
		return false;
	}

	setupThread();	

	return true;
}

void* CNetServer::GetSockExtAPI(SOCKET sock, GUID guid)
{
	PVOID pfnEx = nullptr;
	DWORD dwBytes = 0;

	LONG lret = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &pfnEx, sizeof(pfnEx), &dwBytes, NULL, NULL);
	if (lret == SOCKET_ERROR)
	{
		return NULL;
	}

	return pfnEx;
}


void CNetServer::acceptPost(CSession* pSession)
{
	// AcceptEx 함수포인터 가져오기
	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(mListenSocket, WSAID_ACCEPTEX);

	ZeroMemory(&pSession->mAcceptOverlapped, sizeof(OVERLAPPED));

	DWORD dwBytes;

	if (pfnAcceptEx(mListenSocket, pSession->mSocket, pSession->mBuffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, (LPOVERLAPPED)&pSession->mAcceptOverlapped) == false)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[acceptPost] Error Code : %d",WSAGetLastError());

			CCrashDump::Crash();
		
			return;
		}
	}

	return;
}



// 4번째 연결과 동시에 데이터를 받을지 말지 0은 안 받음
// 5번째 로컬 주소의 길이
// 6번째 클라이언트 주소의 길이
// 7번째 0
// 8번째 등록할 Overppaed

void CNetServer::checkCompleteRecv(CSession* pSession, DWORD transferred)
{
	CRingBuffer* pRecvRingBuffer = &pSession->mRecvRingBuffer;

	pRecvRingBuffer->MoveRear(transferred);

	for (;;)
	{
		int recvSize = pRecvRingBuffer->GetUseSize();

		if (recvSize < sizeof(CMessage::stNetHeader))
		{
			break;
		}

		CMessage::stNetHeader header;
		if (pRecvRingBuffer->Peek((char*)&header, sizeof(header)) != sizeof(header))
		{
			// 내 링버퍼의 문제가 있을 경우이다.	
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		if (header.code != CMessage::mHeaderCode)
		{
			Disconnect(pSession->mSessionID);
					
			break;
		}

		if (header.payloadSize > mMaxMessageSize)
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
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] RingBuffer Error Retval");

			CCrashDump::Crash();
		}

		if (pMessage->MoveWritePos(header.payloadSize) != header.payloadSize)
		{
			// 내 CMessage 문제가 있을 경우이다.
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[checkCompleteRecv] Message Error Retval");

			CCrashDump::Crash();
		}	

		if (pMessage->decode(&header) == false)
		{
			Disconnect(pSession->mSessionID);

			pMessage->Free();

			return;
		}

		try
		{
			OnRecv(pSession->mSessionID, pMessage);
		}
		catch (const message::CExceptionObject& exceptionObject)
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


void CNetServer::checkCompleteSend(CSession* pSession, DWORD transferred)
{	
	CMessage** pSendCompletionArray = pSession->mSendCompletionArray;

	int messageCount = pSession->mMessageCount;

	for (int index = 0; index < messageCount; ++index)
	{
		pSendCompletionArray[index]->Free();
	}

	pSession->mMessageCount = 0;

	InterlockedExchange8((char*)&pSession->mbSendFlag, true);

	sendPost(pSession);

	return;
}



unsigned CNetServer::ExecuteWorkerThread(LPVOID lpParam)
{
	srand((unsigned)time(NULL));

	CNetServer* pThis = (CNetServer*)lpParam;

	pThis->WorkerThread();

	return 1;
}

void CNetServer::completeAccept(CSession* pSession)
{
	createSession(pSession);

	WCHAR clientIP[100];
	InetNtopW(AF_INET, &pSession->mClientAddr.sin_addr, clientIP, _countof(clientIP));
	if (OnConnectionRequest(clientIP, ntohs(pSession->mClientAddr.sin_port)) == false)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] Black IP : %s", clientIP);

		releaseSession(pSession);

		return;
	}

	if (mCurrentClientCount >= mMaxClientCount)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateAcceptThread] CurrentClient is full, Max Client : %d, Current Client : %d", mMaxClientCount, mCurrentClientCount);

		releaseSession(pSession);

		return;
	}

	InterlockedIncrement(&mAcceptTPS);

	InterlockedIncrement(&mAcceptTotal);

	OnClientJoin(pSession->mSessionID);

	recvPost(pSession);

	if (InterlockedDecrement((long*)&pSession->mSessionState) == 0)
	{
		releaseSession(pSession);
	}

	return;
}


void CNetServer::WorkerThread(void)
{
	OnStartWorkerThread();

	DWORD processCount = 0;

	DWORD processTotalTime = 0;

	LONG threadIndex = InterlockedIncrement(&mWorkerThreadIndex);

	DWORD initializeTime = timeGetTime();

	for (;;)
	{
		DWORD transferred = 0;

		UINT64* ptr = nullptr;

		COverappedEx* pOverlappedEx = nullptr;

		DWORD waitStart = timeGetTime();

		GetQueuedCompletionStatus(mIocpHandle, &transferred, (PULONG_PTR)&ptr, (LPOVERLAPPED*)&pOverlappedEx, INFINITE);
				
		DWORD startTime = timeGetTime();

		mWakeupWaitTime[threadIndex] = startTime - waitStart;

		if (pOverlappedEx == nullptr)
		{
			CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateWorkerThread] pOverlapped is nullptr, Error Code : %d\n", WSAGetLastError());

			break;
		}

		CSession* pSession = pOverlappedEx->mpSession;

		if (pSession->mbDisconnectFlag == false)
		{
			switch (pOverlappedEx->mOverappedType)
			{
			case eOverappedType::Accept:

				// AcceptEx 로 인한 완료통지 시 transffered 값은 0이다.
				completeAccept(pSession);

				continue;
			case eOverappedType::Send:

				if (transferred != 0)
				{
					checkCompleteSend(pSession, transferred);
				}

				break;
			case eOverappedType::Recv:
				
				if (transferred != 0)
				{
					checkCompleteRecv(pSession, transferred);
				}

				break;
			default:
				
				CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[UpdateWorkerThread] Exception Overapped, transffered : %d, Error Code : %d, Session ID : %llx\n", transferred, WSAGetLastError(), pSession->mSessionID);

				CCrashDump::Crash();

				break;
			}
		}

		if (InterlockedDecrement((long*)&pSession->mSessionState) == 0)
		{
			releaseSession(pSession);
		}	

		InterlockedIncrement(&mWakeupPerSecond);

		DWORD nowTime = timeGetTime();

		processTotalTime = nowTime - startTime;
		++processCount;

		if (nowTime - initializeTime > 1000)
		{
			mWakeupProcessTime[threadIndex] = processTotalTime / processCount;

			if (mMaxWakeupProcessTime < mWakeupProcessTime[threadIndex])
			{
				mMaxWakeupProcessTime = mWakeupProcessTime[threadIndex];
			}

			processTotalTime = 0;
			processCount = 0;
		}
	}

	OnCloseWorkerThread();

	return;
}




