#pragma once

#include "src/common/containers/tacArray.h"

namespace Tac
{
  struct DesktopWindowHandle
  {
    int mIndex = -1;
  };

  struct DesktopWindowState
  {
    int                 mX = 0;
    int                 mY = 0;
    int                 mWidth = 0;
    int                 mHeight = 0;
    void*               mNativeWindowHandle = nullptr;
    bool                mCursorUnobscured = false;
  };

  static const int kMaxDesktopWindowStateCount = 10;
  typedef Array< DesktopWindowState, kMaxDesktopWindowStateCount > DesktopWindowStates;
  extern DesktopWindowStates sDesktopWindowStates;

  bool AreWindowHandlesEqual( const DesktopWindowHandle&, const DesktopWindowHandle& );
  bool IsWindowHandleValid( const DesktopWindowHandle& );
  void CenterWindow( int *x, int *y, int w, int h );
}

