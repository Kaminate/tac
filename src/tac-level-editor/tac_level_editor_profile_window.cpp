#include "tac_level_editor_profile_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/world/tac_world.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/window/tac_window_handle.h"

#include "tac-level-editor/tac_level_editor.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{

  const char* CreationProfileWindow::gProfileWindowName { "ProfileWindow" };
  bool        CreationProfileWindow::sShowWindow{};
  void        CreationProfileWindow::Update( , Errors& errors )
  {
    if( !sShowWindow )
      return;

    ImGuiSetNextWindowStretch();
    if( ImGuiBegin( "Profile Window" ) )
    {
      TAC_PROFILE_BLOCK;
      if( ImGuiButton( "Close Window" ) )
        sShowWindow = false;
      ImGuiProfileWidget( keyboardApi );
      ImGuiEnd();
    }
  }


} // namespace Tac

