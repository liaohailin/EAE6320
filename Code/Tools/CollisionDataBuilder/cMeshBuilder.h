#ifndef EAE6320_C_MESH_BUILDER_H
#define EAE6320_C_MESH_BUILDER_H

// Header Files
//=============

#include "../BuilderHelper/cbBuilder.h"
#include "../../External/Lua/5.2.3/src/lua.hpp"
// Class Declaration
//==================

namespace eae6320
{
	class cMeshBuilder : public cbBuilder
	{
		// Interface
		//==========
		char* buffer;
		unsigned int bufferIndex;
		unsigned int vertexNum;
		unsigned int indiceNum;

		bool LoadMesh(const char*i_path);
		bool ReadMeshFile(lua_State& io_luaState);
		bool ReadVertexNumber(lua_State& io_luaState);
		bool ReadIndiceNumber(lua_State& io_luaState);
		bool ReadVertices(lua_State& io_luaState);
		bool ReadIndices(lua_State& io_luaState);
		bool LoopThroughtVerticesData(lua_State& io_luaState);
		bool LoopThroughtIndicesData(lua_State& io_luaState);
		bool ReadOneVertexData(lua_State& io_luaState);
		bool GetPositionInfo(lua_State& io_luaState);
		bool GetNormalInfo(lua_State& io_luaState);
		bool GetColorInfo(lua_State& io_luaState);
		bool GetTextureInfo(lua_State& io_luaState);
		bool LoopThroughtPositionData(lua_State& io_luaState);
		bool LoopThroughtNormalData(lua_State& io_luaState);
		bool LoopThroughtColorData(lua_State& io_luaState);
		bool LoopThroughtTextureData(lua_State& io_luaState);
	public:

		// Build
		//------
		
		virtual bool Build(const std::vector<const std::string>& i_argument);
	};
}
#endif
