#pragma once

#include "stdafx.h"

class CServerController
{
public:

	CServerController(void)
		:mbControlModeFlag(FALSE)
		,mbShutdownFlag(FALSE)
		,mpLanMonitoringClient(nullptr)
		,mpLanLoginServer(nullptr)
		,mpNetLoginServer(nullptr)
	{
	}

	~CServerController(void)
	{
	}

	BOOL GetShutdownFlag(void) const
	{
		return mbShutdownFlag;
	}

	void SetLanMonitoringClient(CLanMonitoringClient* pLanMonitoringClient)
	{
		mpLanMonitoringClient = pLanMonitoringClient;

		return;
	}

	void SetLanLoginServer(CLanLoginServer* pLanLoginServer)
	{
		mpLanLoginServer = pLanLoginServer;

		return;
	}

	void SetNetLoginServer(CNetLoginServer* pNetLoginServer)
	{
		mpNetLoginServer = pNetLoginServer;

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

			if ((controlKey == L'd' || controlKey == L'D') && mbControlModeFlag == TRUE)
			{
				CCrashDump::Crash();
			}

			if ((controlKey == L'q' || controlKey == L'Q') && mbControlModeFlag == TRUE)
			{
				mbShutdownFlag = TRUE;
			}

			if ((controlKey == L'p' || controlKey == L'P') && mbControlModeFlag == TRUE)
			{
				CTLSPerformanceProfiler::PrintPerformanceProfile();
				CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();
			}

			if (controlKey == L'l' || controlKey == L'L')
			{
				mbControlModeFlag = FALSE;
			}
		}

		return;
	}

private:

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CLanMonitoringClient* mpLanMonitoringClient;

	CLanLoginServer* mpLanLoginServer;

	CNetLoginServer* mpNetLoginServer;

};

