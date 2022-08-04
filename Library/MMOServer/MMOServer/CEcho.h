#pragma once

class CEcho : public CMMOServer
{
public:

	CEcho(void);

	virtual ~CEcho(void);

private:
	
	virtual bool OnStart(void) final;

	virtual void OnStartAuthenticThread(void) final;

	virtual void OnStartUpdateThread(void) final;

	// accept ���� �ٷ� ȣ��
	virtual bool OnConnectionRequest(WCHAR* userIP, WORD userPort) final;

	// ���Ӽ��� Ư���� 1 Loop ���� ó���ϱ� ���� �Լ��̴�. �̰� ��� �ʿ����
	virtual void OnAuthUpdate(void) final;

	// ���Ӽ��� Ư���� 1 Loop ���� ó���ϱ� ���� �Լ��̴�.
	virtual void OnGameUpdate(void) final;

	// Player �� Session ������ ���� �������̵� �Լ�
	virtual void OnAssociateSessionWithPlayer(CSession **pSession) final;

	virtual void OnReleaseSessionWithPlayer(CSession* pSession) final;

	virtual void OnCloseAuthenticThread(void) final;

	virtual void OnCloseUpdateThread(void) final;

	virtual void OnStop(void) final;
};

