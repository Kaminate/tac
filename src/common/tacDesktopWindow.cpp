#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"

namespace Tac
{
  // rename to g_
  DesktopWindowStates sDesktopWindowStates;

  //WindowParams::WindowParams()
  //{
  //  mWidth = 800;
  //  mHeight = 600;
  //  mX = 50;
  //  mY = 50;
  //}
  //void WindowParams::Center()
  //{
  //  Monitor monitor;
  //  OS::GetPrimaryMonitor( &monitor.w, &monitor.h );
  //  mX = ( monitor.w - mWidth ) / 2;
  //  mY = ( monitor.h - mHeight ) / 2;
  //}
  void CenterWindow( int *x, int *y, int w, int h )
  {
    int monitorW;
    int monitorH;
    OS::GetPrimaryMonitor( &monitorW, &monitorH );
    *x = ( monitorW - w ) / 2;
    *y = ( monitorH - h ) / 2;
  }

  bool AreWindowHandlesEqual( const DesktopWindowHandle& l, const DesktopWindowHandle& r )
  {
    return l.mIndex == r.mIndex;
  }

  bool IsWindowHandleValid( const DesktopWindowHandle& desktopWindowHandle )
  {
    return desktopWindowHandle.mIndex != -1;
  }

  bool operator == ( const DesktopWindowHandle& rhs, const DesktopWindowHandle& lhs )
  {
    return lhs.mIndex == rhs.mIndex;
  }
}

