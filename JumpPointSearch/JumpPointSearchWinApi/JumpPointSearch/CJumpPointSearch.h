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

        // �� �̵� üũ ( �밢���� 1.5f ������ 1.0f �̴�. )
        float G;

        // ������������ ���� ���� ( X + Y )
        float H;

        // G+H ��
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

    // ��ֹ� Ȯ�� enum
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
    // ���� ������ Ž���ϴ� �Լ��Դϴ�.
    void searchVerticalUp(stNode* pParentNode, HBRUSH randBrush);
    void searchVerticalDown(stNode* pParentNode, HBRUSH randBrush);
    void searchHorizontalRight(stNode* pParentNode, HBRUSH randBrush);
    void searchHorizontalLeft(stNode* pParentNode, HBRUSH randBrush);
    //==================================================================================================


    //==================================================================================================================
    // �밢�� ������ Ž���ϴ� �Լ��Դϴ�.
    void searchRightUp(stNode* pParentNode, HBRUSH randBrush ,bool bAssistanceCallFlag = false);
    void searchRightDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag = false);
    void searchLeftUp(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag = false); 
    void searchLeftDown(stNode* pParentNode, HBRUSH randBrush, bool bAssistanceCallFlag = false);
    //===================================================================================================================


    //===============================================================================================================================
    // �밢���� ���� Ž�� �Լ��Դϴ�.
    bool searchHorizontalRightForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush); 
    bool searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush); 
    bool searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y, HBRUSH randBrush); 
    bool searchVerticalDownForDiagonal(stNode* pParentNode,  eNodeDirection nodeDir, int x, int y, HBRUSH randBrush);
    //===============================================================================================================================


    void setNewNode(stNode* pParentNode, eNodeDirection nodeDir, int x, int y);

    // ����� G,H,F ���� �����Ѵ�.
    void setNodeGHF(stNode* pParentNode, int x, int y, float* pG, float* pH, float* pF);

    //void insertRoute(stNode* node);

    // ��Ʈ ����ȭ
    void pathOptimize(stNode* pNode);
 
    // ���� ����Ʈ ����
    void resetOpenList();

    // Ŭ���� ����Ʈ ����
    void resetCloseList();

    // �Ϲ� ��Ʈ ����Ʈ ����
    void resetRouteList();
    
    // ����ȭ ��Ʈ ����Ʈ ����
    void resetOptimizeRoute();

    // brush �� ����
    void resetBlock();

    // jsp �� ����
    void resetJspMap();

    // brush ������ ��ֹ� ���� �����ϰ� ����
    void resetRoute();
    

    // �迭 ��Ʈ ����
    void setRouteArray(stNode* pDestNode, RouteNode routeNodeArray[], int routeNodeArraySize);

    // ��Ʈ �迭 ����
    void resetRouteNodeArray(RouteNode routeNodeArray[], int routeNodeArraySize);

    void setOptimizeArray(stNode* pDestNode, RouteNode optimizeNodeArray[], int optimizeNodeArraySize);

    void resetOptimizeArray(RouteNode optimizeNodeArray[], int optimizeNodeArraySize);

    // F �� ����
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


