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


// accept ���� �ٷ� ȣ��
bool CEcho::OnConnectionRequest(WCHAR* userIP, WORD userPort)
{

	return true;
}

void CEcho::OnAuthUpdate()
{
	return;
}

// ���Ӽ��� Ư���� 1 Loop ���� ó���ϱ� ���� �Լ��̴�.
void CEcho::OnGameUpdate(void)
{
	return;
}

// Player �� Session ������ ���� �������̵� �Լ�
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
