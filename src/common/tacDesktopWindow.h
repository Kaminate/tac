#pragma once

#include "src/common/tacPreprocessor.h"

namespace Tac
{
  TAC_DEFINE_HANDLE( DesktopWindowHandle );

  struct DesktopWindowState
  {
    int                       mX = 0;
    int                       mY = 0;
    int                       mWidth = 0;
    int                       mHeight = 0;
    void*                     mNativeWindowHandle = nullptr;
  };

  static const int            kDesktopWindowCapacity = 10;
  DesktopWindowState*         GetDesktopWindowState( DesktopWindowHandle );
  //DesktopWindowState*         GetDesktopWindowStates();
  void                        CenterWindow( int *x, int *y, int w, int h );
}

