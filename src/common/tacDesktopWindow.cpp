#include "src/common/tacDesktopWindow.h"
#include "src/common/containers/tacArray.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"

namespace Tac
{
  static DesktopWindowState sDesktopWindowStates[ kDesktopWindowCapacity ];
  static DesktopWindowHandle sMouseHoveredWindowHandle;

  int  DesktopWindowRect::GetArea() const { return GetWidth() * GetHeight(); }
  bool DesktopWindowRect::IsEmpty() const { return GetArea() == 0; }
  int  DesktopWindowRect::GetWidth() const { return mRight - mLeft; }
  int  DesktopWindowRect::GetHeight() const { return mBottom - mTop; }


  bool                        IsWindowHovered( const DesktopWindowHandle desktopWindowHandle )
  {
    const bool result = sMouseHoveredWindowHandle == desktopWindowHandle;
    return result;
  }

  void                        SetHoveredWindow( const DesktopWindowHandle desktopWindowHandle )
  {
    sMouseHoveredWindowHandle = desktopWindowHandle;
  }

  void                        CenterWindow( int *x, int *y, int w, int h )
  {
    int monitorW;
    int monitorH;
    OS::GetPrimaryMonitor( &monitorW, &monitorH );
    *x = ( monitorW - w ) / 2;
    *y = ( monitorH - h ) / 2;
  }

  DesktopWindowState*         GetDesktopWindowState( const DesktopWindowHandle desktopWindowHandle )
  {
    return desktopWindowHandle.IsValid()
      ? &sDesktopWindowStates[ ( int )desktopWindowHandle ]
      : nullptr;
  }

  DesktopWindowRect           GetDesktopWindowRectScreenspace( DesktopWindowHandle desktopWindowHandle )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
    DesktopWindowRect desktopWindowRect;
    desktopWindowRect.mTop = desktopWindowState->mY;
    desktopWindowRect.mBottom = desktopWindowState->mY + desktopWindowState->mHeight;
    desktopWindowRect.mLeft = desktopWindowState->mX;
    desktopWindowRect.mRight = desktopWindowState->mX + desktopWindowState->mWidth;
    return desktopWindowRect;
  }

  DesktopWindowRect           GetDesktopWindowRectWindowspace( DesktopWindowHandle desktopWindowHandle )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
    DesktopWindowRect desktopWindowRect;
    desktopWindowRect.mTop = 0;
    desktopWindowRect.mBottom = desktopWindowState->mHeight;
    desktopWindowRect.mLeft = 0;
    desktopWindowRect.mRight = desktopWindowState->mWidth;
    return desktopWindowRect;

  }


}
