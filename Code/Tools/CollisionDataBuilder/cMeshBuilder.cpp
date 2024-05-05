// Header Files
//=============

#include "cMeshBuilder.h"

#include <cassert>
#include <sstream>
#include <fstream>
// Interface
//==========

// Build
//------

bool eae6320::cMeshBuilder::Build(const std::vector<const std::string>& i_arguments)
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
bool eae6320::cMeshBuilder::LoadMesh(const char*i_path)
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

bool eae6320::cMeshBuilder::ReadMeshFile(lua_State& io_luaState)
{
	std::ofstream outfile(m_path_target, std::ofstream::binary);
	bool wereThereErrors = false;

	ReadVertexNumber(io_luaState);
	ReadIndiceNumber(io_luaState);

	//after read the size of vertices and indices initialize buffer
	bufferIndex = 0;
	unsigned int fileSize = sizeof(vertexNum)+sizeof(indiceNum)//vertex and indice number
		+sizeof(float)* 3 * vertexNum //position
		+ sizeof(float)* 3 * vertexNum//normal
		+ sizeof(float)* 3 * vertexNum //color
		+ sizeof(float)* 2 * vertexNum //texture
		+ sizeof(unsigned int)*indiceNum;
	buffer = new char[fileSize];
	memcpy(buffer + bufferIndex, &vertexNum, sizeof(vertexNum));
	bufferIndex += sizeof(vertexNum);
	memcpy(buffer + bufferIndex, &indiceNum, sizeof(indiceNum));
	bufferIndex += sizeof(indiceNum);

	//read vertices
	if (!ReadVertices(io_luaState))
	{
		wereThereErrors = true;
		goto OnExit;
	}
	if (!ReadIndices(io_luaState))
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

bool eae6320::cMeshBuilder::ReadVertexNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertex number
	const char* vertexNumString = "vertexNum";
	lua_pushstring(&io_luaState, vertexNumString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
		vertexNum = (unsigned int)(lua_tonumber(&io_luaState, -1));
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("a number of vertex is required here!");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::ReadIndiceNumber(lua_State& io_luaState)
{
	bool wereThereErrors = false;

	//read indice number
	const char* indiceNumString = "indiceNum";
	lua_pushstring(&io_luaState, indiceNumString);
	lua_gettable(&io_luaState, -2);
	if (lua_isnumber(&io_luaState, -1))
		indiceNum = (unsigned int)(lua_tonumber(&io_luaState, -1));
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("a number of indice is required here!");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::ReadVertices(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* verticesString = "vertices";
	lua_pushstring(&io_luaState, verticesString);
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
bool eae6320::cMeshBuilder::ReadIndices(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* indicesString = "indices";
	lua_pushstring(&io_luaState, indicesString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{//loop through vertices data chunck
		LoopThroughtIndicesData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("indices data chunck is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::LoopThroughtVerticesData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating vertices data chunck
	const int verticesCount = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack
	if (verticesCount != vertexNum)goto OnExit;

	int i = 1;
	while (i <= verticesCount)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_istable(&io_luaState, -1))
			ReadOneVertexData(io_luaState);
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
bool eae6320::cMeshBuilder::LoopThroughtIndicesData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating indices data chunck
	const int indicesCount = luaL_len(&io_luaState, -1);//get the number of indices, indices table is at -1 in stack
	if (indicesCount != indiceNum)goto OnExit;

	int i = 1;
	while (i <= indicesCount)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current position number to buffer
			unsigned int indice = (unsigned int)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &indice, sizeof(indice));
			bufferIndex += sizeof(indice);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("incice is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::ReadOneVertexData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now the main table is at -3

	//first read position data
	if (!GetPositionInfo(io_luaState))
	{
		OutputErrorMessage("fail to get position info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetNormalInfo(io_luaState))
	{
		OutputErrorMessage("fail to get normal info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetColorInfo(io_luaState))
	{
		OutputErrorMessage("fail to get color info");
		wereThereErrors = true;
		goto OnExit;
	}
	if (!GetTextureInfo(io_luaState))
	{
		OutputErrorMessage("fail to get texture coordinate info");
		wereThereErrors = true;
		goto OnExit;
	}
	
OnExit:
	return !wereThereErrors;
}

bool eae6320::cMeshBuilder::GetPositionInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//read vertices
	const char* positionString = "position";
	lua_pushstring(&io_luaState, positionString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through x,y,z of position
		LoopThroughtPositionData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("position data is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::GetNormalInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get color table
	const char* normalString = "normal";
	lua_pushstring(&io_luaState, normalString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through x,y,z of position
		LoopThroughtNormalData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("normal data is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::GetColorInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get color table
	const char* colorString = "color";
	lua_pushstring(&io_luaState, colorString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through x,y,z of position
		LoopThroughtColorData(io_luaState);
	}
	else
	{
		wereThereErrors = true;
		OutputErrorMessage("color data is not a table");
		goto OnExit;
	}
OnExit:
	lua_pop(&io_luaState, 1);

	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::GetTextureInfo(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//get texture coordinate info
	const char* colorString = "texcoord";
	lua_pushstring(&io_luaState, colorString);
	lua_gettable(&io_luaState, -2);
	if (lua_istable(&io_luaState, -1))
	{
		//loop through u,v of position
		LoopThroughtTextureData(io_luaState);
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
bool eae6320::cMeshBuilder::LoopThroughtPositionData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating vertices data chunck
	const int posElementNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack

	int i = 1;
	while (i <= posElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current position number to buffer
			float positionElement = (float)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &positionElement, sizeof(positionElement));
			bufferIndex += sizeof(positionElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("position x,y,z is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::LoopThroughtNormalData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating vertices data chunck
	const int norElementNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack

	int i = 1;
	while (i <= norElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current position number to buffer
			float normalElement = (float)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &normalElement, sizeof(normalElement));
			bufferIndex += sizeof(normalElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("normal nx,ny,nz is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::LoopThroughtColorData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating vertices data chunck
	const int colElementNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack

	int i = 1;
	while (i <= colElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each vertex data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current position number to buffer
			float colorElement = (float)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &colorElement, sizeof(colorElement));
			bufferIndex += sizeof(colorElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("color r,g,b is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}
bool eae6320::cMeshBuilder::LoopThroughtTextureData(lua_State& io_luaState)
{
	bool wereThereErrors = false;
	//now data table is at -2
	//iterating texure data chunck
	const int texElementNum = luaL_len(&io_luaState, -1);//get the number of vertices, vertices table is at -1 in stack

	int i = 1;
	while (i <= texElementNum)
	{
		lua_pushinteger(&io_luaState, i);
		lua_gettable(&io_luaState, -2);
		//go into each texture data
		if (lua_isnumber(&io_luaState, -1))
		{
			//add current texture element to buffer
			float texElement = (float)lua_tonumber(&io_luaState, -1);
			memcpy(buffer + bufferIndex, &texElement, sizeof(texElement));
			bufferIndex += sizeof(texElement);
		}
		else
		{
			wereThereErrors = true;
			OutputErrorMessage("texture u,v is not a number");
			goto OnExit;
		}
		lua_pop(&io_luaState, 1);
		i++;
	}

OnExit:
	return !wereThereErrors;
}



