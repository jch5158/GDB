#pragma once

// ���� 2BYTE�� index ���� 6BYTE�� userID�� ����Ѵ�.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) ������ ���ؼ� ���� 2BYTE�� index�� ���س���.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 


// wsabuf ���̴� �ִ� 200���� �����Ѵ�.

namespace lanclient
{
	constexpr DWORD MAX_WSABUF_SIZE = 200;

	constexpr DWORD MAX_IP_LENGTH = 100;
}

class CLanClient
{
private:

	struct stSessionState
	{
		DWORD referenceCount;

		// ��� == 0, �̻�� == 1
		DWORD bUseFlag;
	};

	class stSession
	{
	public:

		stSession(void)	
			: mbDisconnectFlag(FALSE)
			, mSessionID(0)
			, mSocket(INVALID_SOCKET)
			, mSendOverlapped{ 0, }
			, mRecvOverlapped{ 0, }
			, mMessageCount(0)
			, mbSendFlag(TRUE)
			, mSessionState{ 0, 1 }
			, mRecvRingBuffer()
			, mSendQueue()
			, mSendCompletionArray{ 0, }
		{
		}

		~stSession(void)
		{
		}

		BOOL mbDisconnectFlag;
		UINT64 mSessionID;
		SOCKET mSocket;

		OVERLAPPED mSendOverlapped;
		OVERLAPPED mRecvOverlapped;
		
		DWORD mMessageCount;

		__declspec(align(64))
		BOOL mbSendFlag;

		__declspec(align(64))
		stSessionState mSessionState;

		CRingBuffer mRecvRingBuffer;								
		CLockFreeQueue<CMessage*> mSendQueue;                       
		CMessage* mSendCompletionArray[lanclient::MAX_WSABUF_SIZE];	

	};

public:
	
	CLanClient(void);

	virtual ~CLanClient(void);

	virtual BOOL OnStart(void) = 0;
	
	virtual void OnServerJoin(UINT64 sessionID) = 0;

	virtual void OnServerLeave(UINT64 sessionID) = 0;
	
	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) = 0;
	
	virtual void OnError(DWORD errorCode, const WCHAR* pErrorMessage) = 0;

	virtual void OnStop(void) = 0;

	//���� IP / ��Ʈ / ��Ŀ������ ��(������, ���׼�) / ���ۿɼ� / �ִ������� ��
	BOOL Start(const WCHAR* serverIP, WORD serverPort, BOOL bNagleFlag ,DWORD runningThreadCount, DWORD workerThreadCount);

	BOOL Stop(void);

	BOOL Connect(void);

	BOOL SendPacket(UINT64 sessionID, CMessage* pMessage);

	BOOL Disconnect(UINT64 sessionID);

	WCHAR* GetConnectIP(void) const;

	WORD GetConnectPort(void) const;

	DWORD GetRunningThreadCount(void) const;

	DWORD GetWorkerThreadCount(void) const;

	BOOL GetConnectionState(void) const;

	BOOL GetNagleFlag(void) const;

private:
	
	BOOL setupNetwork(void);

	BOOL setupSocket(void);
	
	BOOL initializeSession(void);

	void associateSocketWithIOCP(void);

	BOOL setupIOCP(void);

	BOOL setupWorkerThreads(void);

	void cleanupWorkerThreads(void);

	void closeIOCP(void);

	void waitClientStartEvent(void);

	void releaseSession(void);

	void clearSendQueue(void);

	void clearSendCompletionArray(void);

	BOOL serverConnect(void);

	void checkSessionClearEvent(void);

	void recvPost(void);

	void sendPost(void);

	BOOL acquireSession(UINT64 sessionID);

	void leaveSession(void);

	void checkCompleteRecv(DWORD transferred);

	void checkCompleteSend(void);

	static DWORD WINAPI ExecuteWorkerThread(LPVOID lpParam);

	void WorkerThread(void);

	HANDLE mClientStartEvent;

	// IOCP �ڵ�
	HANDLE mIocpHandle;

	HANDLE mSessionClearEvent;

	BOOL mbSessionClearFlag;

	BOOL mbNagleFlag;

	// ���� IP
	WCHAR mServerIP[lanclient::MAX_IP_LENGTH];

	// ���� ��Ʈ
	DWORD mServerPort;

	// ��Ŀ ������ ����
	DWORD mWorkerThreadCount;

	// IOCP ���� ������ ����
	DWORD mRunningThreadCount;
	
	// ��Ŀ ������ �ڵ� �迭
	HANDLE* mpWorkerThreadHandle;

	// ��Ŀ ������ ID �迭
	UINT* mpWorkerThreadID;


	// �������� ����
	stSession mSession;
};

