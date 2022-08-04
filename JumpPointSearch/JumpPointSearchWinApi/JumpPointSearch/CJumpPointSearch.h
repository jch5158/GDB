#pragma once

#include <iostream>
#include <Windows.h>
#include <list>

class CJumpPointSearch
{
public:

    struct stNode
    {
        int x;

        int y;

        BYTE nodeDir;

        stNode* pParentNode;

        // 블럭 이동 체크 ( 대각선은 1.5f 직선은 1.0f 이다. )
        float G;

        // 목적지까지의 블럭의 개수 ( X + Y )
        float H;

        // G+H 값
        float F;
    };


    CJumpPointSearch(int x, int y);

    ~CJumpPointSearch();

    bool PathFind(int startX, int startY, int destinationX, int destinationY, RouteNode routeNodeArray[], int routeNodeArraySize, RouteNode optimizeNodeArray[], int optimizeNodeArraySize);

    void SettingMapAttribute(int posX, int posY);

    void ResetAll(RouteNode* routeNodeArray, int routeNodeArraySize, RouteNode* optimizeNodeArray, int optimizeNodeArraySize);

    void ReStart(RouteNode* routeNodeArray, int routeNodeArraySize, RouteNode* optimizeNodeArray, int optimizeNodeArraySize);
   

private:

    CJumpPointSearch(void);

    // 장애물 확인 enum
    enum class eNodeAttribute
    {
        NODE_UNBLOCK,
        NODE_BLOCK
    };

    enum class eNodeDirection
    {
        NODE_DIR_RR,
        NODE_DIR_RD,
        NODE_DIR_DD,
        NODE_DIR_LD,
        NODE_DIR_LL,
        NODE_DIR_LU,
        NODE_DIR_UU,
        NODE_DIR_RU,
        NODE_DIR_ALL
    };

    class CSortAscendingOrder
    {
    public:

        CSortAscendingOrder(void);

        ~CSortAscendingOrder(void);

        bool operator()(const stNode* pLeft, const stNode* pRight) const;
    };

    stNode* findOpenList(int openX, int openY, eNodeDirection nodeDir);

    stNode* findCloseList(int openX, int openY);

    void findRouteNode(stNode* pNode);

    stNode* selectOpenListNode();

    //===================================================================================================
    // 직선 방향을 탐색하는 함수입니다.
    void searchVerticalUp(stNode* pParentNode, HBRUSH randBrush);
    void searchVerticalDown(stNode* pParentNode, HBRUSH randBrush);
    void searchHorizontalRight(stNode* pParentNode, HBRUSH randBrush);
    void searchHorizontalLeft(stNode* pParentNode, HBRUSH randBrush);
    //==================================================================================================


    //==================================================================================================================
    // 대각선 방향을 탐색하는 함수입니다.
    void searchRightUp(stNode* pParentNode, HBRUSH randBrush ,bool bAssistanceCallFlag = false);
    void searchRightDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag = false);
    void searchLeftUp(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag = false); 
    void searchLeftDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag = false);
    //===================================================================================================================


    //===============================================================================================================================
    // 대각선의 직선 탐색 함수입니다.
    bool searchHorizontalRightForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush); 
    bool searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush); 
    bool searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush); 
    bool searchVerticalDownForDiagonal(stNode* pParentNode,  eNodeDirection nodeDir, int x, int y, HBRUSH randBrush);
    //===============================================================================================================================


    void setNewNode(stNode* pParentNode, eNodeDirection nodeDir, int x, int y);

    // 노드이 G,H,F 값을 셋팅한다.
    void setNodeGHF(stNode* pParentNode, int x, int y, float* pG, float* pH, float* pF);

    //void insertRoute(stNode* node);

    // 루트 최적화
    void pathOptimize(stNode* pNode);
 
    // 오픈 리스트 리셋
    void resetOpenList();

    // 클로즈 리스트 리셋
    void resetCloseList();

    // 일반 루트 리스트 리셋
    void resetRouteList();
    
    // 최적화 루트 리스트 리셋
    void resetOptimizeRoute();

    // brush 블럭 리셋
    void resetBlock();

    // jsp 맵 리셋
    void resetJspMap();

    // brush 시작점 장애물 끝점 제외하고 리셋
    void resetRoute();
    

    // 배열 루트 셋팅
    void setRouteArray(stNode* pDestNode, RouteNode routeNodeArray[], int routeNodeArraySize);

    // 루트 배열 리셋
    void resetRouteNodeArray(RouteNode routeNodeArray[], int routeNodeArraySize);

    void setOptimizeArray(stNode* pDestNode, RouteNode optimizeNodeArray[], int optimizeNodeArraySize);

    void resetOptimizeArray(RouteNode optimizeNodeArray[], int optimizeNodeArraySize);

    // F 값 정렬
    //void openListBubbleSort();

    char **mJspMap;
    
    int mMapWidth;
    
    int mMapHeight;

    bool mbFuncSetFlag;

    stNode mStartNode;

    stNode mDestinationNode;

    //CList<CJumpPointSearch::stNode*> mOpenList;
    std::list<stNode*> mOpenList;

    //CList<CJumpPointSearch::stNode*> mCloseList;
    std::list<stNode*> mCloseList;


    //CList<CJumpPointSearch::stNode*> mRouteList;
    std::list<stNode*> mRouteList;


    //CList<CJumpPointSearch::stNode*> mOptimizeRouteList;
    std::list<stNode*> mOptimizeRouteList;
};


