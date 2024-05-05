/*
	This class defines a Maya file translator
*/

#ifndef EAE6320_CMAYAEXPORTER_SCENEOCTREECOLLISION_H
#define EAE6320_CMAYAEXPORTER_SCENEOCTREECOLLISION_H

// Header Files
//=============

#include <maya/MPxFileTranslator.h>
#include <vector>

// Forward Declarations
//=====================

namespace
{
	struct sVertex;
}

// Class Definition
//=================

namespace eae6320
{
	class cMayaExporterSceneOctreeCollisionData : public MPxFileTranslator
	{
		// Inherited Interface
		//====================
		
	public:
		// The writer method is what exports the file
		virtual bool haveWriteMethod() const { return true; }
		virtual MStatus writer( const MFileObject& i_file, const MString& i_options, FileAccessMode i_mode );

		// You can choose what the default file extension of an exported mesh is
		virtual MString defaultExtension() const { return "col"; }

		// Interface
		//==========

	public:

		// This function is used by Maya to create an instance of the exporter (see registerFileTranslator() in EntryPoint.cpp)
		static void* Create()
		{
			return new cMayaExporterSceneOctreeCollisionData;
		}
	};
}

#endif	// EAE6320_CMAYAEXPORTER_H
