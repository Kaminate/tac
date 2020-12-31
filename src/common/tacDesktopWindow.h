#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{
  TAC_DEFINE_HANDLE( DesktopWindowHandle );

  struct DesktopWindowRect
  {
    int  GetArea() const;
    bool IsEmpty() const;
    int  GetWidth() const;
    int  GetHeight() const;
    int  mLeft = 0;
    int  mRight = 0;
    int  mBottom = 0;
    int  mTop = 0;
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
  bool                        IsWindowHovered( DesktopWindowHandle );
  void                        SetHoveredWindow( DesktopWindowHandle );
  DesktopWindowRect           GetDesktopWindowRectScreenspace( DesktopWindowHandle );
  DesktopWindowRect           GetDesktopWindowRectWindowspace( DesktopWindowHandle );
}

