#ifndef MESH_H
#define MESH_H

#include <d3d9.h>
#include <d3dx9shader.h>
#include <cassert>

namespace Mesh
{
	class cMesh
	{
		char* pBinaryFileEntry;//pointer entry for the binary file
		unsigned int binaryFileIndex;//shift for the binary file
		IDirect3DDevice9* pDeviceRef;
		//IDirect3DVertexBuffer9* pVertexBuffer;
		//IDirect3DIndexBuffer9* pIndexBuffer;

		//unsigned int vertexNum;
		//unsigned int indexNum;

		//private function
		//load binary mesh file
		bool LoadBinaryMeshFile(const char*i_binaryMeshFilePath, char*& i_bufferPointer);
		bool CreateVertexAndIndexBuffers();
		bool CreateVertexBuffer(const DWORD i_usage);
		bool CreateIndexBuffer(const DWORD i_usage);
	public:
		D3DXVECTOR3 meshPosition;
		D3DXVECTOR3 meshScale;
		D3DXMATRIX meshRotationMatrix;
		IDirect3DVertexBuffer9* pVertexBuffer;
		IDirect3DIndexBuffer9* pIndexBuffer;

		unsigned int vertexNum;
		unsigned int indexNum;

		cMesh(const char* i_meshPath, IDirect3DDevice9* i_direct3DDevice)
		{
			//initialize private data first
			meshPosition = D3DXVECTOR3(0, 0, 0);
			meshScale = D3DXVECTOR3(1,1,1);
			D3DXMatrixIdentity(&meshRotationMatrix);
			
			binaryFileIndex = 0;
			vertexNum = 0;
			indexNum = 0;

			pDeviceRef = i_direct3DDevice;
			if (LoadBinaryMeshFile(i_meshPath, pBinaryFileEntry))
			{
				//if load binary file success
				//first get the number of vertex number type(unsigned int)
				memcpy(&vertexNum, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
				binaryFileIndex += sizeof(unsigned int);
				//then get the number of index number type(unsigned int)
				memcpy(&indexNum, pBinaryFileEntry + binaryFileIndex, sizeof(unsigned int));
				binaryFileIndex += sizeof(unsigned int);//every time the shift will change
				//now the shift is at the first vertex
				if (!CreateVertexAndIndexBuffers())
					return;
				
			}
		}
		~cMesh()
		{
			//when a mesh is released
			pDeviceRef = NULL;

			delete[] pBinaryFileEntry;
			pBinaryFileEntry = NULL;

			if (pVertexBuffer)
			{
				pVertexBuffer->Release();
				pVertexBuffer = NULL;
			}
			if (pIndexBuffer)
			{
				pIndexBuffer->Release();
				pIndexBuffer = NULL;
			}
		}
	};
}

#endif