#include "stdafx.h"
#include "CNetServer.h"
#include "CContents.h"



void conntents::packingEchoMessage(CMessage* pMessage, long long payload)
{
	*pMessage << payload;

	return;
}

void conntents::packingLoginMessage(CMessage* pMessage, long long payload)
{
	*pMessage << payload;

	return;
}

bool CContents::OnStart(void)
{
	return true;
}

void CContents::OnStop(void)
{
	return;
}

void CContents::OnStartAcceptThread(void)
{
	return;
}

void CContents::OnStartWorkerThread(void)
{
	return;
}


void CContents::OnCloseAcceptThread(void)
{
	return;
}

void CContents::OnCloseWorkerThread(void)
{
	return;
}

// 화이트 IP
bool CContents::OnConnectionRequest(const wchar_t* userIP, unsigned short userPort)
{
	return true;
}

void CContents::OnClientJoin(unsigned long long sessionID)
{
	loginResponse(sessionID);

	return;
}

void CContents::OnClientLeave(unsigned long long sessionID)
{

}


void CContents::OnRecv(unsigned long long sessionID, CMessage* pMessage)
{	
	if (packetProcedure(sessionID, REQ_ECHO, pMessage) == false)
		Disconnect(sessionID);

	return;
}


void CContents::OnError(unsigned int errorCode, const wchar_t* errorMessage)
{

	return;
}

bool CContents::packetProcedure(unsigned long long sessionID, unsigned short messageType, CMessage* pMessage)
{

	switch (messageType)
	{
	case REQ_ECHO:
		
		return echoRequest(sessionID, pMessage);

		break;

	default:
		
		// 클라이언트가 이상한 메시지 Type을 보냄 
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[packetProcedure] Message Type Error, Session ID : %lld, messageType : %d\n", sessionID, messageType);
		
		break;
	}

	return false;	
}

void CContents::loginResponse(unsigned long long sessionID)
{
	CMessage* pMessage = CMessage::Alloc();

	conntents::packingLoginMessage(pMessage, 0x7fffffffffffffff);

	SendPacket(sessionID, pMessage);

	pMessage->Free();
}

bool CContents::echoRequest(unsigned long long  sessionID, CMessage* pMessage)
{	
	long long payload;

	*pMessage >> payload;
	
	echoResponse(sessionID, payload);

	return true;
}

void CContents::echoResponse(unsigned long long sessionID, unsigned long long payload)
{
	CMessage* pMessage = CMessage::Alloc();

	conntents::packingEchoMessage(pMessage, payload);

	if (SendPacket(sessionID, pMessage) == false)
	{
		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"LanServer", L"[echoResponse] SendPacket Failed : %lld\n", sessionID);

		Disconnect(sessionID);
	}

	pMessage->Free();

	return;
}


