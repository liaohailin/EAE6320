// Header Files
//=============

#include "cGenericBuilder.h"

#include <iostream>
#include <sstream>

// Interface
//==========

// Build
//------

bool eae6320::cGenericBuilder::Build( const std::vector<const std::string>& )
{
	bool wereThereErrors = false;

	// Copy the source to the target
	{
		const bool dontFailIfTargetAlreadyExists = false;
		std::string errorMessage;
		if (!eae6320::CopyFile(m_path_source, m_path_target, dontFailIfTargetAlreadyExists, &errorMessage))
		{
			wereThereErrors = true;
			std::stringstream decoratedErrorMessage;
			decoratedErrorMessage << "Windows failed to copy \"" << m_path_source << "\" to \"" << m_path_target << "\": " << errorMessage;
			eae6320::OutputErrorMessage( decoratedErrorMessage.str().c_str(), __FILE__ );
		}
	}
	
	return !wereThereErrors;
}
