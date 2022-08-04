#pragma once

class CCristal;
class CMonster;

// Ÿ�� ��ǥ
struct stTilePoint
{
	INT tileX;
	INT tileY;
};


// �浹���� �� ������ �ݱ� ������ Ž���ϱ� ���� ����ϴ� ����ü
struct stCollisionBox
{
	stTilePoint pointOne;
	stTilePoint pointTwo;
};

// Ÿ�� ��ǥ�� �̿��� ���� ��ǥ�� ���
struct stSector
{
	INT posX;
	INT posY;
};


// ������Ʈ ���� ���� 8���� ����� ���� �׸��� oldSector, curSector ����� ���� ���� �� �߰� ���� ��� �� ���
struct stSectorAround
{
	// �迭�� ��ȿ ����
	INT count;

	// �ִ� 9���� ����� ����
	stSector around[9];
};

struct stSectorAround16
{
	INT count;
	stSector around[16];
};

namespace goddamnbug
{

	// tile ��ǥ�� �������� ���� ��ǥ�� ���� �Լ�
	void GetSector(INT tileX, INT tileY, stSector* pSector);

	// sectorPosX, sectorPosY �� �������� �ֺ� ���� ����带 �������� �Լ�
	void GetSectorAround(INT sectorPosX, INT sectorPosY, stSectorAround* pSectorAround);

	void GetSectorAround16(INT sectorPosX, INT sectorPosY, stSectorAround16* pSectorAround16);

	// �÷��̾� ���� ������Ʈ ���� ������ ��� ���͵�� ���ο� ���͵��� ����.
	void GetUpdateSectorAround(stSector oldSector, stSector curSector, stSectorAround* pRemoveSectorAround, stSectorAround* pAddSectorAround);

	// ���⿡ ���� �浹 �ڽ��� ���ϴ� �Լ�
	void GetAttackCollisionBox(INT rotation, INT tileX, INT tileY, INT collisionRange, stCollisionBox* pCollisionBox);

	void GetAttackSectorAround16(INT rotation, stSector curSector, stSectorAround16* pAttackSectorAround16);


	// �α��� ��� ����
	void PackingLoginResponse(BYTE status, UINT64 accountNo, CMessage* pMessage);
	
	// ĳ���� ���� ���� ����
	void PackingCharacterSelectResponse(BYTE status, CMessage* pMessage);

	// �÷��̾� ���� �� �� ĳ���� ���� ��û ����
	void PackingCreateMyCharacter(UINT64 clientID, BYTE characterType, WCHAR* pNickName, DWORD cbNickName, FLOAT posX, FLOAT posY, WORD rotaion, INT cristalCount, INT HP, INT64 EXP, WORD level, CMessage* pMessage);
	
	// �ٸ� �÷��̾� ���� ���� [ �� ĳ���� �����̶��� ��Ŷ�� ��� �����Ͱ� �ٸ� hp,exp, ������ ����, sit ����, ũ����Ż ���� ]
	void PackingCreateOthreadCharacter(UINT64 clientID, BYTE characterType, WCHAR* pNickName, DWORD cbNickName, FLOAT posX, FLOAT posY, WORD rotation, WORD level, BYTE bRespawnFlag, BYTE bSitFlag, BYTE bDieFlag, CMessage* pMessage);	

	// ���� ���� ��Ŷ ����
	void PackingCreateMonster(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE bRespawnFlag, CMessage* pMessage);
	
	// ������Ʈ ���� ��Ŷ ������ ���� 
	void PackingRemoveObject(UINT64 clientID, CMessage* pMessage);

	// ���� ��ŸƮ ��Ŷ ���� 
	void PackingCharacterMoveStart(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, BYTE vKey, BYTE hKey, CMessage* pMessage);

	// ���� ��ž ��Ŷ ����
	void PackingCharacterMoveStop(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, CMessage* pMessage);
	

	// ���� �̵� ��Ŷ ����
	void PackingMonsterMoveStart(UINT64 clientID, FLOAT posX, FLOAT posY, WORD rotation, CMessage* pMessage);


	// ����1 ��� ��Ŷ ����
	void PackingPlayerAttack1Action(UINT64 clientID, CMessage* pMessage);

	// ����2 ��� ��Ŷ ����
	void PackingPlayerAttack2Action(UINT64 clientID, CMessage* pMessage);

	// ���� ���� ��Ŷ ����
	void PackingMonsterAttackAction(UINT64 clientID, CMessage* pMessage);

	// ũ����Ż �� �׼� ����
	void PackingCristalPickAction(UINT64 clientID, CMessage* pMessage);

	// ũ����Ż �� ���� 
	void PackingCristalPick(UINT64 clientID, UINT64 cristalClientID, INT playerCristalAmount, CMessage* pMessage);

	// �÷��̾� ���� �׼� �Ѹ���
	void PackingCharacterSit(UINT64 clientID, CMessage* pMessage);
	
	// �÷��̾� ����, ������ �� ���� �޴���.
	void PackingAttackDamage(UINT64 attackClientID, UINT64 victimClientID, INT damage, CMessage* pMessage);

	// �÷��̾� ����
	void PackingPlayerDie(UINT64 clientID, INT minusCristal, CMessage* pMessage);

	// ���� ���
	void PackingMonsterDie(UINT64 clientID, CMessage* pMessage);

	// ����� ������Ʈ�� ũ����Ż�� ������, ũ����Ż ���� ����
	void PackingCreateCristal(UINT64 clientID, BYTE cristalType, FLOAT posX, FLOAT posY, CMessage* pMessage);

	// �÷��̾� �ɾҴ� �Ͼ �� �÷��̾� HP ����
	void PackingPlayerHP(INT HP, CMessage* pMessage);
	
	// �÷��̾� ����� ��Ŷ ����
	// �ش� ��Ŷ�� ���� Ŭ���̾�Ʈ�� ��� ������Ʈ�� ������
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

	// ���Ϳ� ���� Ŭ���̾�Ʈ�鿡�� �۽� [ pExceptionPlayer �Ű������� ���ο��� �۽� ���θ� ���� ]
	void SendPacketOneSector(INT sectorPosX, INT sectorPosY, CPlayer* pExceptionPlayer, CMessage* pMessage);

	// ����� ���Ϳ� ���� ��Ŷ�� �۽� [ pExceptionPlayer �Ű������� ���ο��� �۽� ���θ� ���� ]
	void SendPacketAroundSector(INT sectorPosX, INT sectorPosY, CPlayer* pExceptionPlayer, CMessage* pMessage);

	// accountNo�� ID�� NickName�� ��ȸ�Ѵ�.
	BOOL GetIDWithNickFromAccountDB(UINT64 accountNo, WCHAR* pPlayerID, DWORD cbPlayerID, WCHAR* pPlayerNick, DWORD cbPlayerNick, BYTE *pStatus);

	// ���Ͱ� �װų� �÷��̾ ���� �� ũ����Ż ����
	void CreateCristal(FLOAT posX, FLOAT posY, INT tileX, INT tileY, stSector curSector);

	// ��ȿ �ð��� ������� ũ����Ż ����
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

	// accept ���� �ٷ� ȣ��
	virtual bool OnConnectionRequest(WCHAR* userIP, WORD userPort) final;

	virtual void OnAuthUpdate(void) final;                                    // AuthenticThread ���� �ֱ������� ȣ���ϴ� �Լ�

	// UpdateThread ���� ������ ó�������� 1 Loop�� ó���ϴ� �Լ��̴�.
	virtual void OnGameUpdate(void) final;

	// Player �� Session ������ ���� �������̵� �Լ�
	virtual void OnAssociateSessionWithPlayer(CSession **pSession) final;
	virtual void OnReleaseSessionWithPlayer(CSession* pSession) final;


	virtual void OnCloseAuthenticThread(void) final;

	virtual void OnCloseUpdateThread(void) final;

	virtual void OnStop(void) final;


	// gamddb�� Ŀ��Ʈ�ϴ� �Լ�
	void connectDB(void);

	// gamedb ���� ���� �Լ�
	void disconnectDB(void);

	// ĳ���� �⺻ �ɷ�ġ ����
	// �⺻ ���ݷ�, �⺻ ü��, �⺻ ü�� ȸ���� 
	void setPlayerStats(void);
	void getPlayerStatsFromGameDB(INT* pDamage, INT* pHP, INT* pRecoveryHP);

	void setMonsterStats(void);	
	void getMonsterStatsFromGameDB(INT monsterType, INT *pHP, INT* pDamage);


	// DB���� ũ����Ż Ÿ�Ժ� ������ ���� �� ũ����Ż Ÿ�Ժ� ���� ����
	void setCristalAmount(void);
	void getCristalAmountFromGameDB(INT typeAmountArray[]);
		

	// DB ������ ���� �غ�
	BOOL setupDBWriteThread(void);

	// DB ������ �ı�
	void closeDBWriteThread(void);

	// DB ������
	static DWORD WINAPI ExecuteDBWriteThread(void *pParam);

	// DB ������ �Լ�
	void DBWriteThread(void);


	// ���� ���� ����� ���͸� �����ϰ� �����Ѵ�.
	void setMonsters(void);
	void setArea1Monsters(void);
	void setArea2Monsters(void);
	void setArea3Monsters(void);
	void setArea4Monsters(void);
	void setArea5Monsters(void);
	void setArea6Monsters(void);
	void setArea7Monsters(void);


	void authReleasePlayers(void);

	

	// ������Ʈ���� ���� ��ȣ�� �Ҵ��ϱ� ���� ����
	UINT64 mObjectID;

	LONG mDBWriteThreadTPS;


	// DBThread ���Ḧ ���� ����
	BOOL mbDBWriteThreadFlag;

	DWORD mDBWriteThreadID;

	CLanLoginClient* mpLanLoginClient;

	HANDLE mDBWriteThreadHandle;

	HANDLE mDBWriteJobEvent;

	INT64 mLoginRequestCount;


	// ���� Ŀ����
	CRedisConnector mRedisConnector;

	// gamedb�� ���� Ŀ����
	CTLSDBConnector mDBConnector;

	CLockFreeQueue<CDBWriteJob*> mDBWriteJobQueue;

	// AuthenticThread���� GameThread�� �Ѿ�� ���ϰ� ���� �÷��̾ �����ϱ� ���� Map
	// key : accountNo, value : CPlayer* 
	std::unordered_map<UINT64, CPlayer*> mAuthReleasePlayerMap;

	
	// ��ü �÷��̾� ������ �����ϴ� Player Map 
	// �ߺ� �α��� ���θ� Ȯ���ϱ� ���ؼ� accountNo�� key �� ���
	// key : accountNo, value : CPlayer*
	std::unordered_map<UINT64, CPlayer*> mPlayerMap;

	
	// ���� ��ǥ���� ��ġ�� �ִ� CPlayer* ������ �����ϴ� �����̳�
	// key : clientID, value : CPlayer*
	std::unordered_map<UINT64, CPlayer*> mPlayerSectorMap[Y_SECTOR_RANGE][X_SECTOR_RANGE];	

	
	// ���� ��ǥ���� ��ġ�� �ִ� CMonster* ������ �����ϴ� �����̳�
	// key : clientID, value : CPlayer*
	std::unordered_map<UINT64, CMonster*> mMonsterSectorMap[Y_SECTOR_RANGE][X_SECTOR_RANGE];


	// ���� ��ǥ���� ��ġ�� �ִ� CCristal* ������ �����ϴ� �����̳�
	// key : clientID, value : CCristal*
	std::unordered_map<UINT64, CCristal*> mCristalSectorMap[Y_SECTOR_RANGE][X_SECTOR_RANGE];

};

