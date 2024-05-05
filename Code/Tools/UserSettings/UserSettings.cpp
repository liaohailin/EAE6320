// Header Files
//=============

#include "UserSettings.h"

#include <cmath>
#include <sstream>
#include <string>
#include "../../External/Lua/Includes.h"

// Static Data Initialization
//===========================

namespace
{
	unsigned int s_height = 480;
	bool s_isFullScreenModeEnabled = false;
	unsigned int s_width = 640;

	const char* s_userSettingsfileName = "settings.ini";
}

// Helper Function Declarations
//=============================

namespace
{
	bool InitializeIfNecessary();
	bool IsNumberAnInteger( const lua_Number i_number );
	bool LoadUserSettingsIntoLuaTable( lua_State& io_luaState, bool& o_doesUserSettingsFileExist );
	bool PopulateUserSettingsFromLuaTable( lua_State& io_luaState );


	bool GetInitFileInfoWidth(lua_State& io_luaState);
	bool GetInitFileInfoHeight(lua_State& io_luaState);
	bool GetInitFileInfoIsFullScreen(lua_State& io_luaState);
}

// Interface
//==========

unsigned int eae6320::UserSettings::GetHeight()
{
	InitializeIfNecessary();
	return s_height;
}

unsigned int eae6320::UserSettings::GetWidth()
{
	InitializeIfNecessary();
	return s_width;
}

bool eae6320::UserSettings::IsFullScreenModeEnabled()
{
	InitializeIfNecessary();
	return s_isFullScreenModeEnabled;
}

// Helper Function Definitions
//============================

namespace
{
	bool InitializeIfNecessary()
	{
		static bool isInitialized = false;
		if ( isInitialized )
		{
			return true;
		}

		bool wereThereErrors = false;

		// Create a new Lua state
		lua_State* luaState = NULL;
		bool wasUserSettingsEnvironmentCreated = false;
		{
			luaState = luaL_newstate();
			if ( luaState == NULL )
			{
				MessageBox( NULL, "Failed to create a new Lua state", "No User Settings", MB_OK | MB_ICONERROR );
				wereThereErrors = true;
				goto CleanUp;
			}
		}
		// Create an empty table to be used as the Lua environment for the user settings
		{
			lua_newtable( luaState );
			wasUserSettingsEnvironmentCreated = true;
		}
		// Load the user settings
		bool doesUserSettingsFileExist;
		if ( LoadUserSettingsIntoLuaTable( *luaState, doesUserSettingsFileExist ) )
		{
			if ( doesUserSettingsFileExist )
			{
				// Populate the user settings in C from the user settings in the Lua environment
				if ( !PopulateUserSettingsFromLuaTable( *luaState ) )
				{
					wereThereErrors = true;
					goto CleanUp;
				}
			}
		}
		else
		{
			wereThereErrors = true;
			goto CleanUp;
		}

		// Free the Lua environment

	CleanUp:

		if ( luaState )
		{
			if ( wasUserSettingsEnvironmentCreated )
			{
				lua_pop( luaState, 1 );
			}
			// Sanity Check
			{
				int stackItemCount = lua_gettop( luaState );
				if ( stackItemCount != 0 )
				{
					std::ostringstream errorMessage;
					errorMessage << "There are still " << stackItemCount
						<< " items in the Lua stack when the Lua state is being closed";
					MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
					wereThereErrors = true;
				}
			}
			lua_close( luaState );
		}

		isInitialized = !wereThereErrors;
		return isInitialized;
	}

	bool IsNumberAnInteger( const lua_Number i_number )
	{
		lua_Number integralPart;
		lua_Number fractionalPart = modf( i_number, &integralPart );
		return integralPart == i_number;
	}

	bool LoadUserSettingsIntoLuaTable( lua_State& io_luaState, bool& o_doesUserSettingsFileExist )
	{
		// Load the file into a Lua function
		int result = luaL_loadfile( &io_luaState, s_userSettingsfileName );
		if ( result == LUA_OK )
		{
			o_doesUserSettingsFileExist = true;

			// Set the Lua function's environment
			{
				lua_pushvalue( &io_luaState, -2 );
				const char* upValueName = lua_setupvalue( &io_luaState, -2, 1 );
				if ( upValueName == NULL )
				{
					std::stringstream errorMessage;
					errorMessage << "Internal error setting the Lua environment for " << s_userSettingsfileName << "... This should never happen";
					MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
					lua_pop( &io_luaState, 2 );
					return false;
				}
			}
			// Call the Lua function
			{
				int noArguments = 0;
				int noReturnValues = 0;
				int noErrorMessageHandler = 0;
				result = lua_pcall( &io_luaState, noArguments, noReturnValues, noErrorMessageHandler );
				if ( result == LUA_OK )
				{
					return true;
				}
				else
				{
					std::string luaErrorMessage( lua_tostring( &io_luaState, -1 ) );
					lua_pop( &io_luaState, 1 );

					if ( result == LUA_ERRRUN )
					{
						std::stringstream errorMessage;
						errorMessage << "Error in " << s_userSettingsfileName << ": " << luaErrorMessage;
						MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
					}
					else
					{
						std::stringstream errorMessage;
						errorMessage << "Error processing user settings file: " << luaErrorMessage;
						MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
					}

					return false;
				}
			}
		}
		else
		{
			o_doesUserSettingsFileExist = false;

			std::string luaErrorMessage( lua_tostring( &io_luaState, -1 ) );
			lua_pop( &io_luaState, 1 );

			if ( result == LUA_ERRFILE )
			{
				// If loading the file failed because the file doesn't exist it's ok
				if ( GetFileAttributes( s_userSettingsfileName ) == INVALID_FILE_ATTRIBUTES )
				{
					return true;
				}
				else
				{
					std::stringstream errorMessage;
					errorMessage << "User settings file " << s_userSettingsfileName <<
						" exists but couldn't be opened or read: " << luaErrorMessage;
					MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
				}
			}
			else if ( result == LUA_ERRSYNTAX )
			{
				std::stringstream errorMessage;
				errorMessage << "Syntax error in " << s_userSettingsfileName << ": " << luaErrorMessage;
				MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
			}
			else
			{
				std::stringstream errorMessage;
				errorMessage << "Error loading user settings file: " << luaErrorMessage;
				MessageBox( NULL, errorMessage.str().c_str(), "No User Settings", MB_OK | MB_ICONERROR );
			}

			return false;
		}
	}

	bool PopulateUserSettingsFromLuaTable( lua_State& io_luaState )
	{
		//EAE6320_TODO
		//get width
		if (!GetInitFileInfoWidth(io_luaState))
			return false;
		if (!GetInitFileInfoHeight(io_luaState))
			return false;
		if (!GetInitFileInfoIsFullScreen(io_luaState))
			return false;

		return true;
		
	}
	bool GetInitFileInfoWidth(lua_State& io_luaState)
	{
		bool wereThereErrors = false;
		//read vertex number
		const char* widthString = "width";
		lua_pushstring(&io_luaState, widthString);
		lua_gettable(&io_luaState, -2);
		if (lua_isnumber(&io_luaState, -1))
			s_width = (unsigned int)(lua_tonumber(&io_luaState, -1));
		else
		{
			wereThereErrors = true;
			goto OnExit;
		}
	OnExit:
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool GetInitFileInfoHeight(lua_State& io_luaState)
	{
		bool wereThereErrors = false;
		//read vertex number
		const char* heightString = "height";
		lua_pushstring(&io_luaState, heightString);
		lua_gettable(&io_luaState, -2);
		if (lua_isnumber(&io_luaState, -1))
			s_height = (unsigned int)(lua_tonumber(&io_luaState, -1));
		else
		{
			wereThereErrors = true;
			goto OnExit;
		}
	OnExit:
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
	bool GetInitFileInfoIsFullScreen(lua_State& io_luaState)
	{
		bool wereThereErrors = false;
		//read vertex number
		const char* fulscreenString = "fullscreen";
		
		lua_pushstring(&io_luaState, fulscreenString);
		lua_gettable(&io_luaState, -2);
		if (lua_isboolean(&io_luaState, -1))
		{
			//const char* temp = (lua_tostring(&io_luaState, -1));
			//if (temp == "f")s_isFullScreenModeEnabled = false;
			//else s_isFullScreenModeEnabled = true;
			s_isFullScreenModeEnabled = (bool)lua_toboolean(&io_luaState, -1);
		}
		else
		{
			wereThereErrors = true;
			goto OnExit;
		}
		
	OnExit:
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
}
