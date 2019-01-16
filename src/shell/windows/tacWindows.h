// This file is the only place where Tac includes windows.h
// It contains the windows defines ( ie: WIN32_LEAN_AND_MEAN ),
// some windows helper functions, and implementations of
// functions that atm don't yet exist in the std library,
// to be hooked into the shell
//
// ^ jk, opengl stuff includes windows.h too, and it does so
// inside an extern c block. Replacing their include with
// this file causes wacky errors

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "common/tacPreprocessor.h"
#include "common/tacString.h"

struct TacErrors;

TacString TacWin32ErrorToString( DWORD winErrorValue );
TacString TacGetLastWin32ErrorString();

void TacWindowsAssert( const TacErrors& errors );
void TacWindowsDebugBreak();
void TacWindowsPopup( TacString );
void TacWindowsOutput( TacString );


