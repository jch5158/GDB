#pragma once

class CCristal;
class CMonster;

// 타일 좌표
struct stTilePoint
{
	INT tileX;
	INT tileY;
};


// 충돌범위 및 아이템 줍기 범위를 탐색하기 위해 사용하는 구조체
struct stCollisionBox
{
	stTilePoint pointOne;
	stTilePoint pointTwo;
};

// 타일 좌표를 이용해 섹터 좌표를 계산
struct stSector
{
	INT posX;
	INT posY;
};


// 오브젝트 섹터 기준 8방향 영향권 섹터 그리고 oldSector, curSector 계산을 통해 삭제 및 추가 섹터 계산 시 사용
struct stSectorAround
{
	// 배열의 유효 개수
	INT count;

	// 최대 9개의 영향권 섹터
	stSector around[9];
};

struct stSectorAround16
{
	INT count;
	stSector around[16];
};

namespace goddamnbug
{

	// tile 좌표를 기준으로 섹터 좌표를 얻어내는 함수
	void GetSector(INT tileX, INT tileY, stSector* pSector);

	// sectorPosX, sectorPosY 를 기준으로 주변 섹터 어라운드를 가져오는 함수
	void GetSectorAround(INT sectorPosX, INT sectorPosY, stSectorAround* pSectorAround);

	void GetSectorAround16(INT sectorPosX, INT sectorPosY, stSectorAround16* pSectorAround16);

	// 플레이어 섹터 업데이트 이후 범위를 벗어난 섹터들과 새로운 섹터들을 얻어낸다.
	void GetUpdateSectorAround(stSector oldSector, stSector curSector, stSectorAround* pRemoveSectorAround, stSectorAround* pAddSectorAround);

	// 방향에 따라 충돌 박스를 구하는 함수
	void GetAttackCollisionBox(INT rotation, INT tileX, INT tileY, INT collisionRange, stCollisionBox* pCollisionBox);

	void GetAttackSectorAround16(INT rotation, stSector curSector, stSectorAround16* pAttackSectorAround16);


	// 로그인 결과 응답
	void PackingLoginResponse(BYTE status, UINT64 accountNo, CMessage* pMessage);
	
	// 캐릭터 선택 성공 응답
	void PackingCharacterSelectResponse(BYTE status, CMessage* pMessage);

	// 플레이어 접속 시 내 캐릭터 생성 요청 응답
	void PackingCreateMyCharacter(UINT64 clientID, BYTE characterType, WCHAR* pNickName, DWORD cbNickName, FLOAT posX, FLOAT posY, WORD rotaion, INT cristalCount, INT HP, INT64 EXP, WORD level, CMessage* pMessage);
	
	// 다른 플레이어 생성 응답 [ 내 캐릭터 생성이랑은 패킷에 담는 데이터가 다름 hp,exp, 리스폰 여부, sit 여부, 크리스탈 개수 ]
	void PackingCreateOthreadCharacter(UINT64 clientID, BYTE characterType, WCHAR* pNickName, DWORD cbNickName, FLOAT posX, FLOAT posY, WORD rotation, WORD level, BYTE bRespawnFlag, BYTE bSitFlag, BYTE bDieFlag, CMessage* pMessage);	

	// 몬스터 생성 패킷 응답
	void PackingCreateMonster(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE bRespawnFlag, CMessage* pMessage);
	
	// 오브젝트 삭제 패킷 보내기 응답 
	void PackingRemoveObject(UINT64 clientID, CMessage* pMessage);

	// 무브 스타트 패킷 응답 
	void PackingCharacterMoveStart(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE vKey, BYTE hKey, CMessage* pMessage);

	// 무브 스탑 패킷 응답
	void PackingCharacterMoveStop(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, CMessage* pMessage);
	

	// 몬스터 이동 패킷 응답
	void PackingMonsterMoveStart(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, CMessage* pMessage);


	// 어택1 모션 패킷 응답
	void PackingPlayerAttack1Action(UINT64 clientID, CMessage* pMessage);

	// 어택2 모션 패킷 응답
	void PackingPlayerAttack2Action(UINT64 clientID, CMessage* pMessage);

	// 몬스터 어택 패킷 응답
	void PackingMonsterAttackAction(UINT64 clientID, CMessage* pMessage);

	// 크리스탈 픽 액션 응답
	void PackingCristalPickAction(UINT64 clientID, CMessage* pMessage);

	// 크리스탈 픽 응답 
	void PackingCristalPick(UINT64 clientID, UINT64 cristalClientID, INT playerCristalAmount, CMessage* pMessage);

	// 플레이어 앉음 액션 뿌리기
	void PackingCharacterSit(UINT64 clientID, CMessage* pMessage);
	
	// 플레이어 어택, 공격자 및 공격 받는자.
	void PackingAttackDamage(UINT64 attackClientID, UINT64 victimClientID, INT damage, CMessage* pMessage);

	// 플레이어 죽음
	void PackingPlayerDie(UINT64 clientID, INT minusCristal, CMessage* pMessage);

	// 몬스터 사망
	void PackingMonsterDie(UINT64 clientID, CMessage* pMessage);

	// 사망한 오브젝트가 크리스탈을 생성함, 크리스탈 생성 응답
	void PackingCreateCristal(UINT64 clientID, BYTE cristalType, FLOAT posX, FLOAT posY, CMessage* pMessage);

	// 플레이어 앉았다 일어날 때 플레이어 HP 보정
	void PackingPlayerHP(INT HP, CMessage* pMessage);
	
	// 플레이어 재시작 패킷 응답
	// 해당 패킷을 받은 클라이언트는 모든 오브젝트를 삭제함
	void PackingPlayerRestart(CMessage* pMessage);
};

class CGodDamnBug : public CMMOServer
{
public:
	
	CGodDamnBug(void);

	~CGodDamnBug(void);

	CRedisConnector* GetRedisConnector(void); 

	CTLSDBConnector* GetDBConnector(void);

	CLanLoginClient* GetLanLoginClient(void) const;

	LONG GetDBWriteThreadTPS(void) const;

	DWORD GetDBWriteThreadQueueSize(void);

	BOOL InsertPlayerMap(UINT64 accountNo, CPlayer* pPlayer);
	BOOL ErasePlayerMap(UINT64 accountNo);
	CPlayer* FindPlayerFromPlayerMap(UINT64 accountNo);
	
	BOOL InsertPlayerSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID, CPlayer* pPlayer);
	BOOL ErasePlayerSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID);
	std::unordered_map<UINT64, CPlayer*>* FindPlayerSectorMap(INT sectorPosX, INT sectorPosY);

	BOOL InsertCristalSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID, CCristal* pCristal);
	BOOL EraseCristalSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID);
	std::unordered_map<UINT64, CCristal*>* FindCristalSectorMap(INT sectorPosX, INT sectorPosY);

	
	BOOL InsertMonsterSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID, CMonster* pMonster);
	BOOL EraseMonsterSectorMap(INT sectorPosX, INT sectorPosY, UINT64 clientID);
	std::unordered_map<UINT64, CMonster*>* FindMonsterSectorMap(INT sectorPosX, INT sectorPosY);


	BOOL InsertAuthReleasePlayerMap(UINT64 accountNo, CPlayer* pPlayer);
	BOOL EraseAuthReleasePlayerMap(UINT64 accountNo);
	CPlayer* FindPlayerFromAuthReleasePlayerMap(UINT64 accountNo);


	void SetLoginClient(CLanLoginClient* pLanLoginClient);

	UINT64 GetObjectID(void);

	// 섹터에 속한 클라이언트들에게 송신 [ pExceptionPlayer 매개변수로 본인에게 송신 여부를 결정 ]
	void SendPacketOneSector(INT sectorPosX, INT sectorPosY, CPlayer* pExceptionPlayer, CMessage* pMessage);

	// 영향권 섹터에 전부 패킷을 송신 [ pExceptionPlayer 매개변수로 본인에게 송신 여부를 결정 ]
	void SendPacketAroundSector(INT sectorPosX, INT sectorPosY, CPlayer* pExceptionPlayer, CMessage* pMessage);

	// accountNo로 ID와 NickName을 조회한다.
	BOOL GetIDWithNickFromAccountDB(UINT64 accountNo, WCHAR* pPlayerID, DWORD cbPlayerID, WCHAR* pPlayerNick, DWORD cbPlayerNick, BYTE *pStatus);

	// 몬스터가 죽거나 플레이어가 죽을 때 크리스탈 생성
	void CreateCristal(FLOAT posX, FLOAT posY, INT tileX, INT tileY, stSector curSector);

	// 유효 시간이 지날경우 크리스탈 삭제
	void RemoveCristal(CCristal *pCristal);

	void InsertLoginDBWriteJob(BYTE status, CPlayer* pPlayer);
	void InsertLogoutDBWriteJob(BYTE status, CPlayer* pPlayer);
	void InsertDieDBWriteJob(CPlayer* pPlayer);
	void InsertCreateCharacterDBWriteJob(CPlayer* pPlayer);
	void InsertRestartDBWriteJob(CPlayer* pPlayer);
	void InsertCristalPickDBWriteJob(INT getCristalCount, CPlayer* pPlayer);
	void InsertHPRecoveryDBWriteJob(CPlayer* pPlayer);

	void InitializeDBWriteThreadTPS(void);

	BOOL CompareSessionToken(UINT64 accountNo, CHAR* pSessionToken);

	void SendAroundCreateCristal(CCristal* pCristal);

	void SendAroundRemoveCristal(CCristal* pCristal);

	void IncrementLoginRequestCount(void);

	INT64 GetLoginRequestCount(void) const;


	SRWLOCK mPlayerMapLock;


private:

	virtual bool OnStart(void) final;

	virtual void OnStartAuthenticThread(void) final;

	virtual void OnStartUpdateThread(void) final;

	// accept 직후 바로 호출
	virtual bool OnConnectionRequest(WCHAR* userIP, WORD userPort) final;

	virtual void OnAuthUpdate(void) final;                                    // AuthenticThread 에서 주기적으로 호출하는 함수

	// UpdateThread 에서 켄텐츠 처리용으로 1 Loop당 처리하는 함수이다.
	virtual void OnGameUpdate(void) final;

	// Player 와 Session 매핑을 위한 오버라이딩 함수
	virtual void OnAssociateSessionWithPlayer(CSession **pSession) final;
	virtual void OnReleaseSessionWithPlayer(CSession* pSession) final;


	virtual void OnCloseAuthenticThread(void) final;

	virtual void OnCloseUpdateThread(void) final;

	virtual void OnStop(void) final;


	// gamddb에 커넥트하는 함수
	void connectDB(void);

	// gamedb 연결 끊는 함수
	void disconnectDB(void);

	// 캐릭터 기본 능력치 셋팅
	// 기본 공격력, 기본 체력, 기본 체력 회복력 
	void setPlayerStats(void);
	void getPlayerStatsFromGameDB(INT* pDamage, INT* pHP, INT* pRecoveryHP);

	void setMonsterStats(void);	
	void getMonsterStatsFromGameDB(INT monsterType, INT *pHP, INT* pDamage);


	// DB에서 크리스탈 타입별 가격을 읽은 후 크리스탈 타입별 가격 셋팅
	void setCristalAmount(void);
	void getCristalAmountFromGameDB(INT typeAmountArray[]);
		

	// DB 스레드 생성 준비
	BOOL setupDBWriteThread(void);

	// DB 스레드 파괴
	void closeDBWriteThread(void);

	// DB 스레드
	static DWORD WINAPI ExecuteDBWriteThread(void *pParam);

	// DB 스레드 함수
	void DBWriteThread(void);


	// 게임 서버 실행시 몬스터를 생성하고 셋팅한다.
	void setMonsters(void);
	void setArea1Monsters(void);
	void setArea2Monsters(void);
	void setArea3Monsters(void);
	void setArea4Monsters(void);
	void setArea5Monsters(void);
	void setArea6Monsters(void);
	void setArea7Monsters(void);


	void authReleasePlayers(void);

	

	// 오브젝트마다 고유 번호를 할당하기 위한 변수
	UINT64 mObjectID;

	LONG mDBWriteThreadTPS;


	// DBThread 종료를 위한 변수
	BOOL mbDBWriteThreadFlag;

	DWORD mDBWriteThreadID;

	CLanLoginClient* mpLanLoginClient;

	HANDLE mDBWriteThreadHandle;

	HANDLE mDBWriteJobEvent;

	INT64 mLoginRequestCount;


	// 레디스 커넥터
	CRedisConnector mRedisConnector;

	// gamedb를 위한 커넥터
	CTLSDBConnector mDBConnector;

	CLockFreeQueue<CDBWriteJob*> mDBWriteJobQueue;

	// AuthenticThread에서 GameThread로 넘어가지 못하고 끊긴 플레이어를 정리하기 위한 Map
	// key : accountNo, value : CPlayer* 
	std::unordered_map<UINT64, CPlayer*> mAuthReleasePlayerMap;

	
	// 전체 플레이어 정보를 보관하는 Player Map 
	// 중복 로그인 여부를 확인하기 위해서 accountNo를 key 로 사용
	// key : accountNo, value : CPlayer*
	std::unordered_map<UINT64, CPlayer*> mPlayerMap;

	
	// 섹터 좌표마다 위치에 있는 CPlayer* 정보를 관리하는 컨테이너
	// key : clientID, value : CPlayer*
	std::unordered_map<UINT64, CPlayer*> mPlayerSectorMap[Y_SECTOR_RANGE][X_SECTOR_RANGE];	

	
	// 섹터 좌표마다 위치에 있는 CMonster* 정보를 관리하는 컨테이너
	// key : clientID, value : CPlayer*
	std::unordered_map<UINT64, CMonster*> mMonsterSectorMap[Y_SECTOR_RANGE][X_SECTOR_RANGE];


	// 섹터 좌표마다 위치에 있는 CCristal* 정보를 관리하는 컨테이너
	// key : clientID, value : CCristal*
	std::unordered_map<UINT64, CCristal*> mCristalSectorMap[Y_SECTOR_RANGE][X_SECTOR_RANGE];

};

