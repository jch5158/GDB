#pragma once



enum class eChatMessageType
{
	//------------------------------------------------------
    // Chatting Server
    //------------------------------------------------------
	ChatServerMessage,

	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]			    // null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];		// 인증토큰
	//	}
	//
	//------------------------------------------------------------
	LoginRequestMessage,


	//------------------------------------------------------------
    // 채팅서버 로그인 응답
    //
    //	{
    //		WORD	Type
    //
    //		BYTE	Status				// 0:실패	1:성공
    //		INT64	AccountNo
    //	}
    //
    //------------------------------------------------------------
	LoginResponseMessage,


	//------------------------------------------------------------
    // 채팅서버 섹터 이동 요청
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
    // 채팅서버 섹터 이동 결과
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
    // 채팅서버 채팅보내기 요청
    //
    //	{
    //		WORD	Type
    //
    //		INT64	AccountNo
    //		WORD	MessageLen
    //		WCHAR	Message[MessageLen / 2]		// null 미포함
    //	}
    //
    //------------------------------------------------------------	
	ChatRequestMessage,

	//------------------------------------------------------------
    // 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
    //
    //	{
    //		WORD	Type
    //
    //		INT64	AccountNo
    //		WCHAR	ID[20]				// null 포함
    //		WCHAR	Nickname[20]			// null 포함
    //		
    //		WORD	MessageLen
    //		WCHAR	Message[MessageLen / 2]		// null 미포함
    //	}
    //
    //------------------------------------------------------------
	ChatResponseMessage,

	//------------------------------------------------------------
	// 하트비트
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// 클라이언트는 이를 30초마다 보내줌.
	// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
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
	GameServerRun = 10,							// GameServer 실행 여부 ON / OFF
	GameServerCPU,								// GameServer CPU 사용률
	GameServerMemory,							// GameServer 메모리 사용 MByte
	GameServerSessionCount,						// 게임서버 세션 수 (컨넥션 수)
	GameServerAuthModePlayerCount,				// 게임서버 AUTH MODE 플레이어 수
	GameServerGameModePlayerCount,				// 게임서버 GAME MODE 플레이어 수
	GameServerAcceptTPS,						// 게임서버 Accept 처리 초당 횟수
	GameServerRecvTPS,							// 게임서버 패킷처리 초당 횟수
	GameServerSendTPS,							// 게임서버 패킷 보내기 초당 완료 횟수
	GameServerDBWriteTPS,						// 게임서버 DB 저장 메시지 초당 처리 횟수
	GameServerDBWriteQueueUseSize,				// 게임서버 DB 저장 메시지 큐 개수 (남은 수)
	GameServerAuthThreadFPS,					// 게임서버 AUTH 스레드 초당 프레임 수 (루프 수)
	GameServerGameThreadFPS,					// 게임서버 GAME 스레드 초당 프레임 수 (루프 수)
	GameServerMessagePoolUseSize,				// 게임서버 패킷풀 사용량

	ChatServerRun = 30,							// 에이전트 ChatServer 실행 여부 ON / OFF
	ChatServerCPU,								// 에이전트 ChatServer CPU 사용률
	ChatServerMemory,							// 에이전트 ChatServer 메모리 사용 MByte
	ChatServerSessionCount,						// 채팅서버 세션 수 (컨넥션 수)
	ChatServerPlayerCount,						// 채팅서버 인증성공 사용자 수 (실제 접속자)
	ChatServerUpdateTPS,						// 채팅서버 UPDATE 스레드 초당 초리 횟수
	ChatServerMessagePoolUseSize,				// 채팅서버 패킷풀 사용량
	ChatServerJobPoolUseSize,					// 채팅서버 UPDATE MSG 풀 사용량

	ServerCPU = 40,								// 서버컴퓨터 CPU 전체 사용률
	ServerNonpagedMemory,						// 서버컴퓨터 논페이지 메모리 MByte
	ServerRecvBytes,							// 서버컴퓨터 네트워크 수신량 KByte
	ServerSendBytes,							// 서버컴퓨터 네트워크 송신량 KByte
	ServerAvailableMemory						// 서버컴퓨터 사용가능 메모리
};


enum class eMonitoringServerLoginResponseType
{
	LoginSuccess = 1,
	ServerNotFountError,
	SessionKeyError
};
