#pragma once

#include "tac-win32/tac_win32.h"
#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-engine-core/system/tac_platform.h"

namespace Tac
{
  void                Win32WindowManagerInit( Errors& );
  void                Win32WindowManagerPoll( Errors& );
  void                Win32WindowManagerSpawnWindow( const PlatformSpawnWindowParams&, Errors& );
  void                Win32WindowManagerDespawnWindow( const WindowHandle& );
  WindowHandle Win32WindowManagerGetCursorUnobscuredWindow();
  WindowHandle Win32WindowManagerFindWindow( HWND );
  void                Win32WindowManagerDebugImGui();
}

