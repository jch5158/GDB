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

	void SetLoginServerPtr(CNetLoginServer* pLoginServer);

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

	void sendLoginRequest(void);

	void sendProfileInfo(void);
	void sendLoginServerRun(void);
	void sendLoginServerCPU(void);
	void sendLoginServerMemory(void);
	void sendLoginServerSession(void);
	void sendLoginServerAuthTPS(void);
	void sendLoginServerPacketPool(void);

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

	CNetLoginServer* mpLoginServer;

};

