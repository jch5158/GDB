#pragma once

// NetServer EX

// 상위 2BYTE는 index 하위 6BYTE는 userID로 사용한다.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) 연산을 통해서 상위 2BYTE의 index를 구해낸다.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 


namespace netserver
{

	// wsabuf 길이는 최대 200으로 설정한다.
	constexpr int MAX_WSABUF_SIZE = 200;
}




class CSession;

enum class eOverappedType
{
	Accept,
	Send,
	Recv
};

class COverappedEx :public OVERLAPPED
{
public:

	COverappedEx(void)
		:mOverappedType((eOverappedType)-1)
		,mpSession(nullptr)
	{
	}

	~COverappedEx(void)
	{
	}

	eOverappedType mOverappedType;
	CSession* mpSession;
};

class CSession
{
private:

	friend class CNetServer;

	struct stSessionState
	{
		short referenceCount;

		// 사용 == 0, 미사용 == 1
		short bUseFlag;
	};

	CSession(void)
		: mbDisconnectFlag(false)
		, mSessionID(0)
		, mIndex(0)
		, mSocket(socket(AF_INET, SOCK_STREAM, 0))
		, mClientAddr{ 0, }
		, mbSendFlag(false)
		, mMessageCount(0)
		, mSessionState{ 0,1 }
		, mSendCompletionArray{ 0, }
		, mSendQueue()
		, mRecvRingBuffer()
		, mAcceptOverlapped()
		, mSendOverlapped()
		, mRecvOverlapped()
	{
		// 고정된 Session Index 번호
		static UINT64 sessionIndex = 0;

		ZeroMemory(&mAcceptOverlapped, sizeof(COverappedEx));

		mAcceptOverlapped.mOverappedType = eOverappedType::Accept;

		mAcceptOverlapped.mpSession = this;

		mRecvOverlapped.mOverappedType = eOverappedType::Recv;

		mRecvOverlapped.mpSession = this;

		mSendOverlapped.mOverappedType = eOverappedType::Send;

		mSendOverlapped.mpSession = this;

		mIndex = sessionIndex;

		++sessionIndex;
	}

	~CSession(void)
	{

	}

	bool mbDisconnectFlag;		
	UINT64 mSessionID;			
	UINT64 mIndex;				
	SOCKET mSocket;				
	SOCKADDR_IN mClientAddr;	

	char mBuffer[BUFSIZ];

	COverappedEx mAcceptOverlapped;
	COverappedEx mSendOverlapped;
	COverappedEx mRecvOverlapped;

	// Interlocked 사용하는 변수들은 같은 캐시라인에 있지 않도록 한다.
	__declspec(align(64)) bool mbSendFlag;						
	__declspec(align(64)) stSessionState mSessionState;			

	int mMessageCount;											

	CRingBuffer mRecvRingBuffer;								
	CLockFreeQueue<CMessage*> mSendQueue;                       
	CMessage* mSendCompletionArray[netserver::MAX_WSABUF_SIZE];			
};


class CNetServer
{
public:
	
	CNetServer(void);

	virtual ~CNetServer(void);
	
	virtual BOOL OnStart(void) = 0;

	virtual void OnClientJoin(UINT64 sessionID) = 0;

	virtual void OnClientLeave(UINT64 sessionID) = 0;
	
	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) = 0;
	
	// accept 직후 바로 호출
	virtual bool OnConnectionRequest(const WCHAR* userIp, WORD userPort) = 0;

	virtual void OnError(int errorCode, const WCHAR* errorMessage) = 0;

	virtual void OnStop(void) = 0;


	virtual void OnStartWorkerThread(void) = 0;

	virtual void OnCloseWorkerThread(void) = 0;




	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	bool Start(const WCHAR* serverIp, unsigned short serverPort, int maxMessageSize, bool bNagleFlag, BYTE headerCode, BYTE staticKey, int runningThreadCount, int workerThreadCount,int maxClient);

	bool Stop(void);
	
	bool SendPacket(UINT64 sessionID, CMessage* pMessage);

	bool Disconnect(UINT64 sessionID);

	bool GetNagleFlag(void);

	long GetCurrentClientCount(void);

	long GetMaxClientCount(void);

	long GetRunningThreadCount(void);

	long GetWorkerThreadCount(void);

	UINT64 GetAcceptTotal(void) const;

	DWORD GetAcceptTPS(void) const;

	DWORD GetRecvTPS(void) const;

	DWORD GetSendTPS(void) const;


	void InitializeTPS(void);


	DWORD GetWakeupProcessTime(void);

	DWORD GetWakeupWaitTime(void);

	DWORD GetMaxWakeupProcessTime(void) const;

	LONG GetWakeupPerSecond(void);

	LONG GetMaxWakeupPerSecond(void) const;

	LONG GetWakeupOnRecvPerSecond(void);

	LONG GetMaxWakeupOnRecvPerSecond(void) const;


private:

	void createSession(CSession* pSession);

	CSession* findSession(UINT64 sessionID);

	CSession* acquireSession(UINT64 sessionID);

	void leaveSession(CSession* pSession);

	bool setupNetwork(void);

	bool setupWsa(void);

	bool setupSocket(void);

	bool setupClientSocket(SOCKET clientSocket);

	bool bindSocket(void);

	bool listenSocket(void);

	bool setupIOCP(void);

	bool setupSessionArray(void);


	void setupWorkerThread(void);

	void setupThread(void);

	void cleanupWorkerThread(void);

	void acceptPost(CSession* pSession);

	void releaseSocket(CSession* pSession);

	void* GetSockExtAPI(SOCKET sock, GUID guid);

	void releaseSession(CSession* pSession);

	void recvPost(CSession* pSession);

	void sendPost(CSession* pSession);

	void cleanupSessionArray(void);

	void completeAccept(CSession* pSession);

	void checkCompleteRecv(CSession* pSession, DWORD transferred);

	void checkCompleteSend(CSession* pSession, DWORD transferred);

	static unsigned __stdcall ExecuteWorkerThread(LPVOID lpParam);

	void WorkerThread(void);

	__declspec(align(64)) 
	UINT64 mAcceptTotal;

	__declspec(align(64)) 
	DWORD mAcceptTPS;

	__declspec(align(64))
	DWORD mSendTPS;

	__declspec(align(64))
	DWORD mRecvTPS;



	// IOCP WorkerThread가 초당 일어나는 횟수	
	__declspec(align(64))
	LONG mWakeupPerSecond;

	// IOCP WorkerThread가 초당 OnRecv 를 호출하는 횟수
	__declspec(align(64))
	LONG mWakeupOnRecvPerSecond;

	LONG mWorkerThreadIndex;

	DWORD mWakeupProcessTime[15];

	DWORD mWakeupWaitTime[15];

	LONG mMaxWakeupPerSecond;

	LONG mMaxWakeupOnRecvPerSecond;

	DWORD mMaxWakeupProcessTime;





	// 현재 접속자 수
	long mCurrentClientCount;
	
	int mMaxMessageSize;

	// IOCP 핸들
	HANDLE mIocpHandle;

	// 리슨 소켓
	SOCKET mListenSocket;

	// Session 배열
	CSession* mSessionArray;
	

	// stop() 호출할 때 session을 정리하기 위해 사용하는 플래그
	bool mbSessionClearFlag;

	// 네이글 알고리즘 on/off 플래그
	bool mbNagleFlag;

	// 서버 포트
	unsigned short mServerPort;

	// 최대 접속자 수
	long mMaxClientCount;


	// 워커 스레드 개수
	long mWorkerThreadCount;

	// IOCP 러닝 스레드 개수
	long mRunningThreadCount;


	// 모든 Session을 대상으로 Disconnect() 호출 후 모두 release 되었다면 
	// 시그널로 변경
	HANDLE mSessionClearEvent;
	

	// 워커 스레드 핸들 배열
	HANDLE* mpWorkerThreadHandle;

	// 워커 스레드 ID 배열
	unsigned int* mpWorkerThreadID;

	// 서버 IP
	WCHAR mServerIP[MAX_PATH];

};

