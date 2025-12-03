#include "tac_level_editor_main_window.h" // self-inc
#include "tac_level_editor_asset_view.h"
#include "tac_level_editor_system_window.h"
#include "tac_level_editor_game_window.h"
#include "tac_level_editor_property_window.h"
#include "tac_level_editor_profile_window.h"

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"

#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-level-editor/tac_level_editor_shader_graph_window.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  static void ImGuiSaveEntityAs(Entity* entity, Errors& errors)
  {
    if( entity->mParent )
      return; // why?

    TAC_CALL( const AssetPathStringView assetPath{ AssetSaveDialog(
      AssetSaveDialogParams 
      {
        .mSuggestedFilename { entity->mName + ".prefab" },
      }, errors ) } );
    if( assetPath.empty() )
      return;

    const Json entityJson { entity->Save() };
    const String prefabJsonString { entityJson.Stringify() };
    const void* bytes { prefabJsonString.data() };
    const int byteCount{ prefabJsonString.size() };
    TAC_CALL( SaveToFile( assetPath, bytes, byteCount, errors ) );
  }

  static void ImGuiSaveAs()
  {
    static Errors saveErrors;
    Errors& errors = saveErrors;

    if( saveErrors )
      ImGuiText( saveErrors.ToString() );

    if( !ImGuiButton( "save as" ) )
      return;

    World* world{ Creation::GetWorld() };
    for( Entity* entity : world->mEntities )
    {
      TAC_CALL( ImGuiSaveEntityAs( entity, errors ) );
    }
  }

  // -----------------------------------------------------------------------------------------------

  bool CreationMainWindow::sShowWindow{};

  void CreationMainWindow::Update(  Errors& errors )
  {
    if( !sShowWindow )
      return;

    TAC_PROFILE_BLOCK;

    ImGuiSetNextWindowStretch();
    if( !ImGuiBegin( "Main Window", &sShowWindow ) )
      return;

    ImGuiSaveAs();

    ImGuiText( "Windows" );
    ImGuiIndent();
    ImGuiCheckbox( "Show System Window", &CreationSystemWindow::sShowWindow );
    ImGuiCheckbox( "Show Game Window", &CreationGameWindow::sShowWindow );
    ImGuiCheckbox( "Show Properties Window", &CreationPropertyWindow::sShowWindow );
    ImGuiCheckbox( "Show Profile Window", &CreationProfileWindow::sShowWindow );
    ImGuiCheckbox( "Show Asset View", &CreationAssetView::sShowWindow  );
    ImGuiCheckbox( "Show Shader Graph Window", &CreationShaderGraphWindow::sShowWindow  );
    ImGuiUnindent();

    PrefabImGui();

    if( ImGuiButton( "Close window" ))
      sShowWindow = false;

    if( ImGuiButton( "Close Application" ) )
      OS::OSAppStopRunning();

    TAC_CALL( DesktopApp::DebugImGui( errors ) );

    ImGuiEnd();
  }
} // namespace Tac

