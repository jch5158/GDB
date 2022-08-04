#pragma once



enum class eChatMessageType
{
	//------------------------------------------------------
    // Chatting Server
    //------------------------------------------------------
	ChatServerMessage,

	//------------------------------------------------------------
	// ä�ü��� �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]			    // null ����
	//		WCHAR	Nickname[20]		// null ����
	//		char	SessionKey[64];		// ������ū
	//	}
	//
	//------------------------------------------------------------
	LoginRequestMessage,


	//------------------------------------------------------------
    // ä�ü��� �α��� ����
    //
    //	{
    //		WORD	Type
    //
    //		BYTE	Status				// 0:����	1:����
    //		INT64	AccountNo
    //	}
    //
    //------------------------------------------------------------
	LoginResponseMessage,


	//------------------------------------------------------------
    // ä�ü��� ���� �̵� ��û
    //
    //	{
    //		WORD	Type
    //
    //		INT64	AccountNo
    //		WORD	SectorX
    //		WORD	SectorY
    //	}
    //
    //------------------------------------------------------------
	SectorMoveRequestMessage,

	//------------------------------------------------------------
    // ä�ü��� ���� �̵� ���
    //
    //	{
    //		WORD	Type
    //
    //		INT64	AccountNo
    //		WORD	SectorX
    //		WORD	SectorY
    //	}
    //
    //------------------------------------------------------------
	SectorMoveResponseMessage,
	
	//------------------------------------------------------------
    // ä�ü��� ä�ú����� ��û
    //
    //	{
    //		WORD	Type
    //
    //		INT64	AccountNo
    //		WORD	MessageLen
    //		WCHAR	Message[MessageLen / 2]		// null ������
    //	}
    //
    //------------------------------------------------------------	
	ChatRequestMessage,

	//------------------------------------------------------------
    // ä�ü��� ä�ú����� ����  (�ٸ� Ŭ�� ���� ä�õ� �̰ɷ� ����)
    //
    //	{
    //		WORD	Type
    //
    //		INT64	AccountNo
    //		WCHAR	ID[20]				// null ����
    //		WCHAR	Nickname[20]			// null ����
    //		
    //		WORD	MessageLen
    //		WCHAR	Message[MessageLen / 2]		// null ������
    //	}
    //
    //------------------------------------------------------------
	ChatResponseMessage,

	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------	
	HeartbeatRequestMessage
};

enum class eMonitoringMessageType
{
	MonitorServerMessage = 20000,
	LoginSSMessage,
	MonitorDataSSMessage,
	MonitorClientMessage = 25000,
	LoginCSMessage,
	LoginResponseCSMessage,
	MonitorDataCSMessage
};

enum class eMonitorDataType
{
	GameServerRun = 10,							// GameServer ���� ���� ON / OFF
	GameServerCPU,								// GameServer CPU ����
	GameServerMemory,							// GameServer �޸� ��� MByte
	GameServerSessionCount,						// ���Ӽ��� ���� �� (���ؼ� ��)
	GameServerAuthModePlayerCount,				// ���Ӽ��� AUTH MODE �÷��̾� ��
	GameServerGameModePlayerCount,				// ���Ӽ��� GAME MODE �÷��̾� ��
	GameServerAcceptTPS,						// ���Ӽ��� Accept ó�� �ʴ� Ƚ��
	GameServerRecvTPS,							// ���Ӽ��� ��Ŷó�� �ʴ� Ƚ��
	GameServerSendTPS,							// ���Ӽ��� ��Ŷ ������ �ʴ� �Ϸ� Ƚ��
	GameServerDBWriteTPS,						// ���Ӽ��� DB ���� �޽��� �ʴ� ó�� Ƚ��
	GameServerDBWriteQueueUseSize,				// ���Ӽ��� DB ���� �޽��� ť ���� (���� ��)
	GameServerAuthThreadFPS,					// ���Ӽ��� AUTH ������ �ʴ� ������ �� (���� ��)
	GameServerGameThreadFPS,					// ���Ӽ��� GAME ������ �ʴ� ������ �� (���� ��)
	GameServerMessagePoolUseSize,				// ���Ӽ��� ��ŶǮ ��뷮

	ChatServerRun = 30,							// ������Ʈ ChatServer ���� ���� ON / OFF
	ChatServerCPU,								// ������Ʈ ChatServer CPU ����
	ChatServerMemory,							// ������Ʈ ChatServer �޸� ��� MByte
	ChatServerSessionCount,						// ä�ü��� ���� �� (���ؼ� ��)
	ChatServerPlayerCount,						// ä�ü��� �������� ����� �� (���� ������)
	ChatServerUpdateTPS,						// ä�ü��� UPDATE ������ �ʴ� �ʸ� Ƚ��
	ChatServerMessagePoolUseSize,				// ä�ü��� ��ŶǮ ��뷮
	ChatServerJobPoolUseSize,					// ä�ü��� UPDATE MSG Ǯ ��뷮

	ServerCPU = 40,								// ������ǻ�� CPU ��ü ����
	ServerNonpagedMemory,						// ������ǻ�� �������� �޸� MByte
	ServerRecvBytes,							// ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
	ServerSendBytes,							// ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
	ServerAvailableMemory						// ������ǻ�� ��밡�� �޸�
};


enum class eMonitoringServerLoginResponseType
{
	LoginSuccess = 1,
	ServerNotFountError,
	SessionKeyError
};
