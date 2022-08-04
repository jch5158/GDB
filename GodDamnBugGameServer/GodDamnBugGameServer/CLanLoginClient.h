#pragma once

class CLanLoginClient : public CLanClient
{
public:

	CLanLoginClient(void);

	~CLanLoginClient(void);

	virtual BOOL OnStart(void) final;

	virtual void OnServerJoin(UINT64 sessionID) final;

	virtual void OnServerLeave(UINT64 sessionID) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	virtual void OnError(DWORD errorCode, const WCHAR* pErrorMessage) final;

	virtual void OnStop(void) final;

	BOOL GetConnectStateFlag(void) const;

	void NotificationClientLoginSuccess(UINT64 accountNo);

private:

	static DWORD WINAPI ExecuteConnectThread(void* pParam);

	void ConnectThread(void);

	BOOL setupConnectThread(void);

	void closeConnectThread(void);

	//en_PACKET_SS_RES_NEW_CLIENT_LOGIN
	void sendClientLoginSuccess(UINT64 accountNo);
	void sendLoginServerLoginRequest(void);

	void packingClientLoginSuccess(UINT64 accountNo, UINT64 serverType, CMessage* pMessage);
	void packingLoginServerLoginRequest(BYTE serverType, CMessage* pMessage);

	//  로그인 서버에 연결되었는지 확인하는 플래그
	BOOL mbConnectStateFlag;

	BOOL mbConnectThreadFlag;

	UINT64 mSessionID;

	DWORD mConnectThreadID;

	HANDLE mConnectThreadHandle;

	HANDLE mConnectEvent;

};

