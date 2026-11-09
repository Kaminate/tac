#include "tac_level_editor_profile_window.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  static const String sWindowName{ "Profile Window"};
  bool CreationProfileWindow::sShowWindow{};
  void CreationProfileWindow::Update( Errors& )
  {
    if( !Creation::CanShowWindow( sWindowName ) )
      return;
    if( !sShowWindow )
      return;

    ImGuiSetNextWindowStretch();
    if( ImGuiBegin( sWindowName ) )
    {
      TAC_PROFILE_BLOCK;
      if( ImGuiButton( "Close Window" ) )
        sShowWindow = false;
      ImGuiProfileWidget();
      ImGuiEnd();
    }
  }


} // namespace Tac

