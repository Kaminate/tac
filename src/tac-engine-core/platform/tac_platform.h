#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/error/tac_error_handling.h"

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
    kResizeNSEW,
    kCount,
  };

  struct Platform
  {
    static void PlatformImGui( Errors& );
    static void PlatformFrameBegin( Errors& );
    static void PlatformFrameEnd( Errors& );
    static void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& );
    static void PlatformDespawnWindow( WindowHandle );
    static void PlatformMinimizeWindow( WindowHandle );
    static void PlatformSetWindowPos( WindowHandle, v2i );
    static void PlatformSetWindowSize( WindowHandle, v2i );
    static void PlatformSetMouseCursor( PlatformMouseCursor );
    static auto PlatformGetMouseHoveredWindow() -> WindowHandle;
  };

} // namespace Tac
