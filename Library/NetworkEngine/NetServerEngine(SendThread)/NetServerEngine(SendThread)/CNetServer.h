#pragma once


// ���� 2BYTE�� index ���� 6BYTE�� userID�� ����Ѵ�.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) ������ ���ؼ� ���� 2BYTE�� index�� ���س���.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 

// wsabuf ���̴� �ִ� 200���� �����Ѵ�.
constexpr int MAX_WSABUF_SIZE = 200;

extern UINT64 gAcceptTotal;

extern DWORD gUpdateTPS;

extern DWORD gAcceptTPS;

extern DWORD gSendTPS;

extern __declspec(align(64)) DWORD gRecvTPS;

//extern DWORD gSectorPlayer;
//
//extern DWORD gSendPacket;
//
//extern DWORD gSendPacketAround;

class CNetServer
{
public:

	struct stSessionState
	{
		DWORD referenceCount;

		// ��� == 0, �̻�� == 1
		DWORD bUseFlag;
	};

	class __declspec(align(64)) stSession
	{
	public:
		stSession(void)	
			: mbDisconnectFlag(FALSE)
			, mSessionID(0)
			, mIndex(0)
			, mSocket(INVALID_SOCKET)
			, mClientAddr{ 0, }
			, mbSendFlag(FALSE)
			, mMessageCount(0)
			, mSessionState{ 0,1 }
			, mSendCompletionArray{ 0, }
			, mSendQueue()
			, mRecvRingBuffer()
			, mSendOverlapped{ 0, }
			, mRecvOverlapped{ 0, }
		{
			// ������ Session Index ��ȣ
			static UINT64 sessionIndex = 0;

			mIndex = sessionIndex;

			++sessionIndex;
		}

		~stSession(void)
		{

		}

		BOOL mbDisconnectFlag;										// 8
		UINT64 mSessionID;											// 8
		UINT64 mIndex;												// 8
		SOCKET mSocket;												// 8
		SOCKADDR_IN mClientAddr;									// 16 

		// 48
		// 16 ĳ�� ����

		__declspec(align(64)) OVERLAPPED mSendOverlapped;
		OVERLAPPED mRecvOverlapped;

		// Interlocked ����ϴ� �������� ���� ĳ�ö��ο� ���� �ʵ��� �Ѵ�.
		__declspec(align(64)) BOOL mbSendFlag;						// 4
		__declspec(align(64)) stSessionState mSessionState;			// 8
	
		DWORD mMessageCount;										// 4	

		CRingBuffer mRecvRingBuffer;								// 24
		CLockFreeQueue<CMessage*> mSendQueue;                       // 96
		CMessage *mSendCompletionArray[MAX_WSABUF_SIZE];			// 160
	};

public:
	
	CNetServer(void);

	virtual ~CNetServer(void);
	
	virtual void OnClientJoin(UINT64 sessionID) = 0;

	virtual void OnClientLeave(UINT64 sessionID) = 0;
	
	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) = 0;
	
	// accept ���� �ٷ� ȣ��
	virtual BOOL OnConnectionRequest(const WCHAR* userIP, WORD userPort) = 0;

	virtual void OnError(DWORD errorCode, const WCHAR* errorMessage) = 0;

	virtual void OnTimeout(UINT64 sessionID) = 0;

	virtual void OnStop(void) = 0;

	//���� IP / ��Ʈ / ��Ŀ������ ��(������, ���׼�) / ���ۿɼ� / �ִ������� ��
	BOOL Start(const WCHAR* serverIP, DWORD serverPort, BOOL bNagleFlag, BOOL bTimeoutFlag, BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, DWORD workerThreadCount, DWORD maxClientCount);

	BOOL Stop(void);
	
	inline BOOL SendPacket(UINT64 sessionID, CMessage* pMessage)
	{
		stSession* pSession = acquireSession(sessionID);
		if (pSession == nullptr)
		{
			//CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"[SendPacket] Client Not Found, sessionID : %llx", sessionID);

			return FALSE;
		}

		// ��� ���� �׸��� üũ���� ���� Encoding
		pMessage->encode();

		// SendQ�� Enqueue �ϴ� ���� �ٸ� �����忡�� CMessage�� ���� ������ ����������. �׷��� ������ referenceCount�� ������Ų��.
		pMessage->AddReferenceCount();

		pSession->mSendQueue.Enqueue(pMessage);

		sendPost(pSession);

		++gSendTPS;

		leaveSession(pSession);

		return TRUE;
	}

	inline BOOL Disconnect(UINT64 sessionID);

	BOOL GetTimeoutFlag(void);

	BOOL GetNagleFlag(void);

	DWORD GetClientCount(void);

	DWORD GetMaxClientCount(void);

	DWORD GetRunningThreadCount(void);

	DWORD GetWorkerThreadCount(void);
	

private:

	inline stSession* createSession(SOCKET clientSocket, SOCKADDR_IN clientAddr);

	inline stSession* findSession(UINT64 sessionID);

	inline stSession* acquireSession(UINT64 sessionID);

	inline void leaveSession(stSession* pSession);

	BOOL setupNetwork(void);

	BOOL setupSocket(void);

	BOOL bindSocket(void);

	BOOL listenSocket(void);

	BOOL setupIOCP(void);

	BOOL setupSessionArray(void);

	void setupAcceptThread(void);

	void setupWorkerThread(void);

	void setupTimoutThread(void);

	void setupThread(void);

	BOOL closeAcceptThread(void);

	void cleanupWorkerThread(void);


	inline void closeTimeoutThread(void);

	inline void releaseSession(stSession* pSession);

	inline void recvPost(stSession* pSession);

	inline void sendPost(stSession* pSession);

	inline void cleanupSessionArray(void);

	inline void checkCompletionRecv(stSession* pSession, DWORD transferred);

	inline void checkCompletionSend(stSession* pSession);

	static DWORD WINAPI WorkerThread(LPVOID lpParam);

	void executeWorkerThread(void);

	static DWORD WINAPI AcceptThread(LPVOID lpParam);

	void executeAcceptThread(void);

	static DWORD WINAPI TimeoutThread(LPVOID lpParam);

	void executeTimeoutThread(void);	

// ======= ���� ���鼭 ���� �ٲ�� ������� ================	

	// ���� ������ ��
	DWORD mCurrentClientCount;

	// ��밡���� Session index �����ϴ� ����, __declspec(align(64))
	CLockFreeStack<UINT64> mReleaseSessionStack;


// ======= ���� ���� �� �� �ٲ��� �ʴ� ������� ============

	// IOCP �ڵ�
	__declspec (align(64)) HANDLE mIocpHandle;

	// ���� ����
	SOCKET mListenSocket;

	// Session �迭
	stSession* mSessionArray;

	BOOL mbTimeoutFlag;

	// stop() ȣ���� �� session�� �����ϱ� ���� ����ϴ� �÷���
	BOOL mbSessionClearFlag;

	// ���̱� �˰��� on/off �÷���
	BOOL mbNagleFlag;

	// ���� ��Ʈ
	DWORD mServerPort;

	// �ִ� ������ ��
	DWORD mMaxClientCount;

	// ��Ŀ ������ ����
	DWORD mWorkerThreadCount;

	// IOCP ���� ������ ����
	DWORD mRunningThreadCount;

	// Accept ������ ID
	DWORD mAcceptThreadID;

	// Timeout ������ ID
	DWORD mTimeoutThreadID;

	// ��� Session�� ������� Disconnect() ȣ�� �� ��� release �Ǿ��ٸ� 
	// �ñ׳η� ����
	HANDLE mSessionClearEvent;
	
	// Accept ������
	HANDLE mAcceptThreadHandle;

	// Timeout ������ �ڵ�
	HANDLE mTimeoutThreadHandle;

	// ��Ŀ ������ �ڵ� �迭
	HANDLE* mpWorkerThreadHandle;

	// ��Ŀ ������ ID �迭
	DWORD* mpWorkerThreadID;

	// ���� IP
	WCHAR mServerIP[MAX_PATH];

};

