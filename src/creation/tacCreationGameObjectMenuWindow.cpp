#include "creation/tacCreationGameObjectMenuWindow.h"
#include "creation/tacCreation.h"
#include "common/tacEvent.h"
#include "common/tacOS.h"
#include "common/tacDesktopWindow.h"
#include "common/tacUI.h"
#include "common/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"
#include "common/tacTextureAssetManager.h"
#include "common/tackeyboardinput.h"
#include "shell/tacDesktopApp.h"
#include "space/tacworld.h"
#include "space/tacentity.h"


TacCreationGameObjectMenuWindow::~TacCreationGameObjectMenuWindow()
{
  mDesktopWindow->mRequestDeletion = true;
  delete mUIRoot;
  delete mUI2DDrawData;
}
void TacCreationGameObjectMenuWindow::Init( TacErrors& errors )
{
  TacShell* shell = mCreation->mDesktopApp->mShell;
  mCreationSeconds = shell->mElapsedSeconds;
  TacWindowParams windowParams;
  windowParams.mName = "game object menu";
  windowParams.mWidth = 300;
  windowParams.mHeight = 300;
  windowParams.mX =
    mMainWindow->mDesktopWindow->mX +
    ( int )mMainWindow->mGameObjectButton->mPositionRelativeToRoot.x;
  windowParams.mY =
    mMainWindow->mDesktopWindow->mY +
    ( int )mMainWindow->mGameObjectButton->mPositionRelativeToRoot.y +
    ( int )mMainWindow->mGameObjectButton->mSize.y;
  mMainWindow->mDesktopApp->SpawnWindow(
    windowParams,
    &mDesktopWindow,
    errors );

  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mKeyboardInput = shell->mKeyboardInput;

  CreateLayouts();
}
void TacCreationGameObjectMenuWindow::CreateLayouts()
{
  TacUIHierarchyNode* node;
  TacUIHierarchyVisualText* text;

  text = new TacUIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Audio Source";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( TacUISplit::Before, TacUILayoutType::Vertical );
  node->SetVisual( text );

  text = new TacUIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Text";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( TacUISplit::Before, TacUILayoutType::Vertical );
  node->SetVisual( text );

  text = new TacUIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Cube";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( TacUISplit::Before, TacUILayoutType::Vertical );
  node->SetVisual( text );

  text = new TacUIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Empty";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( TacUISplit::Before, TacUILayoutType::Vertical );
  node->SetVisual( text );
  node->mOnClickEventEmitter.AddCallbackFunctional( [&]() { mCreation->CreateEntity(); } );
}
void TacCreationGameObjectMenuWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();

  v2 cursorPos;
  TacOS::Instance->GetScreenspaceCursorPos( cursorPos, errors );
  TAC_HANDLE_ERROR( errors );

  mUIRoot->mUiCursor.x = cursorPos.x - mDesktopWindow->mX;
  mUIRoot->mUiCursor.y = cursorPos.y - mDesktopWindow->mY;
  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );

}

