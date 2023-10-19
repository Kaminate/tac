#pragma once

#include "src/shell/windows/tac_win32.h"
#include "src/common/tac_common.h"

namespace Tac
{
  void                Win32WindowManagerInit( Errors& );
  void                Win32WindowManagerPoll( Errors& );
  void                Win32WindowManagerSpawnWindow( const DesktopWindowHandle&,
                                                     const char* name,
                                                     int x,
                                                     int y,
                                                     int w,
                                                     int h );
  void                Win32WindowManagerDespawnWindow( const DesktopWindowHandle& );
  DesktopWindowHandle Win32WindowManagerGetCursorUnobscuredWindow();
  DesktopWindowHandle Win32WindowManagerFindWindow( HWND );
}

