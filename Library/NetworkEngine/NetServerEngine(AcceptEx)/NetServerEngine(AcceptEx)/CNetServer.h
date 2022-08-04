#pragma once

// NetServer EX

// ���� 2BYTE�� index ���� 6BYTE�� userID�� ����Ѵ�.
#define dfSESSION_ID(index,userID) ((index << 48) | userID)

// &(and) ������ ���ؼ� ���� 2BYTE�� index�� ���س���.
#define dfSESSION_INDEX(sessionID) (( sessionID & 0xffff000000000000 ) >> 48 ) 


namespace netserver
{

	// wsabuf ���̴� �ִ� 200���� �����Ѵ�.
	constexpr int MAX_WSABUF_SIZE = 200;
}




class CSession;

enum class eOverappedType
{
	Accept,
	Send,
	Recv
};

class COverappedEx :public OVERLAPPED
{
public:

	COverappedEx(void)
		:mOverappedType((eOverappedType)-1)
		,mpSession(nullptr)
	{
	}

	~COverappedEx(void)
	{
	}

	eOverappedType mOverappedType;
	CSession* mpSession;
};

class CSession
{
private:

	friend class CNetServer;

	struct stSessionState
	{
		short referenceCount;

		// ��� == 0, �̻�� == 1
		short bUseFlag;
	};

	CSession(void)
		: mbDisconnectFlag(false)
		, mSessionID(0)
		, mIndex(0)
		, mSocket(socket(AF_INET, SOCK_STREAM, 0))
		, mClientAddr{ 0, }
		, mbSendFlag(false)
		, mMessageCount(0)
		, mSessionState{ 0,1 }
		, mSendCompletionArray{ 0, }
		, mSendQueue()
		, mRecvRingBuffer()
		, mAcceptOverlapped()
		, mSendOverlapped()
		, mRecvOverlapped()
	{
		// ������ Session Index ��ȣ
		static UINT64 sessionIndex = 0;

		ZeroMemory(&mAcceptOverlapped, sizeof(COverappedEx));

		mAcceptOverlapped.mOverappedType = eOverappedType::Accept;

		mAcceptOverlapped.mpSession = this;

		mRecvOverlapped.mOverappedType = eOverappedType::Recv;

		mRecvOverlapped.mpSession = this;

		mSendOverlapped.mOverappedType = eOverappedType::Send;

		mSendOverlapped.mpSession = this;

		mIndex = sessionIndex;

		++sessionIndex;
	}

	~CSession(void)
	{

	}

	bool mbDisconnectFlag;		
	UINT64 mSessionID;			
	UINT64 mIndex;				
	SOCKET mSocket;				
	SOCKADDR_IN mClientAddr;	

	char mBuffer[BUFSIZ];

	COverappedEx mAcceptOverlapped;
	COverappedEx mSendOverlapped;
	COverappedEx mRecvOverlapped;

	// Interlocked ����ϴ� �������� ���� ĳ�ö��ο� ���� �ʵ��� �Ѵ�.
	__declspec(align(64)) bool mbSendFlag;						
	__declspec(align(64)) stSessionState mSessionState;			

	int mMessageCount;											

	CRingBuffer mRecvRingBuffer;								
	CLockFreeQueue<CMessage*> mSendQueue;                       
	CMessage* mSendCompletionArray[netserver::MAX_WSABUF_SIZE];			
};


class CNetServer
{
public:
	
	CNetServer(void);

	virtual ~CNetServer(void);
	
	virtual BOOL OnStart(void) = 0;

	virtual void OnClientJoin(UINT64 sessionID) = 0;

	virtual void OnClientLeave(UINT64 sessionID) = 0;
	
	virtual void OnRecv(UINT64 sessionID, CMessage* pMessage) = 0;
	
	// accept ���� �ٷ� ȣ��
	virtual bool OnConnectionRequest(const WCHAR* userIp, WORD userPort) = 0;

	virtual void OnError(int errorCode, const WCHAR* errorMessage) = 0;

	virtual void OnStop(void) = 0;


	virtual void OnStartWorkerThread(void) = 0;

	virtual void OnCloseWorkerThread(void) = 0;




	//���� IP / ��Ʈ / ��Ŀ������ ��(������, ���׼�) / ���ۿɼ� / �ִ������� ��
	bool Start(const WCHAR* serverIp, unsigned short serverPort, int maxMessageSize, bool bNagleFlag, BYTE headerCode, BYTE staticKey, int runningThreadCount, int workerThreadCount,int maxClient);

	bool Stop(void);
	
	bool SendPacket(UINT64 sessionID, CMessage* pMessage);

	bool Disconnect(UINT64 sessionID);

	bool GetNagleFlag(void);

	long GetCurrentClientCount(void);

	long GetMaxClientCount(void);

	long GetRunningThreadCount(void);

	long GetWorkerThreadCount(void);

	UINT64 GetAcceptTotal(void) const;

	DWORD GetAcceptTPS(void) const;

	DWORD GetRecvTPS(void) const;

	DWORD GetSendTPS(void) const;


	void InitializeTPS(void);


	DWORD GetWakeupProcessTime(void);

	DWORD GetWakeupWaitTime(void);

	DWORD GetMaxWakeupProcessTime(void) const;

	LONG GetWakeupPerSecond(void);

	LONG GetMaxWakeupPerSecond(void) const;

	LONG GetWakeupOnRecvPerSecond(void);

	LONG GetMaxWakeupOnRecvPerSecond(void) const;


private:

	void createSession(CSession* pSession);

	CSession* findSession(UINT64 sessionID);

	CSession* acquireSession(UINT64 sessionID);

	void leaveSession(CSession* pSession);

	bool setupNetwork(void);

	bool setupWsa(void);

	bool setupSocket(void);

	bool setupClientSocket(SOCKET clientSocket);

	bool bindSocket(void);

	bool listenSocket(void);

	bool setupIOCP(void);

	bool setupSessionArray(void);


	void setupWorkerThread(void);

	void setupThread(void);

	void cleanupWorkerThread(void);

	void acceptPost(CSession* pSession);

	void releaseSocket(CSession* pSession);

	void* GetSockExtAPI(SOCKET sock, GUID guid);

	void releaseSession(CSession* pSession);

	void recvPost(CSession* pSession);

	void sendPost(CSession* pSession);

	void cleanupSessionArray(void);

	void completeAccept(CSession* pSession);

	void checkCompleteRecv(CSession* pSession, DWORD transferred);

	void checkCompleteSend(CSession* pSession, DWORD transferred);

	static unsigned __stdcall ExecuteWorkerThread(LPVOID lpParam);

	void WorkerThread(void);

	__declspec(align(64)) 
	UINT64 mAcceptTotal;

	__declspec(align(64)) 
	DWORD mAcceptTPS;

	__declspec(align(64))
	DWORD mSendTPS;

	__declspec(align(64))
	DWORD mRecvTPS;



	// IOCP WorkerThread�� �ʴ� �Ͼ�� Ƚ��	
	__declspec(align(64))
	LONG mWakeupPerSecond;

	// IOCP WorkerThread�� �ʴ� OnRecv �� ȣ���ϴ� Ƚ��
	__declspec(align(64))
	LONG mWakeupOnRecvPerSecond;

	LONG mWorkerThreadIndex;

	DWORD mWakeupProcessTime[15];

	DWORD mWakeupWaitTime[15];

	LONG mMaxWakeupPerSecond;

	LONG mMaxWakeupOnRecvPerSecond;

	DWORD mMaxWakeupProcessTime;





	// ���� ������ ��
	long mCurrentClientCount;
	
	int mMaxMessageSize;

	// IOCP �ڵ�
	HANDLE mIocpHandle;

	// ���� ����
	SOCKET mListenSocket;

	// Session �迭
	CSession* mSessionArray;
	

	// stop() ȣ���� �� session�� �����ϱ� ���� ����ϴ� �÷���
	bool mbSessionClearFlag;

	// ���̱� �˰��� on/off �÷���
	bool mbNagleFlag;

	// ���� ��Ʈ
	unsigned short mServerPort;

	// �ִ� ������ ��
	long mMaxClientCount;


	// ��Ŀ ������ ����
	long mWorkerThreadCount;

	// IOCP ���� ������ ����
	long mRunningThreadCount;


	// ��� Session�� ������� Disconnect() ȣ�� �� ��� release �Ǿ��ٸ� 
	// �ñ׳η� ����
	HANDLE mSessionClearEvent;
	

	// ��Ŀ ������ �ڵ� �迭
	HANDLE* mpWorkerThreadHandle;

	// ��Ŀ ������ ID �迭
	unsigned int* mpWorkerThreadID;

	// ���� IP
	WCHAR mServerIP[MAX_PATH];

};

