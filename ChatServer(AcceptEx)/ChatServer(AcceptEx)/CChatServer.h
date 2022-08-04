#pragma once

#include "CPlayer.h"
#include "CJob.h"

namespace chatserver
{
	constexpr INT SECTOR_MAX_WIDTH = 50;
	constexpr INT SECTOR_MAX_HEIGHT = 50;
	constexpr INT MAX_CHAT_LENGTH = 200;
}

class CChatServer : public CNetServer
{
public:
	CChatServer();

	~CChatServer();

	DWORD GetJobQueueStatus(void);

	DWORD GetPlayerCount(void);

	virtual BOOL OnStart(void) final;

	virtual void OnClientJoin(UINT64 sessionID) final;

	virtual void OnClientLeave(UINT64 sessionID) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	virtual void OnStartAcceptThread(void) final;

	virtual void OnCloseAcceptThread(void) final;

	virtual void OnStartWorkerThread(void) final;

	virtual void OnCloseWorkerThread(void) final;

	virtual bool OnConnectionRequest(const WCHAR* userIP, WORD userPort) final;

	virtual void OnError(int errorCode, const WCHAR* errorMessage) final;

	virtual void OnStop(void) final;

	struct stSector
	{
		LONG posY;
		LONG posX;
	};

	struct stSectorAround
	{
		DWORD count;
		stSector aroundSector[9];
	};


	INT GetJobQueueUseSize(void) const;

	void SetWhiteIPModeFlag(BOOL bWhiteIPModeFlag);

	BOOL GetWhiteIPModeFlag(void) const;

	DWORD GetUpdateTPS(void) const;

	void InitializeChatTPS(void);

private:

	static DWORD WINAPI ExecuteUpdateThread(void* pParam);

	void UpdateThread(void);

	CPlayer* findPlayer(UINT64 sessionID);

	//CChatServer::CConnectionState* findConnectionState(UINT64 sessionID);

	BOOL createPlayer(UINT64 acountNumber, UINT64 sessionID, CPlayer** pPlayer);

	void deletePlayer(UINT64 acountNumber);

	CJob* createJob(UINT64 sessionID, CJob::eJobType jobType, CMessage* pMessage = nullptr);


	BOOL setupUpdateThread(void);


	void removeSector(CPlayer* pPlayer);

	void addSector(CPlayer* pPlayer);

	void getSectorAround(INT posY, INT posX, stSectorAround* pSectorAround);

	void sendOneSector(stSector* pSector, CMessage* pMessage);

	void sendAroundSector(stSectorAround* pSectorArround, CMessage* pMessage);


	// Job �б⹮ ���� �Լ�
	void jobProcedure(CJob* pJob);

	void jobProcedureClientJoin(UINT64 sessionID);

	void jobProcedureMessage(UINT64 sessionID, CMessage* pMessage);

	void jobProcedureClientLeave(UINT64 sessionID);

	void jobProcedureServerStop(void);


	// recv �޽��� �б⹮ ���� �Լ�
	BOOL recvProcedure(UINT64 sessionID, DWORD messageType, CMessage* pMessage);

	BOOL recvProcedureLoginRequest(UINT64 sessionID, CMessage* pMessage);

	BOOL recvProcedureSectorMoveRequest(UINT64 sessionID, CMessage* pMessage);

	BOOL recvProcedureChatRequest(UINT64 sessionID, CMessage* pMessage);

	BOOL recvProcedureHeartbeatRequest(UINT64 sessionID);


	// �޽��� ���� �Լ�
	void sendLoginResponse(bool bLoginFlag, CPlayer* pPlayer);

	void sendSectorMoveResponse(CPlayer* pPlayer);

	void sendChatResponse(WORD chatLength, WCHAR* pChat, CPlayer* pPlayer);

	//void sendLoginResponse(bool bCPlayer* pPlayer);


	// ���� �޽��� ���� �Լ�
	void packingLoginResponseMessage(bool bLoginFlag, UINT64 accountNumber, CMessage* pMessage);

	void packingSectorMoveMessage(UINT64 accountNumber, WORD sectorX, WORD sectorY, CMessage* pMessage);

	void packingChatMessage(UINT64 accountNumber, WCHAR* pPlayerID, WCHAR* pPlayerNickName, WORD chatLength, WCHAR* pChat, CMessage* pMessage);


	DWORD mUpdateTPS;

	BOOL mbStopFlag;

	BOOL mbUpdateLoopFlag;

	BOOL mbWhiteIPModeFlag;

	DWORD mPlayerCount;

	DWORD mUpdateThreadID;

	HANDLE mUpdateThreadHandle;

	HANDLE mUpdateEvent;

	stSectorAround mSectorAround[chatserver::SECTOR_MAX_HEIGHT][chatserver::SECTOR_MAX_WIDTH];

	CLockFreeQueue<CJob*> mJobQueue;

	//std::unordered_map<UINT64, CChatServer::CConnectionState*> mConnectionStateMap;

	std::unordered_map<UINT64, CPlayer*> mPlayerMap;

	std::unordered_map<UINT64, CPlayer*> mSectorArray[chatserver::SECTOR_MAX_HEIGHT][chatserver::SECTOR_MAX_WIDTH];


};

