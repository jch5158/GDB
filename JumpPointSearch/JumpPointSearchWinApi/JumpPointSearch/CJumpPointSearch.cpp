#include "stdafx.h"
#include "JumpPointSearchAlgorithm.h"
#include "BresenhamLine.h"
#include "CJumpPointSearch.h"



//===================================================================
// Ÿ�̸ӷ� �ڵ� �Լ� ȣ�� �� ���� ������ ���� �ʵ��� �ϴ� Flag
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
	// ���� ���
	CJumpPointSearch::stNode* curNode;	 

	// ���͸� �Է��ϱ� ������ �ش� �Լ��� ȣ�� �� �ٷ� ����
	if (functionFlag == false)
	{
		return false;
	}

	if (mbFuncSetFlag == true)
	{
		ZeroMemory(&mStartNode, sizeof(stNode));
		ZeroMemory(&mDestinationNode, sizeof(stNode));


		// ������ ����� ��ǥ ����
		mDestinationNode.x = destinationX;
		mDestinationNode.y = destinationY;


		// ���۳�� ��ǥ����
		mStartNode.x = startX;
		mStartNode.y = startY;
		mStartNode.nodeDir = (BYTE)eNodeDirection::NODE_DIR_ALL;
		mStartNode.pParentNode = nullptr;

		// ���۳�� ����
		setNodeGHF(nullptr, startX, startY, &mStartNode.G, &mStartNode.H, &mStartNode.F);

		// ���� ����Ʈ�� �߰�
		mOpenList.push_back(&mStartNode);

		// �� ���� ���� ���õǵ��� ����
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
	
	// �� ã�� ���� ����
	findRouteNode(curNode);

	// openList F�� ����
	//openListBubbleSort();

	mOpenList.sort(CSortAscendingOrder());

	return false;
}

//======================================================
// ��ǥ�� openList�� ã�Ƽ� return 
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
// �ش� ��ǥ�� closeList�� �ִٸ��� true�� �����Ѵ�.
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

	// ��� ���⿡ �´� Ž���Լ��� ȣ���Ѵ�.
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

		// ���¸���Ʈ�� ���� ������� ����
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

			// ���� ������ �ٲ��ش�.
			if (x != mDestinationNode.x || y != mDestinationNode.y)
			{
				brushBlockList[pNewNode->y][pNewNode->x] = blueBrush;
			}
		
			mOpenList.push_back(pNewNode);
		}
		else
		{			
			// ������ ���¸���Ʈ�� �־��� ��� �θ��� G ���� parentNode�� G�� ���� ũ�ٸ�
			// parentNode�� �θ�μ� ���� �̾��ִ� ������ �Ѵ�.
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
// ������ ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchHorizontalRight(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y - 1][x] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchRightUp(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// ù Ž�� �� ������ �Ʒ� ���� Ž�� ������ Ȯ���Ѵ�.
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
	// xIndex = x + 1 �� �ؼ� ���� ��ġ���� Ž���� �Ѵ�. 
	//===============================================================================
	for (int xIndex = x + 1; xIndex < mMapWidth; ++xIndex)
	{
		// ���� ������ ��� �ƹ��͵� ���� �ʰ� return 
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}

		// �������� ���� ��� �ٷ� �̾��ְ� return �Ѵ�. 
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, xIndex, y);

			return;
		}

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
		if (brushBlockList[y][xIndex] != blueBrush && brushBlockList[y][xIndex] != yellowBrush)
		{
			brushBlockList[y][xIndex] = randBrush;
		}

		//=============================================================================
		// x ���� mMapWidth ���� ���� ��쿡�� �����÷ο� ���� x+1 ��ġ�� Ȯ���� �� �ִ�.
		//=============================================================================
		if (xIndex + 1 < mMapWidth)
		{		
			//=============================================================================
			// y ���� 1 �̻��� ��쿡�� ���� ���� Ȯ���� �� �ִ�.
			//=============================================================================
			if (y > 0)
			{
				if (mJspMap[y - 1][xIndex] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][xIndex + 1] == (char)eNodeAttribute::NODE_UNBLOCK)
				{
					// �ش� ��ġ�� ��� �����
					setNewNode(pParentNode, eNodeDirection::NODE_DIR_RR, xIndex, y);

					return;
				}
			}

			//============================================================================
			// y ���� y + 1 < mMapHeight �� ��쿡�� ���� ���� Ȯ���� �� �ִ�.	
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
// ���� ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchHorizontalLeft(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y - 1][x] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftUp(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// ù Ž�� �� ���� �Ʒ� ���� Ž�� ������ Ȯ���մϴ�.
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
		// �ش� ��ǥ�� ��ֹ��� ������ �ٷ� return �Ѵ�.
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}


		// �������� ������ �̾��ְ� �ٷ� return �Ѵ�.
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_LL, xIndex, y);

			return;
		}

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// ���� �� ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchVerticalUp(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y - 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftUp(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// ù Ž�� �� ������ �� ���� Ž�� ������ Ȯ���մϴ�.
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
		// �ش� ��ġ�� ��ֹ��� ���� ��� �ٷ� return
		//=============================================
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}


		//===================================================
		// �ش� ��ġ�� �������� ���� ��� ��� ���� �� return
		//===================================================
		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_UU, x, yIndex);

			return;
		}

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// ���� �Ʒ� ���� Ž�� �Լ��Դϴ�.
//===============================================
void CJumpPointSearch::searchVerticalDown(stNode* pParentNode, HBRUSH randBrush)
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
			if (mJspMap[y][x - 1] == (char)eNodeAttribute::NODE_BLOCK && mJspMap[y + 1][x - 1] == (char)eNodeAttribute::NODE_UNBLOCK)
			{
				searchLeftDown(pParentNode, randBrush, true);
			}
		}

		//=================================================================================
		// ù Ž�� �� ������ �Ʒ� ���� Ž�� ������ Ȯ���մϴ�.
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
		// ��ֹ��� ����ĥ ��� return �Ѵ�.
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return;
		}


		// �������� ������ ��� �������� �̾��ְ� return �Ѵ�.
		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, eNodeDirection::NODE_DIR_DD, x, yIndex);

			return;
		}

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// ������ �� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchRightUp(stNode* pParentNode, HBRUSH randBrush ,bool bAssistanceCallFlag)
{		
	int x = pParentNode->x;
	int y = pParentNode->y;

	//===============================================
	// �밢�� �Լ� ȣ�� �� ���� �Լ� ȣ�� ����
	//===============================================
	if (bAssistanceCallFlag == false)
	{	
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
		searchHorizontalRight(pParentNode, randBrush);

		searchVerticalUp(pParentNode, randBrush);
		//===================================================================
	}

	for(;;)
	{
		// �迭 ������ ��� ��� ȣ�� ���� 
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
// ������ �Ʒ� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchRightDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag)
{	
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
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
// ���� �� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchLeftUp(stNode* pParentNode, HBRUSH randBrush ,bool bAssistanceCallFlag)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
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
// ���� �Ʒ� �밢�� �Լ��Դϴ�. 
//======================================================
void CJumpPointSearch::searchLeftDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag)
{
	int x = pParentNode->x;
	int y = pParentNode->y;

	if (bAssistanceCallFlag == false)
	{
		//===================================================================
		// ���� ���ǿ� ������ 5�������� Ž���ϰ� �ȴ�.
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
// �밢�� �Լ��� ���� ������ ���� 
//=======================================================
bool CJumpPointSearch::searchHorizontalRightForDiagonal(stNode* parentNode, eNodeDirection nodeDir,int x, int y, HBRUSH randBrush)
{

	for (int xIndex = x; xIndex < mMapWidth; ++xIndex)
	{
		if (mJspMap[y][xIndex] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return false;
		}
	
		// ������ ��带 ������ ��쿡�� �� �ڸ��� ��� ������ return 
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(parentNode, nodeDir, x, y);
			
			return true;
		}

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// �밢�� �Լ��� ���� ���� ����
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
		// �������� ������ ��� return
		//=============================================
		if (xIndex == mDestinationNode.x && y == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// �밢�� �Լ��� ���� �� ����
//=======================================================
bool CJumpPointSearch::searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush)
{
	
	for (int yIndex = y; yIndex >= 0; --yIndex)
	{
		// ��ֹ� ������ ��� return
		if (mJspMap[yIndex][x] == (char)eNodeAttribute::NODE_BLOCK)
		{
			return false;
		}
		
		//=================================================
		// ������ ������ ��� ��� ���� �� return
		//=================================================
		if (x == mDestinationNode.x && yIndex == mDestinationNode.y)
		{
			setNewNode(pParentNode, nodeDir, x, y);

			return true;
		}


		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// �밢�� �Լ��� ���� �Ʒ� ����
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

		// ���� ����Ʈ ��� �Ǵ� Ŭ���� ����Ʈ ��忡�� ĥ���� �ʴ´�.
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
// ���� ����Ʈ���� ���ĵǾ� �ִ� F�� ��带 �̴´�.
//================================================================
CJumpPointSearch::stNode* CJumpPointSearch::selectOpenListNode()
{
	// F���� ���� ������ �����س��� ������ begin���� ������ ù ��带 ���� �� �ִ�.
	auto iter = mOpenList.begin();

	if (*iter == nullptr)
	{
		return nullptr;
	}

	// ��� �����͸� �����Ѵ�.
	stNode* node = *iter;

	// ���� ����Ʈ���� �ش� ��带 �����.
	mOpenList.erase(iter);

	// closeList�� �־��ش�.
	mCloseList.push_back(node);

	// Ŭ�����Ʈ�̴� ������ �ٲپ��ش�.
	if ((mStartNode.x != node->x || mStartNode.y != node->y) && (mDestinationNode.x != node->x || mDestinationNode.y != node->y) )
	{
		brushBlockList[node->y][node->x] = yellowBrush;
	}

	return node;
}


//=============================================================
// ��� ����Ʈ�� ���� �ʱ�ȭ ��Ų��.
//=============================================================
void CJumpPointSearch::ResetAll(RouteNode* routeNodeArray, int routeNodeArraySize, RouteNode* optimizeNodeArray, int optimizeNodeArraySize)
 {	
	functionFlag = false;

	mbFuncSetFlag = true;

	// ���� ����Ʈ ����
	resetOpenList();

	resetCloseList();

	resetRouteList();

	resetRouteNodeArray(routeNodeArray, routeNodeArraySize);

	resetOptimizeArray(optimizeNodeArray, optimizeNodeArraySize);

	resetOptimizeRoute();

	// ��� ��� �Ӽ� UNBLOCK
	resetJspMap();

	// ��� ����� ������ �⺻ �������� ����
	resetBlock();
}



//==================================================================
// �������� �������� ������ ����Ʈ�� ���� �ʱ�ȭ�ϰ� �ٽ� ���� ã�´�.
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
// ���¸���Ʈ ��带 �����Ѵ�.
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
// Ŭ�����Ʈ ��带 �����Ѵ�.
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
// ���������� ������������ List�� reset�Ѵ�.
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
// �ڿ������� ���������� ������������ ��θ� reset�Ѵ�.
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
// ������ �����Ѵ�.
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
// ������ ��ֺ� ���¸� �����Ѵ�.
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
// �������� ������ ��ֹ��� �����ϰ� �� ������ �����Ѵ�.
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
// ���������� ������������ ��θ� �ִ´�.
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
// ����ȭ ���� �����ϴ� �Լ��Դϴ�.
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
//		// iter�� ���� ��尡 ������ ������� ������ �����Ѵ�.
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
//			// ������ ������� return �Ѵ�.
//			if (nextNextIter == iterE)
//			{
//				return;
//			}
//
//			endX = (*nextNextIter)->x;
//			endY = (*nextNextIter)->y;
//
//			//======================================================================
//			// ������ �̾����ٸ��� ���� ���� ������ ���� üũ�Ѵ�.
//			// nextIter�� ���� ���� �̵��Ѵ�.
//			//======================================================================
//			if (BresenhamLine::MakeLine(startX, startY, endX, endY) == true)
//			{
//				nextIter = mOptimizeRouteList.erase(nextIter);
//			}
//			else
//			{
//				//=====================================================================================
//				// ��ֹ��� ���ؼ� �̾��� �� ���� ������� nextIter ��带 �������� �ٲ۴�.
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
// F�� ���� ������ �����Ѵ�.
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
