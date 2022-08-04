#pragma once

class CLanMonitoringClient : public CLanClient
{
public:

	CLanMonitoringClient(void);

	~CLanMonitoringClient(void);

	virtual BOOL OnStart(void) final;

	virtual inline void OnServerJoin(UINT64 sessionID) final;

	virtual inline void OnServerLeave(UINT64 sessionID) final;

	virtual inline void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	virtual inline void OnError(DWORD errorCode, const WCHAR* errorMessage) final;

	virtual inline void OnStop(void) final;

	void SendProfileInfo(void);

	void SetServerNo(DWORD serverNo);

	void SetEchoServerPtr(CEcho* pEchoServer);

private:
	
	static DWORD WINAPI ExecuteUpdateThread(void* pParam);

	static DWORD WINAPI ExecuteConnectThread(void* pParam);

	void UpdateThread(void);

	void ConnectThread(void);

	BOOL setupUpdateThread(void);

	BOOL setupConnectThread(void);
	
	void closeUpdateThread(void);

	void closeConnectThread(void);

	void procedureLoginServer(void);

	void procedureGameServerRun(void);

	void procedureGameServerCPU(void);

	void procedureGameServerMemory(void);

	void procedureGameSessionCount(void);

	void procedureAuthModePlayerCount(void);

	void procedureGameModePlayerCount(void);

	void procedureAcceptTPS(void);

	void procedureRecvTPS(void);

	void procedureSendTPS(void);

	void procedureDBWriteTPS(void);

	void procedureDBWriteQueueSize(void);

	void procedureAuthThreadFPS(void);

	void procedureGameThreadFPS(void);

	void procedureMessagePoolUseSize(void);

	// 모니터링 서버에 연결되었음을 알려주는 Flag
	BOOL mbConnectFlag;

	BOOL mbUpdateThreadFlag;

	BOOL mbConnectThreadFlag;

	DWORD mUpdateThreadID;

	DWORD mConnectThreadID;

	DWORD mServerNo;

	UINT64 mSessionID;

	HANDLE mUpdateThreadHandle;

	HANDLE mConnectThreadHandle;

	HANDLE mConnectEvent;

	CEcho* mpEchoServer;
};

