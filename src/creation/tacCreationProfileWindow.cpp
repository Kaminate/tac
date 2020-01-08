#include "common/graphics/tacUI2D.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/graphics/tacImGui.h"
#include "common/tacOS.h"
#include "creation/tacCreation.h"
#include "creation/tacCreationProfileWindow.h"
#include "space/tacworld.h"
#include "space/tacentity.h"
#include "space/tacsystem.h"
#include "shell/tacDesktopApp.h"

TacCreationProfileWindow::~TacCreationProfileWindow()
{
  delete mUI2DDrawData;
}
void TacCreationProfileWindow::Init( TacErrors& errors )
{
  TacShell* shell = mShell;
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;

  TacSettings* settings = shell->mSettings;
};
void TacCreationProfileWindow::ImGui()
{
  TacShell* shell = mShell;

  TacImGuiSetGlobals( shell, mDesktopWindow, mUI2DDrawData );
  TacImGuiBegin( "Profile Window", {} );

  TacImGuiText( "i am the profile window" );

  // to force directx graphics specific window debugging
  if( TacImGuiButton( "close window" ) )
  {
    mDesktopWindow->mRequestDeletion = true;
  }
  TacImGuiEnd();
}
void TacCreationProfileWindow::Update( TacErrors& errors )
{
  TacShell* shell = mShell;
  mDesktopWindow->SetRenderViewDefaults();
  ImGui();
  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}

