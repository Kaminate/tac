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
  CreationMainWindow::CreationMainWindow() {};
  CreationMainWindow::~CreationMainWindow()
  {
    delete mUI2DDrawData;
    //delete mUIRoot;
  }
  void CreationMainWindow::Init( Errors& errors )
  {
    mUI2DDrawData = new UI2DDrawData;
    //mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
    //mUIRoot = new UIRoot;
    //mUIRoot->mUI2DDrawData = mUI2DDrawData;
    //mUIRoot->mDesktopWindow = mDesktopWindow;
    int x;
    int y;
    int w;
    int h;
    Creation::Instance->GetWindowsJsonData( gMainWindowName, &x, &y, &w, &h );
    mDesktopWindowHandle = DesktopWindowManager::Instance->CreateWindow( x, y, w, h );
  }
  void CreationMainWindow::LoadTextures( Errors& errors )
  {
    if( mAreTexturesLoaded )
      return;
    struct TextureAndPath
    {
      Texture** texture;
      const char* path;
    };
    Vector< TextureAndPath > textureAndPaths = {
      { &mIconWindow, "assets/grave.png" },
    { &mIconClose, "assets/icons/close.png" },
    { &mIconMinimize, "assets/icons/minimize.png" },
    { &mIconMaximize, "assets/icons/maximize.png" },
    };
    int loadedTextureCount = 0;
    for( TextureAndPath textureAndPath : textureAndPaths )
    {
      TextureAssetManager::Instance->GetTexture( textureAndPath.texture, textureAndPath.path, errors );
      TAC_HANDLE_ERROR( errors );
      if( *textureAndPath.texture )
        loadedTextureCount++;
    }
    if( loadedTextureCount == textureAndPaths.size() )
      mAreTexturesLoaded = true;

  }

  void CreationMainWindow::ImGuiWindows()
  {
    ImGuiText( "Windows" );
    ImGuiIndent();

    // static because hackery ( the errors get saved in a lambda,
    // which then turns into garbage when it goes out of scope... )
    static Errors createWindowErrors;
    if( ImGuiButton( "System" ) )
      mCreation->CreateSystemWindow( createWindowErrors );
    if( ImGuiButton( "Game" ) )
      mCreation->CreateGameWindow( createWindowErrors );
    if( ImGuiButton( "Properties" ) )
      mCreation->CreatePropertyWindow( createWindowErrors );
    if( ImGuiButton( "Profile" ) )
      mCreation->CreateProfileWindow( createWindowErrors );
    if( createWindowErrors )
      ImGuiText( createWindowErrors.ToString() );
    ImGuiUnindent();
  }
  void CreationMainWindow::ImGui()
  {
    Creation::WindowFramebufferInfo* info = Creation::Instance->FindWindowFramebufferInfo( mDesktopWindowHandle);
    if( !info )
      return;

    SetCreationWindowImGuiGlobals( mDesktopWindow,
                                   mUI2DDrawData,
                                   info->mDesktopWindowState.mWidth,
                                   info->mDesktopWindowState.mHeight );
    ImGuiBegin( "Main Window", {} );
    ImGuiBeginMenuBar();
    ImGuiText( "file | edit | window" );
    ImGuiEndMenuBar();
    if( ImGuiButton( "save as" ) )
    {
      World* world = mCreation->mWorld;


      ;

      for( Entity* entity : world->mEntities )
      {
        if( entity->mParent )
          continue;

        String savePath;
        String suggestedName =
          entity->mName +
          ".prefab";
        Errors saveDialogErrors;
        OS::SaveDialog( savePath, suggestedName, saveDialogErrors );
        if( saveDialogErrors )
        {
          // todo: log it, user feedback
          std::cout << saveDialogErrors.ToString() << std::endl;
          continue;
        }

        mCreation->ModifyPathRelative( savePath );

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
      mDesktopWindow->mRequestDeletion = true;
    }

    ImGuiEnd();
  }
  void CreationMainWindow::Update( Errors& errors )
  {
    Creation::WindowFramebufferInfo* info = Creation::Instance->FindWindowFramebufferInfo( mDesktopWindowHandle);
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

    Render::SetViewFramebuffer( ViewIdMainWindow, info->mFramebufferHandle );
    mUI2DDrawData->DrawToTexture( info->mDesktopWindowState.mWidth,
                                  info->mDesktopWindowState.mHeight,
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

