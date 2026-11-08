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
    struct WindowState
    {
      WindowHandle::Gen       mGeneration          {}; // should we just put the whole WindowHandle here instead?
      String                  mName                {};
      v2i                     mPos                 {};
      v2i                     mSize                {};
      bool                    mShown               {};
      Render::SwapChainHandle mSwapChainHandle     {};
      NativeWindowHandle      mNativeWindowHandle  {};
    };

    struct WindowCreatedParams
    {
      NativeWindowHandle mNWH  {};
      StringView         mName {};
      v2i                mPos  {};
      v2i                mSize {};
    };

    static bool          sCreatesSwapChain;
    static WindowHandle  sAppHovered;

    // Handle desktop event queue on the platform thread
    static void OnWindowCreated( WindowCreatedParams, Errors& );
    static void OnWindowDestroyed( NativeWindowHandle );
    static void OnWindowSize( NativeWindowHandle, v2i, Errors& );

    //static void SetWindowPos( WindowHandle, v2i );
    //static void SetWindowHovered( WindowHandle );
    //static auto GetWindowPos( WindowHandle ) -> v2i;
    //static auto GetWindowNWH( WindowHandle ) -> NativeWindowHandle;
    //static void SetCreatesSwapChain( bool );

    static auto FindWindowHandleFromNWH( NativeWindowHandle ) -> WindowHandle;
    static auto FindWindowStateFromWH( WindowHandle ) -> WindowState*;
    static auto FindWindowStateFromNWH( NativeWindowHandle ) -> WindowState*;

    // Usage:
    //
    //   for( WindowHandle h : AppWindowApiBackend::WindowIterator() )
    //   {
    //     ...
    //   }
    //
    struct WindowIterator
    {
      auto begin() -> WindowIterator;
      auto end() -> WindowIterator;
      auto operator *() -> WindowHandle;
      void operator ++();
      bool operator == ( const WindowIterator& ) const = default;
      WindowHandle::Idx mCurrIdx{};
    };
  };
}

