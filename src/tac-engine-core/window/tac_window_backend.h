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
  inline const int kDesktopWindowCapacity = 100;
}

namespace Tac::WindowBackend
{
#if 0
  struct WindowState
  {
    String mName;
    v2i    mPos;
    v2i    mSize;
    bool   mShown;
  };
#endif

  // +------------------------------------------------------------+
  // |                          TODO                              |
  // +------------------------------------------------------------+
  // | handle destroying and creating windows not having the same |
  // | id in the same frame                                       |
  // +------------------------------------------------------------+

#if 0
  using WindowStates = Array< WindowState, kDesktopWindowCapacity >;

  extern WindowStates sGameLogicCurr;
  extern WindowStates sPlatformCurr;
#endif

  struct SysApi
  {
    // Handle desktop event queue on the platform thread
    void ApplyBegin();
    void SetWindowCreated( WindowHandle, const void*, StringView, v2i pos, v2i size );
    void SetWindowDestroyed( WindowHandle );
    void SetWindowIsVisible( WindowHandle, bool );
    void SetWindowSize( WindowHandle, v2i );
    void SetWindowPos( WindowHandle, v2i );
    void ApplyEnd();

    // Synchronize sim-thread window requests
    void Sync( Errors& );
  };

  // Sim thread functions:

  struct SimApi
  {
    // Synchronize window updates from the sys thread
    void Sync();
  };
}

