#include "stdafx.h"

CLanMonitoringServer::CLanMonitoringServer(void)
	: mbGameServerLoginFlag(FALSE)
	, mbChatServerLoginFlag(FALSE)
	, mbLoginServerLoginFlag(FALSE)
	
	, mbUpdateThreadFlag(TRUE)
	, mbDBWriteThreadFlag(TRUE)

	, mGameServerNo(0)
	, mChatServerNo(0)
	, mLoginServerNo(0)

	, mUpdateThreadID(0)
	, mDBWriteThreadID(0)

	, mUpdateThreadHandle(INVALID_HANDLE_VALUE)
	, mDBWriteThreadHandle(INVALID_HANDLE_VALUE)
	
	, mpNetMonitoringServer(nullptr)
	
	, mServerProfile{0,}
	, mGameServerProfile{0,}
	, mChatServerProfile{0,}
	, mLoginServerProfile{0,}

	, mGameServerStartTime{0,}
	, mChatServerStartTime{0,}
	, mLoginServerStartTime{0,}

	, mServerNoCriticalSection{ 0, }
	, mDBConnector()
	, mServerNoMap()

{
	InitializeCriticalSection(&mServerNoCriticalSection);

	initializaServerProfile();

	initializeGameServerProfile();

	initializeChatServerProfile();

	initializeLoginServerProfile();
}

CLanMonitoringServer::~CLanMonitoringServer(void)
{
	closeUpdateThread();

	closeDBWriteThread();

	DeleteCriticalSection(&mServerNoCriticalSection);
}

void CLanMonitoringServer::OnClientJoin(UINT64 sessionID)
{

	return;
}

BOOL CLanMonitoringServer::OnStart(void)
{
	if (setupUpdateThread() == FALSE)
	{
		return FALSE;
	}

	if (setupDBWriteThread() == FALSE)
	{
		return FALSE;
	}

	// 스레드 생성될 때 까지 기다릴 필요는 있음
	// 하지만 해당 서버에서는 문제가 발생되지 않음

	return TRUE;
}

void CLanMonitoringServer::OnStartWorkerThread(void)
{

	return;
}

void CLanMonitoringServer::OnStartAcceptThread(void)
{

	return;
}

void CLanMonitoringServer::OnClientLeave(UINT64 sessionID)
{
	DWORD serverNo;

	findServerNo(sessionID, &serverNo);

	if (mGameServerNo == serverNo)
	{
		mbGameServerLoginFlag = FALSE;
	}
	else if (mChatServerNo == serverNo)
	{
		mbChatServerLoginFlag = FALSE;
	}
	else if (mLoginServerNo == serverNo)
	{
		mbLoginServerLoginFlag = FALSE;
	}

	eraseServerNo(sessionID);

	return;
}

void CLanMonitoringServer::OnRecv(UINT64 sessionID, CMessage* pMessage)
{
	WORD messageType;

	*pMessage >> messageType;

	if (recvProcedure(sessionID, messageType, pMessage) == FALSE)
	{
		Disconnect(sessionID);
	}

	return;
}

void CLanMonitoringServer::OnCloseWorkerThread(void)
{

	return;
}


void CLanMonitoringServer::OnCloseAcceptThread(void)
{

	return;
}

BOOL CLanMonitoringServer::OnConnectionRequest(const WCHAR* userIP, WORD userPort)
{

	return TRUE;
}

void CLanMonitoringServer::OnError(INT errorCode, const WCHAR* errorMessage)
{

	return;
}

void CLanMonitoringServer::OnStop(void)
{

	return;
}

void CLanMonitoringServer::SetNetMonitoringServer(CNetMonitoringServer* pNetMonitoringServer)
{
	mpNetMonitoringServer = pNetMonitoringServer;

	return;
}

void CLanMonitoringServer::SetGameServerNo(DWORD gameServerNo)
{
	mGameServerNo = gameServerNo;

	return;
}

void CLanMonitoringServer::SetChatServerNo(DWORD chatServerNo)
{
	mChatServerNo = chatServerNo;

	return;
}

void CLanMonitoringServer::SetLoginServerNo(DWORD loginServerNo)
{
	mLoginServerNo = loginServerNo;

	return;
}

BOOL CLanMonitoringServer::GetGameServerLoginFlag(void) const
{
	return mbGameServerLoginFlag;
}

BOOL CLanMonitoringServer::GetLoginServerLoginFlag(void) const
{
	return mbLoginServerLoginFlag;
}

BOOL CLanMonitoringServer::GetChatServerLoginFlag(void) const
{
	return mbChatServerLoginFlag;
}


DWORD CLanMonitoringServer::ExecuteUpdateThread(void* pParam)
{
	CLanMonitoringServer* pLanMonitoringServer = (CLanMonitoringServer*)pParam;

	pLanMonitoringServer->UpdateThread();

	return 1;
}

DWORD CLanMonitoringServer::ExecuteDBWriteThread(void* pParam)
{
	CLanMonitoringServer* pLanMonitoringServer = (CLanMonitoringServer*)pParam;

	pLanMonitoringServer->DBWriteThread();

	return 1;
}


void CLanMonitoringServer::UpdateThread(void)
{
	while (mbUpdateThreadFlag)
	{
		sendHardwareProfile();

		Sleep(1000);
	}

	return;
}

void CLanMonitoringServer::DBWriteThread(void)
{
	RECONNECT:

	if (mDBConnector.Connect((WCHAR*)L"127.0.0.1", 10950, (WCHAR*)L"logdb", (WCHAR*)L"chanhun", (WCHAR*)L"Cksgns123$") == FALSE)
	{
		if (mDBConnector.CheckReconnectErrorCode() == TRUE)
		{
			goto RECONNECT;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[DBWriteThread] Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	while (mbDBWriteThreadFlag)
	{
		// 5분 마다 저장
		Sleep(300000);

		DBWriteMonitoringData();

		initializaServerProfile();

		initializeGameServerProfile();

		initializeChatServerProfile();

		initializeLoginServerProfile();
	}

	mDBConnector.Disconnect();

	return;
}



BOOL CLanMonitoringServer::setupUpdateThread(void)
{
	mUpdateThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteUpdateThread, this, NULL, (UINT*)&mUpdateThreadID);
	if (mUpdateThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetMonitoringServer", L"[setupUpdateThread] Error Code : %d", GetLastError());

		return FALSE;
	}


	return TRUE;
}

BOOL CLanMonitoringServer::setupDBWriteThread(void)
{
	mDBWriteThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ExecuteDBWriteThread, this, NULL, (UINT*)&mDBWriteThreadID);
	if (mDBWriteThreadHandle == INVALID_HANDLE_VALUE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetMonitoringServer", L"[setupDBWriteThread] Error Code : %d", GetLastError());

		return FALSE;
	}

	return TRUE;
}

void CLanMonitoringServer::closeUpdateThread(void)
{
	mbUpdateThreadFlag = FALSE;

	if (WaitForSingleObject(mUpdateThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetMonitoringServer", L"[closeUpdateThread] Error Code : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mUpdateThreadHandle);

	return;
}

void CLanMonitoringServer::closeDBWriteThread(void)
{
	mbDBWriteThreadFlag = FALSE;

	if (WaitForSingleObject(mDBWriteThreadHandle, INFINITE) != WAIT_OBJECT_0)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"NetMonitoringServer", L"[closeDBWriteThread] Error Code : %d", GetLastError());

		CCrashDump::Crash();
	}

	CloseHandle(mDBWriteThreadHandle);

	return;
}


void CLanMonitoringServer::initializaServerProfile(void)
{
	ZeroMemory(&mServerProfile, sizeof(stServerProfile));

	mServerProfile.cpuProfileMin = INT_MAX;

	mServerProfile.nonpagedMemoryProfileMin = INT_MAX;

	mServerProfile.recvKBytesProfileMin = INT_MAX;

	mServerProfile.sendKBytesProfileMin = INT_MAX;

	mServerProfile.availableMemoryProfileMin = INT_MAX;

	return;
}

void CLanMonitoringServer::initializeGameServerProfile(void)
{
	ZeroMemory(&mGameServerProfile, sizeof(stGameServerProfile));

	mGameServerProfile.cpuProfileMin = INT_MAX;

	mGameServerProfile.memoryProfileMin = INT_MAX;

	mGameServerProfile.sessionCountProfileMin = INT_MAX;

	mGameServerProfile.authPlayerProfileMin = INT_MAX;

	mGameServerProfile.gamePlayerProfileMin = INT_MAX;

	mGameServerProfile.acceptTPSProfileMin = INT_MAX;

	mGameServerProfile.recvTPSProfileMin = INT_MAX;

	mGameServerProfile.sendTPSProfileMin = INT_MAX;

	mGameServerProfile.DBWriteTPSProfileMin = INT_MAX;

	mGameServerProfile.DBWriteQSizeProfileMin = INT_MAX;

	mGameServerProfile.authFPSProfileMin = INT_MAX;

	mGameServerProfile.gameFPSProfileMin = INT_MAX;

	mGameServerProfile.messagePoolProfileMin = INT_MAX;

	return;
}


void CLanMonitoringServer::initializeChatServerProfile(void)
{
	ZeroMemory(&mChatServerProfile, sizeof(stChatServerProfile));

	mChatServerProfile.cpuProfileMin = INT_MAX;

	mChatServerProfile.memoryProfileMin = INT_MAX;

	mChatServerProfile.sessionCountProfileMin = INT_MAX;

	mChatServerProfile.playerCountProfileMin = INT_MAX;

	mChatServerProfile.updateTPSProfileMin = INT_MAX;

	mChatServerProfile.messagePoolProfileMin = INT_MAX;

	mChatServerProfile.updateQSizeProfileMin = INT_MAX;

	return;
}

void CLanMonitoringServer::initializeLoginServerProfile(void)
{
	ZeroMemory(&mLoginServerProfile, sizeof(stLoginServerProfile));

	mLoginServerProfile.cpuProfileMin = INT_MAX;

	mLoginServerProfile.memoryProfileMin = INT_MAX;

	mLoginServerProfile.sessionCountProfileMin = INT_MAX;

	mLoginServerProfile.authTPSProfileMin = INT_MAX;

	mLoginServerProfile.messagePoolProfileMin = INT_MAX;

	return;
}


void CLanMonitoringServer::getTime(tm* pNowTime)
{	
	INT64 time64 = NULL;

	_time64(&time64);

	_localtime64_s(pNowTime, &time64);

	return;
}


void CLanMonitoringServer::setServerStartTime(WCHAR* pBuffer, INT bufferCb)
{
	HRESULT retval;

	tm nowTime = { 0, };

	getTime(&nowTime);


	retval = StringCbPrintfW(pBuffer, bufferCb, L"%04d-%02d-%02d %02d:%02d:%02d", nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
	if (FAILED(retval))
	{
		CCrashDump::Crash();
	}

	return;
}


BOOL CLanMonitoringServer::checkProfileTable(WCHAR* pTableName)
{
	RECONNECT:

	//테이블 생성여부 확인하는 쿼리
	if (mDBConnector.Query((WCHAR*)L"SELECT 1 FROM Information_schema.tables WHERE table_schema = 'logdb' AND table_name = '%s';", pTableName) == FALSE)
	{
		if (mDBConnector.CheckReconnectErrorCode() == TRUE)
		{
			mDBConnector.Reconnect();

			goto RECONNECT;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[sendMonitoringQuery] Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	mDBConnector.StoreResult();

	if (mDBConnector.FetchRow() == nullptr)
	{	
		return FALSE;
	}

	mDBConnector.FreeResult();

	return TRUE;
}

void CLanMonitoringServer::getServerProfileTableName(WCHAR* pBuffer, DWORD bufferCb)
{
	tm nowTime = { 0, };

	getTime(&nowTime);

	StringCbPrintf(pBuffer, bufferCb, L"%d%02d_monitoring", nowTime.tm_year + 1900, nowTime.tm_mon + 1);

	return;
}




void CLanMonitoringServer::setServerProfileTable(WCHAR* pTableName)
{
	
RECONNECT:

	if (mDBConnector.Query((WCHAR*)
		L"CREATE TABLE `logdb`.`%s` ("
		"`monitoring_no` BIGINT NOT NULL AUTO_INCREMENT,"
		"`server_no` INT NOT NULL,"
		"`server_start_time` TIMESTAMP NOT NULL,"
		"`monitoring_time` TIMESTAMP NOT NULL DEFAULT NOW(),"
		"`data_type` INT NOT NULL,"
		"`data` INT NOT NULL,"
		"PRIMARY KEY(`monitoring_no`),"
		"INDEX `server_no` (`server_no` ASC) VISIBLE,"
		"INDEX `server_start_time` (`server_start_time` ASC) INVISIBLE,"
		"INDEX `monitoring_time` (`monitoring_time` ASC) INVISIBLE,"
		"INDEX `data_type` (`data_type` ASC) INVISIBLE);"
		, pTableName) == FALSE)
	{
		mDBConnector.Reconnect();

		goto RECONNECT;		
	}

	return;
}

void CLanMonitoringServer::DBWriteMonitoringData(void)
{
	WCHAR pTableName[MAX_PATH];

	getServerProfileTableName(pTableName, sizeof(WCHAR) * MAX_PATH);

	if (checkProfileTable(pTableName) == FALSE)
	{
		setServerProfileTable(pTableName);
	}

	DBWriteServerProfile(pTableName);

	DBWriteGameServerProfile(pTableName);

	DBWriteChatServerProfile(pTableName);

	DBWriteLoginServerProfile(pTableName);

	return;
}

void CLanMonitoringServer::DBWriteServerProfile(WCHAR* pTableName)
{

	for (INT saveType = (INT)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL; saveType <= (INT)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY; ++saveType)
	{
		switch (saveType)
		{
		case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:
			
			//0-0-0 0:0:0
			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::CpuProfileAvr, mServerProfile.cpuProfileAvr);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::CpuProfieMin, mServerProfile.cpuProfileMin);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::CpuProfileMax, mServerProfile.cpuProfileMax);


			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:
			
			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::NonpagedMemoryProfileAvr, mServerProfile.nonpagedMemoryProfileAvr);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::NonpagedMemoryProfileMin, mServerProfile.nonpagedMemoryProfileMin);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::NonpagedMemoryProfileMax, mServerProfile.nonpagedMemoryProfileMax);

			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:
			
			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::RecvBytesProfileAvr, mServerProfile.recvKBytesProfileAvr);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::RecvBytesProfileMin, mServerProfile.recvKBytesProfileMin);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::RecvBytesProfileMax, mServerProfile.recvKBytesProfileMax);

			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::SendBytesProfileAvr, mServerProfile.sendKBytesProfileAvr);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::SendBytesProfileMin, mServerProfile.sendKBytesProfileMin);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::SendBytesProfileMax, mServerProfile.sendKBytesProfileMax);

			
			break;
		case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::AvailableMemoryProfileAvr, mServerProfile.availableMemoryProfileAvr);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::AvailableMemoryProfileMin, mServerProfile.availableMemoryProfileMin);

			sendMonitoringQuery(pTableName, 1, (WCHAR*)L"0-0-0 0:0:0", (INT)eProfileDataType::AvailableMemoryProfileMax, mServerProfile.availableMemoryProfileMax);

			break;
		default:

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[saveServerProfile] saveType : %d", saveType);

			CCrashDump::Crash();
		}
	}
	return;
}

void CLanMonitoringServer::DBWriteGameServerProfile(WCHAR* pTableName)
{
	// 게임 서버가 가동중이지 않으면 저장하지 않는다. 
	if (mGameServerProfile.bServerOnFlag == FALSE)
	{
		return;
	}

	for(INT saveType  = (INT)dfMONITOR_DATA_TYPE_GAME_SERVER_CPU; saveType <= (INT)dfMONITOR_DATA_TYPE_GAME_PACKET_POOL; ++saveType)
	{ 
		switch (saveType)
		{
		case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
		
			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerCpuProfileAvr, mGameServerProfile.cpuProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerCpuProfileMin, mGameServerProfile.cpuProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerCpuProfileMax, mGameServerProfile.cpuProfileMax);

			break;
		
		case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerMemoryProfileAvr, mGameServerProfile.memoryProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerMemoryProfileMin, mGameServerProfile.memoryProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerMemoryProfileMax, mGameServerProfile.memoryProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_SESSION:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerSessionCountProfileAvr, mGameServerProfile.sessionCountProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerSessionCountProfileMin, mGameServerProfile.sessionCountProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerSessionCountProfileMax, mGameServerProfile.sessionCountProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAuthPlayerProfileAvr, mGameServerProfile.authPlayerProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAuthPlayerProfileMin, mGameServerProfile.authPlayerProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAuthPlayerProfileMax, mGameServerProfile.authPlayerProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerGamePlayerProfileAvr, mGameServerProfile.gamePlayerProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerGamePlayerProfileMin, mGameServerProfile.gamePlayerProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerGamePlayerProfileMax, mGameServerProfile.gamePlayerProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAcceptTPSProfileAvr, mGameServerProfile.acceptTPSProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAcceptTPSProfileMin, mGameServerProfile.acceptTPSProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAcceptTPSProfileMax, mGameServerProfile.acceptTPSProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerRecvTPSProfileAvr, mGameServerProfile.recvTPSProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerRecvTPSProfileMin, mGameServerProfile.recvTPSProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerRecvTPSProfileMax, mGameServerProfile.recvTPSProfileMax);

			break;
			
		case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerSendTPSProfileAvr, mGameServerProfile.sendTPSProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerSendTPSProfileMin, mGameServerProfile.sendTPSProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerSendTPSProfileMax, mGameServerProfile.sendTPSProfileMax);

			break;
			
		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerDBWriteTPSProfileAvr, mGameServerProfile.DBWriteTPSProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerDBWriteTPSProfileMin, mGameServerProfile.DBWriteTPSProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerDBWriteTPSProfileMax, mGameServerProfile.DBWriteTPSProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerDBWriteQSizeProfileAvr, mGameServerProfile.DBWriteQSizeProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerDBWriteQSizeProfileMin, mGameServerProfile.DBWriteQSizeProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerDBWriteQSizeProfileMax, mGameServerProfile.DBWriteQSizeProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAuthFPSProfileAvr, mGameServerProfile.authFPSProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAuthFPSProfileMin, mGameServerProfile.authFPSProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerAuthFPSProfileMax, mGameServerProfile.authFPSProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerGameFPSProfileAvr, mGameServerProfile.gameFPSProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerGameFPSProfileMin, mGameServerProfile.gameFPSProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerGameFPSProfileMax, mGameServerProfile.gameFPSProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerMessagePoolProfileAvr, mGameServerProfile.messagePoolProfileAvr);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerMessagePoolProfileMin, mGameServerProfile.messagePoolProfileMin);

			sendMonitoringQuery(pTableName, mGameServerNo, mGameServerStartTime, (INT)eProfileDataType::GameServerMessagePoolProfileMax, mGameServerProfile.messagePoolProfileMax);

			break;

		default:

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[DBWriteGameServerProfile] saveType : %d", saveType);

			CCrashDump::Crash();
		}

	}
	

	return;
}

void CLanMonitoringServer::DBWriteChatServerProfile(WCHAR* pTableName)
{
	// 채팅 서버가 가동중이지 않으면 저장하지 않는다. 
	if (mChatServerProfile.bServerOnFlag == FALSE)
	{
		return;
	}


	for (INT saveType = (INT)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU; saveType <= (INT)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL; ++saveType)
	{
		switch (saveType)
		{
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerCpuProfileAvr, mChatServerProfile.cpuProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerCpuProfileMin, mChatServerProfile.cpuProfileMin);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerCpuProfileMax, mChatServerProfile.cpuProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerMemoryProfileAvr, mChatServerProfile.memoryProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerMemoryProfileMin, mChatServerProfile.memoryProfileMin);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerMemoryProfileMax, mChatServerProfile.memoryProfileMax);


			break;

		case dfMONITOR_DATA_TYPE_CHAT_SESSION:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerSessionCountProfileAvr, mChatServerProfile.sessionCountProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerSessionCountProfileMin, mChatServerProfile.sessionCountProfileMax);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerSessionCountProfileMax, mChatServerProfile.sessionCountProfileMin);


			break;

		case dfMONITOR_DATA_TYPE_CHAT_PLAYER:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerPlayerCountProfileAvr, mChatServerProfile.playerCountProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerPlayerCountProfileMin, mChatServerProfile.playerCountProfileMin);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerPlayerCountProfileMax, mChatServerProfile.playerCountProfileMax);


			break;

		case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerUpdateTPSProfileAvr, mChatServerProfile.updateTPSProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerUpdateTPSProfileMin, mChatServerProfile.updateTPSProfileMin);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerUpdateTPSProfileMax, mChatServerProfile.updateTPSProfileMax);


			break;

		case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerMessagePoolProfileAvr, mChatServerProfile.messagePoolProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerMessagePoolProfileMin, mChatServerProfile.messagePoolProfileMin);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerMessagePoolProfileMax, mChatServerProfile.messagePoolProfileMax);


			break;
			
		case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerUpdateQSizeProfileAvr, mChatServerProfile.updateQSizeProfileAvr);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerUpdateQSizeProfileMin, mChatServerProfile.updateQSizeProfileMin);

			sendMonitoringQuery(pTableName, mChatServerNo, mChatServerStartTime, (INT)eProfileDataType::ChatServerUpdateQSizeProfileMax, mChatServerProfile.updateQSizeProfileMax);


			break;

		default:

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[DBWriteChatServerProfile] saveType : %d", saveType);

			CCrashDump::Crash();

			break;
		}
	}

	return;
}


void CLanMonitoringServer::DBWriteLoginServerProfile(WCHAR* pTableName)
{
	if (mLoginServerProfile.bServerOnFlag == FALSE)
	{
		return;
	}

	for (INT saveType = dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU; saveType <= dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL; ++saveType)
	{
		switch (saveType)
		{
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerCpuProfileAvr, mLoginServerProfile.cpuProfileAvr);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerCpuProfileMin, mLoginServerProfile.cpuProfileMin);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerCpuProfileMax, mLoginServerProfile.cpuProfileMax);

			break;

		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerCpuProfileAvr, mLoginServerProfile.cpuProfileAvr);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerCpuProfileMin, mLoginServerProfile.cpuProfileMin);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerCpuProfileMax, mLoginServerProfile.cpuProfileMax);


			break;

		case dfMONITOR_DATA_TYPE_LOGIN_SESSION:

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerSessionCountAvr, mLoginServerProfile.sessionCountProfileAvr);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerSessionCountMin, mLoginServerProfile.sessionCountProfileMin);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerSessionCountMax, mLoginServerProfile.sessionCountProfileMax);


			break;

		case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerAuthTPSCountAvr, mLoginServerProfile.authTPSProfileAvr);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerAuthTPSCountMin, mLoginServerProfile.authTPSProfileMin);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerAuthTPSCountMax, mLoginServerProfile.authTPSProfileMax);


			break;

		case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerMessagePoolProfileAvr, mLoginServerProfile.messagePoolProfileAvr);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginServerMessagePoolProfileMin, mLoginServerProfile.messagePoolProfileMin);

			sendMonitoringQuery(pTableName, mLoginServerNo, mLoginServerStartTime, (INT)eProfileDataType::LoginSErverMessagePoolProfileMax, mLoginServerProfile.messagePoolProfileMax);

			break;

		default:

			CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[DBWriteLoginServerProfile] saveType : %d", saveType);

			CCrashDump::Crash();
		}
	}

	return;
}

void CLanMonitoringServer::sendMonitoringQuery(WCHAR* pTableName, DWORD serverNo, WCHAR* mServerStartTime, INT dataType, INT data)
{
RECONNECT:

	tm nowTime = { 0, };

	getTime(&nowTime);

	if (mDBConnector.Query((WCHAR*)L"INSERT INTO `%s` (`server_no`, `server_start_time`, `monitoring_time`, `data_type`,`data`) "
		"VALUES ('%d', '%s', DEFAULT, '%d', '%d')", pTableName, serverNo, mServerStartTime, dataType, data) == FALSE)
	{
		if (mDBConnector.CheckReconnectErrorCode() == TRUE)
		{
			mDBConnector.Reconnect();

			goto RECONNECT;
		}

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanMonitoringServer", L"[sendMonitoringQuery] Error Code : %d, Error Message : %s", mDBConnector.GetLastError(), mDBConnector.GetLastErrorMessage());

		CCrashDump::Crash();
	}

	return;
}


BOOL CLanMonitoringServer::insertServerNo(UINT64 sessionID, DWORD serverNo)
{
	CCriticalSection criticalSection(&mServerNoCriticalSection);

	if (findServerNo(sessionID, nullptr) == TRUE)
	{
		return FALSE;
	}

	mServerNoMap.insert(std::make_pair(sessionID, serverNo));

	return TRUE;
}

BOOL CLanMonitoringServer::findServerNo(UINT64 sessionID, DWORD* pServerNo)
{
	CCriticalSection criticalSection(&mServerNoCriticalSection);

	auto iter = mServerNoMap.find(sessionID);

	if (iter == mServerNoMap.end())
	{
		return FALSE;
	}

	if (pServerNo != nullptr)
	{
		*pServerNo = iter->second;
	}

	return TRUE;
}

void CLanMonitoringServer::eraseServerNo(UINT64 sessionID)
{
	CCriticalSection criticalSection(&mServerNoCriticalSection);

	mServerNoMap.erase(sessionID);

	return;
}


BOOL CLanMonitoringServer::recvProcedure(UINT64 sessionID, WORD messageType, CMessage* pMessage)
{
	switch (messageType)
	{
	case en_PACKET_SS_MONITOR_LOGIN:

		recvProcedureLoginSS(sessionID, pMessage);

		break;

	case en_PACKET_SS_MONITOR_DATA_UPDATE:

		recvProcedureMonitorDataSS(sessionID, pMessage);

		break;

	default:

		return FALSE;
	}


	return TRUE;
}

BOOL CLanMonitoringServer::recvProcedureLoginSS(UINT64 sessionID, CMessage* pMessage)
{
	DWORD serverNo;

	*pMessage >> serverNo;

	if (mGameServerNo != serverNo && mChatServerNo != serverNo && mLoginServerNo != serverNo)
	{
		return FALSE;
	}

	if (insertServerNo(sessionID, serverNo) == FALSE)
	{
		return FALSE;
	}

	if (mGameServerNo == serverNo)
	{
		mbGameServerLoginFlag = TRUE;

		setServerStartTime(mGameServerStartTime, _countof(mGameServerStartTime) * 2);
	}
	else if (mChatServerNo == serverNo)
	{
		mbChatServerLoginFlag = TRUE;

		setServerStartTime(mChatServerStartTime, _countof(mGameServerStartTime) * 2);
	}
	else
	{
		mbLoginServerLoginFlag = TRUE;

		setServerStartTime(mLoginServerStartTime, _countof(mLoginServerStartTime) * 2);
	}

	return TRUE;
}

BOOL CLanMonitoringServer::recvProcedureMonitorDataSS(UINT64 sessionID, CMessage* pMessage)
{
	BYTE dataType;

	INT dataValue;

	INT timeStamp;

	*pMessage >> dataType >> dataValue >> timeStamp;

	DWORD serverNo;

	if (findServerNo(sessionID, &serverNo) == FALSE)
	{
		return FALSE;
	}

	saveProcedure(dataType, dataValue);

	sendMonitorDataCS(serverNo, dataType, dataValue, timeStamp);

	return TRUE;
}

void CLanMonitoringServer::sendMonitorDataCS(DWORD serverNo, BYTE dataType, INT dataValue, INT timeStamp)
{
	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)serverNo << dataType << dataValue << timeStamp;

	mpNetMonitoringServer->SendClients(pMessage);

	pMessage->Free();

	return;
}



void CLanMonitoringServer::sendHardwareProfile(void)
{
	CCPUProfiler::GetInstance()->UpdateProcessorsProfile();

	CHardwareProfiler::GetInstance()->UpdateHardwareProfiler();

	sendCPUTotal();

	sendNonpagedMemory();

	sendRecvBytes();

	sendSendBytes();

	sendAvailableMemory();

	return;
}

void CLanMonitoringServer::sendCPUTotal(void)
{
	INT serverCPU = (INT)CCPUProfiler::GetInstance()->GetProcessorTotalPercentage();

	saveProcedure((BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, serverCPU);

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)0 << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << serverCPU << (INT)time(NULL);

	mpNetMonitoringServer->SendClients(pMessage);

	pMessage->Free();
	
	return;
}

void CLanMonitoringServer::sendNonpagedMemory(void)
{
	INT nonpagedPoolMegaBytes = ((INT)CHardwareProfiler::GetInstance()->GetNonpagedPool()) / 1000000;

	saveProcedure((BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, nonpagedPoolMegaBytes);

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)0 << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << nonpagedPoolMegaBytes << (INT)time(NULL);

	mpNetMonitoringServer->SendClients(pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringServer::sendRecvBytes(void)
{
	INT recvBytes = (INT)CHardwareProfiler::GetInstance()->GetRecvBytes() / 1000;

	saveProcedure((BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, recvBytes);

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)0 << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << recvBytes << (INT)time(NULL);

	mpNetMonitoringServer->SendClients(pMessage);

	pMessage->Free();
	
	return;
}

void CLanMonitoringServer::sendSendBytes(void)
{
	INT sendBytes = (INT)CHardwareProfiler::GetInstance()->GetSendBytes() / 1000;

	saveProcedure((BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, sendBytes);

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)0 << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << sendBytes << (INT)time(NULL);

	mpNetMonitoringServer->SendClients(pMessage);

	pMessage->Free();

	return;
}

void CLanMonitoringServer::sendAvailableMemory(void)
{
	INT availableMegaBytes = (INT)CHardwareProfiler::GetInstance()->GetAvailableMegaBytes();

	saveProcedure((BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, availableMegaBytes);

	CMessage* pMessage = CMessage::Alloc();

	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)0 << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << availableMegaBytes << (INT)time(NULL);

	mpNetMonitoringServer->SendClients(pMessage);

	pMessage->Free();

	return;
}


void CLanMonitoringServer::saveProcedure(BYTE dataType, INT dataValue)
{
	switch (dataType)
	{
	case  dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
		
		saveLoginServerRun();

		break;
	case  dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
		
		saveLoginServerCPU(dataValue);

		break;

	case  dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:

		saveLoginServerMemory(dataValue);

		break;

	case  dfMONITOR_DATA_TYPE_LOGIN_SESSION:

		saveLoginServerSessionCount(dataValue);

		break;

	case  dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:

		saveLoginServerAuthTPS(dataValue);

		break;

	case  dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:

		saveLoginServerMessagePoolUseSize(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:

		saveGameServerRun();

		break;

	case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:

		saveGameServerCPU(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:

		saveGameServerMemory(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_SESSION:

		saveGameServerSessionCount(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:

		saveGameServerAuthModePlayerCount(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:

		saveGameServerGameModePlayerCount(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:

		saveGameServerAcceptTPS(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:

		saveGameServerRecvTPS(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:

		saveGameServerSendTPS(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:

		saveGameServerDBWriteTPS(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:

		saveGameServerDBWriteQueueSize(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:

		saveGameServerAuthThreadFPS(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:

		saveGameServerGameThreadFPS(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:

		saveGameServerMessagePoolUseSize(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:

		saveChatServerRun();

		break;

	case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:

		saveChatServerCPU(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:

		saveChatServerMemory(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_CHAT_SESSION:

		saveChatServerSessionCount(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_CHAT_PLAYER:

		saveChatServerPlayerCount(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:

		saveChatServerUpdateTPS(dataValue);

		break;
	case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:

		saveChatServerMessagePoolUseSize(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:

		saveChatServerJobPoolUseSize(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:

		saveServerCPU(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:

		saveServerNonpagedMemory(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:

		saveServerRecvBytes(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:

		saveServerSendBytes(dataValue);

		break;

	case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:

		saveAvailableMemory(dataValue);

		break;

	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError,L"LanMonitoringServer" ,L"[saveProfileData] data type exception, dataType : %d", dataType);

		CCrashDump::Crash();

	}

	return;
}


void CLanMonitoringServer::saveLoginServerRun(void)
{
	mLoginServerProfile.bServerOnFlag = TRUE;

	return;
}
void CLanMonitoringServer::saveLoginServerCPU(INT dataValue)
{
	mLoginServerProfile.cpuProfileTotal += dataValue;

	++mLoginServerProfile.cpuProfileCount;

	mLoginServerProfile.cpuProfileAvr = mLoginServerProfile.cpuProfileTotal / mLoginServerProfile.cpuProfileCount;

	if (mLoginServerProfile.cpuProfileMax < dataValue)
	{
		mLoginServerProfile.cpuProfileMax = dataValue;
	}

	if (mLoginServerProfile.cpuProfileMin > dataValue)
	{
		mLoginServerProfile.cpuProfileMin = dataValue;
	}


	return;
}

void CLanMonitoringServer::saveLoginServerMemory(INT dataValue)
{
	mLoginServerProfile.memoryProfileTotal += dataValue;

	++mLoginServerProfile.memoryProfileCount;

	mLoginServerProfile.memoryProfileAvr = mLoginServerProfile.memoryProfileTotal / mLoginServerProfile.memoryProfileCount;;

	if (mLoginServerProfile.memoryProfileMax < dataValue)
	{
		mLoginServerProfile.memoryProfileMax = dataValue;
	}

	if (mLoginServerProfile.memoryProfileMin > dataValue)
	{
		mLoginServerProfile.memoryProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveLoginServerSessionCount(INT dataValue)
{
	mLoginServerProfile.sessionCountProfileTotal += dataValue;
	
	++mLoginServerProfile.sessionCountProfileCount;

	mLoginServerProfile.sessionCountProfileAvr = mLoginServerProfile.sessionCountProfileTotal / mLoginServerProfile.sessionCountProfileCount;

	if (mLoginServerProfile.sessionCountProfileMax < dataValue)
	{
		mLoginServerProfile.sessionCountProfileMax = dataValue;
	}
	
	if (mLoginServerProfile.sessionCountProfileMin > dataValue)
	{
		mLoginServerProfile.sessionCountProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveLoginServerAuthTPS(INT dataValue)
{
	mLoginServerProfile.authTPSProfileTotal += dataValue;

	++mLoginServerProfile.authTPSProfileCount;

	mLoginServerProfile.authTPSProfileAvr = mLoginServerProfile.authTPSProfileTotal / mLoginServerProfile.authTPSProfileCount;

	if (mLoginServerProfile.authTPSProfileMax < dataValue)
	{
		mLoginServerProfile.authTPSProfileMax = dataValue;
	}

	if (mLoginServerProfile.authTPSProfileMin > dataValue)
	{
		mLoginServerProfile.authTPSProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveLoginServerMessagePoolUseSize(INT dataValue)
{
	mLoginServerProfile.messagePoolProfileTotal += dataValue;

	++mLoginServerProfile.messagePoolProfileCount;

	mLoginServerProfile.messagePoolProfileAvr = mLoginServerProfile.messagePoolProfileTotal / mLoginServerProfile.messagePoolProfileCount;

	if (mLoginServerProfile.messagePoolProfileMax < dataValue)
	{
		mLoginServerProfile.messagePoolProfileMax = dataValue;
	}

	if (mLoginServerProfile.messagePoolProfileMin > dataValue)
	{
		mLoginServerProfile.messagePoolProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveGameServerRun(void)
{
	mGameServerProfile.bServerOnFlag = TRUE;

	return;
}
void CLanMonitoringServer::saveGameServerCPU(INT dataValue)
{
	mGameServerProfile.cpuProfileTotal += dataValue;

	++mGameServerProfile.cpuProfileCount;

	mGameServerProfile.cpuProfileAvr = mGameServerProfile.cpuProfileTotal / mGameServerProfile.cpuProfileCount;

	if (mGameServerProfile.cpuProfileMax < dataValue)
	{
		mGameServerProfile.cpuProfileMax = dataValue;
	}

	if (mGameServerProfile.cpuProfileMin > dataValue)
	{
		mGameServerProfile.cpuProfileMin = dataValue;
	}

	return;
}
void CLanMonitoringServer::saveGameServerMemory(INT dataValue)
{
	mGameServerProfile.memoryProfileTotal += dataValue;

	++mGameServerProfile.memoryProfileCount;

	mGameServerProfile.memoryProfileAvr = mGameServerProfile.memoryProfileTotal / mGameServerProfile.memoryProfileCount;

	if (mGameServerProfile.memoryProfileMax < dataValue)
	{
		mGameServerProfile.memoryProfileMax = dataValue;
	}

	if (mGameServerProfile.memoryProfileMin > dataValue)
	{
		mGameServerProfile.memoryProfileMin = dataValue;
	}

	return;
}
void CLanMonitoringServer::saveGameServerSessionCount(INT dataValue)
{
	mGameServerProfile.sessionCountProfileTotal += dataValue;

	++mGameServerProfile.sessionCountProfileCount;

	mGameServerProfile.sessionCountProfileAvr = mGameServerProfile.sessionCountProfileTotal / mGameServerProfile.sessionCountProfileCount;

	if (mGameServerProfile.sessionCountProfileMax < dataValue)
	{
		mGameServerProfile.sessionCountProfileMax = dataValue;
	}

	if (mGameServerProfile.sessionCountProfileMin > dataValue)
	{
		mGameServerProfile.sessionCountProfileMin = dataValue;
	}


	return;
}


void CLanMonitoringServer::saveGameServerAuthModePlayerCount(INT dataValue)
{
	mGameServerProfile.authPlayerProfileTotal += dataValue;

	++mGameServerProfile.authPlayerProfileCount;

	mGameServerProfile.authPlayerProfileAvr = mGameServerProfile.authPlayerProfileTotal / mGameServerProfile.authPlayerProfileCount;

	if (mGameServerProfile.authPlayerProfileMax < dataValue)
	{
		mGameServerProfile.authPlayerProfileMax = dataValue;
	}

	if (mGameServerProfile.authPlayerProfileMin > dataValue)
	{
		mGameServerProfile.authPlayerProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveGameServerGameModePlayerCount(INT dataValue)
{
	mGameServerProfile.gamePlayerProfileTotal += dataValue;

	++mGameServerProfile.gamePlayerProfileCount;

	mGameServerProfile.gamePlayerProfileAvr = mGameServerProfile.gamePlayerProfileTotal / mGameServerProfile.gamePlayerProfileCount;

	if (mGameServerProfile.gamePlayerProfileMax < dataValue)
	{
		mGameServerProfile.gamePlayerProfileMax = dataValue;
	}

	if (mGameServerProfile.gamePlayerProfileMin > dataValue)
	{
		mGameServerProfile.gamePlayerProfileMin = dataValue;
	}


	return;
}
void CLanMonitoringServer::saveGameServerAcceptTPS(INT dataValue)
{
	mGameServerProfile.acceptTPSProfileTotal += dataValue;

	++mGameServerProfile.acceptTPSProfileCount;

	mGameServerProfile.acceptTPSProfileAvr = mGameServerProfile.acceptTPSProfileTotal / mGameServerProfile.acceptTPSProfileCount;

	if (mGameServerProfile.acceptTPSProfileMax < dataValue)
	{
		mGameServerProfile.acceptTPSProfileMax = dataValue;
	}

	if (mGameServerProfile.acceptTPSProfileMin > dataValue)
	{
		mGameServerProfile.acceptTPSProfileMin = dataValue;
	}

	
	return;
}

void CLanMonitoringServer::saveGameServerRecvTPS(INT dataValue)
{
	mGameServerProfile.recvTPSProfileTotal += dataValue;

	++mGameServerProfile.recvTPSProfileCount;

	mGameServerProfile.recvTPSProfileAvr = mGameServerProfile.recvTPSProfileTotal / mGameServerProfile.recvTPSProfileCount;

	if (mGameServerProfile.recvTPSProfileMax < dataValue)
	{
		mGameServerProfile.recvTPSProfileMax = dataValue;
	}

	if (mGameServerProfile.recvTPSProfileMin > dataValue)
	{
		mGameServerProfile.recvTPSProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveGameServerSendTPS(INT dataValue)
{
	mGameServerProfile.sendTPSProfileTotal += dataValue;

	++mGameServerProfile.sendTPSProfileCount;

	mGameServerProfile.sendTPSProfileAvr = mGameServerProfile.sendTPSProfileTotal / mGameServerProfile.sendTPSProfileCount;

	if (mGameServerProfile.sendTPSProfileMax < dataValue)
	{
		mGameServerProfile.sendTPSProfileMax = dataValue;
	}

	if (mGameServerProfile.sendTPSProfileMin > dataValue)
	{
		mGameServerProfile.sendTPSProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveGameServerDBWriteTPS(INT dataValue)
{
	mGameServerProfile.DBWriteTPSProfileTotal += dataValue;

	++mGameServerProfile.DBWriteTPSProfileCount;

	mGameServerProfile.DBWriteTPSProfileAvr = mGameServerProfile.DBWriteTPSProfileTotal / mGameServerProfile.DBWriteTPSProfileCount;

	if (mGameServerProfile.DBWriteTPSProfileMax < dataValue)
	{
		mGameServerProfile.DBWriteTPSProfileMax = dataValue;
	}

	if (mGameServerProfile.DBWriteTPSProfileMin > dataValue)
	{
		mGameServerProfile.DBWriteTPSProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveGameServerDBWriteQueueSize(INT dataValue)
{
	mGameServerProfile.DBWriteQSizeProfileTotal += dataValue;

	++mGameServerProfile.DBWriteQSizeProfileCount;

	mGameServerProfile.DBWriteQSizeProfileAvr = mGameServerProfile.DBWriteQSizeProfileTotal / mGameServerProfile.DBWriteQSizeProfileCount;

	if (mGameServerProfile.DBWriteQSizeProfileMax < dataValue)
	{
		mGameServerProfile.DBWriteQSizeProfileMax = dataValue;
	}

	if (mGameServerProfile.DBWriteQSizeProfileMin > dataValue)
	{
		mGameServerProfile.DBWriteQSizeProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveGameServerAuthThreadFPS(INT dataValue)
{
	mGameServerProfile.authFPSProfileTotal += dataValue;

	++mGameServerProfile.authFPSProfileCount;

	mGameServerProfile.authFPSProfileAvr = mGameServerProfile.authFPSProfileTotal / mGameServerProfile.authFPSProfileCount;

	if (mGameServerProfile.authFPSProfileMax < dataValue)
	{
		mGameServerProfile.authFPSProfileMax = dataValue;
	}

	if (mGameServerProfile.authFPSProfileMin > dataValue)
	{
		mGameServerProfile.authFPSProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveGameServerGameThreadFPS(INT dataValue)
{
	mGameServerProfile.gameFPSProfileTotal += dataValue;

	++mGameServerProfile.gameFPSProfileCount;

	mGameServerProfile.gameFPSProfileAvr = mGameServerProfile.gameFPSProfileTotal / mGameServerProfile.gameFPSProfileCount;

	if (mGameServerProfile.gameFPSProfileMax < dataValue)
	{
		mGameServerProfile.gameFPSProfileMax = dataValue;
	}

	if (mGameServerProfile.gameFPSProfileMin > dataValue)
	{
		mGameServerProfile.gameFPSProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveGameServerMessagePoolUseSize(INT dataValue)
{
	mGameServerProfile.messagePoolProfileTotal += dataValue;

	++mGameServerProfile.messagePoolProfileCount;

	mGameServerProfile.messagePoolProfileAvr = mGameServerProfile.messagePoolProfileTotal / mGameServerProfile.messagePoolProfileCount;

	if (mGameServerProfile.messagePoolProfileMax < dataValue)
	{
		mGameServerProfile.messagePoolProfileMax = dataValue;
	}

	if (mGameServerProfile.messagePoolProfileMin > dataValue)
	{
		mGameServerProfile.messagePoolProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveChatServerRun(void)
{
	mChatServerProfile.bServerOnFlag = TRUE;

	return;
}


void CLanMonitoringServer::saveChatServerCPU(INT dataValue)
{
	mChatServerProfile.cpuProfileTotal += dataValue;

	++mChatServerProfile.cpuProfileCount;

	mChatServerProfile.cpuProfileAvr = mChatServerProfile.cpuProfileTotal / mChatServerProfile.cpuProfileCount;

	if (mChatServerProfile.cpuProfileMax < dataValue)
	{
		mChatServerProfile.cpuProfileMax = dataValue;
	}

	if (mChatServerProfile.cpuProfileMin > dataValue)
	{
		mChatServerProfile.cpuProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveChatServerMemory(INT dataValue)
{
	mChatServerProfile.memoryProfileTotal += dataValue;

	++mChatServerProfile.memoryProfileCount;

	mChatServerProfile.memoryProfileAvr = mChatServerProfile.memoryProfileTotal / mChatServerProfile.memoryProfileCount;

	if (mChatServerProfile.memoryProfileMax < dataValue)
	{
		mChatServerProfile.memoryProfileMax = dataValue;
	}

	if (mChatServerProfile.memoryProfileMin > dataValue)
	{
		mChatServerProfile.memoryProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveChatServerSessionCount(INT dataValue)
{
	mChatServerProfile.sessionCountProfileTotal += dataValue;

	++mChatServerProfile.sessionCountProfileCount;

	mChatServerProfile.sessionCountProfileAvr = mChatServerProfile.sessionCountProfileTotal / mChatServerProfile.sessionCountProfileCount;

	if (mChatServerProfile.sessionCountProfileMax < dataValue)
	{
		mChatServerProfile.sessionCountProfileMax = dataValue;
	}

	if (mChatServerProfile.sessionCountProfileMin > dataValue)
	{
		mChatServerProfile.sessionCountProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveChatServerPlayerCount(INT dataValue)
{
	mChatServerProfile.playerCountProfileTotal += dataValue;

	++mChatServerProfile.playerCountProfileCount;

	mChatServerProfile.playerCountProfileAvr = mChatServerProfile.playerCountProfileTotal / mChatServerProfile.playerCountProfileCount;

	if (mChatServerProfile.playerCountProfileMax < dataValue)
	{
		mChatServerProfile.playerCountProfileMax = dataValue;
	}

	if (mChatServerProfile.playerCountProfileMin > dataValue)
	{
		mChatServerProfile.playerCountProfileMin = dataValue;
	}


	return;
}


void CLanMonitoringServer::saveChatServerUpdateTPS(INT dataValue)
{
	mChatServerProfile.updateTPSProfileTotal += dataValue;

	++mChatServerProfile.updateTPSProfileCount;

	mChatServerProfile.updateTPSProfileAvr = mChatServerProfile.updateTPSProfileTotal / mChatServerProfile.updateTPSProfileCount;

	if (mChatServerProfile.updateTPSProfileMax < dataValue)
	{
		mChatServerProfile.updateTPSProfileMax = dataValue;
	}

	if (mChatServerProfile.updateTPSProfileMin > dataValue)
	{
		mChatServerProfile.updateTPSProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveChatServerMessagePoolUseSize(INT dataValue)
{
	mChatServerProfile.messagePoolProfileTotal += dataValue;

	++mChatServerProfile.messagePoolProfileCount;

	mChatServerProfile.messagePoolProfileAvr = mChatServerProfile.messagePoolProfileTotal / mChatServerProfile.messagePoolProfileCount;

	if (mChatServerProfile.messagePoolProfileMax < dataValue)
	{
		mChatServerProfile.messagePoolProfileMax = dataValue;
	}

	if (mChatServerProfile.messagePoolProfileMin > dataValue)
	{
		mChatServerProfile.messagePoolProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveChatServerJobPoolUseSize(INT dataValue)
{
	mChatServerProfile.updateQSizeProfileTotal += dataValue;

	++mChatServerProfile.updateQSizeProfileCount;

	mChatServerProfile.updateQSizeProfileAvr = mChatServerProfile.updateQSizeProfileTotal / mChatServerProfile.updateTPSProfileCount;

	if (mChatServerProfile.updateQSizeProfileMax < dataValue)
	{
		mChatServerProfile.updateQSizeProfileMax = dataValue;
	}

	if (mChatServerProfile.updateQSizeProfileMin > dataValue)
	{
		mChatServerProfile.updateQSizeProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveServerCPU(INT dataValue)
{
	mServerProfile.cpuProfileTotal += dataValue;

	++mServerProfile.cpuProfileCount;

	mServerProfile.cpuProfileAvr = mServerProfile.cpuProfileTotal / mServerProfile.cpuProfileCount;

	if (mServerProfile.cpuProfileMax < dataValue)
	{
		mServerProfile.cpuProfileMax = dataValue;
	}

	if (mServerProfile.cpuProfileMin > dataValue)
	{
		mServerProfile.cpuProfileMin = dataValue;
	}

	return;
}

void CLanMonitoringServer::saveServerNonpagedMemory(INT dataValue)
{
	mServerProfile.nonpagedMemoryProfileTotal += dataValue;

	++mServerProfile.nonpagedMemoryProfilCount;

	mServerProfile.nonpagedMemoryProfileAvr = mServerProfile.nonpagedMemoryProfileTotal / mServerProfile.nonpagedMemoryProfilCount;

	if (mServerProfile.nonpagedMemoryProfileMax < dataValue)
	{
		mServerProfile.nonpagedMemoryProfileMax = dataValue;
	}

	if (mServerProfile.nonpagedMemoryProfileMin > dataValue)
	{
		mServerProfile.nonpagedMemoryProfileMin = dataValue;
	}


	return;
}


void CLanMonitoringServer::saveServerRecvBytes(INT dataValue)
{
	mServerProfile.recvKBytesProfileTotal += dataValue;

	++mServerProfile.recvKBytesProfileCount;

	mServerProfile.recvKBytesProfileAvr = mServerProfile.recvKBytesProfileTotal / mServerProfile.recvKBytesProfileCount;

	if (mServerProfile.recvKBytesProfileMax < dataValue)
	{
		mServerProfile.recvKBytesProfileMax = dataValue;
	}

	if (mServerProfile.recvKBytesProfileMin > dataValue)
	{
		mServerProfile.recvKBytesProfileMin = dataValue;
	}

	return;
}


void CLanMonitoringServer::saveServerSendBytes(INT dataValue)
{
	mServerProfile.sendKBytesProfileTotal += dataValue;

	++mServerProfile.sendKBytesProfileCount;

	mServerProfile.sendKBytesProfileAvr = mServerProfile.sendKBytesProfileTotal / mServerProfile.sendKBytesProfileCount;

	if (mServerProfile.sendKBytesProfileMax < dataValue)
	{
		mServerProfile.sendKBytesProfileMax = dataValue;
	}

	if (mServerProfile.sendKBytesProfileMin > dataValue)
	{
		mServerProfile.sendKBytesProfileMin = dataValue;
	}


	return;
}


void CLanMonitoringServer::saveAvailableMemory(INT dataValue)
{
	mServerProfile.availableMemoryProfileTotal += dataValue;

	++mServerProfile.availableMemoryProfileCount;
	
	mServerProfile.availableMemoryProfileAvr = mServerProfile.availableMemoryProfileTotal / mServerProfile.availableMemoryProfileCount;

	if (mServerProfile.availableMemoryProfileMax < dataValue)
	{
		mServerProfile.availableMemoryProfileMax = dataValue;
	}

	if (mServerProfile.availableMemoryProfileMin > dataValue)
	{
		mServerProfile.availableMemoryProfileMin = dataValue;
	}


	return;
}


