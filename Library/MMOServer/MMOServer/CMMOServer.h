#pragma once

namespace mmoserver
{
	// wsabuf 길이는 최대 200으로 설정한다.
	constexpr int MAX_WSABUF_SIZE = 200;
}

class CMMOServer
{
private:

	enum class eSessionMode : int
	{
		NoneMode,             // 세션 미사용 상태
		AuthMode,             // 연결 후 인증모드 상태
		AuthToGameMode,       // 게임 모드로 전환
		GameMode,             // 인증 후 게임모드 상태
        LogoutInAuthMode,     // 종료준비
		LogoutInGameMode,     // 종료준비
		WaitLogoutMode        // 최종 종료
	};

public:

	CMMOServer(void);
	virtual ~CMMOServer(void);

	CMMOServer(const CMMOServer&) = delete;
	CMMOServer& operator=(const CMMOServer&) = delete;

	class CSession
	{
	public:
		friend class CMMOServer;

		CSession(void);
		virtual ~CSession(void) = default;

		CSession(const CSession&) = delete;
		CSession& operator=(const CSession&) = delete;

		void SendPacket(CMessage* pMessage);
		void Disconnect(void);

		void SetAuthToGame();
		void SetRelease();

		bool GetAuthToGameFlag(void) const;
		bool CheckLogoutInAuth(void) const;
		bool CheckLogoutInGame(void) const;
		void GetClientIP(WCHAR* pIP, DWORD bufferCch) const;
		WORD GetClientPort(void) const;

	private:

		virtual void OnAuthClientJoin(void) = 0;                    // AuthMode 신규 클라이언트 접속
		virtual void OnAuthClientLeave(void) = 0;                   // Authentic 스레드에서 나갔어요~ 의미이다.
		virtual void OnAuthMessage(CMessage* pMessage) = 0;         // AuthMode 세션의 클라인트 수신 메시지 처리	
		virtual void OnGameClientJoin(void) = 0;                    // GameMode 신규 클라이언트 접속, 게임 컨텐츠 진입을 위한 데이터 준비 및 셋팅 ( 월드 맵에 캐릭터 셋팅, 퀘스트 준비 등등 )
		virtual void OnGameClientLeave(void) = 0;                   // GameMode 클라이언트 종료, 게임 컨텐츠상의 플레이어 정리 ( 월드 맵에서 제거, 파티 정리 등등 )
		virtual void OnGameMessage(CMessage* pMessage) = 0;         // GameMode 세션의 클라이언트 수신 메시지 처리

		void removeSendCompletionArray(void);
		void removeSendQueue(void);
		void removeRecvCompletionQueue(void);

		//  세션 상태 정보
		eSessionMode        mSessionMode;        // 세션 모드
		UINT64              mSessionID;          // 세션 ID
		int                 mIndex;              // 세션 index
		DWORD               mMsgRecvTime;        // 마지막으로 패킷을 받은 시간
		
		// 세션 Io 작업시 사용하는 정보
		SOCKET              mSocket;
		SOCKADDR_IN         mClientAddr;
		OVERLAPPED          mSendOverlapped;
		OVERLAPPED          mRecvOverlapped;

		
		// Interlocked 변수
		__declspec(align(64)) bool mbSendFlag;
		__declspec(align(64)) long mIoCount;


		// 세션 연결끊길 때 사용하는 플래그 
		bool mbAuthToGameFlag;           // AuthToGame모드로 전환될 때 사용                               
		bool mbDisconnectFlag;           // Disconnect 호출 시                            
		bool mbLogoutFlag;               // IoCount가 0이 될 때
		bool mbReleaseFlag;              // 컨텐츠측에서 더 이상 플레이어에 대한 정보를 사용하지 않을 때 비로소 releaseSession()을 호출한다. ex) 해당 플레이어 DB 작업이 끝났을 때


		int mMessageCount;
		CMessage*                 mSendCompletionArray[mmoserver::MAX_WSABUF_SIZE];  // 송신요청한 패킷 보관 배열 
		CRingBuffer               mRecvRingBuffer;                                   // 링 버퍼          	
		CTemplateQueue<CMessage*> mRecvCompletionQueue;                              // Recv 완성된 메시지를 보관하는 Buffer
		CTemplateQueue<CMessage*> mSendQueue;                                        // Send 할 메시지를 보관하는 Buffer
	};

	// 오픈 IP / 포트 / 네이글 옵션 / TimeoutFlag / headerCode / staticKey/ runningThread / 워커스레드 수(생성수, 러닝수) / 최대접속자 수
	bool Start(const WCHAR* pServerIP, WORD serverPort, DWORD sendFrame, DWORD updateFame, DWORD authenticFrame, DWORD maxMessageSize, bool bNagleFlag, 
		BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, 
		DWORD workerThreadCount, DWORD maxClientCount, DWORD timeoutSec);
	bool Stop(void);

	UINT64 GetAcceptTotal(void) const;
	int GetAcceptTPS(void) const;
	int GetUpdateTPS(void) const;
	int GetAuthenticTPS(void) const;
	int GetSendTPS(void) const;
	int GetRecvTPS(void) const;
	int GetMaxClientCount(void) const;
	int GetCurrentClientCount(void) const;
	int GetRunningThreadCount(void) const;
	int GetWorkerThreadCount(void) const;
	const WCHAR* GetServerBindIP(void) const;
	WORD GetServerBindPort(void) const;
	bool GetNagleFlag(void) const;
	int GetAuthClientCount(void) const;
	int GetGameClientCount(void) const;
	DWORD GetWakeupProcessTime(void) const;
	DWORD GetMaxWakeupWaitTime(void) const;
	DWORD GetMaxWakeupProcessTime(void) const;
	int GetMaxWakeupPerSecond(void) const;
	int GetWakeupPerSecond(void);

	void InitializeTPS(void);

private:

	struct stAcceptInfo
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr;
	};

	virtual bool OnStart(void) = 0;
	virtual void OnStartAuthenticThread(void) = 0;
	virtual void OnStartUpdateThread(void) = 0;
	virtual bool OnConnectionRequest(WCHAR* pUserIP, WORD userPort) = 0;   	// accept 직후 바로 호출	
	virtual void OnAuthUpdate(void) = 0;                                    // AuthenticThread 에서 주기적으로 호출하는 함수
	virtual void OnGameUpdate(void) = 0;                             	    // UpdateThread 에서 켄텐츠 처리용으로 1 Loop 당 처리하는 함수이다.
	virtual void OnAssociateSessionWithPlayer(CSession **pSession) = 0;     // Player 와 Session 매핑을 위한 오버라이딩 함수	
	virtual void OnReleaseSessionWithPlayer(CSession* pSession) = 0;
	virtual void OnCloseAuthenticThread(void) = 0;
	virtual void OnCloseUpdateThread(void) = 0;
	virtual void OnStop(void) = 0;


	// 스레드
	static DWORD WINAPI ExecuteAcceptThread(void* pParam);
	static DWORD WINAPI ExecuteAuthenticThread(void* pParam);
	static DWORD WINAPI ExecuteUpdateThread(void* pParam);
	static DWORD WINAPI ExecuteSendThread(void* pParam);
	static DWORD WINAPI ExecuteWorkerThread(void* pParam);
	void AcceptThread(void);
	void AuthenticThread(void);
	void UpdateThread(void);
	void SendThread(void);
	void WorkerThread(void);


	
	//=============================================================	
	// 아래 함수들 호출
	bool setupMMOServerNetwork(void);
	bool setupListenSocket(void);        	// 리슨 소켓 생성 및 셋팅
	bool bindListenSocket(void);            // 리슨 소켓 바인딩
	bool listenSocket(void);                // 리슨 전환
 	bool setupIOCP(void);                   // IOCP 생성
	void setupSessionPointerArray(void);    // 세션들 셋팅
	//=============================================================


	//=============================================================
	// 스레드 셋팅
	bool setupMMOServerThread(void);
	bool setupAcceptThread(void);           // AcceptThread 생성
	bool setupAuthenticThread(void);        // AuthenticThread 생성
	bool setupUpdateThread(void);           // UpdateThread 생성
	bool setupSendThread(void);             // SendThread 생성
	bool setupWorkerThread(void);           // WorkerThread 생성
	//=============================================================


	// 세션 셋팅
	void initializeSessions(stAcceptInfo* pAcceptInfoArray, DWORD arraySize);
	void associateSocketWithIOCP(SOCKET socket, void* completionKey);

	// 세션 반환
	void releaseSession(CSession *pSession);

	
	// 네트워크 함수
	void recvPost(CSession* pSession);
	void sendPost(CSession* pSession);
	void checkCompletionRecv(CSession* pSession, DWORD transferred);
	void checkCompletionSend(CSession* pSession);


	// AuthThread 함수
	void checkAcceptInfo(void);
	void checkMessageOfAuthMode(void);
	void authSessionTimeout(void);
	void checkLogoutFlagOfAuthMode(void);
	void checkLogoutInAuthMode(void);
	void checkLeaveAuthToGameMode(void);


	// UpdateThread 함수
	void joinAuthToGameMode(void);
	void checkMessageOfGameMode(void);
	void updateSessionTimeout(void);
	void checkLogoutFlagOfGameMode(void);
	void checkLogoutInGameMode(void);
	void checkWaitLogoutMode(void);


	// 서버 종료시 사용하는 함수
	bool closeAcceptThread(void);
	void closeAutenticThread(void);
	void closeUpdateThread(void);
	void closeSendThread(void);
	void closeWorkerThread(void);
	void cleanupSessionArray(void);
	void closeIOCP(void);
	void kickSessions(void);


	// 로깅할 에러코드인지 확인
	bool isLoggingErrorCode(unsigned int errorCode) const;


	// 서버 운영 데이터
	SOCKET                        mListenSocket;           // 리슨 소켓
	std::vector<CSession*>        mSessionArray;           // 세션 배열
	HANDLE                        mIocpHandle;             // IOCP 핸들
	CLockFreeStack<int>           mReleaseSessionStack;	   // 사용가능 세션 index
	CTemplateQueue<stAcceptInfo>  mAcceptInfoQueue;        // Accept 정보

	
														   
    // 서버 셋팅 정보
	std::wstring   mServerIP;             // 서버 바인딩 IP
	WORD           mServerPort;           // 서버 바인딩 포트
	int            mSendFrame;            // sendFrame
	int            mAuthFrame;            // authFrame
	int            mUpdateFrame;          // updateFrame
	int            mMaxMessageSize;       // 메시지 최대 크기
	int            mMaxClientCount;       // 최대 접속자 수
	int            mRunningThreadCount;   // 러닝 스레드 카운트
	int            mWorkerThreadCount;    // 워커 스레드 카운트
	bool           mbNagleFlag;           // nagleFlag
	DWORD          mTimeoutSec;           // 타임아웃시간



	// WorkerThread 성능 추적 데이터
	__declspec(align(64))
	int        mWorkerThreadIndex;           // 로그를 남길 WorkerThread Index;
	int        mWakeupPerSecond;        	 // 모든 WorkerThread가 초당 일어나는 횟수	
	DWORD*     mpWakeupProcessTimeArray;     // WorkerThread가 1초 동안 얼마나 실행되었는지 평균구할 때 사용
	DWORD*     mpWakeupWaitTimeArray; 	     // WorkerThread가 GQCS를 호출 후 얼마뒤에 깨어나는지
	int        mMaxWakeupPerSecond;		     // 초당 가장 많이 일어난 횟수를 구하는 값
	DWORD      mMaxWakeupProcessTime;   	 // WorkerThrad가 가장 많이 프로세싱한 시간



	// 서버 접속자 상태 정보
	UINT64                    mAcceptTotal;	
	int                       mCurrentClientCount;   // 총 접속자 ( auth + game )
	int                       mAuthClientCount;      // auth 접속자 
	int                       mGameClientCount;      // game 접속자
	_declspec (align(64)) int mAcceptTPS;
	_declspec (align(64)) int mUpdateTPS;
	_declspec (align(64)) int mAuthenticTPS;
	_declspec (align(64)) int mSendTPS;
	_declspec (align(64)) int mRecvTPS;



	// 서버 종료 시 사용하는 변수
	bool        mbAutheticThreadFlag;
	bool        mbUpdateThreadFlag;
	bool        mbSendThreadFlag;
	bool        mbSessionClearFlag;
	HANDLE      mSessionClearEvent; 	// 모든 Session을 대상으로 Disconnect() 호출 후 모두 release 되었다면 시그널로 변경


	// thread 정보
	DWORD       mAcceptThreadID;
	DWORD       mAuthThreadID;
	DWORD       mUpdateThreadID;
	DWORD       mSendThreadID;
	DWORD*      mpWorkerThreadIDArray;          // WorkerThread ID 배열
	HANDLE      mAcceptThreadHandle;
	HANDLE      mAuthenticThreadHandle;
	HANDLE      mUpdateThreadHandle;
	HANDLE      mSendThreadHandle;
	HANDLE*     mpWorkerThreadHandleArray; 	    // WorkerThread Handle 배열
};

