#include "tac_level_editor_main_window.h" // self-inc

#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/os/tac_filesystem.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/input/tac_keyboard_input.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_game_object_menu_window.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{
  CreationMainWindow* CreationMainWindow::Instance { nullptr };

  CreationMainWindow::CreationMainWindow()
  {
    Instance = this;
  }

  CreationMainWindow::~CreationMainWindow()
  {
    DesktopApp::GetInstance()->DestroyWindow( mWindowHandle );
    SimWindowApi* windowApi{};
    windowApi->DestroyWindow( mWindowHandle );
    Instance = nullptr;
    //TAC_DELETE mUI2DDrawData;
  }

  void CreationMainWindow::Init( Errors& )
  {
    //mUI2DDrawData = TAC_NEW UI2DDrawData;
    mWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gMainWindowName );
  }

  void CreationMainWindow::LoadTextures( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    //if( mAreTexturesLoaded )
    //  return;
    //struct TextureAndPath
    //{
    //  Render::TextureHandle textureHandle;
    //  const char* path;
    //};
    //Vector< TextureAndPath > textureAndPaths = {
    //  { &mIconWindow, "assets/grave.png" },
    //{ &mIconClose, "assets/icons/close.png" },
    //{ &mIconMinimize, "assets/icons/minimize.png" },
    //{ &mIconMaximize, "assets/icons/maximize.png" },
    //};
    //int loadedTextureCount = 0;
    //for( TextureAndPath textureAndPath : textureAndPaths )
    //{
    //  TAC_CALL( TextureAssetManager::GetTexture( textureAndPath.path, errors ) );
    //  if( *textureAndPath.texture )
    //    loadedTextureCount++;
    //}
    //if( loadedTextureCount == textureAndPaths.size() )
    mAreTexturesLoaded = true;

  }

  void CreationMainWindow::ImGuiWindows( Errors& errors )
  {

    ImGuiText( "Windows" );
    ImGuiIndent();

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

    if( ImGuiButton( "Asset View" ) )
    {
      gCreation.mUpdateAssetView = !gCreation.mUpdateAssetView;
    }


    ImGuiUnindent();
  }

  void CreationMainWindow::ImGuiSaveAs()
  {
    static Errors saveErrors;
    Errors& errors = saveErrors;

    if( saveErrors )
      ImGuiText( saveErrors.ToString() );

    if( !ImGuiButton( "save as" ) )
      return;

    World* world = gCreation.mWorld;
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
      .mSuggestedFilename = entity->mName + ".prefab",
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

  void CreationMainWindow::ImGui(Errors& errors)
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGuiSetNextWindowHandle( mWindowHandle );
    ImGuiSetNextWindowStretch();
    ImGuiBegin( "Main Window" );

    //ImGuiBeginMenuBar();
    //ImGuiText( "file | edit | window" );
    //ImGuiEndMenuBar();

    ImGuiSaveAs();

    TAC_CALL( ImGuiWindows(errors) );

    PrefabImGui();

    mCloseRequested |= ImGuiButton( "Close window" );
    if( ImGuiButton( "Close Application" ) )
      OS::OSAppStopRunning();

    DesktopApp* desktopApp = DesktopApp::GetInstance();
    TAC_CALL( desktopApp->DebugImGui( errors ) );

    ImGuiEnd();
  }

  void CreationMainWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mWindowHandle );
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mWindowHandle );

    TAC_CALL( LoadTextures( errors ) );


    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    TAC_CALL( ImGui( errors ) );

    const v2 size = desktopWindowState->GetSizeV2();
    const Render::Viewport viewport = size;
    const Render::ScissorRect scissorRect = size;

    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, viewport );
    Render::SetViewScissorRect( viewHandle, scissorRect );

    //TAC_CALL(mUI2DDrawData->DrawToTexture( viewHandle,
    //                              desktopWindowState->mWidth,
    //                              desktopWindowState->mHeight,
    //                              errors ) );

    if( CreationGameObjectMenuWindow::Instance )
    {
      WindowHandle WindowHandle = CreationGameObjectMenuWindow::Instance->mWindowHandle;
      TAC_CALL( CreationGameObjectMenuWindow::Instance->Update( errors ) );

      if( Mouse::ButtonJustDown( Mouse::Button::MouseLeft )
          && !IsWindowHovered( WindowHandle )
          && Timestep::GetElapsedTime() != CreationGameObjectMenuWindow::Instance->mCreationSeconds )
      {
        TAC_DELETE CreationGameObjectMenuWindow::Instance;
      }
    }
  }
} // namespace Tac

