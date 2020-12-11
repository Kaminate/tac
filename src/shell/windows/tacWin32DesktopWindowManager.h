#pragma once

namespace Tac
{
  void Win32WindowManagerInit( struct Errors& );
  void Win32WindowManagerPoll( struct Errors& );
  void Win32WindowManagerSpawnWindow( const struct DesktopWindowHandle& handle,
                                  int x,
                                  int y,
                                  int width,
                                  int height );
  DesktopWindowHandle Win32WindowManagerGetCursorUnobscuredWindow();
}

