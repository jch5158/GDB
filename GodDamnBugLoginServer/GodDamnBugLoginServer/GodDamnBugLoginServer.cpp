#include "stdafx.h"
#include "CServerController.h"

namespace serverconfig
{
	constexpr DWORD MAX_IP_LENGTH = 50;
}

BOOL SetupLogSystem(void);

BOOL ParseNetLoginServer(WCHAR* pServerIP, WCHAR* pGameServerIP, WCHAR* pChatServerIP, INT* pServerPort, INT* pGameServerPort, INT* pChatServerPort, INT* pbWhiteIPModeFlag, INT* pMaxMessageSize, INT* pbNagleFlag, INT* pHeaderCode, INT* pStaticKey, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pMaxClientCount);

BOOL ParseLanLoginServer(WCHAR* pServerIP, INT* pServerPort, INT* pbNagleFlag, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pMaxClientCount);

BOOL ParseLanMonitoringClient(WCHAR* pServerIP, INT* pServerPort, INT* pbNagleFlag, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pServerNo);

BOOL NetLoginServerOn(CNetServer** pLoginServer, WCHAR* pServerIP, WCHAR* pGameServerIP, WCHAR* pChatServerIP, INT serverPort, INT gameServerPort, INT chatServerPort, INT bWhiteIPModeFlag, INT maxMessageSize, INT bNagleFlag, INT headerCode, INT staticKey, INT runningThreadCount, INT workerThreadCount, INT maxClientCount);

BOOL LanLoginServerOn(CLanServer** pLanLoginServer, CNetServer* pNetLoginServer, WCHAR* pServerIP, INT serverPort, INT bNagleFlag, INT runnignThreadCount, INT workerThreadCount, INT maxClientCount);

BOOL LanMonitoringClientOn(CLanClient** pLanMonitoringClient, CNetServer* pNetLoginServer, WCHAR* pServerIP, INT serverPort, INT bNagleFlag, INT runnignThreadCount, INT workerThreadCount, INT serverNo);

BOOL NetLoginServerOff(CNetServer** pLoginServer);

BOOL LanLoginServerOff(CLanServer** pLoginServer);

BOOL LanMonitoringClientOff(CLanClient** pLanClient);

INT main()
{
	timeBeginPeriod(1);

	CCrashDump::GetInstance();

	setlocale(LC_ALL, "");

	CTLSPerformanceProfiler::SetPerformanceProfiler(L"GodDamnBugLoginServer", 15);

	CNetServer* pLoginServer = nullptr;

	CLanServer* pLanLoginServer = nullptr;

	CLanClient* pLanMonitoringClient = nullptr;

	WCHAR serverIP[serverconfig::MAX_IP_LENGTH] = { 0, };

	WCHAR gameServerIP[serverconfig::MAX_IP_LENGTH] = { 0, };

	WCHAR chatServerIP[serverconfig::MAX_IP_LENGTH] = { 0, };

	WCHAR lanLoginServerIP[serverconfig::MAX_IP_LENGTH] = { 0, };

	WCHAR lanMonitoringClientIP[serverconfig::MAX_IP_LENGTH] = { 0, };

	INT serverPort;

	INT gameServerPort;

	INT chatServerPort;

	INT lanLoginServerPort;

	INT lanMonitoringClientPort;

	INT bWhiteIPModeFlag;

	INT maxMessageSize;

	INT bNagleFlag;

	INT headerCode;

	INT staticKey;

	INT runningThreadCount;

	INT workerThreadCount;

	INT maxClientCount;

	INT bLanLoginServerNagleFlag;

	INT lanLoginServerMaxClientCount;

	INT lanLoginRunningThreadCount;

	INT lanLoginWorkerThreadCount;


	INT bLanMonitoringNagleFlag;

	INT serverNo;

	INT lanMonitoringRunningThreadCount;

	INT lanMonitoringWorkerThreadCount;


	CCrashDump::GetInstance();

	do
	{
		if (SetupLogSystem() == FALSE)
		{
			break;
		}

		if (ParseNetLoginServer(serverIP, gameServerIP, chatServerIP, &serverPort, &gameServerPort, &chatServerPort, &bWhiteIPModeFlag, &maxMessageSize, &bNagleFlag, &headerCode, &staticKey, &runningThreadCount, &workerThreadCount, &maxClientCount) == FALSE)
		{
			break;
		}

		if (ParseLanLoginServer(lanLoginServerIP, &lanLoginServerPort, &bLanLoginServerNagleFlag, &lanLoginRunningThreadCount, &lanLoginWorkerThreadCount, &lanLoginServerMaxClientCount) == FALSE)
		{
			break;
		}

		if (ParseLanMonitoringClient(lanMonitoringClientIP, &lanMonitoringClientPort, &bLanMonitoringNagleFlag, &lanMonitoringRunningThreadCount, &lanMonitoringWorkerThreadCount, &serverNo) == FALSE)
		{
			break;
		}

		if (NetLoginServerOn(&pLoginServer, serverIP, gameServerIP, chatServerIP, serverPort, gameServerPort, chatServerPort, bWhiteIPModeFlag, maxMessageSize, bNagleFlag, headerCode, staticKey, runningThreadCount, workerThreadCount, maxClientCount) == FALSE)
		{
			break;
		}

		if (LanLoginServerOn(&pLanLoginServer, pLoginServer, lanLoginServerIP, lanLoginServerPort, bLanLoginServerNagleFlag, lanLoginRunningThreadCount, lanLoginWorkerThreadCount, lanLoginServerMaxClientCount) == FALSE)
		{
			break;
		}

		if (LanMonitoringClientOn(&pLanMonitoringClient, pLoginServer, lanMonitoringClientIP, lanMonitoringClientPort, bLanMonitoringNagleFlag, lanMonitoringRunningThreadCount, lanMonitoringWorkerThreadCount, serverNo) == FALSE)
		{
			break;
		}

		CServerController serverControler;

		serverControler.SetNetLoginServer((CNetLoginServer*)pLoginServer);

		serverControler.SetLanLoginServer((CLanLoginServer*)pLanLoginServer);

		serverControler.SetLanMonitoringClient((CLanMonitoringClient*)pLanMonitoringClient);

		while (serverControler.GetShutdownFlag() == FALSE)
		{
			serverControler.ServerControling();

			Sleep(1000);
		}
		if (LanLoginServerOff(&pLanLoginServer) == FALSE)
		{
			break;
		}

		if (LanMonitoringClientOff(&pLanMonitoringClient) == FALSE)
		{
			break;
		}

		if (NetLoginServerOff(&pLoginServer) == FALSE)
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
	if (CSystemLog::GetInstance()->SetLogDirectory(L"LoginServer Log") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[SetupLogSystem] SetLogDirectory Failed");

		return FALSE;
	}

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[SetupLogSystem] Config File Lod Failed");

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
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[SetupLogSystem] LogLevel Value Error : %s", buffer);

		return FALSE;
	}

	if (CSystemLog::GetInstance()->SetLogLevel(logLevel) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[SetupLogSystem] SetLogLevel is Failed : %d", (DWORD)logLevel);

		return FALSE;
	}

	return TRUE;
}


BOOL ParseNetLoginServer(WCHAR* pServerIP, WCHAR* pGameServerIP, WCHAR* pChatServerIP, INT* pServerPort, INT* pGameServerPort, INT* pChatServerPort, INT* pbWhiteIPModeFlag, INT* pMaxMessageSize, INT* pbNagleFlag, INT* pHeaderCode, INT* pStaticKey, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pMaxClientCount)
{
	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] Config File Lod Failed");

		return FALSE;
	}


	if (parser.GetNamespaceString(L"NET_LOGIN_SERVER", L"SERVER_IP", pServerIP, serverconfig::MAX_IP_LENGTH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] SERVER_IP Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"NET_LOGIN_SERVER", L"GAME_SERVER_IP", pGameServerIP, serverconfig::MAX_IP_LENGTH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] GAME_SERVER_IP Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"NET_LOGIN_SERVER", L"CHAT_SERVER_IP", pChatServerIP, serverconfig::MAX_IP_LENGTH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] CHAT_SERVER_IP Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] SERVER_PORT Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"GAME_SERVER_PORT", pGameServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] GAME_SERVER_PORT Config Failed");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"CHAT_SERVER_PORT", pChatServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] CHAT_SERVER_PORT Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"WHITE_IP_MODE", pbWhiteIPModeFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] WHITE_IP_MODE Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"MAX_MESSAGE_SIZE", pMaxMessageSize) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] MAX_MESSAGE_SIZE Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"NAGLE_OPTION", pbNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] NAGLE_OPTION Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"HEADER_CODE", pHeaderCode) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] HEADER_CODE Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"STATIC_KEY", pStaticKey) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] STATIC_KEY Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"RUNNING_THREAD", pRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] RUNNING_THREAD Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"WORKER_THREAD", pWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] WORKER_THREAD Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_LOGIN_SERVER", L"MAX_CLIENT", pMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLoginServer] MAX_CLIENT Config Failed");

		return FALSE;
	}

	return TRUE;
}


BOOL ParseLanLoginServer(WCHAR* pServerIP, INT* pServerPort, INT* pbNagleFlag, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pMaxClientCount)
{
	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] Config File Lod Failed");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"LAN_LOGIN_SERVER", L"SERVER_IP", pServerIP, serverconfig::MAX_IP_LENGTH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] LAN_LOGIN_SERVER Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_SERVER", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] SERVER_PORT Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_SERVER", L"NAGLE_OPTION", pbNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] NAGLE_OPTION Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_SERVER", L"RUNNING_THREAD", pRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] RUNNING_THREAD Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_SERVER", L"WORKER_THREAD", pWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] WORKER_THREAD Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_LOGIN_SERVER", L"MAX_CLIENT", pMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanLoginServer] MAX_CLIENT Config Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL ParseLanMonitoringClient(WCHAR* pServerIP, INT* pServerPort, INT* pbNagleFlag, INT* pRunningThreadCount, INT* pWorkerThreadCount, INT* pServerNo)
{
	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] Config File Lod Failed");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"LAN_MONITORING_CLIENT", L"SERVER_IP", pServerIP, serverconfig::MAX_IP_LENGTH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] SERVER_IP Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"SERVER_PORT", pServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] SERVER_PORT Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"NAGLE_OPTION", pbNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] NAGLE_OPTION Config Failed");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"RUNNING_THREAD", pRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] RUNNING_THREAD Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"WORKER_THREAD", pWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] WORKER_THREAD Config Failed");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_CLIENT", L"SERVER_NUMBER", pServerNo) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[ParseLanMonitoringClient] SERVER_NUMBER Config Failed");

		return FALSE;
	}

	return TRUE;
}



BOOL NetLoginServerOn(CNetServer** pLoginServer, WCHAR* pServerIP, WCHAR* pGameServerIP, WCHAR* pChatServerIP, INT serverPort, INT gameServerPort, INT chatServerPort, INT bWhiteIPModeFlag, INT maxMessageSize, INT bNagleFlag, INT headerCode, INT staticKey, INT runningThreadCount, INT workerThreadCount, INT maxClientCount)
{
	*pLoginServer = new CNetLoginServer;

	if (((CNetLoginServer*)*pLoginServer)->SetGameServerIP(pGameServerIP) == FALSE)
	{
		return FALSE;
	}

	((CNetLoginServer*)*pLoginServer)->SetGameServerPort(gameServerPort);

	if (((CNetLoginServer*)*pLoginServer)->SetChatServerIP(pChatServerIP) == FALSE)
	{
		return FALSE;
	}

	((CNetLoginServer*)*pLoginServer)->SetChatServerPort(chatServerPort);

	((CNetLoginServer*)*pLoginServer)->SetWhiteModeFlag(bWhiteIPModeFlag);

	if ((*pLoginServer)->Start(pServerIP, serverPort, maxMessageSize, bNagleFlag, headerCode, staticKey, runningThreadCount, workerThreadCount, maxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[LoginServerOn] Start Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL LanLoginServerOn(CLanServer** pLanLoginServer, CNetServer* pNetLoginServer, WCHAR* pServerIP, INT serverPort, INT bNagleFlag, INT runnignThreadCount, INT workerThreadCount, INT maxClientCount)
{
	*pLanLoginServer = new CLanLoginServer;

	((CLanLoginServer*)*pLanLoginServer)->SetNetLoginServer((CNetLoginServer*)pNetLoginServer);

	if ((*pLanLoginServer)->Start(pServerIP, serverPort, bNagleFlag, runnignThreadCount, workerThreadCount, maxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[LanLoginServerOn] Start Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL LanMonitoringClientOn(CLanClient** pLanMonitoringClient, CNetServer* pNetLoginServer, WCHAR* pServerIP, INT serverPort, INT bNagleFlag, INT runnignThreadCount, INT workerThreadCount, INT serverNo)
{
	*pLanMonitoringClient = new CLanMonitoringClient;

	((CLanMonitoringClient*)*pLanMonitoringClient)->SetLoginServerPtr((CNetLoginServer*)pNetLoginServer);

	((CLanMonitoringClient*)*pLanMonitoringClient)->SetServerNo(serverNo);

	CHardwareProfiler::SetHardwareProfiler(TRUE, FALSE, FALSE, FALSE, FALSE, nullptr);

	if ((*pLanMonitoringClient)->Start(pServerIP, serverPort, bNagleFlag, runnignThreadCount, workerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[LanMonitoringClientOn] Start Failed");

		return FALSE;
	}

	return TRUE;
}


BOOL NetLoginServerOff(CNetServer** pLoginServer)
{
	if ((*pLoginServer)->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[NetLoginServerOff] Stop Failed");

		return FALSE;
	}

	delete* pLoginServer;

	return TRUE;
}


BOOL LanLoginServerOff(CLanServer** pLoginServer)
{
	if ((*pLoginServer)->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[LanLoginServerOff] Stop Failed");

		return FALSE;
	}

	delete* pLoginServer;

	return TRUE;
}

BOOL LanMonitoringClientOff(CLanClient** pLanClient)
{
	if ((*pLanClient)->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LoginServer", L"[LanMonitoringClientOff] Stop Failed");

		return FALSE;
	}

	delete* pLanClient;

	return TRUE;
}