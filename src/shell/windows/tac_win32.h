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

#include "src/common/tac_common.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <Windowsx.h>

namespace Tac
{
  void             Win32SetStartupParams( HINSTANCE, HINSTANCE, LPSTR, int );
  HINSTANCE        Win32GetStartupInstance();
  HINSTANCE        Win32GetStartupPrevInstance();
  LPSTR            Win32GetStartupCmdLine();
  int              Win32GetStartupCmdShow();
  String           Win32ErrorStringFromDWORD( DWORD );

  //               This function is deleted because any api functions that returns an HRESULT
  //               also specify the possible HRESULT values/reasons.
  String           Win32ErrorStringFromHRESULT( HRESULT ) = delete;

  String           Win32GetLastErrorString();
  //struct String    Win32GetWindowName( HWND );
  //void             Win32Assert( const struct Errors& );
  void             Win32DebugBreak();
  void             Win32OSInit();
  //void             Win32PopupBox( const struct StringView& );
  //void             Win32Output( const struct StringView& );

  const char* HrCallAux( const HRESULT, const char*, const char* );
}

#define TAC_HR_CALL( errors, fn, ... ) {                         \
  const HRESULT hr = fn( __VA_ARGS__ );                          \
  if( FAILED( hr ) )                                             \
  {                                                              \
    const char* msg = Tac::HrCallAux(hr, #fn, #__VA_ARGS__);     \
    TAC_RAISE_ERROR_RETURN( msg, errors, {} );                   \
  }                                                              \
}

#define TAC_HR_CALL_NO_RET( errors, fn, ... ) {                  \
  const HRESULT hr = fn( __VA_ARGS__ );                          \
  if( FAILED( hr ) )                                             \
  {                                                              \
    const char* msg = Tac::HrCallAux(hr, #fn, #__VA_ARGS__);     \
    TAC_RAISE_ERROR( msg, errors );                              \
  }                                                              \
}
