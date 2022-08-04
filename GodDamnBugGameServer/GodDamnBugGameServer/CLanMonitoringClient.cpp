#include "stdafx.h"


CLanMonitoringClient::CLanMonitoringClient(void)
	: mbConnectThreadFlag(FALSE)
	, mbUpdateThreadFlag(FALSE)
	, mbConnectStateFlag(FALSE)
	, mUpdateThreadID(0)
	, mConnectThreadID(0)
	, mServerNo(0)
	, mSessionID(0)
	, mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	, mConnectThreadHandle(INVALID_HANDLE_VALUE)
	, mConnectEvent(CreateEvent(NULL, FALSE, TRUE, nullptr))
	, mpGodDamnBug(nullptr)
{
}

CLanMonitoringClient::~CLanMonitoringClient(void)
{
	closeConnectThread();

	closeUpdateThread();
}

BOOL CLanMonitoringClient::OnStart(void)
{
	if (setupConnectThread() == FALSE)
	{
		return FALSE;
	}

	if (setupUpdateThread() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

void CLanMonitoringClient::OnServerJoin(UINT64 sessionID)
{
	mSessionID = sessionID;

	mbConnectStateFlag = TRUE;

	sendMonitoringServerLogin();

	return;
}

void CLanMonitoringClient::OnServerLeave(UINT64 sessionID)
{
	mbConnectStateFlag = FALSE;

	// 연결이 끊겼으니 시그널로 변경
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


void CLanMonitoringClient::SetServerNo(DWORD serverNo)
{
	mServerNo = serverNo;

	return;
}

void CLanMonitoringClient::SetContentsPtr(CGodDamnBug* pGodDamnBug)
{
	mpGodDamnBug = pGodDamnBug;

	return;
}

BOOL CLanMonitoringClient::GetConnectStateFlag(void) const
{
	return mbConnectStateFlag;
}

DWORD WINAPI CLanMonitoringClient::ExecuteUpdateThread(void* pParam)
{
	CLanMonitoringClient* pLanMonitoringClient = (CLanMonitoringClient*)pParam;

	pLanMonitoringClient->UpdateThread();

	return 1;
}

DWORD WINAPI CLanMonitoringClient::ExecuteConnectThread(void* pParam)
{
	CLanMonitoringClient* pLanMonitoringClient = (CLanMonitoringClient*)pParam;

	pLanMonitoringClient->ConnectThread();

	return 1;
}

void CLanMonitoringClient::UpdateThread(void)
{
	while (mbUpdateThreadFlag == TRUE)
	{
		sendProfileInfo();

		mpGodDamnBug->InitializeTPS();

		mpGodDamnBug->InitializeDBWriteThreadTPS();

		Sleep(1000);
	}

	return;
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
		return FALSE;
	}

	return TRUE;
}

void CLanMonitoringClient::closeUpdateThread(void)
{
	mbUpdateThreadFlag = FALSE;

	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"[CLanMonitoringClient]", L"[closeUpdateThread] WaitForSingleObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mUpdateThreadHandle);

	return;
}


void CLanMonitoringClient::closeConnectThread(void)
{
	if (WaitForSingleObject(mConnectThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"[CLanMonitoringClient]", L"[closeConnectThread] WaitForSingleObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mConnectThreadHandle);

	return;
}

void CLanMonitoringClient::sendMonitoringServerLogin(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringServerLogin(en_PACKET_SS_MONITOR_LOGIN, mServerNo, pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendProfileInfo(void)
{
	if (mbConnectStateFlag == FALSE)
	{
		return;
	}

	CCPUProfiler::GetInstance()->UpdateProcessProfile();

	CHardwareProfiler::GetInstance()->UpdateHardwareProfiler();

	sendGameServerRun();
	
	sendGameServerCPU();
	
	sendGameServerMemory();
	
	sendGameServerSessionCount();
	
	sendGameServerAuthPlayerCount();
	
	sendGameServerGamePlayerCount();
	
	sendGameServerAcceptTPS();
	
	sendGameServerRecvTPS();
	
	sendGameServerSendTPS();
	
	sendGameServerDBWriteTPS();
	
	sendGameServerDBQsize();
	
	sendGameServerAuthThreadFPS();
	
	sendGameServerUpdateThreadFPS();

	sendGameServerPacketPoolSize();

	return;
}

void CLanMonitoringClient::sendGameServerRun(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, TRUE, (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerCPU(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, (INT)CCPUProfiler::GetInstance()->GetProcessTotalPercentage(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}


void CLanMonitoringClient::sendGameServerMemory(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, (INT)CHardwareProfiler::GetInstance()->GetPrivateBytes() / 1000000, (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerSessionCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_SESSION, mpGodDamnBug->GetCurrentClientCount(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerAuthPlayerCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, mpGodDamnBug->GetAuthClientCount(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerGamePlayerCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, mpGodDamnBug->GetGameClientCount(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}


void CLanMonitoringClient::sendGameServerAcceptTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, mpGodDamnBug->GetAcceptTPS(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerRecvTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, mpGodDamnBug->GetRecvTPS(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerSendTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, mpGodDamnBug->GetSendTPS(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}


void CLanMonitoringClient::sendGameServerDBWriteTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, mpGodDamnBug->GetDBWriteThreadTPS(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerDBQsize(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, mpGodDamnBug->GetDBWriteThreadQueueSize(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerAuthThreadFPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, mpGodDamnBug->GetAuthenticTPS(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerUpdateThreadFPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, mpGodDamnBug->GetUpdateTPS(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendGameServerPacketPoolSize(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, CMessage::GetAllocCount(), (INT)time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}



void CLanMonitoringClient::packingMonitoringServerLogin(WORD type, DWORD serverNo, CMessage* pMessage)
{
	*pMessage << type << serverNo;

	return;
}


void CLanMonitoringClient::packingMonitoringData(WORD type, BYTE dataType, INT data, INT time, CMessage* pMessage)
{
	*pMessage << type << dataType << data << time;

	return;
}
