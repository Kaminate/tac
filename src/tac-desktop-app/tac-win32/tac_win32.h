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

//#include "tac-std-lib/tac_core.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <Windowsx.h>
#undef CreateWindow

namespace Tac { struct Errors; struct String; }

namespace Tac
{
  void Win32SetStartupParams( HINSTANCE, HINSTANCE, LPSTR, int );
  auto Win32GetStartupInstance() -> HINSTANCE;
  auto Win32GetStartupPrevInstance() -> HINSTANCE;
  auto Win32GetStartupCmdLine() -> LPSTR;
  auto Win32GetStartupCmdShow() -> int;
  auto Win32ErrorStringFromDWORD( DWORD ) -> String;

  //   This function is deleted because any api functions that returns an HRESULT
  //   also specify the possible HRESULT values/reasons.
  auto Win32ErrorStringFromHRESULT( HRESULT ) -> String = delete;

  auto Win32GetLastErrorString() -> String;
  void Win32DebugBreak();
  void Win32OSInit();

  void HrCallAux( const HRESULT, const char*, Errors& );
}

#define TAC_HR_CALL( fn ) {                                                                        \
  const HRESULT hr { fn };                                                                         \
  const bool failed { FAILED( hr ) };                                                              \
  if( failed )                                                                                     \
  {                                                                                                \
    TAC_CALL( Tac::HrCallAux( hr, #fn, errors ) );                                                 \
  }                                                                                                \
}

#define TAC_HR_CALL_RET( fn ) {                                                                    \
  const HRESULT hr { fn };                                                                         \
  const bool failed { FAILED( hr ) };                                                              \
  if( failed )                                                                                     \
  {                                                                                                \
    TAC_CALL_RET( Tac::HrCallAux( hr, #fn, errors ) );                                             \
  }                                                                                                \
}

