#pragma once
namespace Tac
{
  struct DesktopWindowHandle;
  struct StringView;
  DesktopWindowHandle CreateTrackedWindow( const StringView& path,
                                           int x = 50,
                                           int y = 50,
                                           int w = 800,
                                           int h = 600 );
  void UpdateTrackedWindows();
}