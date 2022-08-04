#pragma once

class CLanLoginServer : public CLanServer
{
public:

	CLanLoginServer(void);

	~CLanLoginServer(void);

	virtual BOOL OnStart(void) final;

	virtual void OnStartWorkerThread(void) final;

	virtual void OnStartAcceptThread(void) final;

	virtual void OnClientJoin(UINT64 sessionID) final;

	virtual void OnClientLeave(UINT64 sessionID) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	virtual void OnCloseWorkerThread(void) final;

	virtual void OnCloseAcceptThread(void) final;

	virtual BOOL OnConnectionRequest(const WCHAR* pUserIP, WORD userPort) final;

	virtual void OnError(INT errorCode, const WCHAR* pErrorMessage) final;

	virtual void OnStop(void) final;

	BOOL GetGameServerLoginFlag(void) const;

	BOOL GetChatServerLoginFlag(void) const;

	void SetNetLoginServer(CNetLoginServer* pNetLoginServer);

private:

	BOOL recvProcedure(UINT64 sessionID, WORD messageType, CMessage* pMessage);

	BOOL recvProcedureLoginRequest(UINT64 sessionID, CMessage* pMessage);

	BOOL recvProcedureClientServerLogin(UINT64 sessionID, CMessage* pMessage);

	UINT64 mGameServerSessionID;

	UINT64 mChatServerSessionID;

	CNetLoginServer* mpNetLoginServer;
};

