#ifndef EAE6320_C_COLLISIONDATA_BUILDER_H
#define EAE6320_C_COLLISIONDATA_BUILDER_H

// Header Files
//=============

#include "../BuilderHelper/cbBuilder.h"
#include "../../External/Lua/5.2.3/src/lua.hpp"
// Class Declaration
//==================

namespace eae6320
{
	class cCollisionDataBuilder : public cbBuilder
	{
		// Interface
		//==========
		char* buffer;
		unsigned int bufferIndex;
		unsigned int fileSize;
		unsigned int octreeNum;
		unsigned int octreeMaxDepth;
		unsigned int triangleInListNum;
		unsigned int vertexNum;
		unsigned int indexNum;

		bool LoadMesh(const char*i_path);
		bool ReadMeshFile(lua_State& io_luaState);

		bool GetOctreeNodeNumber(lua_State& io_luaState);
		bool LoopThroughtOctreeNodeToGetTriangleNumber(lua_State& io_luaState);
		bool AddToTriangleNumber(lua_State& io_luaState);
		bool GetVertexNumber(lua_State& io_luaState);
		bool GetIndexNumber(lua_State& io_luaState);

		bool ReadOctreeMaxDepth(lua_State& io_luaState);

		bool ReadOctreeNode(lua_State& io_luaState);
		bool LoopThroughtOctreesData(lua_State& io_luaState);
		bool ReadOneOctreeData(lua_State& io_luaState);
		bool GetIsCollisionInfo(lua_State& io_luaState);
		bool GetDepthInfo(lua_State& io_luaState);
		bool GetNodeTrackInfo(lua_State& io_luaState);
		bool LoopThroughNodeTrackData(lua_State& io_luaState);
		bool GetHalfWidthInfo(lua_State& io_luaState);
		bool GetCenterInfo(lua_State& io_luaState);
		bool LoopThroughCenterPointData(lua_State& io_luaState);
		bool GetTriangleNumber(lua_State& io_luaState);
		bool GetTriangleListInfo(lua_State& io_luaState);
		bool LoopThroughtTriangleListData(lua_State& io_luaState);

		bool ReadVertexData(lua_State& io_luaState);
		bool LoopThroughtVerticesData(lua_State& io_luaState);

		bool ReadIndexData(lua_State& io_luaState);
		bool LoopThroughtIndicesData(lua_State& io_luaState);
	public:

		// Build
		//------

		virtual bool Build(const std::vector<const std::string>& i_argument);
	};
}
#endif
