#pragma once

class CJumpPointSearch
{
public:

    // 외부에서 사용할 노드
    struct stRouteNode
    {
        bool bUseFlag;
        int x;
        int y;
    };

    // 장애물 확인 enum
    enum class eNodeAttribute : int
    {
        NODE_UNBLOCK,
        NODE_BLOCK,
        NODE_START_POINT,
        NODE_DEST_POINT
    };

private:

    // 탐색 여부 
	enum class eNodeState : int
	{
		NODE_NONE,
		NODE_OPENED,
		NODE_CLOSED
	};

	enum class eNodeDirection : int
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


	struct stNode
	{
		// 블럭 이동 거리
		float G;

		// 목적지까지의 거리
		float H;

		// G+H 값
		float F;

		int x;

		int y;

		eNodeAttribute nodeAttribute;
		eNodeState nodeState;          // open,closed
        eNodeDirection nodeDir;        // 노드 방향

		stNode* pParentNode;
	};

	class CAscendingOrder
	{
	public:

        CAscendingOrder(void) = default;
		~CAscendingOrder(void) = default;

        CAscendingOrder& operator=(const CAscendingOrder&) = delete;

		bool operator()(const stNode* pLeft, const stNode* pRight) const;
	};



public:


    CJumpPointSearch(int mapHeight, int mapWidth);
    ~CJumpPointSearch() = default;

    CJumpPointSearch(const CJumpPointSearch&) = delete;
    CJumpPointSearch& operator=(const CJumpPointSearch&) = delete;

	// map 특성 
	bool SetMapAttribute(int x, int y, eNodeAttribute nodeAttribute);

	void ResetMapAttribute(void);

    bool PathFind(int startX, int startY, int destX, int destY, std::vector<stRouteNode>& route);

private:

	stNode* findOpenNode();

    void findRoute(stNode* node);

	// 노드 생성
	void setNewNode(stNode* pParentNode, eNodeDirection nodeDir, int x, int y);

	// nodeState 초기화
	void resetNodeState(void);

    //===================================================================================================
    // 직선 방향을 탐색하는 함수입니다.
    void searchVerticalUp(stNode* pParentNode);
    void searchVerticalDown(stNode* pParentNode);
    void searchHorizontalRight(stNode* pParentNode);
    void searchHorizontalLeft(stNode* pParentNode);
    //==================================================================================================


    //==================================================================================================================
    // 대각선 방향을 탐색하는 함수입니다.
    void searchRightUp(stNode* pParentNode, bool bAssistanceCallFlag = false);
    void searchRightDown(stNode* pParentNode, bool bAssistanceCallFlag = false);
    void searchLeftUp(stNode* pParentNode, bool bAssistanceCallFlag = false); 
    void searchLeftDown(stNode* pParentNode, bool bAssistanceCallFlag = false);
    //===================================================================================================================


    //===============================================================================================================================
    // 대각선의 직선 탐색 함수입니다.
    bool searchHorizontalRightForDiagonal(stNode* pParentNode, eNodeDirection nodeDir,int x, int y); 
    bool searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y); 
    bool searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y); 
    bool searchVerticalDownForDiagonal(stNode* pParentNode,  eNodeDirection nodeDir, int x, int y);
    //===============================================================================================================================


    // 루트 셋팅
    void setRouteArray(stNode* pDestNode, std::vector<stRouteNode>& route);
    
    // 루트 최적화
    bool makeBresenhamLine(int startX, int startY, int endX, int endY);
    void makeOptimizePath(stNode* pNode);


	int mMapWidth;
	int mMapHeight;

	stNode mDestinationNode;       // 목적지 노드 좌표

	// std::list<stNode*> mOpenList;  // 아직 탐색을 하지 않은 Node List
    std::priority_queue<stNode*, std::vector<stNode*>, CAscendingOrder> mOpenQueue;

	std::vector<std::vector<stNode>> mJspMap;
};


