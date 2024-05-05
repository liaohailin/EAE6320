/*
	This file contains the function declarations for graphics
*/

#ifndef EAE6320_GRAPHICS_H
#define EAE6320_GRAPHICS_H

// Header Files
//=============

#include "precompiled.h"
#include "Arcball.h"
// Interface
//==========

bool Initialize( const HWND i_mainWindow );
void Render();
bool ShutDown();
ArcBall::cArcBall* GetArcBall();
#endif	// EAE6320_GRAPHICS_H
