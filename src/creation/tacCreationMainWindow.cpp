#include "creation/tacCreationMainWindow.h"
#include "common/tacOS.h"
#include "common/tacDesktopWindow.h"
#include "common/tacUI.h"
#include "common/tacUI2D.h"
#include "common/tacDesktopWindow.h"
#include "common/tacShell.h"
#include "common/tacTextureAssetManager.h"

struct TacHandleMainWindowClosed : public TacEvent<>::Handler
{
  void HandleEvent() override
  {
    TacOS::Instance->mShouldStopRunning = true;
  }
};

void TacCreationMainWindow::Init( TacErrors& errors )
{
  mDesktopWindow->mOnDestroyed.AddCallback( new TacHandleMainWindowClosed() );
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
}
void TacCreationMainWindow::Update( TacErrors& errors )
{
  TacUIRoot* uiRoot = mUIRoot;
  TacTextureAssetManager* textureAssetManager = mShell->mTextureAssetManager;

  if( !mAreTexturesLoaded )
  {
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
      textureAssetManager->GetTexture( textureAndPath.texture, textureAndPath.path, errors );
      TAC_HANDLE_ERROR( errors );
      if( *textureAndPath.texture )
        loadedTextureCount++;
    }
    if( loadedTextureCount == textureAndPaths.size() )
      mAreTexturesLoaded = true;
  }

  if( mAreTexturesLoaded && !mAreLayoutsCreated )
  {
    float size = 35;

    bool experimental = true;
    if( experimental )
    {
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

      v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );

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
    }


    bool mShouldCreateTopMostBar = false;
    if( mShouldCreateTopMostBar )
    {
      mTopMostBar = uiRoot->AddMenu( "topmost bar" );
      mTopMostBar->mAutoWidth = true;
      mTopMostBar->mHeightTarget = size;

      bool mShouldCreateTopMostBarLeft = false;
      if( mShouldCreateTopMostBarLeft )
      {
        mTopMostBarLeft = mTopMostBar->Add< TacUILayout >( "topmost bar left" );
        mTopMostBarLeft->mAutoWidth = true;

        bool mShouldCreateWindowIcon = true;
        if( mShouldCreateWindowIcon )
        {
          mLayoutIconWindow = mTopMostBarLeft->Add< TacUILayout >( "window icon" );
          mLayoutIconWindow->mUiWidth = size;
          mLayoutIconWindow->mHeightTarget = size;
          mLayoutIconWindow->mTexture = mIconWindow;
        }

        bool mShouldCreateTitleText = true;
        if( mShouldCreateTitleText )
        {
          TacUITextData uiTextData;
          uiTextData.mUtf8 = "Creation (Running) - Tac Studio";
          uiTextData.mFontSize = 16;
          uiTextData.mColor = v4(
            1, 0, 0,
            //90 / 255.0f,
            //111 / 255.0f,
            //102 / 255.0f,
            1 );
          mTitleText = mTopMostBarLeft->Add< TacUIText >( "creation title text" );
          mTitleText->SetText( uiTextData );
        }
      }

      bool mShouldCreateTopMostBarRight = true;
      if( mShouldCreateTopMostBarRight )
      {
        mTopMostBarRight = mTopMostBar->Add< TacUILayout >( "topmost bar right" );
        mTopMostBarRight->mAutoWidth = true;
        mTopMostBarRight->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;

        bool mShouldCreateMinimizeButton = false;
        if( mShouldCreateMinimizeButton )
        {
          mLayoutIconMinimize = mTopMostBarRight->Add< TacUILayout >( "icon minimize" );
          mLayoutIconMinimize->mTexture = mIconClose;
          mLayoutIconMinimize->mUiWidth = size;
          mLayoutIconMinimize->mHeightTarget = size;
          mLayoutIconMinimize->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconMinimize->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }

        bool mShouldCreateMaximizeButton = false;
        if( mShouldCreateMaximizeButton )
        {
          mLayoutIconMaximize = mTopMostBarRight->Add< TacUILayout >( "icon maximize" );
          mLayoutIconMaximize->mTexture = mIconClose;
          mLayoutIconMaximize->mUiWidth = size;
          mLayoutIconMaximize->mHeightTarget = size;
          mLayoutIconMaximize->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconMaximize->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }

        bool mShouldCreateCloseIcon = true;
        if( mShouldCreateCloseIcon )
        {
          mLayoutIconClose = mTopMostBarRight->Add< TacUILayout >( "icon close" );
          mLayoutIconClose->mTexture = mIconClose;
          mLayoutIconClose->mUiWidth = size;
          mLayoutIconClose->mHeightTarget = size;
          mLayoutIconClose->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
          mLayoutIconClose->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
        }
      }
    }

    mAreLayoutsCreated = true;
  }

  mUIRoot->Update();
  mUIRoot->Render( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
