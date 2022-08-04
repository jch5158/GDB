#include "stdafx.h"
#include "CGodDamnBug.h"
#include "CServerController.h"

BOOL SetupLogSystem(void);

BOOL ParseGodDamnBugConfigFile(
	WCHAR* pServerIP, INT* pServerPort, INT* pAuthenticFrame, INT* pUpdateFrame, INT* pSendFrame, INT* pMaxMessageSize,
	INT* pHeaderCode, INT* pStaticKey, INT* pbNagleFlag, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pMaxClientCount);

BOOL ParseMonitoringClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT* pClientWorkerThreadCount, INT* pServerNo);

BOOL ParseLanLoginClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT* pClientWorkerThreadCount);

BOOL GodDamnBugServerOn(CMMOServer** pGodDamnBug, CLanClient* pLanLoginClient, WCHAR* pServerIP, INT serverPort, INT authenticFrame, INT updateFrame, INT sendFrame, INT maxMessageSize,
	INT headerCode, INT staticKey, INT bNagleFlag, INT runningThreadCount, INT workerThreadCount, INT maxClinetCount);

BOOL MonitoringClientOn(CLanClient** pLanMonitoringClient, CMMOServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bMonitoringClientNagleFlag, INT monitoringClientRunningThreadCount, INT monitoringClientWorkerThreadCount, INT chatServerNo);

BOOL LanLoginClientOn(CLanClient** pLanLoginClient, WCHAR* pServerIP, INT serverPort, BOOL bNagleFlag, INT runningThreadCount, INT workerThreadCount);

BOOL MonitoringClientOff(CLanClient* pMonitoringClient);

BOOL LanLoginClientOff(CLanClient* pLanLoginClient);

BOOL GodDamnBugServerOff(CMMOServer* pGodDamnBug);

int main()
{
    timeBeginPeriod(1);

	CCrashDump::GetInstance();

	setlocale(LC_ALL, "");

	CMMOServer* pGodDamnBug = nullptr;
	CLanClient* pMonitoringClient = nullptr;
	CLanClient* pLanLoginClient = nullptr;

    WCHAR serverIP[50];
    INT serverPort;
    INT authenticFrame;
    INT updateFrame;
    INT sendFrame;
    INT maxMessageSize;
    INT headerCode;
    INT staticKey;
    INT bNagleFlag;
    INT runningThreadCount;
    INT workerThreadCount;
    INT maxClinetCount;

	WCHAR monitoringServerIP[50] = { 0, };
	INT monitoringServerPort;
	INT bMonitoringClientNagleFlag;
	INT monitoringClientRunningThreadCount;
	INT monitoringClientWorkerThreadCount;
	INT chatServerNo;

	WCHAR lanLoginClientIP[50] = { 0, };
	INT lanLoginClientPort;
	INT bLanLoginClientNagleFlag;
	INT lanLoginClientRunningThreadCount;
	INT lanLoginClientWorkerThreadCount;


	do
	{
		if (SetupLogSystem() == FALSE)
		{
			break;
		}

		if (ParseGodDamnBugConfigFile(serverIP, &serverPort, &authenticFrame, &updateFrame, &sendFrame, &maxMessageSize, &headerCode, &staticKey,
			&bNagleFlag, &runningThreadCount, &workerThreadCount, &maxClinetCount) == FALSE)
		{
			break;
		}

		if (ParseMonitoringClient(monitoringServerIP, _countof(monitoringServerIP), &monitoringServerPort, &bMonitoringClientNagleFlag, &monitoringClientRunningThreadCount, &monitoringClientWorkerThreadCount, &chatServerNo) == FALSE)
		{
			break;
		}

		if (ParseLanLoginClient(lanLoginClientIP, _countof(lanLoginClientIP), &lanLoginClientPort, &bLanLoginClientNagleFlag, &lanLoginClientRunningThreadCount, &lanLoginClientWorkerThreadCount) == FALSE)
		{
			break;
		}


		if (LanLoginClientOn(&pLanLoginClient, lanLoginClientIP, lanLoginClientPort, bLanLoginClientNagleFlag, lanLoginClientRunningThreadCount, lanLoginClientWorkerThreadCount) == FALSE)
		{
			break;
		}


		if (GodDamnBugServerOn(&pGodDamnBug, pLanLoginClient, serverIP, serverPort, authenticFrame, updateFrame, sendFrame, maxMessageSize, headerCode, staticKey,
			bNagleFlag, runningThreadCount, workerThreadCount, maxClinetCount) == FALSE)
		{
			break;
		}


		if (MonitoringClientOn(&pMonitoringClient, &pGodDamnBug, monitoringServerIP, monitoringServerPort, bMonitoringClientNagleFlag, monitoringClientRunningThreadCount, monitoringClientWorkerThreadCount, chatServerNo) == FALSE)
		{
			break;
		}

		CServerController serverController;

		serverController.SetGameServer((CGodDamnBug*)pGodDamnBug);

		serverController.SetLoginClient((CLanLoginClient*)pLanLoginClient);

		serverController.SetMonitoringClient((CLanMonitoringClient*)pMonitoringClient);

		while (serverController.GetShutdownFlag() == FALSE)
		{
			serverController.ServerControling();

			Sleep(1000);
		}

		if (MonitoringClientOff(pMonitoringClient) == FALSE)
		{
			break;
		}

		if (LanLoginClientOff(pLanLoginClient) == FALSE)
		{
			break;
		}

		if (GodDamnBugServerOff(pGodDamnBug) == FALSE)
		{
			break;
		}

	} while (0);


    timeEndPeriod(1);

	system("pause");

    return 1;
}


BOOL SetupLogSystem(void)
{

	if (CSystemLog::GetInstance()->SetLogDirectory(L"GodDamnBug Log") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[SetupLogSystem] SetLogDirectory Failed");

		return FALSE;
	}

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[SetupLogSystem] Config File Lod Failed");

		return FALSE;
	}

	WCHAR buffer[MAX_PATH] = { 0, };

	parser.GetString(L"LOG_LEVEL", buffer, MAX_PATH);

	CSystemLog::eLogLevel logLevel;

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
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[SetupLogSystem] LogLevel Value Error : %s", buffer);

		return FALSE;
	}

	if (CSystemLog::GetInstance()->SetLogLevel(logLevel) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[SetupLogSystem] SetLogLevel is Failed : %d", (DWORD)logLevel);

		return FALSE;
	}

	return TRUE;

}


BOOL ParseGodDamnBugConfigFile(
	WCHAR* pServerIP, INT* pServerPort, INT* pAuthenticFrame, INT* pUpdateFrame, INT* pSendFrame, INT* pMaxMessageSize,
	INT* pHeaderCode, INT* pStaticKey, INT* pbNagleFlag, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pMaxClientCount)
{

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[SetupLogSystem] Config File Lod Failed");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"GOD_DAMN_BUG", L"SERVER_IP", pServerIP, 50) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] SERVER_IP");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] SERVER_PORT");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"AUTHENTIC_FRAME", pAuthenticFrame) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] AUTHENTIC_FRAME");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"SEND_FRAME", pSendFrame) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] SEND_FRAME");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"UPDATE_FRAME", pUpdateFrame) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] UPDATE_FRAME");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"MAX_MESSAGE_SIZE", pMaxMessageSize) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] MAX_MESSAGE_SIZE");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"HEADER_CODE", pHeaderCode) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] HEADER_CODE");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"STATIC_KEY", pStaticKey) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] STATIC_KEY");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"RUNNING_THREAD", pRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] RUNNING_THREAD");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"WORKER_THREAD", pWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] WORKER_THREAD");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"NAGLE_OPTION", pbNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] NAGLE_OPTION");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"GOD_DAMN_BUG", L"MAX_CLIENT", pMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"GodDamnBug", L"[ParseGodDamnBugConfigFile] MAX_CLIENT");

		return FALSE;
	}

	return TRUE;
}

BOOL ParseMonitoringClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT* pClientWorkerThreadCount, INT* pServerNo)
{
	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] Config\\ServerConfig.ini Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"LAN_MONITORING_CLIENT", L"SERVER_IP", pServerIP, serverIPCb) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] SERVER_IP Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] SERVER_PORT Not Found");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"NAGLE_OPTION", pbClientNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] NAGLE_OPTION Not Found");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"RUNNING_THREAD", pClientRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] RUNNING_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"WORKER_THREAD", pClientWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] WORKER_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"SERVER_NUMBER", pServerNo) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanClientConfigFile] SERVER_NUMBER Not Found");

		return FALSE;
	}

	return TRUE;
}

BOOL ParseLanLoginClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT* pClientWorkerThreadCount)
{
	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanLoginClient] Config\\ServerConfig.ini Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"LAN_LOGIN_CLIENT", L"SERVER_IP", pServerIP, serverIPCb) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanLoginClient] SERVER_IP Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_CLIENT", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanLoginClient] SERVER_PORT Not Found");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"LAN_LOGIN_CLIENT", L"NAGLE_OPTION", pbClientNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanLoginClient] NAGLE_OPTION Not Found");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"LAN_LOGIN_CLIENT", L"RUNNING_THREAD", pClientRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanLoginClient] RUNNING_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_CLIENT", L"WORKER_THREAD", pClientWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseLanLoginClient] WORKER_THREAD Not Found");

		return FALSE;
	}

	return TRUE;
}


BOOL GodDamnBugServerOn(CMMOServer** pGodDamnBug, CLanClient* pLanLoginClient, WCHAR* pServerIP, INT serverPort, INT authenticFrame, INT updateFrame, INT sendFrame, INT maxMessageSize,
	INT headerCode, INT staticKey, INT bNagleFlag, INT runningThreadCount, INT workerThreadCount, INT maxClinetCount) 
{
	*pGodDamnBug = new CGodDamnBug;

	((CGodDamnBug*)(*pGodDamnBug))->SetLoginClient((CLanLoginClient*)pLanLoginClient);

	if ((*pGodDamnBug)->Start(pServerIP, serverPort, sendFrame, updateFrame, authenticFrame, maxMessageSize,
		bNagleFlag, headerCode, staticKey, runningThreadCount, workerThreadCount, maxClinetCount, 60) == FALSE)
	{
		return FALSE;
	}


	return TRUE;
}

BOOL MonitoringClientOn(CLanClient** pLanMonitoringClient, CMMOServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bMonitoringClientNagleFlag, INT monitoringClientRunningThreadCount, INT monitoringClientWorkerThreadCount, INT chatServerNo)
{
	*pLanMonitoringClient = new CLanMonitoringClient;

	((CLanMonitoringClient*)*pLanMonitoringClient)->SetServerNo(chatServerNo);

	((CLanMonitoringClient*)*pLanMonitoringClient)->SetContentsPtr((CGodDamnBug*)*pChatServer);

	CHardwareProfiler::SetHardwareProfiler(TRUE, FALSE, FALSE, FALSE, FALSE, nullptr);

	if ((*pLanMonitoringClient)->Start(pServerIP, serverPort, (BOOL)bMonitoringClientNagleFlag, monitoringClientRunningThreadCount, monitoringClientWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[MonitoringClientOn] Lan Client Start is Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL LanLoginClientOn(CLanClient** pLanLoginClient, WCHAR* pServerIP, INT serverPort, BOOL bNagleFlag, INT runningThreadCount, INT workerThreadCount)
{
	*pLanLoginClient = new CLanLoginClient;

	if ((*pLanLoginClient)->Start(
		pServerIP, serverPort, bNagleFlag, runningThreadCount, workerThreadCount
	) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[LanLoginClientOn] Server Start is Failed");

		return FALSE;
	}

	return TRUE;
}


BOOL GodDamnBugServerOff(CMMOServer* pGodDamnBug)
{
	if (pGodDamnBug->Stop() == FALSE)
	{
		return FALSE;
	}

	delete pGodDamnBug;

	return TRUE;
}


BOOL MonitoringClientOff(CLanClient* pMonitoringClient)
{
	if (pMonitoringClient->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[MonitoringClientOff] Client Stop is Failed");

		return FALSE;
	}

	delete pMonitoringClient;

	return TRUE;
}


BOOL LanLoginClientOff(CLanClient* pLanLoginClient)
{
	if (pLanLoginClient->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[LanLoginClientOff] Client Stop is Failed");

		return FALSE;
	}

	delete pLanLoginClient;

	return TRUE;
}