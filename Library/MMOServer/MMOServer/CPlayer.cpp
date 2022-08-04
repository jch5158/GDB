#include "stdafx.h"
#include "CPlayer.h"



CPlayer::CPlayer(void)
	:mAccountNumber(0)
{
}

CPlayer::~CPlayer(void)
{
}



void CPlayer::OnAuthClientJoin(void)
{
	return;
}

void CPlayer::OnAuthClientLeave(void)
{
	if (CheckLogoutInAuth() == true)
		SetRelease();

	return;
}

void CPlayer::OnAuthMessage(CMessage* pMessage)
{
	if (authPacketProcedure(pMessage) == false)
		Disconnect();

	return;
}

void CPlayer::OnGameClientJoin(void)
{

	return;
}

void CPlayer::OnGameClientLeave(void)
{
	// Release 될 수 있도록 한다.
	SetRelease();

	return;
}

void CPlayer::OnGameMessage(CMessage* pMessage)
{
	if (gamePacketProcedure(pMessage) == false)
		Disconnect();

	return;
}

bool CPlayer::loginRequest(CMessage* pMessage)
{
	*pMessage >> mAccountNumber;

	SetAuthToGame();

	loginResponse();

	return true;
}

void CPlayer::loginResponse(void)
{
	CMessage* pMessage = CMessage::Alloc();

	echoserver::packingLoginResponse(true, mAccountNumber, pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}


void echoserver::packingLoginResponse(BYTE loginResult, UINT64 accountNo, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_LOGIN << loginResult << accountNo;

	return;
}



bool CPlayer::echoRequest(CMessage* pMessage)
{
	UINT64 accountNumber;
	INT64 sendTick;

	*pMessage >> accountNumber >> sendTick;

	if (accountNumber != mAccountNumber)
		return false;

	echoResponse(sendTick);

	return true;
}

void CPlayer::echoResponse(INT64 sendTick)
{
	CMessage* pMessage = CMessage::Alloc();

	echoserver::packingEchoResponse(mAccountNumber, sendTick, pMessage);

	SendPacket(pMessage);

	pMessage->Free();

	return;
}

void echoserver::packingEchoResponse(UINT64 accountNo, INT64 sendTick, CMessage* pMessage)
{
	*pMessage << (WORD)en_PACKET_CS_GAME_RES_ECHO << accountNo << sendTick;

	return;
}



bool CPlayer::authPacketProcedure(CMessage* pMessage)
{
	WORD messageType;
	*pMessage >> messageType;

	switch (messageType)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:

		return loginRequest(pMessage);

	default:

		// 원래는 세션을 끊어야 하지만 테스트를 위해서 Crash()를 호출한다.
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[messageProc] Echo Test Server Error");

		CCrashDump::Crash();

		return false;
	}
}



bool CPlayer::gamePacketProcedure(CMessage* pMessage)
{
	WORD messageType;

	*pMessage >> messageType;

	switch (messageType)
	{
	case en_PACKET_CS_GAME_REQ_ECHO:

		return echoRequest(pMessage);

	default:

		// 원래는 세션을 끊어야 하지만 테스트를 위해서 Crash()를 호출한다.
		CSystemLog::GetInstance()->Log(true, CSystemLog::eLogLevel::LogLevelError, L"MMOServer", L"[messageProc] Echo Test Server Error");

		CCrashDump::Crash();

		return false;
	}
}


