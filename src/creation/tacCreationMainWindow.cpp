#include "src/creation/tacCreationMainWindow.h"
#include "src/creation/tacCreationGameObjectMenuWindow.h"
#include "src/creation/tacCreation.h"
#include "src/common/tacEvent.h"
#include "src/common/tacOS.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacShell.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"

namespace Tac
{
  CreationMainWindow* CreationMainWindow::Instance = nullptr;
  CreationMainWindow::CreationMainWindow()
  {
    Instance = this;
  }

  CreationMainWindow::~CreationMainWindow()
  {
    Instance = nullptr;
    delete mUI2DDrawData;
    //delete mUIRoot;
  }

  void CreationMainWindow::Init( Errors& )
  {
    mUI2DDrawData = TAC_NEW UI2DDrawData;
    int x;
    int y;
    int w;
    int h;
    Creation::Instance->GetWindowsJsonData( gMainWindowName, &x, &y, &w, &h );
    mDesktopWindowHandle = DesktopWindowManager::Instance->CreateWindow( x, y, w, h );
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
    Creation* creation = Creation::Instance;

    static Errors createWindowErrors;
    if( ImGuiButton( "System" ) )
      creation->CreateSystemWindow( createWindowErrors );
    if( ImGuiButton( "Game" ) )
      creation->CreateGameWindow( createWindowErrors );
    if( ImGuiButton( "Properties" ) )
      creation->CreatePropertyWindow( createWindowErrors );
    if( ImGuiButton( "Profile" ) )
      creation->CreateProfileWindow( createWindowErrors );
    if( createWindowErrors )
      ImGuiText( createWindowErrors.ToString() );
    ImGuiUnindent();
  }

  void CreationMainWindow::ImGui()
  {
    Creation* creation = Creation::Instance;

    Creation::WindowFramebufferInfo* info =
      Creation::Instance->FindWindowFramebufferInfo( mDesktopWindowHandle );
    if( !info )
      return;

    DesktopWindowState* desktopWindowState = FindDesktopWindowState( info->mDesktopWindowHandle );
    if( !desktopWindowState )
      return;

    SetCreationWindowImGuiGlobals( desktopWindowState, mUI2DDrawData );

    ImGuiBegin( "Main Window", {} );
#if 1
    ImGuiBeginMenuBar();
    ImGuiText( "file | edit | window" );
    ImGuiEndMenuBar();
    if( ImGuiButton( "save as" ) )
    {
      World* world = creation->mWorld;
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
          std::cout << saveDialogErrors.ToString() << std::endl;
          continue;
        }

        creation->ModifyPathRelative( savePath );

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
          std::cout << saveToFileErrors.ToString() << std::endl;
          continue;
        }
      }
    }

    ImGuiWindows();

    // to force directx graphics specific window debugging
    if( ImGuiButton( "close window" ) )
    {
      if(mDesktopWindow)
      mDesktopWindow->mRequestDeletion = true;
    }

#endif

    //static bool hello;
    //if( ImGuiButton( "asdf" ) )
    //{
    //  hello = true;
    //}
    //if( hello )
    //  ImGuiText( "hello" );

    ImGuiEnd();
  }

  void CreationMainWindow::Update( Errors& errors )
  {
    Creation::WindowFramebufferInfo* info =
      Creation::Instance->FindWindowFramebufferInfo( mDesktopWindowHandle );
    if( !info )
      return;

    //mDesktopWindow->SetRenderViewDefaults();

    LoadTextures( errors );
    TAC_HANDLE_ERROR( errors );

    ImGui();

    //auto params = DesktopWindowManager::Instance->FindWindowParams( gMainWindowName );
    //if( params )
    //  params->mWidth;
    //  params->mHeight;


    DesktopWindowState* desktopWindowState = FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;

    Viewport viewport;
    viewport.mWidth = (float)desktopWindowState->mWidth;
    viewport.mHeight = (float)desktopWindowState->mHeight;

    ScissorRect scissorRect;
    scissorRect.mXMinRelUpperLeftCornerPixel = 0;
    scissorRect.mXMaxRelUpperLeftCornerPixel = (float)desktopWindowState->mWidth;
    scissorRect.mYMinRelUpperLeftCornerPixel = 0;
    scissorRect.mYMaxRelUpperLeftCornerPixel = (float)desktopWindowState->mHeight;

    Render::SetViewFramebuffer( ViewIdMainWindow, info->mFramebufferHandle );
    Render::SetViewport( ViewIdMainWindow, viewport );
    Render::SetViewScissorRect( ViewIdMainWindow, scissorRect );

    mUI2DDrawData->DrawToTexture( desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  ViewIdMainWindow,
                                  errors );
    TAC_HANDLE_ERROR( errors );

    if( mButtonCallbackErrors )
    {
      errors = mButtonCallbackErrors;
      TAC_HANDLE_ERROR( errors );
    }

    if( mGameObjectMenuWindow )
    {
      mGameObjectMenuWindow->Update( errors );
      TAC_HANDLE_ERROR( errors );

      if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) &&
        !mGameObjectMenuWindow->mDesktopWindow->mCursorUnobscured &&
        Shell::Instance->mElapsedSeconds != mGameObjectMenuWindow->mCreationSeconds )
      {
        delete mGameObjectMenuWindow;
        mGameObjectMenuWindow = nullptr;
      }
    }
  }
}

