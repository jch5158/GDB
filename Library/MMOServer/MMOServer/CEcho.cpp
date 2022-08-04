#include "stdafx.h"


CEcho::CEcho(void)
{

}

CEcho::~CEcho(void)
{

}

bool CEcho::OnStart(void)
{

	return true;
}

void CEcho::OnStartAuthenticThread(void)
{

	return;
}

void CEcho::OnStartUpdateThread(void)
{

	return;
}


// accept 직후 바로 호출
bool CEcho::OnConnectionRequest(WCHAR* userIP, WORD userPort)
{

	return true;
}

void CEcho::OnAuthUpdate()
{
	return;
}

// 게임서버 특성상 1 Loop 마다 처리하기 위한 함수이다.
void CEcho::OnGameUpdate(void)
{
	return;
}

// Player 와 Session 매핑을 위한 오버라이딩 함수
void CEcho::OnAssociateSessionWithPlayer(CSession** pSession)
{
	*pSession = new CPlayer;

	return;
}

void CEcho::OnReleaseSessionWithPlayer(CSession* pSession)
{
	delete pSession;

	return;
}

void CEcho::OnCloseAuthenticThread(void)
{

	return;
}

void CEcho::OnCloseUpdateThread(void)
{

	return;
}

void CEcho::OnStop(void)
{
	return;
}
