#include "tacDesktopWindow.h"
#include "tacUI2D.h"
#include "tacUI.h"
#include "tacShell.h"

TacDesktopWindow::TacDesktopWindow()
{
}
TacDesktopWindow::~TacDesktopWindow()
{
  mOnDestroyed.EmitEvent();
  delete mRendererData;
  delete mMainWindowRenderView;
  //delete mUI2DDrawData;
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

