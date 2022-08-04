#pragma once

// 상위 2BYTE는 index 하위 6BYTE는 userID로 사용한다.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) 연산을 통해서 상위 2BYTE의 index를 구해낸다.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 


// wsabuf 길이는 최대 200으로 설정한다.

namespace netclient
{
	constexpr INT MAX_WSABUF_SIZE = 200;
};

class  CNetClient
{
private:

	struct stSessionState
	{
		DWORD referenceCount;

		// 사용 == 0, 미사용 == 1
		DWORD bUseFlag;
	};

	__declspec(align(64)) class CSession
	{
	public:

		CSession(void)
			: mSocket(INVALID_SOCKET)
			, mbDisconnectFlag(FALSE)			
			, mbSendFlag(TRUE)
			, mMessageCount(0)
			, mSessionState{ 0, }
			, mSendCompletionArray{ 0, }
			, mSendQueue()
			, mRecvRingBuffer()
			, mSendOverlapped{ 0, }
			, mRecvOverlapped{ 0, }
		{
		}

		~CSession(void)
		{
		}

		SOCKET mSocket;
		BOOL mbDisconnectFlag;

		__declspec(align(64)) 
		OVERLAPPED mSendOverlapped;
		OVERLAPPED mRecvOverlapped;

		INT mMessageCount;											// 4	

		__declspec(align(64))
		BOOL mbSendFlag;											// 4
		
		__declspec(align(64))
		stSessionState mSessionState;

		CRingBuffer mRecvRingBuffer;								// 24
		CLockFreeQueue<CMessage*> mSendQueue;                       // 96
		CMessage* mSendCompletionArray[netclient::MAX_WSABUF_SIZE];			// 160
	};

public:
	
	CNetClient(void);

	virtual ~CNetClient(void);
	
	virtual void OnServerJoin(void) = 0;

	virtual void OnServerLeave(void) = 0;
	
	virtual void OnRecv(CMessage* pMessage) = 0;
	
	virtual void OnError(DWORD errorCode, const WCHAR* errorMessage) = 0;

	virtual void OnTimeout(void) = 0;

	virtual void OnStop(void) = 0;

	//오픈 IP / 포트 / 워커스레드 수(생성수, 러닝수) / 나글옵션 / 최대접속자 수
	BOOL Start(const WCHAR* serverIP, WORD serverPort, BOOL bNagleFlag, BOOL bTimeoutFlag, DWORD timeoutPeriod, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount);

	BOOL Stop(void);

	inline BOOL SendPacket(CMessage* pMessage);

	inline BOOL Disconnect(void);

	inline BOOL GetTimeoutFlag(void);

	inline DWORD GetRunningThreadCount(void);

	inline DWORD GetWorkerThreadCount(void);

	inline BOOL GetConnectionState(void);

private:
	
	BOOL setupNetwork(void);

	BOOL setupSocket(void);

	void connectServer(void);

	BOOL initializeSession(void);

	void associateSocketWithIOCP(void);
	
	BOOL setupIOCP(void);

	BOOL setupWorkerThreads(void);

	BOOL setupTimoutThread(void);

	void closeWorkerThreads(void);

	void closeIOCP(void);

	void checkSessionConnection(void);

	void closeTimeoutThread(void);

	void releaseSession(void);

	inline void recvPost(void);

	inline void sendPost(void);

	inline BOOL acquireSession(void);

	inline void leaveSession(void);

	inline void checkCompleteRecv(DWORD transferred);

	inline void checkCompleteSend(void);

	static DWORD WINAPI ExecuteWorkerThread(LPVOID pParam);

	void WorkerThread(void);

	static DWORD WINAPI ExecuteTimeoutThread(LPVOID pParam);

	void TimeoutThread(void);


	CSession mSession;

	// IOCP 핸들
	__declspec(align(64))
	HANDLE mIocpHandle;

	// 커넥트 소켓
	//SOCKET mConnectSocket;

		// 워커 스레드 핸들 배열
	HANDLE* mpWorkerThreadHandle;

	// 워커 스레드 ID 배열
	UINT* mpWorkerThreadID;

	// Timeout 스레드 핸들
	HANDLE mTimeoutThreadHandle;

	HANDLE mSessionClearEvent;

	// 서버 IP
	WCHAR *mServerIP;

	// 서버 포트
	DWORD mServerPort;

	// 워커 스레드 개수
	DWORD mWorkerThreadCount;

	// IOCP 러닝 스레드 개수
	DWORD mRunningThreadCount;

	// Timeout 스레드 ID
	UINT mTimeoutThreadID;	

	BOOL mbNagleFlag;

	BOOL mbTimeoutFlag;

	DWORD mTimeoutPeriod;
};

