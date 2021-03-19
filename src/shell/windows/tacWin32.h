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

// https://developercommunity.visualstudio.com/t/vs-2017-1592-reports-unknown-attribute-no-init-all/387702
// if youre getting intellisence error E1097 and want to try to fix it with preprocessor hacks,
// know that it is fixed in vs2019 and will not be backported to vs2017

#include <Windows.h>
//#include <windef.h>

namespace Tac
{
  void             Win32SetStartupParams( HINSTANCE, HINSTANCE, LPSTR, int );
  HINSTANCE        Win32GetStartupInstance();
  HINSTANCE        Win32GetStartupPrevInstance();
  LPSTR            Win32GetStartupCmdLine();
  int              Win32GetStartupCmdShow();
  struct String    Win32ErrorToString( DWORD );
  struct String    Win32GetLastErrorString();
  //struct String    Win32GetWindowName( HWND );
  //void             Win32Assert( const struct Errors& );
  void             Win32DebugBreak();
  //void             Win32PopupBox( const struct StringView& );
  //void             Win32Output( const struct StringView& );
}
