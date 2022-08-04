#pragma once

// ���� 2BYTE�� index ���� 6BYTE�� userID�� ����Ѵ�.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) ������ ���ؼ� ���� 2BYTE�� index�� ���س���.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 


// wsabuf ���̴� �ִ� 200���� �����Ѵ�.

namespace netclient
{
	constexpr INT MAX_WSABUF_SIZE = 200;
};

class  CNetClient
{
private:

	struct stSessionState
	{
		DWORD referenceCount;

		// ��� == 0, �̻�� == 1
		DWORD bUseFlag;
	};

	__declspec(align(64)) class CSession
	{
	public:

		CSession(void)
			: mSocket(INVALID_SOCKET)
			, mbDisconnectFlag(FALSE)			
			, mbSendFlag(TRUE)
			, mMessageCount(0)
			, mSessionState{ 0, }
			, mSendCompletionArray{ 0, }
			, mSendQueue()
			, mRecvRingBuffer()
			, mSendOverlapped{ 0, }
			, mRecvOverlapped{ 0, }
		{
		}

		~CSession(void)
		{
		}

		SOCKET mSocket;
		BOOL mbDisconnectFlag;

		__declspec(align(64)) 
		OVERLAPPED mSendOverlapped;
		OVERLAPPED mRecvOverlapped;

		INT mMessageCount;											// 4	

		__declspec(align(64))
		BOOL mbSendFlag;											// 4
		
		__declspec(align(64))
		stSessionState mSessionState;

		CRingBuffer mRecvRingBuffer;								// 24
		CLockFreeQueue<CMessage*> mSendQueue;                       // 96
		CMessage* mSendCompletionArray[netclient::MAX_WSABUF_SIZE];			// 160
	};

public:
	
	CNetClient(void);

	virtual ~CNetClient(void);
	
	virtual void OnServerJoin(void) = 0;

	virtual void OnServerLeave(void) = 0;
	
	virtual void OnRecv(CMessage* pMessage) = 0;
	
	virtual void OnError(DWORD errorCode, const WCHAR* errorMessage) = 0;

	virtual void OnTimeout(void) = 0;

	virtual void OnStop(void) = 0;

	//���� IP / ��Ʈ / ��Ŀ������ ��(������, ���׼�) / ���ۿɼ� / �ִ������� ��
	BOOL Start(const WCHAR* serverIP, WORD serverPort, BOOL bNagleFlag, BOOL bTimeoutFlag, DWORD timeoutPeriod, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount);

	BOOL Stop(void);

	inline BOOL SendPacket(CMessage* pMessage);

	inline BOOL Disconnect(void);

	inline BOOL GetTimeoutFlag(void);

	inline DWORD GetRunningThreadCount(void);

	inline DWORD GetWorkerThreadCount(void);

	inline BOOL GetConnectionState(void);

private:
	
	BOOL setupNetwork(void);

	BOOL setupSocket(void);

	void connectServer(void);

	BOOL initializeSession(void);

	void associateSocketWithIOCP(void);
	
	BOOL setupIOCP(void);

	BOOL setupWorkerThreads(void);

	BOOL setupTimoutThread(void);

	void closeWorkerThreads(void);

	void closeIOCP(void);

	void checkSessionConnection(void);

	void closeTimeoutThread(void);

	void releaseSession(void);

	inline void recvPost(void);

	inline void sendPost(void);

	inline BOOL acquireSession(void);

	inline void leaveSession(void);

	inline void checkCompleteRecv(DWORD transferred);

	inline void checkCompleteSend(void);

	static DWORD WINAPI ExecuteWorkerThread(LPVOID pParam);

	void WorkerThread(void);

	static DWORD WINAPI ExecuteTimeoutThread(LPVOID pParam);

	void TimeoutThread(void);


	CSession mSession;

	// IOCP �ڵ�
	__declspec(align(64))
	HANDLE mIocpHandle;

	// Ŀ��Ʈ ����
	//SOCKET mConnectSocket;

		// ��Ŀ ������ �ڵ� �迭
	HANDLE* mpWorkerThreadHandle;

	// ��Ŀ ������ ID �迭
	UINT* mpWorkerThreadID;

	// Timeout ������ �ڵ�
	HANDLE mTimeoutThreadHandle;

	HANDLE mSessionClearEvent;

	// ���� IP
	WCHAR *mServerIP;

	// ���� ��Ʈ
	DWORD mServerPort;

	// ��Ŀ ������ ����
	DWORD mWorkerThreadCount;

	// IOCP ���� ������ ����
	DWORD mRunningThreadCount;

	// Timeout ������ ID
	UINT mTimeoutThreadID;	

	BOOL mbNagleFlag;

	BOOL mbTimeoutFlag;

	DWORD mTimeoutPeriod;
};

