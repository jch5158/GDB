#include "stdafx.h"


BOOL SetupLogSystem(void);

BOOL ParseChatServer(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbServerNagleFlag, INT* pMaxMessageSize, BYTE* pHeaderCode, BYTE* pStaticKey, INT* pServerRunningThreadCount, INT* pServerWorkerThreadCount, INT* pMaxClientCount, INT* pbWhiteIPModeFlag);

BOOL ParseMonitoringClient(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbClientNagleFlag, INT* pClientRunningThreadCount, INT* pClientWorkerThreadCount, INT* pServerNo);

BOOL ChatServerOn(CNetServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bServerNagleFlag, INT maxMessageSize, BYTE headerCode, BYTE staticKey, INT runningThreadCount, INT workerThreadCount, INT maxClientCount, INT bWhiteModeFlag);

BOOL MonitoringClientOn(CLanClient** pLanMonitoringClient, CNetServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bMonitoringClientNagleFlag, INT monitoringClientRunningThreadCount, INT monitoringClientWorkerThreadCount, INT chatServerNo);

BOOL ChatServerOff(CNetServer* pChatServer);

BOOL MonitoringClientOff(CLanClient* pMonitoringClient);

INT wmain()
{
std::cout << "ASDF";

return 1;

	CHardwareProfiler::SetHardwareProfiler(FALSE, TRUE, FALSE, FALSE, FALSE, nullptr);

	CTLSPerformanceProfiler::SetPerformanceProfiler(L"ChatServer", 10);

	CNetServer* pChatServer = nullptr;

	CLanClient* pMonitoringClient = nullptr;

	WCHAR chatServerIP[MAX_PATH] = { 0, };

	INT chatServerPort;

	INT bChatServerNagleFlag;

	INT maxMessageSize;

	BYTE headerCode;

	BYTE staticKey;

	INT bTimeout;

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

	INT bWhiteIPModeFlag;

	CCrashDump::GetInstance();

	timeBeginPeriod(1);

	setlocale(LC_ALL, "");

	//CPerformanceProfiler::SetPerformanceProfiler(L"ChattingServer", 15);

	do
	{
		if (SetupLogSystem() == FALSE)
		{
			break;
		}

		if (ParseChatServer(chatServerIP, _countof(chatServerIP), &chatServerPort, &bChatServerNagleFlag, &maxMessageSize, &headerCode, &staticKey, &chatServerRunningThreadCount, &chatServerWorkerThreadCount, &maxClientCount, &bWhiteIPModeFlag) == FALSE)
		{
			break;
		}

		//if (ParseMonitoringClient(monitoringServerIP, _countof(monitoringServerIP), &monitoringServerPort, &bMonitoringClientNagleFlag, &monitoringClientRunningThreadCount, &monitoringClientWorkerThreadCount, &chatServerNo) == FALSE)
		//{
		//	break;
		//}

		if (ChatServerOn(&pChatServer, chatServerIP, chatServerPort, bChatServerNagleFlag, maxMessageSize, headerCode, staticKey, chatServerRunningThreadCount, chatServerWorkerThreadCount, maxClientCount, bWhiteIPModeFlag) == FALSE)
		{
			break;
		}

		//if (MonitoringClientOn(&pMonitoringClient, &pChatServer, monitoringServerIP, monitoringServerPort, bMonitoringClientNagleFlag, monitoringClientRunningThreadCount, monitoringClientWorkerThreadCount, chatServerNo) == FALSE)
		//{
		//	break;
		//}

		CServerController serverController;

		serverController.SetChatServerPtr((CChattingServer*)pChatServer);

		while (serverController.GetShutdownFlag() == FALSE)
		{
			serverController.ServerControling();

			Sleep(1000);
		}

		if (ChatServerOff(pChatServer) == FALSE)
		{
			break;
		}

		//if (MonitoringClientOff(pMonitoringClient) == FALSE)
		//{
		//	break;
		//}

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

	parser.GetNamespaceString(L"CHATTING_SERVER", L"LOG_LEVEL", buffer, MAX_PATH);

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

BOOL ParseChatServer(WCHAR* pServerIP, DWORD serverIPCb, INT* pServerPort, INT* pbServerNagleFlag, INT* pMaxMessageSize, BYTE* pHeaderCode, BYTE* pStaticKey, INT* pServerRunningThreadCount, INT* pServerWorkerThreadCount, INT* pMaxClientCount, INT *pbWhiteIPModeFlag)
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

	if (parser.GetNamespaceValue(L"CHATTING_SERVER", L"WHITE_IP_MODE", pbWhiteIPModeFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ParseServerConfigFile] WHITE_IP_MODE Not Found");

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


BOOL ChatServerOn(CNetServer** pChatServer, WCHAR* pServerIP, INT serverPort, BOOL bServerNagleFlag, INT maxMessageSize, BYTE headerCode, BYTE staticKey, INT runningThreadCount, INT workerThreadCount, INT maxClientCount, INT bWhiteModeFlag)
{
	*pChatServer = new CChattingServer;

	((CChattingServer*)*pChatServer)->SetWhiteIPModeFlag(bWhiteModeFlag);

	if ((*pChatServer)->Start(
		pServerIP, serverPort, serverPort, bServerNagleFlag, true, headerCode, staticKey, 40, runningThreadCount, workerThreadCount, maxClientCount
	) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ServerOn] Server Start is Failed");

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
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ServerOn] Lan Client Start is Failed");

		return FALSE;
	}

	return TRUE;
}


BOOL ChatServerOff(CNetServer* pChatServer)
{
	if (pChatServer->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ServerOff] Server Stop is Failed");

		return FALSE;
	}

	delete pChatServer;

	return TRUE;
}

BOOL MonitoringClientOff(CLanClient* pMonitoringClient)
{
	if (pMonitoringClient->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"ChattingServer", L"[ServerOff] Client Stop is Failed");

		return FALSE;
	}

	delete pMonitoringClient;

	return TRUE;
}
