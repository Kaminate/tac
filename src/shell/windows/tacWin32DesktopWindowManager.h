#pragma once

namespace Tac
{
  // this should all be named win32 shit, right?
  void WindowsManagerInit( struct Errors& );
  void WindowsManagerPoll( struct Errors& );
  void WindowsManagerSpawnWindow( const struct DesktopWindowHandle& handle,
                                  int x,
                                  int y,
                                  int width,
                                  int height );
  DesktopWindowHandle GetCursorUnobscuredWindow2();
}

