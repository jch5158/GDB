#pragma once


// 상위 2BYTE는 index 하위 6BYTE는 userID로 사용한다.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) 연산을 통해서 상위 2BYTE의 index를 구해낸다.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 

// wsabuf 길이는 최대 200으로 설정한다.

namespace netserver
{
	constexpr int MAX_WSABUF_SIZE = 200;
}

class CNetServer
{
private:

	struct stSessionState
	{
		unsigned int referenceCount;

		// 사용 == 0, 미사용 == 1
		unsigned int bUseFlag;
	};

	class stSession
	{
	public:
		stSession(void);
		~stSession(void) = default;

		stSession(const stSession&) = delete;
		stSession& operator=(const stSession&) = delete;

		// Interlocked 사용하는 변수들은 같은 캐시라인에 있지 않도록 한다.
		__declspec(align(64)) bool             mbSendFlag;
		__declspec(align(64)) stSessionState   mSessionState;

		// 연결 정보
		SOCKET          mSocket;
		SOCKADDR_IN     mClientAddr;

		// 비동기 I/O OVERAPPED
		OVERLAPPED      mRecvOverlapped;
		OVERLAPPED      mSendOverlapped;

		// 세션 상태 정보
		unsigned long long    mSessionID;
		unsigned long long    mIndex;              // 세션 index
		unsigned int          mRecvTime;      	   // timeout을 위한 recvTime 값
		bool                  mbDisconnectFlag;    // disconnect flag

		// 송.수신 컨테이너
		CRingBuffer                 mRecvRingBuffer;
		CLockFreeQueue<CMessage*>   mSendQueue;


		// 송신 중 패킷 반환용 데이터
		int         mMessageCount;
		CMessage*   mSendCompletionArray[netserver::MAX_WSABUF_SIZE];
	};


public:
	
	CNetServer(void);

	virtual ~CNetServer(void);

	CNetServer(const CNetServer&) = delete;
	CNetServer& operator=(const CNetServer&) = delete;

	virtual bool OnStart(void) = 0;	
	virtual void OnStop(void) = 0;
	virtual void OnStartAcceptThread(void) = 0;
	virtual void OnStartWorkerThread(void) = 0;
	virtual void OnCloseAcceptThread(void) = 0;
	virtual void OnCloseWorkerThread(void) = 0;
	virtual bool OnConnectionRequest(const wchar_t* userIP, unsigned short userPort) = 0;   // accept 지후 호출
	virtual void OnClientJoin(unsigned long long sessionID) = 0;
	virtual void OnClientLeave(unsigned long long sessionID) = 0;	
	virtual void OnRecv(unsigned long long sessionID, CMessage* pMessage) = 0;
	virtual void OnError(unsigned int errorCode, const wchar_t* errorMessage) = 0;

	
	bool Start(const wchar_t* pServerIP, unsigned short serverPort, int maxMessageSize, bool bNagleFlag, bool bTimeout,
		unsigned char headerCode, unsigned char staticKey, unsigned int timeoutSec,
		int runningThreadCount, int workerThreadCount, int maxClientCount);
	bool Stop(void);
	

	// 서버 설정 값 확인
	const wchar_t* GetServerBindIP(void) const;
	unsigned short GetServerBindPort(void) const;
	bool GetNagleFlag(void) const;
	int GetMaxClientCount(void) const;
	int GetRunningThreadCount(void) const;
	int GetWorkerThreadCount(void) const;


	// 세션 네트워크 함수
	bool SendPacket(unsigned long long sessionID, CMessage* pMessage);
	bool Disconnect(unsigned long long sessionID);
	bool GetConnectionState(unsigned long long sessionID);



	// 서버 접속자 정보
	unsigned long long GetAcceptTotal(void) const;
	long GetCurrentClientCount(void) const;


	// 클라이언트 정보
	bool GetClientIP(unsigned long long sessionID, wchar_t* pClientIP, int bufferCch);
	bool GetClientPort(unsigned long long sessionID, unsigned short* pClientPort);


	// TPS 정보 가져오기
	long GetAcceptTPS(void) const;
	long GetRecvTPS(void) const;
	long GetSendTPS(void) const;

	
	// TPS 초기화
	void InitializeTPS(void);


	// WorkerThread 성능 정보
	int GetWakeupPerSecond(void);
	int GetMaxWakeupPerSecond(void) const;
	unsigned int GetWakeupProcessTime(void);
	unsigned int GetMaxWakeupProcessTime(void) const;
	unsigned int GetMaxWakeupWaitTime(void);

private:

	void associateSocketWithIOCP(stSession* pSession);      // 소켓과 IOCP 핸들 연결

	bool initializeSession(SOCKET clientSocket, SOCKADDR_IN clientAddr);  // 세션 초기화

	// 세션 획득 관련 함수
	stSession* findSession(unsigned long long sessionID);
	stSession* acquireSession(unsigned long long sessionID);
	void leaveSession(stSession* pSession);

	bool setupNetwork(void);
	bool setupSocket(void);
	bool bindSocket(void);
	bool listenSocket(void);
	bool setupIOCP(void);
	bool setupSessionArray(void);
	void setupThread(void);
	void setupAcceptThread(void);
	void setupWorkerThreads(void);
	void setupTimeoutThread(void);

	void waitServerStartEvent(void);       // 서버 셋팅이 완료되면 return

	// 서버 종료 시 호출하는 함수
	bool closeAcceptThread(void);
	void kickSessions(void);
	void closeTimeoutThread(void);
	void closeWorkerThreads(void);
	void cleanupReleaseStack(void);
	void closeIOCP(void);


	void releaseSession(stSession* pSession);
	void clearSendQueue(CLockFreeQueue<CMessage*> *pSendQueue);
	void clearSendCompletionArray(stSession *pSession);


	// send, recv
	void sendPost(stSession* pSession);
	void recvPost(stSession* pSession);

	void checkCompletionSend(stSession* pSession);
	void checkCompletionRecv(stSession* pSession, unsigned long transferred);

	static unsigned __stdcall ExecuteAcceptThread(void* pParam);
	static unsigned __stdcall ExecuteWorkerThread(void* pParam);
	static unsigned __stdcall ExecuteTimeoutThread(void* pParam);

	void AcceptThread(void);
	void WorkerThread(void);
	void TimeoutThread(void);
	void SendThread(void);

	// 로깅 판단
	bool isLoggingErrorCode(unsigned int errorCode) const;


	// 서버 접속자 데이터
	unsigned long long         mAcceptTotal;
	long                       mCurrentClientCount;


	 
	long                          mWorkerThreadIndex;            // WorkerThread 성능추적 index
	__declspec(align(64)) long    mWakeupPerSecond;              // 모든 IOCP WorkerThread가 초당 일어나는 횟수	
	int                           mMaxWakeupPerSecond;           // IOCP WorkerThread가 초당 가장 많이 일어난 횟수
	unsigned int                  mMaxWakeupProcessTime;         // 한 번 일어나서 완료통지를 가장 오랫동안 처리하는 시간
	unsigned int*                 mpWakeupProcessTimeArray;      // 한 번 일어나서 완료통지를 처리하는 시간 
	unsigned int*                 mpWakeupWaitTimeArray;         // 완료통지를 처리하기 위해 깨어나길 기다리는 시간


	// 서버 TPS
	__declspec(align(64)) long    mAcceptTPS;
	__declspec(align(64)) long    mSendTPS;
	__declspec(align(64)) long    mRecvTPS;


	// 세션 컨테이너
	stSession*             mSessionArray;        // Session 배열
	CLockFreeStack<unsigned long long> mReleaseSessionStack; // 사용가능한 Session index 보관하는 스택, 


	// 서버 셋팅 값
	std::wstring      mServerIP;
	unsigned short    mServerPort;
	int               mMaxClientCount;       // 최대 접속자 수
	int               mMaxMessageSize;       // 메시지 최대 크기
	int               mWorkerThreadCount;    // 워커 스레드 개수
	int               mRunningThreadCount;   // IOCP 러닝 스레드 개수
	unsigned int      mTimeoutSec;      	 // timeout 시간
	bool              mbTimeout;
	bool              mbNagleFlag;           // 네이글 알고리즘 on/off 플래그

	// IOCP
	HANDLE mIocpHandle;
	SOCKET mListenSocket;    // 리슨 소켓


	// 서버 시작 시 시그널
	HANDLE            mServerStartEvent;
	

	// 서버 종료 시 사용
	bool              mbSessionClearFlag;      // stop() 호출할 때 session을 정리하기 위해 사용하는 플래그
	HANDLE            mSessionClearEvent;      // 모든 Session을 대상으로 Disconnect() 호출 후 세션이 모두 release 되었다면 시그널
					  
					  
	// thread Headle  
	HANDLE            mAcceptThreadHandle;
	HANDLE            mTimeoutThreadHandle;
	HANDLE*           mpWorkerThreadHandles;
					  
					  
	// Thread ID	  
	unsigned int      mAcceptThreadID;         
	unsigned int      mTimeoutThreadID;
	unsigned int*     mpWorkerThreadsID;        // 워커 스레드 ID 배열
};

