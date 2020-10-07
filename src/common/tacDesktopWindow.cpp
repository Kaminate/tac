#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"

namespace Tac
{
  DesktopWindowStateCollection DesktopWindowStateCollection::InstanceStuffThread;

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
    Monitor monitor;
    OS::GetPrimaryMonitor( &monitor.w, &monitor.h );
    *x = ( monitor.w - w ) / 2;
    *y = ( monitor.h - h ) / 2;
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

  DesktopWindowState* DesktopWindowStateCollection::FindDesktopWindowState( DesktopWindowHandle desktopWindowHandle )
  {
    if( !IsWindowHandleValid( desktopWindowHandle ) )
      return nullptr;
    for( DesktopWindowState& state : mStates )
      if( state.mDesktopWindowHandle.mIndex == desktopWindowHandle.mIndex )
        return &state;
    return nullptr;
  }

  DesktopWindowState* DesktopWindowStateCollection::GetStateAtIndex( int i )
  {
    return &mStates[ i ];
  }
}

