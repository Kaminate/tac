#pragma once

namespace Tac
{
  void WindowsManagerInit( struct Errors& );
  void WindowsManagerPoll( struct Errors& );
  void WindowsManagerSpawnWindow( const struct DesktopWindowHandle& handle,
                                  int x,
                                  int y,
                                  int width,
                                  int height );
}

