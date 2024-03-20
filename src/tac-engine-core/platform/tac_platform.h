#pragma once

#include "tac-engine-core/window/tac_window_api.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac { struct Errors; }
namespace Tac
{
  struct PlatformSpawnWindowParams
  {
    WindowHandle mHandle;
    StringView   mName;
    v2i          mPos;
    v2i          mSize;
  };

  // hmm... these fns should only be able to be called from the platform thread.
  // (                                                                 )
  // ( should PlatformFns::GetInstance() assert( isplatformthread() )? )
  // (                                                                 )
  struct PlatformFns
  {
    // debug imgui of the platform itself?
    virtual void PlatformImGui( Errors& ) {}

    virtual void PlatformFrameBegin( Errors& ) {}
    virtual void PlatformFrameEnd( Errors& ) {}
    virtual void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) {}
    virtual void PlatformDespawnWindow( WindowHandle ) {}
    virtual void PlatformSetWindowPos( WindowHandle, v2i ) {}
    virtual void PlatformSetWindowSize( WindowHandle, v2i ) {}
    //virtual void PlatformWindowMoveControls( const WindowHandle&,
    //                                         const DesktopWindowRect& ) {}
    //virtual void PlatformWindowResizeControls( const WindowHandle&, int ) {}
    //virtual WindowHandle PlatformGetMouseHoveredWindow() { return{}; }

    static PlatformFns* GetInstance();
    static void         SetInstance( PlatformFns* );
  };

} // namespace Tac
