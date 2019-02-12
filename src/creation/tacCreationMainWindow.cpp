#include "creation/tacCreationMainWindow.h"
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

void TacCreationMainWindow::Init( TacErrors& errors )
{
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mKeyboardInput = mShell->mKeyboardInput;
}
void TacCreationMainWindow::LoadTextures( TacErrors& errors )
{

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
    mShell->mTextureAssetManager->GetTexture( textureAndPath.texture, textureAndPath.path, errors );
    TAC_HANDLE_ERROR( errors );
    if( *textureAndPath.texture )
      loadedTextureCount++;
  }
  if( loadedTextureCount == textureAndPaths.size() )
    mAreTexturesLoaded = true;

}
void TacCreationMainWindow::CreateLayouts()
{
  if( mAreLayoutsCreated )
    return;

  if( !mAreTexturesLoaded )
    return;

  float size = 35;
  TacUIRoot* uiRoot = mUIRoot;

  TacUIHierarchyNode* node = nullptr;
  TacUIHierarchyVisualImage* image = nullptr;
  TacUIHierarchyVisualText* text = nullptr;


  TacUIHierarchyNode* contentArea = uiRoot->mHierarchyRoot;

  TacUIHierarchyNode* menuBar = contentArea->Split(
    TacUISplit::Before, TacUILayoutType::Vertical );

  TacUIHierarchyNode* topBar = contentArea->Split(
    TacUISplit::Before, TacUILayoutType::Vertical );

  TacUIHierarchyNode* statusBar = contentArea->Split(
    TacUISplit::After, TacUILayoutType::Vertical );


  if( topBar )
  {
    topBar->mDebugName = "top bar";
    topBar->mSize.y = size;

    image = new TacUIHierarchyVisualImage();
    image->mTexture = mIconMinimize;
    image->mDims = { size, size };
    node = topBar->Split();
    node->SetVisual( image );

    image = new TacUIHierarchyVisualImage();
    image->mDims = { size, size };
    image->mTexture = mIconMaximize;
    node = topBar->Split( TacUISplit::After, TacUILayoutType::Horizontal );
    node->SetVisual( image );

    image = new TacUIHierarchyVisualImage();
    image->mDims = { size, size };
    image->mTexture = mIconClose;
    node = topBar->Split( TacUISplit::After, TacUILayoutType::Horizontal );
    node->SetVisual( image );
    node->mOnClickEventEmitter.AddCallbackFunctional( [&]()
    {
      mDesktopWindow->mRequestDeletion = true;
    } );

    text = new TacUIHierarchyVisualText();
    text->mUITextData.mUtf8 = "Gravestory (Running) - Moachers Creation Studio";
    text->mUITextData.mFontSize = 16;
    text->mUITextData.mColor = textColor;
    text->mDims = { 400, 50 };
    node = topBar->Split( TacUISplit::Before, TacUILayoutType::Horizontal );
    node->SetVisual( text );

    image = new TacUIHierarchyVisualImage();
    image->mDims = { size, size };
    image->mTexture = mIconWindow;
    node = topBar->Split( TacUISplit::Before, TacUILayoutType::Horizontal );
    node->SetVisual( image );

  }

  if( menuBar )
  {
    menuBar->mDebugName = "menu bar";
    menuBar->mSize.y = 300;

    text = new TacUIHierarchyVisualText();
    text->mUITextData.mUtf8 = "Help";
    text->mUITextData.mFontSize = 16;
    text->mUITextData.mColor = textColor;
    text->mDims = { 100, 50 };
    node = menuBar->Split( TacUISplit::Before );
    node->SetVisual( text );

    text = new TacUIHierarchyVisualText();
    text->mUITextData.mUtf8 = "Window";
    text->mUITextData.mFontSize = 16;
    text->mUITextData.mColor = textColor;
    text->mDims = { 100, 50 };
    node = menuBar->Split( TacUISplit::Before );
    node->SetVisual( text );

    text = new TacUIHierarchyVisualText();
    text->mUITextData.mUtf8 = "Game Object";
    text->mUITextData.mFontSize = 16;
    text->mUITextData.mColor = textColor;
    text->mDims = { 100, 50 };
    mGameObjectButton = menuBar->Split( TacUISplit::Before );
    mGameObjectButton->SetVisual( text );
    mGameObjectButton->mOnClickEventEmitter.AddCallback( new TacFunctionalHandler( [&]()
    {
      if( mGameObjectMenuWindow )
        return;
      mGameObjectMenuWindow = new TacCreationGameObjectMenuWindow;
      mGameObjectMenuWindow->mMainWindow = this;
      mGameObjectMenuWindow->mShell = mShell;
      mGameObjectMenuWindow->Init( mButtonCallbackErrors );
    } ) );

    text = new TacUIHierarchyVisualText();
    text->mUITextData.mUtf8 = "Edit";
    text->mUITextData.mFontSize = 16;
    text->mUITextData.mColor = textColor;
    text->mDims = { 100, 50 };
    node = menuBar->Split( TacUISplit::Before );
    node->SetVisual( text );

    text = new TacUIHierarchyVisualText();
    text->mUITextData.mUtf8 = "File";
    text->mUITextData.mFontSize = 16;
    text->mUITextData.mColor = textColor;
    text->mDims = { 100, 50 };
    node = menuBar->Split( TacUISplit::Before );
    node->SetVisual( text );
  }

  if( contentArea )
  {
    contentArea->mDebugName = "content area";
  }

  if( statusBar )
  {
    statusBar->mDebugName = "status bar";
    statusBar->mSize.y = 30;
  }

  if( false )
  {
    TacString stringified = uiRoot->DebugGenerateGraphVizDotFile();
    TacString filepath = mShell->mPrefPath + "/tac.dot";
    TacErrors errors;
    TacOS::Instance->SaveToFile( filepath, stringified.data(), stringified.size(), errors );
    TacAssert( errors.empty() );
  }

  mAreLayoutsCreated = true;
}
void TacCreationMainWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();

  LoadTextures( errors );
  TAC_HANDLE_ERROR( errors );

  CreateLayouts();

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

  if( mButtonCallbackErrors.size() )
  {
    errors = mButtonCallbackErrors;
    TAC_HANDLE_ERROR( errors );
  }

  if( mGameObjectMenuWindow )
  {
    mGameObjectMenuWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );

    if( mShell->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) &&
      !mGameObjectMenuWindow->mDesktopWindow->mCursorUnobscured &&
      mShell->mElapsedSeconds != mGameObjectMenuWindow->mCreationSeconds )
    {
      delete mGameObjectMenuWindow;
      mGameObjectMenuWindow = nullptr;
    }
  }
}

