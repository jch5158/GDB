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

	mbConnectStateFlag = TRUE;

	sendMonitoringServerLogin();

	return;
}

void CLanMonitoringClient::OnServerLeave(UINT64 sessionID)
{
	mbConnectStateFlag = FALSE;

	// ������ �������� �ñ׳η� ����
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

void CLanMonitoringClient::SetContentsPtr(CChattingServer* pEchoServer)
{
	mpChatServer = pEchoServer;

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

		//mpChatServer->InitializeTPS();

		//mpChatServer->InitializeUpdateTPS();

		Sleep(1000);
	}

	return;
}


void CLanMonitoringClient::ConnectThread(void)
{
	for (;;)
	{
		// ����ȭ ��ü
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

			// 1�ʸ��� �� ���� �õ��Ѵ�.
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

	sendChatServerRun();

	sendChatServerCPU();

	sendChatServerMemory();

	sendChatServerSessionCount();

	sendChatServerPlayerCount();

	sendChatServerUpdateTPS();

	sendChatServerMessagePool();

	sendChatServerJobPool();

	return;
}


void CLanMonitoringClient::sendChatServerRun(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, TRUE, time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerCPU(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, CCPUProfiler::GetInstance()->GetProcessTotalPercentage(), time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerMemory(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, ((INT)CHardwareProfiler::GetInstance()->GetPrivateBytes()) / 1000000, time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerSessionCount(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SESSION, mpChatServer->GetCurrentClientCount(), time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerPlayerCount(void)
{
	CMessage* pMessage = CMessage::Alloc();
	
	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_PLAYER, mpChatServer->GetPlayerCount(), time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerUpdateTPS(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, mpChatServer->GetUpdateTPS(), time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerMessagePool(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, CMessage::GetAllocCount(), time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::sendChatServerJobPool(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingMonitoringData(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, mpChatServer->GetJobQueueStatus(), time(NULL), pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringClient::packingMonitoringServerLogin(WORD type, DWORD serverNo, CMessage* pMessage)
{
	*pMessage << type << mServerNo;

	return;
}


void CLanMonitoringClient::packingMonitoringData(WORD type, BYTE dataType, INT data, INT time, CMessage* pMessage)
{
	*pMessage << type << dataType << data << time;
		
	return;
}
