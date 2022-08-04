#pragma once
class CObjectManager
{
public:

	static CObjectManager* GetInstance(void);

	void InsertObject(CBaseObject* pObject);

	void Update(void);

private:

	CObjectManager(void);

	~CObjectManager(void);

	void ObjectUpdate(void);

	std::list<CBaseObject*> mObjectList;
};

