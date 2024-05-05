 /*
	The main() function is where the program starts execution
*/

// Header Files
//=============

#include <cstdlib>
#include "AssetBuilder.h"
#include <sstream>
#include "../BuilderHelper/UtilityFunctions.h"

// Entry Point
//============

int main( int i_argumentCount, char** i_arguments )
{
	// The command line should have a path to the list of assets to build
	const unsigned int commandCount = 1;
	const unsigned int actualArgumentCount = i_argumentCount - commandCount;
	if ( actualArgumentCount == 1 )
	{
		const char* path_assetsToBuild = i_arguments[1];
		DWORD errorCode;
		std::string errorMessage;
		if ( eae6320::DoesFileExist( path_assetsToBuild, &errorMessage, &errorCode ) )
		{
			if ( BuildAssets( path_assetsToBuild ) )
			{
				return EXIT_SUCCESS;
			}
			else
			{
				return EXIT_FAILURE;
			}
		}
		else
		{
			if ( ( errorCode == ERROR_FILE_NOT_FOUND ) || ( errorCode == ERROR_PATH_NOT_FOUND ) )
			{
				std::stringstream errorMessage;
				errorMessage << "The path to the list of assets to build provided to AssetBuilder.exe "
					"as argument #1 (\"" << path_assetsToBuild << "\") doesn't exist";
				eae6320::OutputErrorMessage( errorMessage.str().c_str() );
			}
			else
			{
				eae6320::OutputErrorMessage( errorMessage.c_str() );
			}
			return EXIT_FAILURE;
		}
	}
	else
	{
		std::stringstream errorMessage;
		errorMessage << "AssetBuilder.exe must be called with a single command line argument "
			"(instead of " << actualArgumentCount << "), "
			"which must be the path to the list of assets to build";
		eae6320::OutputErrorMessage( errorMessage.str().c_str() );
		return EXIT_FAILURE;
	}
}
