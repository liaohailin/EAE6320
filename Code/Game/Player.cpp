#include "precompiled.h"
#include "Player.h"


#define EPSILON 0.0001



namespace Player
{
	struct sVertex
	{
		float x, y, z;//position
		float nx, ny, nz;//normal
		float r, g, b;//color
		float u, v;//coordinate
	};
	
	cPlayer::cPlayer(Mesh::cMesh* i_pPlayerMesh)
	{
		//collision information
		IdentifyCollisionSegment();
		floorFlag=false;
		floorHeight=0.0f;
		pPlayerMesh = i_pPlayerMesh;
	}
	cPlayer::~cPlayer(void)
	{
		pPlayerMesh = NULL;
	}

	void cPlayer::UpdateCollisionSegment(D3DXVECTOR3 i_playerPosition, D3DXVECTOR3 i_playerScale)
	{
		if (pPlayerMesh != NULL)
		{
			//update the collision segment
			IdentifyCollisionSegment();
			ScaleCollisionSegment(i_playerScale);
			TranslateCollisionSegment(i_playerPosition);
		}
	}
	void cPlayer::IdentifyCollisionSegment(void)
	{
		forwardSeg.startPoint = D3DXVECTOR3(0, 0, 0);
		forwardSeg.endPoint = D3DXVECTOR3(0, 0, 1);
		backwardSeg.startPoint = D3DXVECTOR3(0, 0, 0);
		backwardSeg.endPoint = D3DXVECTOR3(0, 0, -1);
		leftSeg.startPoint = D3DXVECTOR3(0, 0, 0);
		leftSeg.endPoint = D3DXVECTOR3(-1, 0, 0);
		rightSeg.startPoint = D3DXVECTOR3(0, 0, 0);
		rightSeg.endPoint = D3DXVECTOR3(1, 0, 0);
		upSeg.startPoint = D3DXVECTOR3(0, 0, 0);
		upSeg.endPoint = D3DXVECTOR3(0, 1, 0);
		downSeg.startPoint = D3DXVECTOR3(0, 0, 0);
		downSeg.endPoint = D3DXVECTOR3(0, -1, 0);
	}
	void cPlayer::ScaleCollisionSegment(D3DXVECTOR3 i_scleElement)
	{
		forwardSeg.endPoint *= i_scleElement.z;
		backwardSeg.endPoint *= i_scleElement.z;
		leftSeg.endPoint *= i_scleElement.x;
		rightSeg.endPoint *= i_scleElement.x;
		upSeg.endPoint *= i_scleElement.y;
		downSeg.endPoint *= i_scleElement.y;
	}
	void cPlayer::TranslateCollisionSegment(D3DXVECTOR3 i_translateElement)
	{
		forwardSeg.startPoint += i_translateElement;
		forwardSeg.endPoint += i_translateElement;
		backwardSeg.startPoint += i_translateElement;
		backwardSeg.endPoint += i_translateElement;
		leftSeg.startPoint += i_translateElement;
		leftSeg.endPoint += i_translateElement;
		rightSeg.startPoint += i_translateElement;
		rightSeg.endPoint += i_translateElement;
		upSeg.startPoint += i_translateElement;
		upSeg.endPoint += i_translateElement;
		downSeg.startPoint += i_translateElement;
		downSeg.endPoint += i_translateElement;
	}
	int cPlayer::IntersectSegmentTriangle(D3DXVECTOR3 i_segStart, D3DXVECTOR3 i_segEnd,
		D3DXVECTOR3 i_triangleA, D3DXVECTOR3 i_triangleB, D3DXVECTOR3 i_triangleC, float& floorHight)
	{
		float u; float v; float w; float t;
		D3DXVECTOR3 ab = i_triangleB - i_triangleA;
		D3DXVECTOR3 ac = i_triangleC - i_triangleA;
		D3DXVECTOR3 qp = i_segStart - i_segEnd;

		// Compute triangle normal. Can be precalculated or cached if
		// intersecting multiple segments against the same triangle
		D3DXVECTOR3 n;
		D3DXVec3Cross(&n, &ab, &ac);

		// Compute denominator d. If d <= 0, segment is parallel to or points
		// away from triangle, so exit early
		float d = D3DXVec3Dot(&qp, &n);
		if (d <= 0.0f) return 0;

		// Compute intersection t value of pq with plane of triangle. A ray
		// intersects if 0 <= t. Segment intersects if 0 <= t <= 1. Delay
		// dividing by d until intersection has been found to pierce triangle
		D3DXVECTOR3 ap = i_segStart - i_triangleA;
		t = D3DXVec3Dot(&ap, &n);
		if (t < 0.0f) return 0;
		if (t > d) return 0; // For segment; exclude this code line for a ray test

		// Compute barycentric coordinate components and test if within bounds
		D3DXVECTOR3 e;
		D3DXVec3Cross(&e,&qp, &ap);
		v = D3DXVec3Dot(&ac, &e);
		if (v < 0.0f || v > d) return 0;
		w = -D3DXVec3Dot(&ab, &e);
		if (w < 0.0f || v + w > d) return 0;

		// Segment/ray intersects triangle. Perform delayed division and
		// compute the last barycentric coordinate component
		float ood = 1.0f / d;
		t *= ood;
		v *= ood;
		w *= ood;
		u = 1.0f - v - w;

		floorHight = v;
		return 1;
	}
	int cPlayer::IntersectSegmentOctreeBox(D3DXVECTOR3 i_segStart, D3DXVECTOR3 i_setEnd,float i_halfWidth, float i_centerX, float i_centerY, float i_centerZ)
	{
		D3DXVECTOR3 c(i_centerX,i_centerY, i_centerZ); // Box center-point
		D3DXVECTOR3 e (i_halfWidth,i_halfWidth,i_halfWidth); // Box halflength extents
		D3DXVECTOR3 m = (i_segStart + i_setEnd) * 0.5f; // Segment midpoint
		D3DXVECTOR3 d = i_setEnd - m; // Segment halflength vector
		m = m - c; // Translate box and segment to origin

		// Try world coordinate axes as separating axes
		float adx = abs(d.x);
		if (abs(m.x) > e.x + adx) return 0;
		float ady = abs(d.y);
		if (abs(m.y) > e.y + ady) return 0;
		float adz = abs(d.z);
		if (abs(m.z) > e.z + adz) return 0;

		// Add in an epsilon term to counteract arithmetic errors when segment is
		// (near) parallel to a coordinate axis
		adx += EPSILON;
		ady += EPSILON;
		adz += EPSILON;

		// Try cross products of segment direction vector with coordinate axes
		if (abs(m.y * d.z - m.z * d.y) > e.y * adz + e.z * ady) return 0;
		if (abs(m.z * d.x - m.x * d.z) > e.x * adz + e.z * adx) return 0;
		if (abs(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx) return 0;

		// No separating axis found; segment must be overlapping AABB
		return 1;
	}
	bool cPlayer::IfCollideWithEnvironment(Mesh::cMesh* i_collideObj)
	{
		//first get the triangle vectors
		//in order to get the vertex and index we need to lock the buffer
		//first detect if there is an intersection with forward segment
		unsigned int* indices;
		unsigned int indexNum = i_collideObj->indexNum;
		{
			const unsigned int lockEntireBuffer = 0;
			const DWORD useDefaultLockingBehavior = 0;
			const HRESULT result = i_collideObj->pIndexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
				reinterpret_cast<void**>(&indices), useDefaultLockingBehavior);
			if (FAILED(result))
			{
				return false;
			}
		}

		sVertex* vertexData;
		{
			const unsigned int lockEntireBuffer = 0;
			const DWORD useDefaultLockingBehavior = 0;
			const HRESULT result = i_collideObj->pVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
				reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
			if (FAILED(result))
			{
				return false;
			}
		}

		//loop through indices and detect if there is a collision
		unsigned int i = 0;
		
		//forward
		while (i < indexNum)
		{
			if (IntersectSegmentTriangle(forwardSeg.startPoint, forwardSeg.endPoint, D3DXVECTOR3(vertexData[indices[i]].x, vertexData[indices[i]].y, vertexData[indices[i]].z),
				D3DXVECTOR3(vertexData[indices[i + 1]].x, vertexData[indices[i + 1]].y, vertexData[indices[i + 1]].z), D3DXVECTOR3(vertexData[indices[i + 2]].x, vertexData[indices[i + 2]].y, vertexData[indices[i + 2]].z),floorHeight))
			{
				floorFlag = false;
				//unlock buffers
				HRESULT result = i_collideObj->pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				result = i_collideObj->pIndexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				return true;
			}

			i += 3;
		}
		//backward
		i = 0;
		while (i < indexNum)
		{
			if (IntersectSegmentTriangle(backwardSeg.startPoint, backwardSeg.endPoint, D3DXVECTOR3(vertexData[indices[i]].x, vertexData[indices[i]].y, vertexData[indices[i]].z),
				D3DXVECTOR3(vertexData[indices[i + 1]].x, vertexData[indices[i + 1]].y, vertexData[indices[i + 1]].z), D3DXVECTOR3(vertexData[indices[i + 2]].x, vertexData[indices[i + 2]].y, vertexData[indices[i + 2]].z), floorHeight))
			{
				floorFlag = false;
				//unlock buffers
				HRESULT result = i_collideObj->pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				result = i_collideObj->pIndexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				return true;
			}

			i += 3;
		}
		//left
		i = 0;
		while (i < indexNum)
		{
			if (IntersectSegmentTriangle(leftSeg.startPoint, leftSeg.endPoint, D3DXVECTOR3(vertexData[indices[i]].x, vertexData[indices[i]].y, vertexData[indices[i]].z),
				D3DXVECTOR3(vertexData[indices[i + 1]].x, vertexData[indices[i + 1]].y, vertexData[indices[i + 1]].z), D3DXVECTOR3(vertexData[indices[i + 2]].x, vertexData[indices[i + 2]].y, vertexData[indices[i + 2]].z), floorHeight))
			{
				floorFlag = false;
				//unlock buffers
				HRESULT result = i_collideObj->pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				result = i_collideObj->pIndexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				return true;
			}

			i += 3;
		}
		//right
		i = 0;
		while (i < indexNum)
		{
			if (IntersectSegmentTriangle(rightSeg.startPoint, rightSeg.endPoint, D3DXVECTOR3(vertexData[indices[i]].x, vertexData[indices[i]].y, vertexData[indices[i]].z),
				D3DXVECTOR3(vertexData[indices[i + 1]].x, vertexData[indices[i + 1]].y, vertexData[indices[i + 1]].z), D3DXVECTOR3(vertexData[indices[i + 2]].x, vertexData[indices[i + 2]].y, vertexData[indices[i + 2]].z), floorHeight))
			{
				floorFlag = false;
				//unlock buffers
				HRESULT result = i_collideObj->pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				result = i_collideObj->pIndexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				return true;
			}

			i += 3;
		}
		//up
		i = 0;
		while (i < indexNum)
		{
			if (IntersectSegmentTriangle(upSeg.startPoint, upSeg.endPoint, D3DXVECTOR3(vertexData[indices[i]].x, vertexData[indices[i]].y, vertexData[indices[i]].z),
				D3DXVECTOR3(vertexData[indices[i + 1]].x, vertexData[indices[i + 1]].y, vertexData[indices[i + 1]].z), D3DXVECTOR3(vertexData[indices[i + 2]].x, vertexData[indices[i + 2]].y, vertexData[indices[i + 2]].z), floorHeight))
			{
				floorFlag = false;
				//unlock buffers
				HRESULT result = i_collideObj->pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				result = i_collideObj->pIndexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				return true;
			}

			i += 3;
		}
		//down
		i = 0;
		while (i < indexNum)
		{
			if (IntersectSegmentTriangle(downSeg.startPoint, downSeg.endPoint, D3DXVECTOR3(vertexData[indices[i]].x, vertexData[indices[i]].y, vertexData[indices[i]].z),
				D3DXVECTOR3(vertexData[indices[i + 1]].x, vertexData[indices[i + 1]].y, vertexData[indices[i + 1]].z), D3DXVECTOR3(vertexData[indices[i + 2]].x, vertexData[indices[i + 2]].y, vertexData[indices[i + 2]].z),floorHeight))
			{
				floorFlag = true;
				//unlock buffers
				HRESULT result = i_collideObj->pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				result = i_collideObj->pIndexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
				return true;
			}

			i += 3;
		}
		{
			//unlock buffers
			HRESULT result = i_collideObj->pVertexBuffer->Unlock();
			if (FAILED(result))
			{
				return false;
			}
			result = i_collideObj->pIndexBuffer->Unlock();
			if (FAILED(result))
			{
				return false;
			}
		}
		floorFlag = false;
		return false;
	}

	bool cPlayer::IfCollideWithOctreeScene(OctreeNode* i_octreeNode, std::vector<float>& i_triangleVerticesList, std::vector<unsigned int>& i_triangleIndicesList,std::vector<unsigned int>&o_triangleTodraw)
	{
		//decide where the player is and add the possible triangles to the list
		{
			//loop through octree list and detect if there is a collision
			//forward
			AddToTriangleList(i_octreeNode, o_triangleTodraw, forwardSeg.startPoint, forwardSeg.endPoint);
			//backward
			AddToTriangleList(i_octreeNode, o_triangleTodraw, backwardSeg.startPoint, backwardSeg.endPoint);
			//left
			AddToTriangleList(i_octreeNode, o_triangleTodraw, leftSeg.startPoint, leftSeg.endPoint);
			//right
			AddToTriangleList(i_octreeNode, o_triangleTodraw, rightSeg.startPoint, rightSeg.endPoint);
			//up
			AddToTriangleList(i_octreeNode, o_triangleTodraw, upSeg.startPoint, upSeg.endPoint);
			//down
			AddToTriangleList(i_octreeNode, o_triangleTodraw, downSeg.startPoint, downSeg.endPoint);
		}

		//detect if collide with triangles
		{
			
			//loop through indices and detect if there is a collision
			unsigned int i = 0;

			//down
			while (i < o_triangleTodraw.size())
			{
				if (IntersectSegmentTriangle(downSeg.startPoint, downSeg.endPoint, 
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 2]), floorHeight))
				{
					floorFlag = true;
					return true;
				}

				i++;
			}
			//forward
			i = 0;
			while (i < o_triangleTodraw.size())
			{
				if (IntersectSegmentTriangle(forwardSeg.startPoint, forwardSeg.endPoint, 
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 2]), floorHeight))
				{
					floorFlag = false;
					return true;
				}

				i ++;
			}
			//backward
			i = 0;
			while (i < o_triangleTodraw.size())
			{
				if (IntersectSegmentTriangle(backwardSeg.startPoint, backwardSeg.endPoint, 
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 2]), floorHeight))
				{
					floorFlag = false;
					return true;
				}

				i++;
			}
			//left
			i = 0;
			while (i < o_triangleTodraw.size())
			{
				if (IntersectSegmentTriangle(leftSeg.startPoint, leftSeg.endPoint, 
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 2]), floorHeight))
				{
					floorFlag = false;
					return true;
				}

				i++;
			}
			//right
			i = 0;
			while (i < o_triangleTodraw.size())
			{
				if (IntersectSegmentTriangle(rightSeg.startPoint, rightSeg.endPoint, 
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 2]), floorHeight))
				{
					floorFlag = false;
					return true;
				}

				i++;
			}
			//up
			i = 0;
			while (i < o_triangleTodraw.size())
			{
				if (IntersectSegmentTriangle(upSeg.startPoint, upSeg.endPoint, 
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 1] * 3 + 2]),
					D3DXVECTOR3(i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 1], i_triangleVerticesList[i_triangleIndicesList[o_triangleTodraw[i] * 3 + 2] * 3 + 2]), floorHeight))
				{
					floorFlag = false;
					return true;
				}

				i++;
			}
			
			floorFlag = false;
			return false;
		}
		
	}
	void cPlayer::AddToTriangleList(OctreeNode* i_octreeNode, std::vector<unsigned int>& i_triangleVerticesIndexList, D3DXVECTOR3& i_segStart, D3DXVECTOR3& i_segEnd)
	{
		if (i_octreeNode == NULL)return;

		if (IntersectSegmentOctreeBox(i_segStart, i_segEnd, i_octreeNode->halfWidth, i_octreeNode->centerX, i_octreeNode->centerY, i_octreeNode->centerZ))
		{
			if (i_octreeNode->triangleList.size() > 0)
			{
				for (unsigned int i = 0; i < i_octreeNode->triangleList.size(); i++)
				{
					i_triangleVerticesIndexList.push_back(i_octreeNode->triangleList[i]);
				}
			}
			else
			{
				for (unsigned int i = 0; i < 8; i++)
				{
					if (i_octreeNode->children[i] == NULL)continue;
					AddToTriangleList(i_octreeNode->children[i], i_triangleVerticesIndexList, i_segStart, i_segEnd);
				}
			}
		}
		else return;
	}
}

