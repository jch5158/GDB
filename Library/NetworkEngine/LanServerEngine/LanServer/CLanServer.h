#pragma once

#define dfSESSION_ID(index,userID) ((index << 48) | userID)

#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 


namespace lanserver
{
	constexpr INT MAX_WSABUF_SIZE = 200;
};
class  CLanServer
{
private:

	struct stSessionState
	{
		DWORD referenceCount;

		// 사용 == 0, 미사용 == 1
		DWORD bUseFlag;
	};

	struct stSession
	{
		stSession(void)	
			: mbDisconnectFlag(FALSE)
			, mSessionID(0)
			, mIndex(0)
			, mSocket(INVALID_SOCKET)
			, mClientAddr{ 0, }
			, mbSendFlag(FALSE)
			, mMessageCount(0)
			, mSessionState{ 0,1 }
			, mSendCompletionArray{ 0, }
			, mSendQueue()
			, mRecvRingBuffer()
			, mSendOverlapped{ 0, }
			, mRecvOverlapped{ 0, }
		{
			// 고정된 Session Index 번호
			static UINT64 sessionIndex = 0;

			mIndex = sessionIndex;

			++sessionIndex;
		}

		~stSession(void)
		{

		}

		BOOL mbDisconnectFlag;
		UINT64 mSessionID;
		UINT64 mIndex;
		SOCKET mSocket;		
		SOCKADDR_IN mClientAddr;

		OVERLAPPED mSendOverlapped;
		OVERLAPPED mRecvOverlapped;
		
		INT mMessageCount;

		__declspec(align(64)) 
		BOOL mbSendFlag;
		
		__declspec(align(64)) 
		stSessionState mSessionState;
			
		CRingBuffer mRecvRingBuffer;		
		CLockFreeQueue<CMessage*> mSendQueue;
		CMessage* mSendCompletionArray[lanserver::MAX_WSABUF_SIZE];
	};

public:
	
	CLanServer(void);

	virtual ~CLanServer(void);

	virtual BOOL OnStart(void) = 0;
	
	virtual void OnStartWorkerThread(void) = 0;

	virtual void OnStartAcceptThread(void) = 0;

	virtual void OnClientJoin(UINT64 sessionID) = 0;

	virtual void OnClientLeave(UINT64 sessionID) = 0;
	
	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) = 0;

	virtual void OnCloseWorkerThread(void) = 0;

	virtual void OnCloseAcceptThread(void) = 0;

	// accept 직후 바로 호출
	virtual BOOL OnConnectionRequest(const WCHAR* pUserIP, WORD userPort) = 0;

	virtual void OnError(INT errorCode, const WCHAR* pErrorMessage) = 0;

	virtual void OnStop(void) = 0;

	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	BOOL Start(const WCHAR* serverIP, WORD serverPort,BOOL bNagleFlag ,DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount);

	BOOL Stop(void);

	BOOL SendPacket(UINT64 sessionID, CMessage* pMessage);

	BOOL Disconnect(UINT64 sessionID);

	void InitializeTPS(void);

	INT GetCurrentClientCount(void) const;

	INT GetMaxClientCount(void) const;

	INT GetWorkerThreadCount(void) const;

	INT GetRunningThreadCount(void) const;

	BOOL GetNagleFlag(void) const;

	UINT64 GetAcceptTotal(void) const;

	WCHAR* GetServerBindIP(void) const;

	WORD GetServerBindPort(void) const;

protected:

	UINT64 mAcceptTotal;

	__declspec(align(64))
	DWORD mAcceptTPS;

	__declspec(align(64))
	DWORD mSendTPS;

	__declspec(align(64))
	DWORD mRecvTPS;


private:
	
	BOOL initializeSession(SOCKET clientSocket, SOCKADDR_IN clientAddr);

	stSession* findSession(UINT64 sessionID);

	stSession* acquireSession(UINT64 sessionID);

	void associateSocketWithIOCP(stSession* pSession);

	void leaveSession(stSession* pSession);

	BOOL setupNetwork(void);

	BOOL setupSocket(void);

	BOOL bindSocket(void);

	BOOL listenSocket(void);

	BOOL setupIOCP(void);

	BOOL setupSessionArray(void);

	void setupAcceptThread(void);

	void setupWorkerThread(void);

	void setupThread(void);

	void waitServerStartEvent(void);

	BOOL closeAcceptThread(void);

	void cleanupWorkerThread(void);

	void cleanupReleaseStack(void);

	void cleanupSessionArray(void);

	void cleanupSendQueue(CLockFreeQueue<CMessage*> *pSendQueue);

	void cleanupSendCompletionArray(stSession* pSession);

	void closeIOCP(void);

	void releaseSession(stSession* pSession);

	void recvPost(stSession* pSession);

	void sendPost(stSession* pSession);

	void kickSessions(void);

	void checkCompleteRecv(stSession* pSession, DWORD transferred);

	void checkCompleteSend(stSession* pSession, DWORD transferred);

	static DWORD WINAPI ExecuteWorkerThread(LPVOID lpParam);

	void WorkerThread(void);

	static DWORD WINAPI ExecuteAcceptThread(LPVOID lpParam);

	void AcceptThread(void);




	// ======= 자주 쓰면서 자주 바뀌는 멤버변수 ================	

	// 현재 접속자 수
	DWORD mCurrentClientCount;

	// 사용가능한 Session index 보관하는 스택, __declspec(align(64))
	CLockFreeStack<UINT64> mReleaseSessionStack;


	// ======= 서버 실행 후 잘 바뀌지 않는 멤버변수 ============

	HANDLE mServerStartEvent;

	// IOCP 핸들
	HANDLE mIocpHandle;

	// 리슨 소켓
	SOCKET mListenSocket;

	// Session 배열
	stSession* mSessionArray;

	// 스레드 핸들 배열
	HANDLE* mpWorkerThreadHandle;

	// 스레드 ID 배열
	UINT* mpWorkerThreadID;
	
	HANDLE mAcceptThreadHandle;


	// 모든 Session을 대상으로 Disconnect() 호출 후 모두 release 되었다면 
	// 시그널로 변경
	HANDLE mSessionClearEvent;

	// 서버 IP
	WCHAR* mServerIP;

	// 서버 포트
	DWORD mServerPort;

	// stop() 호출할 때 session을 정리하기 위해 사용하는 플래그
	BOOL mbSessionClearFlag;

	BOOL mbNagleFlag;
	
	// 최대 접속자 수
	DWORD mMaxClientCount;

	// 워커 쓰레드 개수
	DWORD mWorkerThreadCount;

	// IOCP 러닝 쓰레드 개수
	DWORD mRunningThreadCount;
		
	UINT mAcceptThreadID;

};

