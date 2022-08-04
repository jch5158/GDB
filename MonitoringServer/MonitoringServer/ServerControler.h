#pragma once

class CServerController
{

public:

	CServerController(void)
		: mbControlModeFlag(FALSE)
		, mbShutdownFlag(FALSE)
		, mpLanMonitoringServer(nullptr)
		, mpNetMonitoringServer(nullptr)
	{
	}

	~CServerController(void)
	{

	}

	void SetLanMonitoringServer(CLanMonitoringServer* pLanMonitoringServer)
	{
		mpLanMonitoringServer = pLanMonitoringServer;

		return;
	}

	void SetNetMonitoringServer(CNetMonitoringServer* pNetMonitoringServer)
	{
		mpNetMonitoringServer = pNetMonitoringServer;

		return;
	}

	BOOL GetShutdounFlag() const
	{
		return mbShutdownFlag;
	}

	void ServerControling()
	{
		if (_kbhit() == TRUE)
		{
			WCHAR controlKey = _getwch();

			if (controlKey == L'u' || controlKey == L'U')
			{
				wprintf_s(L"Unlock\n\n");

				mbControlModeFlag = TRUE;
			}

			if ((controlKey == L'd' || controlKey == L'D') && mbControlModeFlag)
			{
				CCrashDump::Crash();
			}

			if ((controlKey == L'q' || controlKey == L'Q') && mbControlModeFlag)
			{
				mbShutdownFlag = TRUE;
			}

			if (controlKey == L'l' || controlKey == L'L')
			{
				wprintf_s(L"Lock\n\n");

				mbControlModeFlag = FALSE;
			}
		}
		//	mpNetMonitoringServer->GetClientCount(), mpNetMonitoringServer->GetRunningThreadCount(), mpNetMonitoringServer->GetWorkerThreadCount(), CSystemLog::GetInstance()->GetLogLevel());
		// ((CChattingServer*)pNetMonitoringServer)->GetPlayerCount(), nagle

		wprintf_s(L"\n\n\n\n"
			L"                                                     [ Monitoring Server ]\n"
			L" 旨收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旬\n\n"
			L"    Net Current Client : %4d / %4d | Running Thread : %d | Worker Thread : %d | Nagle : %d \n\n"
			L"    Net IP : %s | Net Port : %d | Net Accept Total : %5lld\n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Lan Current Client : %4d / %4d | Running Thread : %d | Worker Thread : %d | Nagle : %d \n\n"
			L"    Lan IP : %s | Lan Port : %d | Lan Accept Total : %5lld \n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    GameServer Connect State : %d | ChatServer Connect State : %d | LoginServer Connect State : %d  \n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Control Mode : %d | [ L ] : Control Lock | [ U ] : Control Unlock | [ D ] : Crash | [ Q ] : Exit | LogLevel : %d  \n \n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Chunk Message Alloc Count : %4d\n\n"
			L" 曲收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旭\n"
			, mpNetMonitoringServer->GetCurrentClientCount(), mpNetMonitoringServer->GetMaxClientCount(), mpNetMonitoringServer->GetRunningThreadCount(), mpNetMonitoringServer->GetWorkerThreadCount(), mpNetMonitoringServer->GetNagleFlag(),
			mpNetMonitoringServer->GetServerBindIP(), mpNetMonitoringServer->GetServerBindPort() ,mpNetMonitoringServer->GetAcceptTotal(),
			mpLanMonitoringServer->GetCurrentClientCount(), mpLanMonitoringServer->GetMaxClientCount(), mpLanMonitoringServer->GetRunningThreadCount(), mpLanMonitoringServer->GetWorkerThreadCount(), mpLanMonitoringServer->GetNagleFlag(),
			mpLanMonitoringServer->GetServerBindIP(), mpLanMonitoringServer->GetServerBindPort(), mpLanMonitoringServer->GetAcceptTotal(),
			mpLanMonitoringServer->GetGameServerLoginFlag(), mpLanMonitoringServer->GetChatServerLoginFlag(), mpLanMonitoringServer->GetLoginServerLoginFlag(),
			mbControlModeFlag, CSystemLog::GetInstance()->GetLogLevel(),
			CLockFreeObjectFreeList<CTLSLockFreeObjectFreeList<CMessage>::CChunk>::GetAllocNodeCount()
		);

		return;
	}

private:

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CLanMonitoringServer* mpLanMonitoringServer;

	CNetMonitoringServer* mpNetMonitoringServer;

};
