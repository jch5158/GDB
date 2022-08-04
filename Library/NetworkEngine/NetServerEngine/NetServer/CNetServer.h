#pragma once


// ���� 2BYTE�� index ���� 6BYTE�� userID�� ����Ѵ�.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) ������ ���ؼ� ���� 2BYTE�� index�� ���س���.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 

// wsabuf ���̴� �ִ� 200���� �����Ѵ�.

namespace netserver
{
	constexpr int MAX_WSABUF_SIZE = 200;
}

class CNetServer
{
private:

	struct stSessionState
	{
		unsigned int referenceCount;

		// ��� == 0, �̻�� == 1
		unsigned int bUseFlag;
	};

	class stSession
	{
	public:
		stSession(void);
		~stSession(void) = default;

		stSession(const stSession&) = delete;
		stSession& operator=(const stSession&) = delete;

		// Interlocked ����ϴ� �������� ���� ĳ�ö��ο� ���� �ʵ��� �Ѵ�.
		__declspec(align(64)) bool             mbSendFlag;
		__declspec(align(64)) stSessionState   mSessionState;

		// ���� ����
		SOCKET          mSocket;
		SOCKADDR_IN     mClientAddr;

		// �񵿱� I/O OVERAPPED
		OVERLAPPED      mRecvOverlapped;
		OVERLAPPED      mSendOverlapped;

		// ���� ���� ����
		unsigned long long    mSessionID;
		unsigned long long    mIndex;              // ���� index
		unsigned int          mRecvTime;      	   // timeout�� ���� recvTime ��
		bool                  mbDisconnectFlag;    // disconnect flag

		// ��.���� �����̳�
		CRingBuffer                 mRecvRingBuffer;
		CLockFreeQueue<CMessage*>   mSendQueue;


		// �۽� �� ��Ŷ ��ȯ�� ������
		int         mMessageCount;
		CMessage*   mSendCompletionArray[netserver::MAX_WSABUF_SIZE];
	};


public:
	
	CNetServer(void);

	virtual ~CNetServer(void);

	CNetServer(const CNetServer&) = delete;
	CNetServer& operator=(const CNetServer&) = delete;

	virtual bool OnStart(void) = 0;	
	virtual void OnStop(void) = 0;
	virtual void OnStartAcceptThread(void) = 0;
	virtual void OnStartWorkerThread(void) = 0;
	virtual void OnCloseAcceptThread(void) = 0;
	virtual void OnCloseWorkerThread(void) = 0;
	virtual bool OnConnectionRequest(const wchar_t* userIP, unsigned short userPort) = 0;   // accept ���� ȣ��
	virtual void OnClientJoin(unsigned long long sessionID) = 0;
	virtual void OnClientLeave(unsigned long long sessionID) = 0;	
	virtual void OnRecv(unsigned long long sessionID, CMessage* pMessage) = 0;
	virtual void OnError(unsigned int errorCode, const wchar_t* errorMessage) = 0;

	
	bool Start(const wchar_t* pServerIP, unsigned short serverPort, int maxMessageSize, bool bNagleFlag, bool bTimeout,
		unsigned char headerCode, unsigned char staticKey, unsigned int timeoutSec,
		int runningThreadCount, int workerThreadCount, int maxClientCount);
	bool Stop(void);
	

	// ���� ���� �� Ȯ��
	const wchar_t* GetServerBindIP(void) const;
	unsigned short GetServerBindPort(void) const;
	bool GetNagleFlag(void) const;
	int GetMaxClientCount(void) const;
	int GetRunningThreadCount(void) const;
	int GetWorkerThreadCount(void) const;


	// ���� ��Ʈ��ũ �Լ�
	bool SendPacket(unsigned long long sessionID, CMessage* pMessage);
	bool Disconnect(unsigned long long sessionID);
	bool GetConnectionState(unsigned long long sessionID);



	// ���� ������ ����
	unsigned long long GetAcceptTotal(void) const;
	long GetCurrentClientCount(void) const;


	// Ŭ���̾�Ʈ ����
	bool GetClientIP(unsigned long long sessionID, wchar_t* pClientIP, int bufferCch);
	bool GetClientPort(unsigned long long sessionID, unsigned short* pClientPort);


	// TPS ���� ��������
	long GetAcceptTPS(void) const;
	long GetRecvTPS(void) const;
	long GetSendTPS(void) const;

	
	// TPS �ʱ�ȭ
	void InitializeTPS(void);


	// WorkerThread ���� ����
	int GetWakeupPerSecond(void);
	int GetMaxWakeupPerSecond(void) const;
	unsigned int GetWakeupProcessTime(void);
	unsigned int GetMaxWakeupProcessTime(void) const;
	unsigned int GetMaxWakeupWaitTime(void);

private:

	void associateSocketWithIOCP(stSession* pSession);      // ���ϰ� IOCP �ڵ� ����

	bool initializeSession(SOCKET clientSocket, SOCKADDR_IN clientAddr);  // ���� �ʱ�ȭ

	// ���� ȹ�� ���� �Լ�
	stSession* findSession(unsigned long long sessionID);
	stSession* acquireSession(unsigned long long sessionID);
	void leaveSession(stSession* pSession);

	bool setupNetwork(void);
	bool setupSocket(void);
	bool bindSocket(void);
	bool listenSocket(void);
	bool setupIOCP(void);
	bool setupSessionArray(void);
	void setupThread(void);
	void setupAcceptThread(void);
	void setupWorkerThreads(void);
	void setupTimeoutThread(void);

	void waitServerStartEvent(void);       // ���� ������ �Ϸ�Ǹ� return

	// ���� ���� �� ȣ���ϴ� �Լ�
	bool closeAcceptThread(void);
	void kickSessions(void);
	void closeTimeoutThread(void);
	void closeWorkerThreads(void);
	void cleanupReleaseStack(void);
	void closeIOCP(void);


	void releaseSession(stSession* pSession);
	void clearSendQueue(CLockFreeQueue<CMessage*> *pSendQueue);
	void clearSendCompletionArray(stSession *pSession);


	// send, recv
	void sendPost(stSession* pSession);
	void recvPost(stSession* pSession);

	void checkCompletionSend(stSession* pSession);
	void checkCompletionRecv(stSession* pSession, unsigned long transferred);

	static unsigned __stdcall ExecuteAcceptThread(void* pParam);
	static unsigned __stdcall ExecuteWorkerThread(void* pParam);
	static unsigned __stdcall ExecuteTimeoutThread(void* pParam);

	void AcceptThread(void);
	void WorkerThread(void);
	void TimeoutThread(void);
	void SendThread(void);

	// �α� �Ǵ�
	bool isLoggingErrorCode(unsigned int errorCode) const;


	// ���� ������ ������
	unsigned long long         mAcceptTotal;
	long                       mCurrentClientCount;


	 
	long                          mWorkerThreadIndex;            // WorkerThread �������� index
	__declspec(align(64)) long    mWakeupPerSecond;              // ��� IOCP WorkerThread�� �ʴ� �Ͼ�� Ƚ��	
	int                           mMaxWakeupPerSecond;           // IOCP WorkerThread�� �ʴ� ���� ���� �Ͼ Ƚ��
	unsigned int                  mMaxWakeupProcessTime;         // �� �� �Ͼ�� �Ϸ������� ���� �������� ó���ϴ� �ð�
	unsigned int*                 mpWakeupProcessTimeArray;      // �� �� �Ͼ�� �Ϸ������� ó���ϴ� �ð� 
	unsigned int*                 mpWakeupWaitTimeArray;         // �Ϸ������� ó���ϱ� ���� ����� ��ٸ��� �ð�


	// ���� TPS
	__declspec(align(64)) long    mAcceptTPS;
	__declspec(align(64)) long    mSendTPS;
	__declspec(align(64)) long    mRecvTPS;


	// ���� �����̳�
	stSession*             mSessionArray;        // Session �迭
	CLockFreeStack<unsigned long long> mReleaseSessionStack; // ��밡���� Session index �����ϴ� ����, 


	// ���� ���� ��
	std::wstring      mServerIP;
	unsigned short    mServerPort;
	int               mMaxClientCount;       // �ִ� ������ ��
	int               mMaxMessageSize;       // �޽��� �ִ� ũ��
	int               mWorkerThreadCount;    // ��Ŀ ������ ����
	int               mRunningThreadCount;   // IOCP ���� ������ ����
	unsigned int      mTimeoutSec;      	 // timeout �ð�
	bool              mbTimeout;
	bool              mbNagleFlag;           // ���̱� �˰��� on/off �÷���

	// IOCP
	HANDLE mIocpHandle;
	SOCKET mListenSocket;    // ���� ����


	// ���� ���� �� �ñ׳�
	HANDLE            mServerStartEvent;
	

	// ���� ���� �� ���
	bool              mbSessionClearFlag;      // stop() ȣ���� �� session�� �����ϱ� ���� ����ϴ� �÷���
	HANDLE            mSessionClearEvent;      // ��� Session�� ������� Disconnect() ȣ�� �� ������ ��� release �Ǿ��ٸ� �ñ׳�
					  
					  
	// thread Headle  
	HANDLE            mAcceptThreadHandle;
	HANDLE            mTimeoutThreadHandle;
	HANDLE*           mpWorkerThreadHandles;
					  
					  
	// Thread ID	  
	unsigned int      mAcceptThreadID;         
	unsigned int      mTimeoutThreadID;
	unsigned int*     mpWorkerThreadsID;        // ��Ŀ ������ ID �迭
};

