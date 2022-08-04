#pragma once
class CLanMonitoringServer : public CLanServer
{
public:

	CLanMonitoringServer(void);

	~CLanMonitoringServer(void);

	struct stServerProfile
	{
		UINT64 cpuProfileTotal;
		INT cpuProfileAvr;
		INT cpuProfileMin;
		INT cpuProfileMax;
		INT cpuProfileCount;

		UINT64 nonpagedMemoryProfileTotal;
		INT nonpagedMemoryProfileAvr;
		INT nonpagedMemoryProfileMin;
		INT nonpagedMemoryProfileMax;
		INT nonpagedMemoryProfilCount;

		UINT64 recvKBytesProfileTotal;
		INT recvKBytesProfileAvr;
		INT recvKBytesProfileMin;
		INT recvKBytesProfileMax;
		INT recvKBytesProfileCount;

		UINT64 sendKBytesProfileTotal;
		INT sendKBytesProfileAvr;
		INT sendKBytesProfileMin;
		INT sendKBytesProfileMax;
		INT sendKBytesProfileCount;

		UINT64 availableMemoryProfileTotal;
		INT availableMemoryProfileAvr;
		INT availableMemoryProfileMin;
		INT availableMemoryProfileMax;
		INT availableMemoryProfileCount;
	};


	struct stLoginServerProfile
	{
		BOOL bServerOnFlag;

		UINT64 cpuProfileTotal;
		INT cpuProfileAvr;
		INT cpuProfileMin;
		INT cpuProfileMax;
		INT cpuProfileCount;

		UINT64 memoryProfileTotal;
		INT memoryProfileAvr;
		INT memoryProfileMin;
		INT memoryProfileMax;
		INT memoryProfileCount;

		UINT64 sessionCountProfileTotal;
		INT sessionCountProfileAvr;
		INT sessionCountProfileMin;
		INT sessionCountProfileMax;
		INT sessionCountProfileCount;

		UINT64 authTPSProfileTotal;
		INT authTPSProfileAvr;
		INT authTPSProfileMin;
		INT authTPSProfileMax;
		INT authTPSProfileCount;

		UINT64 messagePoolProfileTotal;
		INT messagePoolProfileAvr;
		INT messagePoolProfileMin;
		INT messagePoolProfileMax;
		INT messagePoolProfileCount;
	};

	struct stGameServerProfile
	{
		BOOL bServerOnFlag;

		UINT64 cpuProfileTotal;
		INT cpuProfileAvr;
		INT cpuProfileMin;
		INT cpuProfileMax;
		INT cpuProfileCount;

		UINT64 memoryProfileTotal;
		INT memoryProfileAvr;
		INT memoryProfileMin;
		INT memoryProfileMax;
		INT memoryProfileCount;

		UINT64 sessionCountProfileTotal;
		INT sessionCountProfileAvr;
		INT sessionCountProfileMin;
		INT sessionCountProfileMax;
		INT sessionCountProfileCount;

		UINT64 authPlayerProfileTotal;
		INT authPlayerProfileAvr;
		INT authPlayerProfileMin;
		INT authPlayerProfileMax;
		INT authPlayerProfileCount;

		UINT64 gamePlayerProfileTotal;
		INT gamePlayerProfileAvr;
		INT gamePlayerProfileMin;
		INT gamePlayerProfileMax;
		INT gamePlayerProfileCount;

		UINT64 acceptTPSProfileTotal;
		INT acceptTPSProfileAvr;
		INT acceptTPSProfileMin;
		INT acceptTPSProfileMax;
		INT acceptTPSProfileCount;

		UINT64 recvTPSProfileTotal;
		INT recvTPSProfileAvr;
		INT recvTPSProfileMin;
		INT recvTPSProfileMax;
		INT recvTPSProfileCount;

		UINT64 sendTPSProfileTotal;
		INT sendTPSProfileAvr;
		INT sendTPSProfileMin;
		INT sendTPSProfileMax;
		INT sendTPSProfileCount;

		UINT64 DBWriteTPSProfileTotal;
		INT DBWriteTPSProfileAvr;
		INT DBWriteTPSProfileMin;
		INT DBWriteTPSProfileMax;
		INT DBWriteTPSProfileCount;

		UINT64 DBWriteQSizeProfileTotal;
		INT DBWriteQSizeProfileAvr;
		INT DBWriteQSizeProfileMin;
		INT DBWriteQSizeProfileMax;
		INT DBWriteQSizeProfileCount;

		UINT64 authFPSProfileTotal;
		INT authFPSProfileAvr;
		INT authFPSProfileMin;
		INT authFPSProfileMax;
		INT authFPSProfileCount;

		UINT64 gameFPSProfileTotal;
		INT gameFPSProfileAvr;
		INT gameFPSProfileMin;
		INT gameFPSProfileMax;
		INT gameFPSProfileCount;

		UINT64 messagePoolProfileTotal;
		INT messagePoolProfileAvr;
		INT messagePoolProfileMin;
		INT messagePoolProfileMax;
		INT messagePoolProfileCount;
	};

	struct stChatServerProfile
	{
		BOOL bServerOnFlag;

		UINT64 cpuProfileTotal;
		INT cpuProfileAvr;
		INT cpuProfileMin;
		INT cpuProfileMax;
		INT cpuProfileCount;

		UINT64 memoryProfileTotal;
		INT memoryProfileAvr;
		INT memoryProfileMin;
		INT memoryProfileMax;
		INT memoryProfileCount;

		UINT64 sessionCountProfileTotal;
		INT sessionCountProfileAvr;
		INT sessionCountProfileMin;
		INT sessionCountProfileMax;
		INT sessionCountProfileCount;

		UINT64 playerCountProfileTotal;
		INT playerCountProfileAvr;
		INT playerCountProfileMin;
		INT playerCountProfileMax;
		INT playerCountProfileCount;

		UINT64 updateTPSProfileTotal;
		INT updateTPSProfileAvr;
		INT updateTPSProfileMin;
		INT updateTPSProfileMax;
		INT updateTPSProfileCount;

		UINT64 messagePoolProfileTotal;
		INT messagePoolProfileAvr;
		INT messagePoolProfileMin;
		INT messagePoolProfileMax;
		INT messagePoolProfileCount;

		UINT64 updateQSizeProfileTotal;
		INT updateQSizeProfileAvr;
		INT updateQSizeProfileMin;
		INT updateQSizeProfileMax;
		INT updateQSizeProfileCount;

	};

	virtual BOOL OnStart(void) final;

	virtual void OnStartWorkerThread(void) final;

	virtual void OnStartAcceptThread(void) final;

	virtual void OnClientJoin(UINT64 sessionID) final;

	virtual void OnClientLeave(UINT64 sessionID) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	virtual void OnCloseWorkerThread(void) final;

	virtual void OnCloseAcceptThread(void) final;

	// accept 직후 바로 호출
	virtual BOOL OnConnectionRequest(const WCHAR* userIP, WORD userPort) final;

	virtual void OnError(INT errorCode, const WCHAR* errorMessage) final;

	virtual void OnStop(void) final;

	void SetNetMonitoringServer(CNetMonitoringServer* pNetMonitoringServer);

	void SetGameServerNo(DWORD gameServerNo);

	void SetChatServerNo(DWORD chatServerNo);

	void SetLoginServerNo(DWORD loginServerNo);

	BOOL GetGameServerLoginFlag(void) const;

	BOOL GetLoginServerLoginFlag(void) const;

	BOOL GetChatServerLoginFlag(void) const;

private:

	class CCriticalSection
	{
	public:
		CCriticalSection(CRITICAL_SECTION* pCriticalSection)
			:mpCriticalSection(pCriticalSection)
		{
			EnterCriticalSection(pCriticalSection);
		}

		~CCriticalSection()
		{
			LeaveCriticalSection(mpCriticalSection);
		}

		CRITICAL_SECTION* mpCriticalSection;
	};

	static DWORD WINAPI ExecuteUpdateThread(void* pParam);

	static DWORD WINAPI ExecuteDBWriteThread(void* pParam);

	void UpdateThread(void);

	void DBWriteThread(void);

	BOOL setupUpdateThread(void);

	BOOL setupDBWriteThread(void);

	void closeUpdateThread(void);

	void closeDBWriteThread(void);

	
	void getTime(tm* pNowTime);

	// 서버 시작 시간 셋팅하기
	void setServerStartTime(WCHAR* pBuffer,INT bufferCb);

	// 테이블 존제여부 확인, 달이 바뀔 경우 테이블이 존제하지 않을 수 있음
	BOOL checkProfileTable(WCHAR* pTableName);

	// 테이블 이름 알아내기
	void getServerProfileTableName(WCHAR* pBuffer, DWORD bufferCb);

	// 테이블 생성하기
	void setServerProfileTable(WCHAR* pTableName);


	void initializaServerProfile(void);

	void initializeGameServerProfile(void);

	void initializeChatServerProfile(void);

	void initializeLoginServerProfile(void);


	void DBWriteMonitoringData(void);

	void DBWriteServerProfile(WCHAR* pTableName);

	void DBWriteGameServerProfile(WCHAR *pTableName);

	void DBWriteChatServerProfile(WCHAR* pTableName);

	void DBWriteLoginServerProfile(WCHAR* pTableName);

	void sendMonitoringQuery(WCHAR* pTableName,DWORD serverNo, WCHAR* mServerStartTime, INT dataType, INT data);
	
	
	BOOL insertServerNo(UINT64 sessionID, DWORD serverNo);

	BOOL findServerNo(UINT64 sessionID, DWORD *pServerNo);

	void eraseServerNo(UINT64 sessionID);



	BOOL recvProcedure(UINT64 sessionID, WORD messageType, CMessage* pMessage);

	BOOL recvProcedureLoginSS(UINT64 sessionID, CMessage* pMessage);

	// 서버로 부터 데이터를 수신받는다.
	BOOL recvProcedureMonitorDataSS(UINT64 sessionID, CMessage* pMessage);

	// 서버로 부터 받은 프로파일링 데이터를 클라이언트에 송신한다.
	void sendMonitorDataCS(DWORD serverNo, BYTE dataType, INT dataValue, INT timeStamp);



	// 1초 주기로 서버 하드웨어 정보를 클라이언트에 송신한다.
	///////////////////////////////////////////////////////
	void sendHardwareProfile(void);
	void sendCPUTotal(void);
	void sendNonpagedMemory(void);
	void sendRecvBytes(void);
	void sendSendBytes(void);
	void sendAvailableMemory(void);
	////////////////////////////////////////////////////////


	void saveProcedure(BYTE dataType, INT dataValue);

	void saveLoginServerRun(void);
	void saveLoginServerCPU(INT dataValue);
	void saveLoginServerMemory(INT dataValue);
	void saveLoginServerSessionCount(INT dataValue);
	void saveLoginServerAuthTPS(INT dataValue);
	void saveLoginServerMessagePoolUseSize(INT dataValue);


	void saveGameServerRun(void);
	void saveGameServerCPU(INT dataValue);
	void saveGameServerMemory(INT dataValue);
	void saveGameServerSessionCount(INT dataValue);
	void saveGameServerAuthModePlayerCount(INT dataValue);
	void saveGameServerGameModePlayerCount(INT dataValue);
	void saveGameServerAcceptTPS(INT dataValue);
	void saveGameServerRecvTPS(INT dataValue);
	void saveGameServerSendTPS(INT dataValue);
	void saveGameServerDBWriteTPS(INT dataValue);
	void saveGameServerDBWriteQueueSize(INT dataValue);
	void saveGameServerAuthThreadFPS(INT dataValue);
	void saveGameServerGameThreadFPS(INT dataValue);
	void saveGameServerMessagePoolUseSize(INT dataValue);


	void saveChatServerRun(void);
	void saveChatServerCPU(INT dataValue);
	void saveChatServerMemory(INT dataValue);
	void saveChatServerSessionCount(INT dataValue);
	void saveChatServerPlayerCount(INT dataValue);
	void saveChatServerUpdateTPS(INT dataValue);
	void saveChatServerMessagePoolUseSize(INT dataValue);
	void saveChatServerJobPoolUseSize(INT dataValue);


	void saveServerCPU(INT dataValue);
	void saveServerNonpagedMemory(INT dataValue);
	void saveServerRecvBytes(INT dataValue);
	void saveServerSendBytes(INT dataValue);
	void saveAvailableMemory(INT dataValue);

	BOOL mbGameServerLoginFlag;
	BOOL mbChatServerLoginFlag;
	BOOL mbLoginServerLoginFlag;

	BOOL mbUpdateThreadFlag;
	BOOL mbDBWriteThreadFlag;

	DWORD mGameServerNo;
	DWORD mChatServerNo;
	DWORD mLoginServerNo;

	DWORD mUpdateThreadID;
	DWORD mDBWriteThreadID;

	HANDLE mUpdateThreadHandle;
	HANDLE mDBWriteThreadHandle;

	CNetMonitoringServer* mpNetMonitoringServer;	

	stServerProfile mServerProfile;
	stLoginServerProfile mLoginServerProfile;
	stGameServerProfile mGameServerProfile;
	stChatServerProfile mChatServerProfile;

	WCHAR mGameServerStartTime[MAX_PATH];
	WCHAR mChatServerStartTime[MAX_PATH];
	WCHAR mLoginServerStartTime[MAX_PATH];

	CRITICAL_SECTION mServerNoCriticalSection;

	CDBConnector mDBConnector;

	std::unordered_map<UINT64, DWORD> mServerNoMap;
};

