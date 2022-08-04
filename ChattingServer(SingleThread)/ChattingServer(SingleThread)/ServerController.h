#pragma once

#include <conio.h>
#include "TPSProfiler/TPSProfiler/CTPSProfiler.h"

class CServerController
{

public:

	CServerController()
		: mbControlModeFlag(FALSE)
		, mbShutdownFlag(FALSE)
		, mpChatServer(nullptr)
		, mTPSProfiler()
	{
		mTPSProfiler.SetTPSProfiler(L"ChatServer_TPS");
	}

	~CServerController()
	{

	}

	void SetChatServerPtr(CChattingServer* pChatServer)
	{
		mpChatServer = pChatServer;

		return;
	}


	void ServerControling(void)
	{		
		if (_kbhit() == TRUE)
		{
			WCHAR controlKey = _getwch();

			if (controlKey == L'u' || controlKey == L'U')
			{

				wprintf_s(L"UnLock\n");

				mbControlModeFlag = TRUE;
			}

			if ((controlKey == L'p' || controlKey == L'p') && mbControlModeFlag == TRUE)
			{
				mTPSProfiler.PrintTPSProfile();
				CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();
			}

			if ((controlKey == L'd' || controlKey == L'D') && mbControlModeFlag == TRUE)
			{
				CCrashDump::Crash();
			}


			if ((controlKey == L'q' || controlKey == L'Q') && mbControlModeFlag == TRUE)
			{
				mbShutdownFlag = TRUE;
			}

			if (controlKey == L'l' || controlKey == L'L')
			{
				wprintf_s(L"Lock\n");

				mbControlModeFlag = FALSE;
			}
		}

		CHardwareProfiler::GetInstance()->UpdateHardwareProfiler();

		wprintf_s(L"\n\n\n\n"
			L"	                                                       [ chat Server ] \n"
			L" 旨收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旬\n\n"
			L"    NetLogin Accept Total : %lld  |  Current Client : %4d / %4d  \n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"             NonPaged Memory     : %5d MB\n\n"
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
			L" 曲收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旭\n\n"
			, mpChatServer->GetAcceptTotal(), mpChatServer->GetCurrentClientCount(), mpChatServer->GetMaxClientCount()
			, (DWORD)(CHardwareProfiler::GetInstance()->GetNonpagedPool()/ 1000000)
			, mpChatServer->GetAcceptTPS(), mpChatServer->GetRecvTPS(), mpChatServer->GetSendTPS(), mpChatServer->GetUpdateTPS()
			, mpChatServer->GetMaxWakeupWaitTime()
			, mpChatServer->GetWakeupPerSecond(), mpChatServer->GetMaxWakeupPerSecond()
			, mpChatServer->GetWakeupProcessTime(), mpChatServer->GetMaxWakeupProcessTime()
			, mbControlModeFlag, CSystemLog::GetInstance()->GetLogLevel()
		);

		mTPSProfiler.SaveTPSInfo(L"Accept TPS", mpChatServer->GetAcceptTPS());
	
		mpChatServer->InitializeTPS();
		mpChatServer->InitializeChatTPS();
	}



	BOOL GetShutdownFlag(void) const
	{
		return mbShutdownFlag;
	}

private:

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CChattingServer* mpChatServer;

	CTPSProfiler mTPSProfiler;
};


