#include "stdafx.h"

//constexpr short SERVER_PORT = 12001;
//
//constexpr const WCHAR* SERVER_IP = L"0.0.0.0";

CMMOServer* pMMOServer = nullptr;

CLanClient* pLanMonitoringClient = nullptr;

WCHAR gpServerIP[MAX_PATH] = { 0, };

WCHAR gpLanClientIP[MAX_PATH] = { 0, };

BYTE gHeaderCode = 0;

BYTE gStaticKey = 0;

INT gServerPort = 0;

INT gLanClientPort = 0;

INT gSendFame = 0;

INT gUpdateFrame = 0;

INT gAuthenticFrame = 0;

INT gMaxMessageSize = 0;


INT gNagleFlag = TRUE;

INT gLanClientNalgeFlag = TRUE;

INT gRunningThreadCount = 0;

INT gLanRunningThreadCount = 0;

INT gWorkerThreadcount = 0;

INT gLanWorkerThreadCount = 0;

INT gMaxClientCount = 0;

INT gTimeoutSec = 0;

void SetupLogSystem(void);

void ParseServerConfigFile(void);

void ParseLanClientConfigFile(void);

void ServerOn(void);

void ServerOff(void);

int wmain()
{
	CCrashDump::GetInstance();

	timeBeginPeriod(1);

	setlocale(LC_ALL, "");

	//CPerformanceProfiler::SetPerformanceProfiler(L"MMOServer", 15);

	pMMOServer = new CEcho;

	pLanMonitoringClient = new CLanMonitoringClient;

	SetupLogSystem();

	ParseServerConfigFile();

	ParseLanClientConfigFile();

	ServerOn();

	CServerController serverController;

	serverController.SetEchoServerPtr((CEcho*)pMMOServer);

	while (serverController.mbShutdownFlag == false)
	{
		serverController.ServerControling(pMMOServer);

		Sleep(1000);
	}

	ServerOff();

	delete pMMOServer;

	delete pLanMonitoringClient;

	timeEndPeriod(1);

	return 1;
}

void SetupLogSystem(void)
{
	if (CSystemLog::GetInstance()->SetLogDirectory(L"MMOServer Log") == FALSE)
	{
		CCrashDump::Crash();
	}

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CCrashDump::Crash();
	}

	WCHAR buffer[MAX_PATH] = { 0, };

	parser.GetNamespaceString(L"MMO_SERVER", L"LOG_LEVEL", buffer, MAX_PATH);

	CSystemLog::eLogLevel logLevel = CSystemLog::eLogLevel::LogLevelDebug;

	if (wcscmp(buffer, L"LOG_LEVEL_DEBUG") == 0)
	{
		logLevel = CSystemLog::eLogLevel::LogLevelDebug;
	}
	else if (wcscmp(buffer, L"LOG_LEVEL_NOTICE") == 0)
	{
		logLevel = CSystemLog::eLogLevel::LogLevelNotice;
	}
	else if (wcscmp(buffer, L"LOG_LEVEL_WARNING") == 0)
	{
		logLevel = CSystemLog::eLogLevel::LogLevelWarning;
	}
	else if (wcscmp(buffer, L"LOG_LEVEL_ERROR") == 0)
	{
		logLevel = CSystemLog::eLogLevel::LogLevelError;
	}
	else
	{
		CCrashDump::Crash();
	}

	if (CSystemLog::GetInstance()->SetLogLevel(logLevel) == FALSE)
	{
		CCrashDump::Crash();
	}

	return;
}


void ParseServerConfigFile(void)
{
	WCHAR pBuffer[MAX_PATH] = { 0, };

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] Config\\ServerConfig.ini Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceString(L"MMO_SERVER", L"SERVER_IP", gpServerIP, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] SERVER_IP Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"SERVER_PORT", &gServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] SERVER_PORT Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"SEND_FRAME", &gSendFame) == FALSE)
	{
		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"UPDATE_FRAME", &gUpdateFrame) == FALSE)
	{
		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"AUTHENTIC_FRAME", &gAuthenticFrame) == FALSE)
	{
		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"MAX_MESSAGE_SIZE", &gMaxMessageSize) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] MAX_MESSAGE_SIZE Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"NAGLE_OPTION", &gNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] NAGLE_OPTION Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceString(L"MMO_SERVER", L"HEADER_CODE", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] HEADER_CODE Not Found");

		CCrashDump::Crash();
	}

	gHeaderCode = (BYTE)wcstol(pBuffer, NULL, 10);

	if (parser.GetNamespaceString(L"MMO_SERVER", L"STATIC_KEY", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] STATIC_KEY Not Found");

		CCrashDump::Crash();
	}

	gStaticKey = (BYTE)wcstol(pBuffer, NULL, 10);

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"RUNNING_THREAD", &gRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] RUNNING_THREAD Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"WORKER_THREAD", &gWorkerThreadcount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] WORKER_THREAD Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"MAX_CLIENT", &gMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] MAX_CLIENT Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"MMO_SERVER", L"TIMEOUT_SEC", &gTimeoutSec) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseServerConfigFile] MAX_CLIENT Not Found");

		CCrashDump::Crash();
	}

	return;
}


void ParseLanClientConfigFile(void)
{
	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] Config\\ServerConfig.ini Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceString(L"LAN_MONITORING_CLIENT", L"SERVER_IP", gpLanClientIP, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] SERVER_IP Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"SERVER_PORT", &gLanClientPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] SERVER_PORT Not Found");

		CCrashDump::Crash();
	}


	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"NAGLE_OPTION", &gLanClientNalgeFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] NAGLE_OPTION Not Found");

		CCrashDump::Crash();
	}


	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"RUNNING_THREAD", &gLanRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] RUNNING_THREAD Not Found");

		CCrashDump::Crash();
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"WORKER_THREAD", &gLanWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] WORKER_THREAD Not Found");

		CCrashDump::Crash();
	}

	INT serverNumber;

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"SERVER_NUMBER", &serverNumber) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] SERVER_NUMBER Not Found");

		CCrashDump::Crash();
	}

	((CLanMonitoringClient*)pLanMonitoringClient)->SetServerNo(serverNumber);


	WCHAR buffer[MAX_PATH];

	if (parser.GetNamespaceString(L"LAN_MONITORING_CLIENT", L"NETWORK_ADAPTOR", buffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ParseLanClientConfigFile] NETWORK_ADAPTOR Not Found");

		CCrashDump::Crash();
	}
	
	CHardwareProfiler::SetHardwareProfiler(TRUE, FALSE, FALSE, FALSE, FALSE, buffer);

	return;
}



void ServerOn(void)
{

	((CLanMonitoringClient*)pLanMonitoringClient)->SetEchoServerPtr((CEcho*)pMMOServer);

	if (pMMOServer->Start(
		gpServerIP, gServerPort, gSendFame, gUpdateFrame, gAuthenticFrame, gMaxMessageSize,(BOOL)gNagleFlag, gHeaderCode, gStaticKey, gRunningThreadCount, gWorkerThreadcount, gMaxClientCount, gTimeoutSec
	) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ServerOn] MMO Server Start is Failed");

		CCrashDump::Crash();
	}

	if (pLanMonitoringClient->Start(gpLanClientIP, gLanClientPort, (BOOL)gLanClientNalgeFlag, gLanRunningThreadCount, gLanWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ServerOn] Lan Client Start is Failed");

		CCrashDump::Crash();
	}

	return;
}

void ServerOff(void)
{
	if (pMMOServer->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ServerOff] Server Stop is Failed");

		CCrashDump::Crash();
	}

	if (pLanMonitoringClient->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[ServerOff] Client Stop is Failed");

		CCrashDump::Crash();

	}

	return;
}
