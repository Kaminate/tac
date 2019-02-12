#include "creation/tacCreationPropertyWindow.h"
#include "common/tacErrorHandling.h"
#include "common/tacUI.h"
#include "common/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"

TacCreationPropertyWindow::~TacCreationPropertyWindow()
{
}

void TacCreationPropertyWindow::Init( TacErrors& errors )
{
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mHierarchyRoot->Split();
}

void TacCreationPropertyWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();
  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
