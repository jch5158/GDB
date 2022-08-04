#include "stdafx.h"

#define SERVERPORT 6000
#define SERVERIP L"0.0.0.0"


CNetServer* pNetwork = nullptr;

void ServerSetup();

bool SetupLogSystem();

void ServerControl();

void ServerOff();

int wmain()
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

	pNetwork = new CContents;

	wprintf_s(L"최대 접속자 : ");

	wscanf_s(L"%s", buffer, (UINT)_countof(buffer));

	INT maxClient = _wtoi(buffer);

	if (pNetwork->Start(SERVERIP, SERVERPORT,1000,true, false, 0x77 ,0x33,60, runningThreadCount, threadCount, maxClient) == false)
		CCrashDump::Crash();

	return;
}


bool SetupLogSystem()
{
	if (CSystemLog::GetInstance()->SetLogDirectory(L"NetServer Log") == false)
		return false;

	wprintf_s(L"1. LOG_LEVEL_DEBUG\n2. LOG_LEVEL_WARNING\n3. LOG_LEVEL_NOTICE\n4. LOG_LEVEL_ERROR\n");
	wprintf_s(L"로그 레벨을 입력하세요 : ");

	wchar_t buffer[10] = { 0, };
	wscanf_s(L"%s", buffer, _countof(buffer));

	if (CSystemLog::GetInstance()->SetLogLevel((CSystemLog::eLogLevel)_wtoi(buffer)) == false)
		return false;

	return true;
}

void ServerControl()
{
	CServerController serverController(pNetwork);

	while (serverController.GetShutdwonFlag() == false)
	{
		serverController.ServerControling();

		Sleep(1000);
	}

	return;
}

void ServerOff()
{
	pNetwork->Stop();

	delete pNetwork;

	return;
}