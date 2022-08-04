#include "stdafx.h"

CLanMonitoringClient::CLanMonitoringClient(void)
	: mbConnectThreadFlag(TRUE)
	, mbUpdateThreadFlag(TRUE)
	, mbConnectFlag(FALSE)
	, mUpdateThreadID(0)
	, mConnectThreadID(0)
	, mServerNo(0)
	, mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	, mConnectThreadHandle(INVALID_HANDLE_VALUE)
	, mConnectEvent(CreateEvent(NULL, TRUE, TRUE, nullptr))
	, mpChatServer(nullptr)
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

	return;
}

void CLanMonitoringClient::SendProfileInfo(void)
{
	if (mbConnectFlag == FALSE)
	{
		return;
	}

	CCPUProfiler::GetInstance()->UpdateProcessProfile();

	CHardwareProfiler::GetInstance()->UpdateHardwareProfiler();

	procedureChatServerRun();

	procedureChatServerCPU();

	procedureChatServerMemory();

	procedureChatServerSessionCount();

	procedureChatServerPlayerCount();

	procedureChatServerUpdateTPS();

	procedureChatServerMessagePool();

	procedureChatServerJobPool();

	return;
}

void CLanMonitoringClient::SetServerNo(DWORD serverNo)
{

	mServerNo = serverNo;

	return;
}

void CLanMonitoringClient::SetContentsPtr(CChattingServer* pEchoServer)
{
	mpChatServer = pEchoServer;

	return;
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
		SendProfileInfo();

		wprintf_s(L"Accept Total : %lld\nAccept TPS : %d\nUpdate TPS : %d\nRecv TPS : %d\nSend TPS : %d\n\n\n\n", mpChatServer->GetAcceptTotal(), mpChatServer->GetAcceptTPS(), mpChatServer->GetUpdateTPS(), mpChatServer->GetRecvTPS(), mpChatServer->GetSendTPS());

		mpChatServer->InitializeTPS();

		mpChatServer->InitializeChatTPS();

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
				// 커넥트에 성공하였으니 넌시그널로 변경
				ResetEvent(mConnectEvent);

				CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"[CLanMonitoringClient]", L"[ConnectThread] Connect Success");

				break;
			}

			// 1초마다 한 번씩 시도한다.
			Sleep(1000);
		}
	}

THREAD_EXIT:

	return;
}


BOOL CLanMonitoringClient::setupUpdateThread(void)
{
	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)&mUpdateThreadID);
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL CLanMonitoringClient::setupConnectThread(void)
{
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


void CLanMonitoringClient::procedureLoginServer(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_LOGIN << (INT)mServerNo;

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerRun(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN << (INT)TRUE << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerCPU(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU << (DWORD)CCPUProfiler::GetInstance()->GetProcessTotalPercentage() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerMemory(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM << ((INT)CHardwareProfiler::GetInstance()->GetPrivateBytes()) / 1000000 << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerSessionCount(void)
{

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SESSION << mpChatServer->GetCurrentClientCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerPlayerCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PLAYER << mpChatServer->GetPlayerCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerUpdateTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS << mpChatServer->GetUpdateTPS() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerMessagePool(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL << (INT)CMessage::GetAllocCount() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::procedureChatServerJobPool(void)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << mpChatServer->GetJobQueueUseSize() << (INT)time(NULL);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}