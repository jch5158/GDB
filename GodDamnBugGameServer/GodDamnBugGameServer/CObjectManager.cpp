#include "stdafx.h"

CObjectManager::CObjectManager(void)
	:mObjectList()
{
}

CObjectManager::~CObjectManager(void)
{
}


CObjectManager* CObjectManager::GetInstance(void)
{
	static CObjectManager objectManager;

	return &objectManager;
}


void CObjectManager::InsertObject(CBaseObject* pObject)
{
	mObjectList.push_back(pObject);

	return;
}

void CObjectManager::Update(void)
{
	ObjectUpdate();

	return;
}

void CObjectManager::ObjectUpdate(void)
{
	auto iterE = mObjectList.end();

	for (auto iter = mObjectList.begin(); iter != iterE;)
	{
		(*iter)->Update();

		if ((*iter)->DeleteObject() == TRUE)
		{
			iter = mObjectList.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	return;
}
