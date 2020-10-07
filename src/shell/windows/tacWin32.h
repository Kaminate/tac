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

//#include "src/common/tacPreprocessor.h"
//#include "src/common/tacString.h"

namespace Tac
{
  extern HINSTANCE ghInstance;
  extern HINSTANCE ghPrevInstance;
  extern LPSTR     glpCmdLine;
  extern int       gnCmdShow;

  //struct Errors;
  struct String;
  struct StringView;
  struct Errors;

  String Win32ErrorToString( DWORD winErrorValue );
  String GetLastWin32ErrorString();
  String GetWin32WindowName( HWND hwnd );

  void WindowsAssert( const Errors& );
  void WindowsDebugBreak();
  void WindowsPopup( const StringView& );
  void WindowsOutput( const StringView& );
}
