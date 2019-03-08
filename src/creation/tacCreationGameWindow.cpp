#include "creation/tacCreationGameWindow.h"
#include "creation/tacCreation.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/tacRenderer.h"
#include "common/tacUI2D.h"
#include "common/tacUI.h"
#include "common/tacImGui.h"
#include "common/tacTextureAssetManager.h"
#include "common/tacOS.h"
#include "space/tacGhost.h"
#include "space/tacgraphics.h"
#include "space/tacworld.h"
#include "space/tacentity.h"

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

void TacCreationGameWindow::RenderGameWorld()
{
  m4 view = M4View(
    mCreation->mEditorCamPos,
    mCreation->mEditorCamForwards,
    mCreation->mEditorCamRight,
    mCreation->mEditorCamUp );
  float farPlane = 10000.0f;
  float nearPlane = 0.1f;
  float a;
  float b;
  mShell->mRenderer->GetPerspectiveProjectionAB( farPlane, nearPlane, a, b );
  float fovyrad = 100.0f * ( 3.14f / 180.0f );
  float aspect = ( float )mDesktopWindow->mWidth / ( float ) mDesktopWindow->mHeight;
  m4 proj = M4ProjPerspective( a, b, fovyrad, aspect );

  TacEntity* entity = mCreation->mSelectedEntity;
  if( entity )
  {
    v3 pos = entity->mPosition;
    v4 posVS4 = view * v4( pos, 1 );
    float clip_height = std::abs( std::tan( fovyrad / 2.0f ) * posVS4.z * 2.0f );

    auto graphics = ( TacGraphics* )mCreation->mWorld->GetSystem( TacSystemType::Graphics );
    v3 x = { 1, 0, 0 };
    v3 y = { 0, 1, 0 };
    v3 z = { 0, 0, 1 };
    v3 red = { 1, 0, 0 };
    v3 grn = { 0, 1, 0 };
    v3 blu = { 0, 0, 1 };

    float arrowLen = clip_height * 0.3f;
    //graphics->DebugDrawArrow( entity->mPosition, entity->mPosition + x * arrowLen, red );
    //graphics->DebugDrawArrow( entity->mPosition, entity->mPosition + y * arrowLen, grn );
    //graphics->DebugDrawArrow( entity->mPosition, entity->mPosition + z * arrowLen, blu );
  }
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

  RenderGameWorld();

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
