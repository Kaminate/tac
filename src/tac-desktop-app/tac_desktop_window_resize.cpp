#include "tac_desktop_window_resize.h"

#include "tac-std-lib/system/tac_desktop_window.h"
#include "src/shell/tac_platform.h"

namespace Tac
{
  struct RequestResize
  {
    bool              mRequested = false;
    int               mEdgePx = 0;
  };

  static RequestResize                 sRequestResize[ kDesktopWindowCapacity ];
}

void Tac::DesktopAppUpdateResize()
{
  PlatformFns* platform = PlatformFns::GetInstance();

  for( int i = 0; i < kDesktopWindowCapacity; ++i )
  {
    const DesktopWindowHandle desktopWindowHandle = { i };
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      continue;

    const RequestResize* requestResize = &sRequestResize[ i ];
    if( !requestResize->mRequested )
      continue;

    platform->PlatformWindowResizeControls( desktopWindowHandle, requestResize->mEdgePx );
    sRequestResize[ i ] = RequestResize();
  }
}

void Tac::DesktopAppImplResizeControls( const DesktopWindowHandle& desktopWindowHandle,
                                        int edgePx )
{
  sRequestResize[ ( int )desktopWindowHandle ] = RequestResize
  {
   .mRequested = true,
   .mEdgePx = edgePx,
  };
}
