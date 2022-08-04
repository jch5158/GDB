#pragma once


class CCristal : public CBaseObject
{
public:

	template <class DATA>
	friend class CTLSLockFreeObjectFreeList;

	static void SetCristalTypeAmount(INT typeAmountArray[]);

	static CCristal* Alloc(UINT64 clientID, FLOAT posX, FLOAT posY, INT tileX, INT tileY, stSector curSector, CGodDamnBug* pGodDamnBug);

	void Free(void);

	INT GetCristalAmount(void) const;

	virtual void Update(void) final;

	virtual BOOL DeleteObject(void) final;

	BYTE GetCristalType(void) const;

private:

	CCristal(void);

	~CCristal(void);

	static BYTE gatchaCristalType(void);
	void setCristalAmount(void);

	void checkCristalDeleteTime(void);
	
	
	BYTE mCristalType;

	INT mAmount;

	DWORD mCreateTime;

	CGodDamnBug* mpGodDamnBug;

	// key : type, value : amount
	inline static std::unordered_map<INT, INT> mCristalAmountMap;
	inline static CTLSLockFreeObjectFreeList<CCristal> mCristalFreeList;
};

