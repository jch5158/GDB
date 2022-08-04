#include "stdafx.h"


CLanLoginClient::CLanLoginClient(void)
	: mbConnectStateFlag(FALSE)
	, mbConnectThreadFlag(FALSE)
	, mSessionID(0)
	, mConnectThreadID(0)
	, mConnectThreadHandle(INVALID_HANDLE_VALUE)
	, mConnectEvent(CreateEvent(NULL, FALSE, TRUE, nullptr))
{
}

CLanLoginClient::~CLanLoginClient(void)
{
	closeConnectThread();
}

BOOL CLanLoginClient::OnStart(void)
{
	if (setupConnectThread() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

void CLanLoginClient::OnServerJoin(UINT64 sessionID)
{
	mbConnectStateFlag = TRUE;

	mSessionID = sessionID;

	sendLoginServerLoginRequest();

	return;
}

void CLanLoginClient::OnServerLeave(UINT64 sessionID)
{
	mbConnectStateFlag = FALSE;

	SetEvent(mConnectEvent);

	return;
}

void CLanLoginClient::OnRecv(UINT64 sessionID, CMessage* pMessage)
{

	return;
}

void CLanLoginClient::OnError(DWORD errorCode, const WCHAR* pErrorMessage)
{

	return;
}

void CLanLoginClient::OnStop(void)
{
	mbConnectThreadFlag = FALSE;

	SetEvent(mConnectEvent);

	return;
}

BOOL CLanLoginClient::GetConnectStateFlag(void) const
{
	return mbConnectStateFlag;
}


void CLanLoginClient::NotificationClientLoginSuccess(UINT64 accountNo)
{
	sendClientLoginSuccess(accountNo);

	return;
}



DWORD WINAPI CLanLoginClient::ExecuteConnectThread(void* pParam)
{
	CLanLoginClient* pLanLoginServerClient = (CLanLoginClient*)pParam;

	pLanLoginServerClient->ConnectThread();

	return 1;
}

void CLanLoginClient::ConnectThread(void)
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

BOOL CLanLoginClient::setupConnectThread(void)
{
	mbConnectThreadFlag = TRUE;

	mConnectThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteConnectThread, this, 0, (UINT*)&mConnectThreadID);
	if (mConnectThreadHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	return TRUE;
}

void CLanLoginClient::closeConnectThread(void)
{
	if (WaitForSingleObject(mConnectThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"[CLanMonitoringClient]", L"[closeConnectThread] WaitForSingleObject Error : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mConnectThreadHandle);

	return;
}

void CLanLoginClient::sendLoginServerLoginRequest(void)
{
	CMessage* pMessage = CMessage::Alloc();

	packingLoginServerLoginRequest(dfSERVER_TYPE_GAME, pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanLoginClient::sendClientLoginSuccess(UINT64 accountNo)
{
	CMessage* pMessage = CMessage::Alloc();

	packingClientLoginSuccess(accountNo, dfSERVER_TYPE_GAME, pMessage);

	SendPacket(mSessionID, pMessage);

	pMessage->Free();

	return;
}

void CLanLoginClient::packingClientLoginSuccess(UINT64 accountNo, UINT64 serverType, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_SS_RES_NEW_CLIENT_LOGIN << accountNo << serverType;

	return;
}

void CLanLoginClient::packingLoginServerLoginRequest(BYTE serverType, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_SS_LOGINSERVER_LOGIN << serverType;

	return;
}