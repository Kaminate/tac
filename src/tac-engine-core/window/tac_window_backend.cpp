#include "tac_window_backend.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"

import std; // mutex

namespace Tac
{
  WindowBackend::WindowStates WindowBackend::sGameLogicCurr;
}

namespace Tac::WindowBackend
{
  using NativeArray = Array< const void*, kWindowCapacity >;

  static std::mutex   sMutex;
  static bool         sModificationAllowed;
  static WindowStates sPlatformCurr;
  static NativeArray  sPlatformNative;
}

namespace Tac
{
  const void* WindowBackend::GetNativeWindowHandle( WindowHandle h )
  {
    return sPlatformNative[ h.GetIndex() ];
  }

  void WindowBackend::ApplyBegin()
  {
    sMutex.lock();
    sModificationAllowed = true;
  }

  void WindowBackend::ApplyEnd()
  {
    sModificationAllowed = false;
    sMutex.unlock();
  }

  void WindowBackend::UpdateGameLogicWindowStates()
  {
    sMutex.lock();
    sGameLogicCurr = sPlatformCurr;
    sMutex.unlock();
  }

  void WindowBackend::SetWindowCreated( WindowHandle h,
                                        const void* native,
                                        StringView name,
                                        v2i pos,
                                        v2i size )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i = h.GetIndex();
    sPlatformCurr[ i ] = WindowState
    {
      .mName = name,
      .mPos = pos,
      .mSize = size,
      .mShown = false,
    };
    sPlatformNative[ i ] = native;
  }

  void WindowBackend::SetWindowDestroyed( WindowHandle h )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i = h.GetIndex();
    sPlatformCurr[ i ] = {};
    sPlatformNative[ i ] = {};
  }

  void WindowBackend::SetWindowIsVisible( WindowHandle h, bool shown )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mShown = shown;
  }

  void WindowBackend::SetWindowSize( WindowHandle h, v2i size )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mSize = size;
  }

  void WindowBackend::SetWindowPos( WindowHandle h, v2i pos )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mPos = pos;
  }
} // namespace Tac

