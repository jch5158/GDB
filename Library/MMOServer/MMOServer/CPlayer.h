#pragma once

namespace echoserver
{
	void packingLoginResponse(BYTE loginResult, UINT64 accountNo, CMessage* pMessage);
	void packingEchoResponse(UINT64 accountNo, INT64 sendTick, CMessage* pMessage);
}

class CPlayer : public CMMOServer::CSession
{
public:

	CPlayer(void);

	virtual ~CPlayer(void);

	CPlayer(const CPlayer&) = delete;
	CPlayer& operator=(const CPlayer&) = delete;

private:

	virtual void OnAuthClientJoin(void) final;
	virtual void OnAuthClientLeave(void) final;
	virtual void OnAuthMessage(CMessage* pMessage) final;
	virtual void OnGameClientJoin(void) final;
	virtual void OnGameClientLeave(void) final;
	virtual void OnGameMessage(CMessage* pMessage) final;

	bool authPacketProcedure(CMessage* pMessage);

	bool gamePacketProcedure(CMessage* pMessage);

	bool loginRequest(CMessage* pMessage);
	void loginResponse(void);

	bool echoRequest(CMessage* pMessage);
	void echoResponse(INT64 sendTick);

	UINT64 mAccountNumber;
};


