#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{
  TAC_DEFINE_HANDLE( DesktopWindowHandle );

  struct DesktopWindowRect
  {
    int mLeft;
    int mRight;
    int mBottom;
    int mTop;
  };

  struct DesktopWindowState
  {
    int                       mX = 0;
    int                       mY = 0;
    int                       mWidth = 0;
    int                       mHeight = 0;
    const void*               mNativeWindowHandle = nullptr;
  };

  static const int            kDesktopWindowCapacity = 10;
  DesktopWindowState*         GetDesktopWindowState( DesktopWindowHandle );
  void                        CenterWindow( int *x, int *y, int w, int h );
  bool                        IsWindowHovered( DesktopWindowHandle );
  void                        SetHoveredWindow( DesktopWindowHandle );
  DesktopWindowRect           GetDesktopWindowRectScreenspace( DesktopWindowHandle );
  DesktopWindowRect           GetDesktopWindowRectWindowspace( DesktopWindowHandle );
}

