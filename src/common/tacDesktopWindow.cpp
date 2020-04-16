
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"

namespace Tac
{
  DesktopWindow::DesktopWindow()
  {
    //mRenderView = new RenderView;
  }
  DesktopWindow::~DesktopWindow()
  {
    mOnDestroyed.EmitEvent( this );
    //delete mRenderView;
  }


  WindowParams::WindowParams()
  {
    mWidth = 800;
    mHeight = 600;
    mX = 50;
    mY = 50;
  }
  void WindowParams::GetCenteredPosition( int w, int h, int* x, int* y, Monitor monitor )
  {
    *x = ( monitor.w - w ) / 2;
    *y = ( monitor.h - h ) / 2;
  }

  void DesktopWindow::SetRenderViewDefaults()
  {
    //Texture* currentBackbufferTexture = nullptr;
    //mRendererData->GetCurrentBackbufferTexture( &currentBackbufferTexture );
    //TAC_ASSERT( currentBackbufferTexture );

    //ScissorRect scissorRect;
    //scissorRect.mXMaxRelUpperLeftCornerPixel = ( float )currentBackbufferTexture->myImage.mWidth;
    //scissorRect.mYMaxRelUpperLeftCornerPixel = ( float )currentBackbufferTexture->myImage.mHeight;

    //Viewport viewport;
    //viewport.mViewportPixelWidthIncreasingRight = ( float )currentBackbufferTexture->myImage.mWidth;
    //viewport.mViewportPixelHeightIncreasingUp = ( float )currentBackbufferTexture->myImage.mHeight;

    //mRenderView->mFramebuffer = currentBackbufferTexture;
    //mRenderView->mFramebufferDepth = mRendererData->mDepthBuffer;
    //mRenderView->mScissorRect = scissorRect;
    //mRenderView->mViewportRect = viewport;
  }

}

