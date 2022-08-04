#pragma once

#define REQ_ECHO 1


namespace conntents
{
	void packingEchoMessage(CMessage* pMessage, long long payload);

	void packingLoginMessage(CMessage* pMessage, long long payload);
}

class CContents : public CNetServer
{
public:

	CContents() = default;
	virtual ~CContents() = default;

	CContents(const CContents&) = delete;
	CContents& operator=(const CContents&) = delete;

	virtual bool OnStart(void) final;
	virtual void OnStop(void) final;
	virtual void OnStartAcceptThread(void) final;
	virtual void OnStartWorkerThread(void) final;
	virtual void OnCloseAcceptThread(void) final;
	virtual void OnCloseWorkerThread(void) final;
	virtual bool OnConnectionRequest(const wchar_t* userIP, unsigned short userPort) final;   // accept 지후 호출
	virtual void OnClientJoin(unsigned long long sessionID) final;
	virtual void OnClientLeave(unsigned long long sessionID) final;
	virtual void OnRecv(unsigned long long sessionID, CMessage* pMessage) final;	
	virtual void OnError(unsigned int errorCode, const wchar_t* errorMessage) final;

private:


	bool packetProcedure(unsigned long long sessionID, unsigned short messageType, CMessage* pMessage);

	void loginResponse(unsigned long long sessionID);

	bool echoRequest(UINT64 sessionID, CMessage* pMessage);
	void echoResponse(unsigned long long sessionID, unsigned long long payload);
};

