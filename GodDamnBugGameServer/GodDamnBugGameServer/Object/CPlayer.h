#pragma once

class CPlayer : public CMMOServer::CSession, public CBaseObject
{
public:

	CPlayer(void);

	~CPlayer(void);

	virtual void OnAuthClientJoin(void) final;

	virtual void OnAuthClientLeave(void) final;

	// 로그인, 캐릭터 선택 컨텐츠 처리
	virtual void OnAuthMessage(CMessage* pMessage) final;

	virtual void OnGameClientJoin(void) final;

	virtual void OnGameClientLeave(void) final;

	virtual void OnGameMessage(CMessage* pMessage) final;

	void SetGodDamnBugPtr(CGodDamnBug* pGodDamnBug);

	static void SetPlayerDefaultValue(INT damage, INT HP, INT recoveryHP);

	void SetPlayerInfo(UINT64 clientID, BYTE characterType, FLOAT posX, FLOAT posY, INT tileX, INT tileY, WORD rotation, INT cristalCount, INT HP, INT64 EXP, WORD level, BOOL bDieFlag, INT damage, INT recoveryHP);

	void SetPlayerRespawnCoordinate(void);

	LONG GetDBWriteCount(void) const;
	UINT64 GetAccountNo(void) const;
	BOOL GetDieFlag(void) const;
	INT64 GetEXP(void) const;
	WORD GetLevel(void) const;
	INT GetHP(void) const;
	INT GetCristalCount(void) const;
	BYTE GetCharacterType(void) const;
	INT GetOldHP(void) const;
	DWORD GetRecoveryHPStartTick(void) const;

	
	void HPRecovery(void);

	void UpdateAttackDamage(INT damage);

	virtual void Update(void) final;
	virtual BOOL DeleteObject(void) final;

	LONG IncrementDBWriteCount(void);

	LONG DecrementDBWriteCount(void);

private:

	BOOL isDuplicateLogin(BYTE *pStatus);
	
	BOOL getPlayerInfoFromGameDB(void);

	BOOL authRecvProcedure(WORD messageType, CMessage* pMessage);
	BOOL gameRecvProcedure(WORD messageType, CMessage* pMessage);

	BOOL recvLoginRequest(CMessage* pMessage);
	BOOL recvCharacterSelectRequest(CMessage* pMessage);
	BOOL recvMoveCharacter(CMessage* pMessage);
	BOOL recvMoveStopCharacter(CMessage* pMessage);
	BOOL recvCharacterAttack1(CMessage* pMessage);
	BOOL recvCharacterAttack2(CMessage* pMessage);
	BOOL recvCristalPick(CMessage* pMessage);
	BOOL recvCharacterSit(CMessage* pMessage);
	BOOL recvPlayerRestart(CMessage* pMessage);


	// 리시브 패킷 검증
	BOOL validateRecvCharacterSelectRequest(BYTE characterType);
	BOOL validateRecvMoveCharacter(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE vKey, BYTE hKey);
	BOOL validateRecvMoveStopCharacter(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation);
	BOOL validateRecvCharacterAttack1(UINT64 clientID);
	BOOL validateRecvCharacterAttack2(UINT64 clientID);
	BOOL validateRecvCristalPick(UINT64 clientID);
	BOOL validateRecvCharacterSit(UINT64 clientID);
	BOOL validateRecvPlayerRestart(void);


	// send
	void sendLoginResponse(BYTE status);
	void sendCharacterSelectResponse(BYTE status);

	// 내 캐릭터 생성요청 보내기
	void sendCreateMyCharacter(void);
	
	// 주변 섹터에 있는 유저들에게 리스폰 캐릭터 생성 요청보내기
	void sendAroundRespawnCharacter(void);
	
	// 주변 섹터에 있는 다른 유저들 캐릭터 생성 요청 보내기
	void sendCreateAroundOthrerObject(void);


	// 플레이어 섹터가 변경되었을 때 호출하는 함수
	void sendAroundUpdateCharacterSector(void);

	// 삭제되는 섹터에 내캐릭터 삭제 요청 보내기
	void sendRemoveSectorDeleteMyCharacter(stSectorAround* pRemoveSectorAround);

	// 삭제되는 섹터에 있는 오브젝트 삭제요청 보내기
	void sendRemoveSectorDeleteObject(stSectorAround* pRemoveSectorAround);

	// 추가되는 섹터에 pPlayer 생성요청 보내기
	void sendAddSectorCreateMyCharacter(stSectorAround* pAddSectorAround);

	// 추가되는 섹터에 있는 오브젝트 생성요청 보내기
	void sendAddSectorCreateObject(stSectorAround* pAddSectorAround);

	// 주변 섹터에 해당 오브젝트 삭제요청 보내기
	void sendAroundRemoveObject(INT sectorPosX, INT sectorPosY, UINT64 clientID);

	void sendAroundCharacterMoveStart(void);
	void sendAroundCharacterMoveStop(void);
	void sendAroundCharacterAttack1(void);
	void sendAroundCharacterAttack2(void);
	void sendAroundCristalPickAction(void);
	void sendAroundCristalPick(UINT64 cristalClientID);
	void sendAroundCharacterSit(void);
	void sendAroundAttack1Damage(CMonster* pVictimMonster, INT damage);
	

	// 범위공격 용
	//void sendAroundAttack2Damage(CMonster** pVicTimMonsterArray, DWORD victimMonsterCount, INT damage, CGodDamnBug::stSectorAround16 *pSectorAround16);
	
	void sendAroundAttack2Damage(CMonster* pVictimMonster, INT damage);
	void sendAroundPlayerDie(INT minusCristal);	
	void sendPlayerHP(void);
	void sendPlayerRestart(void);

	// 플레이어의 섹터의 변경이 되었는지 확인한다.
	BOOL updatePlayerSector(void);

	BOOL getAttack1VictimMonster(CMonster** pVictimMonster);
	
	// 범위 공격용
	//BOOL getAttack2VictimMonsters(CMonster** pVictimMonsterArray, DWORD arrayLength, DWORD* pFindVictimMonsterCount, CGodDamnBug::stSectorAround16* pSectorAround16);

	BOOL getAttack2VictimMonster(CMonster** pVictimMonster);

	void playerRestart(BOOL bObjectRemoveFlag);

	void getCristalPickCollisionBox(stCollisionBox *pCollisionBox);
	BOOL pickCristal(CCristal** pCristal);

	// 플레이어가 Restart 를 하기위한 정보로 셋팅한다.
	void setRestartPlayerInfo(void);

	// mbLoginFlag를 FALSE로 변경해준다.
	void setAuthClientJoinState(void);


	// 캐릭터 생성 시 기본 공격력
	inline static INT mInitializeDamage = 0;

	// 캐릭터 생성 시 기본 HP
	inline static INT mInitializeHP = 0;

	// 캐릭터 생성 시 기본 회복 HP
	inline static INT mInitializeRecoveryHP = 0;

	UINT64 mAccountNo;

	BYTE mCharacterType;

	INT mCristalCount;

	INT mHP;

	INT64 mEXP;

	WORD mLevel;

	BOOL mbDieFlag;

	INT mDamage;

	INT mRecoveryHP;

	BOOL mbSitFlag;

	BOOL mbLoginFlag;

	//상하키 눌림여부. 0 (안눌림) - 1 (아래) - 2 (위)
	BYTE mVKey;

	//좌우키 눌림여부. 0 (안눌림) - 1 (왼쪽) - 2 (오른)
	BYTE mHKey;

	INT mOldHP;

	// 처음 앉은 시간 DB 저장용
	DWORD mRecoveryHPStartTick;

	// 시간에 따른 회복용
	DWORD mRecoveryHPLastTick;

	// DB 스레드에서 처리중인 요청건의 갯수, mDBWriteCount 가 0이 되기 전에 player를 해제하지 않는다.
	// 처리중이라면은 아직 접속중으로 새로운 세션이 접속하였을 때 해당 player 를 이어준다.
	LONG mDBWriteCount;


	CGodDamnBug* mpGodDamnBug;
	
	WCHAR mPlayerID[player::PLAYER_STRING];

	WCHAR mPlayerNick[player::PLAYER_STRING];
};

