#pragma once

class CLanMonitoringClient : public CLanClient
{
public:

	CLanMonitoringClient(void);

	~CLanMonitoringClient(void);

	virtual BOOL OnStart(void) final;

	virtual void OnServerJoin(UINT64 sessionID) final;

	virtual void OnServerLeave(UINT64 sessionID) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	virtual void OnError(DWORD errorCode, const WCHAR* errorMessage) final;

	virtual void OnStop(void) final;

	void SendProfileInfo(void);

	void SetServerNo(DWORD serverNo);

	void SetContentsPtr(CChattingServer* pEchoServer);

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

	void procedureChatServerRun(void);

	void procedureChatServerCPU(void);

	void procedureChatServerMemory(void);

	void procedureChatServerSessionCount(void);

	void procedureChatServerPlayerCount(void);

	void procedureChatServerUpdateTPS(void);

	void procedureChatServerMessagePool(void);

	void procedureChatServerJobPool(void);

	BOOL mbConnectThreadFlag;

	BOOL mbUpdateThreadFlag;

	BOOL mbConnectFlag;

	DWORD mUpdateThreadID;

	DWORD mConnectThreadID;

	DWORD mServerNo;

	UINT64 mSessionID;

	HANDLE mUpdateThreadHandle;

	HANDLE mConnectThreadHandle;

	HANDLE mConnectEvent;

	CChattingServer* mpChatServer;

};

