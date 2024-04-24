// this whole file sucks and should be deleted
#pragma once

#include "tac-engine-core/window/tac_sim_window_api.h"

namespace Tac
{
  struct StringView;
}

namespace Tac
{
  WindowHandle CreateTrackedWindow( const StringView& path,
                                           int x = 50,
                                           int y = 50,
                                           int w = 800,
                                           int h = 600 );
  WindowHandle CreateTrackedWindow( WindowCreateParams );

  void TrackWindowInit( SimWindowApi* );
  void UpdateTrackedWindows();
  void QuitProgramOnWindowClose( const WindowHandle& );
}
