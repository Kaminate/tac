#include "common/tacDesktopWindow.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacRenderer.h"
#include "common/tacShell.h"

TacDesktopWindow::TacDesktopWindow()
{
  mRenderView = new TacRenderView;
}
TacDesktopWindow::~TacDesktopWindow()
{
  mOnDestroyed.EmitEvent();
  delete mRenderView;
}


TacWindowParams::TacWindowParams()
{
  mWidth = 800;
  mHeight = 600;
  mX = 50;
  mY = 50;
}
void TacWindowParams::GetCenteredPosition( int w, int h, int* x, int* y, TacMonitor monitor )
{
  *x = ( monitor.w - w ) / 2;
  *y = ( monitor.h - h ) / 2;
}

void TacDesktopWindow::SetRenderViewDefaults()
{
  TacTexture* currentBackbufferTexture = nullptr;
  mRendererData->GetCurrentBackbufferTexture( &currentBackbufferTexture );
  TacAssert( currentBackbufferTexture );

  TacScissorRect scissorRect;
  scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )currentBackbufferTexture->myImage.mWidth;
  scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )currentBackbufferTexture->myImage.mHeight;

  TacViewport viewport;
  viewport.mViewportPixelWidthIncreasingRight = ( float )currentBackbufferTexture->myImage.mWidth;
  viewport.mViewportPixelHeightIncreasingUp = ( float )currentBackbufferTexture->myImage.mHeight;

  TacRenderView* renderView = mRenderView;
  renderView->mFramebuffer = currentBackbufferTexture;
  renderView->mFramebufferDepth = mRendererData->mDepthBuffer;
  renderView->mScissorRect = scissorRect;
  renderView->mViewportRect = viewport;
}
