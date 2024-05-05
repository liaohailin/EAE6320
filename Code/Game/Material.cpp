#include "precompiled.h"
#include "Material.h"
#include "WindowsProgram.h"

#include <iostream>

namespace Material
{
	bool cMaterial::LoadAsset(const char* i_path)
	{
		bool wereThereErrors = false;

		// Create a new Lua state
		lua_State* luaState = NULL;
		{
			luaState = luaL_newstate();
			if (!luaState)
			{
				wereThereErrors = true;
				std::cerr << "Failed to create a new Lua state\n";
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
				std::cerr << lua_tostring(luaState, -1);
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
						std::cerr << "Asset files must return a table (instead of a " <<
							luaL_typename(luaState, -1) << ")\n";
						// Pop the returned non-table value
						lua_pop(luaState, 1);
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::cerr << "Asset files must return a single table (instead of " <<
						returnedValueCount << " values)\n";
					// Pop every value that was returned
					lua_pop(luaState, returnedValueCount);
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				std::cerr << lua_tostring(luaState, -1);
				// Pop the error message
				lua_pop(luaState, 1);
				goto OnExit;
			}
		}

		// If this code is reached the asset file was loaded successfully,
		// and its table is now at index -1
		if (!LoadTableValues(*luaState))
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
	bool cMaterial::LoadTableValues(lua_State& io_luaState)
	{

		if (!LoadTableValues_constants(io_luaState))
		{
			return false;
		}
		if (!LoadTableValues_shaders(io_luaState))
		{
			return false;
		}
		if (!LoadTableValues_textures(io_luaState))
		{
			return false;
		}
		return true;
	}
	bool cMaterial::LoadTableValues_constants(lua_State& io_luaState)
	{
		bool wereThereErrors = false;

		// Right now the asset table is at -1.
		// After the following table operation it will be at -2
		// and the "textures" table will be at -1:
		const char* key = "constants";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		// It can be hard to remember where the stack is at
		// and how many values to pop.
		// One strategy I would suggest is to always call a new function
		// When you are at a new level:
		// Right now we know that we have an original table at -2,
		// and a new one at -1,
		// and so we _know_ that we always have to pop at least _one_
		// value before leaving this function
		// (to make the original table be back to index -1).
		// If we don't do any further stack manipulation in this function
		// then it becomes easy to remember how many values to pop
		// because it will always be one.
		// This is the strategy I'll take in this example
		// (look at the "OnExit" label):
		if (lua_istable(&io_luaState, -1))
		{
			if (!LoadTableValues_constants_table(io_luaState))
			{
				wereThereErrors = true;
				goto OnExit;
			}
		}
		else
		{
			useDefualtColor = true;
			//wereThereErrors = true;
			//std::cerr << "The value at \"" << key << "\" must be a table "
			//	"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
			//goto OnExit;
		}

	OnExit:

		// Pop the textures table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool cMaterial::LoadTableValues_constants_table(lua_State& io_luaState)
	{
		bool wereThereErrors = false;

		// Right now the asset table is at -1.
		// After the following table operation it will be at -2
		// and the "textures" table will be at -1:
		const char* key = "g_colorModifier";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		// It can be hard to remember where the stack is at
		// and how many values to pop.
		// One strategy I would suggest is to always call a new function
		// When you are at a new level:
		// Right now we know that we have an original table at -2,
		// and a new one at -1,
		// and so we _know_ that we always have to pop at least _one_
		// value before leaving this function
		// (to make the original table be back to index -1).
		// If we don't do any further stack manipulation in this function
		// then it becomes easy to remember how many values to pop
		// because it will always be one.
		// This is the strategy I'll take in this example
		// (look at the "OnExit" label):
		if (lua_istable(&io_luaState, -1))
		{
			if (!LoadTableValues_constants_values(io_luaState))
			{
				wereThereErrors = true;
				goto OnExit;
			}
		}
		else
		{
			useDefualtColor = true;
			//wereThereErrors = true;
			//std::cerr << "The value at \"" << key << "\" must be a table "
			//	"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
			//goto OnExit;
		}

	OnExit:

		// Pop the textures table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}

	bool cMaterial::LoadTableValues_constants_values(lua_State& io_luaState)
	{
		// Right now the asset table is at -2
		// and the textures table is at -1.
		// NOTE, however, that it doesn't matter to me in this function
		// that the asset table is at -2.
		// Because I've carefully called a new function for every "stack level"
		// The only thing I care about is that the textures table that I care about
		// is at the top of the stack.
		// As long as I make sure that when I leave this function it is _still_
		// at -1 then it doesn't matter to me at all what is on the stack below it.
		std::cout << "Iterating through every constant vector value:\n";
		const int textureCount = luaL_len(&io_luaState, -1);
		if ((textureCount % 3) != 0)return false;

		float *valueList = new float[textureCount];
		for (int i = 1; i <= textureCount; ++i)
		{
			lua_pushinteger(&io_luaState, i);
			lua_gettable(&io_luaState, -2);
			valueList[i - 1] = (float)lua_tonumber(&io_luaState, -1);
			std::cout << "\t" << lua_tostring(&io_luaState, -1) << "\n";
			lua_pop(&io_luaState, 1);
		}
		pColorModifier->x = valueList[0];
		pColorModifier->y = valueList[1];
		pColorModifier->z = valueList[2];
		delete valueList;
		valueList = NULL;
		return true;

	}

	bool cMaterial::LoadTableValues_shaders(lua_State& io_luaState)
	{
		bool wereThereErrors = false;

		// Right now the asset table is at -1.
		// After the following table operation it will be at -2
		// and the "textures" table will be at -1:
		const char* key = "shaders";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		// It can be hard to remember where the stack is at
		// and how many values to pop.
		// One strategy I would suggest is to always call a new function
		// When you are at a new level:
		// Right now we know that we have an original table at -2,
		// and a new one at -1,
		// and so we _know_ that we always have to pop at least _one_
		// value before leaving this function
		// (to make the original table be back to index -1).
		// If we don't do any further stack manipulation in this function
		// then it becomes easy to remember how many values to pop
		// because it will always be one.
		// This is the strategy I'll take in this example
		// (look at the "OnExit" label):
		if (lua_istable(&io_luaState, -1))
		{
			if (!LoadTableValues_shaders_paths(io_luaState))
			{
				wereThereErrors = true;
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			std::cerr << "The value at \"" << key << "\" must be a table "
				"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
			goto OnExit;
		}

	OnExit:

		// Pop the textures table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool cMaterial::LoadTableValues_shaders_paths(lua_State& io_luaState)
	{
		// Right now the asset table is at -2
		// and the textures table is at -1.
		// NOTE, however, that it doesn't matter to me in this function
		// that the asset table is at -2.
		// Because I've carefully called a new function for every "stack level"
		// The only thing I care about is that the textures table that I care about
		// is at the top of the stack.
		// As long as I make sure that when I leave this function it is _still_
		// at -1 then it doesn't matter to me at all what is on the stack below it.
		std::string vertexShaderFilePath, fragmentShaderFilePath;
		std::cout << "Iterating through every shader path:\n";
		const int textureCount = luaL_len(&io_luaState, -1);
		for (int i = 1; i <= textureCount; ++i)
		{
			lua_pushinteger(&io_luaState, i);
			lua_gettable(&io_luaState, -2);
			if (i == 1)vertexShaderFilePath = lua_tostring(&io_luaState, -1);
			if (i == 2)fragmentShaderFilePath = lua_tostring(&io_luaState, -1);
			std::cout << "\t" << lua_tostring(&io_luaState, -1) << "\n";
			lua_pop(&io_luaState, 1);
		}
		//load Shaders
		if (!LoadVertexShader(vertexShaderFilePath.c_str()))
		{
			return false;
		}
		if (!LoadFragmentShader(fragmentShaderFilePath.c_str()))
		{
			return false;
		}
		return true;
	}

	bool cMaterial::LoadVertexShader(const char* i_path)
	{
		
			// Load the source code from file and compile it
			void* compiledShader;
			{
				const char* sourceCodeFileName = i_path;
				//const D3DXMACRO* noMacros = NULL;
				//ID3DXInclude* noIncludes = NULL;
				//const char* entryPoint = "main";
				//const char* profile = "vs_3_0";
				//DWORD flags = 0;
				////ID3DXBuffer* errorMessages = NULL;
				//ID3DXConstantTable** noConstants = NULL;
				//#ifdef EAE6320_GRAPHICS_SHOULDDEBUGSHADERSBEUSED
				//flags |=
				//	// Include debug information in shaders
				//	D3DXSHADER_DEBUG |
				//	// Don't do any optimizations to make stepping through the code easier to follow
				//	D3DXSHADER_SKIPOPTIMIZATION;
				//#endif

				std::string* errorMessages = NULL;


				/*HRESULT result = D3DXCompileShaderFromFile( sourceCodeFileName, noMacros, noIncludes, entryPoint, profile, flags,
				&compiledShader, &errorMessages, noConstants );
				*/
				HRESULT result = LoadAndAllocateShaderProgram(sourceCodeFileName, compiledShader, errorMessages);
				if (SUCCEEDED(result))
				{
					/*if ( errorMessages )
					{
					errorMessages->Release();
					}*/
				}
				else
				{
					if (errorMessages)
					{
						std::string errorMessage = std::string("DirectX failed to compiled the vertex shader from the file ") +
							sourceCodeFileName + ":\n" +
							reinterpret_cast<char*>(errorMessages);
						//errorMessages->->Release();
						delete errorMessages;
						return false;
					}
					else
					{
						std::string errorMessage = "DirectX failed to compiled the vertex shader from the file ";
						errorMessage += sourceCodeFileName;
						return false;
					}
				}
			}
			// Create the vertex shader object
			bool wereThereErrors = false;
			{
				HRESULT result = pDeviceRef->CreateVertexShader(reinterpret_cast<DWORD*>(compiledShader),
					&pVertexShader);
				if (FAILED(result))
				{
					MessageBox(*mainWindowRef, "DirectX failed to create the vertex shader", "No Vertex Shader", MB_OK | MB_ICONERROR);
					wereThereErrors = true;
				}
				else
				{
					//if not failling build constant table
					HRESULT constantTableResult = D3DXGetShaderConstantTable(reinterpret_cast<DWORD*>(compiledShader), &vertexShaderConstantsTable);
					if (FAILED(constantTableResult))
					{
						MessageBox(*mainWindowRef, "DirectX failed to create a constant table for vertex shader", "didn't build constant table", MB_OK | MB_ICONERROR);
						wereThereErrors = true;
					}
					
				}
				//delete compiledShader;
			}
			return !wereThereErrors;
	}

	bool cMaterial::LoadFragmentShader(const char* i_path)
	{
			// Load the source code from file and compile it
			void* compiledShader;
			{
				const char* sourceCodeFileName = i_path;
				//const D3DXMACRO* noMacros = NULL;
				//ID3DXInclude* noIncludes = NULL;
				//const char* entryPoint = "main";
				//const char* profile = "ps_3_0";
				//DWORD flags = 0;
				//ID3DXBuffer* errorMessages = NULL;
				//ID3DXConstantTable** noConstants = NULL;
				//#ifdef EAE6320_GRAPHICS_SHOULDDEBUGSHADERSBEUSED
				//flags |=
				//	// Include debug information in shaders
				//	D3DXSHADER_DEBUG |
				//	// Don't do any optimizations to make stepping through the code easier to follow
				//	D3DXSHADER_SKIPOPTIMIZATION;
				//#endif
				//HRESULT result = D3DXCompileShaderFromFile( sourceCodeFileName, noMacros, noIncludes, entryPoint, profile, flags,
				//	&compiledShader, &errorMessages, noConstants );

				std::string* errorMessages = NULL;
				HRESULT result = LoadAndAllocateShaderProgram(sourceCodeFileName, compiledShader, errorMessages);

				if (SUCCEEDED(result))
				{
					/*if ( errorMessages )
					{
					errorMessages->Release();
					}*/
				}
				else
				{
					if (errorMessages)
					{
						std::string errorMessage = std::string("DirectX failed to compiled the fragment shader from the file ") +
							sourceCodeFileName + ":\n" +
							reinterpret_cast<char*>(errorMessages);
						MessageBox(*mainWindowRef, errorMessage.c_str(), "No Fragment Shader", MB_OK | MB_ICONERROR);
						delete errorMessages;
						return false;
					}
					else
					{
						std::string errorMessage = "DirectX failed to compiled the fragment shader from the file ";
						errorMessage += sourceCodeFileName;
						MessageBox(*mainWindowRef, errorMessage.c_str(), "No Fragment Shader", MB_OK | MB_ICONERROR);
						return false;
					}
				}
			}
			// Create the fragment shader object
			bool wereThereErrors = false;
			{
				HRESULT result = pDeviceRef->CreatePixelShader(reinterpret_cast<DWORD*>(compiledShader),
					&pFragmentShader);
				if (FAILED(result))
				{
					MessageBox(*mainWindowRef, "DirectX failed to create the fragment shader", "No Fragment Shader", MB_OK | MB_ICONERROR);
					wereThereErrors = true;
				}
				else
				{
					//if not failling build constant table
					HRESULT constantTableResult = D3DXGetShaderConstantTable(reinterpret_cast<DWORD*>(compiledShader), &fragmentShaderconstantsTable);
					if (FAILED(constantTableResult))
					{
						MessageBox(*mainWindowRef, "DirectX failed to create a constant table", "didn't build constant table", MB_OK | MB_ICONERROR);
						wereThereErrors = true;
					}
					else
					{
						
						if (useDefualtColor)
						{
							fragmentShaderconstantsTable->SetDefaults(pDeviceRef);
						}
						
					}

				}
				//compiledShader->Release();
			}
			return !wereThereErrors;

	}

	bool cMaterial::LoadAndAllocateShaderProgram(const char* i_path, void*& o_compiledShader, std::string* o_errorMessage)
	{
		bool wereThereErrors = false;

		// Load the compiled shader from disk
		o_compiledShader = NULL;
		HANDLE fileHandle = INVALID_HANDLE_VALUE;
		{
			// Open the file
			{
				const DWORD desiredAccess = FILE_GENERIC_READ;
				const DWORD otherProgramsCanStillReadTheFile = FILE_SHARE_READ;
				SECURITY_ATTRIBUTES* useDefaultSecurity = NULL;
				const DWORD onlySucceedIfFileExists = OPEN_EXISTING;
				const DWORD useDefaultAttributes = FILE_ATTRIBUTE_NORMAL;
				const HANDLE dontUseTemplateFile = NULL;
				fileHandle = CreateFile(i_path, desiredAccess, otherProgramsCanStillReadTheFile,
					useDefaultSecurity, onlySucceedIfFileExists, useDefaultAttributes, dontUseTemplateFile);
				if (fileHandle == INVALID_HANDLE_VALUE)
				{
					wereThereErrors = true;
					if (o_errorMessage)
					{
						std::stringstream errorMessage;
						errorMessage << "Windows failed to open the shader file: " << GetLastWindowsError();
						*o_errorMessage = errorMessage.str();
					}
					goto OnExit;
				}
			}
			// Get the file's size
			size_t fileSize;
			{
				LARGE_INTEGER fileSize_integer;
				if (GetFileSizeEx(fileHandle, &fileSize_integer) != FALSE)
				{
					// This is unsafe if the file's size is bigger than a size_t
					fileSize = static_cast<size_t>(fileSize_integer.QuadPart);
				}
				else
				{
					wereThereErrors = true;
					if (o_errorMessage)
					{
						std::stringstream errorMessage;
						errorMessage << "Windows failed to get the size of shader: " << GetLastWindowsError();
						*o_errorMessage = errorMessage.str();
					}
					goto OnExit;
				}
			}
			// Read the file's contents into temporary memory
			o_compiledShader = malloc(fileSize);
			if (o_compiledShader)
			{
				DWORD bytesReadCount;
				OVERLAPPED* readSynchronously = NULL;
				if (ReadFile(fileHandle, o_compiledShader, fileSize,
					&bytesReadCount, readSynchronously) == FALSE)
				{
					wereThereErrors = true;
					if (o_errorMessage)
					{
						std::stringstream errorMessage;
						errorMessage << "Windows failed to read the contents of shader: " << GetLastWindowsError();
						*o_errorMessage = errorMessage.str();
					}
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				if (o_errorMessage)
				{
					std::stringstream errorMessage;
					errorMessage << "Failed to allocate " << fileSize << " bytes to read in the shader program " << i_path;
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}

	OnExit:

		if (wereThereErrors && o_compiledShader)
		{
			free(o_compiledShader);
			o_compiledShader = NULL;
		}
		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			if (CloseHandle(fileHandle) == FALSE)
			{
				if (!wereThereErrors && o_errorMessage)
				{
					std::stringstream errorMessage;
					errorMessage << "Windows failed to close the shader file handle: " << GetLastWindowsError();
					*o_errorMessage = errorMessage.str();
				}
				wereThereErrors = true;
			}
			fileHandle = INVALID_HANDLE_VALUE;
		}

		return !wereThereErrors;
	}

	bool cMaterial::LoadTableValues_textures(lua_State& io_luaState)
	{
		bool wereThereErrors = false;

		const char* key = "textures";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);

		if (lua_istable(&io_luaState, -1))
		{
			if (!LoadTableValues_textures_paths(io_luaState))
			{
				wereThereErrors = true;
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			std::cerr << "The value at \"" << key << "\" must be a table "
				"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
			goto OnExit;
		}

	OnExit:

		// Pop the textures table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool cMaterial::LoadTableValues_textures_paths(lua_State& io_luaState)
	{
		std::string texturePath;
		std::cout << "Iterating through every texture path:\n";
		const int textureCount = luaL_len(&io_luaState, -1);
		for (int i = 1; i <= textureCount; ++i)
		{
			lua_pushinteger(&io_luaState, i);
			lua_gettable(&io_luaState, -2);
			if (i == 1)texturePath = lua_tostring(&io_luaState, -1);
			std::cout << "\t" << lua_tostring(&io_luaState, -1) << "\n";
			lua_pop(&io_luaState, 1);
		}
		//load Shaders
		if (!CreateTexture(texturePath.c_str(), pTexture))
		{
			return false;
		}
		return true;
	}
	bool cMaterial::CreateTexture(const char*i_path, IDirect3DTexture9*& i_ptex)
	{
		HRESULT result = D3DXCreateTextureFromFile(pDeviceRef, i_path, &i_ptex);
		if (SUCCEEDED(result))
			return true;
		else return false;
	}
}