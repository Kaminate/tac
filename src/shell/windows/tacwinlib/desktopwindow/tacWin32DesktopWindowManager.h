#pragma once

#include "src/shell/windows/tacwinlib/tacWin32.h"

namespace Tac
{
  struct Errors;
  void                Win32WindowManagerInit( Errors& );
  void                Win32WindowManagerPoll( Errors& );
  void                Win32WindowManagerSpawnWindow( const DesktopWindowHandle&, int x, int y, int w, int h );
  void                Win32WindowManagerDespawnWindow( const DesktopWindowHandle& );
  DesktopWindowHandle Win32WindowManagerGetCursorUnobscuredWindow();
  DesktopWindowHandle Win32WindowManagerFindWindow( HWND );
}

