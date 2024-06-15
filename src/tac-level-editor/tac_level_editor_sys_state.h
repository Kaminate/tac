#pragma once

#include "tac-level-editor/tac_level_editor_icon_renderer.h"
#include "tac-level-editor/tac_level_editor_widget_renderer.h"

namespace Tac
{

  struct CreationSysState
  {
    void Init( IconRenderer*,
               WidgetRenderer*,

               Errors& );
    void Uninit();

    IconRenderer*     mIconRenderer             {};

    WidgetRenderer*   mWidgetRenderer           {};
  };

} // namespace Tac

