#ifndef PLAYER_H
#define PLAYER_H
#include "d3dx9.h"
#include "Mesh.h"
#include <vector>


namespace Player
{
	
	struct OctreeNode
	{
		bool isCollision;
		unsigned int depth;
		float halfWidth;
		float centerX, centerY, centerZ;
		std::vector<unsigned int> triangleList;
		OctreeNode* children[8];
	};

	struct Segment
	{
		D3DXVECTOR3 startPoint;
		D3DXVECTOR3 endPoint;
	};

	class cPlayer
	{
		void IdentifyCollisionSegment(void);
		void ScaleCollisionSegment(D3DXVECTOR3 i_scaleElement);
		void TranslateCollisionSegment(D3DXVECTOR3 i_translateElement);
		int IntersectSegmentTriangle(D3DXVECTOR3 i_segStart, D3DXVECTOR3 i_segEnd, 
			D3DXVECTOR3 i_triangleA, D3DXVECTOR3 i_triangleB, D3DXVECTOR3 i_triangleC,float& floorHight);
		
	public:
		Segment forwardSeg;
		Segment backwardSeg;
		Segment leftSeg;
		Segment rightSeg;
		Segment upSeg;
		Segment downSeg;
		bool floorFlag;
		float floorHeight;

		Mesh::cMesh* pPlayerMesh;
		cPlayer(Mesh::cMesh* i_pPlayerMesh);
		~cPlayer(void);
		void UpdateCollisionSegment(D3DXVECTOR3 i_playerPosition, D3DXVECTOR3 i_playerScale);
		bool IfCollideWithEnvironment(Mesh::cMesh* i_collideObj);
		bool IfCollideWithOctreeScene(OctreeNode* i_octreeNode, std::vector<float>& i_triangleVerticesList, std::vector<unsigned int>& i_triangleIndicesList,std::vector<unsigned int>&o_triangleTodraw);
		bool AABBColision(float &o_backAmountX, float &o_backAmountY);
		void AddToTriangleList(OctreeNode* i_octreeNode,std::vector<unsigned int>& i_triangleVerticesIndexList,D3DXVECTOR3& i_segStart, D3DXVECTOR3& i_segEnd);
		int IntersectSegmentOctreeBox(D3DXVECTOR3 i_segStart, D3DXVECTOR3 i_setEnd,//segment
			float i_halfWidth, float i_centerX, float i_centerY, float i_centerZ);//octree info
	};
}
#endif