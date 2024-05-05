#include "precompiled.h"
#include "Mesh.h"
#include <fstream>

namespace Mesh
{
	// The vertex information that is sent to the display adaptor must match what the vertex shader expects
	struct sVertex
	{
		float x, y, z;//position
		float nx, ny, nz;//normal
		float r, g, b;//color
		float u, v;//coordinate
	};
	

	bool cMesh::LoadBinaryMeshFile(const char*i_binaryMeshFilePath, char*& i_bufferPointer)
	{
		bool wereThereErrors = false;
		std::ifstream meshFile(i_binaryMeshFilePath, std::ifstream::binary);
		if (meshFile)
		{
			meshFile.seekg(0, meshFile.end);
			int length = (int)meshFile.tellg();
			meshFile.seekg(0, meshFile.beg);

			i_bufferPointer = new char[length];

			meshFile.read(i_bufferPointer, length);
			if (!meshFile)
			{
				wereThereErrors = true;
			}


			meshFile.close();
		}
		else
		{
			wereThereErrors = true;
		}

		return !wereThereErrors;
	}

	bool cMesh::CreateVertexAndIndexBuffers()
	{
		// The usage tells Direct3D how this vertex and index buffers will be used
		DWORD usage = 0;
		{
			// Our code will only ever write to the buffer
			usage |= D3DUSAGE_WRITEONLY;
			// The type of vertex processing should match what was specified when the device interface was created with CreateDevice()
			{
				D3DDEVICE_CREATION_PARAMETERS deviceCreationParameters;
				const HRESULT result = pDeviceRef->GetCreationParameters(&deviceCreationParameters);
				if (SUCCEEDED(result))
				{
					DWORD vertexProcessingType = deviceCreationParameters.BehaviorFlags &
						(D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING);
					usage |= (vertexProcessingType != D3DCREATE_SOFTWARE_VERTEXPROCESSING) ?
						0 : D3DUSAGE_SOFTWAREPROCESSING;
				}
				else
				{
					return false;
				}
			}
		}

		if (!CreateVertexBuffer(usage))
		{
			return false;
		}
		if (!CreateIndexBuffer(usage))
		{
			return false;
		}

		return true;
	}
	bool cMesh::CreateVertexBuffer(const DWORD i_usage)
	{
		// Initialize the vertex format
		HRESULT result;

		// Create a vertex buffer
		const unsigned int vertexCount = vertexNum;
		{
			// EAE6320_TODO: How many vertices does a rectangle have?


			const unsigned int bufferSize = vertexCount * sizeof(sVertex);
			// We will define our own vertex format
			const DWORD useSeparateVertexDeclaration = 0;
			// Place the vertex buffer into memory that Direct3D thinks is the most appropriate
			const D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* const notUsed = NULL;

			result = pDeviceRef->CreateVertexBuffer(bufferSize, i_usage, useSeparateVertexDeclaration, useDefaultPool,
				&pVertexBuffer, notUsed);
			if (FAILED(result))
			{
				return false;
			}
		}
		// Fill the vertex buffer with the rectangle's vertices
		{
			// Before the vertex buffer can be changed it must be "locked"
			sVertex* vertexData;
			{
				const unsigned int lockEntireBuffer = 0;
				const DWORD useDefaultLockingBehavior = 0;
				result = pVertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
					reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
				if (FAILED(result))
				{
					return false;
				}
			}
			// Fill the buffer
			{
				unsigned int i = 0;
				while (i < vertexCount)
				{

					//position
					memcpy(&vertexData[i].x, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].y, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].z, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					//normal
					memcpy(&vertexData[i].nx, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].ny, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].nz, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					//color
					memcpy(&vertexData[i].r, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].g, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].b, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;

					//texture coordinate
					memcpy(&vertexData[i].u, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;
					memcpy(&vertexData[i].v, pBinaryFileEntry + binaryFileIndex, sizeof(float));
					binaryFileIndex += 4;

					i++;
				}

			}
			// The buffer must be "unlocked" before it can be used
			{
				result = pVertexBuffer->Unlock();
				if (FAILED(result))
				{
					return false;
				}
			}
		}

		return true;
	}

	bool cMesh::CreateIndexBuffer(const DWORD i_usage)
	{
		// Create an index buffer

		{
			// We'll use 32-bit indices in this class to keep things simple
			D3DFORMAT format = D3DFMT_INDEX32;
			unsigned int bufferLength;
			{
				// EAE6320_TODO: How many triangles in a rectangle?
				const unsigned int verticesPerTriangle = 3;
				const unsigned int triangleNum = (indexNum) / 3;
				bufferLength = verticesPerTriangle * triangleNum * sizeof(unsigned int);
			}
			D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* notUsed = NULL;

			HRESULT result = pDeviceRef->CreateIndexBuffer(bufferLength, i_usage, format, useDefaultPool,
				&pIndexBuffer, notUsed);
			if (FAILED(result))
			{
				return false;
			}
		}
		// Fill the index buffer with the rectangle's triangles' indices
		{
		// Before the index buffer can be changed it must be "locked"
		unsigned int* indices;
		{
			const unsigned int lockEntireBuffer = 0;
			const DWORD useDefaultLockingBehavior = 0;
			const HRESULT result = pIndexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
				reinterpret_cast<void**>(&indices), useDefaultLockingBehavior);
			if (FAILED(result))
			{
				return false;
			}
		}
		// Fill the buffer
		{
			// EAE6320_TODO: What should the indices be
			// in order to draw the required number of triangles
			// using a left-handed winding order?
			unsigned int i = 0;
			while (i < indexNum)
			{

				//triangle for plane first
				memcpy(&indices[i], pBinaryFileEntry + binaryFileIndex, 4);
				binaryFileIndex += 4;

				i++;
			}

		}
		// The buffer must be "unlocked" before it can be used
		{
			const HRESULT result = pIndexBuffer->Unlock();
			if (FAILED(result))
			{
				return false;
			}
		}
	}

		return true;
	}
}