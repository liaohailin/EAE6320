// Header Files
//=============

#include "cFragmentShaderBuilder.h"

#include <sstream>

// Interface
//==========

// Build
//------

namespace
{
	enum eShaderProgramType
	{
		VERTEX,
		FRAGMENT,

		UNKNOWN
	};
}

bool eae6320::cFragmentShaderBuilder::Build(const std::vector<const std::string>& i_arguments)
{
	// Get which shader program type to compile
	eShaderProgramType shaderProgramType = UNKNOWN;
	{
		if ( i_arguments.size() == 0 )
		{
			shaderProgramType = FRAGMENT;
		}
		else
		{
			OutputErrorMessage(
				"no fragment shader file found",
				m_path_source );
			return false;
		}
	}
	// Get the path to the shader compiler
	std::string path_fxc;
	{
		// Get the path to the DirectX SDK
		std::string path_sdk;
		{
			std::string errorMessage;
			if ( !GetEnvironmentVariable( "DXSDK_DIR", path_sdk, &errorMessage ) )
			{
				std::stringstream decoratedErrorMessage;
				decoratedErrorMessage << "Windows failed to get the path to the DirectX SDK: " << errorMessage;
				OutputErrorMessage( decoratedErrorMessage.str().c_str(), __FILE__ );
				return false;
			}
		}
		path_fxc = path_sdk + "Utilities/bin/" +
#ifndef _WIN64
			"x86"
#else
			"x64"
#endif
			+ "/fxc.exe";
	}
	// Create the command to run
	std::string command;
	{
		std::stringstream commandToBuild;
		commandToBuild << "\"" << path_fxc << "\"";
		// Target profile
		switch ( shaderProgramType )
		{
		case FRAGMENT:
			commandToBuild << " /Tps_3_0";
			break;
		}
		// Entry point
		commandToBuild << " /Emain"
#ifdef EAE6320_GRAPHICS_SHOULDDEBUGSHADERSBEUSED
			// Disable optimizations so that debugging is easier
			<< " /Od"
			// Enable debugging
			<< " /Zi"
#endif
			// Target file
			<< " /Fo\"" << m_path_target << "\""
			// Don't output the logo
			<< " /nologo"
			// Source file
			<< " \"" << m_path_source << "\""
		;
		command = commandToBuild.str();
	}
	// Execute the command
	{
		DWORD exitCode;
		std::string errorMessage;
		if ( ExecuteCommand( command.c_str(), &exitCode, &errorMessage ) )
		{
			return exitCode == EXIT_SUCCESS;
		}
		else
		{
			OutputErrorMessage( errorMessage.c_str(), m_path_source );
			return false;
		}
	}
}
