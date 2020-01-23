#include "creation/tacCreationMainWindow.h"
#include "creation/tacCreationGameObjectMenuWindow.h"
#include "creation/tacCreation.h"
#include "common/tacEvent.h"
#include "common/tacOS.h"
#include "common/tacDesktopWindow.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"
#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/tackeyboardinput.h"
#include "common/graphics/imgui/tacImGui.h"
#include "space/tacworld.h"
#include "space/tacentity.h"
#include "shell/tacDesktopApp.h"

TacCreationMainWindow::~TacCreationMainWindow()
{
  delete mUI2DDrawData;
  delete mUIRoot;
}
void TacCreationMainWindow::Init( TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUIRoot = new TacUIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
}
void TacCreationMainWindow::LoadTextures( TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
  if( mAreTexturesLoaded )
    return;
  struct TacTextureAndPath
  {
    TacTexture** texture;
    const char* path;
  };
  TacVector< TacTextureAndPath > textureAndPaths = {
    { &mIconWindow, "assets/grave.png" },
  { &mIconClose, "assets/icons/close.png" },
  { &mIconMinimize, "assets/icons/minimize.png" },
  { &mIconMaximize, "assets/icons/maximize.png" },
  };
  int loadedTextureCount = 0;
  for( TacTextureAndPath textureAndPath : textureAndPaths )
  {
    TacTextureAssetManager::Instance->GetTexture( textureAndPath.texture, textureAndPath.path, errors );
    TAC_HANDLE_ERROR( errors );
    if( *textureAndPath.texture )
      loadedTextureCount++;
  }
  if( loadedTextureCount == textureAndPaths.size() )
    mAreTexturesLoaded = true;

}

void TacCreationMainWindow::ImGuiWindows()
{
  TacImGuiText( "Windows" );
  TacImGuiIndent();

  // static because hackery ( the errors get saved in a lambda,
  // which then turns into garbage when it goes out of scope... )
  static TacErrors createWindowErrors;
  if( TacImGuiButton( "System" ) )
  {
    mCreation->CreateSystemWindow( createWindowErrors );
  }
  if( TacImGuiButton( "Game" ) )
  {
    mCreation->CreateGameWindow( createWindowErrors );
  }
  if( TacImGuiButton( "Properties" ) )
  {
    mCreation->CreatePropertyWindow( createWindowErrors );
  }
  if( TacImGuiButton( "Profile" ) )
  {
    mCreation->CreateProfileWindow( createWindowErrors );
  }
  if( createWindowErrors.size() )
  {
    TacImGuiText( createWindowErrors.ToString() );
  }
  TacImGuiUnindent();
}
void TacCreationMainWindow::ImGui()
{
  TacShell* shell = mDesktopApp->mShell;
  SetCreationWindowImGuiGlobals( shell, mDesktopWindow, mUI2DDrawData );
  TacImGuiBegin( "Main Window", {} );
  TacImGuiBeginMenuBar();
  TacImGuiText( "file | edit | window" );
  TacImGuiEndMenuBar();
  if( TacImGuiButton( "save as" ) )
  {
    TacWorld* world = mCreation->mWorld;


    TacShell* shell = mDesktopApp->mShell;
    TacOS* os = TacOS::Instance;

    for( TacEntity* entity : world->mEntities )
    {
      if( entity->mParent )
        continue;

      TacString savePath;
      TacString suggestedName =
        entity->mName +
        ".prefab";
      TacErrors saveDialogErrors;
      os->SaveDialog( savePath, suggestedName, saveDialogErrors );
      if( saveDialogErrors.size() )
      {
        // todo: log it, user feedback
        std::cout << saveDialogErrors.ToString() << std::endl;
        continue;
      }

      mCreation->ModifyPathRelative( savePath );

      TacJson entityJson;
      entity->Save( entityJson );

      TacString prefabJsonString = entityJson.Stringify();
      TacErrors saveToFileErrors;
      void* bytes = prefabJsonString.data();
      int byteCount = prefabJsonString.size();
      os->SaveToFile( savePath, bytes, byteCount, saveToFileErrors );
      if( saveToFileErrors.size() )
      {
        // todo: log it, user feedback
        std::cout << saveToFileErrors.ToString() << std::endl;
        continue;
      }
    }
  }

  ImGuiWindows();

  // to force directx graphics specific window debugging
  if( TacImGuiButton( "close window" ) )
  {
    mDesktopWindow->mRequestDeletion = true;
  }

  TacImGuiEnd();
}
void TacCreationMainWindow::Update( TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
  mDesktopWindow->SetRenderViewDefaults();

  LoadTextures( errors );
  TAC_HANDLE_ERROR( errors );

  ImGui();

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );

  if( mButtonCallbackErrors.size() )
  {
    errors = mButtonCallbackErrors;
    TAC_HANDLE_ERROR( errors );
  }

  if( mGameObjectMenuWindow )
  {
    mGameObjectMenuWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );

    if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) &&
      !mGameObjectMenuWindow->mDesktopWindow->mCursorUnobscured &&
      shell->mElapsedSeconds != mGameObjectMenuWindow->mCreationSeconds )
    {
      delete mGameObjectMenuWindow;
      mGameObjectMenuWindow = nullptr;
    }
  }
}

