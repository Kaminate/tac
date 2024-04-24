#include "tac_desktop_window_resize.h"

//#include "tac-engine-core/window/tac_window_api.h"
#include "tac-engine-core/platform/tac_platform.h"

namespace Tac
{
  struct RequestResize
  {
    bool              mRequested {};
    int               mEdgePx    {};
  };

  //static RequestResize                 sRequestResize[ kDesktopWindowCapacity ];
}

void Tac::DesktopAppUpdateResize()
{
#if 0
  PlatformFns* platform = PlatformFns::GetInstance();

  for( int i{}; i < kDesktopWindowCapacity; ++i )
  {
    const WindowHandle windowHandle = { i };
    if( !windowHandle.GetDesktopWindowNativeHandle() )
      continue;

    const RequestResize* requestResize = &sRequestResize[ i ];
    if( !requestResize->mRequested )
      continue;

    platform->PlatformWindowResizeControls( windowHandle, requestResize->mEdgePx );
    sRequestResize[ i ] = RequestResize();
  }
#endif
}

void Tac::DesktopAppImplResizeControls( [[maybe_unused]] const WindowHandle& WindowHandle,
                                        [[maybe_unused]] int edgePx )
{
#if 0
  sRequestResize[ WindowHandle.GetIndex() ] = RequestResize
  {
   .mRequested { true },
   .mEdgePx { edgePx },
  };
#endif
}
