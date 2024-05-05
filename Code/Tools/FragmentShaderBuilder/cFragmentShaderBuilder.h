#ifndef EAE6320_C_FRAGMENT_SHADER_BUILDER_H
#define EAE6320_C_FRAGMENT_SHADER_BUILDER_H

// Header Files
//=============

#include "../BuilderHelper/cbBuilder.h"

// Class Declaration
//==================

namespace eae6320
{
	class cFragmentShaderBuilder : public cbBuilder
	{
		// Interface
		//==========

	public:

		// Build
		//------

		virtual bool Build(const std::vector<const std::string>&);
	};
}
#endif
