#ifndef MATERIAL_H
#define	MATERIAL_H

#include <d3d9.h>
#include <d3dx9shader.h>
#include <cassert>
#include <sstream>
#include "../External/Lua/5.2.3/src/lua.hpp"
namespace Material
{
	class cMaterial
	{
		IDirect3DDevice9* pDeviceRef;
		HWND* mainWindowRef;

		bool LoadAsset(const char* i_path);
		bool LoadTableValues(lua_State& io_luaState);
		bool LoadTableValues_constants(lua_State& io_luaState);
		bool LoadTableValues_constants_table(lua_State& io_luaState);
		bool LoadTableValues_constants_values(lua_State& io_luaState);

		bool LoadTableValues_shaders(lua_State& io_luaState);
		bool LoadTableValues_shaders_paths(lua_State& io_luaState);
		bool LoadVertexShader(const char* i_path);
		bool LoadFragmentShader(const char* i_path);
		// compile shader
		bool LoadAndAllocateShaderProgram(const char* i_path, void*& o_compiledShader, std::string* o_errorMessage);

		bool LoadTableValues_textures(lua_State& io_luaState);
		bool LoadTableValues_textures_paths(lua_State& io_luaState);
		bool CreateTexture(const char*i_path, IDirect3DTexture9*& i_ptex);
	public:
		bool useDefualtColor;
		D3DXVECTOR4* pColorModifier;
		IDirect3DVertexShader9* pVertexShader;
		IDirect3DPixelShader9* pFragmentShader;
		IDirect3DTexture9* pTexture;

		ID3DXConstantTable* vertexShaderConstantsTable;
		ID3DXConstantTable* fragmentShaderconstantsTable;
		void SetColor(float i_r, float i_g, float i_b)
		{
			pColorModifier->x = i_r;
			pColorModifier->y = i_g;
			pColorModifier->z = i_b;
		}
		cMaterial(const char* i_texturePath, IDirect3DDevice9* i_direct3DDevice, HWND* i_windowRef)
		{
			mainWindowRef = i_windowRef;
			pDeviceRef = i_direct3DDevice;
			useDefualtColor = false;
			pColorModifier = new D3DXVECTOR4(1.0f, 1.0f, 1.0f,1.0f);
			pVertexShader = NULL;
			pFragmentShader = NULL;
			pTexture = NULL;
			vertexShaderConstantsTable = NULL;
			fragmentShaderconstantsTable = NULL;

			if (!LoadAsset(i_texturePath))
				return;
		}
		~cMaterial()
		{
			pDeviceRef = NULL;

			if (pColorModifier)
			{
				delete pColorModifier;
				pColorModifier = NULL;
			}
			if (pVertexShader)
			{
				pVertexShader->Release();
				pVertexShader = NULL;
			}
			if (pFragmentShader)
			{
				pFragmentShader->Release();
				pFragmentShader = NULL;
			}
			if (vertexShaderConstantsTable)
			{
				vertexShaderConstantsTable->Release();
				vertexShaderConstantsTable = NULL;
			}
			if (fragmentShaderconstantsTable)
			{
				fragmentShaderconstantsTable->Release();
				fragmentShaderconstantsTable = NULL;
			}
			if (pTexture)
			{
				pTexture->Release();
				pTexture = NULL;
			}
		}
	};
}
#endif