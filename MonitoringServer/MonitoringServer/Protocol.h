#pragma once


//enum class eMessageType
//{
//	MonitorServerMessage = 20000,
//	LoginSSMessage,
//	MonitorDataSSMessage,
//	MonitorClientMessage = 25000,
//	LoginCSMessage,
//	LoginResponseCSMessage,
//	MonitorDataCSMessage
//};
//
//enum class eMonitorDataType
//{
//	GameServerRun = 10,							// GameServer 실행 여부 ON / OFF
//	GameServerCPU,								// GameServer CPU 사용률
//	GameServerMemory,							// GameServer 메모리 사용 MByte
//	GameServerSessionCount,						// 게임서버 세션 수 (컨넥션 수)
//	GameServerAuthModePlayerCount,				// 게임서버 AUTH MODE 플레이어 수
//	GameServerGameModePlayerCount,				// 게임서버 GAME MODE 플레이어 수
//	GameServerAcceptTPS,						// 게임서버 Accept 처리 초당 횟수
//	GameServerRecvTPS,							// 게임서버 패킷처리 초당 횟수
//	GameServerSendTPS,							// 게임서버 패킷 보내기 초당 완료 횟수
//	GameServerDBWriteTPS,						// 게임서버 DB 저장 메시지 초당 처리 횟수
//	GameServerDBWriteQueueUseSize,				// 게임서버 DB 저장 메시지 큐 개수 (남은 수)
//	GameServerAuthThreadFPS,					// 게임서버 AUTH 스레드 초당 프레임 수 (루프 수)
//	GameServerGameThreadFPS,					// 게임서버 GAME 스레드 초당 프레임 수 (루프 수)
//	GameServerMessagePoolUseSize,				// 게임서버 패킷풀 사용량
//
//	ChatServerRun = 30,							// 에이전트 ChatServer 실행 여부 ON / OFF
//	ChatServerCPU,								// 에이전트 ChatServer CPU 사용률
//	ChatServerMemory,							// 에이전트 ChatServer 메모리 사용 MByte
//	ChatServerSessionCount,						// 채팅서버 세션 수 (컨넥션 수)
//	ChatServerPlayerCount,						// 채팅서버 인증성공 사용자 수 (실제 접속자)
//	ChatServerUpdateTPS,						// 채팅서버 UPDATE 스레드 초당 초리 횟수
//	ChatServerMessagePoolUseSize,				// 채팅서버 패킷풀 사용량
//	ChatServerJobPoolUseSize,					// 채팅서버 UPDATE MSG 풀 사용량
//
//	ServerCPU = 40,								// 서버컴퓨터 CPU 전체 사용률
//	ServerNonpagedMemory,						// 서버컴퓨터 논페이지 메모리 MByte
//	ServerRecvBytes,							// 서버컴퓨터 네트워크 수신량 KByte
//	ServerSendBytes,							// 서버컴퓨터 네트워크 송신량 KByte
//	ServerAvailableMemory						// 서버컴퓨터 사용가능 메모리
//};

enum class eProfileDataType
{
	CpuProfileAvr,
	CpuProfieMin,
	CpuProfileMax,
	
	NonpagedMemoryProfileAvr,
	NonpagedMemoryProfileMin,
	NonpagedMemoryProfileMax,
	
	RecvBytesProfileAvr,
	RecvBytesProfileMin,
	RecvBytesProfileMax,
	
	SendBytesProfileAvr,
	SendBytesProfileMin,
	SendBytesProfileMax,
	
	AvailableMemoryProfileAvr,
	AvailableMemoryProfileMin,
	AvailableMemoryProfileMax,

	GameServerCpuProfileAvr,
	GameServerCpuProfileMin,
	GameServerCpuProfileMax,
	
	GameServerMemoryProfileAvr,
	GameServerMemoryProfileMin,
	GameServerMemoryProfileMax,
	
	GameServerSessionCountProfileAvr,
	GameServerSessionCountProfileMin,
	GameServerSessionCountProfileMax,
	
	GameServerAuthPlayerProfileAvr,
	GameServerAuthPlayerProfileMin,
	GameServerAuthPlayerProfileMax,
	
	GameServerGamePlayerProfileAvr,
	GameServerGamePlayerProfileMin,
	GameServerGamePlayerProfileMax,
	
	GameServerAcceptTPSProfileAvr,
	GameServerAcceptTPSProfileMin,
	GameServerAcceptTPSProfileMax,
	
	GameServerRecvTPSProfileAvr,
	GameServerRecvTPSProfileMin,
	GameServerRecvTPSProfileMax,
	
	GameServerSendTPSProfileAvr,
	GameServerSendTPSProfileMin,
	GameServerSendTPSProfileMax,
	
	GameServerDBWriteTPSProfileAvr,
	GameServerDBWriteTPSProfileMin,
	GameServerDBWriteTPSProfileMax,
	
	GameServerDBWriteQSizeProfileAvr,
	GameServerDBWriteQSizeProfileMin,
	GameServerDBWriteQSizeProfileMax,
	
	GameServerAuthFPSProfileAvr,
	GameServerAuthFPSProfileMin,
	GameServerAuthFPSProfileMax,
	
	GameServerGameFPSProfileAvr,
	GameServerGameFPSProfileMin,
	GameServerGameFPSProfileMax,
	
	GameServerMessagePoolProfileAvr,
	GameServerMessagePoolProfileMin,
	GameServerMessagePoolProfileMax,

	ChatServerCpuProfileAvr,
	ChatServerCpuProfileMin,
	ChatServerCpuProfileMax,
    
	ChatServerMemoryProfileAvr,
	ChatServerMemoryProfileMin,
	ChatServerMemoryProfileMax,
    
	ChatServerSessionCountProfileAvr,
	ChatServerSessionCountProfileMin,
	ChatServerSessionCountProfileMax,
    
	ChatServerPlayerCountProfileAvr,
	ChatServerPlayerCountProfileMin,
	ChatServerPlayerCountProfileMax,
    
    ChatServerUpdateTPSProfileAvr,
    ChatServerUpdateTPSProfileMin,
    ChatServerUpdateTPSProfileMax,
    
    ChatServerMessagePoolProfileAvr,
    ChatServerMessagePoolProfileMin,
    ChatServerMessagePoolProfileMax,
    
    ChatServerUpdateQSizeProfileAvr,
    ChatServerUpdateQSizeProfileMin,
    ChatServerUpdateQSizeProfileMax 
};

enum class eLoginResponseType
{
	LoginSuccess = 1,
	ServerNotFountError,
	SessionKeyError
};


