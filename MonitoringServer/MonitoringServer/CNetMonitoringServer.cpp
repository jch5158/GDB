#include "stdafx.h"

CNetMonitoringServer::CNetMonitoringServer(void)
	: mClientList()
	, mLoginKey{ 0, }
	, mClientsCriticalSection{ 0, }
{
	InitializeCriticalSection(&mClientsCriticalSection);

}

CNetMonitoringServer::~CNetMonitoringServer(void)
{

	DeleteCriticalSection(&mClientsCriticalSection);
}

BOOL CNetMonitoringServer::OnStart(void)
{

	return TRUE;
}

void CNetMonitoringServer::OnClientJoin(UINT64 sessionID)
{
	return;
}

void CNetMonitoringServer::OnClientLeave(UINT64 sessionID)
{
	eraseClient(sessionID);

	return;
}


void CNetMonitoringServer::OnStartAcceptThread(void)
{

	return;
}

void CNetMonitoringServer::OnStartWorkerThread(void)
{

	return;
}

void CNetMonitoringServer::OnRecv(UINT64 sessionID, CMessage* pMessage)
{
	WORD messageType;

	*pMessage >> messageType;

	if (recvProcedure(sessionID, messageType, pMessage) == FALSE)
	{
		Disconnect(sessionID);
	}

	return;
}


void CNetMonitoringServer::OnCloseWorkerThread(void)
{
	return;
}

// accept 직후 바로 호출
BOOL CNetMonitoringServer::OnConnectionRequest(const WCHAR* userIP, WORD userPort)
{
	return TRUE;
}


void CNetMonitoringServer::OnCloseAcceptThread(void)
{
	return;
}

void CNetMonitoringServer::OnError(DWORD errorCode, const WCHAR* errorMessage)
{
	return;
}


void CNetMonitoringServer::OnStop(void)
{
	return;
}

void CNetMonitoringServer::SetLoginKey(CHAR* pLoginKey)
{
	strcpy_s(mLoginKey, pLoginKey);

	return;
}

void CNetMonitoringServer::SendClients(CMessage* pMessage)
{
	for (UINT64 sessionID : mClientList)
	{
		SendPacket(sessionID, pMessage);
	}

	return;
}


BOOL CNetMonitoringServer::insertClient(UINT64 sessionID)
{
	CCriticalSection criticalSection(&mClientsCriticalSection);

	if (findClient(sessionID) == TRUE)
	{
		return FALSE;
	}

	mClientList.push_back(sessionID);

	return TRUE;
}

BOOL CNetMonitoringServer::findClient(UINT64 sessionID)
{
	CCriticalSection criticalSection(&mClientsCriticalSection);

	for (UINT64 result : mClientList)
	{
		if (result == sessionID)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CNetMonitoringServer::eraseClient(UINT64 sessionID)
{
	CCriticalSection criticalSection(&mClientsCriticalSection);

	mClientList.remove(sessionID);

	return;
}

void CNetMonitoringServer::getGameProfileTableName(WCHAR* pTableName, DWORD bufferCb)
{



	return;
}


void CNetMonitoringServer::getChatProfileTableName(WCHAR* pTableName, DWORD bufferCb)
{

	return;
}


BOOL CNetMonitoringServer::recvProcedure(UINT64 sessionID, WORD messageType, CMessage* pMessage)
{

	switch (messageType)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:

		recvProcedureLoginCS(sessionID, pMessage);

		break;

	default:

		return FALSE;
	}


	return TRUE;
}




BOOL CNetMonitoringServer::recvProcedureLoginCS(UINT64 sessionID, CMessage* pMessage)
{	
	CHAR loginKey[32];

	pMessage->GetPayload(loginKey, 32);

	pMessage->MoveReadPos(32);

	BYTE responseValue;

	if (memcmp(mLoginKey, loginKey, 32) != 0)
	{
		responseValue = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;
	}
	else
	{
		responseValue = dfMONITOR_TOOL_LOGIN_OK;

		if (insertClient(sessionID) == FALSE)
		{
			return FALSE;
		}
	}

	sendLoginResponseCS(sessionID, responseValue);

	return TRUE;
}


void CNetMonitoringServer::sendLoginResponseCS(UINT64 sessionID, BYTE result)
{
	CMessage *pMessage = CMessage::Alloc();

	packingLoginResponseCS(result, pMessage);

	SendPacket(sessionID, pMessage);

	pMessage->Free();
	
	return;
}


void CNetMonitoringServer::packingLoginResponseCS(BYTE status, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN << status;

	return;
}
