#pragma once

#include "src/shell/windows/tac_win32.h"
#include "src/common/tac_core.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_platform.h"

namespace Tac
{
  void                Win32WindowManagerInit( Errors& );
  void                Win32WindowManagerPoll( Errors& );
  void                Win32WindowManagerSpawnWindow( const PlatformSpawnWindowParams&, Errors& );
  void                Win32WindowManagerDespawnWindow( const DesktopWindowHandle& );
  DesktopWindowHandle Win32WindowManagerGetCursorUnobscuredWindow();
  DesktopWindowHandle Win32WindowManagerFindWindow( HWND );
  void                Win32WindowManagerDebugImGui();
}

