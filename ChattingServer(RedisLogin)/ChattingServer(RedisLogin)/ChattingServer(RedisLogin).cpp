#include "stdafx.h"
#include "ServerController.h"

BOOL SetupLogSystem(void);

BOOL ParseChatServer(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbServerNagleFlag, INT* pbTimeoutFlag, INT* pMaxMessageSize, BYTE* pHeaderCode, BYTE* pStaticKey, INT* pTimeoutSec, INT* pServerRunningThreadCount, INT* pServerWorkerThreadCount, INT* pMaxClientCount);

BOOL ParseMonitoringClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT *pClientWorkerThreadCount, INT* pServerNo);

BOOL ParseLanLoginClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT* pClientWorkerThreadCount);

BOOL ChatServerOn(CNetServer** pChatServer, CLanClient *pLanLoginClient, WCHAR* pServerIP, INT serverPort, BOOL bServerNagleFlag, BOOL bTimeout, INT maxMessageSize, BYTE headerCode, BYTE staticKey, INT timeoutSec, INT runningThreadCount, INT workerThreadCount, INT maxClientCount);

BOOL MonitoringClientOn(CLanClient** pLanMonitoringClient, CNetServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bMonitoringClientNagleFlag, INT monitoringClientRunningThreadCount, INT monitoringClientWorkerThreadCount, INT chatServerNo);

BOOL LanLoginClientOn(CLanClient** pLanLoginClient, WCHAR* pServerIP, INT serverPort, BOOL bNagleFlag, INT runningThreadCount, INT workerThreadCount);

BOOL ChatServerOff(CNetServer* pChatServer);

BOOL MonitoringClientOff(CLanClient* pMonitoringClient);

BOOL LanLoginClientOff(CLanClient* pLanLoginClient);

INT wmain()
{
	CNetServer* pChatServer = nullptr;
	CLanClient* pMonitoringClient = nullptr;
	CLanClient* pLanLoginClient = nullptr;

	WCHAR chatServerIP[MAX_PATH] = { 0, };	
	INT chatServerPort;
	INT bChatServerNagleFlag;
	INT bTimeout;
	INT maxMessageSize;
	BYTE headerCode;
	BYTE staticKey;
	INT timeoutSec;
	INT chatServerRunningThreadCount;
	INT chatServerWorkerThreadCount;
	INT maxClientCount;

	WCHAR monitoringServerIP[MAX_PATH] = { 0, };
	INT monitoringServerPort;
	INT bMonitoringClientNagleFlag;
	INT monitoringClientRunningThreadCount;
	INT monitoringClientWorkerThreadCount;
	INT chatServerNo;

	WCHAR lanLoginClientIP[MAX_PATH] = { 0, };
	INT lanLoginClientPort;
	INT bLanLoginClientNagleFlag;
	INT lanLoginClientRunningThreadCount;
	INT lanLoginClientWorkerThreadCount;


	CCrashDump::GetInstance();
	
	timeBeginPeriod(1);
	
	setlocale(LC_ALL, "");
	
	CTLSPerformanceProfiler::SetPerformanceProfiler(L"ChattingServer", 15);
	
	do
	{
		if (SetupLogSystem() == FALSE)
		{
			break;
		}

		if (ParseChatServer(chatServerIP, _countof(chatServerIP), &chatServerPort, &bChatServerNagleFlag, &bTimeout,&maxMessageSize, &headerCode, &staticKey, &timeoutSec, &chatServerRunningThreadCount, &chatServerWorkerThreadCount, &maxClientCount) == FALSE)
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

		if (ChatServerOn(&pChatServer, pLanLoginClient, chatServerIP, chatServerPort, bChatServerNagleFlag, bTimeout, maxMessageSize, headerCode, staticKey, timeoutSec,chatServerRunningThreadCount, chatServerWorkerThreadCount, maxClientCount) == FALSE)
		{
			break;
		}

		if (MonitoringClientOn(&pMonitoringClient, &pChatServer, monitoringServerIP, monitoringServerPort, bMonitoringClientNagleFlag, monitoringClientRunningThreadCount, monitoringClientWorkerThreadCount, chatServerNo) == FALSE)
		{
			break;
		}


		CServerController serverController;

		serverController.SetChatServer((CChattingServer*)pChatServer);

		serverController.SetMonitoringClient((CLanMonitoringClient*)pMonitoringClient);

		serverController.SetLoginClient((CLanLoginClient*)pLanLoginClient);

		while (serverController.GetShutdownFlag() == FALSE)
		{
			serverController.ServerControling();

			Sleep(1000);
		}


		if (MonitoringClientOff(pMonitoringClient) == FALSE)
		{
			break;
		}

		if (ChatServerOff(pChatServer) == FALSE)
		{
			break;
		}

		if (LanLoginClientOff(pLanLoginClient) == FALSE)
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
	if (CSystemLog::GetInstance()->SetLogDirectory(L"ChattingServer Log") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[SetupLogSystem] SetLogDirectory Failed");

		return FALSE;
	}

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[SetupLogSystem] Config File Lod Failed");

		return FALSE;
	}

	WCHAR buffer[MAX_PATH] = { 0, };

	parser.GetString(L"LOG_LEVEL", buffer, MAX_PATH);

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
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[SetupLogSystem] LogLevel Value Error : %s", buffer);

		return FALSE;
	}

	if (CSystemLog::GetInstance()->SetLogLevel(logLevel) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[SetupLogSystem] SetLogLevel is Failed : %d", (DWORD)logLevel);

		return FALSE;
	}

	return TRUE;
}

BOOL ParseChatServer(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbServerNagleFlag, INT *pbTimeout, INT* pMaxMessageSize, BYTE* pHeaderCode, BYTE* pStaticKey, INT* pTimeoutSec, INT* pServerRunningThreadCount, INT* pServerWorkerThreadCount, INT* pMaxClientCount)
{
	WCHAR pBuffer[MAX_PATH] = { 0, };

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] Config\\ServerConfig.ini Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"CHATTING_SERVER", L"SERVER_IP", pServerIP, serverIPCb) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] SERVER_IP Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] SERVER_PORT Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"NAGLE_OPTION", pbServerNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] NAGLE_OPTION Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"TIMEOUT", pbTimeout) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] NAGLE_OPTION Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"TIMEOUT_SEC", pTimeoutSec) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] NAGLE_OPTION Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"MAX_MESSAGE_SIZE", pMaxMessageSize) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] MAX_MESSAGE_SIZE Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"CHATTING_SERVER", L"HEADER_CODE", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] HEADER_CODE Not Found");

		return FALSE;
	}

	*pHeaderCode = (BYTE)wcstol(pBuffer, NULL, 16);

	if (parser.GetNamespaceString(L"CHATTING_SERVER", L"STATIC_KEY", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] STATIC_KEY Not Found");

		return FALSE;
	}

	*pStaticKey = (BYTE)wcstol(pBuffer, NULL, 16);

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"RUNNING_THREAD", pServerRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] RUNNING_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"WORKER_THREAD", pServerWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] WORKER_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"MAX_CLIENT", pMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] MAX_CLIENT Not Found");

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

BOOL ChatServerOn(CNetServer** pChatServer, CLanClient *pLanLoginClient, WCHAR* pServerIP, INT serverPort, BOOL bServerNagleFlag, BOOL bTimeout, INT maxMessageSize, BYTE headerCode, BYTE staticKey, INT timeoutSec, INT runningThreadCount, INT workerThreadCount, INT maxClientCount) 
{
	*pChatServer = new CChattingServer;

	//((CChattingServer*)(*pChatServer))->SetLoginClient((CLanLoginClient*)pLanLoginClient);

	if ((*pChatServer)->Start(
		pServerIP, serverPort, serverPort, bServerNagleFlag, bTimeout, headerCode, staticKey, timeoutSec,runningThreadCount, workerThreadCount, maxClientCount
	) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ChatServerOn] Server Start is Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL MonitoringClientOn(CLanClient** pLanMonitoringClient, CNetServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bMonitoringClientNagleFlag, INT monitoringClientRunningThreadCount, INT monitoringClientWorkerThreadCount, INT chatServerNo)
{
	*pLanMonitoringClient = new CLanMonitoringClient;

	((CLanMonitoringClient*)*pLanMonitoringClient)->SetServerNo(chatServerNo);

	((CLanMonitoringClient*)*pLanMonitoringClient)->SetContentsPtr((CChattingServer*)*pChatServer);

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

BOOL ChatServerOff(CNetServer *pChatServer)
{
	if (pChatServer->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ChatServerOff] Server Stop is Failed");

		return FALSE;
	}

	delete pChatServer;

	return TRUE;
}

BOOL MonitoringClientOff(CLanClient *pMonitoringClient)
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