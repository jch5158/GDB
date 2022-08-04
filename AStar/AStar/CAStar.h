#pragma once


class CAStar
{
public:

	struct stRouteNode
	{
		// ã�� ����� ��뿩�θ� üũ����
		bool bUseFlag;
		int x;
		int y;
	};

	enum class eNodeAttribute : int
	{
		NODE_UNBLOCK,
		NODE_BLOCK,
		NODE_START_POINT,
		NODE_DESTINATION_POINT
	};


private:

	enum class eNodeState : int
	{
		NODE_NONE,
		NODE_OPENED,
		NODE_CLOSED
	};

	struct stNode
	{
		int x;
		int y;

		float G;  // �� �̵� Ƚ��
		float H;  // ������������ ���� ����
		float F;  // G + H

		eNodeState nodeState;         // Ž������
		eNodeAttribute nodeAttribute; // unblock or block
		stNode* pParentNode;          // �θ� ���
	};

	class CAscendingOrder
	{
	public:

		CAscendingOrder(void) = default;
		~CAscendingOrder(void) = default;	

		CAscendingOrder(const CAscendingOrder&) = delete;
		CAscendingOrder& operator=(const CAscendingOrder&) = delete;

		bool operator()(const stNode* pLeft, const stNode* pRight) const;
	};


public:

	CAStar(int mapWidth, int mapHeight);
	~CAStar(void) = default;

	CAStar(const CAStar&) = delete;
	CAStar& operator=(const CAStar&) = delete;


	// �� ã�� ����
	bool PathFind(int startX, int startY, int destinationX, int destinationY, std::vector<stRouteNode> &route);

	// �� �Ӽ� ����
	bool SetMapAttribute(int x, int y, eNodeAttribute nodeAttribute);

	// �� �Ӽ� ����
	void ResetMapAttribute(void);

private:


	// openList���� ���� ��带 �������� �ֺ� ��带 �����Ѵ�.
	void findRoute(stNode* pNode);

	// openList�� �� ��� ����
	void createNode(int x, int y, stNode* pParentNode);

	// open List���� ���� ���� list�� �̴´�.
	stNode* findOpenNode(void);	

	void setRouteArray(stNode* pDestNode, std::vector<stRouteNode>& route);
	
	// ��� ����ȭ
	bool makeBresenhamLine(int startX, int startY, int endX, int endY);
	void makeOptimizePath(stNode* pNode);

	void resetNodeState(void);  //  NONE_NODE�� �ʱ�ȭ

	int mMapWidth;
	int mMapHeight;

	stNode mDestinationNode;

	std::list<stNode*> mOpenList;
	std::vector<std::vector<stNode>> mMap;
};

