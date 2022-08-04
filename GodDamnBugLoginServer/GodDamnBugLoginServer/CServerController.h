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

		wprintf_s(L"\n\n\n\n"
			L"	                                                       [ Login Server ] \n"
			L" 旨收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旬\n\n"
			L"    NetLogin Accept Total : %lld  |  Current Client : %4d / %4d  |  GameAcceptTotal : %lld  |  ChatAcceptTotal : %lld \n"
			L"      Login Request Count : %lld \n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"	
			L"        Max Wakeup Wait Time     : %5d ms\n"
			L"           Wakeup Per Second     : %5d     [ Max : %5d ]\n"
			L"    Wakeup OnRecv Per Second     : %5d     [ Max : %5d ]\n"
			L"         Wakeup Process Time     : %5d ms  [ Max : %5d ms ]\n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"    Control Mode : %d | LogLevel : %d \n\n"
			L"    [ L ] : Control Lock | [ U ] : Control Unlock | [ D ] : Crash | [ Q ] : Exit | [ P ] : Print Profile\n\n"
			L"  收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收 \n\n"
			L"               Message Chunk Alloc Count             : %d\n"
			L"       ConnectionState Chunk Alloc Count             : %d\n\n"
			L" 曲收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收收旭\n\n"
			, mpNetLoginServer->GetAcceptTotal(), mpNetLoginServer->GetCurrentClientCount(), mpNetLoginServer->GetMaxClientCount(), mpNetLoginServer->GetGameAcceptTotal(), mpNetLoginServer->GetChatAcceptTotal()
			, mpNetLoginServer->GetLoginRequestCount()
			, mpNetLoginServer->GetWakeupWaitTime(),mpNetLoginServer->GetWakeupPerSecond(), mpNetLoginServer->GetMaxWakeupPerSecond()
			, mpNetLoginServer->GetWakeupOnRecvPerSecond(), mpNetLoginServer->GetMaxWakeupOnRecvPerSecond()
			, mpNetLoginServer->GetWakeupProcessTime(), mpNetLoginServer->GetMaxWakeupProcessTime()
			, mbControlModeFlag, CSystemLog::GetInstance()->GetLogLevel()
			, CLockFreeObjectFreeList<CTLSLockFreeObjectFreeList<CMessage>::CChunk>::GetAllocNodeCount()
			, CLockFreeObjectFreeList<CTLSLockFreeObjectFreeList<CNetLoginServer::CConnectionState>::CChunk>::GetAllocNodeCount()
		);

		return;
	}

private:

	BOOL mbControlModeFlag;

	BOOL mbShutdownFlag;

	CLanMonitoringClient* mpLanMonitoringClient;

	CLanLoginServer* mpLanLoginServer;

	CNetLoginServer* mpNetLoginServer;

};

