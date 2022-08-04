#pragma once


namespace chattingserver
{
	constexpr int SECTOR_MAX_WIDTH = 50;
	constexpr int SECTOR_MAX_HEIGHT = 50;
	constexpr int MAX_CHAT_LENGTH = 200;

	void packingLoginReponse(bool bLoginFlag, unsigned long long accountNumber, CMessage* pMessage);
	void packingSectorMoveResponse(unsigned long long accountNumber, unsigned short sectorX, unsigned short sectorY, CMessage* pMessage);
	void packingChatResponse(unsigned long long accountNumber, wchar_t* pPlayerID, unsigned int cbPlayerID, wchar_t* pPlayerNickName, unsigned int cbPlayerNickName, unsigned short chatLength, wchar_t* pChat, CMessage* pMessage);
}

class CChattingServer : public CNetServer
{
private:

	struct stSector
	{
		int posY;
		int posX;
	};

	// 영향권 섹터
	struct stSectorAround
	{
		int count;
		stSector aroundSector[9];
	};

public:

	CChattingServer();
	virtual ~CChattingServer();

	CChattingServer(const CChattingServer&) = delete;
	CChattingServer& operator=(const CChattingServer&) = delete;

	virtual bool OnStart(void) final;
	virtual void OnStop(void) final;
	virtual void OnStartAcceptThread(void) final;
	virtual void OnStartWorkerThread(void) final;
	virtual void OnCloseAcceptThread(void) final;
	virtual void OnCloseWorkerThread(void) final;
	virtual bool OnConnectionRequest(const wchar_t* userIP, unsigned short userPort) final;
	virtual void OnClientJoin(unsigned long long sessionID) final;                             // 신규 세션이 접속하였음을 JobQueue에 Job을 던져서 알려준다.
	virtual void OnClientLeave(unsigned long long sessionID) final;                            // // 세션이 종료하였음을 JobQueue를 통해 알려준다.
	virtual void OnRecv(unsigned long long sessionID, CMessage* pMessage) final;
	virtual void OnError(unsigned int errorCode, const wchar_t* errorMessage) final;

	int GetJobQueueStatus(void) const;
	int GetPlayerCount(void) const;

	long GetUpdateTPS(void) const;
	void InitializeUpdateTPS(void);

private:

	// 스레드 생성 관련 함수
	static unsigned __stdcall ExecuteUpdateThread(void* pParam);
	static unsigned __stdcall ExecuteAuthenticThread(void* pParam);
	void UpdateThread(void);
	void AuthenticThread(void);
	bool setupUpdateThread(void);
	bool setupAuthenticThread(void);
	void closeWaitUpdateThread(void);
	void closeWaitAuthenticThread(void);


	// AccountNoMap
	bool insertAccountNoMap(unsigned long long sessionID, unsigned long long accountNo);
	bool eraseAccountNoMap(unsigned long long sessionID);
	bool findAccountNoFromAccountMap(unsigned long long sessionID, unsigned long long* pAccountNo);


	// PlayerMap 함수
	bool insertPlayerMap(unsigned long long accountNo, CPlayer* pPlayer);
	bool erasePlayerMap(unsigned long long accountNo);
	CPlayer* findPlayerFromPlayerMap(unsigned long long accountNo) const;


	// PlayerSectorMap 함수
	bool insertPlayerSectorMap(int sectorPosX, int sectorPosY, unsigned long long accountNo, CPlayer* pPlayer);
	bool erasePlayerSectorMap(int sectorPosX, int sectorPosY, unsigned long long accountNo);
	void removeSector(CPlayer* pPlayer);


	// Player 생성, 삭제 함수
	bool createPlayer(unsigned long long acountNumber, unsigned long long sessionID, CPlayer **pPlayer);
	bool deletePlayer(unsigned long long acountNumber, unsigned long long sessionID);


	void getSectorAround(int posY, int posX, stSectorAround* pSectorAround);

	// Sector Send
	void sendOneSector(stSector* pSector, CMessage* pMessage);
	void sendAroundSector(stSectorAround* pSectorArround, CMessage* pMessage);


	CJob* createJob(unsigned long long sessionID, CJob::eJobType jobType, CMessage* pMessage = nullptr);

	// Job 분기문 관련 함수
	void jobProcedure(CJob* pJob);
	void clientJoinJobRequest(unsigned long long sessionID);
	void loginJobRequest(unsigned long long sessionID, CJob* pJob);
	void messageJobRequest(unsigned long long sessionID, CMessage* pMessage);
	void clientLeaveJobRequest(unsigned long long sessionID);
	void serverStopJobRequest(void);


	// 메시지 분기문 관련 함수
	bool messageProcedure(unsigned long long sessionID, unsigned short messageType , CMessage* pMessage);
	bool loginRequest(unsigned long long sessionID, CMessage* pMessage);
	bool sectorMoveRequest(unsigned long long sessionID, CMessage* pMessage);
	bool chatRequest(unsigned long long sessionID, CMessage* pMessage);
	bool heartbeatRequest(unsigned long long sessionID);


	// 메시지 응답 함수
	void sendLoginResponse(bool bLoginFlag, unsigned long long accountNo, unsigned long long sessionID);
	void sendSectorMoveResponse(CPlayer* pPlayer);
	void sendChatResponse(unsigned short chatLength, wchar_t* pChat, CPlayer *pPlayer);

	
	bool mbStopFlag;
	bool mbUpdateLoopFlag;

	unsigned int mAuthenticThreadID;
	unsigned int mUpdateThreadID;
	HANDLE mUpdateThreadHandle;
	HANDLE mAuthenticThreadHandle;

	HANDLE mUpdateEvent;
	HANDLE mAuthenticEvent;

	long mUpdateTPS;
	int mPlayerCount;

	CRedisConnector mRedisConnector;

	CLockFreeQueue<CJob*> mJobQueue;
	CTemplateQueue<CJob*> mAuthenticJobQueue;
	

	// 필요가 없네
	// key : sessionID, value : AccountNo 
	std::unordered_map<unsigned long long, unsigned long long> mAccountNoMap;

	// key : accountNo, value : CPlayer*
	std::unordered_map<unsigned long long, CPlayer*> mPlayerMap;

	// key : accountNo, value : CPlayer*
	std::unordered_map<unsigned long long, CPlayer*> mPlayerSectorMap[chattingserver::SECTOR_MAX_HEIGHT][chattingserver::SECTOR_MAX_WIDTH];

	// 서버를 가동할 때 주변 섹터를 미리 구해놓는다.
	stSectorAround mSectorAround[chattingserver::SECTOR_MAX_HEIGHT][chattingserver::SECTOR_MAX_WIDTH];
};

