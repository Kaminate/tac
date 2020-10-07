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
  CreationGameObjectMenuWindow* CreationGameObjectMenuWindow::Instance = nullptr;
  CreationGameObjectMenuWindow::CreationGameObjectMenuWindow()
  {
    Instance = this;
  }
  CreationGameObjectMenuWindow::~CreationGameObjectMenuWindow()
  {
    //mDesktopWindow->mRequestDeletion = true;
    //delete mUIRoot;
    delete mUI2DDrawData;
    Instance = nullptr;
  }
  void CreationGameObjectMenuWindow::Init( Errors& errors )
  {
    //CreationMainWindow* mainWindow = CreationMainWindow::Instance;
    TAC_UNUSED_PARAMETER( errors );
    mCreationSeconds = Shell::Instance.mElapsedSeconds;
    //WindowParams windowParams;
    //windowParams.mName = "game object menu";
    //windowParams.mWidth = 300;
    //windowParams.mHeight = 300;
    //windowParams.mX =
    //  mainWindow->mDesktopWindow->mX +
    //  ( int )mainWindow->mGameObjectButton->mPositionRelativeToRoot.x;
    //windowParams.mY =
    //  mainWindow->mDesktopWindow->mY +
    //  ( int )mainWindow->mGameObjectButton->mPositionRelativeToRoot.y +
    //  ( int )mainWindow->mGameObjectButton->mSize.y;

    //DesktopApp::Instance->SpawnWindow(
    //  windowParams,
    //  &mDesktopWindow,
    //  errors );

    mUI2DDrawData = TAC_NEW UI2DDrawData;
    //mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
    //mUIRoot = TAC_NEW UIRoot;
    //mUIRoot->mUI2DDrawData = mUI2DDrawData;
    //mUIRoot->mDesktopWindow = mDesktopWindow;

    CreateLayouts();
  }
  void CreationGameObjectMenuWindow::CreateLayouts()
  {
    //Creation* creation = Creation::Instance;
    //UIHierarchyNode* node;
    //UIHierarchyVisualText* text;

    //text = TAC_NEW UIHierarchyVisualText;
    //text->mUITextData.mUtf8 = "Audio Source";
    //text->mUITextData.mFontSize = 16;
    //text->mUITextData.mColor = textColor;
    //text->mDims = { 100, 50 };
    //node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
    //node->SetVisual( text );

    //text = TAC_NEW UIHierarchyVisualText;
    //text->mUITextData.mUtf8 = "Text";
    //text->mUITextData.mFontSize = 16;
    //text->mUITextData.mColor = textColor;
    //text->mDims = { 100, 50 };
    //node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
    //node->SetVisual( text );

    //text = TAC_NEW UIHierarchyVisualText;
    //text->mUITextData.mUtf8 = "Cube";
    //text->mUITextData.mFontSize = 16;
    //text->mUITextData.mColor = textColor;
    //text->mDims = { 100, 50 };
    //node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
    //node->SetVisual( text );

    //text = TAC_NEW UIHierarchyVisualText;
    //text->mUITextData.mUtf8 = "Empty";
    //text->mUITextData.mFontSize = 16;
    //text->mUITextData.mColor = textColor;
    //text->mDims = { 100, 50 };
    //node = mUIRoot->mHierarchyRoot->Split( UISplit::Before, UILayoutType::Vertical );
    //node->SetVisual( text );
    //node->mOnClickEventEmitter.AddCallbackFunctional( [ & ]() { creation->CreateEntity(); } );
  }
  void CreationGameObjectMenuWindow::Update( Errors& errors )
  {
    //mDesktopWindow->SetRenderViewDefaults();

    //v2 cursorPos;
    //OS::GetScreenspaceCursorPos( cursorPos, errors );
    //TAC_HANDLE_ERROR( errors );

    //mUIRoot->mUiCursor.x = cursorPos.x - mDesktopWindow->mX;
    //mUIRoot->mUiCursor.y = cursorPos.y - mDesktopWindow->mY;
    //mUIRoot->Update();
    //mUIRoot->Render( errors );
    //TAC_HANDLE_ERROR( errors );

    //mUI2DDrawData->DrawToTexture( 0, 0, 0, errors );
    //TAC_HANDLE_ERROR( errors );

  }


}

