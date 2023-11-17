#include "src/level_editor/tac_level_editor_main_window.h" // self-inc

#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_event.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/system/tac_os.h"
#include "src/level_editor/tac_level_editor.h"
#include "src/level_editor/tac_level_editor_game_object_menu_window.h"
#include "src/level_editor/tac_level_editor_prefab.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

namespace Tac
{
  CreationMainWindow* CreationMainWindow::Instance = nullptr;

  CreationMainWindow::CreationMainWindow()
  {
    Instance = this;
  }

  CreationMainWindow::~CreationMainWindow()
  {
    DesktopAppDestroyWindow( mDesktopWindowHandle );
    Instance = nullptr;
    delete mUI2DDrawData;
  }

  void CreationMainWindow::Init( Errors& )
  {
    mUI2DDrawData = TAC_NEW UI2DDrawData;
    mDesktopWindowHandle = gCreation.CreateDesktopWindow( gMainWindowName );
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
    //  TextureAssetManager::GetTexture( textureAndPath.path, errors );
    //  TAC_HANDLE_ERROR( errors );
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

    if( ImGuiButton( "System" ) )
    {
      gCreation.CreateSystemWindow( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( ImGuiButton( "Game" ) )
    {
      gCreation.CreateGameWindow( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( ImGuiButton( "Properties" ) )
    {
      gCreation.CreatePropertyWindow( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( ImGuiButton( "Profile" ) )
    {
      gCreation.CreateProfileWindow( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( ImGuiButton( "Asset View" ) )
    {
      gCreation.mUpdateAssetView = !gCreation.mUpdateAssetView;
    }


    ImGuiUnindent();
  }

  void CreationMainWindow::ImGuiSaveAs()
  {
    if( ImGuiButton( "save as" ) )
    {
      World* world = gCreation.mWorld;
      for( Entity* entity : world->mEntities )
      {
        if( entity->mParent )
          continue;

        // TODO: use GetAssetSaveDialog

        const String suggestedName = entity->mName + ".prefab";
        Errors errors;
        const Filesystem::Path savePath = OS::OSSaveDialog( suggestedName, errors );
        if( errors )
        {
          // todo: log it, user feedback
          OS::OSDebugPrintLine(errors.ToString());
          continue;
        }

        if( savePath.empty() )
          continue;

        const AssetPathStringView assetPath = ModifyPathRelative( savePath, errors );
        if( errors )
        {
          // todo: log it, user feedback
          OS::OSDebugPrintLine(errors.ToString());
          continue;
        }

        TAC_ASSERT( !assetPath.empty() );

        Json entityJson;
        entity->Save( entityJson );

        const String prefabJsonString = entityJson.Stringify();
        const void* bytes = prefabJsonString.data();
        const int byteCount = prefabJsonString.size();

        SaveToFile( assetPath, bytes, byteCount, errors );
        if( errors )
        {
          // todo: log it, user feedback
          OS::OSDebugPrintLine(errors.ToString());
          continue;
        }
      }
    }
  }

  void CreationMainWindow::ImGui(Errors& errors)
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiSetNextWindowStretch();
    ImGuiBegin( "Main Window" );

    //ImGuiBeginMenuBar();
    //ImGuiText( "file | edit | window" );
    //ImGuiEndMenuBar();

    ImGuiSaveAs();

    ImGuiWindows(errors);
    TAC_HANDLE_ERROR( errors );

    PrefabImGui();

    mCloseRequested |= ImGuiButton( "Close window" );
    if( ImGuiButton( "Close Application" ) )
      OS::OSAppStopRunning();

    DesktopAppDebugImGui( errors );
    TAC_HANDLE_ERROR( errors );

    ImGuiEnd();
  }

  void CreationMainWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;
    DesktopAppResizeControls( mDesktopWindowHandle );
    DesktopAppMoveControls( mDesktopWindowHandle );

    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );

    LoadTextures( errors );
    TAC_HANDLE_ERROR( errors );


    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGui( errors );
    TAC_HANDLE_ERROR( errors );

    const v2 size = desktopWindowState->GetSizeV2();
    const Render::Viewport viewport = size;
    const Render::ScissorRect scissorRect = size;

    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, viewport );
    Render::SetViewScissorRect( viewHandle, scissorRect );

    mUI2DDrawData->DrawToTexture( viewHandle,
                                  desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  errors );
    TAC_HANDLE_ERROR( errors );

    if( CreationGameObjectMenuWindow::Instance )
    {
      DesktopWindowHandle desktopWindowHandle = CreationGameObjectMenuWindow::Instance->mDesktopWindowHandle;
      CreationGameObjectMenuWindow::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      if( Mouse::ButtonJustDown( Mouse::Button::MouseLeft )
          && !IsWindowHovered( desktopWindowHandle )
          && ShellGetElapsedSeconds() != CreationGameObjectMenuWindow::Instance->mCreationSeconds )
      {
        delete CreationGameObjectMenuWindow::Instance;
      }
    }
  }
}

