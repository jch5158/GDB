#pragma once



class CServerController
{

public:

	CServerController(void)
		: mbControlModeFlag(FALSE)
		, mbShutdownFlag(FALSE)
		, mpEchoServer(nullptr)
	{
	}

	~CServerController(void)
	{
	}

	void SetEchoServerPtr(CEcho* pEchoServer)
	{
		mpEchoServer = pEchoServer;

		return;
	}

	void ServerControling(CMMOServer* pMMOServer)
	{
		if (_kbhit() == TRUE)
		{
			WCHAR controlKey = _getwch();

			if (controlKey == L'u' || controlKey == L'U')
			{	
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
				mbControlModeFlag = FALSE;
			}
		}

		wprintf_s(L"\n\n\n\n"
			L"	                                                 [ SERVER CONTROL]\n"
			L" 旨收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旬\n\n"
			L"    Game Server Bind IP : %s | Game Server Bind Port : %d | Game Server Accept Total : %lld \n\n"
			L"    Game Current Client : %4d / %4d | Game Running Thread : %d | Game Worker Thread : %d | Game Nagle : %d \n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Control Mode : %d   |  [ L ] : Control Lock  |  [ U ] : Control Unlock  |  [ D ] : Crash  |  [ Q ] : Exit  |  LogLevel : %d      \n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Chunk Message Alloc Count : %d\n"
			L" 曲收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旭\n\n"
			, mpEchoServer->GetServerBindIP(), mpEchoServer->GetServerBindPort(), mpEchoServer->GetAcceptTotal()
			, mpEchoServer->GetCurrentClientCount(), mpEchoServer->GetMaxClientCount(), mpEchoServer->GetRunningThreadCount(), mpEchoServer->GetWorkerThreadCount(), mpEchoServer->GetNagleFlag()
			, mbControlModeFlag, CSystemLog::GetInstance()->GetLogLevel()
			, CMessage::GetAllocCount()
		);

		return;
	}

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CEcho* mpEchoServer;
};


