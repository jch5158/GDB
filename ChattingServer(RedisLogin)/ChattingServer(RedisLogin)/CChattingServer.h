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

	// ����� ����
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
	virtual void OnClientJoin(unsigned long long sessionID) final;                             // �ű� ������ �����Ͽ����� JobQueue�� Job�� ������ �˷��ش�.
	virtual void OnClientLeave(unsigned long long sessionID) final;                            // // ������ �����Ͽ����� JobQueue�� ���� �˷��ش�.
	virtual void OnRecv(unsigned long long sessionID, CMessage* pMessage) final;
	virtual void OnError(unsigned int errorCode, const wchar_t* errorMessage) final;

	int GetJobQueueStatus(void) const;
	int GetPlayerCount(void) const;

	long GetUpdateTPS(void) const;
	void InitializeUpdateTPS(void);

private:

	// ������ ���� ���� �Լ�
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


	// PlayerMap �Լ�
	bool insertPlayerMap(unsigned long long accountNo, CPlayer* pPlayer);
	bool erasePlayerMap(unsigned long long accountNo);
	CPlayer* findPlayerFromPlayerMap(unsigned long long accountNo) const;


	// PlayerSectorMap �Լ�
	bool insertPlayerSectorMap(int sectorPosX, int sectorPosY, unsigned long long accountNo, CPlayer* pPlayer);
	bool erasePlayerSectorMap(int sectorPosX, int sectorPosY, unsigned long long accountNo);
	void removeSector(CPlayer* pPlayer);


	// Player ����, ���� �Լ�
	bool createPlayer(unsigned long long acountNumber, unsigned long long sessionID, CPlayer **pPlayer);
	bool deletePlayer(unsigned long long acountNumber, unsigned long long sessionID);


	void getSectorAround(int posY, int posX, stSectorAround* pSectorAround);

	// Sector Send
	void sendOneSector(stSector* pSector, CMessage* pMessage);
	void sendAroundSector(stSectorAround* pSectorArround, CMessage* pMessage);


	CJob* createJob(unsigned long long sessionID, CJob::eJobType jobType, CMessage* pMessage = nullptr);

	// Job �б⹮ ���� �Լ�
	void jobProcedure(CJob* pJob);
	void clientJoinJobRequest(unsigned long long sessionID);
	void loginJobRequest(unsigned long long sessionID, CJob* pJob);
	void messageJobRequest(unsigned long long sessionID, CMessage* pMessage);
	void clientLeaveJobRequest(unsigned long long sessionID);
	void serverStopJobRequest(void);


	// �޽��� �б⹮ ���� �Լ�
	bool messageProcedure(unsigned long long sessionID, unsigned short messageType , CMessage* pMessage);
	bool loginRequest(unsigned long long sessionID, CMessage* pMessage);
	bool sectorMoveRequest(unsigned long long sessionID, CMessage* pMessage);
	bool chatRequest(unsigned long long sessionID, CMessage* pMessage);
	bool heartbeatRequest(unsigned long long sessionID);


	// �޽��� ���� �Լ�
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
	

	// �ʿ䰡 ����
	// key : sessionID, value : AccountNo 
	std::unordered_map<unsigned long long, unsigned long long> mAccountNoMap;

	// key : accountNo, value : CPlayer*
	std::unordered_map<unsigned long long, CPlayer*> mPlayerMap;

	// key : accountNo, value : CPlayer*
	std::unordered_map<unsigned long long, CPlayer*> mPlayerSectorMap[chattingserver::SECTOR_MAX_HEIGHT][chattingserver::SECTOR_MAX_WIDTH];

	// ������ ������ �� �ֺ� ���͸� �̸� ���س��´�.
	stSectorAround mSectorAround[chattingserver::SECTOR_MAX_HEIGHT][chattingserver::SECTOR_MAX_WIDTH];
};

