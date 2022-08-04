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

	void SetServerNo(DWORD serverNo);

	void SetContentsPtr(CChattingServer* pEchoServer);

	BOOL GetConnectStateFlag(void) const;

private:

	static DWORD WINAPI ExecuteUpdateThread(void* pParam);

	static DWORD WINAPI ExecuteConnectThread(void* pParam);

	void UpdateThread(void);

	void ConnectThread(void);

	BOOL setupUpdateThread(void);

	BOOL setupConnectThread(void);

	void closeUpdateThread(void);

	void closeConnectThread(void);

	void sendMonitoringServerLogin(void);

	void sendProfileInfo(void);
	void sendChatServerRun(void);
	void sendChatServerCPU(void);
	void sendChatServerMemory(void);
	void sendChatServerSessionCount(void);
	void sendChatServerPlayerCount(void);
	void sendChatServerUpdateTPS(void);
	void sendChatServerMessagePool(void);
	void sendChatServerJobPool(void);

	
	void packingMonitoringServerLogin(WORD type, DWORD serverNo, CMessage* pMessage);
	void packingMonitoringData(WORD type, BYTE dataType, INT data, INT time, CMessage* pMessage);


	BOOL mbConnectThreadFlag;

	BOOL mbUpdateThreadFlag;

	BOOL mbConnectStateFlag;

	DWORD mUpdateThreadID;

	DWORD mConnectThreadID;

	DWORD mServerNo;

	UINT64 mSessionID;

	HANDLE mUpdateThreadHandle;

	HANDLE mConnectThreadHandle;

	HANDLE mConnectEvent;

	CChattingServer* mpChatServer;

};

