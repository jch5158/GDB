#include "stdafx.h"
#include "CLanServer.h"
#include "CContents.h"

BOOL CContents::OnStart(void)
{
	return TRUE;
}

void CContents::OnStartWorkerThread(void)
{
	return;
}

void CContents::OnStartAcceptThread(void)
{
	return;
}

void CContents::OnCloseWorkerThread(void)
{
	return;
}

void CContents::OnCloseAcceptThread(void)
{
	return;
}

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
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer",L"[OnRecv] Message Enqueue Error, retval : %d, useSize : %d\n", retval, useSize);

		CCrashDump::Crash();
	}

	localMessage->MoveWritePos(useSize);

	try 
	{	
		if (!procRecvMessage(sessionID, REQ_ECHO, localMessage))
		{	
			Disconnect(sessionID);
		}
	}
	catch (const message::CExceptionObject& exception)
	{
		// Ŭ���̾�Ʈ�� �Ǽ��� Ŭ���̾�Ʈ�� ���������� ���۵� �޽����� ������ ���
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[OnRecv] procRecvMessage Error, sessionID : %lld ", sessionID);

		Disconnect(sessionID);

		exception.PrintExceptionData();
	}

	localMessage->Free();

	return;
}


// ȭ��Ʈ IP
BOOL CContents::OnConnectionRequest(const WCHAR* userIP, WORD userPort)
{

	return TRUE;
}

void CContents::OnError(INT errorCode, const WCHAR* errorMessage)
{

}

void CContents::OnStop(void)
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
		
		// Ŭ���̾�Ʈ�� �̻��� �޽��� Type�� ���� 
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[procRecvMessage] Message Type Error, Session ID : %lld, messageType : %d\n", sessionID, messageType);
		
		break;
	}

	return FALSE;	
}

bool CContents::procEchoMessage(UINT64 sessionID, CMessage* pMessage)
{	
	UINT64 payload;

	*pMessage >> payload;
	
	packingEchoMessage(pMessage, payload);

	if (SendPacket(sessionID, pMessage) == FALSE)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[procRecvMessage] SendPacket Session Not Found, Session ID : %lld\n", sessionID);

		return FALSE;
	}

	return TRUE;
}

void CContents::packingEchoMessage(CMessage* pMessage, UINT64 payload)
{		
	*pMessage << payload;
}