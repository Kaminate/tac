#pragma once

namespace Tac
{
  void Win32WindowManagerInit( struct Errors& );
  void Win32WindowManagerPoll( struct Errors& );
  void Win32WindowManagerSpawnWindow( const struct DesktopWindowHandle&, int x, int y, int w, int h );
  DesktopWindowHandle Win32WindowManagerGetCursorUnobscuredWindow();
  DesktopWindowHandle Win32WindowManagerFindWindow( HWND );
}

