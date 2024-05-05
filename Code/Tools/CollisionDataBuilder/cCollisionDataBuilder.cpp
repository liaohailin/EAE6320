// Header Files
//=============

#include "cCollisionDataBuilder.h"

#include <cassert>
#include <sstream>
#include <fstream>
// Interface
//==========

// Build
//------

bool eae6320::cCollisionDataBuilder::Build(const std::vector<const std::string>& i_arguments)
{
	bool wereThereErrors = false;
	//build the mesh file into binary file
	{
		const bool dontFailIfTargetAlreadyExists = false;
		std::string errorMessage;
		//load mesh file
		if (!LoadMesh(m_path_source))
		{
			OutputErrorMessage("something wrong with the file", m_path_source);
			wereThereErrors = true;
		}

	}
	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoadMesh(const char*i_path)
{
	bool wereThereErrors = false;

	//create lua state
	lua_State* luaState = NULL;
	{
		luaState = luaL_newstate();
		if (!luaState)
		{
			wereThereErrors = true;
			OutputErrorMessage("fail to create a new lua state");
			goto OnExit;
		}
	}
	// Load the asset file as a "chunk",
	// meaning there will be a callable function at the top of the stack
	{
		const int luaResult = luaL_loadfile(luaState, i_path);
		if (luaResult != LUA_OK)
		{
			wereThereErrors = true;
			OutputErrorMessage("failed to load lua");
			// Pop the error message
			lua_pop(luaState, 1);
			goto OnExit;
		}
	}
	// Execute the "chunk", which should load the asset
	// into a table at the top of the stack
	{
		const int argumentCount = 0;
		const int returnValueCount = LUA_MULTRET;	// Return _everything_ that the file returns
		const int noMessageHandler = 0;
		const int luaResult = lua_pcall(luaState, argumentCount, returnValueCount, noMessageHandler);
		if (luaResult == LUA_OK)
		{
			// A well-behaved asset file will only return a single value
			const int returnedValueCount = lua_gettop(luaState);
			if (returnedValueCount == 1)
			{
				// A correct asset file _must_ return a table
				if (!lua_istable(luaState, -1))
				{
					wereThereErrors = true;
					OutputErrorMessage("Asset files must return a table");
					// Pop the returned non-table value
					lua_pop(luaState, 1);
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				OutputErrorMessage("Asset files must return a single table");
				// Pop every value that was returned
				lua_pop(luaState, returnedValueCount);
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("lua result not OK");
			// Pop the error message
			lua_pop(luaState, 1);
			goto OnExit;
		}
	}
	// If this code is reached the asset file was loaded successfully,
	// and its table is now at index -1
	if (!ReadMeshFile(*luaState))
	{
		wereThereErrors = true;
	}

	// Pop the table
	lua_pop(luaState, 1);
OnExit:

	if (luaState)
	{
		// If I haven't made any mistakes
		// there shouldn't be anything on the stack,
		// regardless of any errors encountered while loading the file:
		assert(lua_gettop(luaState) == 0);

		lua_close(luaState);
		luaState = NULL;
	}

	return !wereThereErrors;

}

bool eae6320::cCollisionDataBuilder::ReadMeshFile(lua_State& io_luaState)
{
	std::ofstream outfile(m_path_target, std::ofstream::binary);
	bool wereThereErrors = false;

	octreeNum = 0;
	octreeMaxDepth = 0;
	triangleInListNum = 0;
	vertexNum = 0;
	indexNum = 0;
	//after read the size of vertices and indices initialize buffer
	bufferIndex = 0;
	//read octree, vertex and index number
	{
		if (!ReadOctreeMaxDepth(io_luaState))
		{
			wereThereErrors = true;
			goto OnExit;
		}
		if (!GetOctreeNodeNumber(io_luaState))
		{
			wereThereErrors = true;
			goto OnExit;
		}
		if(!GetVertexNumber(io_luaState))
		{
			wereThereErrors = true;
			goto OnExit;
		}
		if(!GetIndexNumber(io_luaState))
		{
		wereThereErrors = true;
		goto OnExit;
		}
	}

	//allocate memory
	fileSize =
		sizeof(unsigned int)//triangle number
		+sizeof(unsigned int)//indices number
		+sizeof(unsigned int)*triangleInListNum//triangle in triangle list number
		+ sizeof(float)* vertexNum//vertex 
		+ sizeof(unsigned int)*indexNum//index
		//octree
		+ sizeof(unsigned int)//octree max depth
		+(sizeof(bool)//iscollision
		+sizeof(float)//depth
		+sizeof(unsigned int)*octreeMaxDepth //node track
		+ sizeof(float)//half width
		+sizeof(float)* 3 //center
		+ sizeof(unsigned int)//triangle number
		)*octreeNum;
		

	buffer = new char[fileSize];

	//read and write triangles
	memcpy(buffer + bufferIndex, &vertexNum, sizeof(vertexNum));
	bufferIndex += sizeof(vertexNum);
	memcpy(buffer + bufferIndex, &indexNum, sizeof(indexNum));
	bufferIndex += sizeof(indexNum);

	if (!ReadVertexData(io_luaState))
	{
		wereThereErrors = true;
		goto OnExit;
	}
	if (!ReadIndexData(io_luaState))
	{
		wereThereErrors = true;
		goto OnExit;
	}

	//read and write octree
	memcpy(buffer + bufferIndex, &octreeMaxDepth, sizeof(octreeMaxDepth));
	bufferIndex += sizeof(octreeMaxDepth);
	
	if (!ReadOctreeNode(io_luaState))
	{
		wereThereErrors = true;
		goto OnExit;
	}
	

	outfile.write(buffer, fileSize);
	delete[] buffer;
	outfile.close();


OnExit:
	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::GetOctreeNodeNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* octreesString = "octrees";
	lua_pushstring(&io_luaState, octreesString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		octreeNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack
		//loop through and get the triangle
		LoopThroughtOctreeNodeToGetTriangleNumber(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("vertices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoopThroughtOctreeNodeToGetTriangleNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2

	unsigned int i = 1;
	while (i <= octreeNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_istable(&io_luaState, -1))
			AddToTriangleNumber(io_luaState);
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("vertex data chunck is not a table");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::AddToTriangleNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* triangleNumString = "triangleNum";
	lua_pushstring(&io_luaState, triangleNumString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
	{
		//add current is collision bool to buffer
		triangleInListNum += (unsigned int)lua_tonumber(&io_luaState, -1);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("get triangle number data error");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetVertexNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* vertexString = "vertices";
	lua_pushstring(&io_luaState, vertexString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		vertexNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("vertices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetIndexNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* indexString = "indices";
	lua_pushstring(&io_luaState, indexString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		indexNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("vertices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::ReadOctreeMaxDepth(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* octreesDepthString = "octreeDepth";
	lua_pushstring(&io_luaState, octreesDepthString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
	{//loop through vertices data chunck
		//add current is collision bool to buffer
		octreeMaxDepth = (unsigned int)lua_tonumber(&io_luaState, -1);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("octree max depth is not a number");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::ReadOctreeNode(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* octreesString = "octrees";
	lua_pushstring(&io_luaState, octreesString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{//loop through vertices data chunck
		LoopThroughtOctreesData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("vertices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoopThroughtOctreesData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2

	unsigned int i = 1;
	while (i <= octreeNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_istable(&io_luaState, -1))
			ReadOneOctreeData(io_luaState);
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("vertex data chunck is not a table");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::ReadOneOctreeData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now the main table is at -3

	//first read position data
	if (!GetNodeTrackInfo(io_luaState))
	{
		OutputErrorMessage("fail to get depth info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetIsCollisionInfo(io_luaState))
	{
		OutputErrorMessage("fail to get iscollision info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetDepthInfo(io_luaState))
	{
		OutputErrorMessage("fail to get depth info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetHalfWidthInfo(io_luaState))
	{
		OutputErrorMessage("fail to get halfwidth info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetCenterInfo(io_luaState))
	{
		OutputErrorMessage("fail to get center info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetTriangleNumber(io_luaState))
	{
		OutputErrorMessage("fail to get triangle number info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetTriangleListInfo(io_luaState))
	{
		OutputErrorMessage("fail to get triangle list info");
		wereThereErrors = true;
		goto OnExit;
	}
OnExit:
	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::GetIsCollisionInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* isCollisionString = "isCollision";
	lua_pushstring(&io_luaState, isCollisionString);
	lua_gettable(&io_luaState, -2);
	if (lua_isboolean(&io_luaState, -1))
	{
		//add current is collision bool to buffer
		bool isCollision = lua_toboolean(&io_luaState, -1);
		memcpy(buffer + bufferIndex, &isCollision, sizeof(isCollision));
		bufferIndex += sizeof(isCollision);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("get is collision data error");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetDepthInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get color table
	const char* depthString = "depth";
	lua_pushstring(&io_luaState, depthString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
	{
		//add current depth to buffer
		unsigned int depth = (unsigned int)lua_tonumber(&io_luaState, -1);
		memcpy(buffer + bufferIndex, &depth, sizeof(depth));
		bufferIndex += sizeof(depth);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("get depth number error");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetNodeTrackInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get texture coordinate info
	const char* nodeTrackString = "nodeTrack";
	lua_pushstring(&io_luaState, nodeTrackString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through x,y,z of the center point
		LoopThroughNodeTrackData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("texture data is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoopThroughNodeTrackData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating texure data chunck
	const int nodeElementNum = luaL_len(&io_luaState, -1);//get the number of center point, center point table is at -1 in stack

	int i = 1;
	while (i <= nodeElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each texture data
		if (lua_isstring(&io_luaState, -1))
		{
			//add current texture element to buffer
			unsigned int nodeElement = lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &nodeElement, sizeof(nodeElement));
			bufferIndex += sizeof(nodeElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("node element not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::GetHalfWidthInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get color table
	const char* halfWidthString = "halfWidth";
	lua_pushstring(&io_luaState, halfWidthString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
	{
		//add current half width to buffer
		float halfWidth = (float)lua_tonumber(&io_luaState, -1);
		memcpy(buffer + bufferIndex, &halfWidth, sizeof(halfWidth));
		bufferIndex += sizeof(halfWidth);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("get half width number error");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetCenterInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get texture coordinate info
	const char* centerString = "center";
	lua_pushstring(&io_luaState, centerString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through x,y,z of the center point
		LoopThroughCenterPointData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("texture data is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoopThroughCenterPointData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating texure data chunck
	const int centerElementNum = luaL_len(&io_luaState, -1);//get the number of center point, center point table is at -1 in stack

	int i = 1;
	while (i <= centerElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each texture data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current texture element to buffer
			float centerElement = (float)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &centerElement, sizeof(centerElement));
			bufferIndex += sizeof(centerElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("center x,y,z is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetTriangleNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get color table
	const char* triangleNumString = "triangleNum";
	lua_pushstring(&io_luaState, triangleNumString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
	{
		//add current half width to buffer
		unsigned int triangleNum = (unsigned int)lua_tonumber(&io_luaState, -1);
		memcpy(buffer + bufferIndex, &triangleNum, sizeof(triangleNum));
		bufferIndex += sizeof(triangleNum);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("get triangle number error");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::GetTriangleListInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get texture coordinate info
	const char* triangleListString = "triangleList";
	lua_pushstring(&io_luaState, triangleListString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through x,y,z of the center point
		LoopThroughtTriangleListData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("triangle list data is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoopThroughtTriangleListData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating texure data chunck
	const int triangleListElementNum = luaL_len(&io_luaState, -1);//get the number of center point, center point table is at -1 in stack

	int i = 1;
	while (i <= triangleListElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each texture data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current texture element to buffer
			unsigned int triangleListElement = (unsigned int)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &triangleListElement, sizeof(triangleListElement));
			bufferIndex += sizeof(triangleListElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("triangle list element is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}


bool eae6320::cCollisionDataBuilder::ReadVertexData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* vertexString = "vertices";
	lua_pushstring(&io_luaState, vertexString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{//loop through vertices data chunck
		LoopThroughtVerticesData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("vertices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cCollisionDataBuilder::LoopThroughtVerticesData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2

	unsigned int i = 1;
	while (i <= vertexNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current position number to buffer
			float vertex = (float)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &vertex, sizeof(vertex));
			bufferIndex += sizeof(vertex);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("vertex is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}


bool eae6320::cCollisionDataBuilder::ReadIndexData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* indexString = "indices";
	lua_pushstring(&io_luaState, indexString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		LoopThroughtIndicesData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("vertices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cCollisionDataBuilder::LoopThroughtIndicesData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2

	unsigned int i = 1;
	while (i <= indexNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current position number to buffer
			unsigned int index = (unsigned int)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &index, sizeof(index));
			bufferIndex += sizeof(index);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("index is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}