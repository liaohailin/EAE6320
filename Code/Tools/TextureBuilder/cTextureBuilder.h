#ifndef EAE6320_C_TEXTURE_BUILDER_H
#define EAE6320_C_TEXTURE_BUILDER_H

#include "../BuilderHelper/cbBuilder.h"

namespace eae6320
{
	class cTextureBuilder : public cbBuilder
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