#include "stdafx.h"
#include "CJumpPointSearch.h"


bool CJumpPointSearch::CAscendingOrder::operator()(const stNode* pLeft, const stNode* pRight) const
{
	return pLeft->F < pRight->F;
}



CJumpPointSearch::CJumpPointSearch(int mapWidth, int mapHeight)
	: mMapWidth(mapWidth)
	, mMapHeight(mapHeight)

	, mDestinationNode{ 0, }

	, mOpenList()
	
	, mJspMap()

{	
	mJspMap.resize(mapWidth);

	for (int idxY = 0; idxY < mMapHeight; ++idxY)
	{
		mJspMap[idxY].resize(mapWidth);

		for (int idxX = 0; idxX < mMapWidth; ++idxX)
		{
			mJspMap[idxY][idxX].x = idxX;
			mJspMap[idxY][idxX].y = idxY;
		}
	}
}


bool CJumpPointSearch::SetMapAttribute(int x, int y, eNodeAttribute nodeAttribute)
{
	if (x < 0 || y < 0 || x >= mMapWidth || y >= mMapHeight)
		return false;

	mJspMap[y][x].nodeAttribute = nodeAttribute;

	return true;
}

void CJumpPointSearch::ResetMapAttribute(void)
{
	for (int idxY = 0; idxY < mMapHeight; ++idxY)
	{
		for (int idxX = 0; idxX < mMapWidth; ++idxX)
			mJspMap[idxY][idxX].nodeAttribute = eNodeAttribute::NODE_UNBLOCK;
	}

	return;
}




bool CJumpPointSearch::PathFind(int startX, int startY, int destX, int destY, std::vector<stRouteNode> &route)
{	
	if (startX < 0 || startY < 0 || startX >= mMapWidth || startY >= mMapHeight)
		return false;

	stNode* pStartNode = &mJspMap[startY][startX];

	pStartNode->G = 0.0f;
	pStartNode->H = (float)sqrt(abs(startX - destX) * abs(startX - destX) + abs(startY - destY) * abs(startY - destY));
	pStartNode->F = pStartNode->G + pStartNode->H;
	pStartNode->nodeDir = eNodeDirection::NODE_DIR_ALL;
	pStartNode->pParentNode = nullptr;

	// ������ ��� ����
	mDestinationNode.x = destX;
	mDestinationNode.y = destY;

	// ���� ����Ʈ�� �߰�
	mOpenList.push_back(pStartNode);

	for (;;)
	{
		// ���� ���
		CJumpPointSearch::stNode* curNode = findOpenNode();
		if (curNode == nullptr)
			break;
		else if (curNode->x == mDestinationNode.x && curNode->y == mDestinationNode.y)
		{
			setRouteArray(curNode, route);

			// ���� ����Ʈ ����
			mOpenList.clear();

			// close ����Ʈ ����
			resetNodeState();

			return true;
		}

		// �� ã�� ���� ����
		findRoute(curNode);

		// openList F�� ����
		mOpenList.sort(CAscendingOrder());
	}

	// ���� ����Ʈ ����
	mOpenList.clear();

	// close ����Ʈ ����
	resetNodeState();

	return false;
}


//================================================================
// ���� ����Ʈ���� ���ĵǾ� �ִ� F�� ��带 �̴´�.
//================================================================
CJumpPointSearch::stNode* CJumpPointSearch::findOpenNode()
{
	// F���� ���� ������ �����س��� ������ begin���� ������ ù ��带 ���� �� �ִ�.
	const auto& iter = mOpenList.begin();

	// ���¸���Ʈ�� ���ٸ��� nullptr return �Ѵ�.
	if (iter == mOpenList.end())
		return nullptr;

	// ��� �����͸� �����Ѵ�.
	stNode* node = *iter;

	// ���� ����Ʈ���� �ش� ��带 �����.
	mOpenList.erase(iter);

	mJspMap[node->y][node->x].nodeState = eNodeState::NODE_CLOSED;

	return node;
}



void CJumpPointSearch::findRoute(CJumpPointSearch::stNode* node)
{
	// ��� ���⿡ �´� Ž���Լ��� ȣ���Ѵ�.
	switch (node->nodeDir)
	{
	case eNodeDirection::NODE_DIR_RR:

		searchHorizontalRight(node);

		break;
	case eNodeDirection::NODE_DIR_RD:

		searchRightDown(node);

		break;
	case eNodeDirection::NODE_DIR_DD:

		searchVerticalDown(node);

		break;
	case eNodeDirection::NODE_DIR_LD:

		searchLeftDown(node);

		break;
	case eNodeDirection::NODE_DIR_LL:

		searchHorizontalLeft(node);

		break;
	case eNodeDirection::NODE_DIR_LU:

		searchLeftUp(node);

		break;
	case eNodeDirection::NODE_DIR_UU:

		searchVerticalUp(node);

		break;
	case eNodeDirection::NODE_DIR_RU:

		searchRightUp(node);

		break;
	case eNodeDirection::NODE_DIR_ALL:

		searchRightUp(node, true);

		searchHorizontalRight(node);

		searchRightDown(node, true);

		searchVerticalDown(node);

		searchLeftDown(node, true);

		searchHorizontalLeft(node);

		searchLeftUp(node, true);

		searchVerticalUp(node);

		break;

	default:

		CSystemLog::GetInstance()->Log(TRUE, CSystemLog::eLogLevel::LogLevelError, L"JumpPointSearch", L"[findRoute] Node Direction : %d", node->nodeDir);

		CCrashDump::Crash();

		break;
	}

	return;
}


void CJumpPointSearch::setNewNode(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	if (mJspMap[y][x].nodeState == eNodeState::NODE_CLOSED)
		return;


	// ���¸���Ʈ�� ���� ������� ����
	stNode* pOpenListNode = &mJspMap[y][x];
	if (pOpenListNode->nodeState == eNodeState::NODE_NONE)
	{
		pOpenListNode->G = pParentNode->G + sqrt(abs(pParentNode->x - x) * abs(pParentNode->x - x) + abs(pParentNode->y - y) * abs(pParentNode->y - y));
		pOpenListNode->H = sqrt(abs(mDestinationNode.x - x) * abs(mDestinationNode.x - x) + abs(mDestinationNode.y - y) * abs(mDestinationNode.y - y));
		pOpenListNode->F = pOpenListNode->H + pOpenListNode->G;

		pOpenListNode->nodeDir = nodeDir;

		pOpenListNode->pParentNode = pParentNode;

		pOpenListNode->nodeState = eNodeState::NODE_OPENED;

		mOpenList.push_back(pOpenListNode);
	}
	else
	{
		// ������ ���¸���Ʈ�� �־��� ��� �θ��� G ���� parentNode�� G�� ���� ũ�ٸ�
		// parentNode�� �θ�μ� ���� �̾��ִ� ������ �Ѵ�.
		if (pParentNode->G < pOpenListNode->pParentNode->G)
		{
			pOpenListNode->G = pParentNode->G + sqrt(abs(pParentNode->x - x) * abs(pParentNode->x - x) + abs(pParentNode->y - y) * abs(pParentNode->y - y));
			pOpenListNode->F = pOpenListNode->H + pOpenListNode->G;
			pOpenListNode->pParentNode = pParentNode;
		}
	}

	return;
}


//=============================================================
// Ŭ���� ���� �����Ѵ�.
//=============================================================
void CJumpPointSearch::resetNodeState(void)
{
	for (int indexY = 0; indexY < mMapHeight; ++indexY)
	{
		for (int indexX = 0; indexX < mMapWidth; ++indexX)
			mJspMap[indexY][indexX].nodeState = eNodeState::NODE_NONE;
	}

	return;
}


// commit
//===============================================
// ���� �� ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchVerticalUp(stNode* pParentNode)
{		
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (y > 0)
	{
		//=================================================================================
		// ù Ž�� �� ���� �� ���� Ž�� ������ Ȯ���մϴ�.
		//=================================================================================
		if (x > 0)
		{
			if (mJspMap[y][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftUp(pParentNode, true);  // ���� �� ����Ž��
		}

		//=================================================================================
		// ù Ž�� �� ������ �� ���� Ž�� ������ Ȯ���մϴ�.
		//=================================================================================
		if (x + 1 < mMapWidth)
		{
			if (mJspMap[y][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightUp(pParentNode, true);  // ������ �� ����Ž��
		}
	}
	

	for (int indexY = y - 1; indexY >= 0; --indexY)
	{
		if (mJspMap[indexY][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		if (x == mDestinationNode.x && indexY == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, indexY);

			return;
		}


		if (indexY > 0)
		{
			if (x > 0)
			{
				if (mJspMap[indexY][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY - 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, indexY);

					return;
				}
			}


			if (x + 1 < mMapWidth)
			{
				if (mJspMap[indexY][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY - 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, indexY);

					return;
				}
			}
		}
	}

	return;
}


// commit
//===============================================
// ���� �Ʒ� ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchVerticalDown(stNode* pParentNode)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if(y + 1 < mMapHeight)
	{
		//=================================================================================
		// ù Ž�� �� ���� �Ʒ� ���� Ž�� ������ Ȯ���մϴ�.
		//=================================================================================	
		if (x > 0)
		{
			if (mJspMap[y][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftDown(pParentNode, true);  //  ���� �Ʒ� ����Ž��
		}

		//=================================================================================
		// ù Ž�� �� ������ �Ʒ� ���� Ž�� ������ Ȯ���մϴ�.
		//=================================================================================
		if (x + 1 < mMapWidth)
		{
			if (mJspMap[y][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightDown(pParentNode, true); // ������ �Ʒ� ����Ž��

		}
	}	


	for (int indexY = y + 1; indexY < mMapHeight; ++indexY)
	{
		// ��ֹ��� ����ĥ ��� return �Ѵ�.
		if (mJspMap[indexY][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		// �������� ������ ��� �������� �̾��ְ� return �Ѵ�.
		if (x == mDestinationNode.x && indexY == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, indexY);

			return;
		}

		if (indexY + 1 < mMapHeight)
		{
			if (x > 0)
			{
				if (mJspMap[indexY][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY + 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, indexY);

					return;
				}
			}

			if (x + 1 < mMapWidth)
			{
				if (mJspMap[indexY][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY + 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, indexY);

					return;
				}
			}
		}
	}

	return;
}



// commit
//===============================================
// ������ ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchHorizontalRight(stNode* pParentNode)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	//=================================================================================
	// x ��ǥ�� x + 1 < mMapWidth �� ��쿡�� ������ �����մϴ�. 
	//=================================================================================
	if (x + 1 < mMapWidth)
	{
		//=================================================================================
		// ù Ž�� �� ������ �� ���� Ž�� ������ Ȯ���Ѵ�.
		//=================================================================================
		if (y > 0)
		{
			if (mJspMap[y - 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightUp(pParentNode, true);   // ������ �� �밢�� ����Ž��
		}

		//=================================================================================
		// ù Ž�� �� ������ �Ʒ� ���� Ž�� ������ Ȯ���Ѵ�.
		//=================================================================================
		if (y + 1 < mMapHeight)
		{
			if (mJspMap[y + 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightDown(pParentNode, true);   // ������ �Ʒ� �밢�� ����Ž��
		}
	}

	//===============================================================================
	// indexX = x + 1 �� �ؼ� ���� ��ġ���� Ž���� �Ѵ�. 
	//===============================================================================
	for (int indexX = x + 1; indexX < mMapWidth; ++indexX)
	{
		// ���� ������ ��� �ƹ��͵� ���� �ʰ� return 
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		// �������� ���� ��� �ٷ� �̾��ְ� return �Ѵ�. 
		if (indexX == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, indexX, y);

			return;
		}

		//=============================================================================
		// indexX + 1 ���� mMapWidth ���� ���� ��쿡�� �����÷ο� ���� x + 1 ��ġ�� Ȯ���� �� �ִ�.
		//=============================================================================
		if (indexX + 1 < mMapWidth)
		{
			//=============================================================================
			// y ���� 1 �̻��� ��쿡�� ���� ���� Ȯ���� �� �ִ�.
			//=============================================================================
			if (y > 0)
			{
				if (mJspMap[y - 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][indexX + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					// �ش� ��ġ�� ��� �����
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, indexX, y);

					return;
				}
			}

			//============================================================================
			// y ���� y + 1 < mMapHeight �� ��쿡�� �Ʒ� ���� Ȯ���� �� �ִ�.	
			//============================================================================
			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][indexX + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, indexX, y);

					return;
				}
			}
		}
	}

	return;
}


// commit
//===============================================
// ���� ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchHorizontalLeft(stNode* pParentNode)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (x > 0)
	{
		//=================================================================================
		// ù Ž�� �� ���� �� ���� Ž�� ������ Ȯ���մϴ�.
		//=================================================================================
		if (y > 0)
		{
			if (mJspMap[y - 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftUp(pParentNode, true);
		}

		//=================================================================================
		// ù Ž�� �� ���� �Ʒ� ���� Ž�� ������ Ȯ���մϴ�.
		//=================================================================================
		if (y + 1 < mMapHeight)
		{
			if (mJspMap[y + 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftDown(pParentNode, true);
		}
	}


	for (int indexX = x - 1; indexX >= 0; --indexX)
	{
		// �ش� ��ǥ�� ��ֹ��� ������ �ٷ� return �Ѵ�.
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		// �������� ������ �̾��ְ� �ٷ� return �Ѵ�.
		if (indexX == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, indexX, y);

			return;
		}

		if (indexX > 0)
		{
			if (y > 0)
			{
				if (mJspMap[y - 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][indexX - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, indexX, y);

					return;
				}
			}

			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][indexX - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, indexX, y);

					return;
				}
			}
		}
	}

	return;
}


// commit
//======================================================
// ������ �� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchRightUp(stNode* pParentNode, bool bAssistanceCallFlag)
{		
	// ���� Ž���� �ƴ� ��� �Ʒ� �Լ��� ȣ���Ѵ�.
	if (bAssistanceCallFlag == false)
	{	
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
		searchHorizontalRight(pParentNode);

		searchVerticalUp(pParentNode);		
		//===================================================================
	}

	int x = pParentNode->x;
	int y = pParentNode->y;

	for(;;)
	{
		x += 1;
		y -= 1;

		if (x >= mMapWidth || y < 0 || mJspMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			break;

		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{			
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RU, x, y);

			return;
		}

		// ������ Ž��
		if (searchHorizontalRightForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RU, x, y) == true)
			return;

		// ���� Ž��
		if (searchVerticalUpForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RU, x, y) == true)
			return;
	}

	return;
}

// commit
//======================================================
// ������ �Ʒ� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchRightDown(stNode* pParentNode, bool bAssistanceCallFlag)
{		
	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
		searchHorizontalRight(pParentNode);
		
		searchVerticalDown(pParentNode);	
		//===================================================================
	}

	int x = pParentNode->x;
	int y = pParentNode->y;

	for(;;)
	{
		x += 1;
		y += 1;

		if (x >= mMapWidth || y >= mMapHeight || mJspMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			break;


		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{			
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RD, x, y);

			return;
		}

		if (searchHorizontalRightForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RD, x, y) == true)
			return;

		if (searchVerticalDownForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RD, x, y) == true)
			return;
	}

	return;
}


// commit
//======================================================
// ���� �� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchLeftUp(stNode* pParentNode, bool bAssistanceCallFlag)
{
	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
		searchHorizontalLeft(pParentNode);	

		searchVerticalUp(pParentNode);		
		//===================================================================
	}

	int x = pParentNode->x;
	int y = pParentNode->y;

	for(;;)
	{
		x -= 1;
		y -= 1;

		if (x < 0 || y < 0 || mJspMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			break;


		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{	
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LU, x, y);

			return;
		}

		if (searchHorizontalLeftForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LU, x, y) == true)
			return;

		if (searchVerticalUpForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LU, x, y) == true)
			return;
	}

	return;
}

// commit
//======================================================
// ���� �Ʒ� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchLeftDown(stNode* pParentNode, bool bAssistanceCallFlag)
{
	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
		searchHorizontalLeft(pParentNode);	

		searchVerticalDown(pParentNode);		
		//===================================================================
	}

	int x = pParentNode->x;
	int y = pParentNode->y;

	for(;;)
	{
		x -= 1;
		y += 1;

		if (x < 0 || y >= mMapHeight || mJspMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			break;


		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LD, x, y);

			return;
		}

		if (searchHorizontalLeftForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LD, x, y) == true)
			return;

		if (searchVerticalDownForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LD, x, y) == true)
			return;
	}

	return;
}


// commit
//=======================================================
// �밢�� �Լ��� ���� ������ ���� 
//=======================================================
bool CJumpPointSearch::searchHorizontalRightForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	for (int indexX = x; indexX < mMapWidth; ++indexX)
	{
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;
	
		// ������ ��带 ������ ��� ��� ���� �� return
		if (indexX == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);
			
			return true;
		}

		if (indexX + 1 < mMapWidth)
		{
			if (y > 0)
			{
				if (mJspMap[y - 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][indexX + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}

			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][indexX + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}	
	}

	return false;
}

// commit
//=======================================================
// �밢�� �Լ��� ���� ���� ����
//=======================================================
bool CJumpPointSearch::searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	for (int indexX = x; indexX >= 0; --indexX)
	{
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;

		//=============================================
		// �������� ������ ��� return
		//=============================================
		if (indexX == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}
		
		if (indexX > 0)
		{
			if (y > 0)
			{
				if (mJspMap[y - 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][indexX - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}

			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][indexX - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}
	}

	return false;
}


// commit
//=======================================================
// �밢�� �Լ��� ���� �� ����
//=======================================================
bool CJumpPointSearch::searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{	
	for (int indexY = y; indexY >= 0; --indexY)
	{
		// ��ֹ� ������ ��� return
		if (mJspMap[indexY][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;
		
		//=================================================
		// ������ ������ ��� ��� ���� �� return
		//=================================================
		if (x == mDestinationNode.x && indexY == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}


		if (indexY > 0)
		{
			if (x > 0)
			{
				if (mJspMap[indexY][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY - 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}

			if (x + 1 < mMapWidth)
			{
				if (mJspMap[indexY][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY - 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}
	}

	return false;
}

// commit
//=======================================================
// �밢�� �Լ��� ���� �Ʒ� ����
//=======================================================
bool CJumpPointSearch::searchVerticalDownForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	for (int indexY = y; indexY < mMapHeight; ++indexY)
	{
		if (mJspMap[indexY][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;

		if (x == mDestinationNode.x && indexY == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}

		if (indexY + 1 < mMapHeight)
		{
			if (x > 0)
			{
				if (mJspMap[indexY][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY + 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		
			if (x + 1 < mMapWidth)
			{
				if (mJspMap[indexY][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[indexY + 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}		
	}

	return false;
}




void CJumpPointSearch::setRouteArray(stNode* pDestNode, std::vector<stRouteNode>& route)
{
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



void CJumpPointSearch::makeOptimizePath(stNode* pNode)
{
	int startX;
	int startY;

	int endX;
	int endY;

	for (;;)
	{
		stNode* pNextNode = pNode->pParentNode;
		if (pNextNode == nullptr)
			return;

		startX = pNode->x;
		startY = pNode->y;

		for (;;)
		{
			stNode* pNextNextNode = pNextNode->pParentNode;
			if (pNextNextNode == nullptr)
				return;

			endX = pNextNextNode->x;
			endY = pNextNextNode->y;

			if (makeBresenhamLine(startX, startY, endX, endY) == true)
			{
				pNode->pParentNode = pNextNextNode;

				pNextNode = pNextNextNode;

				if (pNextNode == nullptr)
					return;
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




bool CJumpPointSearch::makeBresenhamLine(int startX, int startY, int endX, int endY)
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

			if (mJspMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
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

			if (mJspMap[y][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			{
				return false;
			}
		}
	}

	return true;
}


