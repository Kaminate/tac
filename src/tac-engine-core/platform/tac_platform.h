#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
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

  enum class PlatformMouseCursor
  {
    kNone,
    kArrow,
    kResizeNS,
    kResizeEW,
    kResizeNE_SW,
    kResizeNW_SE,
    kCount,
  };

  // hmm... these fns should only be able to be called from the platform thread.
  // (                                                                 )
  // ( should PlatformFns::GetInstance() assert( isplatformthread() )? )
  // (                                                                 )
  struct PlatformFns
  {
    // debug imgui of the platform itself?
    virtual void PlatformImGui( Errors& ) const {}

    virtual void PlatformFrameBegin( Errors& ) const {}
    virtual void PlatformFrameEnd( Errors& ) const {}
    virtual void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) const {}
    virtual void PlatformDespawnWindow( WindowHandle ) const {}
    virtual void PlatformSetWindowPos( WindowHandle, v2i ) const {}
    virtual void PlatformSetWindowSize( WindowHandle, v2i ) const {}
    virtual void PlatformSetMouseCursor( PlatformMouseCursor ) const {}

    virtual WindowHandle PlatformGetMouseHoveredWindow() const { return{}; }

    static PlatformFns* GetInstance();
    static void         SetInstance( PlatformFns* );
  };

} // namespace Tac
