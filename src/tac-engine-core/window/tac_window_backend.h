// TODO: The purpose of this file is...

#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/containers/tac_array.h"

#include "tac-engine-core/window/tac_window_api.h"

namespace Tac::WindowBackend
{
  struct WindowState
  {
    String mName;
    v2i    mPos;
    v2i    mSize;
    bool   mShown;
  };

  inline const int kWindowCapacity = 100;

  //                          TODO
  // -------------------------------------------------------------
  // handle destroying and creating windows not having the same
  // id in the same frame
  // -------------------------------------------------------------

  using WindowStates = Array<WindowState, kWindowCapacity >;

  extern WindowStates sGameLogicCurr;

  void ApplyBegin();
  void SetWindowCreated( WindowHandle, const void*, StringView, v2i pos, v2i size );
  void SetWindowDestroyed( WindowHandle );
  void SetWindowIsVisible( WindowHandle, bool );
  void SetWindowSize( WindowHandle, v2i );
  void SetWindowPos( WindowHandle, v2i );
  void ApplyEnd();

  const void* GetNativeWindowHandle( WindowHandle );

  void UpdateGameLogicWindowStates();
}

