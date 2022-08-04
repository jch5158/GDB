#pragma once


class CNetMonitoringServer : public CNetServer
{
public:

	CNetMonitoringServer(void);

	~CNetMonitoringServer(void);

	virtual BOOL OnStart(void) final;

	virtual void OnClientJoin(UINT64 sessionID) final;

	virtual void OnClientLeave(UINT64 sessionID) final;

	virtual void OnStartAcceptThread(void) final;

	virtual void OnStartWorkerThread(void) final;

	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) final;
	
	virtual void OnCloseWorkerThread(void) final;

	// accept 직후 바로 호출
	virtual BOOL OnConnectionRequest(const WCHAR* userIP, WORD userPort) final;

	virtual void OnCloseAcceptThread(void) final;

	virtual void OnError(DWORD errorCode, const WCHAR* errorMessage) final;

	virtual void OnStop(void) final;

	void SetLoginKey(CHAR* pLoginKey);

	void SendClients(CMessage* pMessage);

private:

	class CCriticalSection
	{
	public:
		CCriticalSection(CRITICAL_SECTION* pCriticalSection)
			:mpCriticalSection(pCriticalSection)
		{
			EnterCriticalSection(pCriticalSection);
		}

		~CCriticalSection()
		{
			LeaveCriticalSection(mpCriticalSection);
		}

		CRITICAL_SECTION* mpCriticalSection;
	};


	BOOL insertClient(UINT64 sessionID);
	BOOL findClient(UINT64 sessionID);
	void eraseClient(UINT64 sessionID);

	void getGameProfileTableName(WCHAR* pTableName, DWORD bufferCb);
	void getChatProfileTableName(WCHAR* pTableName, DWORD bufferCb);

	BOOL recvProcedure(UINT64 sessionID, WORD messageType, CMessage* pMessage);
	BOOL recvProcedureLoginCS(UINT64 sessionID, CMessage* pMessage);

	void sendLoginResponseCS(UINT64 sessionID, BYTE result);

	void packingLoginResponseCS(BYTE status, CMessage *pMessage);

	CHAR mLoginKey[MAX_PATH];

	CRITICAL_SECTION mClientsCriticalSection;

	std::list<UINT64> mClientList;
};

