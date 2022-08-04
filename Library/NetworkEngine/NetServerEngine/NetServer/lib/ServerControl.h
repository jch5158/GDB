#pragma once

#include "stdafx.h"


class CServerController
{

public:

	CServerController(CNetServer* pContents)
		: mbControlModeFlag(false)
		, mbShutdownFlag(false)
		, mpServer(pContents)
	{}

	~CServerController() = default;

	CServerController(const CServerController&) = delete;
	CServerController& operator=(const CServerController&) = delete;

	void ServerControling(void)
	{	
		if (_kbhit())
		{
			WCHAR controlKey = _getwch();

			if (controlKey == L'u' || controlKey == L'U')
			{
				wprintf_s(L"Q 입력 시 서버 종료 \n");
				wprintf_s(L"L 입력 시 입력모드 종료\n");

				mbControlModeFlag = true;
			}

			if (controlKey == L'd' || controlKey == L'D')
				CCrashDump::Crash();

			if (controlKey == L'l' || controlKey == L'L')
			{
				wprintf(L"입력 모드 종료\n");
				mbControlModeFlag = false;
			}

			if ((controlKey == L'q' || controlKey == L'Q') && mbControlModeFlag)
				mbShutdownFlag = true;
		}

		wprintf_s(L"\n\n\n\n"
			L"	                                                 [ SERVER CONTROL]\n"
			L" ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n\n"
			L"    Game Server Bind IP : %s | Game Server Bind Port : %d | Game Server Accept Total : %lld \n\n"
			L"    Game Current Client : %4d / %4d | Game Running Thread : %d | Game Worker Thread : %d | Game Nagle : %d \n\n"
			L"  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ \n\n"
			L"    Control Mode : %d   |  [ L ] : Control Lock  |  [ U ] : Control Unlock  |  [ D ] : Crash  |  [ Q ] : Exit  |  LogLevel : %d      \n"
			L"  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ \n\n"
			L"    Chunk Message Alloc Count : %d\n"
			L" ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n"
			, mpServer->GetServerBindIP(), mpServer->GetServerBindPort(), mpServer->GetAcceptTotal()
			, mpServer->GetCurrentClientCount(), mpServer->GetMaxClientCount(), mpServer->GetRunningThreadCount(), mpServer->GetWorkerThreadCount(), mpServer->GetNagleFlag()
			, mbControlModeFlag, CSystemLog::GetInstance()->GetLogLevel()
			, CMessage::GetAllocCount()
		);
	}

	bool GetShutdwonFlag(void) const 
	{
		return mbShutdownFlag;
	}

private:


	bool mbControlModeFlag;

	bool mbShutdownFlag;

	CNetServer* mpServer;
};

