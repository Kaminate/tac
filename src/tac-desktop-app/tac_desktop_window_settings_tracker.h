#pragma once

namespace Tac
{
  struct StringView;
  struct DesktopAppCreateWindowParams;
  struct WindowHandle;
}

namespace Tac
{
  WindowHandle CreateTrackedWindow( const StringView& path,
                                           int x = 50,
                                           int y = 50,
                                           int w = 800,
                                           int h = 600 );
  WindowHandle CreateTrackedWindow( const DesktopAppCreateWindowParams& );

  void UpdateTrackedWindows();
  void QuitProgramOnWindowClose( const WindowHandle& );
}
