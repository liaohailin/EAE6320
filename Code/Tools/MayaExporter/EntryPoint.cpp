/*
	The main() function in this file is the entry point of this program
*/

// Header Files
//=============

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include "cMayaExporter.h"

// Static Data Initialization
//===========================

namespace
{
	// This will be displayed in Maya's dropdown list of available export formats.
	// You should substitute EAE6320_TODO with your name: LastName_FirstName
	const char* s_pluginName = "liao_hailin's EAE6320 Mesh Format";
}

// Entry Point
//============

__declspec(dllexport) MStatus initializePlugin( MObject io_object )
{
	// Create a plugin function set
	MFnPlugin plugin( io_object );

	// Register the exporter
	MStatus status;
	{
		char* noIcon = "none";
		status = plugin.registerFileTranslator( s_pluginName, noIcon, eae6320::cMayaExporter::Create );
		if ( !status )
		{
			MGlobal::displayError( MString( "Failed to register exporter: " ) + status.errorString() );
		}
	}
    return status;
}

__declspec(dllexport) MStatus uninitializePlugin( MObject io_object )
{
	// Create a plugin function set
	MFnPlugin plugin( io_object );

	// Register the exporter
	MStatus status;
	{
		status = plugin.deregisterFileTranslator( s_pluginName );
		if ( !status )
		{
			MGlobal::displayError( MString( "Failed to deregister exporter: " ) + status.errorString() );
		}
	}
    return status;
}
