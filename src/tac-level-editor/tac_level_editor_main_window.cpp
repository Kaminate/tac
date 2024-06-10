#include "tac_level_editor_main_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_game_object_menu_window.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static void ButtonsToOpenEditorWindows()
  {
#if 0
    const struct
    {
      using Fn = void ( LevelEditorWindowManager::* )( Errors& );
      const char* mName;
      Fn          mFn;
    } buttons[] =
    {
      { "System",  &LevelEditorWindowManager::CreateSystemWindow },
      { "Game",  &LevelEditorWindowManager::CreateGameWindow },
      { "Properties",  &LevelEditorWindowManager::CreatePropertyWindow },
      { "Profile",  &LevelEditorWindowManager::CreateProfileWindow },
    };

    // c++17 structured binding
    for( auto [name, fn] : buttons )
    {
      if( ImGuiButton(name) )
      {
        TAC_CALL( ( gCreation.mWindowManager.*fn ) ( errors ) );
      }
    }
#endif
  }

  // -----------------------------------------------------------------------------------------------

  CreationMainWindow* CreationMainWindow::Instance { nullptr };

  CreationMainWindow::CreationMainWindow()
  {
    Instance = this;
  }

  CreationMainWindow::~CreationMainWindow()
  {
    Uninit();
    Instance = nullptr;
  }

  void CreationMainWindow::Uninit()
  {
  }

  void CreationMainWindow::Init( Errors& )
  {
  }

  void CreationMainWindow::LoadTextures( Errors& )
  {
  }

  void CreationMainWindow::ImGuiWindows( Errors& errors )
  {

    ImGuiText( "Windows" );
    ImGuiIndent();

    ButtonsToOpenEditorWindows();

    if( ImGuiButton( "Asset View" ) )
    {
      gCreation.mUpdateAssetView = !gCreation.mUpdateAssetView;
    }


    ImGuiUnindent();
  }

  void CreationMainWindow::ImGuiSaveAs( World* world )
  {
    static Errors saveErrors;
    Errors& errors = saveErrors;

    if( saveErrors )
      ImGuiText( saveErrors.ToString() );

    if( !ImGuiButton( "save as" ) )
      return;

    for( Entity* entity : world->mEntities )
    {
      TAC_CALL( ImGuiSaveAs( entity, errors ) );
    }
  }

  void CreationMainWindow::ImGuiSaveAs(Entity* entity, Errors& errors)
  {
    if( entity->mParent )
      return; // why?

    // TODO: use GetAssetSaveDialog instead of OSSaveDialog

    const AssetSaveDialogParams saveParams
    {
      .mSuggestedFilename { entity->mName + ".prefab" },
    };

    const AssetPathStringView assetPath = TAC_CALL( AssetSaveDialog( saveParams, errors ) );
    if( assetPath.empty() )
      return;

    const Json entityJson = entity->Save();
    const String prefabJsonString = entityJson.Stringify();
    const void* bytes = prefabJsonString.data();
    const int byteCount = prefabJsonString.size();

    TAC_CALL( SaveToFile( assetPath, bytes, byteCount, errors ) );
  }

  void CreationMainWindow::ImGui( World* world, Errors& errors )
  {
    SimWindowApi windowApi;
    if( !windowApi.IsShown( mWindowHandle ) )
      return;

    ImGuiSetNextWindowHandle( mWindowHandle );
    ImGuiSetNextWindowStretch();
    ImGuiBegin( "Main Window" );

    //ImGuiBeginMenuBar();
    //ImGuiText( "file | edit | window" );
    //ImGuiEndMenuBar();

    ImGuiSaveAs( world );

    TAC_CALL( ImGuiWindows(errors) );

    PrefabImGui();

    mCloseRequested |= ImGuiButton( "Close window" );
    if( ImGuiButton( "Close Application" ) )
      OS::OSAppStopRunning();

    DesktopApp* desktopApp = DesktopApp::GetInstance();
    TAC_CALL( desktopApp->DebugImGui( errors ) );

    ImGuiEnd();
  }

  void CreationMainWindow::Update( World* world, Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    TAC_CALL( LoadTextures( errors ) );
    TAC_CALL( ImGui( world, errors ) );
  }
} // namespace Tac

