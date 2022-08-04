#include "stdafx.h"

constexpr INT SERVERPORT = 6000;

constexpr const WCHAR* SERVERIP = L"0.0.0.0";

void ServerSetup();

BOOL SetupLogSystem();

void ServerControl();

void ServerOff();


CLanServer* pNetwork = nullptr;


INT wmain()
{	
	CCrashDump::GetInstance();

	timeBeginPeriod(1);

	setlocale(LC_ALL, "");

	ServerSetup();

	ServerControl();

	ServerOff();

	timeEndPeriod(1);

	return 1;
}

void ServerSetup()
{
	SetupLogSystem();

	WCHAR buffer[10] = { 0, };

	wprintf_s(L"IOCP Running Thread 개수 : ");

	wscanf_s(L"%s", buffer, (UINT)_countof(buffer));

	INT runningThreadCount = _wtoi(buffer);

	wprintf_s(L"WorkerThread 개수 : ");

	wscanf_s(L"%s", buffer, (UINT)_countof(buffer));

	INT threadCount = _wtoi(buffer);

	pNetwork = new CContents();

	wprintf_s(L"최대 접속자 : ");

	wscanf_s(L"%s", buffer, (UINT)_countof(buffer));

	INT maxClient = _wtoi(buffer);

	if (pNetwork->Start(SERVERIP, SERVERPORT, TRUE,runningThreadCount, threadCount, maxClient) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[ServerSetup] Start Func is Failed");
		CCrashDump::Crash();
	}
}


BOOL SetupLogSystem()
{
	if (CSystemLog::GetInstance()->SetLogDirectory(L"LanServer Log") == FALSE)
	{
		return FALSE;
	}

	wprintf_s(L"1. LOG_LEVEL_DEBUG\n2. LOG_LEVEL_WARNING\n3. LOG_LEVEL_NOTICE\n4. LOG_LEVEL_ERROR\n");
	wprintf_s(L"로그 레벨을 입력하세요 : ");

	WCHAR buffer[10] = { 0, };
	wscanf_s(L"%s", buffer, (UINT)_countof(buffer));

	if (CSystemLog::GetInstance()->SetLogLevel((CSystemLog::eLogLevel)_wtoi(buffer)) == FALSE)
	{
		return FALSE;
	}
}

void ServerControl()
{
	CServerController serverController;

	while (!serverController.mbShutdownFlag)
	{
		serverController.ServerControling();

		wprintf_s(L"현재 접속자 : %d\n", pNetwork->GetCurrentClientCount());
		
		wprintf_s(L"Chunk Alloc Count : %d\n", CMessage::GetAllocCount());
		//CTLSLockFreeObjectFreeList<CMessage>::ShowTlsAllocView(L"Message");	

		Sleep(3000);
	}
}

void ServerOff()
{
	pNetwork->Stop();

	delete pNetwork;
}