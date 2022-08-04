#pragma once

#define REQ_ECHO 1

class CContents : public CLanServer
{
public:

	CContents()
	{
	}

	~CContents()
	{
	}

	virtual BOOL OnStart(void) final;

	virtual void OnStartWorkerThread(void) final;

	virtual void OnStartAcceptThread(void) final;

	void OnClientJoin(UINT64 sessionID) final;

	void OnClientLeave(UINT64 sessionID) final;

	void OnRecv(UINT64 sessionID, CMessage* pMessage) final;
	
	virtual void OnCloseWorkerThread(void) final;

	virtual void OnCloseAcceptThread(void) final;

	BOOL OnConnectionRequest(const WCHAR* userIP, WORD userPort) final;

	void OnError(INT errorCode, const WCHAR* errorMessage) final;

	void OnStop(void) final;

private:

	void packingLoginMessage(CMessage* pMessage);

	bool procRecvMessage(UINT64 sessionID, DWORD messageType , CMessage* pMessage);

	bool procEchoMessage(UINT64 sessionID, CMessage* pMessage);

	void packingEchoMessage(CMessage* pMessage, UINT64 payload);

};

