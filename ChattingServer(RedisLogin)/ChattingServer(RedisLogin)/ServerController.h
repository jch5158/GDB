#pragma once

#include "stdafx.h"

extern CHardwareProfiler hardware;

class CServerController
{

public:

	CServerController(void)
		: mbControlModeFlag(FALSE)
		, mbShutdownFlag(FALSE)
		, mpChatServer(nullptr)
		, mpLoginClient(nullptr)
		, mpMonitoringClient(nullptr)
		, mTPSProfiler()
	{
		mTPSProfiler.SetTPSProfiler(L"ChatServer(Redis)");
	}

	~CServerController(void)
	{

	}


	BOOL GetShutdownFlag(void) const
	{
		return mbShutdownFlag;
	}

	void SetChatServer(CChattingServer* pChatServer)
	{
		mpChatServer = pChatServer;

		return;
	}

	void SetMonitoringClient(CLanMonitoringClient* pMonitoringClient)
	{
		mpMonitoringClient = pMonitoringClient;

		return;
	}

	void SetLoginClient(CLanLoginClient* pLoginClient)
	{
		mpLoginClient = pLoginClient;

		return;
	}

	void ServerControling(void)
	{

		if (_kbhit() == TRUE)
		{
			WCHAR controlKey = _getwch();

			if (controlKey == L'u' || controlKey == L'U')
			{
				mbControlModeFlag = TRUE;
			}


			if ((controlKey == L'p' || controlKey == L'p') && mbControlModeFlag == TRUE)
			{
				mTPSProfiler.PrintTPSProfile();
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
			L"	                                                       [ chat Server ] \n"
			L" 旨收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旬\n\n"
			L"    NetLogin Accept Total : %lld  |  Current Client : %4d / %4d  \n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"                  Accept TPS     : %5d\n"
			L"                    Recv TPS     : %5d\n"
			L"                    Send TPS     : %5d\n"
			L"                  Update TPS     : %5d\n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"        Max Wakeup Wait Time     : %5d ms\n"
			L"           Wakeup Per Second     : %5d     [ Max : %5d ]\n"
			L"         Wakeup Process Time     : %5d ms  [ Max : %5d ms ]\n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Control Mode : %d | LogLevel : %d \n\n"
			L"    [ L ] : Control Lock | [ U ] : Control Unlock | [ D ] : Crash | [ Q ] : Exit | [ P ] : Print Profile\n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"               Message Chunk Alloc Count             : %d\n"
			L" 曲收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旭\n\n"
			, mpChatServer->GetAcceptTotal(), mpChatServer->GetCurrentClientCount(), mpChatServer->GetMaxClientCount()
			, mpChatServer->GetAcceptTPS(), mpChatServer->GetRecvTPS(), mpChatServer->GetSendTPS(), mpChatServer->GetUpdateTPS()
			, mpChatServer->GetMaxWakeupWaitTime()
			, mpChatServer->GetWakeupPerSecond(), mpChatServer->GetMaxWakeupPerSecond()
			, mpChatServer->GetWakeupProcessTime(), mpChatServer->GetMaxWakeupProcessTime()
			, mbControlModeFlag, CSystemLog::GetInstance()->GetLogLevel()
			, CMessage::GetAllocCount()
		);

		mTPSProfiler.SaveTPSInfo(L"Update TPS", mpChatServer->GetSendTPS());
		mTPSProfiler.SaveTPSInfo(L"Send TPS", mpChatServer->GetUpdateTPS());

		mpChatServer->InitializeTPS();
		mpChatServer->InitializeUpdateTPS();
	}

private:

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CLanLoginClient* mpLoginClient;

	CChattingServer* mpChatServer;

	CLanMonitoringClient* mpMonitoringClient;



	CTPSProfiler mTPSProfiler;
};

