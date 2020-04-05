#include "src/creation/tacCreationGameObjectMenuWindow.h"
#include "src/creation/tacCreationMainWindow.h"
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
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"

namespace Tac
{

CreationGameObjectMenuWindow::~CreationGameObjectMenuWindow()
{
  mDesktopWindow->mRequestDeletion = true;
  delete mUIRoot;
  delete mUI2DDrawData;
}
void CreationGameObjectMenuWindow::Init( Errors& errors )
{
  Shell* shell = Shell::Instance;
  mCreationSeconds = shell->mElapsedSeconds;
  WindowParams windowParams;
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
  DesktopApp::Instance->SpawnWindow(
    windowParams,
    &mDesktopWindow,
    errors );

  mUI2DDrawData = new UI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUIRoot = new UIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;

  CreateLayouts();
}
void CreationGameObjectMenuWindow::CreateLayouts()
{
  UIHierarchyNode* node;
  UIHierarchyVisualText* text;

  text = new UIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Audio Source";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
  node->SetVisual( text );

  text = new UIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Text";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
  node->SetVisual( text );

  text = new UIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Cube";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
  node->SetVisual( text );

  text = new UIHierarchyVisualText();
  text->mUITextData.mUtf8 = "Empty";
  text->mUITextData.mFontSize = 16;
  text->mUITextData.mColor = textColor;
  text->mDims = { 100, 50 };
  node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
  node->SetVisual( text );
  node->mOnClickEventEmitter.AddCallbackFunctional( [&]() { mCreation->CreateEntity(); } );
}
void CreationGameObjectMenuWindow::Update( Errors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();

  v2 cursorPos;
  OS::Instance->GetScreenspaceCursorPos( cursorPos, errors );
  TAC_HANDLE_ERROR( errors );

  mUIRoot->mUiCursor.x = cursorPos.x - mDesktopWindow->mX;
  mUIRoot->mUiCursor.y = cursorPos.y - mDesktopWindow->mY;
  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );

}


}

