#include "tac_desktop_window_move.h"

#include "tac-engine-core/window/tac_window_api.h"
#include "tac-engine-core/system/tac_platform.h"

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
    const WindowHandle WindowHandle = { i };
    if( !WindowHandle.GetDesktopWindowNativeHandle() )
      continue;

    const RequestMove* requestMove = &sRequestMove[ i ];
    if( !requestMove->mRequested )
      continue;

    const DesktopWindowRect desktopWindowRect = requestMove->mRect.IsEmpty()
      ?  WindowHandle.GetDesktopWindowRectWindowspace() 
      : requestMove->mRect;

    sPlatformFns->PlatformWindowMoveControls( WindowHandle, desktopWindowRect );
    sRequestMove[ i ] = RequestMove();
  }
}


void                Tac::DesktopAppImplMoveControls( const WindowHandle& WindowHandle,
                                                     const DesktopWindowRect& rect )
{
  sRequestMove[ WindowHandle.GetIndex() ] = RequestMove
  {
    .mRequested = true,
    .mRect = rect,
  };
}

void                Tac::DesktopAppImplMoveControls( const WindowHandle& WindowHandle )
{
  sRequestMove[ WindowHandle.GetIndex() ].mRequested = true;
}
