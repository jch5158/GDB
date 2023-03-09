#pragma once

class CJumpPointSearch
{
public:

    // �ܺο��� ����� ���
    struct stRouteNode
    {
        bool bUseFlag;
        int x;
        int y;
    };

    // ��ֹ� Ȯ�� enum
    enum class eNodeAttribute : int
    {
        NODE_UNBLOCK,
        NODE_BLOCK,
        NODE_START_POINT,
        NODE_DEST_POINT
    };

private:

    // Ž�� ���� 
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
		// �� �̵� �Ÿ�
		float G;

		// ������������ �Ÿ�
		float H;

		// G+H ��
		float F;

		int x;

		int y;

		eNodeAttribute nodeAttribute;
		eNodeState nodeState;          // open,closed
        eNodeDirection nodeDir;        // ��� ����

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

	// map Ư�� 
	bool SetMapAttribute(int x, int y, eNodeAttribute nodeAttribute);

	void ResetMapAttribute(void);

    bool PathFind(int startX, int startY, int destX, int destY, std::vector<stRouteNode>& route);

private:

	stNode* findOpenNode();

    void findRoute(stNode* node);

	// ��� ����
	void setNewNode(stNode* pParentNode, eNodeDirection nodeDir, int x, int y);

	// nodeState �ʱ�ȭ
	void resetNodeState(void);

    //===================================================================================================
    // ���� ������ Ž���ϴ� �Լ��Դϴ�.
    void searchVerticalUp(stNode* pParentNode);
    void searchVerticalDown(stNode* pParentNode);
    void searchHorizontalRight(stNode* pParentNode);
    void searchHorizontalLeft(stNode* pParentNode);
    //==================================================================================================


    //==================================================================================================================
    // �밢�� ������ Ž���ϴ� �Լ��Դϴ�.
    void searchRightUp(stNode* pParentNode, bool bAssistanceCallFlag = false);
    void searchRightDown(stNode* pParentNode, bool bAssistanceCallFlag = false);
    void searchLeftUp(stNode* pParentNode, bool bAssistanceCallFlag = false); 
    void searchLeftDown(stNode* pParentNode, bool bAssistanceCallFlag = false);
    //===================================================================================================================


    //===============================================================================================================================
    // �밢���� ���� Ž�� �Լ��Դϴ�.
    bool searchHorizontalRightForDiagonal(stNode* pParentNode, eNodeDirection nodeDir,int x, int y); 
    bool searchHorizontalLeftForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y); 
    bool searchVerticalUpForDiagonal(stNode* pParentNode, eNodeDirection nodeDir, int x, int y); 
    bool searchVerticalDownForDiagonal(stNode* pParentNode,  eNodeDirection nodeDir, int x, int y);
    //===============================================================================================================================


    // ��Ʈ ����
    void setRouteArray(stNode* pDestNode, std::vector<stRouteNode>& route);
    
    // ��Ʈ ����ȭ
    bool makeBresenhamLine(int startX, int startY, int endX, int endY);
    void makeOptimizePath(stNode* pNode);


	int mMapWidth;
	int mMapHeight;

	stNode mDestinationNode;       // ������ ��� ��ǥ

	// std::list<stNode*> mOpenList;  // ���� Ž���� ���� ���� Node List
    std::priority_queue<stNode*, std::vector<stNode*>, CAscendingOrder> mOpenQueue;

	std::vector<std::vector<stNode>> mJspMap;
};


