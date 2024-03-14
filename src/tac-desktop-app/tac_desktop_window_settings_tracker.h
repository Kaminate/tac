#pragma once

namespace Tac
{
  struct StringView;
  struct DesktopAppCreateWindowParams;
  struct DesktopWindowHandle;
}

namespace Tac
{
  DesktopWindowHandle CreateTrackedWindow( const StringView& path,
                                           int x = 50,
                                           int y = 50,
                                           int w = 800,
                                           int h = 600 );
  DesktopWindowHandle CreateTrackedWindow( const DesktopAppCreateWindowParams& );

  void UpdateTrackedWindows();
  void QuitProgramOnWindowClose( const DesktopWindowHandle& );
}
