#pragma once

namespace mmoserver
{
	// wsabuf ���̴� �ִ� 200���� �����Ѵ�.
	constexpr int MAX_WSABUF_SIZE = 200;
}

class CMMOServer
{
private:

	enum class eSessionMode : int
	{
		NoneMode,             // ���� �̻�� ����
		AuthMode,             // ���� �� ������� ����
		AuthToGameMode,       // ���� ���� ��ȯ
		GameMode,             // ���� �� ���Ӹ�� ����
        LogoutInAuthMode,     // �����غ�
		LogoutInGameMode,     // �����غ�
		WaitLogoutMode        // ���� ����
	};

public:

	CMMOServer(void);
	virtual ~CMMOServer(void);

	CMMOServer(const CMMOServer&) = delete;
	CMMOServer& operator=(const CMMOServer&) = delete;

	class CSession
	{
	public:
		friend class CMMOServer;

		CSession(void);
		virtual ~CSession(void) = default;

		CSession(const CSession&) = delete;
		CSession& operator=(const CSession&) = delete;

		void SendPacket(CMessage* pMessage);
		void Disconnect(void);

		void SetAuthToGame();
		void SetRelease();

		bool GetAuthToGameFlag(void) const;
		bool CheckLogoutInAuth(void) const;
		bool CheckLogoutInGame(void) const;
		void GetClientIP(WCHAR* pIP, DWORD bufferCch) const;
		WORD GetClientPort(void) const;

	private:

		virtual void OnAuthClientJoin(void) = 0;                    // AuthMode �ű� Ŭ���̾�Ʈ ����
		virtual void OnAuthClientLeave(void) = 0;                   // Authentic �����忡�� �������~ �ǹ��̴�.
		virtual void OnAuthMessage(CMessage* pMessage) = 0;         // AuthMode ������ Ŭ����Ʈ ���� �޽��� ó��	
		virtual void OnGameClientJoin(void) = 0;                    // GameMode �ű� Ŭ���̾�Ʈ ����, ���� ������ ������ ���� ������ �غ� �� ���� ( ���� �ʿ� ĳ���� ����, ����Ʈ �غ� ��� )
		virtual void OnGameClientLeave(void) = 0;                   // GameMode Ŭ���̾�Ʈ ����, ���� ���������� �÷��̾� ���� ( ���� �ʿ��� ����, ��Ƽ ���� ��� )
		virtual void OnGameMessage(CMessage* pMessage) = 0;         // GameMode ������ Ŭ���̾�Ʈ ���� �޽��� ó��

		void removeSendCompletionArray(void);
		void removeSendQueue(void);
		void removeRecvCompletionQueue(void);

		//  ���� ���� ����
		eSessionMode        mSessionMode;        // ���� ���
		UINT64              mSessionID;          // ���� ID
		int                 mIndex;              // ���� index
		DWORD               mMsgRecvTime;        // ���������� ��Ŷ�� ���� �ð�
		
		// ���� Io �۾��� ����ϴ� ����
		SOCKET              mSocket;
		SOCKADDR_IN         mClientAddr;
		OVERLAPPED          mSendOverlapped;
		OVERLAPPED          mRecvOverlapped;

		
		// Interlocked ����
		__declspec(align(64)) bool mbSendFlag;
		__declspec(align(64)) long mIoCount;


		// ���� ������� �� ����ϴ� �÷��� 
		bool mbAuthToGameFlag;           // AuthToGame���� ��ȯ�� �� ���                               
		bool mbDisconnectFlag;           // Disconnect ȣ�� ��                            
		bool mbLogoutFlag;               // IoCount�� 0�� �� ��
		bool mbReleaseFlag;              // ������������ �� �̻� �÷��̾ ���� ������ ������� ���� �� ��μ� releaseSession()�� ȣ���Ѵ�. ex) �ش� �÷��̾� DB �۾��� ������ ��


		int mMessageCount;
		CMessage*                 mSendCompletionArray[mmoserver::MAX_WSABUF_SIZE];  // �۽ſ�û�� ��Ŷ ���� �迭 
		CRingBuffer               mRecvRingBuffer;                                   // �� ����          	
		CTemplateQueue<CMessage*> mRecvCompletionQueue;                              // Recv �ϼ��� �޽����� �����ϴ� Buffer
		CTemplateQueue<CMessage*> mSendQueue;                                        // Send �� �޽����� �����ϴ� Buffer
	};

	// ���� IP / ��Ʈ / ���̱� �ɼ� / TimeoutFlag / headerCode / staticKey/ runningThread / ��Ŀ������ ��(������, ���׼�) / �ִ������� ��
	bool Start(const WCHAR* pServerIP, WORD serverPort, DWORD sendFrame, DWORD updateFame, DWORD authenticFrame, DWORD maxMessageSize, bool bNagleFlag, 
		BYTE headerCode, BYTE staticKey, DWORD runningThreadCount, 
		DWORD workerThreadCount, DWORD maxClientCount, DWORD timeoutSec);
	bool Stop(void);

	UINT64 GetAcceptTotal(void) const;
	int GetAcceptTPS(void) const;
	int GetUpdateTPS(void) const;
	int GetAuthenticTPS(void) const;
	int GetSendTPS(void) const;
	int GetRecvTPS(void) const;
	int GetMaxClientCount(void) const;
	int GetCurrentClientCount(void) const;
	int GetRunningThreadCount(void) const;
	int GetWorkerThreadCount(void) const;
	const WCHAR* GetServerBindIP(void) const;
	WORD GetServerBindPort(void) const;
	bool GetNagleFlag(void) const;
	int GetAuthClientCount(void) const;
	int GetGameClientCount(void) const;
	DWORD GetWakeupProcessTime(void) const;
	DWORD GetMaxWakeupWaitTime(void) const;
	DWORD GetMaxWakeupProcessTime(void) const;
	int GetMaxWakeupPerSecond(void) const;
	int GetWakeupPerSecond(void);

	void InitializeTPS(void);

private:

	struct stAcceptInfo
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr;
	};

	virtual bool OnStart(void) = 0;
	virtual void OnStartAuthenticThread(void) = 0;
	virtual void OnStartUpdateThread(void) = 0;
	virtual bool OnConnectionRequest(WCHAR* pUserIP, WORD userPort) = 0;   	// accept ���� �ٷ� ȣ��	
	virtual void OnAuthUpdate(void) = 0;                                    // AuthenticThread ���� �ֱ������� ȣ���ϴ� �Լ�
	virtual void OnGameUpdate(void) = 0;                             	    // UpdateThread ���� ������ ó�������� 1 Loop �� ó���ϴ� �Լ��̴�.
	virtual void OnAssociateSessionWithPlayer(CSession **pSession) = 0;     // Player �� Session ������ ���� �������̵� �Լ�	
	virtual void OnReleaseSessionWithPlayer(CSession* pSession) = 0;
	virtual void OnCloseAuthenticThread(void) = 0;
	virtual void OnCloseUpdateThread(void) = 0;
	virtual void OnStop(void) = 0;


	// ������
	static DWORD WINAPI ExecuteAcceptThread(void* pParam);
	static DWORD WINAPI ExecuteAuthenticThread(void* pParam);
	static DWORD WINAPI ExecuteUpdateThread(void* pParam);
	static DWORD WINAPI ExecuteSendThread(void* pParam);
	static DWORD WINAPI ExecuteWorkerThread(void* pParam);
	void AcceptThread(void);
	void AuthenticThread(void);
	void UpdateThread(void);
	void SendThread(void);
	void WorkerThread(void);


	
	//=============================================================	
	// �Ʒ� �Լ��� ȣ��
	bool setupMMOServerNetwork(void);
	bool setupListenSocket(void);        	// ���� ���� ���� �� ����
	bool bindListenSocket(void);            // ���� ���� ���ε�
	bool listenSocket(void);                // ���� ��ȯ
 	bool setupIOCP(void);                   // IOCP ����
	void setupSessionPointerArray(void);    // ���ǵ� ����
	//=============================================================


	//=============================================================
	// ������ ����
	bool setupMMOServerThread(void);
	bool setupAcceptThread(void);           // AcceptThread ����
	bool setupAuthenticThread(void);        // AuthenticThread ����
	bool setupUpdateThread(void);           // UpdateThread ����
	bool setupSendThread(void);             // SendThread ����
	bool setupWorkerThread(void);           // WorkerThread ����
	//=============================================================


	// ���� ����
	void initializeSessions(stAcceptInfo* pAcceptInfoArray, DWORD arraySize);
	void associateSocketWithIOCP(SOCKET socket, void* completionKey);

	// ���� ��ȯ
	void releaseSession(CSession *pSession);

	
	// ��Ʈ��ũ �Լ�
	void recvPost(CSession* pSession);
	void sendPost(CSession* pSession);
	void checkCompletionRecv(CSession* pSession, DWORD transferred);
	void checkCompletionSend(CSession* pSession);


	// AuthThread �Լ�
	void checkAcceptInfo(void);
	void checkMessageOfAuthMode(void);
	void authSessionTimeout(void);
	void checkLogoutFlagOfAuthMode(void);
	void checkLogoutInAuthMode(void);
	void checkLeaveAuthToGameMode(void);


	// UpdateThread �Լ�
	void joinAuthToGameMode(void);
	void checkMessageOfGameMode(void);
	void updateSessionTimeout(void);
	void checkLogoutFlagOfGameMode(void);
	void checkLogoutInGameMode(void);
	void checkWaitLogoutMode(void);


	// ���� ����� ����ϴ� �Լ�
	bool closeAcceptThread(void);
	void closeAutenticThread(void);
	void closeUpdateThread(void);
	void closeSendThread(void);
	void closeWorkerThread(void);
	void cleanupSessionArray(void);
	void closeIOCP(void);
	void kickSessions(void);


	// �α��� �����ڵ����� Ȯ��
	bool isLoggingErrorCode(unsigned int errorCode) const;


	// ���� � ������
	SOCKET                        mListenSocket;           // ���� ����
	std::vector<CSession*>        mSessionArray;           // ���� �迭
	HANDLE                        mIocpHandle;             // IOCP �ڵ�
	CLockFreeStack<int>           mReleaseSessionStack;	   // ��밡�� ���� index
	CTemplateQueue<stAcceptInfo>  mAcceptInfoQueue;        // Accept ����

	
														   
    // ���� ���� ����
	std::wstring   mServerIP;             // ���� ���ε� IP
	WORD           mServerPort;           // ���� ���ε� ��Ʈ
	int            mSendFrame;            // sendFrame
	int            mAuthFrame;            // authFrame
	int            mUpdateFrame;          // updateFrame
	int            mMaxMessageSize;       // �޽��� �ִ� ũ��
	int            mMaxClientCount;       // �ִ� ������ ��
	int            mRunningThreadCount;   // ���� ������ ī��Ʈ
	int            mWorkerThreadCount;    // ��Ŀ ������ ī��Ʈ
	bool           mbNagleFlag;           // nagleFlag
	DWORD          mTimeoutSec;           // Ÿ�Ӿƿ��ð�



	// WorkerThread ���� ���� ������
	__declspec(align(64))
	int        mWorkerThreadIndex;           // �α׸� ���� WorkerThread Index;
	int        mWakeupPerSecond;        	 // ��� WorkerThread�� �ʴ� �Ͼ�� Ƚ��	
	DWORD*     mpWakeupProcessTimeArray;     // WorkerThread�� 1�� ���� �󸶳� ����Ǿ����� ��ձ��� �� ���
	DWORD*     mpWakeupWaitTimeArray; 	     // WorkerThread�� GQCS�� ȣ�� �� �󸶵ڿ� �������
	int        mMaxWakeupPerSecond;		     // �ʴ� ���� ���� �Ͼ Ƚ���� ���ϴ� ��
	DWORD      mMaxWakeupProcessTime;   	 // WorkerThrad�� ���� ���� ���μ����� �ð�



	// ���� ������ ���� ����
	UINT64                    mAcceptTotal;	
	int                       mCurrentClientCount;   // �� ������ ( auth + game )
	int                       mAuthClientCount;      // auth ������ 
	int                       mGameClientCount;      // game ������
	_declspec (align(64)) int mAcceptTPS;
	_declspec (align(64)) int mUpdateTPS;
	_declspec (align(64)) int mAuthenticTPS;
	_declspec (align(64)) int mSendTPS;
	_declspec (align(64)) int mRecvTPS;



	// ���� ���� �� ����ϴ� ����
	bool        mbAutheticThreadFlag;
	bool        mbUpdateThreadFlag;
	bool        mbSendThreadFlag;
	bool        mbSessionClearFlag;
	HANDLE      mSessionClearEvent; 	// ��� Session�� ������� Disconnect() ȣ�� �� ��� release �Ǿ��ٸ� �ñ׳η� ����


	// thread ����
	DWORD       mAcceptThreadID;
	DWORD       mAuthThreadID;
	DWORD       mUpdateThreadID;
	DWORD       mSendThreadID;
	DWORD*      mpWorkerThreadIDArray;          // WorkerThread ID �迭
	HANDLE      mAcceptThreadHandle;
	HANDLE      mAuthenticThreadHandle;
	HANDLE      mUpdateThreadHandle;
	HANDLE      mSendThreadHandle;
	HANDLE*     mpWorkerThreadHandleArray; 	    // WorkerThread Handle �迭
};

