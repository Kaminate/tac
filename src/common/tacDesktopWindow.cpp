#include "src/common/tacDesktopWindow.h"
#include "src/common/containers/tacArray.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacOS.h"

namespace Tac
{
  // DesktopWindowStates sDesktopWindowStates;
  // typedef Array< DesktopWindowState, kDesktopWindowCapacity > DesktopWindowStates;
  // extern DesktopWindowStates         sDesktopWindowStates;
  static DesktopWindowState sDesktopWindowStates[ kDesktopWindowCapacity ];

  void CenterWindow( int *x, int *y, int w, int h )
  {
    int monitorW;
    int monitorH;
    OS::GetPrimaryMonitor( &monitorW, &monitorH );
    *x = ( monitorW - w ) / 2;
    *y = ( monitorH - h ) / 2;
  }

  DesktopWindowState* GetDesktopWindowState( DesktopWindowHandle desktopWindowHandle)
  {
    return &sDesktopWindowStates[ (int)desktopWindowHandle ];
  }

  //bool AreWindowHandlesEqual( const DesktopWindowHandle& l, const DesktopWindowHandle& r )
  //{
  //  return l.mIndex == r.mIndex;
  //}

  //bool IsWindowHandleValid( const DesktopWindowHandle& desktopWindowHandle )
  //{
  //  return desktopWindowHandle.mIndex != -1;
  //}

  //bool operator == ( const DesktopWindowHandle& rhs, const DesktopWindowHandle& lhs )
  //{
  //  return lhs.mIndex == rhs.mIndex;
  //}
}

