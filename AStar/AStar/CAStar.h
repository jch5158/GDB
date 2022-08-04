#pragma once


class CAStar
{
public:

	struct stRouteNode
	{
		// 찾은 경로의 사용여부를 체크해줌
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

		float G;  // 블럭 이동 횟수
		float H;  // 목적지까지의 블럭의 개수
		float F;  // G + H

		eNodeState nodeState;         // 탐색여부
		eNodeAttribute nodeAttribute; // unblock or block
		stNode* pParentNode;          // 부모 노드
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


	// 길 찾기 시작
	bool PathFind(int startX, int startY, int destinationX, int destinationY, std::vector<stRouteNode> &route);

	// 맵 속성 셋팅
	bool SetMapAttribute(int x, int y, eNodeAttribute nodeAttribute);

	// 맵 속성 리셋
	void ResetMapAttribute(void);

private:


	// openList에서 뽑은 노드를 기준으로 주변 노드를 생성한다.
	void findRoute(stNode* pNode);

	// openList에 들어갈 노드 생성
	void createNode(int x, int y, stNode* pParentNode);

	// open List에서 가장 작은 list를 뽑는다.
	stNode* findOpenNode(void);	

	void setRouteArray(stNode* pDestNode, std::vector<stRouteNode>& route);
	
	// 경로 최적화
	bool makeBresenhamLine(int startX, int startY, int endX, int endY);
	void makeOptimizePath(stNode* pNode);

	void resetNodeState(void);  //  NONE_NODE로 초기화

	int mMapWidth;
	int mMapHeight;

	stNode mDestinationNode;

	std::list<stNode*> mOpenList;
	std::vector<std::vector<stNode>> mMap;
};

