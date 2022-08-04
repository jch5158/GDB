#pragma once

#define REQ_ECHO 1

class CEcho : public CNetServer
{
public:

	CEcho()
	{
	}

	~CEcho()
	{
	}

	void OnClientJoin(UINT64 sessionID) final;

	void OnClientLeave(UINT64 sessionID) final;

	void OnRecv(UINT64 sessionID, CMessage* pMessage) final;

	BOOL OnConnectionRequest(const WCHAR* userIP, WORD userPort) final;

	void OnError(DWORD errorCode,const WCHAR* errorMessage) final;

	void OnStop(void) final;

	void OnTimeout(UINT64 sessionID) final;

private:

	void packingLoginMessage(CMessage* pMessage);

	BOOL procRecvMessage(UINT64 sessionID, DWORD messageType, CMessage* pMessage);

	BOOL procEchoMessage(UINT64 sessionID, CMessage* pMessage);

	void packingEchoMessage(CMessage* pMessage, UINT64 payload);

};

