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
    static auto PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) -> NativeWindowHandle;
    static void PlatformDespawnWindow( NativeWindowHandle );
    static void PlatformMinimizeWindow( NativeWindowHandle );
    static void PlatformSetWindowPos( NativeWindowHandle, v2i );
    static void PlatformSetWindowSize( NativeWindowHandle, v2i );
    static void PlatformSetMouseCursor( PlatformMouseCursor );
    static auto PlatformGetMouseHoveredWindow() -> WindowHandle;
  };

} // namespace Tac
