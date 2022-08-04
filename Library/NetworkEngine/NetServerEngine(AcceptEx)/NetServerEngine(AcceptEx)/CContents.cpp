#include "stdafx.h"
#include "CNetServer.h"
#include "CContents.h"



void CContents::OnClientJoin(UINT64 sessionID)
{
	int retval = 0;

	CMessage* pMessage = CMessage::Alloc();
	
	packingLoginMessage(pMessage);

	SendPacket(sessionID, pMessage);

	pMessage->Free();
	
}


void CContents::packingLoginMessage(CMessage* pMessage)
{

	UINT64 payload = 0x7fffffffffffffff;

	*pMessage << payload;

}


BOOL CContents::OnStart(void)
{
	return true;
}

void CContents::OnClientLeave(UINT64 sessionID)
{

}

void CContents::OnRecv(UINT64 sessionID, CMessage* pMessage)
{	
	int retval;

	CMessage* localMessage = CMessage::Alloc();

	int useSize = pMessage->GetUseSize();
	retval = pMessage->GetPayload(localMessage->GetMessagePtr(), useSize);
	if (retval != useSize)
	{	
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LanServer",L"[OnRecv] Message Enqueue Error, retval : %d, useSize : %d\n", retval, useSize);

		CCrashDump::Crash();
	}

	localMessage->MoveWritePos(useSize);

	if (!procRecvMessage(sessionID, REQ_ECHO, localMessage))
	{
		Disconnect(sessionID);
	}

	localMessage->Free();

	return;
}


// 화이트 IP
bool CContents::OnConnectionRequest(const WCHAR* userIp, WORD userPort)
{

	return true;
}

void CContents::OnError(int errorCode, const WCHAR* errorMessage)
{

}

void CContents::OnStop(void)
{

	return;
}


void CContents::OnStartWorkerThread(void)
{

	return;
}


void CContents::OnCloseWorkerThread(void)
{
	return;
}




bool CContents::procRecvMessage(UINT64 sessionID, DWORD messageType, CMessage* pMessage)
{

	switch (messageType)
	{
	case REQ_ECHO:
		
		return procEchoMessage(sessionID, pMessage);

		break;
	default:
		
		// 클라이언트가 이상한 메시지 Type을 보냄 
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[procRecvMessage] Message Type Error, Session ID : %lld, messageType : %d\n", sessionID, messageType);
		
		break;
	}

	return false;	
}

bool CContents::procEchoMessage(UINT64 sessionID, CMessage* pMessage)
{	
	UINT64 payload;

	*pMessage >> payload;
	
	packingEchoMessage(pMessage, payload);

	if (SendPacket(sessionID, pMessage) == false)
	{
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[procRecvMessage] SendPacket Session Not Found, Session ID : %lld\n", sessionID);

		return false;
	}

	return true;
}

void CContents::packingEchoMessage(CMessage* pMessage, UINT64 payload)
{		
	*pMessage << payload;
}