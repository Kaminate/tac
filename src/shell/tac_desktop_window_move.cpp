#include "tac_desktop_window_move.h"

#include "src/common/system/tac_desktop_window.h"
#include "src/shell/tac_platform.h"

namespace Tac
{
  struct RequestMove
  {
    bool              mRequested = false;
    DesktopWindowRect mRect = {};
  };

  static RequestMove                   sRequestMove[ kDesktopWindowCapacity ];
}

void Tac::DesktopAppUpdateMove()
{
  PlatformFns* sPlatformFns = PlatformFns::GetInstance();

  for( int i = 0; i < kDesktopWindowCapacity; ++i )
  {
    const DesktopWindowHandle desktopWindowHandle = { i };
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      continue;

    const RequestMove* requestMove = &sRequestMove[ i ];
    if( !requestMove->mRequested )
      continue;

    const DesktopWindowRect desktopWindowRect = requestMove->mRect.IsEmpty()
      ? GetDesktopWindowRectWindowspace( desktopWindowHandle )
      : requestMove->mRect;

    sPlatformFns->PlatformWindowMoveControls( desktopWindowHandle, desktopWindowRect );
    sRequestMove[ i ] = RequestMove();
  }
}


void                Tac::DesktopAppImplMoveControls( const DesktopWindowHandle& desktopWindowHandle,
                                                     const DesktopWindowRect& rect )
{
  sRequestMove[ ( int )desktopWindowHandle ] = RequestMove
  {
    .mRequested = true,
    .mRect = rect,
  };
}

void                Tac::DesktopAppImplMoveControls( const DesktopWindowHandle& desktopWindowHandle )
{
  RequestMove* request = &sRequestMove[ ( int )desktopWindowHandle ];
  request->mRequested = true;
}
