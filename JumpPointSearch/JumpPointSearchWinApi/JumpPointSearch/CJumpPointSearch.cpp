#include "stdafx.h"
#include "JumpPointSearchAlgorithm.h"
#include "BresenhamLine.h"
#include "CJumpPointSearch.h"



//===================================================================
// 타이머로 자동 함수 호출 시 로직 실행이 되지 않도록 하는 Flag
//===================================================================
bool functionFlag = false;


CJumpPointSearch::CJumpPointSearch(void)
	: mJspMap(nullptr)
	, mMapWidth(0)
	, mMapHeight(0)
	, mbFuncSetFlag(false)
	, mStartNode{ 0, }
	, mDestinationNode{ 0, }
	, mOpenList()
	, mCloseList()
	, mRouteList()
	, mOptimizeRouteList()
{}

CJumpPointSearch::CJumpPointSearch(int mapWidth, int mapHeight)
	: mJspMap(nullptr)
	, mMapWidth(mapWidth)
	, mMapHeight(mapHeight)
	, mbFuncSetFlag(true)
	, mStartNode{ 0, }
	, mDestinationNode{ 0, }
	, mOpenList()
	, mCloseList()
	, mRouteList()
	, mOptimizeRouteList()
{
	mJspMap = new char* [mMapHeight];

	for (int indexY = 0; indexY < mMapHeight; ++indexY)
	{
		mJspMap[indexY] = new char[mMapWidth];
	}

	for (int indexY = 0; indexY < mMapHeight; ++indexY)
	{
		for (int indexX = 0; indexX < mMapWidth; ++indexX)
		{
			mJspMap[indexY][indexX] = (char)eNodeAttribute::NODE_UNBLOCK;
		}
	}
}


CJumpPointSearch::~CJumpPointSearch()
{
	for (int iCntY = 0; iCntY < mMapHeight; ++iCntY)
	{
		free(mJspMap[iCntY]);
	}

	free(mJspMap);
}



void CJumpPointSearch::SettingMapAttribute(int posX, int posY)
{

	mJspMap[posY][posX] = (char)eNodeAttribute::NODE_BLOCK;

}


void CJumpPointSearch::setNodeGHF(stNode* pParentNode, int x, int y, float* pG, float* pH, float* pF)
{		
	if (pParentNode == nullptr)
	{
		*pG = 0.0f;
	}
	else
	{
		float disX = abs(pParentNode->x - x);
		float disY = abs(pParentNode->y - y);

		*pG = pParentNode->G + sqrt(disX * disX + disY * disY);
	}
	
	float width = abs(mDestinationNode.x - x);
	float high = abs(mDestinationNode.y - y);

	*pH = sqrt(width * width + high * high);

	*pF = *pG + *pH;

	return;
}




bool CJumpPointSearch::PathFind(int startX, int startY, int destinationX, int destinationY, RouteNode routeNodeArray[], int routeNodeArraySize, RouteNode optimizeNodeArray[], int optimizeNodeArraySize)
{	
	// 현재 노드
	CJumpPointSearch::stNode* curNode;	 

	// 엔터를 입력하기 전까지 해당 함수를 호출 시 바로 리턴
	if (functionFlag == false)
	{
		return false;
	}

	if (mbFuncSetFlag == true)
	{
		ZeroMemory(&mStartNode, sizeof(stNode));
		ZeroMemory(&mDestinationNode, sizeof(stNode));


		// 목적지 노드의 좌표 설정
		mDestinationNode.x = destinationX;
		mDestinationNode.y = destinationY;


		// 시작노드 좌표설정
		mStartNode.x = startX;
		mStartNode.y = startY;
		mStartNode.nodeDir = (BYTE)eNodeDirection::NODE_DIR_ALL;
		mStartNode.pParentNode = nullptr;

		// 시작노드 셋팅
		setNodeGHF(nullptr, startX, startY, &mStartNode.G, &mStartNode.H, &mStartNode.F);

		// 오픈 리스트에 추가
		mOpenList.push_back(&mStartNode);

		// 한 번만 시작 셋팅되도록 설정
		mbFuncSetFlag = false;
	}



	curNode = selectOpenListNode();
	if (curNode->x == mDestinationNode.x && curNode->y == mDestinationNode.y)
	{
		setRouteArray(curNode, routeNodeArray, routeNodeArraySize);
		
		pathOptimize(curNode);

		setOptimizeArray(curNode, optimizeNodeArray, optimizeNodeArraySize);

		functionFlag = false;
		
		return true;
	}
	
	// 길 찾기 로직 시작
	findRouteNode(curNode);

	// openList F값 정렬
	//openListBubbleSort();

	mOpenList.sort(CSortAscendingOrder());

	return false;
}

//======================================================
// 좌표로 openList를 찾아서 return 
//======================================================
CJumpPointSearch::stNode* CJumpPointSearch::findOpenList(int openX, int openY, eNodeDirection nodeDir)
{
	//auto iterE = mOpenList.end();

	//for (auto iter = mOpenList.begin(); iter != iterE; ++iter)
	//{
	//	if ((*iter)->x == openX && (*iter)->y == openY && (*iter)->nodeDir == (BYTE)nodeDir)
	//	{
	//		return *iter;
	//	}
	//}

	for (const auto pNode : mOpenList)
	{
		if (pNode->x == openX && pNode->y == openY && pNode->nodeDir == (BYTE)nodeDir)
			return pNode;
	}

	return nullptr;
}

//======================================================
// 해당 좌표에 closeList가 있다면은 true를 리턴한다.
//======================================================
CJumpPointSearch::stNode* CJumpPointSearch::findCloseList(int closeX, int closeY)
{
	//auto iterE = mCloseList.end();

	//for (auto iter = mCloseList.begin(); iter != iterE; ++iter)
	//{
	//	if ((*iter)->x == closeX && (*iter)->y == closeY )
	//	{
	//		return *iter;
	//	}
	//}

	for (const auto pNode : mCloseList)
	{
		if (pNode->x == closeX && pNode->y == closeY)
			return pNode;
	}

	return nullptr;
}


void CJumpPointSearch::findRouteNode(CJumpPointSearch::stNode* pNode)
{
	if (pNode == nullptr)
	{
		return;
	}

	HBRUSH randBrush;

	for (;;)
	{
		randBrush = CreateSolidBrush(RGB(rand() % 130 + 126, rand() % 130 + 126, rand() % 130 + 126));

		if (randBrush != grayBrush && randBrush != routeBrush)
		{
			break;
		}
	}

	// 노드 방향에 맞는 탐색함수를 호출한다.
	switch ((eNodeDirection)pNode->nodeDir)
	{
	case eNodeDirection::NODE_DIR_RR:

		searchHorizontalRight(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_RD:

		searchRightDown(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_DD:

		searchVerticalDown(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_LD:

		searchLeftDown(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_LL:

		searchHorizontalLeft(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_LU:

		searchLeftUp(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_UU:

		searchVerticalUp(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_RU:

		searchRightUp(pNode, randBrush);

		break;
	case eNodeDirection::NODE_DIR_ALL:

		searchRightUp(pNode, randBrush, true);

		searchHorizontalRight(pNode, randBrush);

		searchRightDown(pNode, randBrush, true);

		searchVerticalDown(pNode, randBrush);

		searchLeftDown(pNode, randBrush, true);

		searchHorizontalLeft(pNode, randBrush);

		searchLeftUp(pNode, randBrush, true);

		searchVerticalUp(pNode, randBrush);

		break;

	default:
		return;
	}

	return;
}




void CJumpPointSearch::setNewNode(stNode* pParentNode, eNodeDirection nodeDir, int x, int y)
{
	stNode* pNewNode;

	stNode* pOpenListNode;	
	
	stNode* pCloseListNode = findCloseList(x, y);
	if (pCloseListNode == nullptr)
	{		
		float tempG;
		float tempH;
		float tempF;

		setNodeGHF(pParentNode, x, y, &tempG, &tempH, &tempF);

		// 오픈리스트에 없는 노드라면은 생성
		pOpenListNode = findOpenList(x, y, nodeDir);
		if (pOpenListNode == nullptr)
		{
			pNewNode = (stNode*)malloc(sizeof(stNode));

			pNewNode->x = x;

			pNewNode->y = y;			

			pNewNode->G = tempG;

			pNewNode->H = tempH;

			pNewNode->F = tempF;

			pNewNode->nodeDir = (BYTE)nodeDir;

			pNewNode->pParentNode = pParentNode;

			// 블럭의 색상을 바꿔준다.
			if (x != mDestinationNode.x || y != mDestinationNode.y)
			{
				brushBlockList[pNewNode->y][pNewNode->x] = blueBrush;
			}
		
			mOpenList.push_back(pNewNode);
		}
		else
		{			
			// 기존에 오픈리스트에 있었던 노드 부모의 G 값이 parentNode의 G값 보다 크다면
			// parentNode를 부모로서 새로 이어주는 셋팅을 한다.
			if (pParentNode->G < pOpenListNode->pParentNode->G)
			{				
				pOpenListNode->G = tempG;
				
				pOpenListNode->F = tempF;

				pOpenListNode->pParentNode = pParentNode;
			}
		}
	}
	
	return;
}


//===============================================
// 오른쪽 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchHorizontalRight(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y - 1][x] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchRightUp(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// 첫 탐색 시 오른쪽 아래 보조 탐색 영억을 확인한다.
		//=================================================================================
		if (y + 1 < mMapHeight)
		{
			if (mJspMap[y + 1][x] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchRightDown(pParentNode, randBrush, true);
			}
		}
	}	
	
	//===============================================================================
	// xIndex = x + 1 을 해서 다음 위치부터 탐색을 한다. 
	//===============================================================================
	for (int xIndex = x + 1; xIndex < mMapWidth; ++xIndex)
	{
		// 벽을 만났으 경우 아무것도 하지 않고 return 
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}

		// 목적지를 만날 경우 바로 이어주고 return 한다. 
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, xIndex, y);

			return;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[y][xIndex] != blueBrush && brushBlockList[y][xIndex] != yellowBrush)
		{
			brushBlockList[y][xIndex] = randBrush;
		}

		//=============================================================================
		// x 값이 mMapWidth 보다 작을 경우에만 오버플로우 없이 x+1 위치를 확인할 수 있다.
		//=============================================================================
		if (xIndex + 1 < mMapWidth)
		{		
			//=============================================================================
			// y 값이 1 이상일 경우에만 위의 벽을 확인할 수 있다.
			//=============================================================================
			if (y > 0)
			{
				if (mJspMap[y - 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][xIndex + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					// 해당 위치에 노드 만들기
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, xIndex, y);

					return;
				}
			}

			//============================================================================
			// y 값이 y + 1 < mMapHeight 일 경우에만 위의 벽을 확인할 수 있다.	
			//============================================================================
			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][xIndex + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, xIndex, y);

					return;
				}
			}
		}
	}

	return;
}


//===============================================
// 왼쪽 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchHorizontalLeft(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y - 1][x] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftUp(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// 첫 탐색 시 왼쪽 아래 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (y + 1 < mMapHeight)
		{
			if (mJspMap[y + 1][x] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftDown(pParentNode, randBrush, true);
			}
		}
	}	


	for (int xIndex = x - 1; xIndex >= 0; --xIndex)
	{
		// 해당 좌표에 장애물을 만나면 바로 return 한다.
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}


		// 목적지를 만나면 이어주고 바로 return 한다.
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, xIndex, y);

			return;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[y][xIndex] != blueBrush && brushBlockList[y][xIndex] != yellowBrush)
		{
			brushBlockList[y][xIndex] = randBrush;
		}


		if (xIndex > 0)
		{
			if (y > 0)
			{
				if (mJspMap[y - 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][xIndex - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, xIndex, y);

					return;
				}
			}

			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][xIndex - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, xIndex, y);

					return;
				}
			}
		}	
	}

	return;
}


//===============================================
// 수직 위 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchVerticalUp(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftUp(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// 첫 탐색 시 오른쪽 위 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (x + 1 < mMapWidth)
		{
			if (mJspMap[y][x + 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchRightUp(pParentNode, randBrush, true);
			}
		}
	}
	

	for (int yIndex = y - 1; yIndex >= 0; --yIndex)
	{
		//=============================================
		// 해당 위치에 장애물이 있을 경우 바로 return
		//=============================================
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}


		//===================================================
		// 해당 위치에 목적지가 있을 경우 노드 생성 후 return
		//===================================================
		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, yIndex);

			return;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[yIndex][x] != blueBrush && brushBlockList[yIndex][x] != yellowBrush)
		{
			brushBlockList[yIndex][x] = randBrush;
		}


		if (yIndex > 0)
		{
			if (x > 0)
			{
				if (mJspMap[yIndex][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex - 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, yIndex);

					return;
				}
			}


			if (x + 1 < mMapWidth)
			{
				if (mJspMap[yIndex][x + 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex - 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, yIndex);

					return;
				}
			}
		}
	}

	return;
}


//===============================================
// 수직 아래 직선 탐색 함수입니다.
//===============================================
void CJumpPointSearch::searchVerticalDown(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftDown(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// 첫 탐색 시 오른쪽 아래 보조 탐색 영억을 확인합니다.
		//=================================================================================
		if (x + 1 < mMapWidth)
		{
			if (mJspMap[y][x + 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchRightDown(pParentNode, randBrush, true);

			}
		}
	}	


	for (int yIndex = y + 1; yIndex < mMapHeight; ++yIndex)
	{
		// 장애물을 마주칠 경우 return 한다.
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}


		// 목적지를 만났을 경우 목적지를 이어주고 return 한다.
		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, yIndex);

			return;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[yIndex][x] != blueBrush && brushBlockList[yIndex][x] != yellowBrush)
		{
			brushBlockList[yIndex][x] = randBrush;
		}

		if (x > 0 && yIndex + 1 < mMapHeight )
		{
			if (mJspMap[yIndex][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex + 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, yIndex);

				return;
			}
		}

		if (x + 1 < mMapWidth && yIndex + 1 < mMapHeight)
		{
			if (mJspMap[yIndex][x + 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex + 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, yIndex);

				return;
			}
		}		
	}

	return;
}



//======================================================
// 오른쪽 위 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchRightUp(stNode* pParentNode, HBRUSH randBrush ,bool bAssistanceCallFlag)
{		
	int x = pParentNode->x;
	int y = pParentNode->y;

	//===============================================
	// 대각선 함수 호출 시 직선 함수 호출 여부
	//===============================================
	if (bAssistanceCallFlag == false)
	{	
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
		searchHorizontalRight(pParentNode, randBrush);

		searchVerticalUp(pParentNode, randBrush);
		//===================================================================
	}

	for(;;)
	{
		// 배열 범위를 벗어날 경우 호출 안함 
		if (x == mMapWidth - 1 || y == 0)
		{
			break;
		}

		x += 1;

		y -= 1;

		if (mJspMap[y][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			break;
		}

		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RU, x, y);

			return;
		}

		if (searchHorizontalRightForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RU, x, y, randBrush) == true)
		{
			return;
		}

		if (searchVerticalUpForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RU, x, y, randBrush) == true)
		{
			return;
		}
	}

	return;
}


//======================================================
// 오른쪽 아래 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchRightDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag)
{	
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
		searchHorizontalRight(pParentNode, randBrush);
		
		searchVerticalDown(pParentNode, randBrush);
		//===================================================================
	}

	for(;;)
	{
		if (x == mMapWidth - 1 || y == mMapHeight - 1)
		{
			break;
		}

		x += 1;

		y += 1;

		if (mJspMap[y][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			break;
		}

		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{			
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RD, x, y);

			return;
		}

		if (searchHorizontalRightForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RD, x, y, randBrush) == true)
		{
			return;
		}

		if (searchVerticalDownForDiagonal(pParentNode, eNodeDirection::NODE_DIR_RD, x, y, randBrush) == true)
		{
			return;
		}
	}

	return;
}


//======================================================
// 왼쪽 위 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchLeftUp(stNode* pParentNode, HBRUSH randBrush ,bool bAssistanceCallFlag)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
		searchHorizontalLeft(pParentNode, randBrush);

		searchVerticalUp(pParentNode, randBrush);
		//===================================================================
	}

	for(;;)
	{
		if (x == 0 || y == 0)
		{
			break;
		}

		x -= 1;
		y -= 1;

		if (mJspMap[y][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			break;
		}

		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{		
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LU, x, y);

			return;
		}

		if (searchHorizontalLeftForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LU, x, y, randBrush) == true)
		{
			return;
		}

		if (searchVerticalUpForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LU, x, y, randBrush) == true)
		{
			return;
		}
	}

	return;
}


//======================================================
// 왼쪽 아래 대각선 함수입니다. 
//======================================================
void CJumpPointSearch::searchLeftDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// 직선 조건에 맞으면 5방향으로 탐색하게 된다.
		searchHorizontalLeft(pParentNode, randBrush);
		
		searchVerticalDown(pParentNode, randBrush);
		//===================================================================
	}

	for(;;)
	{
		if (x == 0 || y == mMapHeight - 1)
		{
			break;
		}

		x -= 1;
		y += 1;

		if (mJspMap[y][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			break;
		}

		if (mDestinationNode.x == x && mDestinationNode.y == y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LD, x, y);

			return;
		}

		if (searchHorizontalLeftForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LD, x, y, randBrush) == true)
		{
			return;
		}

		if (searchVerticalDownForDiagonal(pParentNode, eNodeDirection::NODE_DIR_LD, x, y, randBrush) == true)
		{
			return;
		}
	}

	return;
}



//=======================================================
// 대각선 함수의 수평 오른쪽 방향 
//=======================================================
bool CJumpPointSearch::searchHorizontalRightForDiagonal(stNode* parentNode, eNodeDirection nodeDir,int x, int y, HBRUSH randBrush)
{

	for (int xIndex = x; xIndex < mMapWidth; ++xIndex)
	{
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return false;
		}
	
		// 목적지 노드를 만났을 경우에는 그 자리에 노드 생성후 return 
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(parentNode, nodeDir, x, y);
			
			return true;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[y][xIndex] != blueBrush && brushBlockList[y][xIndex] != yellowBrush)
		{
			brushBlockList[y][xIndex] = randBrush;
		}

		if (xIndex + 1 < mMapWidth)
		{
			if (y > 0)
			{
				if (mJspMap[y - 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][xIndex + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(parentNode, nodeDir, x, y);

					return true;
				}
			}

			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][xIndex + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(parentNode, nodeDir, x, y);

					return true;
				}
			}
		}	
	}

	return false;
}


//=======================================================
// 대각선 함수의 수평 왼쪽 방향
//=======================================================
bool CJumpPointSearch::searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush)
{

	for (int xIndex = x; xIndex >= 0; --xIndex)
	{
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return false;
		}		

		//=============================================
		// 목적지를 만났을 경우 return
		//=============================================
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[y][xIndex] != blueBrush && brushBlockList[y][xIndex] != yellowBrush)
		{
			brushBlockList[y][xIndex] = randBrush;
		}

		
		if (xIndex > 0)
		{
			if (y > 0)
			{
				if (mJspMap[y - 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][xIndex - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}

			if (y + 1 < mMapHeight)
			{
				if (mJspMap[y + 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][xIndex - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}
	}

	return false;
}


//=======================================================
// 대각선 함수의 수직 위 방향
//=======================================================
bool CJumpPointSearch::searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush)
{
	
	for (int yIndex = y; yIndex >= 0; --yIndex)
	{
		// 장애물 만났을 경우 return
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return false;
		}
		
		//=================================================
		// 목적지 만났을 경우 노드 생성 후 return
		//=================================================
		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}


		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[yIndex][x] != blueBrush && brushBlockList[yIndex][x] != yellowBrush)
		{
			brushBlockList[yIndex][x] = randBrush;
		}

		if (yIndex > 0)
		{
			if (x > 0)
			{
				if (mJspMap[yIndex][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex - 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}

			if (x + 1 < mMapWidth)
			{
				if (mJspMap[yIndex][x + 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex - 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}
	}

	return false;
}


//=======================================================
// 대각선 함수의 수직 아래 방향
//=======================================================
bool CJumpPointSearch::searchVerticalDownForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush)
{

	for (int yIndex = y; yIndex < mMapHeight; yIndex++)
	{
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return false;
		}	

		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}

		// 오픈 리스트 노드 또는 클로즈 리스트 노드에는 칠하지 않는다.
		if (brushBlockList[yIndex][x] != blueBrush && brushBlockList[yIndex][x] != yellowBrush)
		{
			brushBlockList[yIndex][x] = randBrush;
		}

		if (yIndex + 1 < mMapHeight)
		{
			if (x > 0)
			{
				if (mJspMap[yIndex][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex + 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		
			if (x + 1 < mMapWidth)
			{
				if (mJspMap[yIndex][x + 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[yIndex + 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					setNewNode(pParentNode, nodeDir, x, y);

					return true;
				}
			}
		}		
	}

	return false;
}




//================================================================
// 오픈 리스트에서 정렬되어 있던 F값 노드를 뽑는다.
//================================================================
CJumpPointSearch::stNode* CJumpPointSearch::selectOpenListNode()
{
	// F값이 작은 순서로 정렬해놨기 때문에 begin값을 뽑으면 첫 노드를 뽑을 수 있다.
	auto iter = mOpenList.begin();

	if (*iter == nullptr)
	{
		return nullptr;
	}

	// 노드 포인터를 복사한다.
	stNode* node = *iter;

	// 오픈 리스트에서 해당 노드를 지운다.
	mOpenList.erase(iter);

	// closeList에 넣어준다.
	mCloseList.push_back(node);

	// 클로즈리스트이니 색깔을 바꾸어준다.
	if ((mStartNode.x != node->x || mStartNode.y != node->y) && (mDestinationNode.x != node->x || mDestinationNode.y != node->y) )
	{
		brushBlockList[node->y][node->x] = yellowBrush;
	}

	return node;
}


//=============================================================
// 모든 리스트와 블럭을 초기화 시킨다.
//=============================================================
void CJumpPointSearch::ResetAll(RouteNode* routeNodeArray, int routeNodeArraySize, RouteNode* optimizeNodeArray, int optimizeNodeArraySize)
 {	
	functionFlag = false;

	mbFuncSetFlag = true;

	// 오픈 리스트 정리
	resetOpenList();

	resetCloseList();

	resetRouteList();

	resetRouteNodeArray(routeNodeArray, routeNodeArraySize);

	resetOptimizeArray(optimizeNodeArray, optimizeNodeArraySize);

	resetOptimizeRoute();

	// 모든 노드 속성 UNBLOCK
	resetJspMap();

	// 모든 노드의 색상을 기본 색상으로 변경
	resetBlock();
}



//==================================================================
// 시작점과 도착점을 제외한 리스트와 블럭을 초기화하고 다시 길을 찾는다.
//==================================================================
void CJumpPointSearch::ReStart(RouteNode* routeNodeArray, int routeNodeArraySize, RouteNode* optimizeNodeArray, int optimizeNodeArraySize)
{
	functionFlag = true;

	mbFuncSetFlag = true;

	resetOpenList();

	resetCloseList();

	resetRouteList();

	resetRouteNodeArray(routeNodeArray, routeNodeArraySize);

	resetOptimizeArray(optimizeNodeArray, optimizeNodeArraySize);

	resetOptimizeRoute();

	resetJspMap();

	resetRoute();
}


//=============================================================
// 오픈리스트 노드를 리셋한다.
//=============================================================
void CJumpPointSearch::resetOpenList()
{
	auto iterE = mOpenList.end();

	for (auto iter = mOpenList.begin(); iter != iterE;)
	{
		free(*iter);

		iter = mOpenList.erase(iter);
	}
}



//=============================================================
// 클로즈리스트 노드를 리셋한다.
//=============================================================
void CJumpPointSearch::resetCloseList()
{
	auto iterE = mCloseList.end();

	for (auto iter = mCloseList.begin(); iter != iterE;)
	{
		if (&mStartNode == *iter)
		{
			++iter;

			continue;
		}

		free(*iter);

		iter = mCloseList.erase(iter);
	}
}



//=============================================================
// 시작점에서 목적지까지의 List를 reset한다.
//=============================================================
void CJumpPointSearch::resetRouteList()
{
	auto iterE = mRouteList.end();

	for (auto iter = mRouteList.begin(); iter != iterE;)
	{
		iter = mRouteList.erase(iter);
	}
}


//=============================================================
// 자연스러운 시작점에서 목적지까지의 경로를 reset한다.
//=============================================================
void CJumpPointSearch::resetOptimizeRoute()
{
	auto iterE = mOptimizeRouteList.end();

	for (auto iter = mOptimizeRouteList.begin(); iter != iterE;)
	{
		iter = mOptimizeRouteList.erase(iter);
	}
}



//=============================================================
// 블럭들을 리셋한다.
//=============================================================
void CJumpPointSearch::resetBlock()
{
	for (int iCntY = 0; iCntY < mMapHeight; iCntY++)
	{
		for (int iCntX = 0; iCntX < mMapWidth; iCntX++)
		{
			brushBlockList[iCntY][iCntX] = oldBrush;
		}
	}
}



//=============================================================
// 블럭들의 장애불 상태를 리셋한다.
//=============================================================
void CJumpPointSearch::resetJspMap()
{
	for (int iCntY = 0; iCntY < mMapHeight; ++iCntY)
	{
		for (int iCntX = 0; iCntX < mMapWidth; ++iCntX)
		{
			mJspMap[iCntY][iCntX] = (char)eNodeAttribute::NODE_UNBLOCK;
		}
	}
}


//=====================================================================
// 시작점과 목적지 장애물을 제외하고 블럭 색깔을 리셋한다.
//======================================================================
void CJumpPointSearch::resetRoute()
{
	for (int iCntY = 0; iCntY < mMapHeight; iCntY++)
	{
		for (int iCntX = 0; iCntX < mMapWidth; iCntX++)
		{
			if (brushBlockList[iCntY][iCntX] != redBrush && brushBlockList[iCntY][iCntX] != greenBrush && brushBlockList[iCntY][iCntX] != grayBrush)
			{
				brushBlockList[iCntY][iCntX] = oldBrush;
			}
		}
	}
}

//==================================================================
// 정방향으로 목적지까지의 경로를 넣는다.
//==================================================================
//void CJumpPointSearch::insertRoute(stNode* node)
//{
//	for(;;)
//	{	
//		mRouteList.push_front(node);
//
//		mOptimizeRouteList.push_front(node);
//
//		if (node->pParentNode == nullptr)
//		{
//			break;
//		}
//
//		node = node->pParentNode;
//	}
//}

//====================================================
// 최적화 길을 설정하는 함수입니다.
//====================================================
//void CJumpPointSearch::pathOptimize(stNode* pDestNode)
//{
//	int startX;
//	int startY;
//
//	int endX;
//	int endY;
//	
//	auto iter = mOptimizeRouteList.begin();
//
//	auto iterE = mOptimizeRouteList.end();
//	
//	for(;;)
//	{		
//		auto nextIter = std::next(iter);
//		
//		//========================================================================
//		// iter의 다음 노드가 마지막 노드라면은 로직을 종료한다.
//		//=========================================================================
//		if (nextIter == iterE)
//		{
//			break;
//		}
//	
//		startX = (*iter)->x;
//		startY = (*iter)->y;
//
//		for(;;)
//		{		
//			auto nextNextIter = std::next(nextIter);
//
//			// 마지막 노드라면은 return 한다.
//			if (nextNextIter == iterE)
//			{
//				return;
//			}
//
//			endX = (*nextNextIter)->x;
//			endY = (*nextNextIter)->y;
//
//			//======================================================================
//			// 라인이 이어진다면은 그전 노드는 삭제할 노드로 체크한다.
//			// nextIter를 다음 노드로 이동한다.
//			//======================================================================
//			if (BresenhamLine::MakeLine(startX, startY, endX, endY) == true)
//			{
//				nextIter = mOptimizeRouteList.erase(nextIter);
//			}
//			else
//			{
//				//=====================================================================================
//				// 장애물에 의해서 이어질 수 없는 노드라면은 nextIter 노드를 기준으로 바꾼다.
//				//=====================================================================================				
//				iter = nextIter;
//
//				break;
//			}
//		}
//	}
//
//	return;
//}


void CJumpPointSearch::pathOptimize(stNode* pNode)
{
	int startX;
	int startY;

	int endX;
	int endY;

	for (;;)
	{
		stNode* pNextNode = pNode->pParentNode;

		if (pNextNode == nullptr)
		{
			return;
		}

		startX = pNode->x;
		startY = pNode->y;

		for (;;)
		{
			stNode* pNextNextNode = pNextNode->pParentNode;

			if (pNextNextNode == nullptr)
			{
				return;
			}

			endX = pNextNextNode->x;
			endY = pNextNextNode->y;

			if (BresenhamLine::MakeLine(startX, startY, endX, endY) == true)
			{
				pNode->pParentNode = pNextNextNode;

				pNextNode = pNextNextNode;

				if (pNextNode == nullptr)
				{
					return;
				}
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




void CJumpPointSearch::setRouteArray(stNode* pDestNode, RouteNode routeNodeArray[], int routeArraySize)
{	
	for(;;)
	{	
		mRouteList.push_front(pDestNode);
		
		if (pDestNode->pParentNode == nullptr)
		{
			break;
		}

		pDestNode = pDestNode->pParentNode;
	}

	int index = 0;

	auto iterE = mRouteList.end();

	for (auto iter = mRouteList.begin(); iter != iterE; ++iter)
	{
		routeNodeArray[index].mPosX = (*iter)->x;

		routeNodeArray[index].mPosY = (*iter)->y;

		routeNodeArray[index].mRouteFlag = true;

		index += 1;
		
		if (index >= routeArraySize)
		{
			return;
		}
	}

	return;
}

void CJumpPointSearch::resetRouteNodeArray(RouteNode routeNodeArray[], int routeNodeArraySize)
{
	for (int iCnt = 0; iCnt < routeNodeArraySize; ++iCnt)
	{
		routeNodeArray[iCnt].mRouteFlag = false;
	}
}


void CJumpPointSearch::setOptimizeArray(stNode* pDestNode, RouteNode optimizeNodeArray[], int optimizeNodeArraySize)
{
	for (;;)
	{
		mOptimizeRouteList.push_front(pDestNode);

		if (pDestNode->pParentNode == nullptr)
		{
			break;
		}

		pDestNode = pDestNode->pParentNode;
	}

	int index = 0;

	auto iterE = mOptimizeRouteList.end();

	for (auto iter = mOptimizeRouteList.begin(); iter != iterE; ++iter)
	{
		optimizeNodeArray[index].mPosX = (*iter)->x;

		optimizeNodeArray[index].mPosY = (*iter)->y;

		optimizeNodeArray[index].mRouteFlag = true;

		index += 1;

		if (index >= optimizeNodeArraySize)
		{
			return;
		}
	}

	return;
}


void CJumpPointSearch::resetOptimizeArray(RouteNode optimizeNodeArray[], int optimizeNodeArraySize)
{
	for (int iCnt = 0; iCnt < optimizeNodeArraySize; ++iCnt)
	{
		optimizeNodeArray[iCnt].mRouteFlag = false;
	}
}


//=============================================================
// F값 작은 순으로 정렬한다.
//=============================================================
//void CJumpPointSearch::openListBubbleSort() 
//{
//
//	CList<CJumpPointSearch::stNode*>::Iterator iterE = mOpenList.end();
//
//	--iterE;
//
//	int passCount = mOpenList.GetUseSize() - 1;
//
//	for (int iCnt = 0; iCnt < passCount - 1; iCnt++)
//	{
//		for (CList<CJumpPointSearch::stNode*>::Iterator iter = mOpenList.begin(); iter != iterE; ++iter)
//		{
//			CList<CJumpPointSearch::stNode*>::Iterator iterN = iter.GetNextIter();
//
//			if (iter->F > iterN->F)
//			{
//				mOpenList.DataSwap(iter, iterN);
//			}
//		}
//
//		--iterE;
//	}
//}


CJumpPointSearch::CSortAscendingOrder::CSortAscendingOrder(void) {}
CJumpPointSearch::CSortAscendingOrder::~CSortAscendingOrder(void) {}

bool CJumpPointSearch::CSortAscendingOrder::operator()(const stNode* pLeft, const stNode* pRight) const
{
	return pLeft->F < pRight->F;
}
