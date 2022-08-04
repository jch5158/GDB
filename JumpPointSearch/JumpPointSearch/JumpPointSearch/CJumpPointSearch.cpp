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

	// 목적지 노드 셋팅
	mDestinationNode.x = destX;
	mDestinationNode.y = destY;

	// 오픈 리스트에 추가
	mOpenList.push_back(pStartNode);

	for (;;)
	{
		// 현재 노드
		CJumpPointSearch::stNode* curNode = findOpenNode();
		if (curNode == nullptr)
			break;
		else if (curNode->x == mDestinationNode.x && curNode->y == mDestinationNode.y)
		{
			setRouteArray(curNode, route);

			// 오픈 리스트 정리
			mOpenList.clear();

			// close 리스트 정리
			resetNodeState();

			return true;
		}

		// 길 찾기 로직 시작
		findRoute(curNode);

		// openList F값 정렬
		mOpenList.sort(CAscendingOrder());
	}

	// 오픈 리스트 정리
	mOpenList.clear();

	// close 리스트 정리
	resetNodeState();

	return false;
}


//================================================================
// 오픈 리스트에서 정렬되어 있던 F값 노드를 뽑는다.
//================================================================
CJumpPointSearch::stNode* CJumpPointSearch::findOpenNode()
{
	// F값이 작은 순서로 정렬해놨기 때문에 begin값을 뽑으면 첫 노드를 뽑을 수 있다.
	const auto& iter = mOpenList.begin();

	// 오픈리스트가 없다면은 nullptr return 한다.
	if (iter == mOpenList.end())
		return nullptr;

	// 노드 포인터를 복사한다.
	stNode* node = *iter;

	// 오픈 리스트에서 해당 노드를 지운다.
	mOpenList.erase(iter);

	mJspMap[node->y][node->x].nodeState = eNodeState::NODE_CLOSED;

	return node;
}



void CJumpPointSearch::findRoute(CJumpPointSearch::stNode* node)
{
	// 노드 방향에 맞는 탐색함수를 호출한다.
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


	// 오픈리스트에 없는 노드라면은 생성
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
		// 기존에 오픈리스트에 있었던 노드 부모의 G 값이 parentNode의 G값 보다 크다면
		// parentNode를 부모로서 새로 이어주는 셋팅을 한다.
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
// 클로즈 맵을 리셋한다.
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
// 수직 위 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchVerticalUp(stNode* pParentNode)
{		
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (y > 0)
	{
		//=================================================================================
		// 첫 탐색 시 왼쪽 위 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (x > 0)
		{
			if (mJspMap[y][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftUp(pParentNode, true);  // 왼쪽 위 보조탐색
		}

		//=================================================================================
		// 첫 탐색 시 오른쪽 위 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (x + 1 < mMapWidth)
		{
			if (mJspMap[y][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightUp(pParentNode, true);  // 오른쪽 위 보조탐색
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
// 수직 아래 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchVerticalDown(stNode* pParentNode)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if(y + 1 < mMapHeight)
	{
		//=================================================================================
		// 첫 탐색 시 왼쪽 아래 보조 탐색 영억을 확인합니다.
		//=================================================================================	
		if (x > 0)
		{
			if (mJspMap[y][x - 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftDown(pParentNode, true);  //  왼쪽 아래 보조탐색
		}

		//=================================================================================
		// 첫 탐색 시 오른쪽 아래 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (x + 1 < mMapWidth)
		{
			if (mJspMap[y][x + 1].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightDown(pParentNode, true); // 오른쪽 아래 보조탐색

		}
	}	


	for (int indexY = y + 1; indexY < mMapHeight; ++indexY)
	{
		// 장애물을 마주칠 경우 return 한다.
		if (mJspMap[indexY][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		// 목적지를 만났을 경우 목적지를 이어주고 return 한다.
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
// 오른쪽 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchHorizontalRight(stNode* pParentNode)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	//=================================================================================
	// x 좌표가 x + 1 < mMapWidth 일 경우에만 로직을 수행합니다. 
	//=================================================================================
	if (x + 1 < mMapWidth)
	{
		//=================================================================================
		// 첫 탐색 시 오른쪽 위 보조 탐색 영억을 확인한다.
		//=================================================================================
		if (y > 0)
		{
			if (mJspMap[y - 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightUp(pParentNode, true);   // 오른쪽 위 대각선 보조탐색
		}

		//=================================================================================
		// 첫 탐색 시 오른쪽 아래 보조 탐색 영억을 확인한다.
		//=================================================================================
		if (y + 1 < mMapHeight)
		{
			if (mJspMap[y + 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchRightDown(pParentNode, true);   // 오른쪽 아래 대각선 보조탐색
		}
	}

	//===============================================================================
	// indexX = x + 1 을 해서 다음 위치부터 탐색을 한다. 
	//===============================================================================
	for (int indexX = x + 1; indexX < mMapWidth; ++indexX)
	{
		// 벽을 만났으 경우 아무것도 하지 않고 return 
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		// 목적지를 만날 경우 바로 이어주고 return 한다. 
		if (indexX == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, indexX, y);

			return;
		}

		//=============================================================================
		// indexX + 1 값이 mMapWidth 보다 작을 경우에만 오버플로우 없이 x + 1 위치를 확인할 수 있다.
		//=============================================================================
		if (indexX + 1 < mMapWidth)
		{
			//=============================================================================
			// y 값이 1 이상일 경우에만 위의 벽을 확인할 수 있다.
			//=============================================================================
			if (y > 0)
			{
				if (mJspMap[y - 1][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][indexX + 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				{
					// 해당 위치에 노드 만들기
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, indexX, y);

					return;
				}
			}

			//============================================================================
			// y 값이 y + 1 < mMapHeight 일 경우에만 아래 벽을 확인할 수 있다.	
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
// 왼쪽 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchHorizontalLeft(stNode* pParentNode)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (x > 0)
	{
		//=================================================================================
		// 첫 탐색 시 왼쪽 위 보조 탐색 영역을 확인합니다.
		//=================================================================================
		if (y > 0)
		{
			if (mJspMap[y - 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftUp(pParentNode, true);
		}

		//=================================================================================
		// 첫 탐색 시 왼쪽 아래 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (y + 1 < mMapHeight)
		{
			if (mJspMap[y + 1][x].nodeAttribute == eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1].nodeAttribute == eNodeAttribute::NODE_UNBLOCK)
				searchLeftDown(pParentNode, true);
		}
	}


	for (int indexX = x - 1; indexX >= 0; --indexX)
	{
		// 해당 좌표에 장애물을 만나면 바로 return 한다.
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return;

		// 목적지를 만나면 이어주고 바로 return 한다.
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
// 오른쪽 위 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchRightUp(stNode* pParentNode, bool bAssistanceCallFlag)
{		
	// 보조 탐색이 아닌 경우 아래 함수를 호출한다.
	if (bAssistanceCallFlag == false)
	{	
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
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

		// 오른쪽 탐색
		if (searchHorizontalRightForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RU, x, y) == true)
			return;

		// 위쪽 탐색
		if (searchVerticalUpForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RU, x, y) == true)
			return;
	}

	return;
}

// commit
//======================================================
// 오른쪽 아래 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchRightDown(stNode* pParentNode, bool bAssistanceCallFlag)
{		
	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
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
// 왼쪽 위 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchLeftUp(stNode* pParentNode, bool bAssistanceCallFlag)
{
	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
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
// 왼쪽 아래 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchLeftDown(stNode* pParentNode, bool bAssistanceCallFlag)
{
	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
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
// 대각선 함수의 수평 오른쪽 방향 
//=======================================================
bool CJumpPointSearch::searchHorizontalRightForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	for (int indexX = x; indexX < mMapWidth; ++indexX)
	{
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;
	
		// 목적지 노드를 만났을 경우 노드 생성 후 return
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
// 대각선 함수의 수평 왼쪽 방향
//=======================================================
bool CJumpPointSearch::searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	for (int indexX = x; indexX >= 0; --indexX)
	{
		if (mJspMap[y][indexX].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;

		//=============================================
		// 목적지를 만났을 경우 return
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
// 대각선 함수의 수직 위 방향
//=======================================================
bool CJumpPointSearch::searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{	
	for (int indexY = y; indexY >= 0; --indexY)
	{
		// 장애물 만났을 경우 return
		if (mJspMap[indexY][x].nodeAttribute == eNodeAttribute::NODE_BLOCK)
			return false;
		
		//=================================================
		// 목적지 만났을 경우 노드 생성 후 return
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
// 대각선 함수의 수직 아래 방향
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


