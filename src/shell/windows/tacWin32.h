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

// https://stackoverflow.com/questions/61485127/including-windows-h-causes-unknown-attributeno-init-all-error-solved
// ( remove once we migrate out of vs2017 )
#ifndef no_init_all
#define no_init_all
#endif


#include <Windows.h>

namespace Tac
{
  extern HINSTANCE ghInstance;
  extern HINSTANCE ghPrevInstance;
  extern LPSTR     glpCmdLine;
  extern int       gnCmdShow;
  struct String    Win32ErrorToString( DWORD winErrorValue );
  struct String    Win32GetLastErrorString();
  struct String    Win32GetWindowName( HWND hwnd );
  void             Win32Assert( const struct Errors& );
  void             Win32DebugBreak();
  void             Win32PopupBox( const struct StringView& );
  void             Win32Output( const struct StringView& );
}
