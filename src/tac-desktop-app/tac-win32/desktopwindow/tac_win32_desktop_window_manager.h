#pragma once

#include "tac-win32/tac_win32.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-engine-core/platform/tac_platform.h"

namespace Tac
{
  void                Win32WindowManagerInit( Errors& );
  void                Win32WindowManagerPoll( Errors& );
  void                Win32WindowManagerSpawnWindow( const PlatformSpawnWindowParams&, Errors& );
  void                Win32WindowManagerDespawnWindow( WindowHandle );
  //WindowHandle        Win32WindowManagerGetCursorUnobscuredWindow();
  WindowHandle        Win32WindowManagerFindWindow( HWND );
  HWND                Win32WindowManagerGetHWND( WindowHandle );
  void                Win32WindowManagerDebugImGui();
}

