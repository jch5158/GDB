#include "stdafx.h"
#include "CAStar.h"


bool CAStar::CAscendingOrder::operator()(const stNode* pLeft, const stNode* pRight) const
{
	return pLeft->F < pRight->F;
}


CAStar::CAStar(int mapWidth, int mapHeight)
	: mMapWidth(mapWidth)
	, mMapHeight(mapHeight)
	
	, mDestinationNode{ 0, }

	, mOpenList()
	, mMap()
{
	mMap.resize(mapHeight);

	for (int indexY = 0; indexY < mapHeight; ++indexY)
	{
		mMap[indexY].resize(mMapWidth);

		// �� ����
		for (int indexX = 0; indexX < mapWidth; ++indexX)
		{
			mMap[indexY][indexX].x = indexX;
			mMap[indexY][indexX].y = indexY;
		}
	}
}

bool CAStar::PathFind(int startX, int startY, int destinationX, int destinationY, std::vector<stRouteNode> &route)
{
	if (startX < 0 || startY < 0 || startX >= mMapWidth || startY >= mMapHeight)
		return false;

	stNode* pStartNode = &mMap[startY][startX];

	// ������ ��� x,y ��ǥ ���� ( H ���� ���ϱ� ���ؼ��� �ʿ� )
	mDestinationNode.x = destinationX;
	mDestinationNode.y = destinationY;

	pStartNode->G = 0.0f;
	pStartNode->H = (float)(abs(startX - destinationX) + abs(startY - destinationY));
	pStartNode->F = pStartNode->G + pStartNode->H;
	pStartNode->pParentNode = nullptr;


	// ����� ��带 openList�� �߰��Ѵ�.
	mOpenList.push_back(pStartNode);

	for (;;)
	{
		stNode* pOpenListNode = findOpenNode();
		if (pOpenListNode == nullptr)
			break;
		else if (pOpenListNode->x == destinationX && pOpenListNode->y == destinationY)
		{
			setRouteArray(pOpenListNode, route);

			mOpenList.clear();

			resetNodeState();

			return true;
		}

		findRoute(pOpenListNode);

		// node�� F ���� �������� ���� ���� ������ �Ѵ�.
		mOpenList.sort(CAscendingOrder());
	}

	mOpenList.clear();

	resetNodeState();

	return false;
}



bool CAStar::SetMapAttribute(int x, int y, eNodeAttribute nodeAttribute)
{
	if (x < 0 || y < 0 || x >= mMapWidth || y >= mMapHeight)
		return false;

	mMap[y][x].nodeAttribute = nodeAttribute;

	return true;
}

void CAStar::ResetMapAttribute(void)
{
	for (int indexY = 0; indexY < mMapHeight; ++indexY)
	{
		for (int indexX = 0; indexX < mMapWidth; ++indexX)
			mMap[indexY][indexX].nodeAttribute = eNodeAttribute::NODE_UNBLOCK;
	}

	return;
}



void CAStar::findRoute(stNode* pNode)
{
	int x = pNode->x - 1;
	int y = pNode->y - 1;

	for (int indexY = 0; indexY < 3; ++indexY)
	{
		if (y + indexY < 0 || y + indexY >= mMapHeight)
			continue;

		for (int indexX = 0; indexX < 3; ++indexX)
		{
			if (x + indexX < 0 || x + indexX >= mMapWidth)
				continue;

			createNode(x + indexX, y + indexY, pNode);
		}
	}

	return;
}

void CAStar::createNode(int x, int y, stNode* pParentNode)
{
	// ��ֹ� �Ǵ� CLOSED ����� ��� ��带 �������� �ʴ´�.
	if (mMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK || mMap[y][x].nodeState == eNodeState::NODE_CLOSED)
		return;

	stNode* pOpenListNode = &mMap[y][x];
	if (pOpenListNode->nodeState == eNodeState::NODE_NONE)
	{	
		pOpenListNode->pParentNode = pParentNode;

		pOpenListNode->G = pParentNode->G + sqrt(abs(x - pParentNode->x) * abs(x - pParentNode->x) + abs(y - pParentNode->y) * abs(y - pParentNode->y));
		pOpenListNode->H = sqrt(abs(x - mDestinationNode.x) * abs(x - mDestinationNode.x) + abs(y - mDestinationNode.y) * abs(y - mDestinationNode.y));
		pOpenListNode->F = pOpenListNode->G + pOpenListNode->H;

		pOpenListNode->nodeState = eNodeState::NODE_OPENED;
		mOpenList.push_back(pOpenListNode);
	}
	else
	{
		// ���� �θ� ����� G���� �� Ŭ ��� G,H,F ���� ���� �����ϰ� �θ� ��带 ���� �̾��ش�.
		if (pParentNode->G < pOpenListNode->pParentNode->G)
		{
			pOpenListNode->G = pParentNode->G + sqrt(abs(x - pParentNode->x) * abs(x - pParentNode->x) + abs(y - pParentNode->y) * abs(y - pParentNode->y));
			pOpenListNode->F = pOpenListNode->G + pOpenListNode->H;
			pOpenListNode->pParentNode = pParentNode;
		}
	}

	return;
}

CAStar::stNode* CAStar::findOpenNode(void)
{
	const auto& iter = mOpenList.begin();
	if (iter == mOpenList.end())
		return nullptr;

	stNode* pNode = *iter;

	// openList���� �����Ѵ�.
	mOpenList.erase(iter);

	// openList���� ���� node�� CLOSED ���·� �����Ѵ�.
	mMap[pNode->y][pNode->x].nodeState = eNodeState::NODE_CLOSED;

	return pNode;
}



void CAStar::setRouteArray(stNode* pDestNode, std::vector<stRouteNode> &route)
{
	// ��� ����ȭ
	makeOptimizePath(pDestNode);

	int nodeCount = 0;

	stNode* pTempNode = pDestNode;

	for (;;)
	{
		pTempNode = pTempNode->pParentNode;

		if (pTempNode == nullptr)
			break;

		++nodeCount;
	}

	route.resize(nodeCount + 1);

	for (int index = nodeCount; index >= 0; --index)
	{
		route[index].bUseFlag = true;
		route[index].x = pDestNode->x;
		route[index].y = pDestNode->y;

		pDestNode = pDestNode->pParentNode;
	}

	return;
}



void CAStar::resetNodeState(void)
{
	for (int indexY = 0; indexY < mMapHeight; ++indexY)
	{
		for (int indexX = 0; indexX < mMapWidth; ++indexX)
			mMap[indexY][indexX].nodeState = eNodeState::NODE_NONE; 
	}

	return;
}



bool CAStar::makeBresenhamLine(int startX, int startY, int endX, int endY)
{
	int width = abs(startX - endX);
	int height = abs(startY - endY);

	int xFactor = startX > endX ? -1 : 1;
	int yFactor = startY > endY ? -1 : 1;

	if (width > height)
	{
		int y = startY;
		int dest = 2 * height - width;

		for (int x = startX; x != endX; x += xFactor)
		{
			if (dest < 0)
				dest += 2 * height;
			else
			{
				y += yFactor;
				dest += 2 * height - 2 * width;
			}

			if (mMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
				return false;
		}
	}
	else
	{
		int x = startX;
		int dest = 2 * width - height;

		for (int y = startY; y != endY; y += yFactor)
		{
			if (dest < 0)
				dest += 2 * width;
			else
			{
				x += xFactor;
				dest += 2 * width - 2 * height;
			}

			if (mMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
				return false;
		}
	}

	return true;
}


// ���� ������ ��Ʈ�� ã�� ��ο��� �����Ѵ�.
// ���� ������ ��Ʈ�� 3���� ��Ʈ�� ���� �� ��� ��Ʈ�� �����ϰ� �������� �̾��� �� ���̿� ��ֹ��� ���ٸ� ��� ��带 �����Ѵ�.
void CAStar::makeOptimizePath(stNode* pNode)
{
	for (;;)
	{
		stNode* pNextNode = pNode->pParentNode;
		if (pNextNode == nullptr)
			return;

		int startX = pNode->x;
		int startY = pNode->y;

		for (;;)
		{
			stNode* pNextNextNode = pNextNode->pParentNode;
			if (pNextNextNode == nullptr)
				return;

			int endX = pNextNextNode->x;
			int endY = pNextNextNode->y;

			if (makeBresenhamLine(startX, startY, endX, endY) == true)
			{
				pNode->pParentNode = pNextNextNode;

				pNextNode = pNextNextNode;
			}
			else
			{
				pNode = pNextNode;

				break;
			}
		}
	}

	return;
}
