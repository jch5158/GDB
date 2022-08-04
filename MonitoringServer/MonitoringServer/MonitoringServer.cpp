#include "stdafx.h"

BOOL SetupLogSystem(void);



BOOL ParseNetServerConfigFile(WCHAR* pNetServerIP, INT* pNetServerPort, INT* pNetMaxMessageSize, INT* pbNetNagleFlag, INT* pHeaderCode, INT* pStaticKey, INT* pNetRunningThreadCound, INT* pNetWorkerThreadCount, INT* pNetMaxClientCount, CHAR* pLoginKey, DWORD loginKeyCb);

BOOL NetServerOn(CNetServer** pNetServer, WCHAR* pNetServerIP, INT netServerPort, INT netMaxMessageSize, INT bNetNagleFlag, INT headerCode, INT staticKey, INT netRunningThreadCound, INT netWorkerThreadCount, INT netMaxClientCount, CHAR* pLoginKey);

BOOL NetServerOff(CNetServer** pNetServer);

BOOL ParseLanServerConfigFile(WCHAR* pLanServerIP, INT* pLanServerPort, INT* pbLanNagleFlag, INT* pLanRunningThreadCount, INT* pLanWorkerThreadCount, INT* pLanMaxClientCount, INT* pGameServerNo, INT* pChatServerNo, INT *pLoginServerNo, WCHAR* pLanCardName);

BOOL LanServerOn(CLanServer** pLanServer, CNetServer* pNetServer, WCHAR* pLanServerIP, INT lanServerPort, INT bLanNagleFlag, INT lanRunningThreadCount, INT lanWorkerThreadCount, INT lanMaxClientCount, INT gameServerNo, INT loginServerNo, INT chatServerNo, WCHAR* pLanCardName);

BOOL LanServerOff(CLanServer** pLanServer);


INT main()
{
	CCrashDump::GetInstance();

	timeBeginPeriod(1);

	_wsetlocale(LC_ALL, L"");

	do
	{
		CNetServer* pNetServer;

		WCHAR netServerIP[MAX_PATH] = { 0, };

		INT netServerPort;

		INT netMaxMessageSize;

		INT bNetNagleFlag;

		INT headerCode;

		INT staticKey;

		INT netRunningThreadCount;

		INT netWorkerThreadCount;

		INT netMaxClientCount;

		CHAR loginKey[MAX_PATH] = { 0, };

		CLanServer* pLanServer;

		WCHAR lanServerIP[MAX_PATH] = { 0, };

		INT lanServerPort;

		INT bLanNagleFlag;

		INT lanRunningThreadCount;

		INT lanWorkerThreadCount;

		INT lanMaxClientCount;

		INT gameServerNo;

		INT chatServerNo;

		INT loginServerNo;

		WCHAR lanCardName[MAX_PATH] = { 0, };

		if (SetupLogSystem() == FALSE)
		{
			break;
		}

		if (ParseNetServerConfigFile(netServerIP, &netServerPort, &netMaxMessageSize, &bNetNagleFlag, &headerCode, &staticKey, &netRunningThreadCount, &netWorkerThreadCount, &netMaxClientCount, loginKey, _countof(loginKey)) == FALSE)
		{
			break;
		}

		if(ParseLanServerConfigFile(lanServerIP, &lanServerPort,&bLanNagleFlag, &lanRunningThreadCount,&lanWorkerThreadCount,&lanMaxClientCount,&gameServerNo,&chatServerNo,&loginServerNo ,lanCardName) == FALSE)
		{
			break;
		}

		if (NetServerOn(&pNetServer, netServerIP, netServerPort, netMaxMessageSize, bNetNagleFlag, headerCode, staticKey, netRunningThreadCount, netWorkerThreadCount, netMaxClientCount, loginKey) == FALSE)
		{
			break;
		}

		if (LanServerOn(&pLanServer, pNetServer, lanServerIP, lanServerPort, bLanNagleFlag, lanRunningThreadCount, lanWorkerThreadCount, lanMaxClientCount, gameServerNo, chatServerNo, loginServerNo, lanCardName) == FALSE)
		{
			break;
		}

		CServerController controller;

		controller.SetLanMonitoringServer((CLanMonitoringServer*)pLanServer);

		controller.SetNetMonitoringServer((CNetMonitoringServer*)pNetServer);

		while (controller.GetShutdounFlag() == FALSE)
		{
			controller.ServerControling();

			Sleep(1000);
		}

		if (NetServerOff(&pNetServer) == FALSE)
		{
			break;
		}

		if (LanServerOff(&pLanServer) == FALSE)
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
	if (CSystemLog::GetInstance()->SetLogDirectory(L"MonitoringServer Log") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[SetupLogSystem] SetLogDirectory is Failed");

		return FALSE;
	}

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[SetupLogSystem] ServerConfig is Not Found");

		return FALSE;
	}

	WCHAR buffer[MAX_PATH] = { 0, };

	if (parser.GetString(L"LOG_LEVEL", buffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[SetupLogSystem] LOG_LEVEL is Not Found");

		return FALSE;
	}

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
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[SetupLogSystem] LOG_LEVEL Value Error : %s", buffer);

		return FALSE;
	}

	if (CSystemLog::GetInstance()->SetLogLevel(logLevel) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[SetupLogSystem] SetLogLevel is Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL ParseNetServerConfigFile(WCHAR* pNetServerIP, INT* pNetServerPort, INT* pNetMaxMessageSize, INT* pbNetNagleFlag, INT* pHeaderCode, INT* pStaticKey, INT* pNetRunningThreadCound, INT* pNetWorkerThreadCount, INT* pNetMaxClientCount, CHAR* pLoginKey, DWORD loginKeyCb)
{
	WCHAR pBuffer[MAX_PATH] = { 0, };

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] ServerConfig.ini Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"NET_MONITORING_SERVER", L"SERVER_IP", pNetServerIP, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] SERVER_IP Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_MONITORING_SERVER", L"SERVER_PORT", pNetServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] SERVER_PORT Not Found");

		return FALSE;
	}


	if (parser.GetNamespaceValue(L"NET_MONITORING_SERVER", L"NAGLE_OPTION", pbNetNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] NAGLE_OPTION Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_MONITORING_SERVER", L"MAX_MESSAGE_SIZE", pNetMaxMessageSize) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] MAX_MESSAGE_SIZE Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"NET_MONITORING_SERVER", L"HEADER_CODE", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] HEADER_CODE Not Found");

		return FALSE;
	}

	*pHeaderCode = (BYTE)wcstol(pBuffer, NULL, 10);

	if (parser.GetNamespaceString(L"NET_MONITORING_SERVER", L"STATIC_KEY", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] STATIC_KEY Not Found");

		return FALSE;
	}

	 *pStaticKey = (BYTE)wcstol(pBuffer, NULL, 10);

	if (parser.GetNamespaceValue(L"NET_MONITORING_SERVER", L"RUNNING_THREAD", pNetRunningThreadCound) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] RUNNING_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_MONITORING_SERVER", L"WORKER_THREAD", pNetWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] WORKER_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"NET_MONITORING_SERVER", L"MAX_CLIENT", pNetMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] MAX_CLIENT Not Found");

		return FALSE;
	}


	if (parser.GetNamespaceString(L"NET_MONITORING_SERVER", L"LOGIN_KEY", pBuffer, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] LOGIN_KEY Not Found");

		return FALSE;
	}

	WideCharToMultiByte(CP_ACP, 0, pBuffer, wcslen(pBuffer), pLoginKey, loginKeyCb, NULL, NULL);

	return TRUE;
}


BOOL NetServerOn(CNetServer** pNetServer, WCHAR* pNetServerIP, INT netServerPort, INT netMaxMessageSize, INT bNetNagleFlag, INT headerCode, INT staticKey, INT netRunningThreadCound, INT netWorkerThreadCount, INT netMaxClientCount, CHAR* pLoginKey)
{
	*pNetServer = new CNetMonitoringServer;

	((CNetMonitoringServer*)*pNetServer)->SetLoginKey(pLoginKey);

	if ((*pNetServer)->Start(
		pNetServerIP, netServerPort, netMaxMessageSize, bNetNagleFlag, headerCode, staticKey, netRunningThreadCound, netWorkerThreadCount, netMaxClientCount
	) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[NetServerOn] Net Server Start is Failed");

		return FALSE;
	}

	return TRUE;
}


BOOL NetServerOff(CNetServer** pNetServer)
{
	if (((CNetMonitoringServer*)*pNetServer)->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[NetServerOff] Net Server Stop is Failed");

		return FALSE;
	}

	delete* pNetServer;

	return TRUE;
}


BOOL ParseLanServerConfigFile(WCHAR* pLanServerIP, INT* pLanServerPort, INT* pbLanNagleFlag, INT* pLanRunningThreadCount, INT* pLanWorkerThreadCount, INT* pLanMaxClientCount, INT* pGameServerNo, INT* pChatServerNo, INT *pLoginServerNo,WCHAR* pLanCardName)
{

	CParser parser;

	if (parser.LoadFile(L"Config\\ServerConfig.ini") == FALSE)
	{
		return FALSE;
	}

	if (parser.GetNamespaceString(L"LAN_MONITORING_SERVER", L"SERVER_IP", pLanServerIP, MAX_PATH) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] SERVER_IP Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"SERVER_PORT", pLanServerPort) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] SERVER_PORT Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"NAGLE_OPTION", pbLanNagleFlag) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] NAGLE_OPTION Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"RUNNING_THREAD", pLanRunningThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] RUNNING_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"WORKER_THREAD", pLanWorkerThreadCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] WORKER_THREAD Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"MAX_CLIENT", pLanMaxClientCount) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] MAX_CLIENT Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"GAME_SERVER_NO", pGameServerNo) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] GAME_SERVER_NO Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"CHAT_SERVER_NO", pChatServerNo) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] CHAT_SERVER_NO Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceValue(L"LAN_MONITORING_SERVER", L"LOGIN_SERVER_NO", pLoginServerNo) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] LOGIN_SERVER_NO Not Found");

		return FALSE;
	}

	if (parser.GetNamespaceString(L"LAN_MONITORING_SERVER", L"LAN_CARD_NAME", pLanCardName, MAX_PATH), NULL, NULL)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[ParseNetServerConfigFile] LOGIN_KEY Not Found");

		return FALSE;
	}

	return TRUE;
}





BOOL LanServerOn(CLanServer** pLanServer, CNetServer* pNetServer, WCHAR* pLanServerIP, INT lanServerPort, INT bLanNagleFlag, INT lanRunningThreadCount, INT lanWorkerThreadCount, INT lanMaxClientCount, INT gameServerNo, INT chatServerNo,INT loginServerNo ,WCHAR* pLanCardName)
{
	*pLanServer = new CLanMonitoringServer;

	((CLanMonitoringServer*)*pLanServer)->SetNetMonitoringServer((CNetMonitoringServer*)pNetServer);

	((CLanMonitoringServer*)*pLanServer)->SetGameServerNo(gameServerNo);

	((CLanMonitoringServer*)*pLanServer)->SetChatServerNo(chatServerNo);

	((CLanMonitoringServer*)*pLanServer)->SetLoginServerNo(loginServerNo);

	CHardwareProfiler::SetHardwareProfiler(FALSE, TRUE, TRUE, TRUE, TRUE, pLanCardName);

	if ((*pLanServer)->Start(
		pLanServerIP,lanServerPort,bLanNagleFlag,lanRunningThreadCount,lanWorkerThreadCount,lanMaxClientCount
	) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[LanServerOn] LanMonitoringServer Start is Failed");

		return FALSE;
	}

	return TRUE;
}

BOOL LanServerOff(CLanServer** pLanServer)
{
	if ((*pLanServer)->Stop() == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"MonitoringServer", L"[LAnServerOff] LanMonitoringServer Stop is Failed");

		return FALSE;
	}

	delete* pLanServer;

	return TRUE;
}
