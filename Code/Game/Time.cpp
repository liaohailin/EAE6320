// Header Files
//=============
#include "precompiled.h"
#include "Time.h"


#include <cassert>
#include <string>
// Static Data Initialization
//===========================

namespace
{
	bool s_isInitialized = false;

	double s_secondsPerCount = 0.0;
	LARGE_INTEGER s_totalCountsElapsed_atInitializion = { 0 };
	LARGE_INTEGER s_totalCountsElapsed_duringRun = { 0 };
	LARGE_INTEGER s_totalCountsElapsed_previousFrame = { 0 };
}

// Helper Function Declarations
//=============================

namespace
{
	bool InitializeIfNecessary();

	std::string GetFormattedWindowsError( const DWORD i_errorCode );
	std::string GetLastWindowsError( DWORD* o_optionalErrorCode = NULL );
}

// Interface
//==========

// Time
//-----

float eae6320::Time::GetTotalSecondsElapsed()
{
	{
		const bool result = InitializeIfNecessary();
		assert( result );
	}

	return static_cast<float>( static_cast<double>( s_totalCountsElapsed_duringRun.QuadPart ) * s_secondsPerCount );
}

float eae6320::Time::GetSecondsElapsedThisFrame()
{
	{
		const bool result = InitializeIfNecessary();
		assert( result );
	}

	return static_cast<float>(
		static_cast<double>( s_totalCountsElapsed_duringRun.QuadPart - s_totalCountsElapsed_previousFrame.QuadPart )
		* s_secondsPerCount );
}

void eae6320::Time::OnNewFrame()
{
	{
		const bool result = InitializeIfNecessary();
		assert( result );
	}

	// Update the previous frame
	{
		s_totalCountsElapsed_previousFrame = s_totalCountsElapsed_duringRun;
	}
	// Update the current frame
	{
		LARGE_INTEGER totalCountsElapsed;
		const BOOL result = QueryPerformanceCounter( &totalCountsElapsed );
		assert( result != FALSE );
		s_totalCountsElapsed_duringRun.QuadPart = totalCountsElapsed.QuadPart - s_totalCountsElapsed_atInitializion.QuadPart;
	}
}

// Initialization / Shut Down
//---------------------------

bool eae6320::Time::Initialize( std::string* o_errorMessage )
{
	bool wereThereErrors = false;

	// Get the frequency of the high-resolution performance counter
	{
		LARGE_INTEGER countsPerSecond;
		if ( QueryPerformanceFrequency( &countsPerSecond ) != FALSE )
		{
			if ( countsPerSecond.QuadPart != 0 )
			{
				s_secondsPerCount = 1.0 / static_cast<double>( countsPerSecond.QuadPart );
			}
			else
			{
				wereThereErrors = true;
				if ( o_errorMessage )
				{
					*o_errorMessage = "This hardware doesn't support high resolution performance counters!";
				}
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			if ( o_errorMessage )
			{
				*o_errorMessage = GetLastWindowsError();
			}
			goto OnExit;
		}
	}
	// Store how many counts have elapsed so far
	if ( QueryPerformanceCounter( &s_totalCountsElapsed_atInitializion ) == FALSE )
	{
		wereThereErrors = true;
		if ( o_errorMessage )
		{
			*o_errorMessage = GetLastWindowsError();
		}
		goto OnExit;
	}

	s_isInitialized = true;

OnExit:

	return !wereThereErrors;
}

// Helper Function Definitions
//============================

namespace
{
	bool InitializeIfNecessary()
	{
		if ( s_isInitialized )
		{
			return true;
		}
		else
		{
			return eae6320::Time::Initialize();
		}	
	}

	std::string GetFormattedWindowsError( const DWORD i_errorCode )
	{
		std::string errorMessage;
		{
			const DWORD formattingOptions =
				// Get the error message from Windows
				FORMAT_MESSAGE_FROM_SYSTEM
				// The space for the error message should be allocated by Windows
				| FORMAT_MESSAGE_ALLOCATE_BUFFER
				// Any potentially deferred inserts should be ignored
				// (i.e. the error message should be in its final form)
				| FORMAT_MESSAGE_IGNORE_INSERTS;
			const void* messageIsFromWindows = NULL;
			const DWORD useTheDefaultLanguage = 0;
			char* messageBuffer = NULL;
			const DWORD minimumCharacterCountToAllocate = 1;
			va_list* insertsAreIgnored = NULL;
			const DWORD storedCharacterCount = FormatMessage( formattingOptions, messageIsFromWindows, i_errorCode,
				useTheDefaultLanguage, reinterpret_cast<LPSTR>( &messageBuffer ), minimumCharacterCountToAllocate, insertsAreIgnored );
			if ( storedCharacterCount != 0 )
			{
				errorMessage = messageBuffer;
			}
			else
			{
				// If there's an error GetLastError() can be called again,
				// but that is too complicated for this program :)
				errorMessage = "Unknown Windows Error";
			}
			// Try to free the memory regardless of whether formatting worked or not,
			// and ignore any error messages
			LocalFree( messageBuffer );
		}
		return errorMessage;
	}

	std::string GetLastWindowsError( DWORD* o_optionalErrorCode )
	{
		const DWORD errorCode = GetLastError();
		if ( o_optionalErrorCode )
		{
			*o_optionalErrorCode = errorCode;
		}
		return GetFormattedWindowsError( errorCode );
	}
}
