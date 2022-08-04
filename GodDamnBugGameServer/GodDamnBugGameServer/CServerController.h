#pragma once

#include "stdafx.h"
#include <conio.h>


class CServerController
{

public:

	CServerController(void)
		: mbControlModeFlag(FALSE)
		, mbShutdownFlag(FALSE)
		, mpMonitoringClient(nullptr)
	    , mpLoginClient(nullptr)
		, mpGodDamnBug(nullptr)
	{
	}

	~CServerController(void)
	{
	}


	BOOL GetShutdownFlag(void) const
	{
		return mbShutdownFlag;
	}

	void SetGameServer(CGodDamnBug* pGodDamnBug)
	{
		mpGodDamnBug = pGodDamnBug;

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
	}

private:

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CLanMonitoringClient* mpMonitoringClient;

	CLanLoginClient* mpLoginClient;

	CGodDamnBug* mpGodDamnBug;
};

