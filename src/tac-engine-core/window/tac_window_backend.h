// The purpose of this file is to coordinate/implement the synchronization of the window system
// between the game logic ("sim") and platform ("sys") threads.

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/containers/tac_array.h"

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac { struct Errors; }

namespace Tac
{
  inline const int kDesktopWindowCapacity { 100 };

  // +------------------------------------------------------------+
  // |                          TODO                              |
  // +------------------------------------------------------------+
  // | handle destroying and creating windows not having the same |
  // | id in the same frame                                       |
  // +------------------------------------------------------------+

  struct AppWindowApiBackend
  {
    // Handle desktop event queue on the platform thread
    static void SetWindowCreated( WindowHandle,
                                  const void*,
                                  StringView,
                                  v2i pos,
                                  v2i size,
                                  Errors& );
    static void SetWindowDestroyed( WindowHandle );
    static void SetWindowIsVisible( WindowHandle, bool );
    static void SetWindowSize( WindowHandle, v2i, Errors& );
    static void SetWindowPos( WindowHandle, v2i );
    static void SetWindowHovered( WindowHandle );
    static auto GetWindowPos( WindowHandle ) -> v2i;
    static void SetCreatesSwapChain( bool );
  };
}

