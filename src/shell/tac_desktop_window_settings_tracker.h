#pragma once

#include "src/common/tac_common.h"

namespace Tac
{
  DesktopWindowHandle CreateTrackedWindow( const StringView& path,
                                           int x = 50,
                                           int y = 50,
                                           int w = 800,
                                           int h = 600 );
  void UpdateTrackedWindows();
  void QuitProgramOnWindowClose( const DesktopWindowHandle& );
}
