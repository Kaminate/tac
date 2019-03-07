#include "creation/tacCreationGameWindow.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/tacRenderer.h"
#include "common/tacUI2D.h"
#include "common/tacUI.h"
#include "common/tacTextureAssetManager.h"
#include "space/tacGhost.h"


struct TacGameVis : public TacUIHierarchyVisual
{
  void Render( TacErrors& errors ) override
  {
    TacTexture* texture;
    TacString path;
    TacRenderer* renderer = mSoul->mShell->mRenderer;
    TacTextureAssetManager* textureAssetManager = mSoul->mShell->mTextureAssetManager;

    path = "assets/vgb_blue_big.png";
    textureAssetManager->GetTexture( &texture, path, errors );
    TacUI2DState& state = mHierarchyNode->mUIRoot->mUI2DDrawData->mStates.back();
    state.Draw2DBox(
      ( float )mDesktopWindow->mWidth,
      ( float )mDesktopWindow->mHeight,
      v4( 1, 1, 1, 1 ),
      texture );

    float innerBoxPercentOffsetX = 0.159f;
    float innerBoxPercentOffsetY = 0.175f;
    float innerBoxPercentWidth = 1 - ( 2 * innerBoxPercentOffsetX );
    float innerBoxPercentHeight = 1 - ( 2 * innerBoxPercentOffsetY );

    float innerBoxPixelOffsetY = innerBoxPercentOffsetX * ( float )mDesktopWindow->mWidth;
    float innerBoxPixelOffsetX = innerBoxPercentOffsetY * ( float )mDesktopWindow->mHeight;
    float innerBoxPixelWidth = innerBoxPercentWidth * ( float )mDesktopWindow->mWidth;
    float innerBoxPixelHeight = innerBoxPercentHeight * ( float )mDesktopWindow->mHeight;
    state.Translate( innerBoxPixelOffsetY, innerBoxPixelOffsetX );

    TacTexture* outputColor = mRenderView->mFramebuffer;
    TacDepthBuffer* outputDepth = mRenderView->mFramebufferDepth;
    if( !outputColor || outputColor->myImage.mWidth != ( int )innerBoxPixelWidth )
    {
      if( outputColor )
      {
        renderer->RemoveRendererResource( outputColor );
        renderer->RemoveRendererResource( outputDepth );
        outputColor = nullptr;
        outputDepth = nullptr;
      }

      TacImage image;
      image.mWidth = ( int )innerBoxPixelWidth;
      image.mHeight = ( int )innerBoxPixelHeight;
      image.mFormat.mPerElementByteCount = 1;
      image.mFormat.mElementCount = 4;
      image.mFormat.mPerElementDataType = TacGraphicsType::unorm;
      TacTextureData textureData;
      textureData.access = TacAccess::Default;
      textureData.binding = { TacBinding::RenderTarget, TacBinding::ShaderResource };
      textureData.cpuAccess = {};
      textureData.mName = "client view fbo";
      textureData.mStackFrame = TAC_STACK_FRAME;
      textureData.myImage = image;
      renderer->AddTextureResource( &outputColor, textureData, errors );
      TAC_HANDLE_ERROR( errors );

      TacDepthBufferData depthBufferData;
      depthBufferData.mName = "client view depth buffer";
      depthBufferData.mStackFrame = TAC_STACK_FRAME;
      depthBufferData.width = ( int )innerBoxPixelWidth;
      depthBufferData.height = ( int )innerBoxPixelHeight;
      renderer->AddDepthBuffer( &outputDepth, depthBufferData, errors );
      TAC_HANDLE_ERROR( errors );

      mRenderView->mFramebuffer = outputColor;
      mRenderView->mFramebufferDepth = outputDepth;
    }

    //path = "assets/unknown.png";
    //mTextureAssetManager->GetTexture( &texture, path, errors );
    texture = outputColor;

    mRenderView->mScissorRect.mXMaxRelUpperLeftCornerPixel = innerBoxPixelWidth;
    mRenderView->mScissorRect.mYMaxRelUpperLeftCornerPixel = innerBoxPixelHeight;
    mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight = innerBoxPixelWidth;
    mRenderView->mViewportRect.mViewportPixelHeightIncreasingUp = innerBoxPixelHeight;
    uI2DDrawData->DrawToTexture( errors );
    TAC_HANDLE_ERROR( errors );

    state.Draw2DBox(
      innerBoxPixelWidth,
      innerBoxPixelHeight,
      v4( 1, 1, 1, 1 ),
      texture );

    mDims = {
      ( float )mDesktopWindow->mWidth,
      ( float )mDesktopWindow->mHeight };
  }
  TacString GetDebugName() override
  {
    return "game vis";
  }
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacSoul* mSoul = nullptr;
  TacRenderView* mRenderView = nullptr;
  TacUI2DDrawData* uI2DDrawData = nullptr;
  TacCreationGameWindow* creationGameWindow = nullptr;
};

void TacCreationGameWindow::Init( TacErrors& errors )
{
  //mRenderView = new TacRenderView;

  TacShell* shell = mShell;

  auto uI2DDrawData = new TacUI2DDrawData();
  uI2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
  //uI2DDrawData->mRenderView = mRenderView;
  uI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData = uI2DDrawData;

  auto ghost = new TacGhost;
  ghost->mShell = shell;
  //ghost->mUIRoot->mUI2DDrawData = uI2DDrawData;
  //ghost->mRenderView = mRenderView;
  ghost->mRenderView = mDesktopWindow->mRenderView;
  ghost->Init( errors );
  TAC_HANDLE_ERROR( errors );

  shell->AddSoul( ghost );
  mSoul = ghost;


  auto gameVis = new TacGameVis();
  gameVis->mDesktopWindow = mDesktopWindow;
  gameVis->mSoul = ghost;
  //gameVis->mRenderView = mRenderView;
  gameVis->mRenderView = mDesktopWindow->mRenderView;
  gameVis->uI2DDrawData = uI2DDrawData;
  gameVis->creationGameWindow = this;


  mUIRoot = new TacUIRoot;
  mUIRoot->mElapsedSeconds = &mShell->mElapsedSeconds;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mKeyboardInput = mShell->mKeyboardInput;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  mUIRoot->mHierarchyRoot->SetVisual( gameVis );
}
void TacCreationGameWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();
  //SetGhostRenderView();
}
//void TacCreationGameWindow::SetGhostRenderView()
//{
//  TacRenderer* renderer = mShell->mRenderer;
//
//  float innerBoxPercentOffsetX = 0.159f;
//  float innerBoxPercentOffsetY = 0.175f;
//  float innerBoxPercentWidth = 1 - ( 2 * innerBoxPercentOffsetX );
//  float innerBoxPercentHeight = 1 - ( 2 * innerBoxPercentOffsetY );
//
//  float innerBoxPixelOffsetY = innerBoxPercentOffsetX * ( float )mDesktopWindow->mWidth;
//  float innerBoxPixelOffsetX = innerBoxPercentOffsetY * ( float )mDesktopWindow->mHeight;
//  float innerBoxPixelWidth = innerBoxPercentWidth * ( float )mDesktopWindow->mWidth;
//  float innerBoxPixelHeight = innerBoxPercentHeight * ( float )mDesktopWindow->mHeight;
//  TacRenderView* mRenderView = mDesktopWindow->mRenderView;
//  TacTexture* outputColor = mRenderView->mFramebuffer;
//  TacDepthBuffer* outputDepth = mRenderView->mFramebufferDepth;
//
//  if( !outputColor || outputColor->myImage.mWidth != ( int )innerBoxPixelWidth )
//  {
//    if( outputColor )
//    {
//      renderer->RemoveRendererResource( outputColor );
//      renderer->RemoveRendererResource( outputDepth );
//      outputColor = nullptr;
//      outputDepth = nullptr;
//    }
//
//    TacImage image;
//    image.mWidth = ( int )innerBoxPixelWidth;
//    image.mHeight = ( int )innerBoxPixelHeight;
//    image.mFormat.mPerElementByteCount = 1;
//    image.mFormat.mElementCount = 4;
//    image.mFormat.mPerElementDataType = TacGraphicsType::unorm;
//    TacTextureData textureData;
//    textureData.access = TacAccess::Default;
//    textureData.binding = { TacBinding::RenderTarget, TacBinding::ShaderResource };
//    textureData.cpuAccess = {};
//    textureData.mName = "client view fbo";
//    textureData.mStackFrame = TAC_STACK_FRAME;
//    textureData.myImage = image;
//    TacErrors errors; // ???
//    renderer->AddTextureResource( &outputColor, textureData, errors );
//    TAC_HANDLE_ERROR( errors );
//
//    TacDepthBufferData depthBufferData;
//    depthBufferData.mName = "client view depth buffer";
//    depthBufferData.mStackFrame = TAC_STACK_FRAME;
//    depthBufferData.width = ( int )innerBoxPixelWidth;
//    depthBufferData.height = ( int )innerBoxPixelHeight;
//    renderer->AddDepthBuffer( &outputDepth, depthBufferData, errors );
//    TAC_HANDLE_ERROR( errors );
//
//    mRenderView->mFramebuffer = outputColor;
//    mRenderView->mFramebufferDepth = outputDepth;
//  }
//
//  mRenderView->mScissorRect.mXMaxRelUpperLeftCornerPixel = innerBoxPixelWidth;
//  mRenderView->mScissorRect.mYMaxRelUpperLeftCornerPixel = innerBoxPixelHeight;
//  mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight = innerBoxPixelWidth;
//  mRenderView->mViewportRect.mViewportPixelHeightIncreasingUp = innerBoxPixelHeight;
//}
