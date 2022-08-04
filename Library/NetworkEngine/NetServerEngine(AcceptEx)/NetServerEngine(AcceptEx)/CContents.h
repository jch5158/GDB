#pragma once

#define REQ_ECHO 1

class CContents : public CNetServer
{
public:

	CContents()
	{
	}

	~CContents()
	{
	}

	virtual BOOL OnStart(void) final;

	virtual void OnClientJoin(UINT64 sessionID) final;

	virtual void OnClientLeave(UINT64 sessionID) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;
	
	virtual bool OnConnectionRequest(const WCHAR* userIp, WORD userPort) final;

	virtual void OnError(int errorCode,const WCHAR* errorMessage) final;

	virtual void OnStop(void) final;


	virtual void OnStartWorkerThread(void) final;


	virtual void OnCloseWorkerThread(void) final;




private:

	void packingLoginMessage(CMessage* pMessage);

	bool procRecvMessage(UINT64 sessionID, DWORD messageType , CMessage* pMessage);

	bool procEchoMessage(UINT64 sessionID, CMessage* pMessage);

	void packingEchoMessage(CMessage* pMessage, UINT64 payload);

};

