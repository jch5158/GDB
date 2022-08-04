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

	// accept 직후 바로 호출
	virtual bool OnConnectionRequest(WCHAR* userIP, WORD userPort) final;

	// 게임서버 특성상 1 Loop 마다 처리하기 위한 함수이다. 이건 사실 필요없음
	virtual void OnAuthUpdate(void) final;

	// 게임서버 특성상 1 Loop 마다 처리하기 위한 함수이다.
	virtual void OnGameUpdate(void) final;

	// Player 와 Session 매핑을 위한 오버라이딩 함수
	virtual void OnAssociateSessionWithPlayer(CSession **pSession) final;

	virtual void OnReleaseSessionWithPlayer(CSession* pSession) final;

	virtual void OnCloseAuthenticThread(void) final;

	virtual void OnCloseUpdateThread(void) final;

	virtual void OnStop(void) final;
};

