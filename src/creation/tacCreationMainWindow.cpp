#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacJson.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacEvent.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/tacShell.h"
#include "src/common/tacShellTimer.h"
#include "src/common/tacShellTimer.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameObjectMenuWindow.h"
#include "src/creation/tacCreationMainWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"

#include <iostream>

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
    if( createWindowErrors )
      ImGuiText( createWindowErrors.ToString() );
    ImGuiUnindent();
  }

  void CreationMainWindow::ImGui()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiSetNextWindowStretch();
    ImGuiBegin( "Main Window" );



#if 0


#else
    ImGuiBeginMenuBar();
    ImGuiText( "file | edit | window" );
    ImGuiEndMenuBar();
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
        OS::SaveDialog( savePath, suggestedName, saveDialogErrors );
        if( saveDialogErrors )
        {
          // todo: log it, user feedback
          std::cout << saveDialogErrors.ToString().c_str() << std::endl;
          continue;
        }

        ModifyPathRelative( savePath );

        Json entityJson;
        entity->Save( entityJson );

        String prefabJsonString = entityJson.Stringify();
        Errors saveToFileErrors;
        void* bytes = prefabJsonString.data();
        int byteCount = prefabJsonString.size();
        OS::SaveToFile( savePath, bytes, byteCount, saveToFileErrors );
        if( saveToFileErrors )
        {
          // todo: log it, user feedback
          std::cout << saveToFileErrors.ToString().c_str() << std::endl;
          continue;
        }
      }
    }

    ImGuiWindows();

    // to force directx graphics specific window debugging
    //if( ImGuiButton( "close window" ) )
    //{
    //  if(mDesktopWindow)
    //  mDesktopWindow->mRequestDeletion = true;
    //}

#endif

    mCloseRequested |= ImGuiButton( "Close window" );

    ImGuiEnd();
  }

  void CreationMainWindow::Update( Errors& errors )
  {
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

    Viewport viewport;
    viewport.mWidth = ( float )desktopWindowState->mWidth;
    viewport.mHeight = ( float )desktopWindowState->mHeight;

    ScissorRect scissorRect;
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
      DesktopWindowState* menu = GetDesktopWindowState( desktopWindowHandle );
      CreationGameObjectMenuWindow::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft )
          && !IsWindowHovered( desktopWindowHandle )
          && ShellGetElapsedSeconds() != CreationGameObjectMenuWindow::Instance->mCreationSeconds )
      {
        delete CreationGameObjectMenuWindow::Instance;
      }
    }
  }
}

