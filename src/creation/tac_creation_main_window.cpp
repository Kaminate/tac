#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_event.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/system/tac_os.h"
#include "src/creation/tac_creation.h"
#include "src/creation/tac_creation_game_object_menu_window.h"
#include "src/creation/tac_creation_main_window.h"
#include "src/creation/tac_creation_prefab.h"
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
    mDesktopWindowHandle = gCreation.CreateWindow( gMainWindowName );
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

  void CreationMainWindow::ImGuiPrefabs()
  {
    if( !ImGuiCollapsingHeader( "Prefabs" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    PrefabImGui();
  }

  void CreationMainWindow::ImGuiWindows()
  {
    ImGuiText( "Windows" );
    ImGuiIndent();

    static Errors createWindowErrors;
    if( ImGuiButton( "System" ) )
      gCreation.CreateSystemWindow( createWindowErrors );
    if( ImGuiButton( "Game" ) )
      gCreation.CreateGameWindow( createWindowErrors );
    if( ImGuiButton( "Properties" ) )
      gCreation.CreatePropertyWindow( createWindowErrors );
    if( ImGuiButton( "Profile" ) )
      gCreation.CreateProfileWindow( createWindowErrors );

    if( ImGuiButton( "Asset View" ) )
      gCreation.mUpdateAssetView = !gCreation.mUpdateAssetView;


    if( createWindowErrors )
      ImGuiText( createWindowErrors.ToString() );

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

        String savePath;
        String suggestedName = entity->mName + ".prefab";
        Errors saveDialogErrors;
        OS::OSSaveDialog( savePath, suggestedName, saveDialogErrors );
        if( saveDialogErrors )
        {
          // todo: log it, user feedback
          OS::OSDebugPrintLine(saveDialogErrors.ToString());
          continue;
        }

        ModifyPathRelative( savePath );

        Json entityJson;
        entity->Save( entityJson );

        String prefabJsonString = entityJson.Stringify();
        Errors saveToFileErrors;
        void* bytes = prefabJsonString.data();
        int byteCount = prefabJsonString.size();
        //OS::OSSaveToFile( savePath, bytes, byteCount, saveToFileErrors );
        Filesystem::SaveToFile( savePath, bytes, byteCount, saveToFileErrors );
        if( saveToFileErrors )
        {
          // todo: log it, user feedback
          OS::OSDebugPrintLine(saveToFileErrors.ToString());
          continue;
        }
      }
    }
  }
  void CreationMainWindow::ImGui()
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
    ImGuiWindows();
    ImGuiPrefabs();


    mCloseRequested |= ImGuiButton( "Close window" );
    if( ImGuiButton( "Close Application" ) )
      OS::OSAppStopRunning();

    //for( int i = 0; i < 2; ++i )
    //{
    //for( char c = 'a'; c <= 'z'; ++c )
    //{
    //  String s;
    //  s.push_back( c );
    //  ImGuiText( s );
    //}
    //}

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

    ImGui();

    Render::Viewport viewport;
    viewport.mWidth = ( float )desktopWindowState->mWidth;
    viewport.mHeight = ( float )desktopWindowState->mHeight;

    Render::ScissorRect scissorRect;
    scissorRect.mXMinRelUpperLeftCornerPixel = 0;
    scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mWidth;
    scissorRect.mYMinRelUpperLeftCornerPixel = 0;
    scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )desktopWindowState->mHeight;

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

      if( KeyboardIsKeyJustDown( Key::MouseLeft )
          && !IsWindowHovered( desktopWindowHandle )
          && ShellGetElapsedSeconds() != CreationGameObjectMenuWindow::Instance->mCreationSeconds )
      {
        delete CreationGameObjectMenuWindow::Instance;
      }
    }
  }
}

