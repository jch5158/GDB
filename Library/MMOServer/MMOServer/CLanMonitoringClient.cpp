#include "stdafx.h"



CLanMonitoringClient::CLanMonitoringClient(void)
	:mbConnectFlag(FALSE)
	,mbUpdateThreadFlag(FALSE)
	,mbConnectThreadFlag(FALSE)
	,mUpdateThreadID(0)
	,mConnectThreadID(0)
	,mServerNo(0)
	,mSessionID(0)
	,mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	,mConnectThreadHandle(INVALID_HANDLE_VALUE)
	,mConnectEvent(CreateEvent(NULL,FALSE,TRUE,nullptr))
	, mpEchoServer(nullptr)
{
}

CLanMonitoringClient::~CLanMonitoringClient(void)
{
	closeUpdateThread();

	closeConnectThread();
}

BOOL CLanMonitoringClient::OnStart(void)
{
	if (setupUpdateThread() == FALSE)
	{
		return FALSE;
	}

	if (setupConnectThread() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}


void CLanMonitoringClient::OnServerJoin(UINT64 sessionID)
{
	mSessionID = sessionID;

	mbConnectFlag = TRUE;

	procedureLoginServer();

	return;
}

void CLanMonitoringClient::OnServerLeave(UINT64 sessionID)
{
	mbConnectFlag = FALSE;

	SetEvent(mConnectEvent);

	return;
}

void CLanMonitoringClient::OnRecv(UINT64 sessionID, CMessage* pMessage)
{

	return;
}

void CLanMonitoringClient::OnError(DWORD errorCode, const WCHAR* errorMessage)
{

	return;
}

void CLanMonitoringClient::OnStop(void)
{
	mbConnectThreadFlag = FALSE;

	SetEvent(mConnectEvent);

	return;
}

void CLanMonitoringClient::SendProfileInfo(void)
{
	CCPUProfiler::GetInstance()->UpdateProcessProfile();

	CHardwareProfiler::GetInstance()->UpdateHardwareProfiler();

	// Server Run
	procedureGameServerRun();

	procedureGameServerCPU();

	procedureGameServerMemory();

	procedureGameSessionCount();

	procedureAuthModePlayerCount();

	procedureGameModePlayerCount();

	procedureAcceptTPS();

	procedureRecvTPS();

	procedureSendTPS();

	procedureDBWriteTPS();

	procedureDBWriteQueueSize();

	procedureAuthThreadFPS();

	procedureGameThreadFPS();

	procedureMessagePoolUseSize();

	return;
}

void CLanMonitoringClient::SetServerNo(DWORD serverNo)
{
	mServerNo = serverNo;

	return;
}

void CLanMonitoringClient::SetEchoServerPtr(CEcho* pEchoServer)
{
	mpEchoServer = pEchoServer;

	return;
}

DWORD WINAPI CLanMonitoringClient::ExecuteUpdateThread(void* pParam)
{
	CLanMonitoringClient* pLanMonitoringServer = (CLanMonitoringClient*)pParam;

	pLanMonitoringServer->UpdateThread();

	return 1;
}

void CLanMonitoringClient::UpdateThread(void)
{

	while (mbUpdateThreadFlag)
	{
		SendProfileInfo();

		mpEchoServer->InitializeTPS();

		Sleep(1000);
	}

	return;
}


DWORD WINAPI CLanMonitoringClient::ExecuteConnectThread(void* pParam)
{
	CLanMonitoringClient* pLanMonitoringServer = (CLanMonitoringClient*)pParam;

	pLanMonitoringServer->ConnectThread();

	return 1;
}

void CLanMonitoringClient::ConnectThread(void)
{
	for (;;)
	{
		// 동기화 객체
		if (WaitForSingleObject(mConnectEvent, INFINITE) != WAIT_OBJECT_0)
		{
			CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"[CLanMonitoringClient]", L"[ConnectThread] WaitForSingleObject Error : %d", GetLastError());

			CCrashDump::Crash();
		}

		for (;;)
		{
			if (mbConnectThreadFlag == FALSE)
			{
				goto THREAD_EXIT;
			}

			if (Connect() == TRUE)
			{			
				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"[CLanMonitoringClient]", L"[ConnectThread] Connect Success");

				break;
			}

			// 1초마다 한 번씩 시도한다.
			Sleep(1000);
		}
	}

THREAD_EXIT:

	CloseHandle(mConnectEvent);

	return;
}


BOOL CLanMonitoringClient::setupUpdateThread(void)
{
	mbUpdateThreadFlag = TRUE;

	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)&mUpdateThreadID);
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringClient", L"[setupUpdateThread] _beginthreadex Error : %d", GetLastError());

		return FALSE;
	}

	return TRUE;
}

BOOL CLanMonitoringClient::setupConnectThread(void)
{
	mbConnectThreadFlag = TRUE;

	mConnectThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteConnectThread, this, NULL, (UINT*)&mConnectThreadID);
	if (mConnectThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringClient", L"[setupConnectThread] _beginthreadex Error : %d", GetLastError());

		return FALSE;
	}

	return TRUE;
}

void CLanMonitoringClient::closeUpdateThread(void)
{
	mbUpdateThreadFlag = FALSE;

	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringClient", L"[closeUpdateThread] WaitForSingObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mUpdateThreadHandle);

	return;
}


void CLanMonitoringClient::closeConnectThread(void)
{
	mbConnectThreadFlag = FALSE;

	if (WaitForSingleObject(mConnectThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringClient", L"[closeConnectThread] WaitForSingObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mConnectThreadHandle);

	return;
}

void CLanMonitoringClient::procedureLoginServer(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_LOGIN << (INT)mServerNo;

	SendPacket(mSessionID, pMessage);

	pMessage->Free();
	return;
}


void CLanMonitoringClient::procedureGameServerRun(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_RUN << (INT)TRUE << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();
	
	return;
}


void CLanMonitoringClient::procedureGameServerCPU(void)
{
	CMessage* pMessage = CMessage::Alloc();
	
	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_CPU << (DWORD)CCPUProfiler::GetInstance()->GetProcessTotalPercentage() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureGameServerMemory(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_MEM << ((INT)CHardwareProfiler::GetInstance()->GetPrivateBytes())/1000000 << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();
	
	return;
}

void CLanMonitoringClient::procedureGameSessionCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_SESSION << mpEchoServer->GetCurrentClientCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();
	
	return;
}

void CLanMonitoringClient::procedureAuthModePlayerCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER << mpEchoServer->GetAuthClientCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureGameModePlayerCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER << mpEchoServer->GetGameClientCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureAcceptTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS << mpEchoServer->GetAcceptTPS() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureRecvTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS << mpEchoServer->GetRecvTPS() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureSendTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS << mpEchoServer->GetSendTPS() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureDBWriteTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS << (INT)0 << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}


void CLanMonitoringClient::procedureDBWriteQueueSize(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG << (INT)0 << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}


void CLanMonitoringClient::procedureAuthThreadFPS(void)
{

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS << mpEchoServer->GetAuthenticTPS() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureGameThreadFPS(void) 
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS << mpEchoServer->GetUpdateTPS() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureMessagePoolUseSize(void)
{
	CMessage* pMessage = CMessage::Alloc();

	//*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_POOL << (INT)CLockFreeObjectFreeList<CTLSLockFreeObjectFreeList<CMessage>::CChunk>::GetAllocNodeCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}