#include "stdafx.h"

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

	wscanf_s(L"%s", buffer, (unsigned int)_countof(buffer));

	int runningThreadCount = _wtoi(buffer);

	wprintf_s(L"WorkerThread 개수 : ");

	wscanf_s(L"%s", buffer, (unsigned int)_countof(buffer));

	int threadCount = _wtoi(buffer);

	pNetwork = new CContents();

	wprintf_s(L"최대 접속자 : ");

	wscanf_s(L"%s", buffer, (unsigned int)_countof(buffer));

	int maxClient = _wtoi(buffer);

	if (pNetwork->Start(SERVERIP, SERVERPORT, 2000, true, 0x77, 0x33, runningThreadCount, threadCount, maxClient) == false)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"NetServer", L"SERVERIP :%d SERVERPORT : %d, runningThreadCount : %d, threadCount : %d, maxClient : %d", SERVERIP, SERVERPORT, runningThreadCount, threadCount, maxClient);

		CCrashDump::Crash();
	}
}


bool SetupLogSystem()
{
	if (CSystemLog::GetInstance()->SetLogDirectory(L"LanServer Log") == false)
	{
		return false;
	}

	wprintf_s(L"1. LOG_LEVEL_DEBUG\n2. LOG_LEVEL_WARNING\n3. LOG_LEVEL_NOTICE\n4. LOG_LEVEL_ERROR\n");
	wprintf_s(L"로그 레벨을 입력하세요 : ");

	WCHAR buffer[10] = { 0, };
	wscanf_s(L"%s", buffer, (unsigned int)_countof(buffer));

	if (CSystemLog::GetInstance()->SetLogLevel((CSystemLog::eLogLevel)_wtoi(buffer)) == false)
	{
		return false;
	}
}

void ServerControl()
{
	CServerController serverController;

	while (!serverController.mbShutdownFlag)
	{
		serverController.ServerControling();

		Sleep(100);
	}
}

void ServerOff()
{
	pNetwork->Stop();

	delete pNetwork;
}