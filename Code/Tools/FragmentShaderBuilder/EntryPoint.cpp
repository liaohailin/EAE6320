/*
	The main() function is where the program starts execution
*/

// Header Files
//=============

#include "cFragmentShaderBuilder.h"

// Entry Point
//============

int main( int i_argumentCount, char** i_arguments )
{
	return eae6320::Build<eae6320::cFragmentShaderBuilder>(i_arguments, i_argumentCount);
}
