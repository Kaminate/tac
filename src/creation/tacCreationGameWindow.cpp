#include "creation/tacCreationGameWindow.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/tacRenderer.h"
#include "common/tacUI2D.h"
#include "common/tacUI.h"
#include "common/tacImGui.h"
#include "common/tacTextureAssetManager.h"
#include "common/tacOS.h"
#include "space/tacGhost.h"

void TacCreationGameWindow::Init( TacErrors& errors )
{
  TacShell* shell = mShell;

  auto uI2DDrawData = new TacUI2DDrawData();
  uI2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
  uI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData = uI2DDrawData;


  mUIRoot = new TacUIRoot;
  mUIRoot->mElapsedSeconds = &mShell->mElapsedSeconds;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mKeyboardInput = mShell->mKeyboardInput;
  mUIRoot->mDesktopWindow = mDesktopWindow;
}
void TacCreationGameWindow::SetImGuiGlobals()
{
  TacErrors mousePosErrors;
  v2 mousePosScreenspace;
  TacOS::Instance->GetScreenspaceCursorPos( mousePosScreenspace, mousePosErrors );
  if( mousePosErrors.empty() )
  {
    gTacImGuiGlobals.mMousePositionDesktopWindowspace = {
      mousePosScreenspace.x - mDesktopWindow->mX,
      mousePosScreenspace.y - mDesktopWindow->mY };
    gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = mDesktopWindow->mCursorUnobscured;
  }
  else
  {
    gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = false;
  }

  gTacImGuiGlobals.mUI2DDrawData = mUI2DDrawData;
  gTacImGuiGlobals.mKeyboardInput = mShell->mKeyboardInput;
}
void TacCreationGameWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();
  SetImGuiGlobals();

  if( mSoul )
  {
    auto ghost = ( TacGhost* )mSoul;
    ghost->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  TacImGuiBegin( "gameplay overlay", { 300, 75 } );
  if( mSoul )
  {
    if( TacImGuiButton( "Stop" ) )
    {
      delete mSoul;
      mSoul = nullptr;
    }
  }
  else
  {
    if( TacImGuiButton( "Play" ) )
    {
      auto ghost = new TacGhost;
      ghost->mShell = mShell;
      ghost->mRenderView = mDesktopWindow->mRenderView;
      ghost->Init( errors );
      TAC_HANDLE_ERROR( errors );
      mSoul = ghost;
    }
  }
  TacImGuiEnd();

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
