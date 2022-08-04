#pragma once


// 상위 2BYTE는 index 하위 6BYTE는 userID로 사용한다.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) 연산을 통해서 상위 2BYTE의 index를 구해낸다.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 

// wsabuf 길이는 최대 200으로 설정한다.
constexpr int MAX_WSABUF_SIZE = 200;

extern UINT64 gAcceptTotal;

extern DWORD gUpdateTPS;

extern DWORD gAcceptTPS;

extern DWORD gSendTPS;

extern __declspec(align(64)) DWORD gRecvTPS;

//extern DWORD gSectorPlayer;
//
//extern DWORD gSendPacket;
//
//extern DWORD gSendPacketAround;

class CNetServer
{
public:

	struct stSessionState
	{
		DWORD referenceCount;

		// 사용 == 0, 미사용 == 1
		DWORD bUseFlag;
	};

	class __declspec(align(64)) stSession
	{
	public:
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

		BOOL mbDisconnectFlag;										// 8
		UINT64 mSessionID;											// 8
		UINT64 mIndex;												// 8
		SOCKET mSocket;												// 8
		SOCKADDR_IN mClientAddr;									// 16 

		// 48
		// 16 캐시 버림

		__declspec(align(64)) OVERLAPPED mSendOverlapped;
		OVERLAPPED mRecvOverlapped;

		// Interlocked 사용하는 변수들은 같은 캐시라인에 있지 않도록 한다.
		__declspec(align(64)) BOOL mbSendFlag;						// 4
		__declspec(align(64)) stSessionState mSessionState;			// 8
	
		DWORD mMessageCount;										// 4	

		CRingBuffer mRecvRingBuffer;								// 24
		CLockFreeQueue<CMessage*> mSendQueue;                       // 96
		CMessage *mSendCompletionArray[MAX_WSABUF_SIZE];			// 160
	};

public:
	
	CNetServer(void);

	virtual ~CNetServer(void);
	
	virtual void OnClientJoin(UINT64 sessionID) = 0;

	virtual void OnClientLeave(UINT64 sessionID) = 0;
	
	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) = 0;
	
	// accept 직후 바로 호출
	virtual BOOL OnConnectionRequest(const WCHAR* userIP, WORD userPort) = 0;

	virtual void OnError(DWORD errorCode, const WCHAR* errorMessage) = 0;

	virtual void OnTimeout(UINT64 sessionID) = 0;

	virtual void OnStop(void) = 0;

	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	BOOL Start(const WCHAR* serverIP, DWORD serverPort, BOOL bNagleFlag, BOOL bTimeoutFlag, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount);

	BOOL Stop(void);
	
	inline BOOL SendPacket(UINT64 sessionID, CMessage* pMessage)
	{
		stSession* pSession = acquireSession(sessionID);
		if (pSession == nullptr)
		{
			//CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[SendPacket] Client Not Found, sessionID : %llx", sessionID);

			return FALSE;
		}

		// 헤더 셋팅 그리고 체크섬과 같이 Encoding
		pMessage->encode();

		// SendQ에 Enqueue 하는 순간 다른 스레드에서 CMessage에 대한 참조가 가능해진다. 그러기 때문에 referenceCount를 증가시킨다.
		pMessage->AddReferenceCount();

		pSession->mSendQueue.Enqueue(pMessage);

		sendPost(pSession);

		++gSendTPS;

		leaveSession(pSession);

		return TRUE;
	}

	inline BOOL Disconnect(UINT64 sessionID);

	BOOL GetTimeoutFlag(void);

	BOOL GetNagleFlag(void);

	DWORD GetClientCount(void);

	DWORD GetMaxClientCount(void);

	DWORD GetRunningThreadCount(void);

	DWORD GetWorkerThreadCount(void);
	

private:

	inline stSession* createSession(SOCKET clientSocket, SOCKADDR_IN clientAddr);

	inline stSession* findSession(UINT64 sessionID);

	inline stSession* acquireSession(UINT64 sessionID);

	inline void leaveSession(stSession* pSession);

	BOOL setupNetwork(void);

	BOOL setupSocket(void);

	BOOL bindSocket(void);

	BOOL listenSocket(void);

	BOOL setupIOCP(void);

	BOOL setupSessionArray(void);

	void setupAcceptThread(void);

	void setupWorkerThread(void);

	void setupTimoutThread(void);

	void setupThread(void);

	BOOL closeAcceptThread(void);

	void cleanupWorkerThread(void);


	inline void closeTimeoutThread(void);

	inline void releaseSession(stSession* pSession);

	inline void recvPost(stSession* pSession);

	inline void sendPost(stSession* pSession);

	inline void cleanupSessionArray(void);

	inline void checkCompletionRecv(stSession* pSession, DWORD transferred);

	inline void checkCompletionSend(stSession* pSession);

	static DWORD WINAPI WorkerThread(LPVOID lpParam);

	void executeWorkerThread(void);

	static DWORD WINAPI AcceptThread(LPVOID lpParam);

	void executeAcceptThread(void);

	static DWORD WINAPI TimeoutThread(LPVOID lpParam);

	void executeTimeoutThread(void);	

// ======= 자주 쓰면서 자주 바뀌는 멤버변수 ================	

	// 현재 접속자 수
	DWORD mCurrentClientCount;

	// 사용가능한 Session index 보관하는 스택, __declspec(align(64))
	CLockFreeStack<UINT64> mReleaseSessionStack;


// ======= 서버 실행 후 잘 바뀌지 않는 멤버변수 ============

	// IOCP 핸들
	__declspec (align(64)) HANDLE mIocpHandle;

	// 리슨 소켓
	SOCKET mListenSocket;

	// Session 배열
	stSession* mSessionArray;

	BOOL mbTimeoutFlag;

	// stop() 호출할 때 session을 정리하기 위해 사용하는 플래그
	BOOL mbSessionClearFlag;

	// 네이글 알고리즘 on/off 플래그
	BOOL mbNagleFlag;

	// 서버 포트
	DWORD mServerPort;

	// 최대 접속자 수
	DWORD mMaxClientCount;

	// 워커 스레드 개수
	DWORD mWorkerThreadCount;

	// IOCP 러닝 스레드 개수
	DWORD mRunningThreadCount;

	// Accept 스레드 ID
	DWORD mAcceptThreadID;

	// Timeout 스레드 ID
	DWORD mTimeoutThreadID;

	// 모든 Session을 대상으로 Disconnect() 호출 후 모두 release 되었다면 
	// 시그널로 변경
	HANDLE mSessionClearEvent;
	
	// Accept 스레드
	HANDLE mAcceptThreadHandle;

	// Timeout 스레드 핸들
	HANDLE mTimeoutThreadHandle;

	// 워커 스레드 핸들 배열
	HANDLE* mpWorkerThreadHandle;

	// 워커 스레드 ID 배열
	DWORD* mpWorkerThreadID;

	// 서버 IP
	WCHAR mServerIP[MAX_PATH];

};

