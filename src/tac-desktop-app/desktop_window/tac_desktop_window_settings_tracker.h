// this whole file sucks and should be deleted
#pragma once

#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  WindowHandle CreateTrackedWindow( StringView path,
                                           int x = 50,
                                           int y = 50,
                                           int w = 800,
                                           int h = 600 );
  WindowHandle CreateTrackedWindow( WindowCreateParams );

  void TrackWindowInit( SimWindowApi*, SettingsNode );
  void UpdateTrackedWindows();
  void QuitProgramOnWindowClose( const WindowHandle& );
}
