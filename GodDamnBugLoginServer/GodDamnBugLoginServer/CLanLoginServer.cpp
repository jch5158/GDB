#include "stdafx.h"

CLanLoginServer::CLanLoginServer(void)
	:mGameServerSessionID(0)
	,mChatServerSessionID(0)
	,mpNetLoginServer(nullptr)
{

}

CLanLoginServer::~CLanLoginServer(void)
{

}

BOOL CLanLoginServer::OnStart(void)
{

	return TRUE;
}

void CLanLoginServer::OnStartWorkerThread(void)
{
	return;
}

void CLanLoginServer::OnStartAcceptThread(void)
{
	return;
}

void CLanLoginServer::OnClientJoin(UINT64 sessionID)
{
	return;
}

void CLanLoginServer::OnClientLeave(UINT64 sessionID)
{
	if (mGameServerSessionID == sessionID)
	{
		mGameServerSessionID = 0;
	}
	else
	{
		mChatServerSessionID = 0;
	}

	return;
}

void CLanLoginServer::OnRecv(UINT64 sessionID, CMessage* pMessage)
{
	WORD messageTyps;

	*pMessage >> messageTyps;

	if (recvProcedure(sessionID, messageTyps, pMessage) == FALSE)
	{
		Disconnect(sessionID);
	}

	return;
}

void CLanLoginServer::OnCloseWorkerThread(void)
{
	return;
}

void CLanLoginServer::OnCloseAcceptThread(void)
{
	return;
}

BOOL CLanLoginServer::OnConnectionRequest(const WCHAR* pUserIP, WORD userPort)
{

	return TRUE;
}

void CLanLoginServer::OnError(INT errorCode, const WCHAR* pErrorMessage)
{
	return;
}

void CLanLoginServer::OnStop(void)
{

	return;
}

BOOL CLanLoginServer::GetGameServerLoginFlag(void) const
{
	if (mGameServerSessionID == 0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CLanLoginServer::GetChatServerLoginFlag(void) const
{
	if (mChatServerSessionID == 0)
	{
		return FALSE;
	}

	return TRUE;
}

void CLanLoginServer::SetNetLoginServer(CNetLoginServer* pNetLoginServer)
{
	mpNetLoginServer = pNetLoginServer;

	return;
}

BOOL CLanLoginServer::recvProcedure(UINT64 sessionID, WORD messageType, CMessage* pMessage)
{
	switch (messageType)
	{
	case en_PACKET_SS_LOGINSERVER_LOGIN:

		return recvProcedureLoginRequest(sessionID, pMessage);

	case en_PACKET_SS_RES_NEW_CLIENT_LOGIN:

		return recvProcedureClientServerLogin(sessionID, pMessage);

	default:
		return FALSE;
	}

	return TRUE;
}

BOOL CLanLoginServer::recvProcedureLoginRequest(UINT64 sessionID, CMessage* pMessage)
{
	BYTE serverType;

	*pMessage >> serverType;

	if (serverType == dfSERVER_TYPE_GAME)
	{
		mGameServerSessionID = sessionID;
	}
	else if(serverType == dfSERVER_TYPE_CHAT)
	{
		mChatServerSessionID = sessionID;
	}
	else
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"CLanLoginServer", L"[recvProcedureLoginRequest] Server Type : %d", serverType);

		CCrashDump::Crash();
	}

	return TRUE;
}

BOOL CLanLoginServer::recvProcedureClientServerLogin(UINT64 sessionID, CMessage* pMessage)
{
	UINT64 accountNo;

	UINT64 serverType;

	*pMessage >> accountNo >> serverType;

	mpNetLoginServer->NotificationSessionConnect(serverType);

	return TRUE;
}